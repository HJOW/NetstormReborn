// 실제Client seed소유권과선택CoreRefresh누락만검사하며전체Session직렬화성공으로표현하지않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawFortCoreWrite.h"
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace netstorm::client;
namespace {
// Run없이실제Client의장기소유자만구성하며창/옵션저장/게임을시작하지않는다.
ClientOptions CoreSeedClientOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options; }
// 실제Template헤더/stream의지정DWORD를관측하며부모기대를생성하지않는다.
std::uint32_t CoreSeedDword(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{}; //비정렬필드의현재전체비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; //하위BYTE부터원래폭만읽는다.
}
}

// 실제Client의seed는한DWORD이고CRT/Game/IDinit/설정변경으로초기화되거나복제되지않는다.
TEST_CASE(ClientFortCoreSeed_ActualClientOneDwordAndIndependentGlobals) {
    Client client(CoreSeedClientOptions());const auto owner=client.RandomGlobals();const auto& constant=std::as_const(*owner); //같은실제장기소유자의읽기/쓰기뷰다.
    CHECK(owner && owner->FortCoreSeed()==0 && &owner->FortCoreSeed()==&constant.FortCoreSeed());
    auto* const address=&owner->FortCoreSeed();owner->FortCoreSeed()=0xabcd1234U;
    static_cast<void>(owner->Game().Next(32));static_cast<void>(owner->Crt().Next());owner->GameMode()=2;owner->SubscriberIdInitialized()=9;
    client.Configuration().SetInt("readOnlyForts",1);
    CHECK(&client.RandomGlobals()->FortCoreSeed()==address && constant.FortCoreSeed()==0xabcd1234U);
    CHECK(&owner->FortCoreSeed()!=&owner->GameMode() && &owner->FortCoreSeed()!=&owner->CrtState() && &owner->FortCoreSeed()!=&owner->SubscriberIdInitialized());
}

// 실제서비스두개가Client반납뒤까지같은owner/seed주소를보유한다.실제Session설치실행과구별한다.
TEST_CASE(ClientFortCoreSeed_ServiceOwnershipAndFreshClient) {
    RawCampaignSessionServices first,second;std::weak_ptr<ClientRandomGlobals> weak;std::uint32_t* address{}; //Session서비스의실제필드만사용하며새seed장부는만들지않는다.
    { Client client(CoreSeedClientOptions());first.randomGlobals=second.randomGlobals=client.RandomGlobals();weak=first.randomGlobals;address=&first.randomGlobals->FortCoreSeed();*address=0x89abcdefU; }
    CHECK(!weak.expired() && &second.randomGlobals->FortCoreSeed()==address && second.randomGlobals->FortCoreSeed()==0x89abcdefU);
    first.randomGlobals.reset();CHECK(!weak.expired() && second.randomGlobals->FortCoreSeed()==0x89abcdefU);
    { Client fresh(CoreSeedClientOptions());CHECK(fresh.RandomGlobals().get()!=second.randomGlobals.get() && fresh.RandomGlobals()->FortCoreSeed()==0 && second.randomGlobals->FortCoreSeed()==0x89abcdefU); } //새Client만최초0이며이전서비스소유자는보존한다.
    second.randomGlobals.reset();CHECK(weak.expired()); //최종해제뒤저장해둔seed주소를역참조하지않는다.
}

// 실제Core부모의refresh누락은Create뒤에서중단하고같은seed조회/SP/일반쓰기를앞당기지않는다.
TEST_CASE(ClientFortCoreSeed_MissingCoreRefreshPreservesSeedAndCreatedSection) {
    using namespace netstorm::o;
    Client client(CoreSeedClientOptions());const auto owner=client.RandomGlobals();owner->FortCoreSeed()=0xabcd0000U;
    CampaignTemplateStorage storage;storage.Install(1,FortTemplate{},"core-seed-boundary");std::array<std::uint8_t,1620> players{}; //실제Storage와명시Player물리가부모보다오래살아있다.
    std::uint32_t seedReads{},writes{},spCalls{};std::optional<TemplateWriteStream> stream; //선택경계의부분효과만관측한다.
    FortCoreWriteGlobals globals;globals.metamap=[] { return 0U; };globals.missionPost=[] { return 0U; };globals.editor=[] { return 0U; }; //정상Editor자연ret와다른명시비Editor선택입력이다.
    globals.players=[&] { return std::span<std::uint8_t>(players); };globals.playerOwner=[] { return 1U; };globals.seed=[&]()->std::uint32_t& { ++seedReads;return owner->FortCoreSeed(); };
    FortCoreWriteHooks hooks;hooks.manager=[&](auto slot) { CHECK(slot==1);return CoreSeedDword(storage.Headers(),20); };
    hooks.create=[&](auto manager,auto name) { CHECK(manager==CoreSeedDword(storage.Headers(),20) && name=="CoreData");stream=storage.CreateSection(1,name);return 1U; };
    hooks.write=[&](auto,auto) { ++writes; };hooks.sp=[&](auto,float) { ++spCalls; }; //앞당긴호출은실패관측으로드러나며제품에는빈성공을연결하지않는다.
    bool rejected{};try { WriteFortCoreData(1,globals,hooks); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && stream && seedReads==0 && writes==0 && spCalls==0 && owner->FortCoreSeed()==0xabcd0000U);
    CHECK(storage.SectionOrder(1)==std::vector<std::string>{"CoreData"});
    if (stream) storage.AccessSection(*stream,[&](auto fields,const auto&) { CHECK(CoreSeedDword(fields,4)==0 && CoreSeedDword(fields,8)==0); }); //실제동기필드만읽고span을밖으로반환하지않는다.
}
