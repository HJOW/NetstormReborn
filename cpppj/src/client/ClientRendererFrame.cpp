// 원문 앞단/배경 본체는 유지하고 실제 Client/Screen/Gump/Pool 장치만 제공한다.
#include "client/ClientRendererFrame.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientAudio.h"
#include "client/RawRendererPrelude.h"
#include "client/RawSurfaceWorld.h"
#include "client/TileGroupCollection.h"
#include "client/ObjectCollection.h"
#include "client/DrawSort.h"
#include "client/GameWorld.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/RawCampaignFactoryDisplay.h"
#include "client/RawMenuControls.h"
#include "client/GumpRedraw.h"
#include "client/UberGump.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <sstream>

namespace netstorm::client { // 같은 현재 저장소를 원문 호출 위치에서 다시 조회한다.
namespace { // Native signed 덧셈과 단정도 저장/clip만 공통 처리한다.
std::int32_t Add(std::int32_t value,std::uint32_t delta) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+delta); } //화면+16/+11의 실제32비트 감김이다.
o::RenderWorldRect WorldRect(ScreenRect rect,const RawCameraViewState& view) {
    constexpr float xScale=0.0625F,yScale=std::bit_cast<float>(0x3dba2e8cU); //보호503b54/504194의 실제1/16·단정도1/11이다.
    const auto x=[&](auto pixel) { return static_cast<float>(static_cast<double>(Add(pixel,16))*xScale+std::bit_cast<float>(view.worldBits[0])); }; //FILD/FMUL/FADD 뒤 FSTP32다.
    const auto y=[&](auto pixel) { return static_cast<float>(static_cast<double>(Add(pixel,11))*yScale+std::bit_cast<float>(view.worldBits[1])); }; //현재 snapshot의 분수 원점을 보존한다.
    return {x(rect.left),y(rect.top),x(rect.right),y(rect.bottom)};
} //499758 이후의 가시 월드 계산이며 픽셀 원점에서 분수값을 재구성하지 않는다.
o::RenderWorldRect ClipWorld(o::RenderWorldRect rect,o::RenderWorldRect bounds) {
    const auto clamp=[](float value,float low,float high) { return std::max(low,std::min(value,high)); }; //4970c0의 각 좌표 제한 순서다.
    return {clamp(rect.left,bounds.left,bounds.right),clamp(rect.top,bounds.top,bounds.bottom),clamp(rect.right,bounds.left,bounds.right),clamp(rect.bottom,bounds.top,bounds.bottom)};
} //화면 dirty의 월드 경계를 현재 전체 지도 경계 안으로 각각 제한한다.
}

// 실제 Draw와 독립 원문 fixture가 같은 제품 환산/clip 본체를 호출한다.
o::RenderWorldRect RendererCollectionViewport(ScreenRect dirty,ScreenRect map,const RawCameraViewState& view) { return ClipWorld(WorldRect(dirty,view),WorldRect(map,view)); }
// 양수 화면의 곱셈도 원문32비트 저장 뒤 signed 나눗셈 순서를 유지한다.
std::int32_t RendererCollectionRowLimit(std::int32_t width,std::int32_t height) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(width/16)*static_cast<std::uint32_t>(Add(height,10)))/11; }

// 초기화/새 미션/해상도 전환으로 Native 전역을 다시 만들지 않는다.
ClientRendererFrame::ClientRendererFrame(Client& client):client_(client) {}
ClientRendererFrame::~ClientRendererFrame()=default; //collector를 실제 storage보다 먼저 반납한다.
void ClientRendererFrame::ReadOptions() { client_.configuration_.ReadInt("showSoftAsserts",softAsserts_); } //441270의 누락 유지와 같은 현재 값을 Frame 진단 관문에 제공한다.

// 실제 파일/난수/색/클립/잠금은 Client가 소유한 동일 장치를 빌린다.
RendererCloudHooks ClientRendererFrame::CloudDevices() {
    RendererCloudHooks hooks;
    hooks.assertion=[](auto line) { std::clog<<"Renderer cloudDirtyRectCount < NUM_CLOUD_DIRTY_RECT .\\Renderer.cpp "<<line<<'\n'; };
    hooks.clear=[this](auto rect,auto color) { ++cleared_;client_.screen_->Clear(rect,static_cast<std::uint8_t>(color)); };
    hooks.draw=[this](auto rect,auto x,auto y,bool alternate) {
        ++cloudCalls_;lastArea_=rect;lastOffsetX_=x;lastOffsetY_=y;
        // Windows10/11 호환 화면은 원본 독점 DirectDraw 대신 같은 DIB2를 쓴다. 비DIB 별도 표면을 일반 DIB로 속이지 않고 기존 명시 장치 경계를 유지한다.
        client_.kencloudGlobals_->Draw(*client_.screen_,{static_cast<std::uint32_t>(rect.left),static_cast<std::uint32_t>(rect.top),static_cast<std::uint32_t>(rect.right),static_cast<std::uint32_t>(rect.bottom),static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y)},client_.files_,client_.configuration_,client_.randomGlobals_->Game(),alternate);
    };
    return hooks;
}

// 입력/메뉴 갱신 뒤 현재 화면의 앞단을 실행한다. 실제 표/범위/전역을 별도 성공 상태로 복제하지 않는다.
bool ClientRendererFrame::Prepare() {
    if (client_.options_.edition!=o::OriginalEdition::Patch1078 || !client_.menu_ || !client_.menu_->rawButtons_) return false;
    auto& screen=*client_.screen_;auto& renderer=*client_.renderer_;auto& clouds=*client_.kencloudGlobals_;auto& camera=client_.gameplayGlobals_->Camera();auto& ui=*client_.menu_->rawButtons_;
    const auto& view=ui.MapViewRect();auto dirtyCount=static_cast<std::uint32_t>(renderer.CameraDirtyEntries().size()); //호스트 vector의 개수는 Native0 store 경계의 clearDirty로 같은 실제 표에 반영한다.
    RendererPreludeHooks hooks;hooks.markCloudDirty=[&clouds] { clouds.MarkDirty(); };
    hooks.queueCloudDirty=[this](auto rect) { const auto checking=static_cast<std::uint32_t>(softAsserts_);AppendRendererCloudDirty(globals_.CloudQueue(),rect,checking,CloudDevices()); };
    hooks.overrideActive=[this,&ui] { lastOverride_=ui.CurrentOverride();return lastOverride_; };
    hooks.updateClouds=[this,&clouds] { clouds.Update(client_.files_,client_.configuration_,client_.randomGlobals_->Game()); };
    hooks.rowDirty=[&clouds](auto row,auto y) { return clouds.RowDirty(static_cast<std::uint32_t>(row),static_cast<std::uint32_t>(y)); };
    hooks.invalidate=[&renderer](auto rect,auto flags) { renderer.Invalidate(rect,flags); };
    hooks.clearDirty=[&renderer] { renderer.ClearDirty(); };
    hooks.invalidateBackground=[this,&camera] { RefreshRendererCloudBackground(globals_.CloudQueue(),camera.Snapshot().pixels[0],camera.Snapshot().pixels[1],camera.Denominator().current,CloudDevices()); };
    PrepareRendererFrame({globals_.Scan(),renderer.BackgroundRedraw(),dirtyCount,globals_.CloudDirtyCount()},
        {screen.flags_,clouds.WorldRedraw(),screen.width_,screen.height_,client_.screenHeight_,camera.Snapshot().pixels[1],view.left,view.top,view.right,view.bottom},hooks);
    ++prepared_;return true;
}

// 같은 dirty에서 UI→건설→지형→일반 객체→미리보기 수집 뒤 배경→단일 정렬 목록을 출력한다.
RendererWorldDrawing ClientRendererFrame::Drawing() {
    return {[this](auto clip,auto flags) { return Collect(clip,flags); },[this](auto clip,auto flags,auto covered) { Background(clip,flags,covered); },
        [this](auto pixels,auto pitch,auto height,auto clip) { Submit(pixels,pitch,height,clip); }};
}

// 같은 Pool/해시/공통 UI의 실제 항목을 원문5,000칸에 모은다. bit4도 UI 수집은 실행한다.
std::uint32_t ClientRendererFrame::Collect(ScreenRect clip,std::uint32_t flags) {
    client_.screen_->SetClip(clip.left,clip.top,clip.right,clip.bottom);++collected_;
    if (clip.left==0 && clip.top==0 && clip.right==client_.screen_->width_ && clip.bottom==client_.screen_->height_) ++fullDirty_;
    dirtyPixels_+=static_cast<std::uint64_t>(static_cast<std::int64_t>(clip.right-clip.left)*(clip.bottom-clip.top)); //Renderer가 이미 화면 안의 양의 dirty로 제한한 실제 면적이다.
    const auto storage=client_.SurfaceStorage();
    if (storage_!=storage || collectorHeight_!=client_.screenHeight_) {
        objects_.reset();groups_.reset();storage_=storage;collectorHeight_=client_.screenHeight_;
        groups_=std::make_unique<TileGroupCollection>(storage_->hash,storage_->pool,TileGroupCollectionState{collectorHeight_,0,0x80000000U});
        objects_=std::make_unique<ObjectCollection>(storage_->hash,storage_->pool,storage_->types,storage_->shapes,0);
    }
    const auto& camera=client_.gameplayGlobals_->Camera().Snapshot();const auto& map=client_.menu_->rawButtons_->MapViewRect();
    mapClip_=ClampOriginalScreenRect(clip,map); //445290의 좌표별 제한을 UI 수집 전에 저장하고 같은 배경 호출까지 유지한다.
    const auto visible=RendererCollectionViewport(clip,map,camera);
    DrawCollectionHooks hooks;
    hooks.ui=[this](auto rect,auto output,auto limit) { return client_.menu_->rawButtons_->Menus()->CollectDrawing(rect,output,limit); };
    hooks.groups=[this](auto rect,auto limit,auto& covered) { return groups_->Collect(rect,limit,covered); };
    hooks.objects=[this](auto rect,auto output,auto limit) { return objects_->Collect(rect,output,limit); };
    if (auto* world=client_.menu_->world_.get()) {
        hooks.construction=[world](auto rect,auto output,auto limit) { return world->Session().Particles().CollectDrawing(rect,output,limit); };
        if (world->Session().HasCursorRuntimeForState()) hooks.preview=[world](auto rect,auto output,auto limit) { return world->Session().Cursor().Display().CollectDrawing(rect,output,limit); }; // 원문 커서가 아직 생성되지 않은 연결 화면에는 미리보기 항목이 없다.
    }
    const auto result=CollectDrawEntries({clip,visible,WorldRect(clip,camera),flags,RendererCollectionRowLimit(client_.screenWidth_,client_.screenHeight_)},entries_,hooks);
    entryCount_=result.count;
    for (const auto& entry:std::span(entries_).first(static_cast<std::size_t>(entryCount_))) if (entry.kind<collectedKinds_.size()) ++collectedKinds_[entry.kind]; //관측은 실제 상위 항목만 세며 지형 내부 순서를 건드리지 않는다.
    return result.backgroundCovered ? 1U : 0U;
}

// 실제 지도 dirty 안에서만 현재 날씨/구름/시차 배경을 선택해 같은 Screen에 출력한다.
void ClientRendererFrame::Background(ScreenRect clip,std::uint32_t flags,std::uint32_t covered) {
    static_cast<void>(clip);++backgrounds_;lastCovered_=covered;
    const auto& camera=client_.gameplayGlobals_->Camera();const auto black=static_cast<std::uint32_t>(client_.screen_->Colors().basic[0]);
    DrawRendererBackdrop(mapClip_,flags,covered,{client_.screen_->flags_,globals_.HideGroups(),client_.WorldAudio().Scene().tint,black,camera.Snapshot().pixels[0],camera.Snapshot().pixels[1],camera.Denominator().current},CloudDevices());
}

// PE의 sort 초기값1대로 원문 CRT 정렬 후 같은 UI 출력 문맥에서 종류0..4를 한 번씩 처리한다.
void ClientRendererFrame::Submit(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
    auto entries=std::span(entries_).first(static_cast<std::size_t>(entryCount_));SortDrawEntries(entries);
    auto& menus=*client_.menu_->rawButtons_->Menus();
    menus.WithDrawing(pixels,pitch,height,client_.screen_->Colors(),client_.screen_->ShadeMaps(),clip,[&](const RawMenuControls::DrawingToken& ui) {
        DrawDispatchHooks hooks;
        hooks.sprite=[&](const auto& entry) {
            auto* world=client_.menu_->world_.get();if (!world || !world->factoryDisplay_) throw std::logic_error("Native SpriteDraw 실제 World/공통 출력기 부재");
            auto& session=world->Session();
            world->factoryDisplay_->DrawEntry(entry,pixels,pitch,height,clip,client_.gameplayGlobals_->Camera().Snapshot().pixels,session.LocalOwner(),client_.menu_->hideBuildings_,
                [&session](auto sid,auto x,auto y) { session.DrawBridgeAdditional(sid,x,y); });
        };
        hooks.group=[this](auto token) { return groups_->Group(token); };hooks.ui=ui;
        hooks.construction=[&](auto token) { auto* world=client_.menu_->world_.get();if (!world) throw std::logic_error("Native RenderProcess 실제 Session 부재");world->Session().Particles().DrawEntry(token,pixels,pitch,height,clip); };
        hooks.preview=[&](const auto& entry) { auto* world=client_.menu_->world_.get();if (!world) throw std::logic_error("Native Preview 실제 Session 부재");world->Session().Cursor().Display().DrawEntry(entry,pixels,pitch,height,clip); };
        for (const auto& entry:entries) { //묶음 숨김은 상위 항목마다 현재 DWORD를 다시 읽는다. 내부 묶음은 별도 정렬하지 않는다.
            if (entry.kind<submittedKinds_.size()) ++submittedKinds_[entry.kind];
            DispatchDrawEntries(std::span(&entry,1),{globals_.HideGroups()!=0,false},hooks);
        }
    });
}

// 정상 Draw의 잠금 반납 뒤·Present 앞에서 원문4994b0 말미의 dirty 정리를 실행한다.
void ClientRendererFrame::Finish() { client_.kencloudGlobals_->CleanDirty();++finished_; }

// CLI 보고는 실제 프레임/구름 전역과 마지막 호출만 읽는다. 게임 판정이나 난수에는 쓰지 않는다.
std::string ClientRendererFrame::Diagnostics() const {
    const auto& cloud=client_.kencloudGlobals_->State();std::ostringstream out;
    out<<"raw_renderer_frame\t"<<prepared_<<'\t'<<collected_<<'\t'<<backgrounds_<<'\t'<<cloudCalls_<<'\t'<<cleared_<<'\t'<<finished_<<'\t'<<globals_.Scan()[0]<<'\t'<<globals_.Scan()[17]<<'\t'<<globals_.Scan()[18]<<'\t'<<globals_.CloudDirtyCount()<<'\t'<<cloud[2053]<<'\t'<<cloud[515]<<'\t'<<cloud[2051]<<'\t'<<cloud[2052]<<'\t'<<lastCovered_<<'\t'<<lastOverride_<<'\n';
    out<<"raw_renderer_cloud_area\t"<<lastArea_.left<<'\t'<<lastArea_.top<<'\t'<<lastArea_.right<<'\t'<<lastArea_.bottom<<'\t'<<lastOffsetX_<<'\t'<<lastOffsetY_<<'\n';
    out<<"raw_renderer_scene\t"<<entryCount_;
    for (auto count:collectedKinds_) out<<'\t'<<count; //실제 수집 종류별 누계다.
    for (auto count:submittedKinds_) out<<'\t'<<count; //같은 목록의 실제 분배 종류별 누계다.
    out<<'\n'<<"raw_renderer_dirty_area\t"<<fullDirty_<<'\t'<<dirtyPixels_<<'\n';return out.str();
}
}
