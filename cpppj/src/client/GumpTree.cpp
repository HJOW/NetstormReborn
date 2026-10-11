// 원본은 현재 필드를 재조회한다. 특히 콜백 전에 저장한 next와 이후의 깊이/정렬을 구분한다.
#include "client/GumpTree.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 기본 void·월드 부착·동종 교체·화면 제한/깊이 상속 생략·드래그 기준점 비트다.
constexpr std::uint32_t kVoid=8,kWorld=0x2000,kReplace=0x4000,kUnclamped=0x20000,kOwnDepth=0x200000,kGrab=0x10;
// 원본 지도 중심은 signed DWORD 덧셈의 감김 뒤 2로 나눈다.
std::int32_t Center(std::int32_t low,std::int32_t high) {
    return std::bit_cast<std::int32_t>(std::uint32_t(low)+std::uint32_t(high))/2;
}
// 형제 체인 순환/과도한 길이는 호스트에서 거부한다. 콜백 변경 뒤에도 현재 방문 경로를 검사한다.
void Enter(std::unordered_set<std::uint32_t>& seen,std::uint32_t token) {
    if (seen.size()>=kGumpRegistryCapacity || !seen.insert(token).second) throw std::runtime_error("Gump 트리 링크 순환/한도 초과");
}
}
// 공통 가상 처리는 필수다. 월드 투영은 해당 분기에서만 요구한다.
GumpTree::GumpTree(GumpCreateState& state,GumpTreeState& tree,GumpTreeHooks hooks):state_(state),tree_(tree),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.redraw || !hooks_.push || !hooks_.attach || !hooks_.kill) throw std::invalid_argument("Gump 트리 저장소/처리기 필요");
}
// 저장소 주소는 콜백/부착 동안 안정되어 있어야 한다.
GumpCreateNode& GumpTree::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("Gump 트리 토큰/저장소 없음");return *node;
}
// 부모 기본 크기는 float 저장값이며 루트 화면 정수도 먼저 float으로 저장한다.
std::array<float,2> GumpTree::Align(std::uint32_t token) const {
    auto& node=Node(token);const auto& parent=Node(node.parent);float width=parent.baseWidth,height=parent.baseHeight,x=node.x,y=node.y;
    if (node.parent==state_.collection.root) { width=static_cast<float>(state_.screenWidth);height=static_cast<float>(state_.screenHeight); }
    const auto flags=node.alignFlags;
    if ((flags&0xc0) && node.parent!=state_.collection.root) throw std::runtime_error("지도 중심 정렬의 루트 부모 필요");
    if (flags&1) { node.baseWidth=width;node.width=static_cast<float>(double(width)*node.scale); }
    if (flags&2) { node.baseHeight=height;node.height=static_cast<float>(double(height)*node.scale); }
    if (flags&4) x=static_cast<float>((double(width)-node.baseWidth)*0.5);
    if (flags&8) y=static_cast<float>((double(height)-node.baseHeight)*0.5);
    if (flags&0x10) x=static_cast<float>(double(width)-node.baseWidth);
    if (flags&0x20) y=static_cast<float>(double(height)-node.baseHeight);
    if (flags&0x40) x=static_cast<float>(double(Center(tree_.mapView.left,tree_.mapView.right))-double(node.baseWidth)*0.5);
    if (flags&0x80) y=static_cast<float>(double(Center(tree_.mapView.top,tree_.mapView.bottom))-double(node.baseHeight)*0.5);
    return {x,y};
}
// 밀어내기는 자신의 parent/next/previous를 지우지 않는다. 가상 재그리기에는 분리된 전역 링크를 보여준다.
void GumpTree::Push(std::uint32_t token) const {
    auto& node=Node(token);if (node.flags&kVoid) throw std::runtime_error("이미 void인 Gump 밀어내기");
    if (!node.previous && !node.parent && node.id) throw std::runtime_error("부모 없는 Gump의 id 0 필요");
    node.flags|=kVoid;
    if (node.previous) {
        Node(node.previous).next=node.next;
        if (state_.collection.edition==o::OriginalEdition::Patch1078 && tree_.pushDebugMode==3) Node(node.previous).next=node.previous;
    } else if (node.parent) {
        Node(node.parent).child=node.next;
        if (state_.collection.edition==o::OriginalEdition::Patch1078 && tree_.pushDebugMode==4) Node(node.parent).child=node.parent;
    }
    if (node.next) Node(node.next).previous=node.previous;
    hooks_.redraw(token,true);
}
// 부착의 상태 쓰기/가상 호출 순서는 원본과 같고 삭제 요청은 별도 실제 수명 처리기에 전달한다.
void GumpTree::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Node(token);
    if (state_.collection.edition!=o::OriginalEdition::Patch1078 && !(node.flags&kVoid)) throw std::runtime_error("CD Gump 부착의 void 필요");
    const auto destination=parent ? parent : node.parent;Node(destination);
    node.flags&=~kVoid;if (parent) node.parent=parent;
    if (node.parent!=state_.collection.root && !(node.flags&kOwnDepth)) node.depth=Node(node.parent).depth+(node.secondaryDepth ? node.secondaryDepth : 0xfffffffdU);
    if (node.flags&kReplace) {
        std::unordered_set<std::uint32_t> seen;
        // 삭제 요청 전에 next를 보관한다. 죽은 항목도 원본의 교체 요청 대상이다.
        for (auto current=Node(node.parent).child;current;) {
            Enter(seen,current);const auto next=Node(current).next;
            if (Node(current).flags&kReplace) hooks_.kill(current);current=next;
        }
    }
    node.previous=0;node.next=Node(node.parent).child;Node(node.parent).child=token;
    if (node.next) Node(node.next).previous=token;
    if (!(mode&1) && !(node.flags&kUnclamped)) {
        ScreenPoint origin{};
        if (node.flags&kWorld) {
            if (!hooks_.project) throw std::runtime_error("월드 Gump 부착의 투영 필요");
            origin=hooks_.project(node.object);x=static_cast<float>(double(origin.x)+x);y=static_cast<float>(double(origin.y)+y);
        }
        if (node.parent==state_.collection.root) {
            const bool patch=state_.collection.edition==o::OriginalEdition::Patch1078;
            const double width=patch ? double(state_.screenWidth) : double(static_cast<float>(state_.screenWidth));
            const double height=patch ? double(state_.screenHeight) : double(static_cast<float>(state_.screenHeight));
            if (width<double(x)+node.width) x=static_cast<float>(width-node.width);
            if (height<double(y)+node.height) y=static_cast<float>(height-node.height);
            if (patch ? x<0.0F : x<=0.0F) x=0.0F;
            if (patch ? y<0.0F : y<=0.0F) y=0.0F;
        }
        if (node.flags&kWorld) { x=static_cast<float>(double(x)-origin.x);y=static_cast<float>(double(y)-origin.y); }
    }
    node.x=x;node.y=y;if (mode&2) { const auto position=Align(token);node.x=position[0];node.y=position[1]; }
    hooks_.redraw(token,false);std::unordered_set<std::uint32_t> seen;
    // redraw가 바꾼 현재 child를 읽는다. 각 자식의 next는 가상 Push 전에 보관한다.
    for (auto child=node.child;child;) {
        Enter(seen,child);auto& below=Node(child);const auto next=below.next;
        if (below.alignFlags || below.secondaryDepth) {
            hooks_.push(child);
            if (below.secondaryDepth) below.depth=node.depth+below.secondaryDepth;
            const auto position=Align(child);hooks_.attach(child,position[0],position[1],0,0);
        }
        child=next;
    }
}
// 자식 재귀 후 현재 next를 읽는다. 부호 있는 깊이도 DWORD 덧셈으로 넘침을 보존한다.
void GumpTree::Depth(std::uint32_t token,std::uint32_t value,bool add,std::vector<std::uint32_t>& active) const {
    if (active.size()>=kGumpRegistryCapacity || std::find(active.begin(),active.end(),token)!=active.end()) throw std::runtime_error("Gump 깊이 재귀 순환/한도 초과");
    active.push_back(token);auto& node=Node(token);std::unordered_set<std::uint32_t> seen;
    // visible/dead/void와 관계없이 모든 자식의 깊이를 갱신한다.
    for (auto child=node.child;child;child=Node(child).next) { Enter(seen,child);Depth(child,value,add,active); }
    node.depth=add ? node.depth+value : value;active.pop_back();
}
// 현재 깊이 하위 DWORD에 같은 delta를 모든 자식부터 더한다.
void GumpTree::AddDepth(std::uint32_t token,std::uint32_t delta) const { std::vector<std::uint32_t> active;Depth(token,delta,true,active); }
// 모든 자식부터 지정 DWORD 깊이로 바꾼다.
void GumpTree::SetDepth(std::uint32_t token,std::uint32_t depth) const { std::vector<std::uint32_t> active;Depth(token,depth,false,active); }
// 드래그 기준점은 밀어내기 전 값으로 보정하며 그 뒤에는 가상 부착에 좌표를 넘긴다.
void GumpTree::Move(std::uint32_t token,float x,float y,std::uint32_t parent) const {
    auto& node=Node(token);
    if (node.flags&kGrab) { x=static_cast<float>(double(x)-std::bit_cast<float>(node.positioning[0]));y=static_cast<float>(double(y)-std::bit_cast<float>(node.positioning[1])); }
    if (!(node.flags&kVoid)) hooks_.push(token);hooks_.attach(token,x,y,parent,0);
}
// 먼저 밀어내므로 기준 노드/현재 parent/크기도 콜백 이후 값을 사용한다.
void GumpTree::MoveRelative(std::uint32_t token,std::uint32_t reference,float x,float y,std::uint8_t anchor,std::uint32_t parent) const {
    auto& node=Node(token);if (!(node.flags&kVoid)) hooks_.push(token);
    const auto& base=Node(reference);
    if (reference!=node.parent) { x=static_cast<float>(double(x)+base.x);y=static_cast<float>(double(y)+base.y); }
    if (anchor&8) x=static_cast<float>(double(x)+base.width);
    if (anchor&4) y=static_cast<float>(double(y)+base.height);
    hooks_.attach(token,x,y,parent,0);
}
}
