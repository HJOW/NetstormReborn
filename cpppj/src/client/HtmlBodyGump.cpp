// 원본 부모/자식의 현재 필드를 각 효과 뒤 다시 읽고 가상 Push 뒤 위치/크기 재조회를 유지한다.
#include "client/HtmlBodyGump.h"
#include <new>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 선택한 실제 효과가 없으면 미연결 경계를 드러낸다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("HTML 부모 실제 처리기 누락");return value; }
// x87의 현재 계산값으로 기본/배율 크기를 각각 저장한다.
void Width(GumpCreateNode& node,double value) { node.baseWidth=static_cast<float>(value);node.width=static_cast<float>(value*node.scale); }
void Height(GumpCreateNode& node,double value) { node.baseHeight=static_cast<float>(value);node.height=static_cast<float>(value*node.scale); } // 높이도 먼저 float로 좁히지 않는다.
}
// 실제 상태/노드를 복사하지 않고 도달한 때만 하위 처리기를 요구한다.
HtmlBodyGump::HtmlBodyGump(HtmlBodyGumpState& state,HtmlBodyGumpHooks hooks):state_(state),hooks_(std::move(hooks)) {}
// 실제 파생 저장소가 없으면 원본 포인터 접근의 호스트 계약 위반이다.
HtmlBodyGumpNode& HtmlBodyGump::Node(std::uint32_t token) const { auto* node=Need(hooks_.node)(token);if (!node) throw std::out_of_range("HTML 부모 노드 누락");return *node; }
// 같은 기본 필드는 현재 트리의 주소여야 한다.
GumpCreateNode& HtmlBodyGump::Base(std::uint32_t token) const { auto* node=Need(hooks_.base)(token);if (!node) throw std::out_of_range("HTML 부모/자식 기본 노드 누락");return *node; }
// 첫바이트 두 개만0으로 쓰며 나머지 기존 배열 바이트는 보존한다.
std::uint32_t HtmlBodyGump::Construct(std::uint32_t token) const {
    Need(hooks_.constructBase)(token,0,0,0,1);auto& node=Node(token);node.vtable=0x50bd38;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.kind));node.text=0;node.body[0]=node.links[0]=0;
    Width(node,state_.screenWidth/2);Height(node,state_.screenHeight/2);return token;
}
// 지연 삭제 후 현재 소유 필드를0으로 만든다. 버퍼/다른 자식은 지우지 않는다.
void HtmlBodyGump::Clear(std::uint32_t token) const { if (Node(token).text) { Need(hooks_.kill)(Node(token).text);Node(token).text=0; } }
// 이미 지정된 외부 출력은 덮어쓰지 않고 본문/링크 첫NUL만 먼저 설치한다.
void HtmlBodyGump::Parse(std::uint32_t token,const HtmlBodyParserHooks& parser) const {
    Node(token).body[0]=Node(token).links[0]=0;if (!Need(parser.hasOutput)()) Need(parser.bind)(Node(token).body,Node(token).links);Need(parser.run)();
}
// 원본은 기존 자식을 Kill하되 새 생성 전까지 그 토큰을 보존한다.
void HtmlBodyGump::SetText(std::uint32_t token,std::string_view text) const {
    text=text.substr(0,text.find('\0'));if (Node(token).text) Need(hooks_.kill)(Node(token).text);
    const auto child=Need(hooks_.allocate)(0xd8);if (!child) throw std::bad_alloc();Need(hooks_.constructText)(child,0,0,5,state_.color);Node(token).text=child;
    Need(hooks_.attach)(Node(token).text,0,0,token,0);Need(hooks_.styled)(Node(token).text,text);Need(hooks_.scroll)(Node(token).text,0);Need(hooks_.slider)(Node(token).text,1);Need(hooks_.scrollable)(Node(token).text,1);
    if (!(Node(token).flags&8U)) { Need(hooks_.push)(token);Need(hooks_.attach)(token,Node(token).x,Node(token).y,0,0); }
}
// 가로 여백을 뺀 x87 계산값을 기본/배율 크기에 각각 저장한다.
void HtmlBodyGump::Fit(std::uint32_t token,float inset) const {
    if (!Node(token).text) return;Need(hooks_.push)(Node(token).text);Width(Base(Node(token).text),static_cast<double>(Node(token).baseWidth)-inset);Height(Base(Node(token).text),Node(token).baseHeight);Need(hooks_.attach)(Node(token).text,inset,0,token,0);
}
// 원본46cc30처럼 부모 부착이 끝난 뒤 현재 자식 크기를 다시 계산한다.
void HtmlBodyGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { Need(hooks_.baseAttach)(token,x,y,parent,mode);Fit(token,0); }
}
