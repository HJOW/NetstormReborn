// 원문00464a50/00464ab0/00499370/00499310/00445290의 계산과 호출 순서를 보존한다.
#include "client/GumpRedraw.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client { // 장치 호출 전까지 원래32비트 좌표/폭과 현재 UI 노드를 유지한다.
namespace { // CRT 절삭/32비트 감김은 원문과 같은 지점에서만 적용한다.
constexpr std::size_t kParentLimit=1000; //원문 등록 한도 안의 조상만 허용하며 손상된 순환을 호스트에서 거부한다.
std::int32_t Ftol(double value) { //원문64비트 CRT 절삭의 하위 DWORD이며 비유한/범위 밖은 불확정 정수의 하위0이다.
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second)); } //XYWH 끝점의 원문32비트 덧셈이다.
std::int32_t Sub(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(first)-static_cast<std::uint32_t>(second)); } //499310의 signed 양의 폭 판정에 쓰는 실제 하위32비트 차이다.
}

// 역전 지도도 원문 순서대로 각 축의 lower를 남기며 별도 범위 교정을 하지 않는다.
ScreenRect ClampOriginalScreenRect(ScreenRect rectangle,ScreenRect bounds) {
    const auto clamp=[](auto value,auto low,auto high) { return std::max(low,std::min(value,high)); }; //445290의 각 좌표 제한 순서다.
    return {clamp(rectangle.left,bounds.left,bounds.right),clamp(rectangle.top,bounds.top,bounds.bottom),clamp(rectangle.right,bounds.left,bounds.right),clamp(rectangle.bottom,bounds.top,bounds.bottom)};
}
// 실제 dirty 장치가 없는 생성은 요청을 빈 성공으로 바꾸기 전에 거부한다.
GumpRedraw::GumpRedraw(const GumpCollection& collection,GumpRedrawHooks hooks):collection_(collection),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.map || !hooks_.invalidate) throw std::invalid_argument("Gump 갱신 현재 장치 누락");
}
// 현재 토큰 조회는 Position 전과 뒤의 원문 필드 읽기 위치를 보존한다.
GumpCollectionNode& GumpRedraw::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::out_of_range("Gump 갱신 노드 부재");return *node;
}
// 숨은 조상을 만나면 위치/캐시/Renderer를 호출하지 않는다. 순환/한도 오류는 호스트 계약이다.
bool GumpRedraw::Visible(std::uint32_t token) const {
    std::unordered_set<std::uint32_t> seen;
    while (true) { //원문 자기/parent의 현재 bit1과 다음parent를 같은 순서로 조회한다.
        if (seen.size()>=kParentLimit || !seen.insert(token).second) throw std::runtime_error("Gump 갱신 조상 순환/한도 초과");
        const auto& node=Node(token);if (!(node.flags&1U)) return false;if (!node.parent) return true;token=node.parent;
    }
}
// Position 뒤의 실제 크기를 각각 절삭한다. 원문은 전달된 인수와 무관하게 flags0을 요청한다.
void GumpRedraw::Whole(std::uint32_t token,std::uint32_t argument) const {
    static_cast<void>(argument);if (!Visible(token)) return;
    const auto position=collection_.Position(token);const auto& node=Node(token);const auto width=Ftol(node.width),height=Ftol(node.height);
    if (width>0 && height>0) hooks_.invalidate({position.x,position.y,Add(position.x,width),Add(position.y,height)},0);
}
// Bounds가 Position+실수 크기를 각 끝점에서 절삭한 뒤 현재 지도를 읽는다.
void GumpRedraw::Map(std::uint32_t token,std::uint32_t argument) const {
    static_cast<void>(argument);if (!Visible(token)) return;
    const auto bounds=collection_.Bounds(token);const auto rectangle=ClampOriginalScreenRect(bounds,hooks_.map());
    if (rectangle.right!=rectangle.left && Sub(rectangle.right,rectangle.left)>=0 && rectangle.bottom!=rectangle.top && Sub(rectangle.bottom,rectangle.top)>=0) hooks_.invalidate(rectangle,0);
}
}
