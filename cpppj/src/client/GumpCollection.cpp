// Gump 화면 좌표와 표시 목록의 원본 분기/쓰기 순서를 구현한다.
#include "client/GumpCollection.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 원본 visible, 화면 밖 자식 통과, 월드 부착, 삭제 대기 비트와 MAX_GUMPS다.
constexpr std::uint32_t kVisible=1,kPassChildren=0x20,kWorldAttached=0x2000,kDead=0x10000;
constexpr std::size_t kMaxGumps=1000;
// CRT _ftol의 64비트 절삭 뒤 하위 DWORD다. 비유한/64비트 범위 밖은 원본 불확정 정수의 하위 0이다.
std::int32_t Ftol(double number) {
    if (!std::isfinite(number) || number<-9223372036854775808.0 || number>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(number))));
}
// 원본 정수 화면 좌표 덧셈은 DWORD 넘침을 보존한다.
std::int32_t Add(std::int32_t a,std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b));
}
// 역전 사각형을 거부하고 맞닿은 가장자리는 제외한다. 내부의 폭/높이 0은 원본처럼 허용한다.
bool Intersects(ScreenRect a,ScreenRect b) {
    return a.left<=a.right && a.top<=a.bottom && b.left<=b.right && b.top<=b.bottom &&
        a.left<b.right && b.left<a.right && a.top<b.bottom && b.top<a.bottom;
}
// 현재 재귀 경로와 형제 체인의 손상/과도한 길이를 거부한다. 서로 다른 경로의 중복 토큰은 허용한다.
void Enter(std::unordered_set<std::uint32_t>& seen,std::uint32_t token) {
    if (seen.size()>=kMaxGumps || !seen.insert(token).second) throw std::runtime_error("Gump 링크 순환 또는 한도 초과");
}
}
GumpCollection::GumpCollection(const GumpCollectionState& state,GumpCollectionHooks hooks)
    :state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 저장소 조회 필요");
}
GumpCollectionNode& GumpCollection::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 토큰/저장소 없음");
    return *node;
}
ScreenPoint GumpCollection::Anchor(const GumpCollectionNode& node) const {
    if (node.parent!=state_.root || !hooks_.project) throw std::runtime_error("월드 부착 Gump의 루트/투영 필요");
    const auto point=hooks_.project(node.object);
    return {Add(point.x,Ftol(node.x)),Add(point.y,Ftol(node.y))};
}
ScreenPoint GumpCollection::Position(std::uint32_t token) const {
    auto& node=Node(token);
    if (node.flags&kWorldAttached) return Anchor(node);
    float x=node.x,y=node.y;std::unordered_set<std::uint32_t> seen;Enter(seen,token);
    // 각 부모의 덧셈 직후 float32로 저장하며 부모 자신의 캐시는 사용하지 않는다.
    for (auto parent=node.parent;parent;parent=Node(parent).parent) {
        Enter(seen,parent);const auto& above=Node(parent);
        if (above.flags&kWorldAttached) {
            const auto point=Anchor(above);
            const double px=state_.edition==o::OriginalEdition::Patch1078 ? static_cast<double>(point.x) : static_cast<double>(static_cast<float>(point.x));
            const double py=state_.edition==o::OriginalEdition::Patch1078 ? static_cast<double>(point.y) : static_cast<double>(static_cast<float>(point.y));
            x=static_cast<float>(px+static_cast<double>(x));y=static_cast<float>(py+static_cast<double>(y));
        } else {
            x=static_cast<float>(static_cast<double>(above.x)+static_cast<double>(x));
            y=static_cast<float>(static_cast<double>(above.y)+static_cast<double>(y));
        }
    }
    node.cached={Ftol(x),Ftol(y)};return node.cached;
}
ScreenRect GumpCollection::Bounds(std::uint32_t token) const {
    const auto point=Position(token);const auto& node=Node(token);
    return {point.x,point.y,Ftol(static_cast<double>(point.x)+node.width),Ftol(static_cast<double>(point.y)+node.height)};
}
std::int32_t GumpCollection::CollectNode(std::uint32_t token,ScreenRect view,
    std::span<o::RenderCollectionEntry> output,std::int32_t limit) const {
    std::unordered_set<std::uint32_t> active;
    // 재귀 호출마다 남은 개수와 출력 위치를 전달한다. 자식 호출이 링크를 바꾸면 현재 next를 읽는다.
    const auto collect=[&](auto&& self,std::uint32_t current,std::span<o::RenderCollectionEntry> remaining,std::int32_t available)->std::int32_t {
        if (available<1) return 0;
        Enter(active,current);const bool visible=Intersects(Bounds(current),view);auto& node=Node(current);
        if (!visible && !(node.flags&kPassChildren)) { active.erase(current);return 0; }
        std::int32_t count=0;
        if (visible) {
            if (remaining.empty()) throw std::length_error("Gump 표시 출력 용량 부족");
            remaining.front()={node.x,node.y,current,std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(node.depth)),2,0};
            remaining=remaining.subspan(1);count=1;--available;
        }
        std::unordered_set<std::uint32_t> siblings;
        // 자식은 visible만 검사한다. dead 자식을 제외하는 필터는 원본에 없다.
        for (auto child=node.child;child;child=Node(child).next) {
            Enter(siblings,child);
            if (Node(child).flags&kVisible) {
                const auto added=self(self,child,remaining,available);
                remaining=remaining.subspan(static_cast<std::size_t>(added));available-=added;count+=added;
            }
        }
        active.erase(current);return count;
    };
    return collect(collect,token,output,limit);
}
std::int32_t GumpCollection::Collect(ScreenRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const {
    auto head=Node(state_.root).child;if (state_.overrideHead) head=state_.overrideHead;
    std::int32_t count=0;std::unordered_set<std::uint32_t> siblings;
    // 최상위 next는 재귀 전에 저장한다. limit은 각 루트에서 줄이지 않는 원본의 동작이다.
    while (head) {
        Enter(siblings,head);const auto current=head;auto& node=Node(current);head=node.next;
        if ((node.flags&kVisible) && !(node.flags&kDead)) {
            const auto added=CollectNode(current,view,output.subspan(static_cast<std::size_t>(count)),limit);
            if (added>std::numeric_limits<std::int32_t>::max()-count) throw std::length_error("Gump 표시 개수 넘침");
            count+=added;
        }
    }
    return count;
}
void BindGumpCollection(DrawCollectionHooks& hooks,const GumpCollection& collector) {
    // UI 전용 dirty와 일반 병합이 모두 같은 실제 트리 수집기를 사용한다.
    hooks.ui=[&collector](ScreenRect view,auto output,auto limit) { return collector.Collect(view,output,limit); };
}
}
