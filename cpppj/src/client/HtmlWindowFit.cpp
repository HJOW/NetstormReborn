// FST32 저장 위치·현재 재조회·메뉴 pop과 각 가상 부착 순서를 유지한다.
#include "client/HtmlWindowFit.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::client { // 원문 HTML 전체 배치 구현이다.
namespace { // 원문 수치/선택 장치/하위DWORD 연산을 모은다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("HTML 전체 Fit 실제 장치 미연결");return hook; } // 선택한 경계에서만 요구한다.
std::int32_t Int(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 signedDWORD를 같은 비트로 읽는다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return Int(std::uint32_t(a)+std::uint32_t(b)); } // 정수 주소/배치 덧셈은 하위32비트다.
std::int32_t Sub(std::int32_t a,std::int32_t b) { return Int(std::uint32_t(a)-std::uint32_t(b)); } // signed 넘침을 만들지 않는 원문 뺄셈이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Int(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)))); } // 원문_ftol의64비트 절삭 하위DWORD다.
std::size_t Slot(std::uint32_t index) { if (index>=20) throw std::out_of_range("HTML Fit 실제20행 밖 접근");return index; } // 유효20행 밖 물리를 임의 공급하지 않는다.
}
HtmlWindowFit::HtmlWindowFit(HtmlWindowFitHooks hooks):hooks_(std::move(hooks)) { if (!hooks_.node || !hooks_.base) throw std::invalid_argument("HTML Fit 같은 실제 노드 필요"); } // 나머지 장치는 선택 경계에서 확인한다.
HtmlWindowGumpNode& HtmlWindowFit::Node(std::uint32_t token) const { auto* node=token ? hooks_.node(token) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::logic_error("HTML Fit 실제 부모 없음");return *node; } // 표시 사본을 만들지 않는다.
GumpCreateNode& HtmlWindowFit::Base(std::uint32_t token) const { auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::logic_error("HTML Fit 실제 자식 없음");return *node; } // 현재 본문/버튼/메뉴를 읽는다.
std::int32_t HtmlWindowFit::Read(std::uint32_t address) const { return Int(Need(hooks_.read)(address)); } // 현재 Client 전역을 호출 시점에 다시 읽는다.
void HtmlWindowFit::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    Node(token);float maximum{}; // 원문 local20의 초기0이다.
    for (std::uint32_t index=0;Int(index)<Int(Node(token).controlCount);++index) { const auto slot=Slot(index);Need(hooks_.fitButton)(Node(token).controls[slot].child);const auto width=Base(Node(token).controls[slot].child).baseWidth;if (!(maximum>width)) maximum=width; } // 각 측정 뒤 현재 자식/개수를 읽으며 unordered도 원문 대입 경로다.
    const auto count=Int(Node(token).controlCount);float span=static_cast<float>(double(count)*maximum);if (count>1) span=static_cast<float>(double(Int(std::uint32_t(count)*15U-15U))+span);
    Need(hooks_.push)(Node(token).body);const auto requested=Sub(Int(Node(token).requestedWidth),40);float target=span;if (!(double(span)>double(requested))) target=static_cast<float>(requested);
    Need(hooks_.fitContent)(Node(token).body,0,0);auto* measured=&Base(Node(token).body);
    if (!(measured->baseWidth<=target)) { // 원문 natural>target 또는 unordered일 때만 긴 본문·화면 제한을 수행한다.
        std::int32_t previousHeight{},previousWidth{};
        for (;;) { // FST32 폭을20씩 늘리며 현재 측정 높이/화면 제한/종횡비를 원문 순서로 판정한다.
            measured->baseWidth=target;measured->width=static_cast<float>(double(target)*measured->scale);Need(hooks_.fitHeight)(Node(token).body);
            const auto available=Read(0x5ca9b4);
            if (double(Sub(available,40))<double(target)+40) { auto& body=Base(Node(token).body);const auto width=Sub(available,80);body.baseWidth=static_cast<float>(width);body.width=static_cast<float>(double(width)*body.scale);Need(hooks_.fitHeight)(Node(token).body);break; }
            if (Node(token).requestedHeight) { auto& body=Base(Node(token).body);const auto height=Sub(Int(Node(token).requestedHeight),65);if (double(height)>body.baseHeight) { body.baseHeight=static_cast<float>(height);body.height=static_cast<float>(double(height)*body.scale); } }
            measured=&Base(Node(token).body);
            if (double(measured->baseHeight)!=double(previousHeight)) { previousHeight=Ftol(measured->baseHeight);previousWidth=Ftol(target); }
            if (double(target)-previousWidth>50) { auto& first=Base(Node(token).body);first.baseWidth=static_cast<float>(previousWidth);first.width=static_cast<float>(double(previousWidth)*first.scale);auto& second=Base(Node(token).body);second.baseHeight=static_cast<float>(previousHeight);second.height=static_cast<float>(double(previousHeight)*second.scale);break; }
            target=static_cast<float>(double(target)+20);if (!(double(measured->baseWidth)*2<double(measured->baseHeight)*4)) break;
        }
        if (double(Read(0x531860))<double(target)+40 || double(Read(0x531864))<double(Base(Node(token).body).baseHeight)+65) {
            const auto limit=Sub(Read(0x531860),40);if (double(target)>double(limit)) target=static_cast<float>(limit);
            auto& body=Base(Node(token).body);body.baseWidth=target;body.width=static_cast<float>(double(target)*body.scale);auto& second=Base(Node(token).body);const double height=double(target)*0.5;second.baseHeight=static_cast<float>(height);second.height=static_cast<float>(height*second.scale);
            if (Node(token).requestedHeight) { auto& current=Base(Node(token).body);const auto requestedHeight=Sub(Int(Node(token).requestedHeight),65);if (double(requestedHeight)>current.baseHeight) { current.baseHeight=static_cast<float>(requestedHeight);current.height=static_cast<float>(double(requestedHeight)*current.scale); } }
            Need(hooks_.slider)(Node(token).body,1);Need(hooks_.scrollable)(Node(token).body,1);Need(hooks_.requestScroll)(Node(token).body,0);
        }
    } else Need(hooks_.fitHeight)(static_cast<std::uint32_t>(Node(token).body));
    auto& root=Node(token);auto* body=&Base(root.body);const double fullWidth=double(target)+40;root.baseWidth=static_cast<float>(fullWidth);root.width=static_cast<float>(fullWidth*root.scale);const double fullHeight=double(body->baseHeight)+65;root.baseHeight=static_cast<float>(fullHeight);root.height=static_cast<float>(fullHeight*root.scale);
    const auto halfWidth=Ftol(fullWidth*0.5);auto buttonY=Ftol(double(body->baseHeight)+30);
    if (Node(token).menuColumns) {
        const auto firstToken=Need(hooks_.currentMenu)();auto* first=&Base(firstToken);const auto menuHalf=Ftol(double(first->baseWidth)*0.5);auto menuHeight=Ftol(first->baseHeight);auto menuX=Sub(halfWidth,menuHalf);if (menuX<20) menuX=20;
        if (Int(Node(token).menuColumns)>1) { menuX=Sub(menuX,menuHalf/2);Need(hooks_.attachMenu)(menuX,buttonY,token);const float oldHeight=static_cast<float>(menuHeight);const auto current=Need(hooks_.currentMenu)();if (oldHeight>Base(current).baseHeight) menuHeight=Ftol(oldHeight);else menuHeight=Ftol(Base(Need(hooks_.currentMenu)()).baseHeight);menuX=Add(menuX,Add(menuHalf,menuHalf)); }
        Need(hooks_.attachMenu)(menuX,buttonY,token);auto& current=Node(token);const double height=double(menuHeight)+current.baseHeight+10;current.baseHeight=static_cast<float>(height);current.height=static_cast<float>(height*current.scale);const auto width=Int(Node(token).menuColumns*std::uint32_t(menuHalf)*2U+40U);if (double(width)>Node(token).baseWidth) { auto& node=Node(token);node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(double(width)*node.scale); }
        buttonY=Ftol(double(buttonY)+first->baseHeight+10);Node(token).menuColumns=0;
    }
    auto buttonX=Ftol((double(Node(token).baseWidth)-span)*0.5);const float yPosition=static_cast<float>(buttonY);
    for (std::uint32_t index=0;Int(index)<Int(Node(token).controlCount);++index) { const auto slot=Slot(index);Need(hooks_.push)(Node(token).controls[slot].child);auto& button=Base(Node(token).controls[slot].child);button.baseWidth=maximum;button.width=static_cast<float>(double(maximum)*button.scale);const float xPosition=static_cast<float>(buttonX);Need(hooks_.attach)(Node(token).controls[slot].child,xPosition,yPosition,token,0);buttonX=Ftol(double(xPosition)+maximum+15); } // 각 Push/Attach 뒤 현재 행/개수를 읽는다.
    body=&Base(Node(token).body);const auto bodyX=Ftol(double(Node(token).baseWidth)-body->baseWidth)/2;Need(hooks_.attach)(Node(token).body,static_cast<float>(bodyX),20,token,0);
    if (Need(hooks_.reference)(Node(token).auxiliarySerial)) { x=static_cast<float>((double(Read(0x5c78f0))-Node(token).baseWidth)*0.5);y=static_cast<float>((double(Read(0x5c78f4))-Node(token).baseHeight)*0.5); }
    else { const auto left=Read(0x5ca9cc),right=Read(0x5ca9d4),bottom=Read(0x5ca9d8),top=Read(0x5ca9d0);x=static_cast<float>((double(Sub(right,left))-Node(token).baseWidth)*0.5+left);y=static_cast<float>((double(Sub(bottom,top))-Node(token).baseHeight)*0.5+Read(0x5ca9d0)); }
    Need(hooks_.attachBase)(token,x,y,parent,mode);Need(hooks_.addDepth)(token,0xffffffb0U-Node(token).depth);
}
}
