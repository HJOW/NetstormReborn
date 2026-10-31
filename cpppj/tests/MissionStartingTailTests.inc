// 실제 로그인 후반의 UI2→기한→다음8/인자20→팔레트→접속→UI3와 생성 전 색 수명을 접합한다.
#include "client/CampaignClientServices.h"
#include "client/RawButtonControls.h"
#include "client/GameWorld.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/PaletteColors.h"
#include "CameraTestDevices.h"

// 실제 연결기가 명령 상수를 고정하거나 단순 refresh로 대체하면 캡처/소리가 발생하지 않는다.
TEST_CASE(MissionStartingTail_ActualBindingRequeriesHudCommandAndRoutes) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));std::size_t sounds{};
    RawButtonHooks hooks;hooks.sound=[&] { ++sounds; };RawButtonControls controls(OriginalEdition::Patch1078,std::move(hooks));const RawButtonAssets button{90,0,20,16,{7,255,13}};const std::array<GumpControl,1> nodes{{{7,{0,0,20,16},false,true,false}}};std::array<std::string,8> labels{};labels[7]="Current";controls.SetControls(nodes,labels,640,480,button);
    CampaignLoginServices login;BindCampaignLoginStateClientServices(client,controls,login);CHECK(bool(login.initializeRenderer) && bool(login.uiMode) && bool(login.battlePalette));
    client.LocalStateGlobals()->HudModeCommand()=InputCode::kLeftButton;login.uiMode(2);CHECK(controls.Pressed()==7 && controls.HasCapture() && sounds==1);
    client.LocalStateGlobals()->HudModeCommand()=InputCode::kLeftButton|InputCode::kRelease;login.uiMode(3);CHECK(!controls.Pressed() && !controls.HasCapture() && sounds==1); // 현재 명령을 다시 읽고 mode를 첫 좌표로 실제 Root에 전달한다.
}

// 장치 누락의 중간 쓰기와 실제 World의 생성 전 팔레트를 같은 로컬 요청/송수신으로 확인한다.
TEST_CASE(MissionStartingTail_ActualWorldOrderingFailuresAndDeferredPalette) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const unsigned selected:{0U,1U,2U}) { // UI 장치 누락/팔레트 장치 누락/전체 접합을 각각 새 Session으로 실행한다.
        LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();auto gameplay=std::make_shared<ClientGameplayGlobals>();auto services=devices.Make(owner,true);services.gameplayGlobals=gameplay;services.cameraCommit=netstorm::test::camera::CommitScreenless;std::vector<std::string> events;std::size_t packets{};GameWorld* current{}; // 실제 World 재설정도 기존 독립497960을 사용한다.
        services.localLogin->closeProgress=[] {}; //656의 후반 인자/색 비교 범위이며 안내 실제 수명은 별도 비동기 접합에서 관측한다.
        auto request=std::make_shared<MissionFortLoadRequest>();const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const std::uint32_t port=6799;MissionFortRequestHooks requestHooks;requestHooks.selectFort=[request](auto name) { request->fort=name; };requestHooks.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };WriteMissionFortRequest("thewarbegins",{request->next,request->sequence,address,port},requestHooks);services.missionFortRequest=request;
        std::array<ScreenColor,256> palette{};std::array<std::uint32_t,256> logical{};
        for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(255-index));palette[index]={color.blue,color.green,color.red,0};logical[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 실제 호스트 BGRX와 원문 RGB DWORD를 구별한다. 기존 색 역순이며 caller 버퍼 변경 뒤에도256색이 남아야 한다.
        const auto expected=BuildPaletteColorTable(logical);owner->ActivityEnd()=17;
        std::uint32_t redraw{};services.localLogin->initializeRenderer=[&] { CHECK(request->consumed && request->zone.empty() && packets==0);events.emplace_back("Renderer");gameplay->Camera().InitializeNativeRenderer(1024,768,256,redraw,gameplay->MapRevision());gameplay->player.playerKnown=99;owner->ActivityEnd()=19; };
        if (selected!=0) services.localLogin->uiMode=[&](auto mode) { events.push_back("UI"+std::to_string(mode));if (mode==2) { CHECK(gameplay->player.playerKnown==0 && owner->ActivityEnd()==19 && packets==0);devices.wall=131.5; }else { CHECK(mode==3 && owner->State().current[0]==13 && owner->State().phase[0]!=14); } };
        if (selected!=1) services.localLogin->battlePalette=[&] {
            events.emplace_back("Palette");CHECK(owner->ActivityEnd()==141.5 && owner->State().immediate[0]==8 && owner->State().immediate[1]==20 && owner->State().immediate[32]==1 && owner->State().current[0]==7 && owner->State().current[32]==0 && packets==0);CHECK(current && !current->Session().HasCursorRuntimeForState());
            bool blocked{};try { static_cast<void>(current->Session().Cursor()); }catch (const std::logic_error&) { blocked=true; }CHECK(blocked);current->SetPalette(palette);palette.fill({0,0,0,0}); // 생성 전에는 입력용 Cursor 관문을 열지 않고 원래 색을 복사한다.
        };
        devices.observe=[&](auto,auto) { if (!packets) { events.emplace_back("Packet");CHECK(owner->ActivityEnd()==141.5 && owner->State().current[1]==20 && owner->State().current[32]==1); }++packets; }; // 이전 Zone 핸들은 원문 그대로 남지만 접속에서 역참조하지 않는다.
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},request->fort,std::move(services),{1024,768},true);current=&world;
        bool rejected{};try { world.BeginLocalConnection(); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find("실제 미션 로그인 장치 미연결")!=std::string::npos;std::fprintf(stderr,"STARTING_TAIL selected=%u phase=%u known=%u cursor=%u error=%s\n",selected,world.Session().LocalConnectionPhase(),world.Session().PlayerKnown(),world.Session().HasCursorRuntimeForState(),error.what()); }
        if (selected==0) { CHECK(rejected && events==std::vector<std::string>{"Renderer"} && owner->ActivityEnd()==19 && owner->State().immediate[0]==0 && packets==0); }
        else if (selected==1) { CHECK(rejected && events==std::vector<std::string>({"Renderer","UI2"}) && owner->ActivityEnd()==141.5 && owner->State().immediate[0]==8 && owner->State().immediate[1]==20 && owner->State().current[0]==7 && packets==0); }
        else { const bool ready=!rejected && events==std::vector<std::string>({"Renderer","UI2","Palette","Packet","UI3"}) && packets>0 && world.Session().LocalConnectionPhase()==14 && world.Session().PlayerKnown();CHECK(ready);if (ready) { const auto colors=world.Session().Cursor().Display().TextColors();CHECK(colors[0]==expected.basic[7] && colors[1]==expected.basic[5]);CHECK(world.Session().Cursor().Lifecycle().warningFont==expected.basic[7] && world.Session().Cursor().Lifecycle().costFont==expected.basic[5]); } } // 수명 자체가 실패하면 같은 설치 전 Cursor 예외를 추가로 만들지 않는다.
    }
}
