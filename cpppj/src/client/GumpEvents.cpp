// 커서 선택의 signed 깊이/현재 전역과 자식 우선·부모 전파의 현재 링크 읽는 순서를 보존한다.
#include "client/GumpEvents.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 visible/화면 밖 자식 통과/void/dead와 이벤트 소비 플래그 및 사건 종류 마스크다.
constexpr std::uint32_t kVisible=1,kPassChildren=0x20,kVoid=8,kDead=0x10000;
constexpr std::uint32_t kHelp=0x200,kKeyboard=0x400,kMouse=0x800,kEventMask=0x03ffffff,kMouseEvent=0x02000000;
// 커서 사각형은 왼쪽/위 끝을 포함하고 오른쪽/아래 끝을 제외한다.
bool Contains(ScreenRect rect,ScreenPoint point) {
    return rect.left<=point.x && point.x<rect.right && rect.top<=point.y && point.y<rect.bottom;
}
// 한 재귀 경로/형제 체인의 중복만 거부한다. 정상 객체의 반복 방문 순서는 바꾸지 않는다.
void Enter(std::vector<std::uint32_t>& active,std::uint32_t token) {
    if (std::find(active.begin(),active.end(),token)!=active.end()) throw std::runtime_error("Gump 이벤트 링크 순환");
    active.push_back(token);
}
}
// 실제 수집기가 같은 노드의 화면 캐시를 갱신하도록 별도 사각형 사본을 만들지 않는다.
GumpEvents::GumpEvents(GumpCreateState& create,GumpLifetimeState& lifetime,GumpEventState& state,
    const GumpCollection& collection,GumpEventHooks hooks):create_(create),lifetime_(lifetime),state_(state),collection_(collection),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 이벤트 저장소 필요");
}
// 현재 노드는 가상 처리기에서 자신의 해제를 예약할 수 있지만 이 호출 동안 주소가 유지되어야 한다.
GumpCreateNode& GumpEvents::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("Gump 이벤트 토큰 없음");return *node;
}
// help 마우스 사건에서 사각형 밖이면 키보드/일반 마우스 플래그로 다시 통과시키지 않는다.
bool GumpEvents::BaseFilter(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    if (token==lifetime_.currentDialog) return true;
    const auto flags=Node(token).flags,kind=event&kEventMask;
    if ((flags&kHelp) && (kind==0x10000 || kind==0x20000))
        return Contains(collection_.Bounds(token),{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
    return ((flags&kKeyboard) && !(event&kMouseEvent) && kind!=0x10000 && kind!=0x20000) ||
        ((flags&kMouse) && (event&kMouseEvent));
}
// 사각형은 플래그 판정 전에 실제 조회한다. 이전 노드가 상태를 바꾸면 다음 링크/깊이는 현재 값이다.
std::uint32_t GumpEvents::PickTopLevel(ScreenPoint point) const {
    std::uint32_t selected=0;std::int32_t depth=1000;std::vector<std::uint32_t> siblings;
    // 루트 자신/손자는 검사하지 않으며 동일 깊이에서는 기존 선택을 보존한다.
    for (auto child=Node(create_.entries[0]).child;child;child=Node(child).next) {
        Enter(siblings,child);const auto bounds=collection_.Bounds(child);const auto& node=Node(child);
        if (Contains(bounds,point) && std::bit_cast<std::int32_t>(node.depth)<depth && (node.flags&kVisible) && !(node.flags&kDead)) {
            depth=std::bit_cast<std::int32_t>(node.depth);selected=child;
        }
    }
    return selected;
}
// 매 호출에서 원본 전역을 초기화한다. mask는 후보만 제한하며 재귀 진입을 막지 않는다.
std::uint32_t GumpEvents::Pick(ScreenPoint point,std::uint32_t mask) const {
    state_.point=point;state_.mask=mask;state_.bestDepth=10000;state_.selected=0;
    std::vector<std::uint32_t> active;PickChildren(create_.entries[0],active);return state_.selected;
}
// 자식은 위치 계산 후 현재 플래그를 읽는다. 화면 밖 통과는 visible/void/dead 조건을 생략하지 않는다.
void GumpEvents::PickChildren(std::uint32_t token,std::vector<std::uint32_t>& active) const {
    Enter(active,token);std::vector<std::uint32_t> siblings;
    // 현재 경계/선택 전역을 사용하며 재귀에서 바뀐 next를 그 뒤 읽는다.
    for (auto child=Node(token).child;child;child=Node(child).next) {
        Enter(siblings,child);const auto bounds=collection_.Bounds(child);const bool inside=Contains(bounds,state_.point);const auto& node=Node(child);
        if (((node.flags&kPassChildren) || inside) && (node.flags&kVisible) && !(node.flags&(kDead|kVoid))) {
            if (inside && std::bit_cast<std::int32_t>(node.depth)<std::bit_cast<std::int32_t>(state_.bestDepth) && (!state_.mask || (node.flags&state_.mask))) {
                state_.bestDepth=node.depth;state_.selected=child;
            }
            PickChildren(child,active);
        }
    }
    active.pop_back();
}
// 가상 처리기는 전달 시에만 필요하다. 선택/기본 필터만 쓰는 호스트는 처리기를 제공하지 않아도 된다.
std::uint32_t GumpEvents::Dispatch(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden) const {
    if (!hooks_.handle) throw std::invalid_argument("Gump 이벤트 처리기 필요");
    std::vector<std::uint32_t> active;return Deliver(token,event,first,second,includeHidden,active);
}
// 자식 성공이면 부모의 필터와 처리기를 실행하지 않고 성공 자식의 토큰을 그대로 반환한다.
std::uint32_t GumpEvents::Deliver(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,
    bool includeHidden,std::vector<std::uint32_t>& active) const {
    Enter(active,token);std::vector<std::uint32_t> siblings;
    // visible 이외의 dead/void 필터는 원본 전달 함수에 없다. 강제 모드는 visible도 무시한다.
    for (auto child=Node(token).child;child;child=Node(child).next) {
        Enter(siblings,child);
        if ((Node(child).flags&kVisible) || includeHidden) {
            const auto result=Deliver(child,event,first,second,includeHidden,active);
            if (result) { active.pop_back();return result; }
        }
    }
    const bool accepts=hooks_.filter ? hooks_.filter(token,event,first,second) : BaseFilter(token,event,first,second);
    const auto result=accepts && hooks_.handle(token,event,first,second) ? token : 0U;
    active.pop_back();return result;
}
// 실패한 부모가 자신의 부모를 바꾸면 그 현재 링크를 다음 목적지로 사용한다.
std::uint32_t GumpEvents::Bubble(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    if (!hooks_.handle) throw std::invalid_argument("Gump 이벤트 처리기 필요");std::vector<std::uint32_t> seen;
    // 원본은 현재 부모의 가상 처리기만 호출하며 기본 필터를 거치지 않는다.
    for (auto parent=Node(token).parent;parent;parent=Node(parent).parent) {
        Enter(seen,parent);const auto result=hooks_.handle(parent,event,first,second);if (result) return result;
    }
    return 0;
}
}
