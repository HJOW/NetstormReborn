// 원문 HTML 부모의 현재 재조회/원시 큐/부분 효과를 그대로 공통 UI에 전달한다.
#include "client/HtmlWindowGump.h"
#include <bit>
#include <stdexcept>
#include <string>
namespace netstorm::client { // 실제 HTML 부모의 수명/본문/입력 구현이다.
namespace { // 원문 상수와 선택 분기의 장치 확인만 모은다.
constexpr std::uint32_t kVtable=0x515e7c; // 보호10.78 HTML 부모의 실제 가상 표다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("HTML 부모 실제 장치 미연결");return hook; } // 실행되는 경계에서만 요구한다.
std::size_t Slot(std::uint32_t index) { if (index>=20) throw std::out_of_range("HTML 부모 실제20행 밖 접근");return index; } // 원문의 유효 할당 밖 물리를 임의 성공으로 채우지 않는다.
}
HtmlWindowGump::HtmlWindowGump(HtmlWindowGumpState& state,HtmlWindowGumpHooks hooks):state_(state),hooks_(std::move(hooks)) { if (!hooks_.node || !hooks_.base) throw std::invalid_argument("HTML 부모 같은 노드 필요"); } // 나머지 장치는 분기에서만 확인한다.
HtmlWindowGumpNode& HtmlWindowGump::Node(std::uint32_t token) const { auto* node=token ? hooks_.node(token) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::logic_error("HTML 부모 같은 실제 주소 없음");return *node; } // 기본 주소를 복제하지 않는다.
GumpCreateNode& HtmlWindowGump::Base(std::uint32_t token) const { auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::logic_error("HTML 실제 자식 없음");return *node; } // 현재 실제 본문/버튼을 조회한다.
std::uint32_t HtmlWindowGump::Construct(std::uint32_t token,std::uint32_t width,std::uint32_t height) const {
    Node(token);Need(hooks_.constructDecoration)(token);auto& node=Node(token);node.vtable=kVtable;
    for (auto& entry:node.controls) entry={0,0,0}; // 원문20행의 세DWORD만 초기화한다.
    node.timeoutSeconds=0;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.kind));node.controlCount=0;node.body=0;node.auxiliarySerial=0;node.html[0]=0;node.updateDepth=0;node.menuColumns=0;node.timeoutCommand=0;node.onExitCommand=0;node.requestedWidth=width;node.requestedHeight=height;++state_.instances;return token;
}
void HtmlWindowGump::Destruct(std::uint32_t token) const {
    Node(token).vtable=kVtable;--state_.instances;
    if (Node(token).onExitCommand) { const auto command=Node(token).onExitCommand,parameter=Node(token).onExitParameter;Need(hooks_.queue)(command,parameter,parameter); } // 원문은 OnExit 필드를0으로 덮지 않는다.
    const auto auxiliary=Need(hooks_.reference)(Node(token).auxiliarySerial);if (auxiliary) Need(hooks_.kill)(auxiliary);state_.lastCloseTime=Need(hooks_.wallSeconds)();Need(hooks_.destructBase)(token);
}
void HtmlWindowGump::ClearButtons(std::uint32_t token) const {
    for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Node(token).controlCount);++index) { auto& entry=Node(token).controls[Slot(index)];if (entry.child) { Need(hooks_.kill)(entry.child);entry.child=0; }entry.command=0;entry.parameter=0; } // Kill 뒤 현재 count를 다음 조건에서 다시 읽는다.
    Node(token).controlCount=0;
}
void HtmlWindowGump::AddButton(std::uint32_t token,std::string_view label,std::uint32_t command,std::uint32_t parameter,std::uint32_t instant) const {
    if (std::bit_cast<std::int32_t>(Node(token).controlCount)>=20) return;Slot(Node(token).controlCount);
    const auto child=Need(hooks_.allocate)(0x170);if (child) Need(hooks_.constructButton)(child,label);auto& entry=Node(token).controls[Slot(Node(token).controlCount)];entry.child=child;
    Need(hooks_.attach)(child,0,0,token,0);entry.parameter=parameter;entry.command=command;
    if (instant) { auto* button=Need(hooks_.button)(child);if (!button) throw std::logic_error("HTML 실제 돌 버튼 없음");button->buttonFlags|=4; }++Node(token).controlCount;
}
void HtmlWindowGump::SetBody(std::uint32_t token,std::string_view text) const {
    text=text.substr(0,text.find('\0'));std::string appended; // 원문의 첫NUL까지이며 끝LF가 있으면 입력 자체를 빌린다.
    if (text.empty() || text.back()!='\n') { if (text.size()>1006) throw std::length_error("HTML 원문 지역 LF 버퍼 범위 초과");appended.assign(text);appended+='\n';text=appended; }
    if (Node(token).body) { Need(hooks_.kill)(Node(token).body);Node(token).body=0; }
    const auto child=Need(hooks_.allocate)(0xd8);if (child) Need(hooks_.constructBody)(child);Node(token).body=child;Base(child).secondaryDepth=0xffffffffU;
    Need(hooks_.scrollable)(Node(token).body,0);Need(hooks_.parseBody)(Node(token).body,text);Need(hooks_.attach)(Node(token).body,0,0,token,0);
    const auto auxiliary=Need(hooks_.reference)(Node(token).auxiliarySerial);if (auxiliary) Need(hooks_.kill)(auxiliary);
    if (!(Node(token).flags&8)) { Need(hooks_.push)(token);Need(hooks_.attach)(token,Node(token).x,Node(token).y,0,0); } // 전체 HTML 가상 Fit이 미연결이면 이 선택 경계에서 명시 실패한다.
}
std::uint32_t HtmlWindowGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const auto code=event&0x03ffffffU;
    const auto fire=[&](const HtmlWindowControl& entry) { const auto command=entry.command,parameter=entry.parameter;Need(hooks_.queue)(command,parameter,parameter);Need(hooks_.kill)(token);return 1U; }; // 큐 뒤 부모Kill이며 release 비트를 따로 거부하지 않는다.
    if (code==state_.enterEvent && Node(token).controls[0].child) return fire(Node(token).controls[0]);
    if (code==state_.clickEvent) { for (const auto& entry:Node(token).controls) if (entry.child && first==Base(entry.child).serial) return fire(entry); } // 현재 count와 무관하게 실제20행을 검사한다.
    else if (code==state_.tagEvent) { if (second) { Need(hooks_.kill)(token);Need(hooks_.link)(second); }return 1; }
    else if (code==state_.closedEvent) { Need(hooks_.kill)(token);return 1; }
    return Need(hooks_.baseHandle)(token,event,first,second);
}
void HtmlWindowGump::Idle(std::uint32_t token) const {
    if (Node(token).timeoutSeconds==0) return;const auto now=Need(hooks_.wallSeconds)();
    if (now>=Node(token).timeoutSeconds) { const auto command=Node(token).timeoutCommand,parameter=Node(token).timeoutParameter;Need(hooks_.queue)(command,parameter,parameter);Need(hooks_.kill)(token); } // NaN에서는 clock 뒤 ordered 비교가 거짓이며 원문처럼 명령0도 별도 거부하지 않는다.
}
}
