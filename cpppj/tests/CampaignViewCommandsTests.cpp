// 실제 GameWorld/Session의 Home·명시 SID/owner·Carrier Kernel 예약과 Reload 현재 목록을 검증한다.
#include "DudeSpawnSupport.h"
#include "CameraTestDevices.h"
#include "client/GameWorld.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/RawCameraCommit.h"
#include "client/RawCameraPresent.h"
#include <cstring>
#include "o/RawFortObjectLoad.h"
#include <stdexcept>

using namespace netstorm::client;
namespace {
// 보호 미션 설치에 필요한 외부 음향/번역만 제공하며 카메라·Player·사제·Regular는 실제 본체를 사용한다.
struct CameraCommandServices {
    SoundList names{OriginalEdition::Patch1078};std::array<std::uint8_t,4> you{'Y','o','u',0}; // 장기 소리 이름표와 명시 번역 입력이다.
    // 별도 창/장치 없이 같은 원본 Session을 설치하는 실제 외부 효과 계약이다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };result.cameraCommit=netstorm::test::camera::CommitScreenless;
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("View 검사에서 미선택 삭제 효과"); };result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// 카메라만 읽는 두 raw 자산을 같은 실제 풀에 설치하며 가상 생성/Pop 동작을 대신하지 않는다.
Sid CameraCommandTarget(RawCampaignSession& session,float x,float y) {
    const auto core=session.Core();const auto sid=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));return sid;
}
}
// Home은 Player+7c 우선/+84 fallback이며 dead 첫 대상이면 다른 후보를 찾지 않는다.
TEST_CASE(CampaignViewCommands_GameWorldCurrentHomeAndExplicitSid) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",services.Make(),{640,480});auto& session=world.Session();const auto core=session.Core();auto types=session.World().MutableTypes(); // 실제 GameWorld가 같은 Session을 소유한다.
    types[70].footX=3;types[70].footY=2;types[70].flags2=0;const auto first=CameraCommandTarget(session,10,20),second=CameraCommandTarget(session,30,40);auto players=session.PlayerRecords();const auto offset=session.LocalOwner()*180U;Put(players,offset+0x7c,first.value);Put(players,offset+0x84,second.value);
    world.Home(false);CHECK(world.ToScreen(0,0).x==208 && world.ToScreen(0,0).y==42);core.pool.AllocatedBytes(first)[11]=2;world.Home(false);CHECK(world.ToScreen(0,0).x==208 && world.ToScreen(0,0).y==42); // 비0 dead SID는 +84로 재선택하지 않는다.
    Put(players,offset+0x7c,0);world.Home(false);CHECK(world.ToScreen(0,0).x==-112 && world.ToScreen(0,0).y==-178);Put(players,offset+0x84,0);world.Home(false);CHECK(world.ToScreen(0,0).x==-112 && world.ToScreen(0,0).y==-178);
    core.pool.AllocatedBytes(first)[11]=0;world.Resize(800,600);world.CenterOnObject(first.value);CHECK(world.ToScreen(0,0).x==288 && world.ToScreen(0,0).y==102); // 현재 장치 크기와 전체 foot을 쓰고 테두리 clamp를 추가하지 않는다.
    bool rejected{};try { world.CenterOnObject(0x10000U+first.value); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);world.CenterOnObject(0);CHECK(world.ToScreen(0,0).x==288 && world.ToScreen(0,0).y==102);
}
// 실제4950d0 참여 조회·491 Regular·원본005c89b8을 같은 Session에서 사용한다.
TEST_CASE(CampaignViewCommands_ActualPriestPreviewAndResetDepth) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",services.Make(),{640,480});auto& session=world.Session();const auto core=session.Core();const auto found=session.FindPriestParticipation(session.LocalOwner());CHECK(found!=0);if (!found) return;const Sid priest{static_cast<std::uint16_t>(found)};auto raw=core.pool.AllocatedBytes(priest);auto types=session.World().MutableTypes(); // 같은 실제 살아 있는 사제 목록을 조회한다.
    types[raw[10]].footX=3;types[raw[10]].footY=2;types[raw[10]].flags2|=0x210000;Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(30.0F));raw[40]=0;
    session.World().SetLoadingDepth(4);CameraHomeState state;state.suppressionDepth=99;const auto initialCount=core.kernel.Size();std::uint32_t applied{};
    session.ResetCamera(state,[&](auto,auto) { ++applied;CHECK(session.FortObjectSettings().loadingDepth==5 && state.suppressionDepth==5); });CHECK(applied==1 && state.suppressionDepth==4 && session.FortObjectSettings().loadingDepth==4 && core.kernel.Size()==initialCount);
    world.Home(true);CHECK(world.ToScreen(0,0).x==48 && world.ToScreen(0,0).y==-68 && core.kernel.Size()==initialCount); // 깊이4에서는 실제 preview가 생성하지 않는다.
    session.World().SetLoadingDepth(0);world.ViewPriest(session.LocalOwner());const auto* preview=core.processes.FindEvent(priest,0x491);CHECK(preview && std::bit_cast<std::uint32_t>(preview->Payload())==0x3e4ccccd && core.kernel.Size()==initialCount+1);
    world.CenterCamera(100,100);const auto previous=world.ToScreen(0,0);CHECK(session.FindPriestParticipation(257)==0);world.ViewPriest(257);CHECK(world.ToScreen(0,0).x==previous.x && world.ToScreen(0,0).y==previous.y); // 명시 owner257을 로컬 owner로 바꾸지 않는다.
    world.Home(true);CHECK(world.ToScreen(0,0).x==48 && world.ToScreen(0,0).y==-68);
}
// Reload 목록은 사본이 아니라 현재 signed count와 현재 DWORD 항목을 재조회한다.
TEST_CASE(CampaignViewCommands_ReloadCurrentListsAndModeContract) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",services.Make());session.Load();auto& temples=session.World().Storage()->sidLists.temples;const auto saved=temples;const auto firstCount=session.GameMenuReloadCount(false); // first는 별도 outposts이며 temples 변경과 독립이다.
    temples.entries={100U,200U,300U};temples.count=2;CHECK(session.GameMenuReloadCount(true)==2 && session.GameMenuReloadSid(true,0)==100 && session.GameMenuReloadSid(true,1)==200 && session.GameMenuReloadCount(false)==firstCount);
    temples.entries[0]=400;temples.count=1;CHECK(session.GameMenuReloadCount(true)==1 && session.GameMenuReloadSid(true,0)==400);temples.count=0xffffffffU;CHECK(session.GameMenuReloadCount(true)==-1);temples=saved;
    bool rejected{};try { session.ReloadTheamAt(std::bit_cast<std::uint32_t>(10.0F),std::bit_cast<std::uint32_t>(20.0F),1); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected); // 이 명령 어댑터는 원본 caller의 mode0만 수용한다.
}

// 같은 원문 두 번째 헤더005e1898은500칸 공장과 독립인20칸 신전 목록이다.
TEST_CASE(CampaignViewCommands_ReloadSecondUsesActualTempleRegistry) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices services;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",services.Make());session.Load();auto& lists=session.World().Storage()->sidLists;CHECK(lists.temples.count>0);if (!lists.temples.count) return;
    const auto count=lists.temples.count,first=lists.temples.Items()[0],factoryCount=lists.factories.count;const auto factoryFirst=lists.factories.entries[0];lists.factories.count=0xffffffffU;lists.factories.entries[0]=0xfedcba98;
    CHECK(session.GameMenuReloadCount(true)==std::bit_cast<std::int32_t>(count) && session.GameMenuReloadSid(true,0)==first);lists.factories.count=factoryCount;lists.factories.entries[0]=factoryFirst;
    lists.temples.entries[0]=0x89abcdef;CHECK(session.GameMenuReloadSid(true,0)==0x89abcdef);lists.temples.entries[0]=first;
}

// 실제 World/Session의 목표 요청은 같은 Client 전역만 쓰고 표시/음향은 실제497960 뒤에 바뀐다.
TEST_CASE(CampaignViewCommands_CenterRequestDefersCurrentViewsUntilNativeCommit) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices devices;auto services=devices.Make();
    const auto gameplay=std::make_shared<ClientGameplayGlobals>();const auto local=std::make_shared<ClientLocalStateGlobals>(); // 실제 Client와 같이 World/Session 밖에서 두 별도 주소를 소유한다.
    services.gameplayGlobals=gameplay;services.localStateGlobals=local;auto& camera=gameplay->Camera();std::uint32_t redraw{},worldRedraw{},initialRevision{};camera.InitializeNativeRenderer(640,480,256,worldRedraw,initialRevision);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});
    const auto rendererBefore=camera.RendererView(),snapshotBefore=camera.Snapshot();const auto mapRevision=gameplay->MapRevision();std::uint32_t soundViews{};world.soundViewChanged=[&](auto) { ++soundViews; };world.TakeChanged();local->MetaRebuild()=0xffffffffU;
    world.CenterCamera(100.25F,101.75F);CHECK((camera.Target().pixels==std::array<std::int32_t,2>{1284,879}) && local->MetaRebuild()==0);
    CHECK(std::memcmp(&rendererBefore,&camera.RendererView(),64)==0 && std::memcmp(&snapshotBefore,&camera.Snapshot(),64)==0);CHECK(gameplay->MapRevision()==mapRevision && soundViews==0 && !world.TakeChanged());CHECK(world.MinimapView().camera==snapshotBefore.worldBits);
    std::vector<DirtyRegion> dirty;std::uint32_t locks{},unlocks{},rootRedraws{},scrolls{};const std::uint32_t flags=2;const std::int32_t denominator=15,width=640,height=480;const auto clip=world.MapView();const RawCameraCommitViewport viewport{clip.left,clip.top,clip.right-clip.left,clip.bottom-clip.top};const ScreenPoint mouse{200,100}; // 실제 DIB/지도와 같은 장치 입력이며 currentview를 직접 설정하지 않는다.
    RawCameraCommitHooks hooks;hooks.lockCursor=[&](auto skip) { CHECK(skip==0);++locks; };hooks.unlockCursor=[&] { ++unlocks; };hooks.redrawRoots=[&] { ++rootRedraws; };hooks.scroll=[&](auto,auto,auto,auto,auto,auto) { ++scrolls; };hooks.scrollBackground=[](auto,auto,auto,auto,auto,auto) { CHECK(false); };
    hooks.addDirty=[&](auto rect,auto value) { dirty.push_back({rect,value}); };hooks.addBackgroundDirty=[](auto) { CHECK(false); };hooks.updateMouse=[](auto) {}; // OS 장치의 호출 경계만 제공하고 실제 commit/setter는 제품 본체를 쓴다.
    CommitCameraTarget({camera.RendererView(),camera.Snapshot(),camera.Target(),redraw,worldRedraw,dirty},{flags,denominator,width,height,viewport,clip,mouse},hooks);
    CHECK(camera.RendererView().pixels==camera.Target().pixels && camera.Snapshot().pixels==camera.Target().pixels && locks==1 && unlocks==1 && rootRedraws==2);CHECK(scrolls==2 && local->MetaRebuild()==0 && soundViews==0);
    world.CameraApplied();CHECK(soundViews==1 && world.TakeChanged()); // 실제 프레임 말미의 적용 결과만 기존 표면/입자/음향에 공개한다.
}

// 같은 Client의 두 카운터/현재 목표와 화면 장치 호출 위치를 실제 Home/무효 대상에서 확인한다.
TEST_CASE(CampaignViewCommands_ImmediateHomeUsesSharedCountersAndFullCommit) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices devices;auto services=devices.Make();
    const auto gameplay=std::make_shared<ClientGameplayGlobals>();const auto local=std::make_shared<ClientLocalStateGlobals>();const auto cloud=std::make_shared<ClientKencloudGlobals>();services.gameplayGlobals=gameplay;services.localStateGlobals=local;services.kencloudGlobals=cloud; // Session 바깥에서 같은 Client 수명의 세 owner를 유지한다.
    auto& camera=gameplay->Camera();std::uint32_t initialization{};camera.InitializeNativeRenderer(640,480,256,cloud->WorldRedraw(),initialization);std::vector<std::array<std::uint32_t,6>> entered; // 현재 목표/이전 적용값과두 실제 공유 카운터다.
    services.cameraCommit=[&](const CampaignCameraCommitContext& context) {
        CHECK(&context.camera==&camera && &context.worldRedraw==&cloud->WorldRedraw() && context.width==640 && context.height==480);
        entered.push_back({static_cast<std::uint32_t>(camera.Target().pixels[0]),static_cast<std::uint32_t>(camera.Target().pixels[1]),static_cast<std::uint32_t>(camera.RendererView().pixels[0]),static_cast<std::uint32_t>(camera.RendererView().pixels[1]),context.worldRedraw,local->MetaRebuild()});
        netstorm::test::camera::CommitScreenless(context); // 현재 카메라를 임의setter로 바꾸지 않고 실제497960을 실행한다.
    };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});auto& session=world.Session();CHECK(!entered.empty());
    auto types=session.World().MutableTypes();types[70].footX=3;types[70].footY=2;types[70].flags2=0;const auto target=CameraCommandTarget(session,10,20);auto players=session.PlayerRecords();Put(players,session.LocalOwner()*180U+0x7c,target.value);const auto before=camera.RendererView().pixels;entered.clear();cloud->WorldRedraw()=0xffffffffU;local->MetaRebuild()=0xffffffffU;std::uint32_t sounds{};world.soundViewChanged=[&](auto) { ++sounds; };world.TakeChanged();
    world.Home(false);CHECK(entered.size()==1 && sounds==1 && world.TakeChanged());if (entered.size()!=1) return;CHECK((entered[0]==std::array<std::uint32_t,6>{static_cast<std::uint32_t>(-208),static_cast<std::uint32_t>(-42),static_cast<std::uint32_t>(before[0]),static_cast<std::uint32_t>(before[1]),0,0}));
    CHECK((camera.RendererView().pixels==std::array<std::int32_t,2>{-208,-42}) && camera.Snapshot().pixels==camera.RendererView().pixels); // 즉시 부모는 Present의 가장자리 제한을 거치지 않는다.
    world.Home(false);CHECK(entered.size()==2 && cloud->WorldRedraw()==1 && local->MetaRebuild()==1 && sounds==1 && !world.TakeChanged()); // 같은 위치도 부모 카운터/실제commit은 호출하되 표면/음향을 중복 공개하지 않는다.
    session.Core().pool.AllocatedBytes(target)[11]=2;world.Home(false);CHECK(entered.size()==2 && cloud->WorldRedraw()==1 && local->MetaRebuild()==1 && sounds==1); // 첫 Home SID가dead면 다른 대상이나 부모를 요청하지 않는다.
}

// 호환 viewport 변경이 아직 표시되지 않은 목표와 서로 다른 현재 Renderer/snapshot을 덮지 않아야 한다.
TEST_CASE(CampaignViewCommands_ViewportResizeKeepsPendingTargetUntilNativePresent) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CameraCommandServices devices;auto services=devices.Make(); // 실제 보호 Fort와 같은 Session을 쓰고 화면 장치만 명시한다.
    const auto gameplay=std::make_shared<ClientGameplayGlobals>();const auto local=std::make_shared<ClientLocalStateGlobals>();const auto cloud=std::make_shared<ClientKencloudGlobals>();ScreenRect dock{85,0,1024,672}; // 현재 도킹 LTRB와 세 장기 owner다.
    services.gameplayGlobals=gameplay;services.localStateGlobals=local;services.kencloudGlobals=cloud;services.mapView=[&] { return dock; };
    auto& camera=gameplay->Camera();std::uint32_t initialRevision{};camera.InitializeNativeRenderer(1024,768,256,cloud->WorldRedraw(),initialRevision);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{1024,768});
    std::vector<SoundView> sound;world.soundViewChanged=[&](auto value) { sound.push_back(value); }; // 현재 viewport 통지와 나중 카메라 적용 통지를 구별한다.
    const std::array<ScreenPoint,4> sizes{{{1280,720},{1280,800},{800,600},{640,480}}}; // 16:9/16:10/두4:3의 호스트 viewport 입력이다.
    for (const auto size:sizes) { // 매 크기에서 다른 두 현재 시야와 범위 밖의 대기 목표를 독립 입력한다.
        SetCameraViewWorld(camera.RendererView(),194.25F,200.75F);SetCameraViewWorld(camera.Snapshot(),201.125F,202.375F);camera.RequestPixels(3500,2400);
        const auto renderer=std::bit_cast<std::array<std::uint32_t,16>>(camera.RendererView()),snapshot=std::bit_cast<std::array<std::uint32_t,16>>(camera.Snapshot());const auto zoom=camera.Target().zoom; // 전체16DWORD와 아직 적용하지 않은 배율이다.
        cloud->WorldRedraw()=0x12345678U;local->MetaRebuild()=0xfffffffeU;dock={85,0,size.x,size.y-96};sound.clear();world.TakeChanged();
        world.Resize(size.x,size.y);
        CHECK((std::bit_cast<std::array<std::uint32_t,16>>(camera.RendererView())==renderer && std::bit_cast<std::array<std::uint32_t,16>>(camera.Snapshot())==snapshot));
        CHECK((camera.Target().pixels==std::array<std::int32_t,2>{3500,2400}) && camera.Target().zoom==zoom && cloud->WorldRedraw()==0x12345678U && local->MetaRebuild()==0xfffffffeU);
        CHECK((camera.Target().limits==std::array<std::int32_t,2>{4096-size.x,2816-size.y}) && world.TakeChanged());
        CHECK(sound.size()==1);if (sound.size()!=1) return;CHECK(sound[0].cameraX==3218 && sound[0].cameraY==2226 && sound[0].left==85 && sound[0].top==0 && sound[0].right==size.x && sound[0].bottom==size.y-96);
        world.Resize(size.x,size.y);CHECK(sound.size()==2 && !world.TakeChanged() && camera.Target().pixels[0]==3500 && camera.Target().pixels[1]==2400); // 같은 크기 재통지도 대기 목표/적용 시점에 쓰지 않는다.
        RawCameraPresentHooks present;std::uint32_t flips{};present.flip=[&] { ++flips; };present.commit=[&] { netstorm::test::camera::CommitScreenless({camera,cloud->WorldRedraw(),size.x,size.y}); }; // 원문 Present 제한과 전체497960을 실제 본체로 실행한다.
        PresentCameraTarget(camera.Target(),{gameplay->PatchDiagnostic(),size.x,size.y,gameplay->WorldCells()},present);
        CHECK(flips==1 && camera.RendererView().pixels==camera.Target().limits && camera.Snapshot().pixels==camera.Target().limits && sound.size()==2);
        world.CameraApplied();CHECK(sound.size()==3 && world.TakeChanged() && sound.back().cameraX==4096-size.x && sound.back().cameraY==2816-size.y); // 실제 적용 뒤에만 현재 원점의 표면/입자/음향을 공개한다.
    }
}
