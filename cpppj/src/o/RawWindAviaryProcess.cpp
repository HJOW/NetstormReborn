// Wind의 임시 플라이어 표시와 실제 비행 시작을 구별하고 현재 필드 재조회를 보존한다.
#include "o/RawWindAviaryProcess.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택한 원문 DWORD만 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[at+i])<<(8*i);return value; } // 네 바이트 순회를 사용한다.
// 지정한 폭만 쓰고 미정 padding은 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint64_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i)); } // 필요한 BYTE/WORD/DWORD/DOUBLE 폭이다.
// raw double을 계산 없이 읽는다.
double Time(const WindAviaryProcessFields& f,std::size_t at) { return std::bit_cast<double>(std::uint64_t(Read(f.bytes,at))|(std::uint64_t(Read(f.bytes,at+4))<<32)); }
// 이 저장 자체에는 dirty를 추가하지 않는다. Frame의 예약double 쓰기도 같은 규칙이다.
void Time(WindAviaryProcessFields& f,std::size_t at,double value) { Put(f.bytes,at,std::bit_cast<std::uint64_t>(value),8); }
// 상태/임시 플라이어의 각각 다른 변경 비트를 기록한다.
void State(WindAviaryProcessFields& f,std::uint32_t value) { f.bytes[8]|=4;Put(f.bytes,24,value); }
// 현재 플라이어 DWORD를 원본 dirty20으로 갱신한다.
void Flyer(WindAviaryProcessFields& f,std::uint32_t value) { f.bytes[8]|=0x20;Put(f.bytes,48,value); }
// 현재 inner 부모의 하위WORD를 읽는다.
Sid Parent(const WindAviaryProcessFields& f) { return Sid{static_cast<std::uint16_t>(Read(f.bytes,64))}; }
// 현재 임시 플라이어의 하위WORD를 읽는다.
Sid Flyer(const WindAviaryProcessFields& f) { return Sid{static_cast<std::uint16_t>(Read(f.bytes,48))}; }
// 원문 float 위치를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
// 선택하지 않은 경계는 요구하지 않고 선택된 누락은 앞선 쓰기를 유지하며 거부한다.
template<class Function> const Function& Need(const Function& fn) { if (!fn) throw std::logic_error("데빌메이커 실제 하위 효과 미연결");return fn; }
}
RawWindAviaryProcess::RawWindAviaryProcess(SidPool& pool,WindAviaryGlobals globals,WindAviaryHooks hooks):pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("데빌메이커는 PATCH만 복원했습니다"); } // 다른 판본 ABI를 허용하지 않는다.
// 중복 조회 이후 현재 processType/server를 생성자/부착 시점에 다시 읽는다.
void RawWindAviaryProcess::PostPop(Sid parent,std::uint32_t flags) const {
    if ((flags&1U) && !(globals_.fort && !globals_.friendlyFire) && globals_.server) {
        if (Need(hooks_.findProcess)(parent,globals_.processType).value) Need(hooks_.assertion)(0x275);
        auto fields=Need(hooks_.allocate)();if (fields) RawWindAviaryState::Initialize(*fields,globals_.server,globals_.now);Need(hooks_.attach)(std::move(fields),parent,globals_.processType);
    }
    Need(hooks_.damageable)(parent,flags);
}
// 원문45b680에는 난수나 대기 시각 변경이 없다.
void RawWindAviaryProcess::FlyerDied(WindAviaryProcessFields& fields,const std::function<void(WindAviaryProcessFields&)>& refresh) const { if (!globals_.server) return;State(fields,2);Need(refresh)(fields); }
// 원본은 임시DWORD를0으로 지우거나 free1을 검사하지 않으며 진단 뒤에도 처음 구한 포인터를 사용한다.
void RawWindAviaryProcess::FormDestroy(WindAviaryProcessFields& fields) const {
    if (!globals_.boss) return;const auto stored=Read(fields.bytes,48);if (!stored) return; // +48은 WORD가 아닌 현재 DWORD다.
    const auto offset=stored*50U; // 원본32비트 주소 곱셈의 감김을 보존한다. 상위bit31 별칭도 같은 주소다.
    if (offset<50U || offset>=pool_.Capacity()*50U) return;
    if (offset%50U) throw std::logic_error("데빌메이커 임시 포인터가 슬롯 경계 밖입니다"); // 손상된 비정렬 포인터의 임의 가상 호출은 지원하지 않는다.
    const Sid sid{static_cast<std::uint16_t>(offset/50U)};const auto raw=pool_.Slot(sid); // guard를 통과한 뒤에만 현재 슬롯을 읽는다.
    if (raw[11]&2U) return;if (raw[10]!=globals_.flyerType) Need(hooks_.assertion)(0x8e4);Need(hooks_.destroy)(sid,0);
}
// 원문은 진입 부모를 보존하면서 생성/연결의 일부 인수만 현재 필드에서 다시 읽는다.
void RawWindAviaryProcess::Frame(WindAviaryProcessFields& f) const {
    const auto parent=Parent(f);const auto raw=pool_.Slot(parent);if ((raw[40]&0x20U) || globals_.editor) return;
    if (Read(f.bytes,24)==1 && raw[10]!=globals_.aviaryType) State(f,11);
    const auto stage=Read(f.bytes,24);
    // 새 실제 플라이어는 직접frame13을 쓰고 소유자를 먼저 지정한다.
    const auto create=[&] { const auto sid=Need(hooks_.create)(globals_.flyerType);Flyer(f,sid.value);Put(pool_.AllocatedBytes(sid),36,13);Need(hooks_.setOwner)(sid,raw[34]);return sid; };
    switch (stage) {
    case 1:
        f.bytes[8]|=2;Time(f,16,globals_.now);Time(f,16,globals_.now+1.0);State(f,2);Time(f,32,globals_.now);f.bytes[8]|=0x30;Time(f,40,globals_.now);Put(f.bytes,48,0);return;
    case 2:case 7: {
        const auto elapsed=static_cast<float>(globals_.now-Time(f,40));double delay{};auto current=stage; // 경과만float로 저장하고 x87의 후속 곱/덧셈은double까지 유지한다.
        if (Time(f,32)!=0 && Time(f,32)<=globals_.now) {
            const auto next=std::bit_cast<std::int32_t>(Read(raw,36)+1U)%6;if (globals_.checking) Need(hooks_.checkFrame)(parent,next,0x884);Need(hooks_.setFrame)(parent,next);
            current=Read(f.bytes,24);delay=.3f;
            if (current==7 || elapsed<2.0f) {
                const auto delta=current==7 ? static_cast<double>(elapsed) : static_cast<double>(elapsed)-2.0;const auto scaled=delta*delta*static_cast<double>(.05f);
                delay=scaled>0 && scaled>.3f ? static_cast<double>(.3f) : scaled<=0 ? 0.0 : scaled;
            } else delay=0;
            Time(f,32,globals_.now+delay);
        }
        if (current==7) { if (!(delay<.3f)) State(f,5);return; }
        if (Read(f.bytes,48)!=0 && elapsed<6.0f) {
            const auto converted=static_cast<std::int32_t>((static_cast<double>(elapsed)-4.0)*-4.5);const auto frame=std::clamp(13-converted,13,21);const auto sid=Flyer(f);if (globals_.checking) Need(hooks_.checkFrame)(sid,frame,0x8a6);Need(hooks_.setFrame)(sid,frame); // 004e49c0은 카운터가 아닌 x87의0방향 정수 절삭이다.
        }
        if ((Read(f.bytes,48)!=0 || globals_.boss==0) && elapsed>6.0f) {
            if (globals_.server) { const auto sid=Flyer(f);Need(hooks_.createFlyerProcess)(sid,Parent(f));Need(hooks_.bindFlyer)(Flyer(f),Parent(f));Flyer(f,0); }
            f.bytes[8]|=0x10;Time(f,40,globals_.now);State(f,7);
        }
        if (globals_.server && elapsed>4.0f && Read(f.bytes,48)==0) {
            const auto sid=create();const auto actual=Parent(f);if (!pool_.Slot(actual)[6] && !pool_.Slot(actual)[7]) Need(hooks_.assertion)(0xa4);
            Put(pool_.AllocatedBytes(actual),12,sid.value,2);const auto x=Float(raw,14),y=Float(raw,18);const auto height=Need(hooks_.height)(parent,std::bit_cast<std::int32_t>(Read(raw,36)));const auto foot=Need(hooks_.footY)(parent);
            const auto py=static_cast<float>((static_cast<double>(height)*.09090909361839294-static_cast<double>(foot))+y);Put(pool_.AllocatedBytes(sid),12,Parent(f).value,2);Need(hooks_.pop)(sid,static_cast<float>(static_cast<double>(x)-1.399999976158142),py);
        }
        return;
    }
    case 5:if (globals_.now-Time(f,40)>26.0) State(f,1);return;
    case 11:f.bytes[8]|=2;Time(f,16,globals_.now);State(f,12);Flyer(f,0);return;
    case 12:
        if (globals_.server && Read(f.bytes,48)==0) { const auto sid=create();const auto pos=Need(hooks_.circlePosition)(parent,std::bit_cast<std::int32_t>(Read(f.bytes,68)),std::bit_cast<std::int32_t>(Read(f.bytes,72)));Put(pool_.AllocatedBytes(sid),12,Parent(f).value,2);Need(hooks_.pop)(sid,pos[0],pos[1]); }
        if (Read(f.bytes,48)!=0 || globals_.boss==0) { if (globals_.server) { Need(hooks_.createFlyerProcess)(Flyer(f),Parent(f));Flyer(f,0); }State(f,13); }return;
    case 13:Need(hooks_.finish)(f);return;
    default:if (globals_.checking && (stage<1 || stage>13)) Need(hooks_.assertion)(0x8da);return;
    }
}
WindAviaryProcess::WindAviaryProcess(const RawWindAviaryProcess& logic,WindAviaryProcessFields fields):logic_(logic),fields_(std::move(fields)) {} // 같은 초기80BYTE를 소유한다.
void WindAviaryProcess::RunFrame() { logic_.Frame(fields_); } // 실제 Kernel 호출이다.
void WindAviaryProcess::OnAttached() { Put(fields_.bytes,60,Form().value);Put(fields_.bytes,64,Parent().value); } // 실제 inner 부착 필드다.
void WindAviaryProcess::OnFormDestroy(std::uint32_t) { logic_.FormDestroy(fields_); } // Form 삭제 전에 임시 객체만 정리하고 프로세스 반납은 Host에 맡긴다.
void WindAviaryProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {} // 원본 빈 통지 몸체다.
WindAviaryProcessFields& WindAviaryProcess::Fields() { return fields_; } // 현재 단일 원문이다.
std::uint16_t WindAviaryProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged) { OnAttached();return RawWindAviaryState::WriteProcess(fields_,packet,parentChanged); } // 부착 전 전송도 같은 Host 필드를 사용한다.
}
