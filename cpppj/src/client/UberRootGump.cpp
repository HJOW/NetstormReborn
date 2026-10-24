// 전체 Uber 생성자는 실제 자식을 생략하지 않고 current 포인터/등록표를 원문 위치마다 다시 읽는다.
#include "client/UberRootGump.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kVtable=0x0051503cU,kBackgroundType=0x005ca920U,kBackgroundFrame=0x005ca924U,kPanelFrame=0x005ca928U; // 실제 Uber 가상표/세 배경 전역이다.
constexpr std::uint32_t kWidth=0x005c78f0U,kHeight=0x005c78f4U,kFortGump=0x005411c8U,kTitle=0x00565e70U; // 실제 화면/TYPE/현재 메시지 저장소다.
// 선택한 원문 효과만 도달 위치에서 요구하여 부분 효과를 숨기지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Uber 루트의 실제 하위 몸체 미연결");return hook; }
// 원본 signed DWORD의 해석과 감기는 빼기를 호스트 UB 없이 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본 정수 화면 높이의 차이를 같은 하위 DWORD로 계산한다.
std::int32_t Subtract(std::uint32_t value,std::uint32_t amount) { return Signed(value-amount); }
}
// 공통 상태의 수명은 생성/교체/가상 callback보다 길어야 한다.
UberRootGump::UberRootGump(GumpCreateState& create,UberRootHooks hooks):create_(create),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Uber 루트10.78 판본 필요");
}
// 실제 소비한 토큰의 유효 주소만 요구한다. 미연결 자식을 가짜 기본노드로 대체하지 않는다.
GumpCreateNode& UberRootGump::Base(std::uint32_t token) const {
    auto* value=token ? Need(hooks_.base)(token) : nullptr;if (!value) throw std::runtime_error("Uber 루트 실제 기본노드 없음");return *value;
}
// 파생의 주소는 공통 기본 주소와 같아야 한다.
UberRootNode& UberRootGump::Node(std::uint32_t token) const {
    auto* value=token ? Need(hooks_.node)(token) : nullptr;if (!value || static_cast<GumpCreateNode*>(value)!=&Base(token)) throw std::runtime_error("Uber 루트 파생 저장소 불일치");return *value;
}
// 실제 구분선의 축/제한을 같은 공통 가상 분배 저장소에 쓴다.
HudDividerNode& UberRootGump::Divider(std::uint32_t token) const {
    auto* value=token ? Need(hooks_.divider)(token) : nullptr;if (!value || static_cast<GumpCreateNode*>(value)!=&Base(token)) throw std::runtime_error("Uber 구분선 파생 저장소 불일치");return *value;
}
// 전역을 캐시하거나 다른 모드의 호스트 상수로 바꾸지 않는다.
std::uint32_t UberRootGump::Read(std::uint32_t address) const { return Need(hooks_.read)(address); }
// 원문4437f0의C3를근거로복원한다.가짜미구현하위의성공대체가아니다.
void UberRootGump::Empty() const { if (hooks_.emptyObserver) hooks_.emptyObserver(); }
// 새 생성 중인 this도 아직 이전루트인 등록0칸으로 치환하지 않는다.
HudDockNodeView UberRootGump::DockView(std::uint32_t token) const {
    Node(token);return {[this,token](HudDockPart part) {
        if (part==HudDockPart::Root) return token;const auto& n=Node(token);
        switch (part) {
        case HudDockPart::Vertical:return n.vertical;case HudDockPart::MinimapDivider:return n.minimapDivider;case HudDockPart::ChatDivider:return n.chatDivider;
        case HudDockPart::Minimap:return n.minimap;case HudDockPart::Production:return n.production;case HudDockPart::Chat:return n.chat;case HudDockPart::Knowledge:return n.knowledge;case HudDockPart::Root:break;
        }throw std::logic_error("Uber 현재 HUD 필드 종류 오류");
    }};
}
// 같은 자식의 전체 가상 Attach를 실행하며 마지막 mode는0이다.
void UberRootGump::Attach(std::uint32_t child,float x,float y,std::uint32_t parent) const { Base(child);Need(hooks_.attach)(child,x,y,parent,0); }
// 각 등록 callback 이후 다시 현재 Uber 필드를 읽어 원문10호출을 이어간다.
void UberRootGump::Layout(std::uint32_t token) const {
    Need(hooks_.registerSlave)(Node(token).vertical,Node(token).minimapDivider,1);
    hooks_.registerSlave(Node(token).vertical,Node(token).chatDivider,4);
    hooks_.registerSlave(Node(token).vertical,Node(token).minimap,1);
    hooks_.registerSlave(Node(token).vertical,Node(token).production,1);
    hooks_.registerSlave(Node(token).vertical,Node(token).knowledge,1);
    hooks_.registerSlave(Node(token).vertical,Node(token).chat,4);
    hooks_.registerSlave(Node(token).minimapDivider,Node(token).minimap,8);
    hooks_.registerSlave(Node(token).minimapDivider,Node(token).production,2);
    hooks_.registerSlave(Node(token).minimapDivider,Node(token).knowledge,2);
    hooks_.registerSlave(Node(token).chatDivider,Node(token).chat,8);
    auto& vertical=Divider(Node(token).vertical);vertical.minX=60;vertical.maxX=130;vertical.axis|=4;
    const auto height=Read(kHeight);auto& minimap=Divider(Node(token).minimapDivider);minimap.axis|=8;minimap.minY=Subtract(height,123);minimap.maxY=Signed(height);
    auto& chat=Divider(Node(token).chatDivider);chat.axis|=8;chat.minY=25;chat.maxY=Subtract(Read(kHeight),25);
}
// 원문 생성/포인터 저장/field 쓰기/가상 부착/정리의 전체 정상 순서다.
std::uint32_t UberRootGump::Construct(std::uint32_t token) const {
    Node(token);Need(hooks_.constructBase)(token,0,0,0,1);Node(token).vtable=kVtable;
    Node(token).command=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Read(0x00540451U)))));Node(token).flags|=0x800U;
    Need(hooks_.write)(kBackgroundType,Read(kFortGump));hooks_.write(kBackgroundFrame,0);hooks_.write(kPanelFrame,2);
    Node(token).chatDivider=0;Node(token).minimapDivider=0;Node(token).vertical=0;Node(token).chat=0;
    auto next=Need(hooks_.allocate)(0x11c); // new가 바꾼 화면/TYPE/frame은 생성 인자에서 새로 읽는다.
    if (next) { const auto frame=Read(0x00542664U),type=Read(kBackgroundType);const auto height=Signed(Read(kHeight));Need(hooks_.constructDivider)(next,0,5,height,0x03400000U,type,frame); }
    Node(token).vertical=next;Attach(next,80,0,token);
    next=hooks_.allocate(0x11c);
    if (next) { const auto frame=Read(0x00542664U),type=Read(kBackgroundType);Need(hooks_.constructDivider)(next,0,80,5,0x02a00000U,type,frame); }
    Node(token).minimapDivider=next;Attach(next,0,400,token);
    next=hooks_.allocate(0x11c);
    if (next) { const auto frame=Read(kBackgroundFrame),type=Read(kBackgroundType);const auto width=Signed(Read(kWidth));Need(hooks_.constructDivider)(next,18,width,5,0x02a00000U,type,frame); }
    Node(token).chatDivider=next;Attach(next,85,static_cast<float>(Subtract(Read(kHeight),96)),token);
    Empty();Node(token).minimap=Need(hooks_.createMinimap)();Base(Node(token).minimap).flags|=0x20000U;
    auto child=Node(token).minimap;const auto scaledHeight=static_cast<float>(static_cast<double>(Base(child).scale)*128.0);Base(child).baseHeight=128;Base(child).height=scaledHeight;
    child=Node(token).minimap;const auto scaledWidth=static_cast<float>(static_cast<double>(Base(child).scale)*128.0);Base(child).baseWidth=128;Base(child).width=scaledWidth;
    Attach(Node(token).minimap,0,0,token);
    Node(token).production=Need(hooks_.createProduction)();Base(Node(token).production).flags|=0x20000U;Attach(Node(token).production,0,0,token);
    Node(token).knowledge=Need(hooks_.createKnowledge)();child=Node(token).knowledge;const auto width=static_cast<double>(Signed(Read(kWidth)));Base(child).baseWidth=static_cast<float>(width);Base(child).width=static_cast<float>(width*Base(child).scale); // FILD→FST는레지스터를좁히지않고그대로FMUL한다.
    child=Node(token).knowledge;const auto height=static_cast<double>(Signed(Read(kHeight)));Base(child).baseHeight=static_cast<float>(height);Base(child).height=static_cast<float>(height*Base(child).scale);
    Base(Node(token).knowledge).flags|=0x20000U;Attach(Node(token).knowledge,0,0,token);
    static_cast<void>(Need(hooks_.type)(Read(kFortGump)));Node(token).chat=Need(hooks_.createChat)();Base(Node(token).chat).flags|=0x20000U;
    const auto label=Need(hooks_.translate)("Chat Window");Need(hooks_.help)(Node(token).chat,label);Need(hooks_.helpWord)(Node(token).chat,"chatView");
    Empty();Attach(Node(token).chat,0,0,token);Empty();Need(hooks_.refreshTechnology)();Layout(token);
    Need(hooks_.cleanup)();child=Node(token).vertical;Need(hooks_.move)(child,Base(child).x,Base(child).y,0);
    hooks_.cleanup();child=Node(token).minimapDivider;hooks_.move(child,Base(child).x,Base(child).y,0);
    hooks_.cleanup();child=Node(token).chatDivider;hooks_.move(child,Base(child).x,Base(child).y,0);hooks_.cleanup();
    if ((Read(kTitle)&0xffU)!=0) {
        next=hooks_.allocate(0xb0);if (next) Need(hooks_.constructMessage)(next,Need(hooks_.savedMessage)());Node(token).message=next;
        const auto x=static_cast<float>(static_cast<double>(Signed(Read(0x00531860U))/2)-static_cast<double>(Base(next).baseWidth)*0.5);
        const auto y=static_cast<float>(static_cast<double>(Signed(Read(0x00531864U))/2)-static_cast<double>(Base(next).baseHeight)*0.5);Attach(next,x,y,token);
        if (!Need(hooks_.equalText)(hooks_.savedMessage(),"defeat") && !hooks_.equalText(hooks_.savedMessage(),"titleMenu") && !hooks_.equalText(hooks_.savedMessage(),"victory")) {
            const std::array<std::uint32_t,9> expression{0xfffffffeU,Read(0x00540cecU),0xfffffffeU,Read(0x00540cf8U),0xfffffffdU,hooks_.savedMessage(),0xfffffffcU,0x0051501cU,0}; // 원문 포인터/제어 토큰을 그대로 보관한다.
            const auto condition=Need(hooks_.formatMessage)(expression);if (Need(hooks_.testMessage)(condition)) Need(hooks_.visitMessage)(hooks_.savedMessage());
            Need(hooks_.messageEvent)(Node(token).message,Read(0x00542a04U),0,1);Need(hooks_.overlay)(Node(token).message,true);Need(hooks_.setDepth)(Node(token).message,0xffffff9cU);
        }
    }
    return token;
}
// 새 루트의 임시 등록 count는 보존하며 기존 루트의 각 next는 Push보다 먼저 캡처한다.
void UberRootGump::Replace() const {
    Empty();const auto allocated=Need(hooks_.allocate)(0xc8);const auto token=allocated ? Construct(allocated) : 0U;
    auto& node=Base(token);if (node.id>=create_.entries.size()) throw std::out_of_range("Uber 임시 등록ID 범위 오류");create_.entries[node.id]=0;create_.collection.root=create_.entries[0];node.id=0; // 같은등록0칸의호스트별칭도즉시반영한다.
    const auto root=create_.entries[0];std::unordered_set<std::uint32_t> visited;
    for (auto child=Base(root).child;child;) { // 현재 직계 자식만 옮기며 Push/Attach 중 바뀐 다음 링크로 재탐색하지 않는다.
        if (!visited.insert(child).second) throw std::logic_error("Uber 이전루트 형제 순환");const auto next=Base(child).next;
        Need(hooks_.push)(child);Attach(child,Base(child).x,Base(child).y,token);child=next;
    }
    Need(hooks_.replaceRoot)(token);
}
}
