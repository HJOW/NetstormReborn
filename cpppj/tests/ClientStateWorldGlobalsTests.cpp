// 실제 Client·표면·UI를 사용해 Native State 직접 전역의 값 사본 없는 연결과 폭을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientStateWorldGlobals.h"
#include "client/ClientMain.h"
#include "client/BitmapFont.h"
#include "client/CampaignTemplateStorage.h"
#include "client/ClientAudio.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientRandomGlobals.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <vector>

namespace { // 원본 파일은 읽기만 하고 창·장치를 만들지 않는 실제 Client 장면이다.
using namespace netstorm::client;
using namespace netstorm::o;
// 같은 보호 자산 경로의 비활성 음향 Client를 준비한다.
ClientOptions CleanupOptions() { ClientOptions result;result.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";result.noAudio=true;return result; }
// 실패하는 선택 경계의 예외만 관측한다. 원문 반환·효과를 대체하지 않는다.
template<class Function> bool CleanupThrows(Function&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
// 실제 DWORD 저장소와 Native 주소의 독립 대응 입력이다.
struct CleanupWord { std::uint32_t address;std::uint32_t* actual; }; // actual은 새 map 값이 아니라 현재 owner 필드의 주소다.
// 같은 Client/표면/Template/공통 Quick Help를 실제로 구성한다.
struct CleanupScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 기존 보호 타입·SHP 자료다.
    Client client{CleanupOptions()}; // 모든 대여보다 먼저 만들고 마지막에 반납한다.
    std::shared_ptr<RawSurfaceStorage> surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux()); // 실제 동일 권한·보조 owner를 pin한다.
    FontStore fonts{assets.files,"Arial"};RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 1.0; }}}; // 공통 UI/글꼴이며 화면을 열지 않는다.
    CampaignTemplateStorage templates;RawMenuControls* menus{};StateWorldCleanupGlobals globals; // 실제9헤더와 같은 팝업 명령을 빌린다.
    // 선택 외부 음향/입력만 관측하고 게임 계산은 실제 body에 맡긴다.
    CleanupScene() {
        CHECK(!client.SurfaceStorageIfInitialized() && CleanupThrows([&] { client.SurfaceStorage(); })); // 기존 필수 조회의 예외 계약과 새 순수 조회의 NULL을 구별한다.
        CHECK(CleanupThrows([&] { client.WorldAudio(); }));client.InitializeNativeAudio(); // 실제 Client의 기존 noAudio 초기화 본체이며 외부 음향 사본을 만들지 않는다.
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};hooks.gameSeconds=[] { return 0.0; };
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));globals=BindClientStateWorldGlobals({client,*surface,templates,*menus,client.WorldAudio()});
    }
    // Native 주소와 별도로 실제 필드를 직접 읽고 써 getter/writer가 함께 잘못된 필드를 고른 경우도 검출한다.
    std::vector<CleanupWord> Words() {
        auto& local=*client.LocalStateGlobals();auto& game=*client.GameplayGlobals();auto& aux=*game.GameAux();
        return {{0x5424bc,&aux.RefundPercent()},{0x542534,&local.CachedPeerIndex()},{0x542538,&local.CachedPeerValue()},{0x54253c,&local.CachedPeerFieldA4()},{0x542540,&local.CachedPeerFieldA8()},{0x542544,&local.ScriptBreak()},
            {0x5467b8,&client.NsInfoGlobals()->zone},{0x5467bc,&aux.SubscriberMarker()},{0x54c8a0,&surface->chunks.Mode()},{0x54db14,&game.FullDeck()},{0x55a420,&local.SourceType()},{0x5949d4,&game.MapRevision()},
            {0x595090,&aux.SavedGeyserCount()},{0x5950ac,&aux.OrdinaryGeysers()},{0x5950b0,&aux.GeyserFrameFlag()},{0x595344,&game.player.playerKnown},{0x59a8ac,&game.HideBuildings()},{0x59a8b0,&client.KencloudGlobals()->WorldRedraw()},
            {0x5c7a74,&local.Begin()},{0x5c85a0,&local.CentralCreated()},{0x5c85a4,&game.world.editor},{0x5c85b4,&local.ConnectionNotified()},{0x5c85c0,&local.ZonePending()},{0x5c85c8,&local.MetadataMode()},{0x5c85cc,&local.StateAuxCC()},{0x5c85e0,&local.AbortPending()},{0x5c8bb0,&aux.Archipelago()},{0x5ca938,&menus->PopupState().command}};
    }
};
}

// DWORD 전체 비트와 이웃 필드 불변을 실제 owner 주소에 대조한다.
TEST_CASE(ClientStateWorldGlobals_AllDwordOwnersAndIndependentCurrentReads) {
    CleanupScene scene;auto words=scene.Words();CHECK(words.size()==28);
    // 서로 다른 초기 흔적이 주소 교환·여러 필드 일괄0을 드러낸다.
    for (std::size_t index=0;index<words.size();++index) *words[index].actual=0x80000000U+static_cast<std::uint32_t>(index);
    for (std::size_t index=0;index<words.size();++index) { // 각 쓰기는 해당 필드 한 DWORD만 바꾼다.
        std::vector<std::uint32_t> before;for (const auto& word:words) before.push_back(*word.actual); // 같은 실제 필드의 이전값이다.
        CHECK(scene.globals.read32(words[index].address)==before[index]);scene.globals.write32(words[index].address,0xffffffffU);
        for (std::size_t other=0;other<words.size();++other) CHECK(*words[other].actual==(other==index ? 0xffffffffU : before[other])); // 인접 owner를 합치지 않는다.
        *words[index].actual=0x12340000U+static_cast<std::uint32_t>(index);CHECK(scene.globals.read32(words[index].address)==*words[index].actual);
    }
}

// bool은 정상0/1만 저장하고 signed·float는 Native DWORD의 모든 비트를 보존한다.
TEST_CASE(ClientStateWorldGlobals_BoolContractSignedAndFloatBits) {
    CleanupScene scene;auto& game=*scene.client.GameplayGlobals();auto& aux=*game.GameAux();auto& mode=scene.surface->ownerMode;
    const std::array<std::pair<std::uint32_t,bool*>,8> fields{{{0x540bc0,&game.Server()},{0x540bc4,&game.Boss()},{0x594fa4,&game.world.missionPost},{0x594fb8,&mode.fort},{0x594fbc,&mode.battle},{0x594fc8,&mode.challenge},{0x5c85d4,&scene.client.LocalStateGlobals()->SavedGame()},{0x5ca8f0,&aux.PriestQuarterHp()}}}; // 실제 현재 bool 여덟 주소다.
    for (const auto& [address,actual]:fields) { // 비정규 DWORD를 조용히 true로 줄이지 않는다.
        scene.globals.write32(address,0);CHECK(!*actual && scene.globals.read32(address)==0);scene.globals.write32(address,1);CHECK(*actual && scene.globals.read32(address)==1);CHECK(CleanupThrows([&] { scene.globals.write32(address,2); }) && *actual);
    }
    for (const auto value:{0U,0x80000000U,0xffffffffU}) { // signed 자원/비용/포획과 float 좌표는 폭을 줄이지 않는다.
        scene.globals.write32(0x595088,value);scene.globals.write32(0x59508c,value);scene.globals.write32(0x5453e4,value);scene.globals.write32(0x5c848c,value);scene.globals.write32(0x5c8490,value);
        CHECK(aux.ResourcePoolBits()==value && std::bit_cast<std::uint32_t>(aux.TotalCost())==value && std::bit_cast<std::uint32_t>(aux.AllowAnyCapture())==value && std::bit_cast<std::uint32_t>(aux.LostX())==value && std::bit_cast<std::uint32_t>(aux.LostY())==value);
    }
}

// 프레임22칸과 바로 뒤 두 Zone DWORD·벽시각22칸은 서로 독립이다.
TEST_CASE(ClientStateWorldGlobals_FrameTableLowWordAndWideStoresPreserveNeighbors) {
    CleanupScene scene;auto& activity=*scene.client.ConnectionActivityGlobals();activity.ActiveStamp()=0x12345678;activity.ActiveZoneWord()=0x87654321;activity.WallSeconds().fill(0xabcddcba);
    for (std::uint32_t index=0;index<22;++index) { // 실제22주소 모두 현재 같은 표를 읽고 쓴다.
        activity.Frames()[index]=index+1;CHECK(scene.globals.read32(0x5c84e8+index*4)==index+1);scene.globals.write32(0x5c84e8+index*4,0xffffffffU);CHECK(activity.Frames()[index]==0xffffffffU);
    }
    CHECK(activity.ActiveStamp()==0x12345678 && activity.ActiveZoneWord()==0x87654321 && std::all_of(activity.WallSeconds().begin(),activity.WallSeconds().end(),[](auto value) { return value==0xabcddcba; }));
    auto& seed=scene.client.RandomGlobals()->FortCoreSeed();seed=0xabcd7654;scene.globals.write16(0x5c89b4,0x1234);CHECK(seed==0xabcd1234);scene.globals.write16(0x5c89b4,0);CHECK(seed==0xabcd0000);
    auto& local=*scene.client.LocalStateGlobals();auto& aux=*scene.client.GameplayGlobals()->GameAux();const std::array<std::pair<std::uint32_t,double*>,8> wide{{{0x5950b8,&aux.GeyserDeadline()},{0x5950c0,&aux.CleanupDeadline()},{0x5950c8,&aux.SecondaryGeyserDeadline()},{0x595358,&aux.ScheduledDrawDeadline()},{0x5c85f0,&local.LoginEnd()},{0x5c85f8,&local.ConnectEnd()},{0x5c8610,&local.ReconnectSaveEnd()},{0x5c8618,&local.CaptureEnd()}}}; // 음악 외 실제8기한이다.
    for (const auto& [address,actual]:wide) { scene.globals.write64Bits(address,0x8000000000000000ULL);CHECK(std::bit_cast<std::uint64_t>(*actual)==0x8000000000000000ULL);scene.globals.write64Bits(address,0);CHECK(std::bit_cast<std::uint64_t>(*actual)==0); } // -0과+0을 구별한다.
    const auto* audio=&scene.client.WorldAudio();CHECK(CleanupThrows([&] { scene.client.InitializeNativeAudio(); }) && &scene.client.WorldAudio()==audio); // 재초기화로 대여 주소를 교체하지 않는다.
    scene.globals.write64Bits(0x565e68,0x7ff8000012345678ULL);CHECK(std::bit_cast<std::uint64_t>(scene.client.WorldAudio().Scene().songEnd)==0x7ff8000012345678ULL);
}

// 현재 Player base·TYPE·Template manager를 캡처 사본으로 바꾸지 않는다.
TEST_CASE(ClientStateWorldGlobals_CurrentPlayerTypeTemplateAndUnknownAddresses) {
    CleanupScene scene;InitializeClientPlayerRecords(scene.client);const auto base=scene.client.PlayerRecords()->Base();CHECK(scene.globals.players().data()==scene.client.PlayerRecords()->Records().data());InitializeClientPlayerRecords(scene.client);CHECK(base!=scene.client.PlayerRecords()->Base() && scene.globals.players().data()==scene.client.PlayerRecords()->Records().data());
    auto headers=scene.templates.Headers();headers[20]=0x78;headers[21]=0x56;headers[22]=0x34;headers[23]=0x12;CHECK(scene.globals.templateManager(1)==0x12345678);std::fill(headers.begin()+20,headers.begin()+24,std::uint8_t{}); // 실제 manager를 만들지 않은 독립 헤더 입력은 소멸 전에 복원한다.
    auto& cost=scene.globals.typeDefault(122);CHECK(&cost==&scene.surface->types[122].cost);cost=123.5F;CHECK(scene.surface->types[122].cost==123.5F);
    for (const auto address:{0x5c84e9U,0x5c8540U,0x5c8544U,0x5c8548U,0xdeadbeefU}) { CHECK(CleanupThrows([&] { scene.globals.read32(address); }) && CleanupThrows([&] { scene.globals.write32(address,0); })); } // 간극·다른 표와 미등록주소는0으로 가장하지 않는다.
    CHECK(CleanupThrows([&] { scene.globals.write16(0x5c89b6,0); }) && CleanupThrows([&] { scene.globals.write64Bits(0x5c8614,0); }) && CleanupThrows([&] { scene.globals.templateManager(9); }) && CleanupThrows([&] { scene.globals.typeDefault(188); }));
}

// 원문 부모의 직접 쓰기 전체를 실제 owner에 연결한다. Effect/SP/AI 하위는 여기서 호출 관측 경계다.
TEST_CASE(ClientStateWorldGlobals_NativeParentUsesActualStoresAndKeepsBoundaryOrder) {
    CleanupScene scene;InitializeClientPlayerRecords(scene.client);auto& game=*scene.client.GameplayGlobals();game.player.playerKnown=0;game.player.templateOwner=1;auto& aux=*game.GameAux();auto& local=*scene.client.LocalStateGlobals();local.State().phase[0]=33;aux.PriestQuarterHp()=true;game.HideBuildings()=7;scene.menus->PopupState().command=0xabcdef;scene.menus->PopupState().argument.fill('z');scene.client.RandomGlobals()->FortCoreSeed()=0xabcdefff;
    std::vector<StateWorldCleanupEffect> effects;std::vector<std::uint32_t> disconnected;std::size_t resetAi{},resetPriest{},other{},pools{};StateWorldCleanupHooks hooks;hooks.effect=[&](auto value) { effects.push_back(value); };hooks.disconnect=[&](auto value) { disconnected.push_back(value); };hooks.pauseDepth=[] { return 0U; };hooks.resetAi=[&](auto) { ++resetAi; };hooks.resetPriest=[&](auto) { ++resetPriest; };hooks.resetOtherPlayer=[&](auto,std::uint32_t value) { CHECK(value==0);++other; };hooks.setSp=[](auto,float) {};hooks.selectCore=[](auto,auto) {};hooks.resetPool=[&](auto mode) { CHECK(mode==0);++pools; };hooks.wallSeconds=[] { return 123.5; };
    CleanupStateWorld(0,scene.globals,hooks);CHECK(disconnected==std::vector<std::uint32_t>({10,11}) && resetAi==8 && resetPriest==8 && other==8 && pools==1);CHECK(effects.front()==StateWorldCleanupEffect::CloseAllConnections && effects.back()==StateWorldCleanupEffect::MapMode);
    CHECK(!aux.PriestQuarterHp() && game.HideBuildings()==0 && scene.menus->PopupState().command==0 && scene.menus->PopupState().argument[0]=='z' && scene.client.RandomGlobals()->FortCoreSeed()==0xabcd0000 && local.ReconnectSaveEnd()==133.5 && !game.Server() && !game.Boss());CHECK(scene.surface->types[122].cost==2000.F && game.FullDeck()==0 && local.CachedPeerIndex()==0xffffffffU && local.State().phase[0]==33);
    for (std::uint32_t owner=1;owner<9;++owner) for (std::uint32_t field=0x7c;field<=0x98;field+=4) { const auto bytes=scene.globals.players();CHECK(bytes[owner*180+field]==0 && bytes[owner*180+field+3]==0); } // 같은 현재 실제 Player의 지정 여덟 DWORD만 확인한다.
}

// 잘못된 실제 저장소·권한 소유자의 연결은 어떤 Native store보다 먼저 거부한다.
TEST_CASE(ClientStateWorldGlobals_RejectsForeignSurfaceBeforeEffects) {
    CleanupScene scene;auto foreign=MakeRawSurfaceStorage(scene.assets.game);scene.client.GameplayGlobals()->FullDeck()=0x81234567;CHECK(CleanupThrows([&] { BindClientStateWorldGlobals({scene.client,*foreign,scene.templates,*scene.menus,scene.client.WorldAudio()}); }) && scene.client.GameplayGlobals()->FullDeck()==0x81234567);
}

// 공장의 같은 주소 재연결은 허용하고 외부 bool로 설정 저장소를 바꾸는 시도는 거부한다.
TEST_CASE(ClientStateWorldGlobals_FactoryQuarterHpBindingKeepsActualOwner) {
    CleanupScene scene;SquidFactory factory(scene.surface->pool,scene.surface->types);auto& quarter=scene.client.GameplayGlobals()->GameAux()->PriestQuarterHp();factory.BindWeakenedMana(quarter);factory.BindWeakenedMana(quarter);bool foreign{};CHECK(CleanupThrows([&] { factory.BindWeakenedMana(foreign); }));
    quarter=true;const auto weak=factory.Create(158,2);const auto bytes=scene.surface->pool.AllocatedBytes(weak);CHECK(bytes[26]==25 && bytes[27]==0 && bytes[28]==0 && bytes[29]==0);quarter=false;const auto normal=factory.Create(158,2);const auto next=scene.surface->pool.AllocatedBytes(normal);CHECK(next[26]==100 && next[27]==0 && next[28]==0 && next[29]==0);
}
