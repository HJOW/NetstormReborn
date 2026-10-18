// 맨 오 워 수명/프레임과 공통 비행 제어의 현재 읽기·부분 쓰기·가상 호출 순서를 유지한다.
#include "o/RawFlyerFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kDirty=8,kHome=10,kTarget=12,kSpeed=16,kRadius=20,kLast=32,kReturn=40,kSearch=64,kInitialDelay=72,kAttack=80,kInitialized=84,kLanding=88,kAnimation=96,kLifetime=104; // outer 기준 BYTE 오프셋이며 표적 검색64와 최초20초 타이머72를 구별한다.
constexpr double kRainDelay=0.10000000149011612; // 실제00509c58의 double이며0.1로 축약하지 않는다.
// 원본 비정렬 little endian의 필요한 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { std::uint32_t value=0;for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[at+i])<<(i*8);return value; }
// 원문 WORD/DWORD 쓰기만 수행해 padding을 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(i*8)); }
// 같은 raw 이미지의 double을 IEEE 비트로 읽는다.
double Time(const FlyerProcessFields& fields,std::size_t at) { return std::bit_cast<double>(std::uint64_t(Read(fields.bytes,at))|(std::uint64_t(Read(fields.bytes,at+4))<<32)); }
// 원본 double의 두 DWORD만 기록한다.
void Time(FlyerProcessFields& fields,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); }
// 현재 raw float 필드의 입력 비트를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
// 현재 부모/표적의 Pop 좌표 snapshot이다.
std::array<float,2> Position(std::span<const std::uint8_t> raw) { return {Float(raw,14),Float(raw,18)}; }
// 실제 WORD 표적을 현재 필드에서 다시 읽는다.
Sid Target(const FlyerProcessFields& fields) { return Sid{static_cast<std::uint16_t>(Read(fields.bytes,kTarget,2))}; }
// 실제 위치 검사는 원본처럼0/256 경계를 제외하며 NaN도 거부한다.
bool Valid(std::array<float,2> point) { return point[0]>0 && point[1]>0 && point[0]<256 && point[1]<256; }
// 원본 Point.clamp의 축마다1..255 포함 경계다.
void Clamp(std::array<float,2>& point) { for (auto& value:point) { value=value>=1.0f ? value : 1.0f;if (value>255.0f) value=255.0f; } } // 각 축의 원문 비교 순서를 보존한다.
// 원본 Point 거리의 double 곱/합/제곱근이며 float delta와 구별한다.
double Distance(std::array<float,2> first,std::array<float,2> second) { const auto x=static_cast<double>(first[0])-second[0],y=static_cast<double>(first[1])-second[1];return std::sqrt(x*x+y*y); }
// 원본 CRT 정상 좌표의0방향 절삭이다.
std::int32_t Truncate(float value) { if (!std::isfinite(value) || value<-2147483648.0f || value>=2147483648.0f) throw std::out_of_range("비행 좌표 ftol 정상 범위 오류");return static_cast<std::int32_t>(value); }
// 선택된 원문 하위가 비어 있으면 앞선 쓰기를 유지하고 명시 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("비행 Frame 실제 하위 몸체 미연결");return function; }
}
RawFlyerFrame::RawFlyerFrame(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const FlyerFrameType> details,std::span<std::uint32_t> soundFrames,FlyerFrameGlobals globals,FlyerFrameHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),details_(details.begin(),details.end()),soundFrames_(soundFrames),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || details.size()!=188 || soundFrames.size()!=188) throw std::invalid_argument("비행 Frame은 PATCH188의 같은 TYPE/장부만 사용합니다");
}
// 기본 초기화와 맨 오 워 수명 초기화의 실제 쓰기 순서다.
void RawFlyerFrame::Initialize(FlyerProcessFields& fields,bool rain) const {
    if (rain) Time(fields,kLifetime,globals_.now+60.0);Put(fields.bytes,kAttack,0);Put(fields.bytes,kLanding,0);
    const auto ready=globals_.now+20.0;Put(fields.bytes,kInitialized,1);Time(fields,kInitialDelay,ready);
}
// 수명은 boss일 때만 검사한다. server가 아니어도 boss이면 삭제한다.
void RawFlyerFrame::Frame(FlyerProcessFields& fields) const {
    const auto rain=Read(fields.bytes,0)==0x509f60U;
    if (rain) {
        if (!Read(fields.bytes,kInitialized)) Initialize(fields,true);
        if (globals_.boss && ((pool_.Slot(fields.Parent())[40]&0x20U) || Time(fields,kLifetime)<=globals_.now)) { Need(hooks_.destroy)(fields.Parent(),0x200000);return; }
    }
    BaseFrame(fields);
}
// 현재 TYPE의 발자국과 좌표를 사용하는004adea0이다.
std::array<float,2> RawFlyerFrame::Center(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto& type=types_.at(raw[10]);return {static_cast<float>(static_cast<double>(Float(raw,14))-type.footX*.5),static_cast<float>(static_cast<double>(Float(raw,18))-type.footY*.5)};
}
// 현재 표적의 의존 해제가 재진입으로 필드를 바꾸면 그 뒤 WORD를 다시 덮어쓴다.
void RawFlyerFrame::ClearTarget(FlyerProcessFields& fields) const {
    if (!globals_.boss) Need(hooks_.assertion)(0x2ac);
    const auto target=Target(fields);if (!target.value) return;
    Need(hooks_.removeDepend)(fields.Form(),target);fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,0,2);Notify(fields,1,0,0);Need(hooks_.refreshForm)(fields);
}
// 실제 메시지1 등은 빈 몸체다. 사건2/server일 때만 표적을 해제한다.
void RawFlyerFrame::Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const {
    if (Read(fields.bytes,0)==0x509f24U) { Need(hooks_.sunNotify)(fields,message,info1,info2);return; }BaseNotify(fields,message,info1,info2);
}
// 실제 공통45b760은 메시지1을 생략하며, 전용45bd60은 호출자가 먼저 분배한다.
void RawFlyerFrame::BaseNotify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t,std::uint32_t info2) const {
    if (message!=2 || !globals_.server) return;if (info2!=Target(fields).value) Need(hooks_.assertion)(0x2ba);ClearTarget(fields);
}
// 더스트 데빌과0/ffff 홈은 통지를 생략한다. 현재 홈이 실제 live일 때만 가상+34다.
void RawFlyerFrame::ParentDying(FlyerProcessFields& fields) const {
    if (pool_.Slot(fields.Parent())[10]==globals_.windType) return;const auto home=fields.Home();if (!home.value || home.value==0xffff || home.value>=pool_.Capacity()) return;
    if (!(pool_.Slot(home)[11]&3U)) Need(hooks_.homeNotice)(home,0,0);
}
// 원본은 dead 종속을 허용하며 free/void와69보다 큰 타입만 거부한다.
bool RawFlyerFrame::CheckForms(Sid parent) const {
    const auto raw=pool_.Slot(parent);if (raw[10]<=69 || (raw[11]&5U)) return false;auto next=Read(raw,6,2);std::size_t guard=0;
    while (next) { // 같은 종속의 현재 next를 읽으며 정상 체인을 끝까지 확인한다.
        if (next>=pool_.Capacity() || ++guard>pool_.Capacity()) throw std::logic_error("비행 checkForms 정상 범위 밖/순환 체인");const auto child=pool_.Slot(Sid{static_cast<std::uint16_t>(next)});if ((child[11]&5U) || child[10]>69) return false;next=Read(child,4,2);
    }
    return true;
}
// 착륙 대상 포인터는 이동 분기에서 선택한 홈 snapshot이며 제거 인수는 현재 target/Form이다.
bool RawFlyerFrame::ReturnHome(FlyerProcessFields& fields,Sid parent,Sid home) const {
    if (Read(pool_.Slot(home),12,2)!=fields.Parent().value) { Need(hooks_.finish)(fields);return false; }
    auto& sun=Need(hooks_.sunProcess)(home,globals_.sunAviaryProcess);
    if (!Read(fields.bytes,kLanding)) {
        Put(fields.bytes,kLanding,1);sun.bytes[8]|=4;Put(sun.bytes,24,9);Need(hooks_.refreshSun)(sun);
        Need(hooks_.unpop)(home,0);Put(pool_.AllocatedBytes(home),36,27);Need(hooks_.repop)(home,0);return true;
    }
    if (sun.Stage()==9) return true;
    Need(hooks_.removeDepend)(fields.Form(),Target(fields));fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,0,2);
    const auto delay=Need(hooks_.returnDelay)(fields);Time(fields,kReturn,delay+globals_.now);Need(hooks_.refreshForm)(fields);Need(hooks_.command)(parent,2);Put(fields.bytes,kLanding,0);return true;
}
// 초기화/메모리/표적/이동/프레임 및 서버 후반을 원본 순서대로 실행한다.
void RawFlyerFrame::BaseFrame(FlyerProcessFields& fields) const {
    if (!Read(fields.bytes,kInitialized)) { if (Read(fields.bytes,0)==0x509f24U) Need(hooks_.sunInitialize)(fields);else Initialize(fields,Read(fields.bytes,0)==0x509f60U); }
    if (pool_.Slot(fields.Parent())[11]&2U) return;if (pool_.Slot(fields.Parent())[11]&1U) return;
    if (globals_.checking && !Need(hooks_.robust)(fields)) Need(hooks_.assertion)(0x42c);
    if (fields.Home().value && !Need(hooks_.sidRobust)(fields.Home(),false)) Need(hooks_.assertion)(0x431);
    const auto parent=fields.Parent();if (!Need(hooks_.sidRobust)(parent,true)) Need(hooks_.assertion)(0x434);const auto raw=pool_.Slot(parent);
    if (raw[40]&1U) return;if (raw[40]&0x20U) { if (globals_.boss) Need(hooks_.destroy)(parent,0x200000);return; }
    auto position=Position(raw);const auto elapsed=globals_.now-Time(fields,kLast);const auto storedElapsed=static_cast<float>(elapsed); // 원문은 비교 전 float snapshot도 저장한다.
    if (elapsed>0) {
        if (!Valid(position)) Need(hooks_.assertion)(0x447);
        if (globals_.server && Time(fields,kReturn)!=0 && Time(fields,kReturn)<=globals_.now) {
            if (pool_.Slot(fields.Parent())[10]!=globals_.sunType) Need(hooks_.assertion)(0x44b);
            if (Target(fields).value) Need(hooks_.removeDepend)(fields.Form(),Target(fields));
            const auto home=fields.Home();if (!home.value || home.value>=pool_.Capacity() || (pool_.Slot(home)[11]&1U) || Read(pool_.Slot(home),12,2)!=Read(fields.bytes,56)) { Need(hooks_.finish)(fields);return; }
            fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,home.value,2);Need(hooks_.addDepend)(Target(fields),fields.Form());Time(fields,kReturn,0);Need(hooks_.refreshForm)(fields);Need(hooks_.command)(parent,3);
        }
        const auto target=Target(fields);
        if (!target.value || (pool_.Slot(target)[11]&1U)) position=Position(pool_.Slot(parent));
        else {
            auto targetPosition=Center(target);const auto different=target!=fields.Home(); // 홈 여부는 offset 호출 전 snapshot이다.
            if (!different && pool_.Slot(fields.Parent())[10]!=globals_.sunType) Need(hooks_.assertion)(0x46f);
            const auto offset=Need(hooks_.offset)(parent,!different);
            for (std::size_t axis=0;axis<2;++axis) targetPosition[axis]=static_cast<float>(static_cast<double>(targetPosition[axis])+offset[axis]); // 실제 저장된 float 벡터 덧셈이다.
            const auto orbit=Need(hooks_.scatter)(Target(fields),fields.Parent());
            for (std::size_t axis=0;axis<2;++axis) targetPosition[axis]=static_cast<float>(static_cast<double>(targetPosition[axis])+orbit[axis]); // 두 번째 벡터도 별도 float 저장이다.
            Clamp(targetPosition);if (!Valid(targetPosition)) Need(hooks_.assertion)(0x487);
            const std::array<float,2> delta{static_cast<float>(static_cast<double>(targetPosition[0])-position[0]),static_cast<float>(static_cast<double>(targetPosition[1])-position[1])}; // 거리 계산과 달리 이동 delta는 float다.
            const auto distance=std::max(Distance(targetPosition,position),static_cast<double>(0.00009999999747378752f));
            const auto ratio=std::min(1.0,(static_cast<double>(Float(fields.bytes,kSpeed))/distance)*storedElapsed);
            for (std::size_t axis=0;axis<2;++axis) position[axis]=static_cast<float>(ratio*delta[axis]+position[axis]); // 원문의 두 축을 각각 float로 기록한다.
            if (distance>=Float(fields.bytes,kRadius)) {
                Need(hooks_.refreshTime)(parent);
                if (Read(fields.bytes,kAttack)) { Notify(fields,1,0,0);Put(fields.bytes,kAttack,0);if (pool_.Slot(parent)[10]==globals_.rainType) Need(hooks_.removeSound)(parent,globals_.soundProcess); }
            } else if (different) {
                if (!Read(fields.bytes,kAttack)) { Need(hooks_.refreshTime)(parent);Put(fields.bytes,kAttack,1);Notify(fields,1,1,0); }
                else if (pool_.Slot(parent)[10]!=globals_.windType) Need(hooks_.hit)(parent,Target(fields));
            } else if (distance<1.0 && globals_.server && !ReturnHome(fields,parent,target)) return;
        }
    }
    const auto frame=Read(fields.bytes,0)==0x509f60U ? RainFrame(fields) : Need(hooks_.otherFrame)(fields);
    if (Valid(position)) {
        const auto current=pool_.Slot(parent);if (position!=Position(current) || std::bit_cast<std::uint32_t>(frame)!=Read(current,36)) {
            const auto old=Position(current);Need(hooks_.unpop)(parent,0);Put(pool_.AllocatedBytes(parent),36,std::bit_cast<std::uint32_t>(frame));Need(hooks_.pop)(parent,position[0],position[1],0x40);
            if (Truncate(old[0])!=Truncate(position[0]) || Truncate(old[1])!=Truncate(position[1])) Need(hooks_.movedCell)(parent);
        }
    }
    fields.bytes[kDirty]|=0x40;Time(fields,kLast,globals_.now);if (!globals_.server) return;
    if (Target(fields).value) {
        const auto current=fields.Parent();if (!Need(hooks_.checkForms)(current)) Need(hooks_.assertion)(0x4f3);
        const auto range=details_.at(pool_.Slot(current)[10]).range;const auto target=Target(fields);bool keep=Need(hooks_.mayTarget)(fields.Parent(),Target(fields));
        if (keep) { const auto count=Need(hooks_.closerCount)(Target(fields),fields.Parent());keep=count<Need(hooks_.capacity)(Target(fields)); }
        if (keep || Target(fields)==fields.Home()) {
            const auto center=Center(target);const auto source=pool_.Slot(current);const std::array<float,2> delta{static_cast<float>(static_cast<double>(center[0])-std::bit_cast<std::int32_t>(Read(source,42))),static_cast<float>(static_cast<double>(center[1])-std::bit_cast<std::int32_t>(Read(source,46)))};
            const auto distance=std::sqrt(static_cast<double>(delta[0])*delta[0]+static_cast<double>(delta[1])*delta[1]);if (!(distance<range)) ClearTarget(fields);
        } else ClearTarget(fields);
        if (Target(fields).value) return;
    }
    if (Time(fields,kSearch)<=globals_.now) {
        Need(hooks_.findTarget)(fields);
        if (Target(fields).value) { Need(hooks_.addDepend)(Target(fields),fields.Form());Need(hooks_.refreshForm)(fields);return; }
        Time(fields,kSearch,globals_.now+5.0);
    }
}
// 현재 frame6/표적/공격 상태가 모두 맞을 때만 무기를 만들고 타입당 같은 tick의 소리를 억제한다.
std::int32_t RawFlyerFrame::RainFrame(FlyerProcessFields& fields) const {
    if (!(globals_.now>=Time(fields,kAnimation))) return std::bit_cast<std::int32_t>(Read(pool_.Slot(fields.Parent()),36));
    Time(fields,kAnimation,globals_.now+kRainDelay);const auto parent=fields.Parent();
    if (Read(pool_.Slot(parent),36)==6 && Target(fields).value && Read(fields.bytes,kAttack)) {
        Need(hooks_.launch)(fields.Parent(),Target(fields),0);const auto type=pool_.Slot(parent)[10];
        if (soundFrames_[type]!=globals_.frame) { soundFrames_[pool_.Slot(parent)[10]]=globals_.frame;const auto position=Position(pool_.Slot(parent));Need(hooks_.sound)(position[0],position[1],details_.at(pool_.Slot(parent)[10]).sound); }
    }
    const auto frame=std::bit_cast<std::int32_t>(Read(pool_.Slot(fields.Parent()),36));const auto count=Need(hooks_.runCount)(fields.Parent());
    if (!count) throw std::domain_error("원본 비행 프레임 signed 나머지0 경로");const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(frame)+1U);if (next==INT32_MIN && count==-1) throw std::overflow_error("원본 비행 idiv 오버플로 경로");return next%count;
}
const SidPool& RawFlyerFrame::Pool() const { return pool_; } // 현재 동일 World/Host의 풀이다.
}
