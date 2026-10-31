// 실제 World의 상태 Tick/패킷/완료 설치를 분리해 창·소켓·게임 대기 없이 확인한다.
#include "o/RawMissionFrameGate.h"

namespace { // 같은 실제 상태/표면/요청과 명시 UI·시계 장치를 각 World 수명에 보관한다.
struct MissionAsyncScene {
    const netstorm::test::dudespawn::DudeAssets& assets{netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)}; // 보호 TYPE/SHP/fort를 읽기만 한다.
    LocalStateSessionDevices devices; // 기존 실제 로컬 프로토콜의 외부 시계/음향/UI 경계다.
    std::shared_ptr<ClientLocalStateGlobals> state{std::make_shared<ClientLocalStateGlobals>()}; // Native 다섯 블록과 기한의 단일 owner다.
    std::shared_ptr<ClientGameplayGlobals> gameplay{std::make_shared<ClientGameplayGlobals>()}; // 현재 Player/카메라/권한은 Session과 같은 주소다.
    std::shared_ptr<RawSurfaceStorage> surface{MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux())}; // 부분 반납 뒤에도 대여 해제를 확인한다.
    std::shared_ptr<MissionFortLoadRequest> request{std::make_shared<MissionFortLoadRequest>()}; // 실제86BYTE 복사본과 반납 후 인자를 보관한다.
    std::array<ScreenColor,256> palette{};std::array<std::uint32_t,256> logical{}; // 실제 BGRX와 원문 RGB DWORD를 구별한 선택 색이다.
    std::uint32_t redraw{},pauseDepth{};std::size_t packets{},closed{},frameReads{}; // 실제 콜백/게임 프레임 준비만 관측한다.
    std::vector<std::uint32_t> modes; // 실제 UI2/3의 호출 순서다.
    netstorm::o::MissionFrameGateState gate; // 설치 중 자기 정지가 같은 미션 상태에 남는지 확인한다.
    std::unique_ptr<GameWorld> world; // 모든 callback 대상과 외부 장치보다 먼저 반납한다.
    MissionAsyncScene() { // 독립 입력을 만든 뒤 같은 실제 World/로컬 요청을 설치한다.
        const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const std::uint32_t port=6799;MissionFortRequestHooks hooks;hooks.selectFort=[this](auto name) { request->fort=name; };hooks.duplicate=[this](auto& packet) { return CaptureMissionFortZone(*request,packet); };WriteMissionFortRequest("thewarbegins",{request->next,request->sequence,address,port},hooks);
        for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(255-index));palette[index]={color.blue,color.green,color.red,0};logical[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 선택한 실제 색을 역순으로 바꿔 기본색 재사용을 구별한다.
        surface->ownerMode.battle=true;gameplay->WorldCells()=256;auto services=devices.Make(state,true);services.gameplayGlobals=gameplay;services.surfaceStorage=surface;services.cameraCommit=netstorm::test::camera::CommitScreenless;services.missionFortRequest=request; // 실제 State7의 전투 모드가 Zone/로더보다 앞선다.
        services.localLogin->initializeRenderer=[this] { CHECK(request->consumed);gameplay->Camera().InitializeNativeRenderer(1024,768,256,redraw,gameplay->MapRevision()); };
        services.localLogin->uiMode=[this](auto mode) { modes.push_back(mode);if (mode==3) CHECK(closed>0 && state->State().current[0]==13); };
        services.localLogin->closeProgress=[this] { ++closed; };services.localLogin->battlePalette=[this] { CHECK(world && !world->Session().HasCursorRuntimeForState());world->SetPalette(palette);palette.fill({0,0,0,0}); };
        devices.observe=[this](auto,auto) { ++packets; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());world=std::make_unique<GameWorld>(assets.game,assets.files,std::move(fort),MissionPlayers{},request->fort,std::move(services),ScreenPoint{1024,768},true);
        world->frameTime=[this] { ++frameReads;netstorm::o::FrameTime value;value.wall=devices.wall;return value; };world->paused=[] { return true; };
    }
    void PauseBeforeConnection() { netstorm::o::PauseMissionFrameForDialog(gate,[this] { return pauseDepth!=0; },[this] { ++pauseDepth; }); } // 실제 자기 정지 공통 몸체를 미설치 World에서도 호출한다.
};
}

// 상태8→다음 Tick9/10→패킷→상태14와 호스트 공개 사이를 실제 큐/위임으로 구별한다.
TEST_CASE(MissionAsyncConnection_FrameBoundariesAndOneTimePublication) {
    MissionAsyncScene scene;scene.PauseBeforeConnection();scene.PauseBeforeConnection();CHECK(scene.pauseDepth==1 && scene.gate.scriptPaused==1);scene.world->StartLocalConnection();auto& session=scene.world->Session();
    CHECK(scene.world->Connecting() && session.LocalConnectionPhase()==8 && scene.state->State().next[0]==9 && !session.LocalConnectionReady() && !session.HasCursorRuntimeForState() && scene.packets==0 && scene.modes==std::vector<std::uint32_t>{2});CHECK(!scene.world->CompleteLocalConnection());
    CHECK(scene.world->Report().find("raw_connection_pending\t8\t0\t0")!=std::string::npos); // 진단도 준비 중 커서의 물리 접근을 열지 않는다.
    scene.world->PrepareKernelFrame();scene.world->FinishKernelFrame();scene.world->RunFrame();scene.world->Step(1.0);CHECK(scene.frameReads==0 && scene.packets==0);session.PumpLocalConnection();CHECK(session.LocalConnectionPhase()==8 && scene.packets==0);
    CHECK(session.PollLocalConnectionState());CHECK(session.LocalConnectionPhase()==10 && !session.LocalConnectionReady() && !session.HasCursorRuntimeForState() && scene.packets==0);session.PumpLocalConnection();
    CHECK(session.LocalConnectionReady() && session.LocalConnectionPhase()==14 && session.PlayerKnown() && scene.packets>0 && scene.modes==std::vector<std::uint32_t>({2,3}) && scene.closed>0);CHECK(scene.world->Connecting());scene.world->PrepareKernelFrame();CHECK(scene.frameReads==0);
    CHECK(scene.world->CompleteLocalConnection() && !scene.world->Connecting());const auto expected=BuildPaletteColorTable(scene.logical);const auto actual=session.Cursor().Display().TextColors();CHECK(actual[0]==expected.basic[7] && actual[1]==expected.basic[5]);CHECK(!scene.world->CompleteLocalConnection());
    bool rejected{};try { scene.world->StartLocalConnection(); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && !scene.world->Connecting() && session.LocalConnectionReady());
}

// timeout은 다음3을 한 번 예약하고 큰 부모는 다음 Tick에 남기며 미설치 World도 정상 반납한다.
TEST_CASE(MissionAsyncConnection_TimeoutDefersParentAndAbandonsPartialWorld) {
    MissionAsyncScene scene;scene.world->StartLocalConnection();CHECK(scene.gameplay->Borrowed() && scene.surface->Borrowed());scene.devices.wall=scene.state->ConnectEnd()+1.0;
    CHECK(scene.world->Session().PollLocalConnectionState());CHECK(scene.state->State().next[0]==3 && scene.world->Session().LocalConnectionPhase()==8 && scene.packets==0 && !scene.world->Session().LocalConnectionReady());CHECK(!scene.world->Session().PollLocalConnectionState() && scene.state->State().next[0]==3);CHECK(!scene.world->CompleteLocalConnection());
    scene.world.reset();CHECK(!scene.gameplay->Borrowed() && !scene.surface->Borrowed() && scene.state->State().next[0]==3); // 반납이 실제 다음 부모나 장기 블록을 초기화하지 않는다.
}
