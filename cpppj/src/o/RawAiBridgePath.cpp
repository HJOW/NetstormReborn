// 다리 탐색의 signed 상태 전이와 배치 뒤 현재 값 재조회·부분 효과를 보존한다.
#include "o/RawAiBridgePath.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 비정렬 little endian DWORD를 원래 BYTE 순서로 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 경로 DWORD 읽기 범위");
    std::uint32_t value{};
    // 지정 네 BYTE만 읽는다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 원본 DWORD의 signed 비교를 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 도달한 하위가 없으면 앞선 효과를 유지하고 해당 위치에서 오류를 알린다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 다리 경로 실제 하위 미연결");return function; }
}
// 생성자에서는 탐색/게임 장치 효과를 실행하지 않는다.
RawAiBridgePath::RawAiBridgePath(RawAiBrainMemory& memory,const SidPool& pool,const ContainedFinderState& finder,AiBridgePathGlobals globals,AiBridgePathHooks hooks):memory_(memory),pool_(pool),finder_(finder),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI 다리 경로 PATCH 풀 누락");
}
// 소멸된 토큰이나 작은 가짜 본체를 허용하지 않는다.
std::span<std::uint8_t> RawAiBridgePath::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 다리 경로 실제0xb08 본체 누락");return raw; }
// 하위 효과 뒤 본체를 다시 조회한다.
std::uint32_t RawAiBridgePath::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 본체의 지정 필드 폭만 쓴다.
void RawAiBridgePath::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const {
    auto raw=Body(token);if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 경로 DWORD 쓰기 범위");
    // 상위와 하위 필드를 보존하며 네 BYTE를 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 두 좌표의 signed zero와 무효 좌표도 그대로 읽는다.
RawPathPoint RawAiBridgePath::Point(std::uint32_t token,std::size_t offset) const { return {std::bit_cast<float>(Field(token,offset)),std::bit_cast<float>(Field(token,offset+4))}; }
// 원래 XY 순서대로 결과를 쓴다.
void RawAiBridgePath::Point(std::uint32_t token,std::size_t offset,RawPathPoint value) const { Field(token,offset,std::bit_cast<std::uint32_t>(value.x));Field(token,offset+4,std::bit_cast<std::uint32_t>(value.y)); }
// mapLimit은 원본처럼 float에 저장한 뒤 strict 비교한다.
bool RawAiBridgePath::Valid(RawPathPoint point) const { const auto limit=static_cast<float>(globals_.mapLimit);return point.x>0 && point.x<limit && point.y>0 && point.y<limit; }
// 삭제 이후의 현재 시각과 rate를 더하며 소유자/TYPE/kind 필터를 추가하지 않는다.
void RawAiBridgePath::Consume(std::uint32_t token,Sid parent,std::uint32_t pattern) const {
    static_cast<void>(Body(token));RawContainedFinder cursor(pool_,finder_);
    // 원본 contained 순서에서 사용 중인 조각을 건너뛰고 첫 패턴 일치만 소비한다.
    for (auto sid=cursor.Begin(parent);sid.value;sid=cursor.Next()) {
        const auto raw=pool_.Slot(sid);const auto flags=static_cast<std::uint32_t>(raw[20])|(static_cast<std::uint32_t>(raw[21])<<8);
        const auto word=static_cast<std::uint32_t>(raw[12])|(static_cast<std::uint32_t>(raw[13])<<8);
        if ((flags&0x140U)!=0x40U || word!=pattern) continue;
        Need(hooks_.destroy)(sid,0);
        const auto due=std::bit_cast<std::uint64_t>(static_cast<double>(std::bit_cast<float>(Field(token,0x600)))+globals_.now);
        Field(token,0x630,static_cast<std::uint32_t>(due));Field(token,0x634,static_cast<std::uint32_t>(due>>32));return;
    }
}
// 전이 이후 다음 단계를 같은 호출에서 실행하지 않는다.
bool RawAiBridgePath::Step(std::uint32_t token,RawPathPoint& source,RawPathPoint target) const {
    switch (Field(token,0x5f0)) {
    case 0:if (Signed(Field(token,0x628))>=Signed(Field(token,0x604)-1U)) Field(token,0x5f0,1);break;
    case 1:Need(hooks_.initialize)(token,source,target);Field(token,0x5f0,Field(token,0x5f0)+1U);break;
    case 2:if (Need(hooks_.prepare)(token)) Field(token,0x5f0,Field(token,0x5f0)+1U);break;
    case 3:if (Need(hooks_.search)(token)) Field(token,0x5f0,Field(token,0x5f0)+1U);break;
    case 4:if (Signed(Field(token,0x628))>=Signed(Field(token,0x604))) Field(token,0x5f0,5);break;
    case 5:source=Point(token,0x6bc);Field(token,0x5f0,0);return true;
    default:break;
    }
    return false;
}
// 선택한 끝점은 배치/삭제가 본체를 바꾸어도 이번 반환값으로 유지한다.
RawPathPoint RawAiBridgePath::Extend(std::uint32_t token,RawPathPoint source,RawPathPoint target) const {
    if (!Step(token,source,target)) return source;
    const auto result=Point(token,0x6bc);
    if (Valid(result)) {
        const auto direction=Field(token,0x6b8),pattern=Field(token,0x6b4);const auto point=Point(token,0x6ac);
        Need(hooks_.deploy)(token,globals_.bridgeType,point,pattern,direction);
        const auto parent=Need(hooks_.technology)(Field(token,0x260));Consume(token,parent,Field(token,0x6b4));
        const auto currentParent=Need(hooks_.technology)(Field(token,0x260));Need(hooks_.refresh)(token,currentParent);
        if (globals_.logKind==29) { Need(hooks_.marker)(token,globals_.endMarker,result);Need(hooks_.marker)(token,globals_.targetMarker,target); }
    }
    return result;
}
// 시작점 부재는 즉시 timeout 실패로 바꾸지 않으며 기존 타이머도 건드리지 않는다.
bool RawAiBridgePath::Advance(std::uint32_t token,RawPathPoint target) const {
    const auto current=Point(token,0x3d0);
    if (!Valid(current)) {
        const auto start=Need(hooks_.start)(token,target);Point(token,0x3d0,start);
        if (Valid(start) && globals_.logKind==29) Need(hooks_.marker)(token,globals_.startMarker,start);
        return true;
    }
    const auto next=Extend(token,current,target);Point(token,0x3d0,next);
    if (Valid(next)) { Field(token,0x3d8,1);return true; }
    const auto timeout=Field(token,0x3d8)-1U;Field(token,0x3d8,timeout);
    if (Signed(timeout)>0) return true;
    Field(token,0x3d8,1);if (Field(token,0xb00)) Need(hooks_.log)(token,globals_.logKind,"Bridge timeout\n");return false;
}
// 같은 실제 풀의 주소를 반환한다.
const SidPool& RawAiBridgePath::Pool() const { return pool_; }
}
