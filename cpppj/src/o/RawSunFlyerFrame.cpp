// 원본 float 저장 위치와 현재 필드 재조회·가상 효과 호출 순서를 보존한다.
#include "o/RawSunFlyerFrame.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 outer 기준의 선택 필드 위치다. 기존112바이트와 꼬리24바이트를 하나의 원문으로 다룬다.
constexpr std::size_t kBomb=96,kDropTime=104,kNextDrop=112,kNextFrame=120,kSavedX=128,kSavedY=132;
// 보호 PE가 공급하는 장식 주기/역수·폭발 프레임 간격·본체 복귀량이다.
constexpr double kPeriod=0.14535599946975708,kInversePeriod=6.8796610832214355,kExplosionDelay=0.03999999910593033;
constexpr float kRestore=0.05000000074505806f;
// 비정렬 원본 슬롯의 DWORD만 읽는다.
std::uint32_t RawRead(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value{};
    // 원본 little endian 네 바이트다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[at+index])<<(8*index);return value;
}
// 원본 슬롯의 DWORD만 바꾸며 인접 필드는 유지한다.
void RawWrite(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) {
    // 현재 네 바이트의 낮은 자리부터 쓴다.
    for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 분리한 두 배열을 넘는 호스트 포인터 산술 없이 실제136바이트 필드를 읽는다.
std::uint32_t Read(const FlyerProcessFields& fields,std::size_t at) {
    if (at+4>136) throw std::out_of_range("훨리기그 필드 범위 오류");std::uint32_t value{};
    // 각 바이트가 속한 원본 영역을 선택한다.
    for (std::size_t index=0;index<4;++index) { const auto offset=at+index;value|=std::uint32_t(offset<112 ? fields.bytes[offset] : fields.sunTail[offset-112])<<(8*index); }return value;
}
// 원본 DWORD 폭만 선택 영역에 쓴다.
void Write(FlyerProcessFields& fields,std::size_t at,std::uint32_t value) {
    if (at+4>136) throw std::out_of_range("훨리기그 필드 범위 오류");
    // 두 저장소를 이어진 포인터로 취급하지 않는다.
    for (std::size_t index=0;index<4;++index) { const auto offset=at+index;auto& byte=offset<112 ? fields.bytes[offset] : fields.sunTail[offset-112];byte=static_cast<std::uint8_t>(value>>(8*index)); }
}
// 실제 두 DWORD의 double 저장 비트를 읽는다.
double Time(const FlyerProcessFields& fields,std::size_t at) { return std::bit_cast<double>(std::uint64_t(Read(fields,at))|(std::uint64_t(Read(fields,at+4))<<32)); }
// 원본 double 저장만 수행한다.
void Time(FlyerProcessFields& fields,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Write(fields,at,static_cast<std::uint32_t>(bits));Write(fields,at+4,static_cast<std::uint32_t>(bits>>32)); }
// 현재 raw 위치의 binary32 성분이다.
float Position(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(RawRead(raw,at)); }
// 실제 DWORD 폭의 폭탄 번호를 호스트 SID 범위로 확인한다.
Sid Bomb(const FlyerProcessFields& fields) { const auto value=Read(fields,kBomb);if (value>65535) throw std::out_of_range("훨리기그 폭탄 SID 범위 오류");return {static_cast<std::uint16_t>(value)}; }
// 원본 signed 프레임 증가의 DWORD 감김이다.
std::int32_t Increment(std::int32_t value) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+1U); }
// 원본 idiv 정상 입력을 실행하며0/오버플로 경로를 명시 오류로 남긴다.
std::int32_t Remainder(std::int32_t value,std::int32_t divisor) {
    if (!divisor || (value==INT32_MIN && divisor==-1)) throw std::domain_error("훨리기그 원본 idiv 무효 경로");return value%divisor;
}
// 선택된 경계만 실제 호출 직전에 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("훨리기그 실제 하위 몸체 미연결");return function; }
}
// 다른 판본/타입 번호/셰이프 자료를 혼용하지 않는다.
RawSunFlyerFrame::RawSunFlyerFrame(SidPool& pool,std::span<const SquidDisplayShape> shapes,std::span<const SunFlyerFrameType> types,
    SunFlyerFrameGlobals globals,SunFlyerFrameHooks hooks):pool_(pool),shapes_(shapes),types_(types.begin(),types.end()),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || shapes.size()!=188 || types.size()!=188) throw std::invalid_argument("훨리기그 Frame PATCH 자료 불일치");
}
// 생성자가 미정으로 둔 위치 두 성분은 전용 첫 초기화에서만 -10000으로 쓴다.
void RawSunFlyerFrame::Initialize(FlyerProcessFields& fields) const {
    Time(fields,kDropTime,0);Write(fields,kBomb,0);Write(fields,kSavedY,std::bit_cast<std::uint32_t>(-10000.0f));Write(fields,kSavedX,std::bit_cast<std::uint32_t>(-10000.0f));
    Write(fields,80,0);Write(fields,88,0);Time(fields,72,globals_.now+20.0);Write(fields,84,1);
}
// 일반 메시지1은 전용 발사/정지이며 그 밖은 실제 공통 통지로 넘긴다.
void RawSunFlyerFrame::Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const {
    if (message!=1) { Need(hooks_.baseNotify)(fields,message,info1,info2);return; }
    if (!info1) { Write(fields,80,0);Time(fields,kNextDrop,0); }else Time(fields,kNextDrop,globals_.now+2.0);
}
// 삭제 효과가 현재 폭탄 필드를 바꿔도 원본은 마지막에0을 다시 쓴다.
void RawSunFlyerFrame::ClearBomb(FlyerProcessFields& fields) const {
    const auto bomb=Bomb(fields);if (Need(hooks_.robust)(bomb,true)) Need(hooks_.destroy)(bomb,0x10);Write(fields,kBomb,0);
}
// 폭탄 정리 뒤에는 같은 현재 필드를 공통 홈 통지에 전달한다.
void RawSunFlyerFrame::ParentDying(FlyerProcessFields& fields) const { ClearBomb(fields);Need(hooks_.baseParentDying)(fields); }
// frameCheck와 실제 현재 물리 프레임의 첫float를 사용한다.
float RawSunFlyerFrame::Width(Sid parent) const {
    const auto raw=pool_.Slot(parent);const auto type=raw[10];if (type>=shapes_.size()) throw std::out_of_range("훨리기그 SHP TYPE 범위 오류");const auto& shape=shapes_[type];const auto frame=RawRead(raw,36);
    if (!shape.loaded || frame>=shape.frames.size()) throw std::out_of_range("훨리기그 현재 SHP 프레임 오류");return shape.frames[frame].cellWidth;
}
// 전체 전용 폭탄/부모/타이머 분기를 수행한 뒤에만 기존 공통Frame을 실행한다.
void RawSunFlyerFrame::Frame(FlyerProcessFields& fields) const {
    if (!Read(fields,84)) Initialize(fields);const auto parent=fields.Parent();const auto& bombType=types_.at(globals_.bombType); // 첫 부모/폭탄 TYPE 포인터 snapshot이다.
    if (pool_.Slot(parent)[40]&0x20U) { if (globals_.boss) Need(hooks_.destroy)(parent,0x200000);return; }
    const auto bomb=Bomb(fields); // pause 관문보다 먼저 읽은 현재 폭탄이다.
    if (bomb.value && !globals_.paused) {
        if (!(pool_.Slot(bomb)[11]&4U)) Need(hooks_.unpop)(bomb,0);
        const auto frame=std::bit_cast<std::int32_t>(RawRead(pool_.Slot(bomb),36)); // Unpop 뒤 현재 프레임이다.
        if (frame<bombType.fallingFrames) {
            const auto before=pool_.Slot(parent);const auto x=Position(before,14),ceiling=static_cast<float>(static_cast<double>(Position(before,18))+4.0);const auto elapsed=static_cast<float>(globals_.now-Time(fields,kDropTime)); // 원본 세 snapshot/float 저장이다.
            const auto left=static_cast<float>(static_cast<double>(x)-static_cast<double>(Width(parent))*.5);const auto displacement=(static_cast<double>(elapsed)*9.0+10.0)*elapsed;const auto candidate=displacement+Position(pool_.Slot(parent),18); // 넓은 값을 비교 전float로 좁히지 않는다.
            auto y=ceiling;if (!(candidate>ceiling)) y=static_cast<float>(displacement+Position(pool_.Slot(parent),18));
            RawWrite(pool_.AllocatedBytes(bomb),36,std::bit_cast<std::uint32_t>(Remainder(Increment(frame),bombType.fallingFrames)));Need(hooks_.pop)(bomb,left,y,0);
            if (y>=ceiling) RawWrite(pool_.AllocatedBytes(bomb),36,std::bit_cast<std::uint32_t>(bombType.explosionFirst));
        } else {
            if (Time(fields,kNextFrame)<=globals_.now) {
                const auto next=Increment(frame);RawWrite(pool_.AllocatedBytes(bomb),36,std::bit_cast<std::uint32_t>(next));
                if (next>=bombType.explosionEnd) { ClearBomb(fields);return; }
                pool_.AllocatedBytes(bomb)[35]=0xed;Time(fields,kNextFrame,globals_.now+kExplosionDelay);
            }
            Need(hooks_.repop)(bomb,0);
        }
        const auto current=pool_.Slot(parent);const auto x=Position(current,14),y=Position(current,18); // 후속 공간 경계 전 현재 두 위치다.
        if (y>std::bit_cast<float>(Read(fields,kSavedY))) { Need(hooks_.unpop)(parent,0);Need(hooks_.pop)(parent,x,static_cast<float>(static_cast<double>(y)-kRestore),0); }
    }
    if (Need(hooks_.mayTarget)(fields.Parent(),Sid{static_cast<std::uint16_t>(Read(fields,12))}) && Time(fields,kNextDrop)!=0 && Time(fields,kNextDrop)<=globals_.now && !Read(fields,kBomb)) {
        const auto created=Need(hooks_.create)(globals_.bombType,2);auto raw=pool_.AllocatedBytes(created);raw[40]|=8;RawWrite(raw,36,0);
        if (Read(fields,kBomb)) Need(hooks_.assertion)(0x5be);Write(fields,kBomb,created.value);Time(fields,kDropTime,globals_.now);
        Need(hooks_.unpop)(parent,0);const auto current=pool_.Slot(parent);Write(fields,kSavedX,RawRead(current,14));Write(fields,kSavedY,RawRead(current,18));
        Need(hooks_.pop)(parent,Position(current,14),static_cast<float>(static_cast<double>(Position(current,18))+.4),0);
        const auto after=pool_.Slot(parent);const auto x=Position(after,14),y=Position(after,18);const auto sound=types_.at(pool_.Slot(created)[10]).sound; // Pop 뒤의 현재 위치와 생성 폭탄 TYPE다.
        Need(hooks_.sound)(x,y,sound);Time(fields,kNextDrop,globals_.now+2.0);
    }
    Need(hooks_.baseFrame)(fields);
}
// 첫 fmod의 Form 위상은float scratch 저장 뒤에도 넓은 값을 유지한다.
std::int32_t RawSunFlyerFrame::Animation(FlyerProcessFields& fields) const {
    const auto count=Need(hooks_.runCount)(fields.Parent());const auto countFloat=static_cast<float>(count);const auto phase=std::fmod(static_cast<double>(fields.Form().value),kPeriod)*kInversePeriod+globals_.now;const auto remainder=std::fmod(phase,kPeriod);
    const auto rounded=static_cast<float>((static_cast<double>(countFloat)-1.0)*remainder*kInversePeriod);const auto value=static_cast<double>(rounded)+.5;
    if (!std::isfinite(value) || value<INT32_MIN || value>=2147483648.0) throw std::out_of_range("훨리기그 원본 장식 프레임 정수화 범위 오류");return static_cast<std::int32_t>(value);
}
// 보호 PE의00509f3c는 float60이다.
double RawSunFlyerFrame::ReturnDelay() { return 60.0; }
}
