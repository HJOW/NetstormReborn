// 명령·검색·조회 뒤 현재 필드를 다시 읽고 원본 float 저장 위치와 첫 동률을 유지한다.
#include "o/RawFlyerTarget.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kHead=6,kType=10,kState=11,kX=14,kY=18,kDirty=8,kTarget=12,kReturn=40; // raw 객체와 outer 필드의 실제 BYTE 위치다.
constexpr float kRightBias=0.99999f; // 원본00502310의 오른쪽/아래 절삭 전 보정이며 ceil로 대신하지 않는다.
constexpr std::array<std::array<float,2>,3> kScatter{{{0,-1},{-1,1},{1,1}}}; // PE00532794의 실제 포위 순서다.
// 원본 비정렬 정수의 필요한 바이트만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) {
    std::uint32_t value=0;
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[at+i])<<(8*i); // 필드 폭 밖의 padding을 읽지 않는다.
    return value;
}
// 원본 WORD/DWORD 폭만 써 주변 필드를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i)); // 낮은 바이트부터 원문 순서로 기록한다.
}
// 원본 좌표 float의 IEEE 비트를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
// 실제 ftol의 signed64 절삭 뒤 하위 DWORD를 보존한다. 비유한·범위 밖 CRT 입력은 명시 거부한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("비행 표적 좌표 ftol 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 같은 raw 이미지의 현재 double을 그대로 읽는다.
double Time(const FlyerProcessFields& fields,std::size_t at) { return std::bit_cast<double>(std::uint64_t(Read(fields.bytes,at))|(std::uint64_t(Read(fields.bytes,at+4))<<32)); }
// 원본 double의 두 DWORD만 대입한다.
void Time(FlyerProcessFields& fields,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); }
// Point.distance의 float 입력 사이 차이는 중간 float 저장 없이 double로 계산한다.
double Distance(std::array<float,2> a,std::array<float,2> b) { const auto x=static_cast<double>(a[0])-b[0],y=static_cast<double>(a[1])-b[1];return std::sqrt(x*x+y*y); }
// 원본 Point.isValid는0/256 경계와 비유한 좌표를 제외한다.
bool Valid(std::array<float,2> point) { return point[0]>0 && point[1]>0 && point[0]<256 && point[1]<256; }
// Rect 확장은 추가 점을 먼저1..255로 자른 뒤 기존 꼭짓점의 유효성에 따라 교체한다.
void Include(std::array<float,4>& rect,std::array<float,2> point) {
    for (auto& value:point) { value=value>=1.0f ? value : 1.0f;if (value>255.0f) value=255.0f; } // 두 축의 원문 clamp 순서를 보존한다.
    if (!Valid({rect[0],rect[1]})) { rect[0]=point[0];rect[1]=point[1]; }
    else { rect[0]=std::min(rect[0],point[0]);rect[1]=std::min(rect[1],point[1]); }
    if (!Valid({rect[2],rect[3]})) { rect[2]=point[0];rect[3]=point[1]; }
    else { rect[2]=std::max(rect[2],point[0]);rect[3]=std::max(rect[3],point[1]); }
}
// 선택된 미복원 하위를 빈 성공으로 바꾸지 않고 앞선 원문 쓰기를 보존한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("비행 표적 실제 하위 몸체 미연결");return function; }
// 의존 finder가 후보를 반환하기 전에 저장하는 next와 타입 필터를 같은 풀에서 실행한다.
class DependCursor {
public:
    DependCursor(const SidPool& pool,Sid parent,const std::uint32_t& depend):pool_(pool),depend_(depend),next_(Read(pool.Slot(parent),kHead,2)) {} // 현재 부모 head부터 시작한다.
    Sid Next() {
        while (next_) { // 원본은 free/dead 상태를 추가 배제하지 않는다.
            if (next_>=pool_.Capacity() || ++steps_>pool_.Capacity()) throw std::logic_error("비행 의존 finder 범위/순환 오류");
            const Sid current{static_cast<std::uint16_t>(next_)};const auto raw=pool_.Slot(current);next_=Read(raw,4,2);
            if (raw[kType]<=69 && raw[kType]==depend_) return current;
        }
        return {};
    }
private:
    const SidPool& pool_; // 현재 객체와 의존 체인의 같은 저장소다.
    const std::uint32_t& depend_; // 각 후보에서 다시 읽는 의존 TYPE 전역이다.
    std::uint32_t next_{}; // 반환 전에 저장한 다음 Form 번호다.
    std::size_t steps_{}; // 손상된 정상 범위 밖 무한 체인만 거부한다.
};
}
RawFlyerTarget::RawFlyerTarget(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const float> ranges,FlyerTargetGlobals globals,FlyerTargetHooks hooks)
    :pool_(pool),types_(types),ranges_(ranges.begin(),ranges.end()),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || ranges.size()!=188) throw std::invalid_argument("비행 표적은 같은 PATCH188TYPE 자료가 필요합니다");
}
// 원문의 Parent getter는 부모 head가 비어 있으면 진단한 뒤 같은 포인터를 반환한다.
Sid RawFlyerTarget::Parent(FlyerProcessFields& fields) const { const auto sid=fields.Parent();if (!Read(pool_.Slot(sid),kHead,2)) Need(hooks_.assertion)(0xa4);return sid; }
std::array<float,2> RawFlyerTarget::Center(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto& type=types_[raw[kType]];
    return {static_cast<float>(static_cast<double>(Float(raw,kX))-type.footX*.5),static_cast<float>(static_cast<double>(Float(raw,kY))-type.footY*.5)};
}
// capacity는 free 비트를 추가 검사하지 않으며 dead와 실제1×1 발자국만 구별한다.
std::int32_t RawFlyerTarget::Capacity(Sid target) const {
    if (!target.value) return 3;const auto raw=pool_.Slot(target);const auto& type=types_[raw[kType]];return !(raw[kState]&2U) && type.footX==1 && type.footY==1 ? 1 : 3;
}
// 비교 기준 거리는 float에 먼저 저장하고 다른 비행체의 거리는 double 상태로 비교한다.
std::int32_t RawFlyerTarget::CloserCount(Sid target,Sid flyer) const {
    if (!target.value || !flyer.value) return 0;
    if ((pool_.Slot(target)[kState]&3U) || (pool_.Slot(flyer)[kState]&3U)) return 0;
    const auto center=Center(target);const auto mine=static_cast<float>(Distance(Center(flyer),center));std::int32_t count{};
    DependCursor cursor(pool_,target,globals_.dependType);
    for (auto form=cursor.Next();form.value;form=cursor.Next()) { // 반환 전 저장한 next로 같은 의존 목록을 계속한다.
        const auto watcher=Need(hooks_.watcher)(form);if (!watcher.value) continue;const auto type=pool_.Slot(watcher)[kType];
        if (type<10 || type>=globals_.processEnd) continue;const auto parent=Need(hooks_.processParent)(watcher);
        if (parent!=flyer && (types_[pool_.Slot(parent)[kType]].flags2&0x100000U) && Distance(Center(parent),center)<mine) ++count;
    }
    return count;
}
// 자기 의존 Form의 번호를 먼저 찾고 다시 시작해 더 작은 Form 번호의 비행체만 센다.
std::int32_t RawFlyerTarget::SlotIndex(Sid target,Sid flyer) const {
    Sid mine{};DependCursor first(pool_,target,globals_.dependType);
    for (auto form=first.Next();form.value;form=first.Next()) { // 첫 검색은 상대 Form의 가상 부모를 직접 비교한다.
        const auto watcher=Need(hooks_.watcher)(form);if (watcher.value && Read(pool_.Slot(watcher),16,2)==flyer.value) { mine=form;break; }
    }
    if (!mine.value) { if (globals_.checking) Need(hooks_.assertion)(0x374);return 0; }
    std::int32_t count{};DependCursor second(pool_,target,globals_.dependType);
    for (auto form=second.Next();form.value;form=second.Next()) { // 순회 순서 대신 실제 Form 번호의 크기로 자리를 결정한다.
        const auto watcher=Need(hooks_.watcher)(form);if (!watcher.value) continue;const auto type=pool_.Slot(watcher)[kType];
        if (type<10 || type>=globals_.processEnd) continue;const auto parent=Need(hooks_.processParent)(watcher);
        if (!Read(pool_.Slot(parent),kHead,2)) Need(hooks_.assertion)(0xa4);
        if ((types_[pool_.Slot(parent)[kType]].flags2&0x100000U) && form.value<mine.value) ++count;
    }
    return count;
}
std::array<float,2> RawFlyerTarget::Scatter(Sid target,Sid flyer) const {
    if (Capacity(target)==1) return {0,0};const auto index=SlotIndex(target,flyer);
    if (index>=0 && index<Capacity(target)) return kScatter.at(static_cast<std::size_t>(index));return {0,0};
}
// 현재 원문 pointer·전역 읽기 순서를 유지하며 거리 동률은 먼저 반환된 후보를 남긴다.
void RawFlyerTarget::Find(FlyerProcessFields& fields) const {
    if (!globals_.server) Need(hooks_.assertion)(0x3af);const auto parent=Parent(fields);
    if (!Need(hooks_.checkForms)(parent)) Need(hooks_.assertion)(0x3b7);
    const auto home=fields.Home();const auto raw=pool_.Slot(parent);const auto range=ranges_[raw[kType]];auto origin=Center(parent);
    const auto offset=Need(hooks_.offset)(parent);origin={static_cast<float>(static_cast<double>(origin[0])-offset[0]),static_cast<float>(static_cast<double>(origin[1])-offset[1])};
    if (!Read(raw,42) && !Read(raw,46)) { Put(pool_.AllocatedBytes(parent),42,std::bit_cast<std::uint32_t>(Truncate(origin[0])));Put(pool_.AllocatedBytes(parent),46,std::bit_cast<std::uint32_t>(Truncate(origin[1]))); }
    else origin={static_cast<float>(std::bit_cast<std::int32_t>(Read(raw,42))),static_cast<float>(std::bit_cast<std::int32_t>(Read(raw,46)))};
    if (Read(fields.bytes,kTarget,2)) Need(hooks_.assertion)(0x3d5);fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,0,2);
    std::array<float,4> rect{origin[0],origin[1],origin[0],origin[1]}; // 원본은 현재 출발점 하나로 시작한다.
    Include(rect,{static_cast<float>(static_cast<double>(origin[0])+range),static_cast<float>(static_cast<double>(origin[1])+range)});
    Include(rect,{static_cast<float>(static_cast<double>(origin[0])-range),static_cast<float>(static_cast<double>(origin[1])-range)});
    auto best=static_cast<float>(static_cast<double>(range)+range);
    const SquidSearchArea area{Truncate(rect[0]),Truncate(rect[1]),Truncate(static_cast<double>(rect[2])+kRightBias),Truncate(static_cast<double>(rect[3])+kRightBias)};
    for (auto target=Need(hooks_.begin)(area,1);target.value;target=Need(hooks_.next)()) { // 원본 일반 finder의 같은 순서와 중복 후보를 유지한다.
        const auto flags=types_[pool_.Slot(target)[kType]].flags2;
        if (!Need(hooks_.mayTarget)(fields.Parent(),target) || (flags&0x120000U) || target==home || ((flags&0x210000U) && pool_.Slot(parent)[kType]==globals_.sunType)) continue;
        const auto capacity=Capacity(target);if (CloserCount(target,parent)>=capacity) continue;
        const auto first=Center(target),second=Center(target);const std::array<float,2> delta{static_cast<float>(static_cast<double>(first[0])-origin[0]),static_cast<float>(static_cast<double>(second[1])-origin[1])};
        if (!(std::sqrt(static_cast<double>(delta[0])*delta[0]+static_cast<double>(delta[1])*delta[1])<range)) continue;
        const auto distance=Distance(Center(target),origin);if (!(distance<best && distance<range)) continue;
        best=static_cast<float>(distance);fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,target.value,2);
        if (pool_.Slot(Parent(fields))[kType]==globals_.sunType && Time(fields,kReturn)==0.0) { const auto delay=Need(hooks_.returnDelay)(fields);Time(fields,kReturn,delay+globals_.now);Need(hooks_.command)(Parent(fields),2); }
    }
    if (!Need(hooks_.checkForms)(parent)) Need(hooks_.assertion)(0x413);
}
const SidPool& RawFlyerTarget::Pool() const { return pool_; } // 연결된 검색기와 같은 풀인지 확인한다.
}
