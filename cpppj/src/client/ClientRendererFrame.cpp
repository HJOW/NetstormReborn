// 원문 앞단/배경 본체는 유지하고 실제 Client/Screen/Gump/Pool 장치만 제공한다.
#include "client/ClientRendererFrame.h"
#include "client/VFXLine.h" //진단방향선도기존실제VFX본문으로그린다.
#include "client/DrawWater.h" //같은 실제 후단 finder/카메라/VFX·전체 viewport 출력을 연결한다.
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/NativeMetaScene.h" //같은현재CS/실제Pool·Gump를메타표시본체에연결한다.
#include "client/NativeIslandRig.h" //같은 현재 Client/Kernel/ProcessForm과 실제 메타 섬 애니메이션을 연결한다.
#include "client/ClientMetaSpriteDraw.h" //메타 객체+a4도 현재 통합 화면/메뉴 글꼴 장치를 빌린다.
#include "client/CameraHome.h" //메타전체배치뒤의실제4340d0을현재Renderer한계에연결한다.
#include "o/OriginalText.h" //현재번역문을기존영어/한국어표시바이트경계로변환한다.
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
void ClientRendererFrame::ReadOptions() { client_.configuration_.ReadInt("showSoftAsserts",client_.gameplayGlobals_->AiDebug().Checking());client_.gameplayGlobals_->DrawFire()=0; } //현재 Checking을 읽고 원문4356a6처럼 Fire만 끈다. 다른 진단 DWORD는 보존한다.

// 현재 장기 DWORD와 실제 Screen/루트 슬롯의 주소만 빌려 callback 뒤의 읽기도 같은 물리를 사용한다.
NativeMetaPollGlobals ClientRendererFrame::MetaGlobals() {
    auto& game=*client_.gameplayGlobals_;auto& local=*client_.localStateGlobals_;auto& screen=*client_.screen_;auto& ui=*client_.menu_->rawButtons_;auto& storage=*client_.SurfaceStorage();auto& aux=*game.GameAux();
    const auto& colors=screen.Colors();if (colors.named.size()<=36) throw std::out_of_range("메타의현재10.78팔레트색표부족");
    return {{*client_.ChallengeGlobals(),game.player.playerKnown,game.player.battleOwner,local.MetadataMode(),screen.flags_,storage.pool.CapacityValue(),ui.RootTokenValue(),colors.basic[5],colors.basic[7],colors.basic[1],colors.named[36].index,
        {game.Camera().RendererView(),ui.MapViewRect(),screen.width_,screen.height_}},local.MetaZones().records,client_.NsInfoGlobals()->zone,aux.DependFormType(),aux.ProcessTypeEnd(),storage.Checking(),storage.ownerMode.challenge,game.Boss()};
}
// 선택된 가상효과는 같은 현재 Session을 다시 조회하고 free/active의 새 관문을 추가하지 않는다.
NativeMetaSceneHooks ClientRendererFrame::MetaDevices() {
    NativeMetaSceneHooks hooks;client_.menu_->rawButtons_->Menus()->BindMetaSceneGumps(hooks);
    const auto sid=[this](std::uint32_t number) { if (number>65535U || number>=client_.SurfaceStorage()->pool.Capacity()) throw std::out_of_range("메타의현재DWORD SID물리범위");return o::Sid{static_cast<std::uint16_t>(number)}; }; //전체DWORD를검사한뒤에만같은WORD주소를사용한다.
    hooks.display.raw=[this,sid](auto number)->std::span<std::uint8_t> {
        auto& pool=client_.gameplayGlobals_->RequireSession().Core().pool;const auto bytes=pool.Slot(sid(number));
        return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; //실제비const Pool의현재물리다. AllocatedBytes의free필터를추가하지않고원문50BYTE쓰기만허용한다.
    };
    hooks.display.unpop=[this,sid](auto number,auto flags) { client_.gameplayGlobals_->RequireSession().World().Unpop(sid(number),flags); };
    hooks.display.setKind=[this,sid,raw=hooks.display.raw](auto number,auto kind) {
        auto bytes=raw(number);bytes[12]=static_cast<std::uint8_t>(kind);bytes[13]=static_cast<std::uint8_t>(kind>>8);const auto core=client_.gameplayGlobals_->RequireSession().Core();if (bytes[10]==core.bridgeState.bridgeType) core.building.NotifySurface(sid(number));
    }; //004ac220은소유자BYTE가아닌QA WORD와조건부현재다리통지다.
    hooks.display.paintOwner=[this,sid](auto number,auto owner) { client_.gameplayGlobals_->RequireSession().World().SetAssetOwner(sid(number),owner); };
    hooks.display.destroy=[this,sid](auto number,auto flags) { static_cast<void>(client_.gameplayGlobals_->RequireSession().World().Destroy(sid(number),flags)); };
    hooks.display.move=[this,sid](auto number,float x,float y,auto flags) { client_.gameplayGlobals_->RequireSession().World().Pop(sid(number),x,y,flags); };
    hooks.display.translate=[this](auto text) { const auto encoded=o::EncodeDisplayText(client_.Translate(text));return std::string(encoded.begin(),encoded.end()); }; //안내는실제현재언어와같은글꼴바이트로한번변환한다.
    hooks.display.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };
    hooks.display.denominator=[this](auto value) { client_.gameplayGlobals_->Camera().SelectDenominator(value,client_.kencloudGlobals_->WorldRedraw()); };
    hooks.display.dirty=[this](auto rectangle,auto flags) { client_.renderer_->Invalidate(rectangle,flags); };
    hooks.create=[this](auto type,auto flags) { return std::uint32_t(client_.gameplayGlobals_->RequireSession().World().Create(type,flags).value); };
    hooks.frameCount=[this,sid](auto number) { const auto core=client_.gameplayGlobals_->RequireSession().Core();const auto type=core.pool.Slot(sid(number))[10];if (type>=core.shapes.size()) throw std::out_of_range("메타현재TYPE프레임개수물리부족");return std::bit_cast<std::uint32_t>(core.shapes[type].frameCount); }; //004ad6c0은현재TYPE+114의SHP프레임개수다.가상78의기본프레임과구별한다.
    hooks.selectFrame=[this](auto count) { return client_.randomGlobals_->Game().Next(count); }; //004558c0의같은게임난수를한번소비하며0한계를임의로고치지않는다.
    hooks.resetCamera=[this] {
        auto& camera=client_.gameplayGlobals_->Camera();CameraHomeState state;state.width=client_.screen_->width_;state.height=client_.screen_->height_;state.challengeType=client_.ChallengeGlobals()->ParticipantType();state.worldWidth=camera.Target().limits[0];state.worldHeight=camera.Target().limits[1];
        client_.gameplayGlobals_->RequireSession().ResetCamera(state,[this](auto,auto) { client_.menu_->CommitCameraNow(); });
    };return hooks;
}
// 논리 표시의 현재 view만 공급하고 전체/주기 본체의 원문 관문과 효과 순서는 그대로 둔다.
void ClientRendererFrame::RefreshMetaDisplay(bool whole) {
    auto globals=MetaGlobals();const auto hooks=MetaDevices();
    if (globals.display.playerKnown) {
        const auto& view=client_.gameplayGlobals_->Camera().Snapshot();client_.gameplayGlobals_->RequireSession().World().SetDisplay({view.pixels[0],view.pixels[1],std::bit_cast<std::int32_t>(view.zoom),{globals.display.view.viewport.left,globals.display.view.viewport.top,globals.display.view.viewport.right,globals.display.view.viewport.bottom}},
            {[this] { return client_.renderer_->FullRedrawPending(); },[this](auto rect,auto flags) { client_.renderer_->Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    }
    if (whole) { const auto& colors=client_.screen_->Colors();BuildMetaScene({globals,client_.gameplayGlobals_->Camera().Target(),client_.ChallengeGlobals()->GeometryMode(),client_.ChallengeGlobals()->ZoneGlyphType(),colors.basic[0],colors.basic[7],colors.basic[2],client_.localStateGlobals_->MetaPending()},hooks); }
    else PollMetaDisplay(globals,hooks.display);
}
// 설명 수신은 다른 참가자/전투의 Poll 대신 지정한 현재 전투 한 개만 갱신한다.
void ClientRendererFrame::UpdateMetaBattle(std::uint32_t battle) { const auto globals=MetaGlobals();const auto hooks=MetaDevices();netstorm::client::UpdateMetaBattle(battle,globals.display,hooks.display); }
void Client::UpdateNativeMetaBattle(std::uint32_t battle) { if (!rendererFrame_) throw std::logic_error("특정 메타 전투의 실제 Renderer owner 미설치");rendererFrame_->UpdateMetaBattle(battle); } //실제 장기 owner로만 위임하며 장치를 생성하지 않는다.
void Client::RefreshNativeMetaDisplay(bool whole) { if (!rendererFrame_) throw std::logic_error("메타표시의실제Renderer owner미설치");rendererFrame_->RefreshMetaDisplay(whole); } //원문State Poll의두표시위치를같은실제Frame장치에전달한다.

// 장기 Rig의 콜백은 매번 현재 UI/Session을 다시 찾고 CS Image owner는 최종 반납까지 pin한다.
NativeIslandRig& ClientRendererFrame::AcquireIslandRig(std::uint32_t object,float height,std::uint32_t kill) {
    if (object>65535U || object>=client_.SurfaceStorage()->pool.Capacity()) throw std::out_of_range("Rig 실제 DWORD SID 물리 범위");
    const auto devices=MetaDevices().display;NativeIslandRigHooks hooks;hooks.object.raw=devices.raw;hooks.object.unpop=devices.unpop;hooks.object.setKind=devices.setKind;hooks.object.destroy=devices.destroy;hooks.object.move=devices.move;hooks.object.assertion=devices.assertion; //실제로 쓰는 장기 World/Pool 장치만 보관하고 이전 UI callback 묶음은 남기지 않는다.
    hooks.object.findGump=[this](auto id)->GumpCreateNode* { NativeMetaDisplayHooks current;if (!client_.menu_ || !client_.menu_->rawButtons_ || !client_.menu_->rawButtons_->Menus()) throw std::logic_error("Rig 현재 Gump 장치 미설치");client_.menu_->rawButtons_->Menus()->BindMetaDisplayGumps(current);return current.findGump(id); }; //이전 Uber의 RawMenuControls 포인터를 장기 Rig에 저장하지 않는다.
    hooks.killGump=[this](auto id) { if (!client_.menu_ || !client_.menu_->rawButtons_ || !client_.menu_->rawButtons_->Menus()) throw std::logic_error("Rig 현재 Gump Kill 장치 미설치");client_.menu_->rawButtons_->Menus()->KillRegisteredGump(id); };
    NativeIslandRigGlobalsGetter globals=[this,owner=client_.ChallengeGlobals()] { return NativeIslandRigGlobals{MetaGlobals(),{client_.Time().game,client_.gameplayGlobals_->player.special,owner->Projection().camera.height}}; };
    const auto core=client_.GameplayGlobals()->RequireSession().Core();auto* rig=FindOrCreateNativeIslandRig(core.processes,core.kernel,o::Sid{static_cast<std::uint16_t>(object)},height,kill,std::move(globals),hooks);if (!rig) throw std::runtime_error("실제 메타 IslandRig 생성/부착 실패");return *rig;
}
// 실제 Client의 장기 Renderer 조립기만 빌리며 조회만으로 프레임/네트워크를 진행하지 않는다.
NativeIslandRig& Client::AcquireNativeIslandRig(std::uint32_t object,float height,std::uint32_t kill) { if (!rendererFrame_) throw std::logic_error("Rig 실제 Renderer owner 미설치");return rendererFrame_->AcquireIslandRig(object,height,kill); }

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
    hooks.queueCloudDirty=[this](auto rect) { const auto checking=static_cast<std::uint32_t>(client_.gameplayGlobals_->AiDebug().Checking());AppendRendererCloudDirty(globals_.CloudQueue(),rect,checking,CloudDevices()); };
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
    if (client_.gameplayGlobals_->Borrowed()) {
        auto& session=client_.gameplayGlobals_->RequireSession();hooks.construction=[&session](auto rect,auto output,auto limit) { return session.Particles().CollectDrawing(rect,output,limit); };
        if (session.HasCursorRuntimeForState()) hooks.preview=[&session](auto rect,auto output,auto limit) { return session.Cursor().Display().CollectDrawing(rect,output,limit); }; //메타도같은실제표시목록을사용하며미생성커서미리보기는추가하지않는다.
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
            auto* world=client_.menu_->world_.get();auto* display=world ? world->factoryDisplay_.get() : client_.menu_->MetaSpriteDisplay();if (!display) throw std::logic_error("Native SpriteDraw 실제 게임/메타 공통 출력기 부재");
            auto& session=client_.gameplayGlobals_->RequireSession();
            display->DrawEntry(entry,pixels,pitch,height,clip,client_.gameplayGlobals_->Camera().Snapshot().pixels,session.LocalOwner(),client_.menu_->hideBuildings_,
                [&session](auto sid,auto x,auto y) { session.DrawBridgeAdditional(sid,x,y); },[&](auto sid,auto x,auto y) { DrawClientMetaSprite(client_,menus,sid,{x,y},pixels,pitch,height,clip); });
        };
        hooks.group=[this](auto token) { return groups_->Group(token); };hooks.ui=ui;
        hooks.construction=[&](auto token) { client_.gameplayGlobals_->RequireSession().Particles().DrawEntry(token,pixels,pitch,height,clip); };
        hooks.preview=[&](const auto& entry) { client_.gameplayGlobals_->RequireSession().Cursor().Display().DrawEntry(entry,pixels,pitch,height,clip); }; //수집때실제존재를확인한같은Session커서만출력한다.
        for (const auto& entry:entries) { //묶음 숨김은 상위 항목마다 현재 DWORD를 다시 읽는다. 내부 묶음은 별도 정렬하지 않는다.
            if (entry.kind<submittedKinds_.size()) ++submittedKinds_[entry.kind];
            DispatchDrawEntries(std::span(&entry,1),{globals_.HideGroups()!=0,false},hooks);
        }
    });
}

// 같은 화면 잠금/clip과 현재 팔레트/기존BFS 배열을 지연 연결한다.
RendererDiagnosticHooks ClientRendererFrame::DiagnosticDevices() {
    auto& screen=*client_.screen_;const auto storage=client_.SurfaceStorage();RendererDiagnosticHooks hooks;
    hooks.screen.clip=[&screen](auto rect) { screen.SetClip(rect.left,rect.top,rect.right,rect.bottom); };
    const auto paint=[&screen](const auto& operation) { //원문4a2270/4a23b0은전체viewport로잠시바꾼뒤이전clip을복구한다.
        const auto saved=screen.Clip();const ScreenRect full{0,0,screen.Width(),screen.Height()};screen.SetClip(full.left,full.top,full.right,full.bottom);auto* pixels=screen.Lock();
        try { operation(std::span<std::uint8_t>(pixels,static_cast<std::size_t>(screen.Pitch())*static_cast<std::size_t>(screen.Height())),screen.Pitch(),screen.Height(),full); }
        catch (...) { screen.Unlock();screen.SetClip(saved.left,saved.top,saved.right,saved.bottom);throw; } //호스트예외에도잠금/clip만반납하며이미그린픽셀은되돌리지않는다.
        screen.Unlock();screen.SetClip(saved.left,saved.top,saved.right,saved.bottom);
    };
    hooks.screen.rectangle=[paint](auto rect,auto color) { paint([&](auto pixels,auto pitch,auto height,auto clip) { DrawWaterRectangle(pixels,pitch,height,clip,rect,color); }); };
    hooks.line=[paint](auto first,auto second,auto color) { paint([&](auto pixels,auto pitch,auto height,auto clip) { DrawVfxLine(pixels,pitch,height,clip,{first[0],first[1]},{second[0],second[1]},color); }); };
    hooks.screen.present=[&screen] { screen.PresentPaletteClear(); };
    hooks.colors=[&screen]()->std::span<const std::uint32_t> { return screen.Colors().basic; };
    hooks.invalidGraph=[this]()->std::uint32_t { return client_.gameplayGlobals_->RequireSession().Core().bookkeeping.invalidGraph; }; //메타도현재Session의같은Graph장부를읽고없는월드를합성하지않는다.
    hooks.distances=[storage]()->std::span<const std::int16_t> { const auto& current=storage->pathSearchWork;if (!current.initialized || !current.work) return {};return current.work->distance; }; //기존실제BFS가쓴원문거리표만읽고새검색은하지않는다.
    hooks.assertion=[](auto line) { std::clog<<"원문 Renderer 진단 "<<line<<'\n'; };return hooks;
}
// 플래그0은Screen/Pool/거리표를빌리지않는원문단락이다.
void ClientRendererFrame::DrawDiagnostic(RendererDiagnosticKind kind,const std::uint32_t& flag) {
    if (!flag) return;const auto storage=client_.SurfaceStorage();const auto& camera=client_.gameplayGlobals_->Camera();auto& renderer=client_.GetRenderer();
    RawRendererDiagnostics(storage->pool,storage->hash,storage->types,storage->spots).Draw(kind,camera.RendererView(),camera.Snapshot(),client_.screen_->Width(),client_.screen_->Height(),flag,client_.gameplayGlobals_->FireMode(),client_.gameplayGlobals_->WorldCells(),renderer.BackgroundRedraw(),DiagnosticDevices());
}
// 일반Draw/캡처뒤Hash→Fire→Spot은각자현재플래그를다시읽는다.
void ClientRendererFrame::DrawDiagnosticsBeforeWater() {
    auto& globals=*client_.gameplayGlobals_;DrawDiagnostic(RendererDiagnosticKind::Hash,globals.DrawHash());DrawDiagnostic(RendererDiagnosticKind::Fire,globals.DrawFire());DrawDiagnostic(RendererDiagnosticKind::Spot,globals.DrawSpot());
}
// Water뒤의Board와마지막Present/snapshot은서로다른현재효과다.
void ClientRendererFrame::DrawDiagnosticsAfterWater() { DrawDiagnostic(RendererDiagnosticKind::Board,client_.gameplayGlobals_->DrawBoard()); }

//994b0 반환/정리와 연속캡처 뒤의 별도 진단 표시다. dirty별 sprite/overlay로 섞지 않는다.
void ClientRendererFrame::DrawWater() {
    auto& enabled=client_.gameplayGlobals_->DrawWater();if (!enabled) return; //원문 flag0은 실제 물리/화면 장치를 요구하지 않는다.
    const auto storage=client_.SurfaceStorage();auto& screen=*client_.screen_;auto& camera=client_.gameplayGlobals_->Camera();
    DrawWaterHooks hooks;hooks.clip=[&screen](auto rect) { screen.SetClip(rect.left,rect.top,rect.right,rect.bottom); };
    hooks.rectangle=[this](auto rect,auto color) { DrawOutlineRectangle(rect,color); }; // 같은 실제4a2270 장치를 개발자 강조와 공유한다.
    hooks.present=[&screen] { screen.PresentPaletteClear(); }; //이 기존 helper는 같은004a46d0의 전체 viewport 출력/clip 복구다. 새 팔레트 변경은 수행하지 않는다.
    RawDrawWater(storage->pool,storage->hash,storage->types).Draw(camera.RendererView(),camera.Snapshot(),screen.Width(),screen.Height(),enabled,screen.Colors().basic[3],hooks); //원문005ad128은 같은 파일 팔레트 기본 파랑이다.
}

// 사각형마다 현재 clip을 보존하고 전체 viewport의 기존 VFX 네 선을 그린다.
void ClientRendererFrame::DrawOutlineRectangle(ScreenRect rectangle,std::uint32_t color) {
    auto& screen=*client_.screen_;const auto saved=screen.Clip();const ScreenRect full{0,0,screen.Width(),screen.Height()};screen.SetClip(full.left,full.top,full.right,full.bottom);auto* pixels=screen.Lock();
    try { DrawWaterRectangle({pixels,static_cast<std::size_t>(screen.Pitch())*static_cast<std::size_t>(screen.Height())},screen.Pitch(),screen.Height(),full,rectangle,color); }
    catch (...) { screen.Unlock();screen.SetClip(saved.left,saved.top,saved.right,saved.bottom);throw; } // 부분 픽셀은 보존하고 호스트 잠금/clip만 반납한다.
    screen.Unlock();screen.SetClip(saved.left,saved.top,saved.right,saved.bottom);
}
// 같은 현재 snapshot의 가로 칸 크기를 두 축에 쓰는 원문 비대칭 좌표를 보존한다.
void ClientRendererFrame::HighlightDeveloperPoint(float x,float y) {
    if (!client_.screen_) throw std::logic_error("개발자 강조의 현재 Screen 미설치");auto& screen=*client_.screen_;const auto& view=client_.gameplayGlobals_->Camera().Snapshot();const auto point=ProjectCameraView(view,x,y);
    const auto wrap=[](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }; // 원문의 signed DWORD 덧셈/뺄셈 감김을 정의된 계산으로 유지한다.
    ScreenRect rectangle{wrap(std::bit_cast<std::uint32_t>(point[0])-std::bit_cast<std::uint32_t>(view.cellPixels[0])+1U),wrap(std::bit_cast<std::uint32_t>(point[1])-std::bit_cast<std::uint32_t>(view.cellPixels[0])+1U),wrap(std::bit_cast<std::uint32_t>(point[0])-1U),wrap(std::bit_cast<std::uint32_t>(point[1])-1U)};
    screen.SetClip(rectangle.left,rectangle.top,rectangle.right,rectangle.bottom); //4345e0은 이 작은 clip을 원래값으로 복구하지 않는다.
    for (int stroke=0;stroke<3;++stroke) { // 각 선 뒤 현재 노랑0059afd8을 다시 읽고 사각형을 한 픽셀씩 줄인다.
        DrawOutlineRectangle(rectangle,screen.Colors().basic[5]);rectangle.left=wrap(std::bit_cast<std::uint32_t>(rectangle.left)+1U);rectangle.top=wrap(std::bit_cast<std::uint32_t>(rectangle.top)+1U);rectangle.right=wrap(std::bit_cast<std::uint32_t>(rectangle.right)-1U);rectangle.bottom=wrap(std::bit_cast<std::uint32_t>(rectangle.bottom)-1U);
    }
    screen.PresentPaletteClear(); //4a46d0만 실행하며 장면 재그리기/Frame/flip을 추가하지 않는다.
}
// 실제 현재 Frame 장치가 동일 Screen의 private Native 출력 경계를 소유한다.
void Client::HighlightDeveloperPoint(float x,float y) { if (!rendererFrame_) throw std::logic_error("개발자 강조의 실제 Renderer owner 미설치");rendererFrame_->HighlightDeveloperPoint(x,y); }

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
