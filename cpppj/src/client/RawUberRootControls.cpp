// 알고리즘을 반복하지 않고 원문의 전체 생성/도킹/드래그/공통 수명을 연결한다.
#include "client/RawUberRootControls.h"
#include "client/RawMenuControls.h"
#include "client/RawBackgroundGifControls.h"
#include "client/GumpBackgroundDraw.h"
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 모든 파생은 같은 공통 토큰/등록표를 사용한다.
struct RawUberRootControls::Impl {
    RawMenuTree common;RawMenuControls& menus;RawUberRootContext context; // 실제 장기 소유자를 빌린다.
    std::map<std::uint32_t,std::unique_ptr<UberRootNode>> roots; // 새/이전/부분 생성 루트의 실제 저장소다.
    std::map<std::uint32_t,std::unique_ptr<HudDividerNode>> dividers; // 세대별 실제 구분선이며 scalar까지 주소를 유지한다.
    std::int8_t dividerKind{55};std::uint32_t drawRegistration{}; // 보호PE00540452의 실제 종류와 자기 출력 대여다.
    HudDockHooks dockHooks;std::unique_ptr<HudDividerGump> divider; // 공통 기본 호출과 실제 구분선 생성/부착이다.
    std::unique_ptr<HudDividerInput> input;std::unique_ptr<UberRootGump> root; // 전체 원문 부모와 실제 드래그를 소유한다.
    std::int32_t previousWidth{},previousHeight{}; // 새 Windows 화면 장치 전환의 직전 크기 관측이며 원문 게임 상태를 대신하지 않는다.
    // 현재 저장소에서만 조회하며 다른 파생을 잘못 캐스팅하지 않는다.
    UberRootNode* Root(std::uint32_t token) const { const auto found=roots.find(token);return found==roots.end() ? nullptr : found->second.get(); }
    // 구분선의 같은 상속 주소를 반환한다.
    HudDividerNode* Divider(std::uint32_t token) const { const auto found=dividers.find(token);return found==dividers.end() ? nullptr : found->second.get(); }
    // 현재 등록표에서 실제 serial/비dead를 검색하고 이전 세대로 fallback하지 않는다.
    std::uint32_t Reference(std::uint32_t serial) const {
        for (std::size_t index=1;index<common.create.entries.size();++index) { // 원문은 루트0을 제외한 고정999칸을 읽는다.
            const auto token=common.create.entries[index];const auto* node=token ? common.node(token) : nullptr;
            if (node && node->serial==serial && !(node->flags&0x10000U)) return token;
        }return 0;
    }
    // 원문 요청의 실제 파생을 확보하고 생성 전에 공통 조회가 찾을 수 있게 한다.
    std::uint32_t Allocate(std::size_t bytes) {
        if (bytes==0xb0) { auto* gif=menus.BackgroundGifs();if (!gif) throw std::logic_error("Uber 실제 배경 GIF 공장 미설치");return gif->Allocate(); }
        const auto token=common.token();
        if (bytes==0xc8) roots.emplace(token,std::make_unique<UberRootNode>());
        else if (bytes==0x11c) dividers.emplace(token,std::make_unique<HudDividerNode>());
        else throw std::logic_error("Uber 실제 파생 할당 크기 오류");return token;
    }
    // 호출 시점의 해당 실제 this 필드를 읽는 dock를 짧게 빌린다.
    HudDock Dock(std::uint32_t token) const { return HudDock(root->DockView(token),context.globals,dividerKind,dockHooks); }
    // 원문 자식과 공통 장치를 한 번 묶고 실제 생성은 공개 Replace까지 미룬다.
    Impl(RawMenuTree tree,RawMenuControls& owner,RawUberRootContext supplied):common(std::move(tree)),menus(owner),context(std::move(supplied)),dockHooks(context.dock) {
        dockHooks.node=common.node;dockHooks.divider=[this](auto token) { return Divider(token); };
        dockHooks.constructBackground=[this](auto token,float width,float height,auto flags,auto type,auto frame) { menus.BackgroundBackend().Construct(token,width,height,flags,1,type,frame); };
        dockHooks.activate=[this](auto token,auto enabled) { common.lifetime.SetOverlay(token,enabled!=0); };
        dockHooks.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
        dockHooks.push=[this](auto token) { common.tree.Push(token); };dockHooks.attach=common.attach;
        dockHooks.move=[this](auto token,float x,float y,auto parent) { common.tree.Move(token,x,y,parent); };
        dockHooks.resolveSerial=[this](auto serial) { return Reference(serial); };
        dockHooks.bubble=[this](auto token,auto event,auto first,auto second) { common.events.Bubble(token,event,first,second); };
        dockHooks.closePopups=[this] { menus.RequestClosePopups(); };
        divider=std::make_unique<HudDividerGump>(common.create.screenWidth,common.create.screenHeight,dividerKind,dockHooks);
        HudDividerInputHooks drag;drag.divider=dockHooks.divider;drag.base=common.node;drag.cursor=context.cursor;drag.poll=context.poll;
        drag.baseHandle=[this](auto token,auto event,auto first,auto second) { return menus.BaseHandle(token,event,first,second); };
        drag.menuInstances=[this] { return menus.MenuInstances(); };input=std::make_unique<HudDividerInput>(common.collection,common.input,common.tree,std::move(drag));
        auto effects=context.root;effects.node=[this](auto token) { return Root(token); };effects.divider=dockHooks.divider;effects.base=common.node;
        effects.allocate=[this](auto bytes) { return Allocate(bytes); };effects.constructBase=[this](auto token,auto kind,float width,float height,auto flags) { common.factory.Construct(token,kind,width,height,flags); };
        effects.constructDivider=[this](auto token,auto axis,auto width,auto height,auto flags,auto type,auto frame) { divider->Construct(token,axis,width,height,flags,type,frame); };
        effects.attach=common.attach;effects.move=dockHooks.move;effects.push=dockHooks.push;
        effects.registerSlave=[this](auto master,auto slave,auto flags) { divider->RegisterSlave(master,slave,flags); };
        effects.cleanup=[this] { common.lifetime.Cleanup(); };effects.overlay=[this](auto token,bool enabled) { common.lifetime.SetOverlay(token,enabled); };
        effects.setDepth=[this](auto token,auto depth) { common.tree.SetDepth(token,depth); };
        effects.replaceRoot=[this](auto token) { common.lifetime.ReplaceRoot(token);if (!context.replaced) throw std::logic_error("Uber 실제 루트 별칭 반납 미연결");context.replaced(token);common.changed(); };
        root=std::make_unique<UberRootGump>(common.create,std::move(effects));
        drawRegistration=menus.RegisterExternalGumps({{},[this](auto token) { return Root(token) || Divider(token); },[this](auto token,auto&,auto&,auto& draw) { if (Divider(token)) draw.Draw(token); }});
    }
};
RawUberRootControls::RawUberRootControls(RawMenuTree tree,RawMenuControls& menus,RawUberRootContext context):impl_(std::make_unique<Impl>(std::move(tree),menus,std::move(context))) {} // 실제 고정 소유자만 만든다.
RawUberRootControls::~RawUberRootControls() {
    auto& s=*impl_;
    while (!s.roots.empty()) { const auto token=s.roots.begin()->first;if (s.common.create.entries[s.Root(token)->id]==token) { s.common.lifetime.Kill(token);s.common.lifetime.Cleanup(); }else s.roots.erase(token); } // 실패한 부분 생성의 미부착 부모도 실제 자식 우선 수명으로 반납한다.
    while (!s.dividers.empty()) { const auto token=s.dividers.begin()->first;if (s.common.create.entries[s.Divider(token)->id]==token) { s.common.lifetime.Kill(token);s.common.lifetime.Cleanup(); }else s.dividers.erase(token); } // 기본 생성 전 할당만 등록 없이 직접 반납한다.
    s.menus.UnregisterExternalGumps(s.drawRegistration);
} // 살아 있는 다른 HUD의 출력은 반납하지 않는다.
void RawUberRootControls::Replace() { auto& s=*impl_;s.root->Replace();s.previousWidth=s.common.create.screenWidth;s.previousHeight=s.common.create.screenHeight; } // 전체 생성자와 원문 교체 본체를 그대로 호출한다.
// 새로운 화면 크기의 확장은 원본 세 구분선의 같은 Move/Attach/슬레이브 몸체로 처리한다.
void RawUberRootControls::Reflow() {
    auto& s=*impl_;if (!Current()) throw std::logic_error("실제 전체 HUD 없는 화면 재배치");
    const auto width=s.common.create.screenWidth,height=s.common.create.screenHeight;if (width==s.previousWidth && height==s.previousHeight) return;
    const auto token=s.common.create.entries[0];auto* node=s.Root(token);const auto delta=static_cast<double>(height)-s.previousHeight;
    const auto miniY=static_cast<float>(s.Divider(node->minimapDivider)->y+delta),chatY=static_cast<float>(s.Divider(node->chatDivider)->y+delta); // 사용자 드래그/숨김 뒤의 현재 하단 거리를 유지한다.
    auto* vertical=s.Divider(node->vertical);vertical->baseHeight=static_cast<float>(height);vertical->height=static_cast<float>(static_cast<double>(height)*vertical->scale);
    auto* mini=s.Divider(node->minimapDivider);mini->minY=height-123;mini->maxY=height;auto* chat=s.Divider(node->chatDivider);chat->maxY=height-25;
    s.common.lifetime.Cleanup();s.common.tree.Move(node->vertical,vertical->x,vertical->y,0);s.common.lifetime.Cleanup();
    s.common.tree.Move(node->minimapDivider,s.Divider(node->minimapDivider)->x,miniY,0);s.common.lifetime.Cleanup();
    s.common.tree.Move(node->chatDivider,s.Divider(node->chatDivider)->x,chatY,0);s.common.lifetime.Cleanup();
    s.previousWidth=width;s.previousHeight=height;if (!s.context.dock.finishResize) throw std::logic_error("실제 화면 재배치 후처리 미연결");s.context.dock.finishResize();s.common.changed();
}
GumpCreateNode* RawUberRootControls::Node(std::uint32_t token) const { if (auto* node=impl_->Root(token)) return node;return impl_->Divider(token); } // 동일 기본 주소다.
GumpBackgroundNode* RawUberRootControls::Background(std::uint32_t token) const { return impl_->Divider(token); } // 같은 실제 구분선 배경이다.
bool RawUberRootControls::Current() const {
    const auto& s=*impl_;const auto* node=s.Root(s.common.create.entries[0]);if (!node || (node->flags&0x10000U)) return false;
    for (const auto token:{node->vertical,node->minimapDivider,node->chatDivider,node->minimap,node->production,node->knowledge,node->chat}) { // 부분 생성/이미 반납된 자식을 완성한 HUD로 판정하지 않는다.
        const auto* child=token ? s.common.node(token) : nullptr;if (!child || (child->flags&0x10000U)) return false;
    }return true;
}
void RawUberRootControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { auto& s=*impl_;if (s.Divider(token)) s.divider->Attach(token,x,y,parent,mode);else if (s.Root(token)) s.common.tree.Attach(token,x,y,parent,mode);else throw std::out_of_range("Uber 실제 부착 대상 없음"); } // 실제 가상 분배다.
std::uint32_t RawUberRootControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
    auto& s=*impl_;if (s.Divider(token)) return s.input->Handle(token,event,first,second);if (!s.Root(token)) throw std::out_of_range("Uber 실제 입력 대상 없음");
    s.dockHooks.baseHandle=[&s,token](auto code,auto a,auto b) { return s.menus.BaseHandle(token,code,a,b); };return s.Dock(token).Handle(event,first,second); // 같은 this의 기본 Help로만 위임한다.
}
void RawUberRootControls::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) { if (impl_->Divider(token)) impl_->input->Broadcast(token,event,first,second); } // 루트의 기본 방송은 빈 본문이다.
void RawUberRootControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { impl_->common.lifetime.DeleteScalar(token,flags); } // 두 파생의 원문 scalar는 기본 소멸이다.
void RawUberRootControls::Release(std::uint32_t token) { impl_->roots.erase(token);impl_->dividers.erase(token); } // 실제 반납 뒤만 저장소를 지운다.
// 고정 화면 좌표를 가정하지 않고 같은 현재 파생의 표시/사각형만 조회한다.
std::optional<ScreenPoint> RawUberRootControls::Point(std::string_view label) const {
    const auto& s=*impl_;if (!Current()) return {};const auto* root=s.Root(s.common.create.entries[0]);std::uint32_t token{};
    if (label=="hud:vertical") token=root->vertical;else if (label=="hud:minimap-divider") token=root->minimapDivider;else if (label=="hud:chat-divider") token=root->chatDivider;else return {};
    if (!(s.Divider(token)->flags&1U)) return {};const auto bounds=s.common.collection.Bounds(token);return ScreenPoint{(bounds.left+bounds.right)/2,(bounds.top+bounds.bottom)/2};
}
std::string RawUberRootControls::Report() const {
    const auto& s=*impl_;const auto& g=s.context.globals;std::ostringstream out;
    out<<"raw_uber\t"<<s.common.create.entries[0]<<'\t'<<s.roots.size()<<'\t'<<s.dividers.size()<<'\t'<<g.mapRect.left<<'\t'<<g.mapRect.top<<'\t'<<g.mapRect.right<<'\t'<<g.mapRect.bottom<<'\n';
    for (const auto& [token,node]:s.roots) out<<"raw_uber_children\t"<<token<<'\t'<<node->vertical<<'\t'<<node->minimapDivider<<'\t'<<node->chatDivider<<'\t'<<node->minimap<<'\t'<<node->production<<'\t'<<node->knowledge<<'\t'<<node->chat<<'\n'; // 실제 필드만 관측한다.
    for (const auto& [token,node]:s.dividers) out<<"raw_hud_divider\t"<<token<<'\t'<<node->serial<<'\t'<<node->x<<'\t'<<node->y<<'\t'<<node->baseWidth<<'\t'<<node->baseHeight<<'\t'<<node->axis<<'\t'<<node->minX<<'\t'<<node->maxX<<'\t'<<node->minY<<'\t'<<node->maxY<<'\t'<<node->flags<<'\n'; // 드래그/제한/화면 전환의 실제 현재 상태다.
    return out.str();
}
}
