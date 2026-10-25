#include "client/ClientGameplayGlobals.h" // 실제0054db14 DWORD의 동일 장기 owner를 빌린다.
// 원본Deck/Contains전체와기술표시상위관찰을실제Player/Factory/ContentHost/SP통지에대조한다.
#include "DudeSpawnSupport.h"
#include "o/RawProductionDeck.h"
#include "o/RawSpStoreNoticeBindings.h"
#include "o/ContentFormHost.h"
#include "client/RawCampaignProductionBindings.h"
#include <memory>
#include <optional>
#include <type_traits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본격리관찰기의opaque주소다.호스트포인터로해석하지않는다.
constexpr std::uint32_t kDeckToken=0x11100000;
// 선택되지않은생성/장치/삭제경계는빈성공으로넘기지않는다.
[[noreturn]] void ProductionMissing() { throw std::logic_error("생산검사에서선택하지않은실제하위경계"); }
// 원본count/미초기화바이트/모든물리항목을입력으로만준비한다.
std::array<std::uint8_t,kProductionDeckBytes> DeckInput(const std::vector<std::string>& values) {
    std::array<std::uint8_t,kProductionDeckBytes> raw;raw.fill(static_cast<std::uint8_t>(Number(values[5])));
    // 기대출력이아닌같은66개입력항목이다.활성개수밖꼬리도보존한다.
    for (std::size_t index=0;index<kProductionEntryCapacity;++index) {
        raw[index*5]=std::array<std::uint8_t,4>{0,71,158,255}[index%4];raw[index*5+1]=static_cast<std::uint8_t>(index%7+1);raw[index*5+2]=std::array<std::uint8_t,5>{255,0,1,128,127}[index%5];raw[index*5+3]=37;raw[index*5+4]=static_cast<std::uint8_t>(index);
    }
    Put(raw,0x14c,Number(values[4]));Put(raw,0x150,0x12345678);Put(raw,0x154,0x23456789);Put(raw,0x158,0x3456789a);Put(raw,0x15c,0x456789ab);return raw;
}
// 실제함수가보고한진단위치를원본Deck.cppline으로보존한다.
std::uint32_t AssertionLine(ProductionDeckAssertion where) {
    switch (where) { case ProductionDeckAssertion::DuplicateType:return 100;case ProductionDeckAssertion::Capacity:return 102;case ProductionDeckAssertion::TypeTruncated:return 45;case ProductionDeckAssertion::ChanceTruncated:return 46;case ProductionDeckAssertion::PowerTruncated:return 47;case ProductionDeckAssertion::RemainingTruncated:return 48; }
    throw std::logic_error("Deck진단위치누락");
}
// 저장된초기Deck의각바이트를입력그대로복원한다.생산결과모델은계산하지않는다.
std::array<std::uint8_t,kProductionDeckBytes> DeckBytes(std::string_view hex) {
    if (hex.size()!=kProductionDeckBytes*2) throw std::invalid_argument("생산Deck입력704hex길이오류");std::array<std::uint8_t,kProductionDeckBytes> bytes{};
    // 두16진글자는정확한원본한바이트다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index*2,2)),nullptr,16));
    return bytes;
}
// 같은 실제 표면 월드가 요구하는 원본 타입 전역을 공급한다. 삭제 경계를 비활성 값으로 우회하지 않는다.
BridgeConnectState ProductionLinks() {
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;links.battle=true;links.mission=true;return links;
}
// 동일보호188TYPE/Pool/Player9/Factory/ContentHost를보관하는검사월드다.
struct ProductionScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호자료를읽는실제VFS/TYPE다.
    netstorm::client::RawSurfaceWorld world{OriginalEdition::Patch1078,assets.game.TypeTable().Types(),assets.frames,assets.shapes,ProductionLinks(),97}; // 실제월드가단일Pool/TYPE/장부를소유한다.
    netstorm::client::RawWorldCore core{world.Core()}; // 같은월드의참조만빌린다.
    SidPool& pool=core.pool; // 모든실제생성자가공유한다.
    std::span<const RiftTypeRecord> types=core.types; // 보호TYPE의genus/생성자/발자국을수정하지않는다.
    SquidPostPopState& book=core.bookkeeping; // 같은실제공통장부다.
    SquidPostPop& post=core.postPop; // Player생성계약과같은공통몸체다.
    std::array<std::uint8_t,9*180> records{}; // 실제Player9원문이다.
    PlayerWorldState state; // 현재PlayerKnown과서로다른두owner다.
    std::unique_ptr<RawPlayerWorld> player; // 실제현재getter를제공한다.
    CarrierPreDestroyState carrierState; // 실제containedType/권한전역이다.
    RawCarrierPreDestroy carrier{pool,carrierState,{[](Sid,std::uint32_t) { ProductionMissing(); },[](Sid,std::uint32_t) { ProductionMissing(); },[] { ProductionMissing(); }}}; // 삭제효과는조회에서실행하면실패한다.
    SquidFactory& factory=core.factory; // 실제Player152/타입6생성자다.
    ContainedCreateState createState; // 같은실제타입6전역이다.
    GameRandom random; // 실제SP저장소의게임난수다.
    ScrambledSpStore sp{random,[] { return 1234U; }}; // 실제Player내용물통지접두가읽는유일한저장소다.
    std::int32_t tracking{}; // 같은SP추적전역의안정저장소다.
    std::uint32_t problem{},fallback{},moneyChecks{},gumpType{3},event{0x02000031}; // 현재실제통지전역이다.
    std::unique_ptr<RawPlayerContentNotice> notice; // 빈통지가아닌실제SP접두/종속순회다.
    std::unique_ptr<ContentFormHost> forms; // 같은원본정렬/부착/부모통지를수행한다.
    std::unique_ptr<RawContainedCreate> create; // 실제Factory와실제ContentHost의생성사슬이다.
    std::unique_ptr<RawProductionDeck> deck; // 실제Player+68의유일한소유블록이다.
    std::unique_ptr<RawIndicateTechnology> indicate; // 실제현재부모/기존종속/기술생성이다.
    std::vector<std::string> trace; // 실행한원본경계요청만관찰한다.
    // 같은원본입력과실제하위만준비한다.반환/선택결과는입력하지않는다.
    explicit ProductionScene(const std::vector<std::string>& values) {
        state.templateOwner=Number(values[1]);state.battleOwner=Number(values[8]);state.playerKnown=Number(values[7]);Put(records,state.battleOwner*180+0x9c,50);
        PlayerWorldHooks hooks;hooks.assertion=[](PlayerWorldAssertion) { ProductionMissing(); };hooks.find=[](float,float,std::uint32_t)->Sid { ProductionMissing(); };hooks.create=[](std::uint32_t,std::uint32_t)->Sid { ProductionMissing(); };hooks.setOwner=[](Sid,std::uint32_t) { ProductionMissing(); };hooks.pop=[](Sid,float,float,std::uint32_t) { ProductionMissing(); };hooks.reserveRegular=[]()->bool { ProductionMissing(); };hooks.addRegular=[](Sid,std::uint32_t,float) { ProductionMissing(); };
        player=std::make_unique<RawPlayerWorld>(pool,post,records,state,std::move(hooks));
        deck=std::make_unique<RawProductionDeck>(*player,[this,pattern=Number(values[5])](std::span<std::uint8_t> raw) { std::fill(raw.begin(),raw.end(),static_cast<std::uint8_t>(pattern));trace.push_back("N:352"); },[this](ProductionDeckAssertion where) { trace.push_back("A:"+std::to_string(AssertionLine(where))); },kDeckToken);
        const auto mode=values[0];
        if ((mode!="Get" && mode!="Contains") || Number(values[6])) { const auto token=deck->Get(state.templateOwner);CHECK(token==kDeckToken);const auto input=DeckInput(values);std::copy(input.begin(),input.end(),deck->Bytes(token).begin());if (!Number(values[6])) Put(records,state.templateOwner*180+0x68,0); }
        trace.clear();sp.BindTrackingOffset(tracking);sp.Initialize();const auto reads=BindSpStoreNotice(sp);
        PlayerContentNoticeHooks noticeHooks;noticeHooks.money=[this](std::uint32_t owner) { return std::bit_cast<float>(sp.Get(owner)); };noticeHooks.gump=[](std::uint32_t)->std::uint32_t { ProductionMissing(); };noticeHooks.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { ProductionMissing(); };
        notice=std::make_unique<RawPlayerContentNotice>(pool,PlayerContentNoticeState{reads.cursorA,reads.cursorB,problem,fallback,moneyChecks,state.playerKnown,state.battleOwner,reads.firstSpPool,reads.firstPermutation,gumpType,event},std::move(noticeHooks));
        forms=std::make_unique<ContentFormHost>(pool,types,[this](Sid parent,Sid form,std::uint32_t reason) { notice->Notify(parent,form.value,reason); });
        ContainedCreateHooks createHooks;createHooks.create=[this](std::uint32_t type,std::uint32_t flags) { CHECK(type==6);return factory.CreateContained(flags); };createHooks.notifySurface=[](Sid) { ProductionMissing(); };
        createHooks.attach=[this](Sid form,Sid parent,std::uint32_t flags) { const auto raw=pool.Slot(form);trace.push_back("C:"+std::to_string(parent.value)+','+std::to_string(Get(raw,18,2))+','+std::to_string(Get(raw,12,2))+','+std::to_string(Get(raw,20,2))+','+std::to_string(flags)+','+std::to_string(raw[30]));forms->Attach(form,parent,flags); };
        create=std::make_unique<RawContainedCreate>(pool,createState,std::move(createHooks));indicate=std::make_unique<RawIndicateTechnology>(*player,carrier,*create);
        if (mode=="Indicate") {
            // 실제할당/생성자로원본관찰의Technology SID50을준비한다.
            for (std::uint16_t sid=5;sid<50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
            CHECK(factory.Create(152,2)==Sid{50});
            // 원본입력의종속SID60도실제타입6생성자로확보한다.
            for (std::uint16_t sid=51;sid<60;++sid) CHECK(pool.Allocate(2)==Sid{sid});
            CHECK(factory.CreateContained(2)==Sid{60});auto raw=pool.AllocatedBytes(Sid{60});Put(raw,18,Number(values[4]),2);Put(raw,20,Number(values[5]),2);Put(raw,16,50,2);raw[11]=8;
            if (Number(values[6])) Put(pool.AllocatedBytes(Sid{50}),6,60,2);
        }
    }
    // 관찰요청순서를원본fixture와동일하게직렬화한다.
    std::string Events() const { std::string text;for (const auto& item:trace) { if (!text.empty()) text+=';';text+=item; }return text.empty() ? "-" : text; }
};
// native재계산도구와동일한실제Factory/내용물입력만준비한다.반환값이나포함여부는계산하지않는다.
void RefreshNodes(ProductionScene& scene,std::uint32_t variant,std::uint32_t level) {
    CHECK(scene.factory.Create(85,2)==Sid{5});CHECK(scene.factory.Create(124,2)==Sid{6});CHECK(scene.factory.Create(86,2)==Sid{7});
    CHECK(scene.pool.Allocate(2)==Sid{8});CHECK(scene.pool.Allocate(2)==Sid{9});
    // 모든종속은기존실제타입6생성자로먼저확보한다.
    for (std::uint16_t sid=10;sid<14;++sid) CHECK(scene.factory.CreateContained(2)==Sid{sid});
    const std::array<std::array<std::uint32_t,8>,7> nodes{{
        {{5,85,(level<<5)|std::array<std::uint32_t,4>{0,1,2,4}[variant%4],1,10,0,0,0}},
        {{6,124,0,variant%5==1 ? 2U : 1U,0,0,0,0}},{{7,86,0,variant%5==2 ? 2U : 1U,13,0,0,0}},
        {{10,6,8,0,0,11,71,0x10}},{{11,6,1,0,0,12,variant%4==2 ? 83U : 71U,variant%3 ? 0x10U : 1U}},
        {{12,6,2,0,0,0,158,0x10}},{{13,6,0,0,0,0,83,0x10}}
    }};
    // 실제VT/TYPE를보존하고원문관찰의현재owner/head/next/kind/mask/state만명시입력한다.
    for (const auto& input:nodes) { auto raw=scene.pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(input[0])});CHECK(raw[10]==input[1]);raw[34]=static_cast<std::uint8_t>(input[3]);Put(raw,6,input[4],2);Put(raw,4,input[5],2);if (input[1]==6) { Put(raw,18,input[6],2);Put(raw,20,input[7],2); }raw[11]=static_cast<std::uint8_t>(input[2]); }
    scene.book.factories.entries={5,6,7};scene.book.factories.count=3;
    CHECK((scene.types[85].flags1&0x04000000U)!=0 && (scene.types[85].flags2&0x4000U)!=0 && (scene.types[124].flags2&0x200U)!=0);
}
}

// 새248입력의전체Deck/실제현재Player/요청순서를독립원본관찰과대조한다.
TEST_CASE(ProductionDeck_ReplaysNative248) {
    const auto fixture=LoadFixture(NETSTORM_PRODUCTION_DECK_FIXTURE);CHECK(fixture.rows.size()==248);std::size_t checked=0;
    // 매행실제월드를새로만들어이전생산출력에의존하지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);const auto values=Split(row[0],',');CHECK(values.size()==9);ProductionScene scene(values);const auto& mode=values[0];const auto type=Number(values[2]),chance=Number(values[3]);std::string result="-";
        if (mode=="Get") result=std::to_string(scene.deck->Get(Number(values[1])));else if (mode=="Contains") result=scene.deck->Contains(type) ? "1" : "0";else if (mode=="Initialize") scene.deck->Initialize(kDeckToken);else if (mode=="Clear") scene.deck->ClearEntries(kDeckToken);else if (mode=="Append") scene.deck->Append(kDeckToken,type,std::bit_cast<std::int32_t>(chance));else if (mode=="Activate") scene.deck->Activate(kDeckToken,type,std::bit_cast<std::int32_t>(chance));else if (mode=="Compact") scene.deck->Compact(kDeckToken);else if (mode=="Indicate") {
            const bool rejected=Throws([&] { scene.indicate->Indicate(type); });CHECK(rejected==(type>=scene.types.size())); // 원본생성관찰경계뒤의호스트TYPE범위계약은명시예외다.
        } else CHECK(false);
        CHECK(result==row[1]);CHECK(Hex(scene.deck->Bytes(kDeckToken))==row[2]);CHECK(Hex(scene.records)==row[3]);CHECK(scene.Events()==row[4]);++checked;
    }
    CHECK(checked==248);
}
// 현재templateOwner와다른battleOwner/서로다른실제블록수명을보존한다.
TEST_CASE(ProductionDeck_CurrentPlayerAndOwnedLifetime) {
    const auto values=Split("Get,1,0,0,4,167,0,1,1",',');ProductionScene scene(values);const auto one=scene.deck->Get(1),eight=scene.deck->Get(8);scene.deck->Append(one,71,1);scene.deck->Append(eight,158,1);
    scene.state.templateOwner=1;CHECK(scene.deck->Contains(71) && !scene.deck->Contains(158));scene.state.templateOwner=8;CHECK(scene.deck->Contains(158) && !scene.deck->Contains(71));CHECK(Get(scene.records,180+0x68)==one && Get(scene.records,8*180+0x68)==eight);
    Put(scene.records,180+0x68,0x7fffffff);scene.deck.reset();CHECK(Get(scene.records,180+0x68)==0x7fffffff && Get(scene.records,8*180+0x68)==0);CHECK(scene.indicate->Pool().Edition()==OriginalEdition::Patch1078);
}
// 새242개원본재계산/Factorymode0출력을실제188TYPE/같은장부/허용표/내용물로대조한다.
TEST_CASE(ProductionDeck_ReplaysFactoryRefreshNative242) {
    const auto fixture=LoadFixture(NETSTORM_PRODUCTION_REFRESH_FIXTURE);CHECK(fixture.rows.size()==242);std::size_t checked=0;
    // 같은입력을실제월드/Factory/Player수명에매행새로설치한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);const auto values=Split(row[0],',');CHECK(values.size()==10);const auto owner=Number(values[1]),variant=Number(values[9]),level=Number(values[8]);
        ProductionScene scene(Split("Initialize,"+std::to_string(owner)+",0,0,4,167,1,1,"+std::to_string(owner),','));RefreshNodes(scene,variant,level);const auto input=DeckBytes(row[1]);std::copy(input.begin(),input.end(),scene.deck->Bytes(kDeckToken).begin());scene.core.ownerMode.fort=Number(values[3])!=0;scene.core.ownerMode.battle=Number(values[4])!=0;
        std::array<std::uint32_t,256> restrictions{};const auto blocked=Number(values[7]);restrictions[71]=blocked&1U;restrictions[133]=blocked&2U;
        RawTotalMade allowed(OriginalEdition::Patch1078,scene.types,scene.book,restrictions,[](TotalMadeAssertion) { ProductionMissing(); });bool mission=Number(values[2])!=0;std::uint32_t fullDeck=Number(values[5]),freeTechnology=153,templeProduct=133;
        netstorm::client::CampaignProductionEffects effects;effects.configInt=[&](std::string_view key) { CHECK(key=="getTeleportTechFree");scene.trace.push_back("Q:"+std::string(key));return static_cast<std::int32_t>(Number(values[6])); };effects.modeAssertion=[&] { scene.trace.push_back("A:331"); };
        netstorm::client::RawCampaignProductionBindings production(scene.core,*scene.player,scene.carrier,*scene.create,*scene.deck,allowed,scene.carrierState,{mission,fullDeck,freeTechnology,templeProduct},std::move(effects));const auto records=scene.records;const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());std::uint32_t result=0;
        if (values[0]=="Refresh") result=production.Refresh(owner);else { CHECK(values[0]=="Factory");result=std::bit_cast<std::uint32_t>(production.AppendFactoryProducts(Sid{5},kDeckToken)); }
        CHECK(result==Number(row[2]));CHECK(Hex(scene.deck->Bytes(kDeckToken))==row[3]);CHECK(scene.Events()==row[4]);CHECK(scene.records==records && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));CHECK(restrictions[71]==(blocked&1U) && restrictions[133]==(blocked&2U));CHECK(&production.Pool()==&scene.pool && &production.Deck()==scene.deck.get());++checked;
    }
    CHECK(checked==242);
}
// 표시소비자가같은현재허용표/Factory목록으로명시재계산해야하는원본시점을유지한다.
TEST_CASE(ProductionDeck_CurrentRestrictionsAndFactoriesAreShared) {
    ProductionScene scene(Split("Initialize,1,0,0,4,167,1,1,1",','));RefreshNodes(scene,0,0);scene.deck->ClearEntries(kDeckToken);scene.core.ownerMode.battle=true;
    std::array<std::uint32_t,256> restrictions{};RawTotalMade allowed(OriginalEdition::Patch1078,scene.types,scene.book,restrictions,[](TotalMadeAssertion) { ProductionMissing(); });bool mission=true;std::uint32_t full=0,freeTechnology=153,templeProduct=133;std::uint32_t config=0;
    netstorm::client::RawCampaignProductionBindings production(scene.core,*scene.player,scene.carrier,*scene.create,*scene.deck,allowed,scene.carrierState,{mission,full,freeTechnology,templeProduct},{[&](std::string_view key) { CHECK(key=="getTeleportTechFree");return static_cast<std::int32_t>(config); },[] { ProductionMissing(); }});
    production.RefreshCurrent();CHECK(production.Contains(71) && production.Contains(133) && !production.Contains(153));allowed.Allow(71,0);allowed.Allow(133,0);production.RefreshCurrent();
    CHECK(production.Contains(71) && !production.Allowed(71) && !production.Contains(133) && !production.Allowed(133)); // 일반Factory는허용조회후에도포함하고신전기본생산만금지조건으로제외한다.
    scene.book.factories.count=0;CHECK(production.Contains(71));production.RefreshCurrent();CHECK(!production.Contains(71));config=1;production.RefreshCurrent();CHECK(production.Contains(153));
    scene.state.templateOwner=8;CHECK(!production.Contains(153));production.RefreshCurrent();CHECK(production.Contains(153));
}
// 누락할당/모르는현재포인터/물리capacity를생산성공이나빈목록으로숨기지않는다.
TEST_CASE(ProductionDeck_RequiresRealStorageAndPhysicalBounds) {
    ProductionScene scene(Split("Initialize,1,0,0,4,167,1,1,1",','));CHECK(Throws([&] { RawProductionDeck missing(*scene.player,{}); }));
    CHECK(Throws([&] { scene.deck->Get(9); }));CHECK(Throws([&] { scene.deck->Bytes(0xffffffff); }));
    Put(scene.records,180+0x68,0x7fffffff);CHECK(Throws([&] { scene.deck->Contains(71); }));Put(scene.records,180+0x68,kDeckToken);
    auto raw=scene.deck->Bytes(kDeckToken);Put(raw,0x14c,66);const auto before=Hex(raw);CHECK(Throws([&] { scene.deck->Append(kDeckToken,83,1); }));CHECK(Hex(raw)==before);
    Put(raw,0x14c,67);CHECK(Throws([&] { scene.deck->Contains(71); }));
}

// 모드 진단 효과 뒤 현재 fullDeck를 다시 읽고 같은 장기 DWORD로 실제 재계산의 자연 return을 선택한다.
TEST_CASE(ProductionDeck_SharedFullDeckReadsAfterModeDiagnostic) {
    netstorm::client::ClientGameplayGlobals globals;ProductionScene scene(Split("Initialize,1,0,0,4,167,1,1,1",','));scene.deck->ClearEntries(kDeckToken);scene.deck->Append(kDeckToken,71,1);scene.deck->Bytes(kDeckToken)[2]=0x7e;scene.core.ownerMode.fort=scene.core.ownerMode.battle=false;
    std::array<std::uint32_t,256> restrictions{};RawTotalMade allowed(OriginalEdition::Patch1078,scene.types,scene.book,restrictions,[](TotalMadeAssertion) { ProductionMissing(); });bool mission=false;std::uint32_t freeTechnology=153,templeProduct=133,queries{},diagnostics{};netstorm::client::CampaignProductionEffects effects;
    effects.configInt=[&](auto) { ++queries;return 0; };effects.modeAssertion=[&] { ++diagnostics;globals.FullDeck()=0x80000000U; };netstorm::client::RawCampaignProductionBindings production(scene.core,*scene.player,scene.carrier,*scene.create,*scene.deck,allowed,scene.carrierState,{mission,globals.FullDeck(),freeTechnology,templeProduct},effects);const auto before=Hex(scene.deck->Bytes(kDeckToken));
    CHECK(production.RefreshCurrent()==kDeckToken && diagnostics==1 && queries==0 && globals.FullDeck()==0x80000000U && Hex(scene.deck->Bytes(kDeckToken))==before);
    scene.core.ownerMode.battle=true;globals.FullDeck()=0xffffffffU;production.RefreshCurrent();CHECK(queries==0 && Hex(scene.deck->Bytes(kDeckToken))==before && globals.FullDeck()==0xffffffffU);globals.FullDeck()=0;production.RefreshCurrent();CHECK(queries==1 && Get(scene.deck->Bytes(kDeckToken),0x14c)==0 && globals.FullDeck()==0);
}

// 실제9Player의현재getter와자기Deck free가끝난증빙만소비해oldPlayer의재조회책임을반납한다.
TEST_CASE(ProductionDeck_StateLinksProofUsesActualParentAndClosesConsumers) {
    static_assert(std::is_nothrow_move_constructible_v<RawProductionDeck::PlayerLinksPass> && !std::is_copy_constructible_v<RawProductionDeck::PlayerLinksPass>); // 실제 반납 뒤 optional 보관은 증빙의 noexcept 이동만 사용한다.
    ProductionScene scene(Split("Get,1,0,0,4,167,0,1,1",','));std::array<std::uint32_t,9> tokens{};
    for (std::uint32_t owner=0;owner<9;++owner) { tokens[owner]=scene.deck->Get(owner);Put(scene.records,owner*180+0x9c,owner+31);Put(scene.records,owner*180+8,owner+1); } // 원문이 방문하는9행의실제Deck만할당한다.
    const auto before=scene.records;std::size_t reads{};auto pass=scene.deck->CleanupPlayerLinksForState([&] { ++reads;return std::span<std::uint8_t>(scene.records); });CHECK(reads==36 && !scene.deck->PlayerLinksReturnedForState());
    for (std::size_t index=0;index<scene.records.size();++index) { const auto field=index%180;const bool zero=(field>=8 && field<12)||(field>=0x68 && field<0x6c)||(field>=0x9c && field<0xa0);CHECK(scene.records[index]==(zero ? 0 : before[index])); } // 세DWORD밖전체BYTE는실제부모의보존계약이다.
    scene.deck->ReturnPlayerLinksForState(std::move(pass));CHECK(scene.deck->PlayerLinksReturnedForState());const auto allocations=scene.trace.size();CHECK(Throws([&] { scene.deck->Get(1); }) && Throws([&] { scene.deck->Release(0); }) && Throws([&] { scene.deck->Bytes(tokens[1]); }) && scene.trace.size()==allocations);scene.deck.reset(); // 실제증빙소비뒤소멸자는oldPlayer를재조회하지않는다.
}

// 선택된잘못된Deck free의앞선필드쓰기만남으며정상복귀pass나반납완료를만들지않는다.
TEST_CASE(ProductionDeck_StateLinksFailurePreservesNativePrefixAndNeedsActualRecovery) {
    ProductionScene scene(Split("Get,1,0,0,4,167,0,1,1",','));std::array<std::uint32_t,9> tokens{};
    for (std::uint32_t owner=0;owner<9;++owner) { tokens[owner]=scene.deck->Get(owner);Put(scene.records,owner*180+0x9c,owner+31);Put(scene.records,owner*180+8,owner+1); } // 실제현재포인터만각행에설치한다.
    Put(scene.records,3*180+0x68,0xdeadcaf0U);CHECK(Throws([&] { scene.deck->CleanupPlayerLinksForState([&] { return std::span<std::uint8_t>(scene.records); }); }) && !scene.deck->PlayerLinksReturnedForState());
    for (std::uint32_t owner=0;owner<9;++owner) { CHECK(Get(scene.records,owner*180+0x9c)==(owner<=3 ? 0U : owner+31));CHECK(Get(scene.records,owner*180+8)==(owner<=3 ? 0U : owner+1));if (owner<3) CHECK(Get(scene.records,owner*180+0x68)==0); } // 실패행뒤원문미방문필드를지우지않는다.
    CHECK(Get(scene.records,3*180+0x68)==0xdeadcaf0U);Put(scene.records,3*180+0x68,tokens[3]);auto pass=scene.deck->CleanupPlayerLinksForState([&] { return std::span<std::uint8_t>(scene.records); });Put(scene.records,8*180+8,7);CHECK(Throws([&] { scene.deck->ReturnPlayerLinksForState(std::move(pass)); }) && !scene.deck->PlayerLinksReturnedForState());Put(scene.records,8*180+8,0);scene.deck->ReturnPlayerLinksForState(std::move(pass));CHECK(scene.deck->PlayerLinksReturnedForState());
}

// 다른모듈과소멸뒤같은주소의새Deck은이전정상복귀증빙을소비할수없다.
TEST_CASE(ProductionDeck_StateLinksForeignMovedAndExpiredProofsAreRejected) {
    ProductionScene first(Split("Get,1,0,0,4,167,0,1,1",',')),second(Split("Get,1,0,0,4,167,0,1,1",','));auto pass=first.deck->CleanupPlayerLinksForState([&] { return std::span<std::uint8_t>(first.records); });CHECK(Throws([&] { second.deck->ReturnPlayerLinksForState(std::move(pass)); }));first.deck->ReturnPlayerLinksForState(std::move(pass));CHECK(first.deck->PlayerLinksReturnedForState() && !second.deck->PlayerLinksReturnedForState() && Throws([&] { first.deck->ReturnPlayerLinksForState(std::move(pass)); }));
    std::optional<RawProductionDeck> reused;reused.emplace(*second.player,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); });const auto* address=&*reused;auto old=reused->CleanupPlayerLinksForState([&] { return std::span<std::uint8_t>(second.records); });reused.reset();reused.emplace(*second.player,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); });CHECK(&*reused==address && Throws([&] { reused->ReturnPlayerLinksForState(std::move(old)); }) && !reused->PlayerLinksReturnedForState());
}

// 실제부모의현재getter가재진입해도전체정리호출이복귀하기전에는이전pass를소비하지못한다.
TEST_CASE(ProductionDeck_StateLinksActualParentMustReturnBeforeConsumption) {
    ProductionScene scene(Split("Get,1,0,0,4,167,0,1,1",','));auto previous=scene.deck->CleanupPlayerLinksForState([&] { return std::span<std::uint8_t>(scene.records); });std::size_t reads{};bool rejected{};
    auto current=scene.deck->CleanupPlayerLinksForState([&] { if (!reads++) try { scene.deck->ReturnPlayerLinksForState(std::move(previous)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("진행 중")!=std::string_view::npos; }return std::span<std::uint8_t>(scene.records); });CHECK(reads==27 && rejected && !scene.deck->PlayerLinksReturnedForState());scene.deck->ReturnPlayerLinksForState(std::move(current));CHECK(scene.deck->PlayerLinksReturnedForState());
}
