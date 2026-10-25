#include "client/CampaignCursorClientEffects.h"
// 실제 Client 장기 표와 Session의 일반64h 수신/타이머/현재 모드 소비를 창 없이 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h" // 실제 로딩 뒤 커서의 Placement와 Update를 호출하는 완전한 선언이다.
#include <algorithm>
#include <bit>
#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace { // 실제 자산과 장기 owner를 빌리며 외부 시계/UI 효과만 관측한다.
using namespace netstorm::client;
using namespace netstorm::o;
using CameraViewWords=std::array<std::uint32_t,16>; // padding 없는 실제16DWORD 복사의 검사 관측형이다.

// Client::Run/장치 초기화를 호출하지 않고 보호 파일만 읽는 Client 옵션이다.
ClientOptions ActivitySessionOptions() {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options;
}

// 일반 Login과 같은 Session을 구성한다. Player/Reply/상태/수신 결과를 가짜 성공으로 쓰지 않는다.
struct ActivitySessionDevices {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 실제 보호 자산을 읽기만 한다.
    Client client{ActivitySessionOptions()}; // 실제 두 활동 표의 owner이며 모든 Session보다 오래 산다.
    SoundList names{OriginalEdition::Patch1078};std::array<std::uint8_t,4> you{'Y','o','u',0}; // Session보다 오래 사는 음향 이름/번역 입력이다.
    double wall{100.75};std::uint32_t frame{7},frameReads{};bool receiving{}; // 현재 외부 시계/프레임과64h 본문 관측 범위다.
    std::vector<std::string> trace;std::vector<std::array<std::uint32_t,2>> removed; // 실제 record/Wall/Frame/UI 순서와 삭제 인자만 관측한다.
    std::function<void()> onWall,onFrame;std::function<void(std::uint32_t,std::uint32_t)> onRemove; // 현재 getter/마지막 UI 효과의 명시 부수효과다.

    ActivitySessionDevices() { client.SubscriberGlobals()->MySubscriber()=client.SubscriberGlobals()->LoginSubscriber(); } // Tutorial Init 앞의 실제 현재/login ID 복사 입력이며 로그인 flags/응답을 합성하지 않는다.

    // 실제 Client의 같은 장기 owner를 공급하고 선택된 외부 장치만 명시한다.
    RawCampaignSessionServices Services(bool login=true) {
        names.Initialize();RawCampaignSessionServices result;result.connectionActivityGlobals=client.ConnectionActivityGlobals();result.localStateGlobals=client.LocalStateGlobals();result.gameplayGlobals=client.GameplayGlobals();result.subscriberGlobals=client.SubscriberGlobals();
        result.soundNames=&names;result.unixSeconds=[] { return 946684800; };result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[this] { if (receiving) { trace.emplace_back("wall");if (onWall) onWall(); }return wall; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("활동 Session 검사의 미선택 UI"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("활동 Session 검사의 미선택 삭제"); };result.player.log=[](std::uint32_t,std::string_view) {};result.missionOutcome.demo=[] { return 0U; };
        result.connectionFrameNumber=[this] { ++frameReads;if (receiving) trace.emplace_back("frame");if (onFrame) onFrame();return frame; };
        result.removeConnectionStatus=[this](auto kind,auto player) { trace.emplace_back("remove");removed.push_back({kind,player});if (onRemove) onRemove(kind,player);receiving=false; };
        if (login) {
            CampaignLoginServices effects; // 실제 handshake의 장치 경계이며 서버/클라이언트 패킷 몸체는 Session이 실행한다.
            effects.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };effects.wallSeconds=result.sound.wallSeconds;effects.clock=[] { return 100750U; };
            effects.refresh=[] {};effects.playerUiBusy=[] { return false; };effects.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};
            effects.log=[](std::string_view) {};effects.observePacket=[this](auto,bool,std::uint32_t,auto packet) { if (packet[1]==0x64) { trace.emplace_back("record");receiving=true; } };
            result.localLogin=std::move(effects);result.ai.participation.clearChat=[](std::uint32_t) {};result.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
        }
        return result;
    }
    // 로드 전 Session을 실제 보호 Fort에 구성하고 호출자가 LoadLocalConnection 선택 시점을 정한다.
    std::unique_ptr<RawCampaignSession> Create(RawCampaignSessionServices services) {
        return std::make_unique<RawCampaignSession>(assets.game,assets.files,FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable()),"thewarbegins",std::move(services));
    }
};

// PE0052ee4e의64h/version1/length5이며 마지막 BYTE는 본문이 읽지 않는 독립 입력이다.
std::array<std::uint8_t,5> ActivitySessionPacket(std::uint8_t version=1) { return {version,0x64,5,0,0xa5}; }
// 일반 수신의 기록에서 실제64h 개수만 관측하며 수신기를 직접 호출하지 않는다.
std::size_t ActivityWireCount(const RawCampaignSession& session) {
    const auto packets=session.LocalConnectionPackets();return static_cast<std::size_t>(std::count(packets.begin(),packets.end(),std::uint8_t{0x64}));
}
}

// 같은 실제 Client의 표/간극 주소는 미선택 Session의 생성/소멸/재대여에서 바뀌지 않는다.
TEST_CASE(ConnectionActivitySession_ActualClientOwnerSurvivesUnselectedSessions) {
    static_assert(std::is_same_v<decltype(std::declval<const Client&>().ConnectionActivityGlobals()),const std::shared_ptr<ClientConnectionActivityGlobals>&>); // provider가 같은 실제 장기 소유 슬롯을 빌리는 계약이다.
    ActivitySessionDevices devices;const auto owner=devices.client.ConnectionActivityGlobals();CHECK(owner->ActiveStamp()==0 && owner->ActiveZoneWord()==0);
    for (std::size_t index=0;index<kConnectionActivityEntries;++index) { // Client 최초 BSS0을 확인한 뒤 두 표의 모든 칸을 구별한다.
        CHECK(owner->Frames()[index]==0 && owner->WallSeconds()[index]==0);owner->Frames()[index]=static_cast<std::uint32_t>(0x1000+index);owner->WallSeconds()[index]=static_cast<std::uint32_t>(0x2000+index);
    }
    owner->ActiveStamp()=37;owner->ActiveZoneWord()=41;const auto frames=owner->Frames(),wall=owner->WallSeconds();const auto* address=owner->Frames().data();
    for (int pass=0;pass<2;++pass) { // 실제 Session 두 수명에 같은 Client owner를 빌리되64h/Timer를 선택하지 않는다.
        auto services=devices.Services(false);services.connectionFrameNumber={};services.removeConnectionStatus={};{ const auto session=devices.Create(std::move(services));CHECK(session->LocalConnectionPhase()==0); }
        CHECK(devices.client.ConnectionActivityGlobals()==owner && owner->Frames().data()==address && owner->Frames()==frames && owner->WallSeconds()==wall && owner->ActiveStamp()==37 && owner->ActiveZoneWord()==41);
    }
    CHECK(!devices.frameReads && devices.trace.empty() && devices.removed.empty());
}

// 실제 Zone은 간극만 쓰고 일반64h는 송신 뒤 같은 큐/record/현재 표를 소비한다.
TEST_CASE(ConnectionActivitySession_ZoneAndQueuedWireUseActualClientTables) {
    ActivitySessionDevices devices;const auto owner=devices.client.ConnectionActivityGlobals();owner->Frames().fill(0x11111111U);owner->WallSeconds().fill(0x22222222U);owner->ActiveStamp()=37;owner->ActiveZoneWord()=41;
    auto session=devices.Create(devices.Services());session->LoadLocalConnection();CHECK(session->LocalConnectionPhase()==14 && owner->ActiveStamp()==0 && owner->ActiveZoneWord()==0 && !devices.frameReads);
    CHECK(std::all_of(owner->Frames().begin(),owner->Frames().end(),[](auto value) { return value==0x11111111U; }) && std::all_of(owner->WallSeconds().begin(),owner->WallSeconds().end(),[](auto value) { return value==0x22222222U; }));
    const auto before=ActivityWireCount(*session);const auto packet=ActivitySessionPacket();session->SendLocalPacket(1,packet);CHECK(ActivityWireCount(*session)==before && owner->Frames()[9]==0x11111111U && owner->WallSeconds()[9]==0x22222222U);
    session->PumpLocalConnection();devices.receiving=false;CHECK(ActivityWireCount(*session)==before+1 && devices.frameReads==1 && owner->Frames()[9]==7 && owner->WallSeconds()[9]==100 && devices.removed.empty());CHECK((devices.trace==std::vector<std::string>{"record","wall","frame"})); // 실제 missionPost1은 같은004d3090 관문에서 UI 삭제를 생략한다.
    devices.trace.clear();devices.client.GameplayGlobals()->world.missionPost=false;devices.frame=0;devices.wall=125.75;session->SendLocalPacket(1,ActivitySessionPacket(0x7f));session->PumpLocalConnection();
    CHECK((devices.removed==std::vector<std::array<std::uint32_t,2>>{{2,9}}));CHECK((devices.trace==std::vector<std::string>{"record","wall","frame","remove"}));CHECK(owner->Frames()[9]==0 && owner->WallSeconds()[9]==125 && devices.frameReads==2 && ActivityWireCount(*session)==before+2); // 정책0은 높은 버전도 일반 dispatcher에서 그대로 수신하고 frame0을 정규화하지 않는다.
    CHECK(owner->Frames()[8]==0x11111111U && owner->Frames()[10]==0x11111111U && owner->WallSeconds()[8]==0x22222222U && owner->WallSeconds()[10]==0x22222222U);
}

// Frame 효과 뒤 현재 server를 조회하고 최종 삭제 효과가 바꾼 실제 표를 반환 뒤 보존한다.
TEST_CASE(ConnectionActivitySession_CurrentServerAndFinalUiEffectsStayAliased) {
    ActivitySessionDevices devices;auto session=devices.Create(devices.Services());session->LoadLocalConnection();auto core=session->Core();const auto owner=devices.client.ConnectionActivityGlobals();devices.client.GameplayGlobals()->world.missionPost=false;
    owner->Frames()[9]=17;owner->WallSeconds()[9]=19;devices.wall=200.75;devices.frame=0xfedcba98U;
    devices.onFrame=[&] { CHECK(owner->Frames()[9]==17 && owner->WallSeconds()[9]==19);devices.client.GameplayGlobals()->Server()=false; };
    devices.onRemove=[&](auto kind,auto player) { CHECK(kind==1 && player==0 && owner->Frames()[9]==0xfedcba98U && owner->WallSeconds()[9]==200);owner->Frames()[9]=0xabcdef01U;owner->WallSeconds()[9]=0x12345678U; };
    session->SendLocalPacket(1,ActivitySessionPacket());session->PumpLocalConnection();CHECK((devices.removed==std::vector<std::array<std::uint32_t,2>>{{1,0}}));CHECK(owner->Frames()[9]==0xabcdef01U && owner->WallSeconds()[9]==0x12345678U && !core.processState.Server());
    CHECK((devices.trace==std::vector<std::string>{"record","wall","frame","remove"}));
}

// 누락 Frame/UI는 ctor에서 앞당겨 요구하지 않고 실제 수신 선택 위치의 부분 효과를 보존한다.
TEST_CASE(ConnectionActivitySession_MissingSelectedHooksFailAtNativeBoundary) {
    for (const bool missingFrame:{true,false}) { // Frame은 두 store 전, UI는 두 store 뒤의 서로 다른 실패다.
        ActivitySessionDevices devices;auto services=devices.Services();if (missingFrame) services.connectionFrameNumber={};else services.removeConnectionStatus={};auto session=devices.Create(std::move(services));session->LoadLocalConnection();devices.client.GameplayGlobals()->world.missionPost=false;
        const auto owner=devices.client.ConnectionActivityGlobals();owner->Frames()[9]=17;owner->WallSeconds()[9]=19;const auto before=ActivityWireCount(*session);session->SendLocalPacket(1,ActivitySessionPacket());std::string failure;
        try { session->PumpLocalConnection(); }catch (const std::logic_error& error) { failure=error.what(); }devices.receiving=false;
        CHECK(failure==(missingFrame ? "실제 연결 활동 프레임 조회 미연결" : "실제 연결 상태 WarningMessages 삭제 미연결"));
        CHECK(ActivityWireCount(*session)==before+1 && devices.removed.empty());
        if (missingFrame) { CHECK(owner->Frames()[9]==17 && owner->WallSeconds()[9]==19 && !devices.frameReads);CHECK((devices.trace==std::vector<std::string>{"record","wall"})); }
        else { CHECK(owner->Frames()[9]==7 && owner->WallSeconds()[9]==100 && devices.frameReads==1);CHECK((devices.trace==std::vector<std::string>{"record","wall","frame"})); }
    }
}

// 비서버 Timer는 Client9 객체가 존재할 때 같은 Receive를 부르며 가짜64h 송수신을 만들지 않는다.
TEST_CASE(ConnectionActivitySession_TimerRemoteStatusUsesSameModuleWithoutPacket) {
    ActivitySessionDevices devices;auto session=devices.Create(devices.Services());session->LoadLocalConnection();const auto owner=devices.client.ConnectionActivityGlobals();auto core=session->Core();core.processState.Server()=false;
    CHECK(devices.client.GameplayGlobals()->world.missionPost);devices.wall=130.75;devices.frame=0x12345678U;devices.client.LocalStateGlobals()->LoginEnd()=160.75;const auto before=std::vector<std::uint8_t>(session->LocalConnectionPackets().begin(),session->LocalConnectionPackets().end());
    session->PumpLocalConnection();CHECK(owner->Frames()[9]==0x12345678U && owner->WallSeconds()[9]==130 && devices.frameReads==1 && devices.removed.empty());CHECK(devices.client.LocalStateGlobals()->LoginEnd()==0.0 && session->LocalConnectionPhase()==14);
    CHECK(std::equal(before.begin(),before.end(),session->LocalConnectionPackets().begin(),session->LocalConnectionPackets().end()) && ActivityWireCount(*session)==0 && devices.trace.empty()); // 같은 표 갱신은 타이머의 직접 Native 호출이며 packet record를 합성하지 않는다.
}

// 실제 Client/Session/Storage가 같은 owner를 빌리고 기존 Prepare의 Gun 사본으로 false를 재오염하지 않는다.
TEST_CASE(ConnectionActivitySession_AuthorityOwnerIdentityAndPrepareDoNotResynchronize) {
    std::shared_ptr<netstorm::client::ClientGameplayGlobals> globals; // 첫Client반납뒤에도같은Authority를가진실제공유owner를유지한다.
    {
        ActivitySessionDevices first;auto session=first.Create(first.Services(false));auto core=session->Core();globals=first.client.GameplayGlobals();
        CHECK(session->World().Storage()->Authority()==globals->Authority() && &core.processState.Server()==&globals->Server() && &core.processState.Boss()==&globals->Boss());
        globals->Server()=false;globals->Boss()=false;core.processState.server=core.processState.boss=true;session->PrepareFrame({},true);CHECK(!core.processState.Server() && !core.processState.Boss() && !core.pool.IsServer());
        core.processState.Server()=true;CHECK(globals->Server() && !globals->Boss());globals->Server()=false;session.reset();CHECK(!globals->Borrowed() && !globals->Server() && !globals->Boss());
        session=first.Create(first.Services(false));CHECK(globals->Server() && globals->Boss() && session->Core().pool.IsServer()); // 같은일반Session의기존fresh호환정책을검사한다.
        globals->Server()=false;globals->Boss()=false; // 다음Client의최초true가보존한첫owner의false를덮지않는지구별한다.
    }
    CHECK(!globals->Borrowed() && !globals->Server() && !globals->Boss()); // Session을먼저반납하고Client도정상소멸한뒤두번째Client를만든다.
    {
        ActivitySessionDevices second;auto other=second.Create(second.Services(false));auto separate=other->Core();const auto independent=second.client.GameplayGlobals();
        CHECK(other->World().Storage()->Authority()==independent->Authority() && globals->Authority()!=independent->Authority());
        CHECK(independent->Server() && independent->Boss() && separate.pool.IsServer());CHECK(!globals->Server() && !globals->Boss());
    }
}

// 다른 실제 권한 owner는 gameplay/fresh/Player/Storage 대여보다 먼저 거부한다.
TEST_CASE(ConnectionActivitySession_ForeignAuthorityFailsBeforeAnyBorrowOrMutation) {
    ActivitySessionDevices devices;const auto globals=devices.client.GameplayGlobals();globals->Server()=globals->Boss()=false;auto services=devices.Services(false);auto foreign=MakeRawSurfaceStorage(devices.assets.game);services.surfaceStorage=foreign;
    const auto before=std::vector<std::uint8_t>(foreign->pool.Bytes().begin(),foreign->pool.Bytes().end());bool rejected{};try { devices.Create(std::move(services)); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && !globals->Borrowed() && !foreign->Borrowed() && foreign->Generation()==0 && !globals->Server() && !globals->Boss() && foreign->Authority()->server && foreign->Authority()->boss);CHECK(std::equal(before.begin(),before.end(),foreign->pool.Bytes().begin()));
}

// 실제 Client의 snapshot/논리Renderer/목표는 일반 Session 생성·반납·재대여에서 초기화하지 않는다.
TEST_CASE(CameraView_ActualClientSessionAliasesFractionAndCurrentCursorReads) {
    ActivitySessionDevices devices;auto& camera=devices.client.GameplayGlobals()->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.RendererView(),1.25F,2.5F);camera.CopyRendererSnapshot();camera.RequestPixels(71,83);const auto before=std::bit_cast<CameraViewWords>(camera.Snapshot());const auto requested=camera.Target().pixels;Renderer nativeRenderer(1024,768);
    for (std::uint32_t index=0;index<2;++index) { // 같은Client owner를실제Load한두Runtime이차례로빌린다.
        std::uint32_t invalidated{},resets{};CameraHomeState home;home.width=1024;home.height=768;std::unique_ptr<RawCampaignSession> session;auto services=devices.Services(false);
        services.install.resetCamera=[&] { CHECK(session);++resets;session->ResetCamera(home,[&](std::int32_t x,std::int32_t y) { camera.RequestPixels(x,y);camera.ApplyPixelsToRendererAndSnapshot(x,y); }); }; // Load의실제Reset/Home본문과카메라setter를연결하며원점효과를빈함수로버리지않는다.
        session=devices.Create(std::move(services));CHECK(&session->CameraGlobals()==&camera && &session->CameraGlobals().Snapshot().worldBits==&camera.Snapshot().worldBits);CHECK(resets==0 && std::bit_cast<CameraViewWords>(camera.Snapshot())==before && camera.Target().pixels==requested); // 아직커서를요청하지않고생성보존만확인한다.
        session->Load();CHECK(resets==1); // 실제Player/World/Graph/Reset/커서설치꼬리가정상복귀한뒤에만공개Cursor를사용한다.
        SetCameraViewWorld(camera.RendererView(),std::bit_cast<float>(before[0]),std::bit_cast<float>(before[1]));camera.CopyRendererSnapshot();camera.RequestPixels(requested[0],requested[1]); // Load의실제초기시야선택후기존과같은분수입력을새검사입력으로명시설정한다.
        auto& cursor=session->Cursor();CHECK(cursor.Placement().sharedScreenOrigin==&camera.Snapshot().pixels);cursor.SetView({999,888,0,0,640,480});session->SetParticleView({999,888,0,0,640,480},{},false);
        CHECK(std::bit_cast<CameraViewWords>(camera.Snapshot())==before && camera.Target().pixels[0]==71 && cursor.Placement().ScreenOriginX()==20 && cursor.Placement().ScreenOriginY()==27);auto& drop=session->CursorDropSettings();drop.lockDepth=1;cursor.Placement().hotspotX=cursor.Placement().hotspotY=cursor.Placement().imageWidth=cursor.Placement().imageHeight=0;cursor.Update(0,0);CHECK(drop.x==1.25F && drop.y==2.5F);
        SurfaceDisplayHooks display;display.invalidate=[&](SquidDisplayRect rect,std::uint32_t flags) { CHECK(rect.left==0 && rect.top==0 && rect.right==640 && rect.bottom==480 && flags==0);++invalidated; };session->SetParticleView({20,27,0,0,640,480},display,false);CHECK(invalidated==1); // 장치연결전pending전체갱신을실제현재장치에한번전달한다.
        SetCameraViewWorld(camera.Snapshot(),1.5F,2.75F);session->SetParticleView({24,30,0,0,640,480},display,false);CHECK(invalidated==2);cursor.Update(0,0);CHECK(drop.x==1.5F && drop.y==2.75F && cursor.Placement().ScreenOriginX()==24 && cursor.Placement().ScreenOriginY()==30);SetCameraViewWorld(camera.Snapshot(),1.25F,2.5F);
        auto& data=devices.client.GameplayGlobals()->CursorGlobals();CHECK(&cursor.Lifecycle()==&data.Lifecycle() && &cursor.Placement()==&data.Placement() && &cursor.SnapToGrid()==&data.SnapToGrid());data.SnapToGrid()=1;cursor.Update(0,0);CHECK(drop.x==1 && drop.y==2);data.SnapToGrid()=0; // 같은현재DWORD의변경을실제RawCursorPosition이소비한다.
        CampaignCursorEffects effects;effects.placement.invalidate=MakeClientCursorPrepareHooks(devices.client,[&]() -> Renderer& { return nativeRenderer; }).invalidate;cursor.SetEffects(std::move(effects));const auto local=drop.localPlayer,battle=drop.battle,blocked=drop.blocked;drop.localPlayer=57;drop.battle=59;drop.blocked=61;
        CHECK(session->HasCursorRuntimeForState());session->CancelCursorForState();CHECK(drop.localPlayer==57 && drop.battle==59 && drop.blocked==61 && drop.lockDepth==1 && data.Lifecycle().previewCount==0);drop.localPlayer=local;drop.battle=battle;drop.blocked=blocked; // NativeCancel에는외곽game.prepare의Player/모드동기화가들어가지않는다.
    }
    CHECK(std::bit_cast<CameraViewWords>(camera.Snapshot())==before && camera.Target().pixels[1]==83 && !devices.client.GameplayGlobals()->Borrowed());
}

// 선택된 생성 실패도 이미 존재하는 카메라/분수/목표를 초기값이나 이전 호스트 사본으로 덮지 않는다.
TEST_CASE(CameraView_ActualClientFailedSessionKeepsCurrentOwner) {
    ActivitySessionDevices devices;auto& camera=devices.client.GameplayGlobals()->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.RendererView(),3.125F,4.875F);camera.CopyRendererSnapshot();camera.RequestPixels(71,83);const auto before=std::bit_cast<CameraViewWords>(camera.Snapshot());auto services=devices.Services(false);services.soundNames=nullptr; // World 구성 뒤 RequireSounds에서 실제 필수 장치 누락을 선택한다.
   bool rejected{};
    try { static_cast<void>(devices.Create(std::move(services))); }catch (const std::invalid_argument& error) { rejected=std::string(error.what()).find("SoundList")!=std::string::npos; }
    CHECK(rejected && !devices.client.GameplayGlobals()->Borrowed() && std::bit_cast<CameraViewWords>(camera.Snapshot())==before && camera.Target().pixels[0]==71);auto session=devices.Create(devices.Services(false));CHECK(&session->CameraGlobals()==&camera && session->CameraGlobals().Snapshot().worldBits[0]==0x40480000U);
}
