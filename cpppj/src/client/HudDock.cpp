// 원본의 현재 노드 재조회와 float 저장 위치를 보존하는 HUD 배치 본체다.
#include "client/HudDock.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 실제10.78 divider 가상 표와 HUD 명령의 원시 값이다.
constexpr std::uint32_t kDividerVtable=0x00514fd8,kMoved=0x0200003a,kResized=0x02000046,
    kHudMode=0x0200003d,kChatSize=0x02000005,kChatToggle=0x02000088;
// 원본 signed64 _ftol의 하위DWORD를 호스트 signed넘침 없이 읽는다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원본 DWORD 합산의 하위32BIT다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 원본 DWORD 뺄셈의 하위32BIT다.
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// FST field→FMUL scale의 순서이므로 저장된float32를 다시 곱하지 않는다.
void Width(GumpCreateNode& node,double value) { node.baseWidth=static_cast<float>(value);node.width=static_cast<float>(value*node.scale); }
// 세로도 FST 후 아직 확장 정밀도인 값을 현재scale에 곱한다.
void Height(GumpCreateNode& node,double value) { node.baseHeight=static_cast<float>(value);node.height=static_cast<float>(value*node.scale); }
}
// 기본 저장소와 재배치에 필요한 실제 공통 처리기만 처음에 확인한다.
HudDock::HudDock(HudDockNodes& nodes,HudDockGlobals globals,const std::int8_t& color,HudDockHooks hooks)
    :HudDock(HudDockNodeView{[&nodes](HudDockPart part) {
        switch (part) {
        case HudDockPart::Root:return nodes.root;case HudDockPart::Vertical:return nodes.vertical;case HudDockPart::MinimapDivider:return nodes.minimapDivider;case HudDockPart::ChatDivider:return nodes.chatDivider;
        case HudDockPart::Minimap:return nodes.minimap;case HudDockPart::Production:return nodes.production;case HudDockPart::Chat:return nodes.chat;case HudDockPart::Knowledge:return nodes.knowledge;
        }throw std::logic_error("HUD 현재 필드 종류 오류");
    }},globals,color,std::move(hooks)) {}
// 실제 부모의 현재 필드 조회도 기존8노드 몸체와 같은 소유권/가상 분배를 사용한다.
HudDock::HudDock(HudDockNodeView nodes,HudDockGlobals globals,const std::int8_t& color,HudDockHooks hooks)
    :nodes_(std::move(nodes)),globals_(globals),color_(color),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.divider || !hooks_.baseAttach || !hooks_.push || !hooks_.attach || !hooks_.move ||
        !hooks_.resolveSerial || !hooks_.bubble || !hooks_.closePopups) throw std::invalid_argument("HUD의 실제 노드/트리 처리기가 필요합니다");
}
// 필드가 선택된 위치에서만 조회를 요구하므로 구분선 생성에 다른 패널을 미리 요구하지 않는다.
std::uint32_t HudDock::Token(HudDockPart part) const { if (!nodes_.token) throw std::logic_error("HUD 실제 부모 필드 미연결");return nodes_.token(part); }
// 원본 유효 포인터의 수명 계약을 토큰 경계에서 검사한다.
GumpCreateNode& HudDock::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("HUD Gump 저장소 없음");return *node;
}
// 제품 투영이 별도 복사본을 반환하지 않도록 같은 상속 주소를 요구한다.
HudDividerNode& HudDock::Divider(std::uint32_t token) const {
    auto* node=token ? hooks_.divider(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=&Node(token)) throw std::runtime_error("HUD divider의 동일 저장소 필요");return *node;
}
// 기본 배경 생성의 signed정수→float32 변환 뒤 원본 확장 필드만 초기화한다.
std::uint32_t HudDock::Construct(std::uint32_t token,std::uint32_t axis,std::int32_t width,std::int32_t height,
    std::uint32_t backgroundFlags,std::uint32_t type,std::uint32_t frame) const {
    if (!hooks_.constructBackground || !hooks_.activate) throw std::runtime_error("HUD divider 생성 경계 없음");
    hooks_.constructBackground(token,static_cast<float>(width),static_cast<float>(height),backgroundFlags,type,frame);
    auto& node=Divider(token);node.vtable=kDividerVtable;node.flags|=0x60200;node.command=static_cast<std::int32_t>(color_);node.axis=axis;
    // 기존 제한4개는 보존하며 원본20DWORD 슬레이브 영역만 지운다.
    for (auto& slave:node.slaves) slave={0,0};
    hooks_.activate(token,1);return token;
}
// 원본은 꽉 찬 표의 진단이 복귀해도 자식에slave표시를 남긴다.
void HudDock::RegisterSlave(std::uint32_t master,std::uint32_t slave,std::uint32_t flags) const {
    if (!slave) return;auto& divider=Divider(master);std::size_t index=0;
    // serial0은 빈 슬롯이며 동일 serial의 중복 등록을 허용한다.
    for (;index<divider.slaves.size();++index) if (!divider.slaves[index].first) break;
    if (index==divider.slaves.size()) {
        if (!hooks_.assertion) throw std::runtime_error("HUD slave 한도 진단 경계 없음");hooks_.assertion(83);
    } else divider.slaves[index]={Node(slave).serial,flags};
    Node(slave).flags|=0x40000;
}
// 첫popup정리 뒤 oldY를 저장하며 기본부착과 방송 뒤에는 현재 좌표/관계를 다시 읽는다.
void HudDock::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    hooks_.closePopups();auto& master=Divider(token);const float oldY=master.y;
    hooks_.baseAttach(token,x,y,parent,mode);if (!(master.flags&1)) return;
    if (master.axis&0x10) Width(master,double(globals_.screenWidth)-x);
    hooks_.bubble(token,kMoved,master.serial,0);
    // 고정10번을 순회하지만 매번 현재 serial을 읽으며 해결한 slave포인터는 네 관계 분기에서 유지한다.
    for (std::size_t index=0;index<master.slaves.size();++index) {
        if (!master.slaves[index].first) continue;const auto slave=hooks_.resolveSerial(master.slaves[index].first);if (!slave) continue;
        auto& child=Node(slave);const float preservedY=static_cast<float>(double(y)-(double(oldY)-child.y));
        if (master.slaves[index].second&1) {
            hooks_.push(slave);Width(child,double(master.x)-child.x);hooks_.attach(slave,child.x,child.y,0,1);
        }
        if (master.slaves[index].second&2) {
            hooks_.push(slave);
            if (master.slaves[index].second&0x10) Height(child,double(master.y)-child.y);
            if (!(master.slaves[index].second&0x20)) { Height(child,double(master.y)-child.y);hooks_.attach(slave,child.x,child.y,0,1); }
            else hooks_.attach(slave,child.x,preservedY,0,1);
        }
        if (master.slaves[index].second&4) {
            hooks_.push(slave);const double right=double(master.baseWidth)+master.x; // 두 필드 합은 slave폭 계산이 끝날 때까지float32로 줄이지 않는다.
            Width(child,double(globals_.screenWidth)-right);hooks_.attach(slave,static_cast<float>(right),child.y,0,1);
        }
        if (master.slaves[index].second&8) {
            hooks_.push(slave);const double bottom=double(master.baseHeight)+master.y; // 확장 합과 실제Attach의float32 인자를 구별한다.
            Height(child,(master.slaves[index].second&0x40) ? (double(oldY)-master.y)+child.baseHeight : double(globals_.screenHeight)-bottom);
            hooks_.attach(slave,child.x,static_cast<float>(bottom),0,1);
        }
    }
}
// 원문HUD 직계의 실제자식생성 뒤 등록 순서와 제한값을 그대로 쓴다.
void HudDock::RegisterNormalLayout() const {
    RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::MinimapDivider),1);RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::ChatDivider),4);
    RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::Minimap),1);RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::Production),1);
    RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::Knowledge),1);RegisterSlave(Token(HudDockPart::Vertical),Token(HudDockPart::Chat),4);
    RegisterSlave(Token(HudDockPart::MinimapDivider),Token(HudDockPart::Minimap),8);RegisterSlave(Token(HudDockPart::MinimapDivider),Token(HudDockPart::Production),2);
    RegisterSlave(Token(HudDockPart::MinimapDivider),Token(HudDockPart::Knowledge),2);RegisterSlave(Token(HudDockPart::ChatDivider),Token(HudDockPart::Chat),8);
    auto& vertical=Divider(Token(HudDockPart::Vertical));vertical.minX=60;vertical.maxX=130;vertical.axis|=4;
    auto& minimap=Divider(Token(HudDockPart::MinimapDivider));minimap.minY=Subtract(globals_.screenHeight,123);minimap.maxY=globals_.screenHeight;minimap.axis|=8;
    auto& chat=Divider(Token(HudDockPart::ChatDivider));chat.minY=25;chat.maxY=Subtract(globals_.screenHeight,25);chat.axis|=8;
}
// 표시모드와divider사건이 실제HUD의현재직계자식/전역을 통해 같은viewport를 갱신한다.
std::uint32_t HudDock::Handle(std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const auto command=event&0x03ffffff; // 원본은release비트도 제거한 뒤 이 몸체를 처리한다.
    if (command==kChatToggle) {
        if (!hooks_.chatVisible || !hooks_.chatInput) throw std::runtime_error("HUD의 실제 채팅 설정 경계 없음");
        if (second && (second!=2 || double(globals_.screenHeight)<=Node(Token(HudDockPart::ChatDivider)).y)) {
            hooks_.chatVisible(true);
            if (double(globals_.screenHeight)<=Node(Token(HudDockPart::ChatDivider)).y) {
                hooks_.push(Token(HudDockPart::ChatDivider));const auto token=Token(HudDockPart::ChatDivider);hooks_.attach(token,Node(token).x,globals_.savedChatY,Token(HudDockPart::Root),0);
            }
        } else {
            hooks_.chatVisible(false);
            if (Node(Token(HudDockPart::ChatDivider)).y<double(globals_.screenHeight)) globals_.savedChatY=Node(Token(HudDockPart::ChatDivider)).y;
            hooks_.chatInput(false);hooks_.push(Token(HudDockPart::ChatDivider));const auto token=Token(HudDockPart::ChatDivider);
            hooks_.attach(token,Node(token).x,static_cast<float>(globals_.screenHeight),Token(HudDockPart::Root),0);
        }
    }
    if (command==kMoved || command==kResized) {
        if (!hooks_.invalidate) throw std::runtime_error("HUD dirty 경계 없음");
        if (Token(HudDockPart::Vertical) && first==Node(Token(HudDockPart::Vertical)).serial) {
            const auto& node=Node(Token(HudDockPart::Vertical));const auto left=Truncate(double(node.x)+node.baseWidth);const auto height=globals_.screenHeight;
            globals_.mapWidth=Subtract(globals_.screenWidth,left);globals_.mapLeft=left;
            hooks_.invalidate(0,0,Add(left,1),height,0);
            hooks_.invalidate(globals_.mapLeft,globals_.mapHeight,globals_.mapWidth,Subtract(globals_.screenHeight,globals_.mapHeight),0);
        }
        if (Token(HudDockPart::ChatDivider) && first==Node(Token(HudDockPart::ChatDivider)).serial) {
            const auto height=Truncate(Node(Token(HudDockPart::ChatDivider)).y);globals_.mapHeight=height;
            hooks_.invalidate(globals_.mapLeft,height,globals_.mapWidth,Subtract(globals_.screenHeight,height),0);
        }
        const auto left=globals_.mapLeft,top=globals_.mapTop,right=Add(left,globals_.mapWidth),bottom=Add(top,globals_.mapHeight); // 레지스터/스택에 먼저 고정한 정수LTRB다.
        globals_.mapRect={left,top,right,bottom};globals_.mapFloatRect={static_cast<float>(left),static_cast<float>(top),static_cast<float>(right),static_cast<float>(bottom)};
        if (Token(HudDockPart::MinimapDivider)) {
            const auto& node=Node(Token(HudDockPart::MinimapDivider));globals_.leftBottomRect={0,Truncate(node.y),Truncate(node.baseWidth),globals_.screenHeight};
        } else globals_.leftBottomRect={};return 1;
    }
    if (command!=kHudMode) {
        if (command==kChatSize) {
            const float y=static_cast<float>(Subtract(Subtract(globals_.screenHeight,std::bit_cast<std::int32_t>(second)),5));
            hooks_.push(Token(HudDockPart::ChatDivider));const auto token=Token(HudDockPart::ChatDivider);hooks_.attach(token,Node(token).x,y,Token(HudDockPart::Root),0);return 1;
        }
        if (!hooks_.baseHandle) throw std::runtime_error("HUD 기본 입력 경계 없음");return hooks_.baseHandle(event,first,second);
    }
    if (first==1 || first==3) {
        Node(Token(HudDockPart::Vertical)).flags|=1;Node(Token(HudDockPart::MinimapDivider)).flags|=1;Node(Token(HudDockPart::Minimap)).flags|=1;Node(Token(HudDockPart::Production)).flags|=1;Node(Token(HudDockPart::Knowledge)).flags&=~1U;
        hooks_.move(Token(HudDockPart::Vertical),(first==3 || globals_.showHud) ? 80.0F : -Node(Token(HudDockPart::Vertical)).baseWidth,0,0);
        hooks_.move(Token(HudDockPart::MinimapDivider),0,static_cast<float>(Subtract(globals_.screenHeight,80)),0);
    } else if (first==4) {
        Node(Token(HudDockPart::Production)).flags&=~1U;Node(Token(HudDockPart::Knowledge)).flags|=1;Node(Token(HudDockPart::Minimap)).flags&=~1U;
        hooks_.move(Token(HudDockPart::Vertical),170,0,0);hooks_.move(Token(HudDockPart::MinimapDivider),0,static_cast<float>(globals_.screenHeight),0);
    }
    hooks_.push(Token(HudDockPart::ChatDivider));const auto token=Token(HudDockPart::ChatDivider);hooks_.attach(token,Node(token).x,Node(token).y,0,0);
    ++globals_.fullRefresh;if (!hooks_.finishResize) throw std::runtime_error("HUD 화면 후처리 경계 없음");hooks_.finishResize();return 0;
}
}
