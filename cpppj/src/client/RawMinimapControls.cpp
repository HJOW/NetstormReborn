// 원본 셀 그림·crop·가상 입력을 같은 실제 Gump 수집/정렬/픽셀에 조립한다.
#include "client/RawMinimapControls.h"
#include "client/RawCampaignSession.h"
#include <algorithm>
#include <bit>
#include <sstream>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 생산0번과 독립적으로 반납하는 공통 메뉴 외부 출력 슬롯이다.
constexpr std::uint32_t kDrawSlot=1,kMinimapVtable=0x0050d51c;
// 현재 전체 생성자에 필요한 실제 물리만 요구하며 빈 대체 월드를 만들지 않는다.
std::shared_ptr<RawSurfaceStorage> RequireStorage(std::shared_ptr<RawSurfaceStorage> storage) {
    if (!storage || storage->pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("미니맵의 실제10.78 물리 저장소 필요");return storage;
}
}
// 파생 기본 노드와 bitmap/큐/원본 처리기는 움직이지 않는 한 주소에서 서로 참조한다.
struct RawMinimapControls::Impl {
    RawMenuTree common;RawMenuControls& menus;RawMinimapHooks hooks; // 실제 공통 처리기와 현재 카메라 장치다.
    std::shared_ptr<RawSurfaceStorage> storage;o::SquidPop* pop{};o::SquidUnpop* unpop{}; // 물리는 소유하고 현재 월드의 통지만 짧게 빌린다.
    GumpCreateNode node;std::uint32_t token{}; // 실제 공통 등록된 기본 주소다.
    MinimapGeometryState geometry;o::MetadisplayDirtyState& state;o::MinimapRasterState colors; // 원본 crop/bitmap/큐/색 전역이다.
    o::RawMinimapRaster raster;o::RawMetadisplayDirty dirty; // 실제 같은Pool의 기존 공간 통지와 셀 그림이다.
    bool modified{}; // 현재Refresh가 실제 redraw/셀 쓰기를 수행했는지다.
    // 연결 훅은 실제 호출 시점에 현재 카메라·모드·프레임을 조회한다.
    Impl(RawMenuTree tree,RawMenuControls& menu,std::shared_ptr<RawSurfaceStorage> data,o::MetadisplayDirtyState& shared,RawMinimapHooks supplied)
        :common(std::move(tree)),menus(menu),hooks(std::move(supplied)),storage(RequireStorage(std::move(data))),state(shared),raster(storage->pool,storage->hash,storage->types,colors),
        dirty(storage->pool,state,{[this] { SyncSize();if (RebuildMinimap(geometry,hooks.view())) { common.changed();modified=true; } },
            [this](auto x,auto y) { colors.challenge=storage->ownerMode.challenge;colors.frame=state.frames;raster.DrawCell(state.bitmap,x,y);modified=true; }}) {
        if (!hooks.view || !hooks.center || !hooks.move || !hooks.inputMode || !hooks.poll) throw std::invalid_argument("실제 미니맵 장치 연결 누락");
        menus.SetExternalGumps(kDrawSlot,{{},[this](auto value) { return value && value==token; },[this](auto,auto& target,auto&,auto&) { Draw(target); }});
    }
    // 노드의 실제 현재 크기를 기하 몸체에 전달하며 별도 UI 크기를 누적하지 않는다.
    void SyncSize() { geometry.width=node.baseWidth;geometry.height=node.baseHeight; }
    // 원본 clipGump 안에서 crop 블릿→양 끝 포함 카메라 사각형을 같은8BIT 픽셀로 합성한다.
    void Draw(GumpPixelTarget& target) {
        if (state.bitmap.empty()) return;SyncSize();const auto plan=PlanMinimapDraw(geometry,hooks.view(),common.collection.Position(token)); // 같은 실제 화면 원점이다.
        const auto bounds=common.collection.Bounds(token); // bitmap의128폭과 구별되는 실제 전체 Gump 경계다.
        const auto sourceBase=std::int64_t(plan.source.y)*128+plan.source.x; // 원본의 평면 BYTE 오프셋이다.
        target.RawBitmap(state.bitmap,128,sourceBase,plan.size,plan.destination,bounds);
        const auto left=plan.camera.left,top=plan.camera.top,right=plan.camera.right-1,bottom=plan.camera.bottom-1; // 원본 외곽선은 화면 절대 좌표를 쓴다.
        const std::array<GumpLine,4> lines{{{{left,top},{right,top},colors.owners[2]},{{left,bottom},{right,bottom},colors.owners[2]},{{left,top},{left,bottom},colors.owners[2]},{{right,top},{right,bottom},colors.owners[2]}}}; // 원본005bec2c 흰색의 위/아래/왼쪽/오른쪽 순서다.
        target.LinesClipped(lines,bounds);
    }
};
// 생성 전에 같은Session 참조와 실제 장치 경계를 확인한다.
RawMinimapControls::RawMinimapControls(RawMenuTree tree,RawMenuControls& menus,RawCampaignSession& session,o::MetadisplayDirtyState& state,RawMinimapHooks hooks)
    :RawMinimapControls(std::move(tree),menus,session.World().Storage(),state,std::move(hooks)) { BindWorld(session.Core()); }
// 초기 HUD도 같은 실제 물리를 사용하며 Session 참조나 Core 사본을 보관하지 않는다.
RawMinimapControls::RawMinimapControls(RawMenuTree tree,RawMenuControls& menus,std::shared_ptr<RawSurfaceStorage> storage,o::MetadisplayDirtyState& state,RawMinimapHooks hooks)
    :impl_(std::make_unique<Impl>(std::move(tree),menus,std::move(storage),state,std::move(hooks))) {}
// 실제 같은 Pool/공간 자료를 확인한 뒤에만 현재 모듈의 통지를 설치한다.
void RawMinimapControls::BindWorld(RawWorldCore core) {
    auto& s=*impl_;if (s.pop || s.unpop || &core.pool!=&s.storage->pool || &core.hash!=&s.storage->hash || &core.unpop.Pool()!=&s.storage->pool || &core.ownerMode!=&s.storage->ownerMode || core.types.data()!=s.storage->types.data() || core.spots.data()!=s.storage->spots.data()) throw std::invalid_argument("미니맵의 현재 월드 물리/대여 불일치");
    s.pop=&core.pop;s.unpop=&core.unpop;s.pop->SetMetadisplayDirty(&s.dirty);s.unpop->SetMetadisplayDirty(&s.dirty);s.state.fullRefresh=1;
}
// 통지의 대여를 먼저 끊으며 실제 물리 배열과 bitmap은 유지한다.
void RawMinimapControls::UnbindWorld() { auto& s=*impl_;if (s.pop) s.pop->SetMetadisplayDirty(nullptr);if (s.unpop) s.unpop->SetMetadisplayDirty(nullptr);s.pop=nullptr;s.unpop=nullptr; }
// 외부 Draw와 Pop 통지의 참조를 같은Session 반납 전에 끊는다.
RawMinimapControls::~RawMinimapControls() { Close();UnbindWorld();impl_->menus.SetExternalGumps(kDrawSlot,{}); }
// 제목/Help 문자열은 기존 공통 메뉴의 안정된 정적 토큰을 사용한다.
void RawMinimapControls::Open() {
    auto& s=*impl_;if (s.token) throw std::logic_error("현재 미니맵 중복 생성");s.token=s.common.token();s.common.factory.Construct(s.token,0,128,128,0x301);s.node.vtable=kMinimapVtable;s.node.command=29;s.node.textPointer=0x61000004;s.node.alternateTextPointer=0x61000005;s.geometry={};
    s.dirty.Refresh({});s.state.displayPresent=true;s.state.displayFlags=static_cast<std::uint8_t>(s.node.flags);s.state.fullRefresh=1;
    Attach(s.token,0,0,s.common.create.collection.root,0);s.node.flags|=0x20000;
}
// 공통Kill이Idle/캡처/등록표를 함께 처리하며 독립bitmap은 처리기와 같은 수명이다.
void RawMinimapControls::Close() { auto& s=*impl_;if (s.token) { s.common.lifetime.Kill(s.token);s.common.lifetime.Cleanup(); }s.state.displayPresent=false; }
// 원본 mode3의 세로분할x80/가로분할H-80/선두께5가 만드는 기본 영역이다. 전체dock 드래그는 별도 복원 경계다.
void RawMinimapControls::Resize(int screenHeight) {
    auto& s=*impl_;if (!s.token || screenHeight<=80) throw std::invalid_argument("미니맵 실제 화면 높이/수명 오류");
    const auto y=static_cast<float>(screenHeight-75); // 하단divider 다음5픽셀에서 시작한다.
    if (s.node.baseWidth==80 && s.node.baseHeight==75 && s.node.x==0 && s.node.y==y) return;
    s.node.baseWidth=s.node.width=80;s.node.baseHeight=s.node.height=75;s.common.tree.Push(s.token);Attach(s.token,0,y,s.common.create.collection.root,0);s.common.changed();
}
// 실제 원본 검색 결과의 전용 소유자 순서이며 일반 Player 색을 대신 쓰지 않는다.
void RawMinimapControls::SetPalette(const PaletteColorTable& palette) {
    if (palette.named.size()<20) throw std::invalid_argument("10.78 미니맵 파일 팔레트 색 전역 누락");auto& s=*impl_;const auto old=s.colors; // 같은 팔레트 재합성은 전체 갱신을 반복하지 않는다.
    s.colors.black=palette.basic[0];s.colors.owners={palette.named[11].index,palette.basic[1],palette.basic[7],palette.named[10].index,palette.basic[4],palette.basic[5],palette.named[12].index,palette.basic[6]};s.colors.neutralIsle=palette.named[18].index;s.colors.neutralNoIsland=palette.named[19].index;s.colors.challengeGreen=palette.basic[2];
    if (old.black!=s.colors.black || old.owners!=s.colors.owners || old.neutralIsle!=s.colors.neutralIsle || old.neutralNoIsland!=s.colors.neutralNoIsland || old.challengeGreen!=s.colors.challengeGreen) ++s.state.fullRefresh;
}
// 현재 모달/정지 상태를 추가하지 않고 원본 displayPresent/visible 조건만 같은 큐에서 읽는다.
bool RawMinimapControls::Refresh() {
    auto& s=*impl_;const auto view=s.hooks.view();s.modified=false;s.state.displayPresent=s.token!=0;s.state.displayFlags=static_cast<std::uint8_t>(s.node.flags);
    s.dirty.Refresh({view.camera,{view.viewport.left,view.viewport.top,view.viewport.right,view.viewport.bottom}});return s.modified;
}
// 공통 생성/수명/입력은 같은 기본주소를 빌린다.
GumpCreateNode* RawMinimapControls::Node(std::uint32_t token) const { return token && token==impl_->token ? &impl_->node : nullptr; }
// 실제 크기 변경 뒤crop 보정→기본부착 순서를 유지한다.
void RawMinimapControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { auto& s=*impl_;if (!Node(token)) throw std::out_of_range("미니맵 부착 대상 없음");s.SyncSize();ClampMinimapAttachment(s.geometry);s.common.tree.Attach(token,x,y,parent,mode); }
// 선택 이동 모드에서는 카메라 드래그/캡처를 시작하지 않는다. 일반 누름은 기본Help 반환0을 보존한다.
std::uint32_t RawMinimapControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
    auto& s=*impl_;if (!Node(token)) throw std::out_of_range("미니맵 입력 대상 없음");s.SyncSize();const auto local=s.common.collection.ScreenToLocal(token,{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)}); // 원본은 사건 종류 검사 전에 로컬 변환한다.
    if ((event&0x03ffffffU)==InputCode::kLeftButton) {
        if (event&InputCode::kRelease) { s.geometry.scrolling=0;s.common.lifetime.SetIdle(token,false);return 1; }
        const auto target=MinimapTarget(s.geometry,local[0],local[1]);const auto mode=s.hooks.inputMode(); // 현재 모드를 클릭 시점에 읽는다.
        if (mode==2 || mode==5) { s.hooks.move(target[0],target[1]);return 1; }
        s.hooks.center(target[0],target[1]);s.common.changed();s.common.input.Capture(token);s.geometry.scrolling=1;s.common.lifetime.SetIdle(token,true);
    }
    return s.menus.BaseHandle(token,event,first,second);
}
// 현재 캡처 중의 폴링 위치는 미니맵 밖에서도 동일한 로컬 좌표로 전달된다.
void RawMinimapControls::Idle(std::uint32_t token) { auto& s=*impl_;if (!Node(token) || !s.geometry.scrolling) throw std::logic_error("미니맵 드래그Idle 상태 오류");s.SyncSize();const auto mouse=s.hooks.poll();const auto local=s.common.collection.ScreenToLocal(token,{mouse.x,mouse.y});const auto target=DragMinimap(s.geometry,local[0],local[1]);s.hooks.center(target[0],target[1]);s.common.changed(); }
// 별도 bitmap 해제 없는 기본 scalar 소멸자다.
void RawMinimapControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { impl_->common.lifetime.DeleteScalar(token,flags); }
// 같은scalar가끝난뒤만등록대상의포인터를없앤다.
void RawMinimapControls::Release(std::uint32_t token) { if (token==impl_->token) { impl_->token=0;impl_->state.displayPresent=false; } }
// 좌표 지정은 같은 현재 경계를 읽기만 하며 crop/카메라/게임 상태를 쓰지 않는다.
std::optional<ScreenPoint> RawMinimapControls::Point(std::string_view label) const {
    if (!impl_->token || !label.starts_with("minimap:")) return {};const auto comma=label.find(',');if (comma==label.npos) return {};const auto origin=impl_->common.collection.Position(impl_->token); // Draw와 같은 현재 위치다.
    return ScreenPoint{origin.x+std::stoi(std::string(label.substr(8,comma-8))),origin.y+std::stoi(std::string(label.substr(comma+1)))};
}
// 같은raw큐와전체bitmap해시를읽어실제갱신/수명을관찰한다.
std::string RawMinimapControls::Report() const {
    const auto& s=*impl_;if (!s.token) return {};const auto bounds=s.common.collection.Bounds(s.token);std::uint32_t hash=2166136261U; // 독립bitmap의읽기전용FNV32다.
    // 초기화/셀그림을비교할때모든BYTE를포함한다.
    for (const auto byte:s.state.bitmap) hash=(hash^byte)*16777619U;
    std::ostringstream out;out<<"minimap\t"<<s.node.id<<'\t'<<bounds.left<<'\t'<<bounds.top<<'\t'<<bounds.right-bounds.left<<'\t'<<bounds.bottom-bounds.top<<'\t'<<s.geometry.cropX<<'\t'<<s.geometry.cropY<<'\t'<<s.geometry.scrolling<<'\t'<<s.state.frames<<'\t'<<s.state.count<<'\t'<<s.state.fullRefresh<<'\t'<<hash;return out.str();
}
}
