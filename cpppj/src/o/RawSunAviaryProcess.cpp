// 실제 훨리베이스의 현재 읽기 위치와 효과 이후 상태/시각 재조회를 보존한다.
#include "o/RawSunAviaryProcess.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kDirty=8,kTime=16,kStage=24,kForm=36,kParent=40,kCounter=44; // outer와 inner의 실제 필드 오프셋이다.
constexpr double kFrameDelay=0.20000000298023224,kLaunchDelay=3.0; // PE005066e0/00501448의 실제 double 상수다.
// 비정렬 원문 정수를 호스트 엔디언에 의존하지 않고 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[at+i])<<(8*i);return value; } // 지정한 폭의 낮은 바이트부터 조합한다.
// 다른 payload를 건드리지 않고 실제 store 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i)); } // 원문 WORD/DWORD 폭을 보존한다.
// 선택되지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("훨리베이스 실제 하위 소비자 미연결");return function; }
// 원본 signed 값의 덧셈은 낮은 DWORD에서 감긴다.
std::int32_t Add(std::int32_t value,int delta) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta)); }
// 실제 double 저장을 두 DWORD로 나누고 padding을 보존한다.
void StoreTime(SunAviaryProcessFields& fields,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,kTime,static_cast<std::uint32_t>(bits));Put(fields.bytes,kTime+4,static_cast<std::uint32_t>(bits>>32)); }
// 단계/타이머 변경의 각각 다른 원본 dirty 비트다.
void Stage(SunAviaryProcessFields& fields,std::uint32_t value) { fields.bytes[kDirty]|=4;Put(fields.bytes,kStage,value); }
// 이 타이머는 현재 시각을 저장한 뒤 같은 위치에서 현재 전역과 지연을 다시 더한다.
void SetTime(SunAviaryProcessFields& fields,const double& now,double delay) { fields.bytes[kDirty]|=2;StoreTime(fields,now);StoreTime(fields,now+delay); }
// 현재 원문 좌표의 IEEE 비트를 읽는다.
float Position(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
}
std::uint8_t SunAviaryProcessFields::Dirty() const { return bytes[kDirty]; } // 현재 변경 비트다.
double SunAviaryProcessFields::Time() const { return std::bit_cast<double>(std::uint64_t(Read(bytes,kTime))|(std::uint64_t(Read(bytes,kTime+4))<<32)); } // raw double을 그대로 읽는다.
std::uint32_t SunAviaryProcessFields::Stage() const { return Read(bytes,kStage); } // AP 상태 DWORD다.
std::uint32_t SunAviaryProcessFields::Counter() const { return Read(bytes,kCounter); } // signed 비교와 별개인 원문 DWORD다.
Sid SunAviaryProcessFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; } // 현재 inner Form이다.
Sid SunAviaryProcessFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; } // 현재 inner 부모다.
// Native helper의 첫 일치 순서를 유지하고 손상된 무한 체인은 풀 크기로 제한한다.
Sid FindAttachedProcess(const SidPool& pool,Sid parent,std::uint32_t type) {
    auto value=Read(pool.Slot(parent),6,2);std::uint32_t visited{}; // 현재 부모 머리와 순환 보호 횟수다.
    while (value) { // 다음 링크는 현재 후보를 검사한 뒤 읽는다.
        if (value>=pool.Capacity() || ++visited>=pool.Capacity()) throw std::logic_error("현재 프로세스 Form 체인 오류");
        const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=pool.Slot(sid);if (raw[10]==type && !(raw[11]&2U)) return sid;value=Read(raw,4,2);
    }
    return {};
}
RawSunAviaryProcess::RawSunAviaryProcess(SidPool& pool,SunAviaryProcessGlobals globals,SunAviaryProcessHooks hooks)
    :pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("훨리베이스 프로세스는 PATCH만 복원했습니다"); } // 다른 판본 ABI를 임의 허용하지 않는다.
// 비서버는 state를 초기화하지 않는 원문과 unused padding을 보존한다.
void RawSunAviaryProcess::Initialize(SunAviaryProcessFields& fields) const {
    StoreTime(fields,0);fields.bytes[kDirty]=0xff;Put(fields.bytes,32,0x5019ec);Put(fields.bytes,kParent,0);Put(fields.bytes,kForm,0);
    Put(fields.bytes,0,0x509dd8);Put(fields.bytes,32,0x509db8);Put(fields.bytes,kCounter,0);
    if (globals_.server) { StoreTime(fields,globals_.now);fields.bytes[kDirty]|=4;Put(fields.bytes,kStage,1); }
}
// 실제 할당 실패도 NULL inner 부착 요청으로 전달한다. Host는 해당 비정상 경로를 명시 거부할 수 있다.
void RawSunAviaryProcess::PostPop(Sid parent,std::uint32_t flags) const {
    if (globals_.server && (flags&1U)) {
        if (Need(hooks_.findProcess)(parent,globals_.processType).value) Need(hooks_.assertion)(0x1e2);
        auto fields=Need(hooks_.allocate)(0x30);if (fields) Initialize(*fields);Need(hooks_.attach)(std::move(fields),parent,globals_.processType,8);
    }
    Need(hooks_.damageable)(parent,flags);
}
// stage2/4의 frame은 진입 시 읽으며 호출 뒤 단계/횟수/시각만 원본 위치에서 다시 읽는다.
void RawSunAviaryProcess::Frame(SunAviaryProcessFields& fields) const {
    const auto parent=fields.Parent();const auto raw=pool_.Slot(parent);if ((raw[40]&0x20U) || globals_.editor) return;
    const auto stage=fields.Stage();const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));
    const auto due=[&] { return globals_.now>=fields.Time(); }; // 원본 ordered x87 비교이며 unordered는 거짓이다.
    const auto sound=[&] { Need(hooks_.sound)(Position(raw,14),Position(raw,18),"apStart.wav"); }; // 첫 부모 포인터의 현재 좌표다.
    switch (stage) {
    case 1:Need(hooks_.setFrame)(parent,27);Stage(fields,8);sound();return;
    case 2:
        if (frame>25) { Stage(fields,6);return; }if (!due()) return;
        if (globals_.checking) Need(hooks_.checkFrame)(parent,Add(frame,1),0x724);
        Need(hooks_.setFrame)(parent,Add(frame,1));SetTime(fields,globals_.now,kFrameDelay);return;
    case 3:if (due()) Stage(fields,4);return;
    case 4:
        if (frame<1) { Stage(fields,5);return; }if (!due()) return;
        if (globals_.checking) Need(hooks_.checkFrame)(parent,Add(frame,-1),0x751);
        Need(hooks_.setFrame)(parent,Add(frame,-1));SetTime(fields,globals_.now,kFrameDelay);return;
    case 6:
        if (globals_.server) {
            const auto flyer=Need(hooks_.create)(globals_.flyerType,0);Put(pool_.AllocatedBytes(flyer),36,0);
            Need(hooks_.setOwner)(flyer,raw[34]);const auto current=fields.Parent();
            if (!Read(pool_.Slot(current),6,2)) Need(hooks_.assertion)(0xa4);
            Put(pool_.AllocatedBytes(current),12,flyer.value,2);
            const auto x=static_cast<float>(static_cast<double>(Position(raw,14))+static_cast<double>(0.44999998807907104F));
            const auto y=static_cast<float>(static_cast<double>(Position(raw,18))-3.0);
            Put(pool_.AllocatedBytes(flyer),12,fields.Parent().value,2);Need(hooks_.pop)(flyer,x,y,0);
            Need(hooks_.bindFlyer)(flyer,fields.Parent());Stage(fields,3);SetTime(fields,globals_.now,kLaunchDelay);Need(hooks_.refreshForm)(fields);
        }
        return;
    case 8:case 9: {
        if (!due()) return;auto next=frame; // 원문 진입 frame과 별도의 계산 결과다.
        if (!(fields.Counter()&1U)) {
            if (frame>29) { Put(fields.bytes,kCounter,fields.Counter()+1U);next=Add(frame,-1); }else next=Add(frame,1);
        } else if (frame>27) next=Add(frame,-1);
        else { if (fields.Stage()==8) sound();Put(fields.bytes,kCounter,fields.Counter()+1U);next=Add(frame,1); }
        Need(hooks_.setFrame)(parent,next);
        if (std::bit_cast<std::int32_t>(fields.Counter())>9) {
            Put(fields.bytes,kCounter,0);if (fields.Stage()==8) { Need(hooks_.setFrame)(parent,11);Put(fields.bytes,kStage,2); }else Put(fields.bytes,kStage,5);fields.bytes[kDirty]|=4;
        }
        SetTime(fields,globals_.now,kFrameDelay);return;
    }
    default:if (globals_.checking && (stage<1 || stage>10)) Need(hooks_.assertion)(0x75e);return;
    }
}
// 0045d6e0의 현재 inner 인자 선택과0045b070의 BYTE/WORD/DWORD 쓰기를 결합한다.
std::uint16_t RawSunAviaryProcess::WriteProcess(SunAviaryProcessFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message) {
    const auto parent=(includeParent || force) ? fields.Parent().value : 0;const auto dirty=force ? 0xffU : fields.Dirty();const auto length=9U+(parent ? 4U : 0U)+((dirty&2U) ? 8U : 0U)+((dirty&4U) ? 4U : 0U);
    if (packet.size()<length) throw std::out_of_range("훨리베이스 패킷 버퍼 크기 오류");if (force) fields.bytes[kDirty]=0xff;
    packet[0]=protocol;packet[1]=message;Put(packet,4,fields.Form().value);std::size_t at=9;
    if (parent) { fields.bytes[kDirty]|=1;Put(packet,at,parent);at+=4; }
    if (fields.Dirty()&2U) { std::copy_n(fields.bytes.begin()+kTime,8,packet.begin()+static_cast<std::ptrdiff_t>(at));at+=8; }
    if (fields.Dirty()&4U) { Put(packet,at,fields.Stage());at+=4; }
    Put(packet,2,static_cast<std::uint32_t>(at),2);packet[8]=fields.Dirty();fields.bytes[kDirty]=0;return static_cast<std::uint16_t>(at);
}
const SidPool& RawSunAviaryProcess::Pool() const { return pool_; } // Host가 같은 풀인지 확인한다.
SunAviaryProcess::SunAviaryProcess(const RawSunAviaryProcess& logic,SunAviaryProcessFields fields):logic_(logic),fields_(std::move(fields)) {} // 원문 이미지의 소유권을 받는다.
void SunAviaryProcess::RunFrame() { logic_.Frame(fields_); } // 실제 Kernel 프레임이다.
void SunAviaryProcess::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value); } // Host의 실제 부착 쓰기를 반영한다.
void SunAviaryProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {} // 원본 inner+1c는00480260의 ret12 빈 몸체다.
SunAviaryProcessFields& SunAviaryProcess::Fields() { return fields_; } // 같은 원문을 수정한다.
const SunAviaryProcessFields& SunAviaryProcess::Fields() const { return fields_; } // 같은 원문을 읽는다.
std::uint16_t SunAviaryProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force) { OnAttached();return RawSunAviaryProcess::WriteProcess(fields_,packet,parentChanged,force); } // 원본 writer 호출은 OnAttached보다 앞서기도 한다.
}
