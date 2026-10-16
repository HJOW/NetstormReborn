// 실제 맨 오 워 풀의 현재 읽기 위치와 효과 이후 상태/시각 재조회를 보존한다.
#include "o/RawRainAviaryProcess.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kDirty=8,kTime=16,kStage=24,kForm=36,kParent=40; // outer와 inner의 실제 필드 오프셋이다.
constexpr double kFrameDelay=0.20000000298023224,kLaunchDelay=3.0; // PE005066e0/00501448의 실제 double 상수다.
// 비정렬 원문 정수를 호스트 엔디언에 의존하지 않고 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[at+i])<<(8*i);return value; } // 지정한 폭의 낮은 바이트부터 조합한다.
// 다른 payload를 건드리지 않고 실제 store 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i)); } // 원문 WORD/DWORD 폭을 보존한다.
// 선택되지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("맨 오 워 풀 실제 하위 소비자 미연결");return function; }
// 원본 signed 값의 덧셈은 낮은 DWORD에서 감긴다.
std::int32_t Add(std::int32_t value,int delta) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta)); }
// 실제 double 저장을 두 DWORD로 나누고 padding을 보존한다.
void StoreTime(RainAviaryProcessFields& fields,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,kTime,static_cast<std::uint32_t>(bits));Put(fields.bytes,kTime+4,static_cast<std::uint32_t>(bits>>32)); }
// 단계/타이머 변경의 각각 다른 원본 dirty 비트다.
void Stage(RainAviaryProcessFields& fields,std::uint32_t value) { fields.bytes[kDirty]|=4;Put(fields.bytes,kStage,value); }
// 이 타이머는 현재 시각을 저장한 뒤 같은 위치에서 현재 전역과 지연을 다시 더한다.
void SetTime(RainAviaryProcessFields& fields,const double& now,double delay) { fields.bytes[kDirty]|=2;StoreTime(fields,now);StoreTime(fields,now+delay); }
// 현재 원문 좌표의 IEEE 비트를 읽는다.
float Position(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
}
std::uint8_t RainAviaryProcessFields::Dirty() const { return bytes[kDirty]; } // 현재 변경 비트다.
double RainAviaryProcessFields::Time() const { return std::bit_cast<double>(std::uint64_t(Read(bytes,kTime))|(std::uint64_t(Read(bytes,kTime+4))<<32)); } // raw double을 그대로 읽는다.
std::uint32_t RainAviaryProcessFields::Stage() const { return Read(bytes,kStage); } // AP 상태 DWORD다.
Sid RainAviaryProcessFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; } // 현재 inner Form이다.
Sid RainAviaryProcessFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; } // 현재 inner 부모다.
RawRainAviaryProcess::RawRainAviaryProcess(SidPool& pool,RainAviaryProcessGlobals globals,RainAviaryProcessHooks hooks)
    :pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("맨 오 워 풀 프로세스는 PATCH만 복원했습니다"); } // 다른 판본 ABI를 임의 허용하지 않는다.
// 비서버는 state를 초기화하지 않는 원문과 unused padding을 보존한다.
void RawRainAviaryProcess::Initialize(RainAviaryProcessFields& fields) const {
    StoreTime(fields,0);fields.bytes[kDirty]=0xff;Put(fields.bytes,32,0x5019ec);Put(fields.bytes,kParent,0);Put(fields.bytes,kForm,0);
    Put(fields.bytes,0,0x509e20);Put(fields.bytes,32,0x509e00);
    if (globals_.server) { StoreTime(fields,globals_.now);fields.bytes[kDirty]|=4;Put(fields.bytes,kStage,1); }
}
// 실제 할당 실패도 NULL inner 부착 요청으로 전달한다. Host는 해당 비정상 경로를 명시 거부할 수 있다.
void RawRainAviaryProcess::PostPop(Sid parent,std::uint32_t flags) const {
    if (globals_.server && !(globals_.fort && !globals_.friendlyFire) && (flags&1U)) {
        if (Need(hooks_.findProcess)(parent,globals_.processType).value) Need(hooks_.assertion)(0x257);
        auto fields=Need(hooks_.allocate)(0x30);if (fields) Initialize(*fields);Need(hooks_.attach)(std::move(fields),parent,globals_.processType,8);
    }
    Need(hooks_.damageable)(parent,flags);
}
// 원본의 처음 부모/frame snapshot과 하위 효과 뒤 현재 Parent/시각/단계를 구분한다.
void RawRainAviaryProcess::Frame(RainAviaryProcessFields& fields) const {
    const auto parent=fields.Parent();const auto raw=pool_.Slot(parent);if ((raw[40]&0x20U) || globals_.editor) return;
    const auto stage=fields.Stage();const auto frame=std::bit_cast<std::int32_t>(Read(raw,36)); // 같은 부모의 진입 시 프레임이다.
    const auto due=[&] { return globals_.now>=fields.Time(); }; // 실제 ordered 비교로 기한 전/일치/후를 구분한다.
    switch (stage) {
    case 1:
        Need(hooks_.setFrame)(parent,0);Stage(fields,2);Need(hooks_.sound)(Position(raw,14),Position(raw,18),"manOWarBuild.wav");return;
    case 2:
        if (frame>11) { Stage(fields,6);return; }if (!due()) return;
        if (globals_.checking) Need(hooks_.checkFrame)(parent,Add(frame,1),0x7fa);
        Need(hooks_.setFrame)(parent,Add(frame,1));SetTime(fields,globals_.now,kFrameDelay);return;
    case 6:
        if (globals_.server) {
            const auto flyer=Need(hooks_.create)(globals_.flyerType,0);Put(pool_.AllocatedBytes(flyer),36,0);
            Need(hooks_.setOwner)(flyer,raw[34]);const auto current=fields.Parent(); // Owner 효과 뒤 실제 Form의 부모를 다시 읽는다.
            if (!Read(pool_.Slot(current),6,2)) Need(hooks_.assertion)(0xa4);
            Put(pool_.AllocatedBytes(current),12,flyer.value,2);
            const auto x=static_cast<float>(static_cast<double>(Position(raw,14))-0.5); // 실제00500ee4의binary32 0.5다.
            const auto y=static_cast<float>(static_cast<double>(Position(raw,18))-2.0); // 실제00501444의binary32 2.0이다.
            Put(pool_.AllocatedBytes(flyer),12,fields.Parent().value,2);Need(hooks_.pop)(flyer,x,y,0);
            Need(hooks_.bindFlyer)(flyer,fields.Parent());
        }
        Stage(fields,5);Need(hooks_.setFrame)(parent,0);SetTime(fields,globals_.now,kLaunchDelay);return;
    case 10:if (due()) Stage(fields,2);return;
    default:if (globals_.checking && (stage<1 || stage>10)) Need(hooks_.assertion)(0x820);return;
    }
}
// 0045d870의 현재 inner 인자 선택과0045b2a0의 BYTE/WORD/DWORD 쓰기를 결합한다.
std::uint16_t RawRainAviaryProcess::WriteProcess(RainAviaryProcessFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message) {
    const auto parent=(includeParent || force) ? fields.Parent().value : 0;const auto dirty=force ? 0xffU : fields.Dirty();const auto length=9U+(parent ? 4U : 0U)+((dirty&2U) ? 8U : 0U)+((dirty&4U) ? 4U : 0U);
    if (packet.size()<length) throw std::out_of_range("맨 오 워 풀 패킷 버퍼 크기 오류");if (force) fields.bytes[kDirty]=0xff;
    packet[0]=protocol;packet[1]=message;Put(packet,4,fields.Form().value);std::size_t at=9;
    if (parent) { fields.bytes[kDirty]|=1;Put(packet,at,parent);at+=4; }
    if (fields.Dirty()&2U) { std::copy_n(fields.bytes.begin()+kTime,8,packet.begin()+static_cast<std::ptrdiff_t>(at));at+=8; }
    if (fields.Dirty()&4U) { Put(packet,at,fields.Stage());at+=4; }
    Put(packet,2,static_cast<std::uint32_t>(at),2);packet[8]=fields.Dirty();fields.bytes[kDirty]=0;return static_cast<std::uint16_t>(at);
}
const SidPool& RawRainAviaryProcess::Pool() const { return pool_; } // Host가 같은 풀인지 확인한다.
RainAviaryProcess::RainAviaryProcess(const RawRainAviaryProcess& logic,RainAviaryProcessFields fields):logic_(logic),fields_(std::move(fields)) {} // 원문 이미지의 소유권을 받는다.
void RainAviaryProcess::RunFrame() { logic_.Frame(fields_); } // 실제 Kernel 프레임이다.
void RainAviaryProcess::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value); } // Host의 실제 부착 쓰기를 반영한다.
void RainAviaryProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {} // 원본 inner+1c는00480260의 ret12 빈 몸체다.
RainAviaryProcessFields& RainAviaryProcess::Fields() { return fields_; } // 같은 원문을 수정한다.
const RainAviaryProcessFields& RainAviaryProcess::Fields() const { return fields_; } // 같은 원문을 읽는다.
std::uint16_t RainAviaryProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force) { OnAttached();return RawRainAviaryProcess::WriteProcess(fields_,packet,parentChanged,force); } // 원본 writer 호출은 OnAttached보다 앞서기도 한다.
}
