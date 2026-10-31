// 시작 CD 기록과 실제 Client/Session의 Zone 뒤 Renderer 호출을 창 없이 접합한다.
#include "client/ClientCdGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "o/RawBaseFileReadPolicy.h"

// 시작0 store는 조회보다 앞서며 State7 결과/RNG/캐시와 Client 사이를 합치지 않는다.
TEST_CASE(MissionEntry_StartupCdZeroBeforeQueryAndSeparateLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;auto client=std::make_unique<Client>(std::move(options));const auto owner=client->CdGlobals();CHECK(owner->StartupPresent()==0);
    client->LocalStateGlobals()->CdPresent()=37;client->RandomGlobals()->CrtState()=0x89abcdefU;std::size_t probes{},roots{};
    CdPresenceHooks hooks;hooks.path=[](auto root,auto marker) { return JoinBaseFileRoot(root,marker); };
    hooks.exists=[&](auto) { CHECK(owner->StartupPresent()==0);++probes;return true; };
    hooks.roots=[&](auto output) { ++roots;const std::array<char,5> data{'D',':','\\',0,0};std::copy(data.begin(),data.end(),output.begin());return 5U; };
    owner->InitializeStartup(hooks);CHECK(owner->StartupPresent()==1 && roots==1 && probes==1);const auto* address=&owner->StartupPresent();
    hooks.exists=[&](auto)->bool { CHECK(owner->StartupPresent()==0);throw std::logic_error("선택된 실제 CD 조회의 명시 실패"); };bool rejected{};try { owner->InitializeStartup(hooks); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && owner->StartupPresent()==0 && std::string(owner->CachedRoot().data())=="D:\\" && roots==1);
    hooks.exists=[&](auto) { ++probes;return false; };hooks.roots=[&](auto) { ++roots;return 0U; };owner->InitializeStartup(hooks);
    CHECK(owner->StartupPresent()==0 && owner->CachedRoot()[0]==0 && roots==2 && probes==2 && &owner->StartupPresent()==address);CHECK(client->LocalStateGlobals()->CdPresent()==37 && client->RandomGlobals()->CrtState()==0x89abcdefU);
    client.reset();ClientOptions otherOptions;otherOptions.gameDirectory=assets.root;Client other(std::move(otherOptions));CHECK(other.CdGlobals()->StartupPresent()==0 && other.CdGlobals()->CachedRoot().data()!=owner->CachedRoot().data()); // 실제 Client의 동시 생성 금지를 유지하며 순차 수명만 검사한다.
}

// 실제 앞선 Player 소유권을 채택한 두 Session에서 누락 거부와 원문 호출 위치/장기 카메라 초기화를 구별한다.
TEST_CASE(MissionEntry_LoginRendererAfterZoneReleaseBeforeFirstPacket) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const bool connected:{false,true}) { // 장치 누락/실제 Client 장치를 각각 새 Client/Session 수명으로 실행한다.
        ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));InitializeClientPlayerRecords(client);client.SubscriberGlobals()->MySubscriber()=client.SubscriberGlobals()->LoginSubscriber();const std::uint32_t metamap{};client.CoreDataGlobals()->Select(client.GameplayGlobals()->player,metamap,0,1);
        LocalStateSessionDevices devices;auto services=devices.Make(client.LocalStateGlobals(),true);services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.coreDataGlobals=client.CoreDataGlobals();services.adoptInitializedPlayers=true;
        auto request=std::make_shared<MissionFortLoadRequest>();const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const std::uint32_t port=6799;
        MissionFortRequestHooks requestHooks;requestHooks.selectFort=[request](auto name) { request->fort=name; };requestHooks.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };WriteMissionFortRequest("thewarbegins",{request->next,request->sequence,address,port},requestHooks);services.missionFortRequest=request;
        std::size_t packets{},initializations{};devices.observe=[&](auto,auto) { CHECK(initializations==1);++packets; };auto& camera=client.GameplayGlobals()->Camera();
        if (connected) services.localLogin->initializeRenderer=[&] {
            CHECK(request->consumed && request->zone.empty() && request->zone.capacity()==0 && client.LocalStateGlobals()->Blocks()[3][32]==0 && packets==0);++initializations;
            client.InitializeNativeRendererView();CHECK(camera.RendererView().cellPixels[0]==16 && camera.RendererView().cellPixels[1]==11 && camera.Snapshot().zoom==65536 && camera.Snapshot().pixels[0]==0 && camera.Snapshot().pixels[1]==0);
            CHECK(camera.Target().limits[0]==4096-kDefaultScreenWidth && camera.Target().limits[1]==2816-kDefaultScreenHeight && camera.Target().pixels[0]==0 && camera.Target().pixels[1]==0 && camera.Target().zoom==65536);CHECK(client.KencloudGlobals()->WorldRedraw()==1 && client.GameplayGlobals()->MapRevision()==0 && camera.Denominator().current==37);
        };
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),request->fort,std::move(services));
        client.GameplayGlobals()->WorldCells()=256;camera.RendererView().cellPixels={7,13};camera.RendererView().worldBits={0x3f800000,0x40000000};camera.CopyRendererSnapshot();camera.RequestPixels(73,91);camera.Denominator().current=37;client.GameplayGlobals()->MapRevision()=0xffffffffU;client.KencloudGlobals()->WorldRedraw()=17;
        bool rejected{};try { session.LoadLocalConnection(); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find("실제 미션 로그인 장치 미연결")!=std::string::npos; }
        if (connected) CHECK(!rejected && initializations==1 && packets>0 && session.LocalConnectionPhase()==14 && session.PlayerKnown());
        else CHECK(rejected && initializations==0 && packets==0 && request->consumed && session.LocalConnectionPhase()==0 && camera.RendererView().cellPixels[0]==7 && camera.Target().pixels[0]==73 && client.GameplayGlobals()->MapRevision()==0xffffffffU);
    }
}
