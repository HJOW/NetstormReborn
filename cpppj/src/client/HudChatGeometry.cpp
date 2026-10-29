// 0042e6e0은 root 자체를 그리지 않고 현재 네 자식의 크기와 가상 호출 순서를 갱신한다.
#include "client/HudChatGeometry.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kVisible=1; // 현재 Console 표시 여부이며 root의 표시와 구별한다.
constexpr double kHelperHeight=15.0,kConsoleLeft=17.0; // 보호 PE00504a90/005037e0의 실제 float 상수다.
}
// 미연결 장치를 생성 성공처럼 대체하지 않는다. 조회 외 경계는 실제 도달할 때 검사한다.
HudChatGeometry::HudChatGeometry(HudChatGeometryHooks hooks):hooks_(std::move(hooks)) {
    if (!hooks_.chat || !hooks_.node) throw std::invalid_argument("ChatWindow 같은 노드 저장소 필요");
}
// root의 파생/기본 주소를 현재 공통 트리와 대조한다.
HudChatNode& HudChatGeometry::Chat(std::uint32_t token) const {
    auto* node=token ? hooks_.chat(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.node(token)) throw std::runtime_error("ChatWindow 공유 root 없음");return *node;
}
// 자식은 생성자가 연결한 실제 공통 Gump 주소만 허용한다.
GumpCreateNode& HudChatGeometry::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("ChatWindow 실제 자식 없음");return *node;
}
// 원문은 기본 크기를 float에 저장한 뒤 저장 전 중간값으로 배율을 곱한다.
void HudChatGeometry::Size(std::uint32_t token,double width,double height) const {
    auto& node=Node(token);node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(width*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(height*node.scale);
}
// callback 뒤 바뀐 root 자식 포인터는 호출자가 다시 조회한다.
void HudChatGeometry::Push(std::uint32_t token) const {
    Node(token);if (!hooks_.push) throw std::invalid_argument("ChatWindow 가상 Push 필요");hooks_.push(token);
}
// 최종 자식 부착의 parent와 mode는 원문 인자 그대로다.
void HudChatGeometry::AttachVirtual(std::uint32_t token,float x,float y,std::uint32_t parent) const {
    Node(token);if (!hooks_.attach) throw std::invalid_argument("ChatWindow 가상 Attach 필요");hooks_.attach(token,x,y,parent,0);
}
// 기존 부모 부착 뒤 배경→텍스트→helper→Console 순서로 현재 필드를 재조회한다.
void HudChatGeometry::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& root=Chat(token);if (!hooks_.baseAttach) throw std::invalid_argument("ChatWindow 기본 Attach 필요");hooks_.baseAttach(token,x,y,parent,mode);
    Push(root.background);
    const double backgroundHeight=double(root.baseHeight)-((Node(root.console).flags&kVisible) ? Node(root.console).baseHeight : 0.0); // Console 표시 때만 본문 높이에서 입력줄 높이를 뺀다.
    Size(root.background,double(root.baseWidth)-root.leftPadding,backgroundHeight);
    auto& helper=Node(root.helper);const bool showHelper=root.helperVisible && (Node(root.console).flags&kVisible); // helper는 Console과 원시 BYTE 두 관문을 모두 요구한다.
    helper.baseHeight=showHelper ? static_cast<float>(kHelperHeight) : 0.0F;helper.height=static_cast<float>((showHelper ? kHelperHeight : 0.0)*helper.scale);
    AttachVirtual(root.background,static_cast<float>(root.leftPadding),0,token);
    if (!hooks_.bottomIndex) throw std::invalid_argument("ChatWindow 실제 표시 줄 조회004bc2c0 필요");const auto bottom=hooks_.bottomIndex(root.text); // 변경 전 본문 높이에서 현재 줄 위치를 읽는다.
    Push(root.text);Size(root.text,Node(root.background).baseWidth,Node(root.background).baseHeight);
    if (!hooks_.requestBottom) throw std::invalid_argument("ChatWindow 실제 맨 아래 요청 필요");hooks_.requestBottom(root.text,bottom);
    AttachVirtual(root.text,0,0,root.background);
    Push(root.helper);AttachVirtual(root.helper,static_cast<float>(root.leftPadding),static_cast<float>(double(root.baseHeight)-Node(root.helper).baseHeight),token);
    Push(root.console);auto& console=Node(root.console);const double consoleWidth=Node(root.text).baseWidth; // 원문은 Console 높이를 그대로 두고 텍스트 폭만 복사한다.
    console.baseWidth=static_cast<float>(consoleWidth);console.width=static_cast<float>(consoleWidth*console.scale);
    AttachVirtual(root.console,static_cast<float>(double(root.leftPadding)+kConsoleLeft),static_cast<float>(double(root.baseHeight)-Node(root.console).baseHeight),token);
}
// 표시 설정은 별도 다음 Attach가 소비하며 여기서는 기본 플래그나 root 크기를 바꾸지 않는다.
void HudChatGeometry::SetHelperVisible(std::uint32_t token,std::uint32_t value) const { Chat(token).helperVisible=value!=0; }
// challenge에서는 value0 요청도 Console을 표시한다. 현재 자식과 root의 좌표는 각 콜백 뒤 재조회한다.
void HudChatGeometry::SetInputVisible(std::uint32_t token,std::uint32_t value) const {
    if (!token) return;auto& root=Chat(token);Push(token);Push(root.console);
    if (!value && !hooks_.challenge) throw std::invalid_argument("ChatWindow 현재 challenge 필요");
    if (value || hooks_.challenge()) Node(root.console).flags|=kVisible;else Node(root.console).flags&=~kVisible;
    const auto console=root.console;AttachVirtual(console,Node(console).x,Node(console).y,0);
    AttachVirtual(token,root.x,root.y,0);
}
// 원문은 root Push/전체 기하 갱신을 하지 않으며 Push 이후의 현재 root와 Console을 다시 읽는다.
void HudChatGeometry::ToggleInputVisible() const {
    if (!hooks_.current) throw std::logic_error("ChatWindow 현재 포인터 조회 필요");
    const auto before=hooks_.current();if (!before) return;Push(Chat(before).console);
    auto& root=Chat(hooks_.current());Node(root.console).flags^=kVisible;
    const auto console=root.console;AttachVirtual(console,Node(console).x,Node(console).y,0);
}
// 현재 Chat이 없으면 자식/노드 장치를 조회하지 않는 원문 순서를 유지한다.
std::uint32_t HudChatGeometry::InputVisible() const {
    if (!hooks_.current) throw std::logic_error("ChatWindow 현재 포인터 조회 필요");
    const auto current=hooks_.current();return current ? Node(Chat(current).console).flags&kVisible : 0U;
}
// 뗌은 일반 입력에서 먼저 거부되며 Ctrl/Shift/Alt 비트는 문자 Enter를 막지 않는다.
bool DispatchChatInputKey(const InputEvent& event,const std::function<std::uint32_t()>& visible,const std::function<void()>& show,const std::function<void()>& open) {
    if ((event.code&InputCode::kRelease) || (event.code&0x03ffffffU)!=13U) return false;
    if (!visible) throw std::logic_error("Enter의 현재 입력줄 조회 필요");if (visible()) return false;
    if (!show || !open) throw std::logic_error("Enter의 입력줄 표시/Chat 열기 필요");show();open();return true;
}
// 요청을 재조회하는 옵션 callback과 프레임 소비를 구별하며 원문처럼 호출 후0을 쓴다.
void ConsumeChatInputToggle(std::int32_t& request,const std::function<void()>& toggle) {
    if (!request) return;if (!toggle) throw std::logic_error("프레임말 Chat 전환 장치 필요");toggle();request=0;
}
}
