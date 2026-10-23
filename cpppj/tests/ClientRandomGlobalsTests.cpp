// Client의 실제 CRT/Subscriber 초기화 소유자와 Session의 Seed·지형 alias를 검사하는 적용후용 제안이다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/RawCampaignSession.h"
#include "o/RawSubscriberIdGenerate.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace netstorm::client;
namespace {
// Run 없이 실제 Client의 소유권만 생성하므로 창/화면/음향 장치의 초기화는 실행하지 않는다.
ClientOptions RandomClientOptions() {
    ClientOptions options; // 실제 보호 archive는 기존 Client 생성자처럼 읽기만 한다.
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options;
}
// 두 Session에 같은 실제 Client 소유자를 주며 소리/번역/표시만 명시 장치로 공급한다.
struct RandomSessionDevices {
    std::shared_ptr<ClientRandomGlobals> globals; // Session 생성보다 먼저 빌려 모든 callback보다 오래 유지한다.
    SoundList names{OriginalEdition::Patch1078}; // 같은 장치가 빌리는 실제 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 설치의 명시 번역 입력이다.
    std::int32_t seconds{946684800}; // 원문SP초기화의 실제time32경계를 두 생성자에서 서로 다르게 공급한다.
    std::vector<std::uint32_t> beforeSeed; // srand 전 callback에서 본 같은 CRT의 현재값을 관측한다.
    RawCampaignSessionServices Make(bool supplyOwner=true) { // Session의 실제 Seed/지형/파편 몸체와 난수 상태는 대체하지 않는다.
        names.Initialize();RawCampaignSessionServices result; // 미선택 게임 경계는 기존 Session의 명시 오류를 유지한다.
        if (supplyOwner) result.randomGlobals=globals;result.soundNames=&names;
        result.unixSeconds=[this] { beforeSeed.push_back(globals->CrtState());return seconds; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.production.effects.configInt=[](std::string_view key) { if (key!="getTeleportTechFree") throw std::logic_error("공유CRT 검사의 생산 설정");return 0; };
        result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("공유CRT 검사의 없는 UI 처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("공유CRT 검사의 미선택 삭제효과"); };
        result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
}

// DWORD소비자와CrtRandom wrapper가 복사/동기화 없이 같은 상태를 양방향으로 변경한다.
TEST_CASE(ClientRandomGlobals_OnePhysicalStateAndNativeCrtStep) {
    static_assert(!std::is_copy_constructible_v<ClientRandomGlobals> && !std::is_copy_assignable_v<ClientRandomGlobals>); // Borrow가 빌린 주소를 복제로 나누지 않는다.
    static_assert(!std::is_move_constructible_v<ClientRandomGlobals> && !std::is_move_assignable_v<ClientRandomGlobals>); // 내부 포인터가 이전 객체를 가리키는 이동을 금지한다.
    ClientRandomGlobals owner;const auto& constant=owner; // 같은 실제 소유자의 읽기/쓰기 view다.
    CHECK(owner.CrtState()==1 && owner.Crt().State()==1 && owner.SubscriberIdInitialized()==0);
    CHECK(&owner.CrtState()==&constant.CrtState() && &owner.Crt()==&constant.Crt() && &owner.SubscriberIdInitialized()==&constant.SubscriberIdInitialized());
    CHECK(owner.Crt().Next()==41 && owner.CrtState()==2745024U); // 기존 독립원문 trace의holdrand1 첫 갱신과004e64ab 상위15비트다.
    owner.CrtState()=0;CHECK(owner.Crt().State()==0 && owner.Crt().Next()==38 && owner.CrtState()==2531011U); // 0도유효하며GameRandom의0대체규칙을추가하지않는다.
    owner.Crt().SetState(0x89abcdefU);CHECK(owner.CrtState()==0x89abcdefU);
    owner.SubscriberIdInitialized()=0x80000002U;CHECK(constant.SubscriberIdInitialized()==0x80000002U && owner.CrtState()==0x89abcdefU); // ID초기화DWORD는별도전체비트를보존한다.
}

// 기존native fixture의 두연속호출 기대와 같은 원문몸체를 같은Client소유자에 연결한다.
TEST_CASE(ClientRandomGlobals_SubscriberNativeTwoCallsWithoutReinitialization) {
    ClientRandomGlobals owner;std::uint32_t timeCalls{},clockCalls{};SubscriberIdGenerateHooks devices; // 최초CRT1/init0의 실제 새소유자 입력이다.
    devices.time32=[&] { ++timeCalls;return 0; };devices.clock=[&] { ++clockCalls;return 0U; };
    devices.expired=[](std::int32_t,std::string_view) { throw std::logic_error("공유CRT native입력의예상밖소진"); };
    auto hooks=BindSubscriberIdCrtRandom(owner.Crt(),std::move(devices)); // 같은 장기wrapper를 직접 빌리며 임시Borrow를 만들지 않는다.
    CHECK(GenerateSubscriberId({owner.SubscriberIdInitialized()},hooks)==128150324U);
    CHECK(owner.SubscriberIdInitialized()==1 && owner.CrtState()==4058565956U && timeCalls==1 && clockCalls==1);
    hooks.time32=[]()->std::int32_t { throw std::logic_error("두번째Subscriber에서time32재초기화"); };hooks.clock=[]()->std::uint32_t { throw std::logic_error("두번째Subscriber에서clock재초기화"); };
    CHECK(GenerateSubscriberId({owner.SubscriberIdInitialized()},hooks)==87100601U);
    CHECK(owner.SubscriberIdInitialized()==1 && owner.CrtState()==2294671304U && owner.Crt().State()==2294671304U); // subscriber-id-generate-x86의0,0,1,0,0,0,2,none 양정밀도기대다.
}

// 실제 Client getter가 한소유자를 유지하고 Config/외부반납이 상태를 초기화하지 않으며 새Client만1/0부터 시작한다.
TEST_CASE(ClientRandomGlobals_ActualClientOwnershipAndFreshClient) {
    static_assert(std::is_same_v<decltype(std::declval<const Client&>().RandomGlobals()),const std::shared_ptr<ClientRandomGlobals>&>); // 실제 소유자슬롯조회이며임시새객체/테스트전용값이아니다.
    std::shared_ptr<ClientRandomGlobals> previous;std::weak_ptr<ClientRandomGlobals> lifetime; // 앞Client 종료뒤까지 실제소유자와Borrow수명을 구별한다.
    {
        Client client(RandomClientOptions());previous=client.RandomGlobals();lifetime=previous; // Run 없는 실제 Client 생성자다.
        const auto* slot=&client.RandomGlobals(); // 같은Client소유슬롯주소를Config변경뒤에도비교한다.
        CHECK(previous && previous->CrtState()==1 && previous->SubscriberIdInitialized()==0);
        auto consumer=client.RandomGlobals(); // 다른 Session과 같은 공유소비자에 해당한다.
        CHECK(consumer.get()==previous.get() && &consumer->Crt()==&previous->Crt() && &consumer->CrtState()==&previous->CrtState());
        previous->Crt().SetState(0x76543210U);previous->SubscriberIdInitialized()=7;
        client.Configuration().Configuration().AppendSection("END");client.Configuration().SetInt("readOnlyForts",1);
        consumer.reset();CHECK(&client.RandomGlobals()==slot && client.RandomGlobals().get()==previous.get() && previous->CrtState()==0x76543210U && previous->SubscriberIdInitialized()==7);
    }
    CHECK(!lifetime.expired() && previous->Crt().State()==0x76543210U && previous->SubscriberIdInitialized()==7);
    {
        Client fresh(RandomClientOptions());CHECK(fresh.RandomGlobals().get()!=previous.get()); // 이전객체를유지해주소재사용과구별한다.
        CHECK(fresh.RandomGlobals()->CrtState()==1 && fresh.RandomGlobals()->SubscriberIdInitialized()==0);
        CHECK(previous->CrtState()==0x76543210U && previous->SubscriberIdInitialized()==7);
    }
    previous.reset();CHECK(lifetime.expired()); // 최종참조반납뒤에는실제DWORD/wrapper를조회하지않는다.
}

// 두실제Session의원문SP재시드와지형CRT소비가같은Client상태에직접도달하며IDinit은Session전환으로리셋되지않는다.
TEST_CASE(ClientRandomGlobals_TwoActualSessionsShareSeedAndTerrain) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 보호자료는읽기만하며기대난수에제품상태를사용하지않는다.
    Client client(RandomClientOptions());RandomSessionDevices firstDevices,secondDevices; // 각Session의소리표/콜백도각사용자보다오래유지한다.
    auto globals=client.RandomGlobals();firstDevices.globals=secondDevices.globals=globals;secondDevices.seconds=1; // 두장치는같은실제Client상태만공유한다.
    globals->CrtState()=0xfedcba98U;globals->SubscriberIdInitialized()=17; // 최초소유뒤다른소비자가남긴현재상태를명시입력한다.
    {
        RawCampaignSession first(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"shared-crt-first",firstDevices.Make()); // SP.Initialize가원래호출하는srand는생략하지않는다.
        CHECK(firstDevices.beforeSeed==std::vector<std::uint32_t>{0xfedcba98U} && globals->CrtState()==943813385U); // campaign-seed-x86의Reseed/time946684800기대다.
        globals->Crt().SetState(0x89abcdefU);
        {
            RawCampaignSession second(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"shared-crt-second",secondDevices.Make()); // 새Session의바인딩이현재owner를1/0으로재설정하지않아야한다.
            CHECK(secondDevices.beforeSeed==std::vector<std::uint32_t>{0x89abcdefU} && globals->CrtState()==304070270U); // 같은native fixture의Reseed/time1기대다.
            CHECK(globals->SubscriberIdInitialized()==17);
            globals->CrtState()=0x12345678U;first.Load(); // 첫Session의같은지형99cache가현재공유DWORD를소비하는지확인한다.
            CHECK(globals->CrtState()!=0x12345678U && globals->SubscriberIdInitialized()==17);
            globals->CrtState()=0x87654321U;second.Load(); // 두번째Session도첫Session뒤의현재같은상태를소비한다.
            CHECK(globals->CrtState()!=0x87654321U && globals->SubscriberIdInitialized()==17);
        }
        CHECK(globals->SubscriberIdInitialized()==17 && client.RandomGlobals().get()==globals.get());
    }
    CHECK(globals->SubscriberIdInitialized()==17 && globals->Crt().State()==globals->CrtState());
    CHECK(assets.files.Read("d/thewarbegins.fort")==original); // 전체원본1394manifest검사와구별하는선택입력재읽기다.
}

// provider를생략한기존독립Session은자체CRT0에서시작하고다른Client의스트림/IDinit을건드리지않는다.
TEST_CASE(ClientRandomGlobals_IndependentSessionFallbackStaysSeparate) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 같은보호입력으로fallback만선택한다.
    Client client(RandomClientOptions());RandomSessionDevices devices;devices.globals=client.RandomGlobals(); // 미주입Client소유자의불변을관측한다.
    devices.globals->CrtState()=0x13579bdfU;devices.globals->SubscriberIdInitialized()=19;
    {
        RawCampaignSession independent(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"independent-crt",devices.Make(false)); // randomGlobals필드의생략을명시한다.
        CHECK(devices.beforeSeed==std::vector<std::uint32_t>{0x13579bdfU} && devices.globals->CrtState()==0x13579bdfU);
        independent.Load();CHECK(devices.globals->CrtState()==0x13579bdfU && devices.globals->SubscriberIdInitialized()==19);
    }
    CHECK(devices.globals->CrtState()==0x13579bdfU && devices.globals->SubscriberIdInitialized()==19);
}
