// 같은 Client의 섬/신전/영역을 World·Session·MP·패킷 실행 없이 조립해 실제 수명을 확인한다.
#include "DudeSpawnSupport.h"
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include "client/RawCampaignOwnership.h"
#include "client/RawProcessHostStorage.h" //같은실제Host의성장Regular부착/실행을직접관찰한다.
#include "o/SquidProcess.h" //실제Regular의사건/횟수/주기를검사하며가짜예약을만들지않는다.
#include "o/RawPlayerParticipation.h" //공개Player초기화의동맹mask자식도기존실제본문을사용한다.
#include "o/RawAiWallet.h" //삭제환불의실제owner0/NULL brain지갑관문을그대로사용한다.
#include "o/RawAiObjectNotice.h" //실제Player의NULL brain은기존전체통지본문의자연반환으로처리한다.
#include "o/RawBombTargets.h" //같은TYPE사거리를쓰던기존다른소비자의연결도확인한다.
#include <bit>
#include <optional>
#include <iostream> //최초실패의실제삭제깊이/현재필드를보존한다.

namespace { //명시입력/장치관찰은게임본문이나원본기대값을대체하지않는다.
using namespace netstorm::client;using namespace netstorm::o; //제품의실제동일부품을사용한다.
// 선택하지 않은 네트워크/효과를 성공 대용으로 처리하지 않는다.
template<class Result,class... Args> std::function<Result(Args...)> RegionForbidden(std::string_view label="영역 공통 검사에서 선택 금지된 외부 효과") {
    return [label](Args...)->Result { throw std::logic_error(std::string(label)); };
}
// 실제 원문 폭만 써 슬롯/섬/Player의 이웃 바이트를 보존한다.
void RegionPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //정렬되지않은지정DWORD만기록한다.
}
// 실제 현재 필드의 DWORD 비트만 읽는다.
std::uint32_t RegionRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result{};for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[offset+index])<<(index*8);return result; //이웃필드를섞지않고little endian으로조합한다.
}
struct RegionScene { //게임실행기없이실제장기owner/보호자산/물리와선택장치를소유한다.
    std::shared_ptr<ClientGameplayGlobals> gameplay{std::make_shared<ClientGameplayGlobals>()}; //같은현재권한/전역이다.
    std::shared_ptr<RawSurfaceStorage> storage{MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay->Authority(),gameplay->GameAux())}; //실제원본자산의유일Pool/Hash다.
    std::shared_ptr<ClientRandomGlobals> random{std::make_shared<ClientRandomGlobals>()}; //같은현재게임난수다.
    std::shared_ptr<ClientSpGlobals> sp{std::make_shared<ClientSpGlobals>(random,[] { return 123U; })}; //초기화하지않은실제분산SP다.
    std::shared_ptr<ClientPlayerRecords> players{std::make_shared<ClientPlayerRecords>()}; //처음base0인실제Player할당owner다.
    std::shared_ptr<ClientKencloudGlobals> kencloud{std::make_shared<ClientKencloudGlobals>()}; //같은실제redrawDWORD다.
    std::shared_ptr<ClientConstructionGlobals> owner{std::make_shared<ClientConstructionGlobals>()}; //같은본문/상태를보유할장기owner다.
    std::optional<std::string> detail; //원문진단상세의실제수명이다.
    std::uint32_t refreshes{},traces{},clears{},themes{1},themeReads{}; //외부선택장치의실제호출을관찰한다.
    std::function<void()> onRefresh; //실제UI선택뒤base/장치변경을관찰한다.
    std::unique_ptr<RawAiWallet> wallet; //기존실제현재Player지갑본문이며비영AI선택은금지한다.
    RawAiBrainMemory aiMemory{0x26000000U,RegionForbidden<void,std::span<std::uint8_t>>("영역 검사 비영 AI 할당 금지")}; //빈실제메모리owner이며가짜brain을할당하지않는다.
    double now{};std::uint32_t outpostType{126}; //NULL brain통지가읽지않는현재시각/원문TYPE입력이다.
    std::uint32_t spRefreshes{},walletCalls{},aiNotices{},lostSounds{},postPopSelections{}; //실제공통환불/통지/표시장치의선택수를관찰한다.
    // 최초 결합은 원문 게임/Player 초기화·패킷·월드 대여를 실행하지 않는다.
    explicit RegionScene(bool observePostPop=false) {
        storage->BindTransmissionDevices({RegionForbidden<void,std::span<std::uint8_t>>("영역 검사 금지 Native 방송"),RegionForbidden<bool,std::int32_t>(),RegionForbidden<void,std::int32_t,std::span<std::uint8_t>>()});
        storage->BindFenceAlliances([weak=std::weak_ptr<ClientPlayerRecords>(players)](auto row,auto column) { const auto current=weak.lock();if (!current) throw std::logic_error("검사 실제 Player 반납 뒤 동맹 선택");return current->Alliances().at(row*9+column); }); //게임/Player초기화없이실제동맹배열만선택호출동안pin한다.
        storage->BindSerializationGlobals(std::make_shared<FortSerializationGlobals>());
        storage->BindDisplayDiagnostics(detail,[](auto,auto,auto) { throw std::logic_error("유효 영역 입력에서 원문 단언 발생"); });
        storage->BindGraphTrace([this](auto,std::string_view) { ++traces; });storage->EnablePatchDestroyDiagnostics();
        RawSurfaceDisplayBindings display;display.view=[] { return SquidDisplayView{0,0,65536,{0,0,1024,768}}; };display.suppressed=[] { return true; };display.invalidate=RegionForbidden<void,SquidDisplayRect,std::uint32_t>();storage->BindDisplayDevices(std::move(display));
        wallet=std::make_unique<RawAiWallet>(RegionForbidden<AiWalletMemoryAccess>("영역 검사 비영 AI 지갑 대여 금지"),[this]()->std::span<const std::uint8_t> { return players->Records(); },AiWalletReads{[this] { return storage->ownerMode.challenge; },RegionForbidden<bool>()});
        storage->BindRewardDevices({std::shared_ptr<ScrambledSpStore>(sp,&sp->Store()),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->world.editor),[this] { ++spRefreshes; },[this](auto player,auto amount) { ++walletCalls;wallet->Credit(player,amount); }});
        storage->BindPostPopDevices({[this](auto player,auto sid,auto added) { ++aiNotices;if (storage->ownerMode.challenge || std::bit_cast<std::int32_t>(player)<1) return;RawAiObjectNotice body(aiMemory,storage->pool,storage->types,players->Records(),now,outpostType,[this] { return storage->ownerMode.challenge; },{});body.Notice(player,sid,added); },[current=gameplay] { return current->player.battleOwner; },[this,observePostPop] { if (!observePostPop) RegionForbidden<void>("영역 검사 금지 PostPop UI")();CHECK(gameplay->selection.mouseType==0);++postPopSelections; }});
        storage->BindDeletionDevices({[](auto,auto,auto) {},[this](Sid sid) { ++lostSounds;CHECK(storage->pool.Slot(sid)[34]==gameplay->player.battleOwner); }});storage->BindProcessDevices({[this] { return gameplay->selection.selected; },[this] { ++clears;gameplay->selection.selected=0; }});
        storage->BindDamageableDevices({std::shared_ptr<ContainedFinderState>(gameplay,&gameplay->finder),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->FortKeepGenus()),RegionForbidden<void,Sid>("영역 검사 금지 붕괴"),RegionForbidden<void,Sid>("영역 검사 금지 폭발"),RegionForbidden<void,const DamageableSoundEvent&>("영역 검사 금지 피해 소리"),RegionForbidden<void,float,float,std::uint32_t,std::uint16_t>("영역 검사 금지 해방 사제 생성"),RegionForbidden<void,Sid,std::int32_t,bool>("영역 검사 금지 Dude 드롭"),RegionForbidden<void,float,float,std::uint32_t,std::uint32_t>("영역 검사 금지 섬 드롭")});
        storage->links.bridgeType=82;storage->links.islandType=94;storage->links.stalagType=95;storage->links.connectorType=155;storage->links.noIslandType=157;
        owner->Bind(storage,gameplay,{});owner->BindBacking(sp,players,gameplay->BattleOptionsOwner());owner->BindRandom(random);owner->BindRegionContext(kencloud);owner->BindDevices(Devices());
        storage->BindRegionLifecycle(
            [weak=std::weak_ptr<ClientConstructionGlobals>(owner)](auto sid,auto player) { const auto current=weak.lock();if (!current) throw std::logic_error("검사 실제 영역 owner 반납 뒤 선택");current->VortexOwner().Set(sid,player); },
            [weak=std::weak_ptr<ClientConstructionGlobals>(owner)](auto sid,auto flags) { const auto current=weak.lock();if (!current) throw std::logic_error("검사 실제 영역 owner 반납 뒤 선택");current->VortexPre().PreDestroy(sid,flags); },
            [weak=std::weak_ptr<ClientConstructionGlobals>(owner)](auto x,auto y) { const auto current=weak.lock();if (!current) throw std::logic_error("검사 실제 영역 owner 반납 뒤 선택");current->Regions().UpdateNative(x,y); });
    }
    ~RegionScene() { owner.reset();storage.reset(); } //관찰기가살아있는동안실제Host와장치를먼저반납한다.
    // 필수 장치는 선택 시 실패하게 하고 영역 UI/설정의 작은 외부 경계만 관찰한다.
    ClientConstructionDevices Devices() {
        ClientConstructionDevices devices{RegionForbidden<std::int32_t,std::string_view,std::int32_t>(),RegionForbidden<double>(),RegionForbidden<void>(),RegionForbidden<void>(),RegionForbidden<void,PlayerGeyserBoundsAssertion>(),RegionForbidden<std::string,std::string_view,std::string_view>(),RegionForbidden<void,std::string_view>()};
        devices.refreshProduction=[this] { ++refreshes;if (onRefresh) onRefresh(); };devices.themeMode=[this] { ++themeReads;return themes; };return devices;
    }
    // 공개된 실제 초기화 부모로 이전 배열을 반납하고 새 Player/factory와 동맹표를 준비한다.
    std::uint32_t PlayerBlock() {
        PlayerParticipationState state;std::array<std::uint8_t,9> hidden{}; //원문현재known0과초기화자식의같은지역입력이다.
        players->Initialize([this,&state,&hidden](auto player,auto mask) { RawPlayerParticipation child(OriginalEdition::Patch1078,players->Records(),players->Alliances(),hidden,state,{});child.SetAlliance(player,mask); });return players->Base();
    }
    void PlayerWrite(std::uint32_t player,std::size_t field,std::uint32_t value) { RegionPut(players->Records(),player*0xb4+field,value); } //공개Records의지정현재필드만입력한다.
    std::uint32_t PlayerRead(std::uint32_t player,std::size_t field) { return RegionRead(players->Records(),player*0xb4+field); } //공개Records에서현재필드의실제비트를읽는다.
    // 지정한 실제 생성자 VT를 원본 TYPE 표에서 찾아 실제 Factory에 전달한다.
    Sid Asset(std::uint32_t table) {
        for (std::uint32_t type=70;type<storage->types.size();++type) { //사용할수없는실제생성자는선택후보가아니다.
            std::uint32_t current{};try { current=storage->Factory().ConstructorVtable(storage->types[type]); }catch (const std::logic_error&) { continue; }
            if (current==table) { const auto sid=storage->CreateAsset(type,2);storage->pool.AllocatedBytes(sid)[40]|=9U;return sid; }
        }
        throw std::logic_error("실제 영역 자산 생성자 VT 부재");
    }
    // 실제 단일 지면의 raw 지역/좌표와 hash0 입력을 연결한다. Pop·송신은 실행하지 않는다.
    Sid Surface(std::uint32_t region,int x=100,int y=100) {
        const auto sid=storage->CreateAsset(storage->GameAux()->TerrainType(),2);auto raw=storage->pool.AllocatedBytes(sid);raw[40]|=9U;
        raw[8]=static_cast<std::uint8_t>(region);raw[9]=static_cast<std::uint8_t>(region>>8);RegionPut(raw,14,std::bit_cast<std::uint32_t>(float(x)));RegionPut(raw,18,std::bit_cast<std::uint32_t>(float(y)));storage->hash.Cell(0,x,y)=sid.value;return sid;
    }
    // 실제 섬 실행기의 원문 Add/Bounds로 한 기록만 준비한다.
    std::uint32_t Island() { owner->Islands().Reset();const auto result=owner->Islands().Add(0,0);owner->Islands().Bounds(static_cast<std::int32_t>(result),{90,90,110,110});return result; }
    // 현재 슬롯의 원문 두 float 좌표만 지정한다.
    void Position(Sid sid,float x=100,float y=100) { auto raw=storage->pool.AllocatedBytes(sid);RegionPut(raw,14,std::bit_cast<std::uint32_t>(x));RegionPut(raw,18,std::bit_cast<std::uint32_t>(y)); }
    // 실제136 생성자를 사용하고 명시 원문 슬롯 입력만 지정한다. Pop/송신은 실행하지 않는다.
    Sid Fence(float x=100,float y=100,std::uint8_t player=0,std::uint8_t extra=0) { const auto sid=storage->CreateAsset(136,2);Position(sid,x,y);auto raw=storage->pool.AllocatedBytes(sid);raw[34]=player;raw[40]=extra;CHECK(RegionRead(raw,0)==0x509470U);return sid; }
    // 원문 finder 입력의 실제 SHP/TYPE 단계와 next 체인을 같은 물리에 설치한다.
    void Index(Sid sid) { const auto raw=storage->pool.Slot(sid);const auto size=storage->Display().FrameSize(OriginalEdition::Patch1078,raw,RegionRead(raw,36));const auto level=SquidHash::ObjectLevel(storage->types[raw[10]].flags2,size[0],size[1]);auto& head=storage->hash.Bucket(level,std::bit_cast<float>(RegionRead(raw,14)),std::bit_cast<float>(RegionRead(raw,18)));auto bytes=storage->pool.AllocatedBytes(sid);bytes[4]=static_cast<std::uint8_t>(head);bytes[5]=static_cast<std::uint8_t>(head>>8);head=sid.value; }
    // 실제 생존 슬롯만 세며 새 객체의 수명 비트를 성공값으로 바꾸지 않는다.
    std::vector<Sid> OfType(std::uint32_t type) { std::vector<Sid> result;for (std::uint32_t value=5;value<storage->pool.Capacity();++value) { const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=storage->pool.Slot(sid);if (!(raw[11]&1U) && raw[10]==type) result.push_back(sid); }return result; } //현재같은Pool의할당범위만순회한다.
    // 실패는숨기지않고실제현재필드/삭제깊이를기록한뒤같은예외를다시전달한다.
    bool Destroy(Sid sid) { try { return storage->DestroyAsset(sid,0x10); }catch (...) { const auto raw=storage->pool.Slot(sid);std::cerr<<"type="<<std::uint32_t(raw[10])<<" state="<<std::uint32_t(raw[11])<<" extra="<<std::uint32_t(raw[40])<<" pre="<<storage->BaseDestroy().PreDepth()<<" post="<<storage->BaseDestroy().PostDepth()<<" ui="<<refreshes<<'\n';throw; } }
};
}

// 고유Bomb본문구성은Player/SP/난수/현재TYPE를초기화하거나World를대여하지않는다.
TEST_CASE(ClientBomb_LongBodySharesCurrentPhysicalOwner) {
    RegionScene scene;const auto random=scene.random->Game().State();scene.storage->types[113].range=19.75F;
    auto* body=&scene.storage->BombPre();CHECK(&scene.storage->BombPre()==body && scene.storage->types[113].range==19.75F);
    CHECK(scene.players->Base()==0 && !scene.sp->Store().Initialized() && scene.random->Game().State()==random && !scene.storage->Borrowed());
}

// TYPE이름이나범주로넓히지않고매호출의실제VT502370만선택한다.
TEST_CASE(ClientBomb_HandlesOnlyCurrentActualVtable) {
    RegionScene scene;const auto sid=scene.Asset(0x502370U);auto raw=scene.storage->pool.AllocatedBytes(sid);CHECK(raw[10]==113 && scene.storage->BombPre().Handles(sid));
    RegionPut(raw,0,0x509470U);CHECK(!scene.storage->BombPre().Handles(sid));RegionPut(raw,0,0x502370U);CHECK(scene.storage->BombPre().Handles(sid));
}

// 원문의extra1/8각비트는폭발/소리없이현재공통Pre의실제선택해제를실행한다.
TEST_CASE(ClientBomb_NoWorldExtraGuardsReachActualCommonParent) {
    for (const auto extra:{1U,8U,9U}) { //각자연관문입력을독립장기물리에서확인한다.
        RegionScene scene;const auto sid=scene.Asset(0x502370U);scene.storage->pool.AllocatedBytes(sid)[40]=static_cast<std::uint8_t>(extra);scene.gameplay->selection.selected=sid.value;
        CHECK(scene.storage->DestroyAsset(sid,0x100010U));
        CHECK(scene.clears==1 && scene.gameplay->selection.selected==0 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.players->Base()==0 && !scene.storage->Borrowed());
    }
}

// extra가0이어도원문의100000플래그가없으면큰효과보다같은공통부모를직접호출한다.
TEST_CASE(ClientBomb_NoWorldMissingExplosionFlagUsesCommonParent) {
    RegionScene scene;const auto sid=scene.Asset(0x502370U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.gameplay->selection.selected=sid.value;
    CHECK(scene.storage->DestroyAsset(sid,0x10U));
    CHECK(scene.clears==1 && scene.gameplay->selection.selected==0 && scene.storage->BaseDestroy().PreDepth()==0 && !scene.storage->Borrowed());
}

// 실제외부송신은원문10비트로억제하고동일고유Pre/공통Post/풀반납까지실행한다.
TEST_CASE(ClientBomb_NoWorldDestroyRunsBothActualParents) {
    RegionScene scene;const auto sid=scene.Asset(0x502370U);const auto free=scene.storage->pool.FreeCount();scene.gameplay->selection.selected=sid.value;
    CHECK(scene.storage->DestroyAsset(sid,0x100010U));
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.clears==1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && !scene.storage->Borrowed());
}

// 선택된큰폭발장치가실패하면뒤소리/선택해제/공통Pre를앞당기지않는다.
TEST_CASE(ClientBomb_SelectedExplosionFailurePreservesEarlierBoundary) {
    RegionScene scene;const auto sid=scene.Asset(0x502370U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.gameplay->selection.selected=sid.value;
    const auto raw=scene.storage->pool.Slot(sid);const std::vector<std::uint8_t> before(raw.begin(),raw.end());bool rejected{};
    try { scene.storage->BombPre().PreDestroy(sid,0x100010U); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="영역 검사 금지 폭발"; }
    const auto after=scene.storage->pool.Slot(sid);CHECK(rejected && std::equal(before.begin(),before.end(),after.begin()) && scene.clears==0 && scene.gameplay->selection.selected==sid.value && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 장기본문을직접호출해도다른현재VT를Point Blast로간주하거나공통부모로조용히통과시키지않는다.
TEST_CASE(ClientBomb_WrongActualVtableIsRejectedBeforeEffects) {
    RegionScene scene;const auto sid=scene.Fence();bool rejected{};
    try { scene.storage->BombPre().PreDestroy(sid,0x100010U); }catch (const std::invalid_argument& error) { rejected=std::string_view(error.what())=="Bomb 고유전처리 VT불일치"; }
    CHECK(rejected && scene.clears==0 && scene.storage->BaseDestroy().PreDepth()==0);
}

// 전체 장기 본체를 구성해도 Player base/난수/섬/장기 캐시는 초기화하지 않는다.
TEST_CASE(ClientRegions_NoWorldBindDoesNotReadPlayersOrResetState) {
    RegionScene scene;const auto random=scene.random->Game().State();scene.kencloud->WorldRedraw()=19;scene.gameplay->MapRevision()=23;
    auto* islands=&scene.owner->Islands();auto* votes=&scene.owner->Regions();auto* vortex=&scene.owner->VortexOwner();auto* pre=&scene.owner->VortexPre();
    scene.owner->RegionPaintGlobals().coreVariants.emplace().fill(17);scene.owner->RegionVoteGlobals().affected.push_back({1,2,3});scene.owner->BindRegionContext(scene.kencloud);
    CHECK(&scene.owner->Islands()==islands && &scene.owner->Regions()==votes && &scene.owner->VortexOwner()==vortex && &scene.owner->VortexPre()==pre);
    CHECK(scene.players->Base()==0 && scene.random->Game().State()==random && scene.kencloud->WorldRedraw()==19 && scene.gameplay->MapRevision()==23);
    CHECK(scene.owner->RegionPaintGlobals().coreVariants->at(0)==17 && scene.owner->RegionVoteGlobals().affected.size()==1 && !scene.storage->Borrowed() && scene.storage->Generation()==0);
    const auto lifetime=std::weak_ptr<ClientConstructionGlobals>(scene.owner);scene.owner.reset();CHECK(lifetime.expired()); //Storage의세callback이강한역참조순환을만들지않는다.
}

// 실제4f530 편향으로 다음 칸을 선택하며 Player/월드 없이 같은 hash0를 읽는다.
TEST_CASE(ClientRegions_CurrentGetterUsesOriginalSnapAndSameHash) {
    RegionScene scene;const auto surface=scene.Surface(2,101,101);CHECK(scene.owner->IslandNumber(100.25F,100.25F)==2);
    scene.storage->pool.AllocatedBytes(surface)[8]=7;CHECK(scene.owner->IslandNumber(100.25F,100.25F)==7 && scene.owner->IslandNumber(256,256)==127 && scene.players->Base()==0);
}

// 모든실제신전/Outpost는NoWorld가상10에서같은부모를거쳐깊이/풀을반납한다.
TEST_CASE(ClientRegions_FiveActualDeletionParentsWithoutRuntime) {
    for (const auto table:{0x5196f0U,0x5197a8U,0x519860U,0x519918U,0x509210U}) { //공중3종/전투/MP는선택하지않는다.
        RegionScene scene;const auto sid=scene.Asset(table);const auto before=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10));
        CHECK(scene.storage->pool.FreeCount()==before+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.traces==1);
        CHECK(scene.players->Base()==0 && scene.refreshes==0 && !scene.storage->Borrowed());
    }
}

// Player base가바뀐뒤이전신전해제와새신전등록은새블록만사용한다.
TEST_CASE(ClientRegions_VortexOwnerReadsActualCurrentPlayerBase) {
    RegionScene scene;auto& body=scene.owner->VortexOwner();const auto sid=scene.Asset(0x5196f0U);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=0;raw[34]=1;
    const auto oldBase=scene.PlayerBlock();scene.PlayerWrite(1,0x7c,sid.value);scene.PlayerWrite(1,0x80,sid.value);
    scene.PlayerBlock();scene.PlayerWrite(1,0x7c,sid.value);scene.PlayerWrite(1,0x80,sid.value);scene.gameplay->MapRevision()=41;
    scene.storage->SetAssetOwner(sid,2);CHECK(&body==&scene.owner->VortexOwner() && scene.storage->pool.Slot(sid)[34]==2);
    CHECK(scene.PlayerRead(1,0x7c)==0 && scene.PlayerRead(1,0x80)==0 && scene.PlayerRead(2,0x7c)==sid.value && scene.PlayerRead(2,0x80)==sid.value);
    CHECK(scene.players->Base()!=oldBase && scene.refreshes==1 && scene.gameplay->MapRevision()==42);
}

// 전체섬Owner는실제지면owner를거주구역에전파하고현재Player첫지역만쓴다.
TEST_CASE(ClientRegions_IslandOwnerPropagatesResidenceAndCurrentFirstRegion) {
    RegionScene scene;auto& islands=scene.owner->Islands();const auto region=scene.Island();const auto surface=scene.Surface(region);scene.storage->pool.AllocatedBytes(surface)[34]=2;
    const auto residence=scene.storage->CreateAsset(scene.owner->FinishOwnerState().residenceType,2);scene.Position(residence);scene.storage->pool.AllocatedBytes(residence)[40]|=9U;
    scene.storage->sidLists.residences.entries={residence.value};scene.storage->sidLists.residences.count=1;const auto oldBase=scene.PlayerBlock();scene.PlayerWrite(2,0x78,127);
    scene.PlayerBlock();scene.PlayerWrite(2,0x78,127);scene.kencloud->WorldRedraw()=30;scene.gameplay->MapRevision()=99;islands.Owner(static_cast<std::int32_t>(region),2);
    CHECK(islands.Owner(static_cast<std::int32_t>(region))==2 && scene.storage->pool.Slot(residence)[34]==2 && scene.PlayerRead(2,0x78)==region);
    CHECK(scene.players->Base()!=oldBase && scene.kencloud->WorldRedraw()==31 && scene.gameplay->MapRevision()==1);
}

// 실제Outpost두목록압축뒤전체투표/도장이같은지면을중립으로바꾼다.
TEST_CASE(ClientRegions_OutpostDeletionRunsActualPaintAfterLists) {
    RegionScene scene;const auto region=scene.Island();const auto surface=scene.Surface(region);scene.storage->pool.AllocatedBytes(surface)[34]=2;
    const auto sid=scene.Asset(0x509210U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=0;raw[34]=1;
    scene.storage->sidLists.outposts.entries={sid.value,sid.value};scene.storage->sidLists.outposts.count=2;scene.storage->sidLists.residences.entries={sid.value};scene.storage->sidLists.residences.count=1;
    scene.PlayerBlock();scene.kencloud->WorldRedraw()=10;scene.gameplay->MapRevision()=77;CHECK(scene.Destroy(sid));
    CHECK(scene.storage->sidLists.outposts.count==0 && scene.storage->sidLists.residences.count==0 && scene.storage->pool.Slot(surface)[34]==0);
    CHECK(scene.kencloud->WorldRedraw()==13 && scene.gameplay->MapRevision()==1 && scene.players->Base()!=0 && scene.refreshes==0 && scene.owner->RegionPaintGlobals().coreVariants.has_value());
    CHECK(scene.sp->Store().Initialized() && scene.spRefreshes==1 && scene.walletCalls==1 && scene.aiNotices==1 && scene.aiMemory.Allocations().empty());
}

// 신전삭제는실제섬/도장/공통부모뒤UI가바꾼현재Player를정리한다.
TEST_CASE(ClientRegions_VortexDeletionUsesCurrentPlayersAfterUi) {
    RegionScene scene;const auto region=scene.Island();const auto surface=scene.Surface(region);scene.storage->pool.AllocatedBytes(surface)[34]=2;
    const auto sid=scene.Asset(0x5196f0U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=0;raw[34]=1;
    scene.owner->Islands().Temple(static_cast<std::int32_t>(region),sid.value);scene.storage->sidLists.temples.entries={sid.value};scene.storage->sidLists.temples.count=1;scene.storage->sidLists.residences.entries={sid.value};scene.storage->sidLists.residences.count=1;
    const auto oldBase=scene.PlayerBlock();scene.PlayerWrite(1,0x7c,sid.value);scene.PlayerWrite(1,0x84,sid.value);
    scene.onRefresh=[&] { if (scene.refreshes==2) { scene.PlayerBlock();scene.storage->pool.AllocatedBytes(sid)[34]=2;scene.PlayerWrite(2,0x7c,sid.value);scene.PlayerWrite(2,0x80,sid.value);scene.PlayerWrite(2,0x84,sid.value);scene.PlayerWrite(2,0x88,sid.value); } };
    scene.kencloud->WorldRedraw()=20;scene.gameplay->MapRevision()=77;CHECK(scene.Destroy(sid));
    CHECK(scene.refreshes==2 && scene.storage->sidLists.temples.count==0 && scene.storage->sidLists.residences.count==0 && scene.owner->Islands().Temple(static_cast<std::int32_t>(region))==0);
    CHECK(scene.storage->pool.Slot(surface)[34]==0 && scene.kencloud->WorldRedraw()==24 && scene.gameplay->MapRevision()==1);
    CHECK(scene.PlayerRead(2,0x7c)==0 && scene.PlayerRead(2,0x80)==0 && scene.PlayerRead(2,0x84)==0 && scene.PlayerRead(2,0x88)==0 && scene.players->Base()!=oldBase);
    CHECK(scene.sp->Store().Initialized() && scene.spRefreshes==1 && scene.walletCalls==1 && scene.aiNotices==1 && scene.aiMemory.Allocations().empty());
}

// 실제소유자wrapper는같은장기신전/상태를빌리고다른사제분배는짧은수명에남긴다.
TEST_CASE(ClientRegions_WrapperSharesBodyAndCurrentDevicesPinSelectedCall) {
    RegionScene scene;const auto sid=scene.Asset(0x5196f0U);std::uint32_t others{};RawCampaignOwnership wrapper(scene.owner,[&](auto object,auto player) { CHECK(object==sid && player==8);++others; });CHECK(&wrapper.Vortex()==&scene.owner->VortexOwner());
    std::weak_ptr<int> watch;auto next=scene.Devices();std::uint32_t oldCalls{},newCalls{};next.refreshProduction=[&] { ++newCalls; };
    { auto token=std::make_shared<int>(1);watch=token;auto first=scene.Devices();first.refreshProduction=[&,token] { ++oldCalls;scene.owner->BindDevices(next);CHECK(!watch.expired()); };scene.owner->BindDevices(std::move(first)); }
    wrapper.SetOwner(sid,1);CHECK(watch.expired() && oldCalls==1 && newCalls==0);wrapper.SetOwner(sid,2);CHECK(newCalls==1 && others==0);
    RegionPut(scene.storage->pool.AllocatedBytes(sid),0,0x503360U);wrapper.SetOwner(sid,8);CHECK(others==1); //비신전은현재실제상위분배를유지한다.
}

// 같은투표본문이선택한현재TYPE테마와현재설정을매호출다시읽는다.
TEST_CASE(ClientRegions_VotesUseCurrentTypeThemeAndActualPaintState) {
    RegionScene scene;const auto region=scene.Island();const auto surface=scene.Surface(region);const auto temple=scene.Asset(0x5196f0U);scene.Position(temple);scene.storage->pool.AllocatedBytes(temple)[34]=1;
    scene.storage->sidLists.temples.entries={temple.value};scene.storage->sidLists.temples.count=1;auto& type=scene.storage->types[scene.storage->pool.Slot(temple)[10]];auto* votes=&scene.owner->Regions();
    type.theme=1;votes->UpdateNative(100,100);CHECK(scene.owner->RegionPaintGlobals().theme==3 && scene.storage->pool.Slot(surface)[34]==1 && scene.themeReads==1);
    type.theme=2;votes->UpdateNative(100,100);CHECK(&scene.owner->Regions()==votes && scene.owner->RegionPaintGlobals().theme==1 && scene.themeReads==2);
    scene.themes=0;votes->UpdateNative(100,100);CHECK(scene.owner->RegionPaintGlobals().theme==0 && scene.themeReads==3 && scene.players->Base()==0);
}

// 원문signed음수목록은물리선검사없이건너뛰고도장/현재두DWORD의후단은실행한다.
TEST_CASE(ClientRegions_NegativeNativeCountsSkipListsAndKeepPaint) {
    RegionScene scene;scene.storage->sidLists.outposts.count=0xffffffffU;scene.storage->sidLists.temples.count=0x80000000U;scene.kencloud->WorldRedraw()=71;scene.gameplay->MapRevision()=99;
    scene.owner->Regions().UpdateNative(100,100);CHECK(scene.kencloud->WorldRedraw()==74 && scene.gameplay->MapRevision()==1 && scene.players->Base()==0 && scene.themeReads==0);
}

// 실제Player선택실패는앞선공통Owner쓰기만남기고임시배열/UI성공을만들지않는다.
TEST_CASE(ClientRegions_MissingSelectedPlayerPreservesOriginalOwnerPrefix) {
    RegionScene scene;const auto sid=scene.Asset(0x5196f0U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.gameplay->MapRevision()=19;bool rejected{};
    try { scene.storage->SetAssetOwner(sid,2); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && scene.storage->pool.Slot(sid)[34]==2 && scene.players->Base()==0 && scene.refreshes==0 && scene.gameplay->MapRevision()==19);
}

// 반납된장기owner는성공으로우회하지않고원문두목록제거/삭제접두뒤명시실패한다.
TEST_CASE(ClientRegions_ExpiredOwnerKeepsOutpostDeletionPrefix) {
    RegionScene scene;const auto sid=scene.Asset(0x509210U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.storage->sidLists.outposts.entries={sid.value};scene.storage->sidLists.outposts.count=1;scene.storage->sidLists.residences.entries={sid.value};scene.storage->sidLists.residences.count=1;
    const auto before=scene.storage->pool.FreeCount();scene.owner.reset();bool rejected{};try { scene.storage->DestroyAsset(sid,0x10); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="검사 실제 영역 owner 반납 뒤 선택"; }
    CHECK(rejected && scene.storage->sidLists.outposts.count==0 && scene.storage->sidLists.residences.count==0 && scene.storage->pool.FreeCount()==before && (scene.storage->pool.Slot(sid)[11]&3U)==2U);
    CHECK(scene.storage->BaseDestroy().PreDepth()==1 && scene.storage->BaseDestroy().PostDepth()==0 && scene.traces==1);
}

// 신전Pre의현재signed음수Outpost개수도같은실제영역부모와UI후단을통과한다.
TEST_CASE(ClientRegions_VortexPreSkipsNegativeCurrentOutposts) {
    RegionScene scene;const auto region=scene.Island();scene.Surface(region);const auto sid=scene.Asset(0x5196f0U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[40]=0;
    scene.owner->Islands().Temple(static_cast<std::int32_t>(region),sid.value);scene.storage->sidLists.temples.entries={sid.value};scene.storage->sidLists.temples.count=1;scene.storage->sidLists.outposts.count=0xffffffffU;
    CHECK(scene.Destroy(sid));CHECK(scene.refreshes==2 && scene.storage->sidLists.outposts.count==0xffffffffU && scene.storage->sidLists.temples.count==0 && scene.players->Base()==0);
}

// 같은실제세역할주소와본문을월드없이보유하며Player/난수/로딩값을초기화하지않는다.
TEST_CASE(ClientIce_LongBodySharesCurrentRolesWithoutWorld) {
    RegionScene scene;const auto seed=scene.random->Game().State();scene.storage->LoadingDepth()=7;auto& types=scene.storage->IceTowerGlobals();auto* body=&scene.storage->IceTower();
    types.ice=70;types.growing=71;types.noIsland=72;scene.storage->GameAux()->RainBlockerType()=148;scene.storage->GameAux()->GrowingRainBlockerType()=149;scene.storage->links.noIslandType=94;
    CHECK(&scene.storage->IceTower()==body && &scene.storage->IceTowerGlobals()==&types && types.Ice()==148 && types.Growing()==149 && types.NoIsland()==94);
    CHECK(scene.players->Base()==0 && !scene.sp->Store().Initialized() && scene.random->Game().State()==seed && scene.storage->LoadingDepth()==7 && !scene.storage->Borrowed() && scene.storage->Generation()==0);
}

// 실제네생성자VT의후처리는성장형의공통부모와완성형의Damageable부모를구별한다.
TEST_CASE(ClientIce_FourActualPostParentsWithoutRuntime) {
    for (const auto table:{0x50e888U,0x50e940U,0x50ea08U,0x50ead0U}) { //추상입력의미선택성장/드롭/MP효과를추가하지않는다.
        RegionScene scene;const auto sid=scene.Asset(table);const auto before=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10));
        CHECK(scene.storage->pool.FreeCount()==before+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.traces==1 && scene.players->Base()==0);
    }
}

// 최초성장형Pop은같은Host에실제Regular를부착하고기본실행도같은장기본문으로돌아온다.
TEST_CASE(ClientIce_NoWorldPopSchedulesActualGrowthRegular) {
    RegionScene scene(true);const auto sid=scene.storage->CreateAsset(150,0);scene.storage->pool.AllocatedBytes(sid)[40]|=0x20U;
    CHECK(scene.storage->PopAsset(sid,128,128,0x10)==RawPopResult::Registered);auto* process=scene.storage->Processes().Host().FindEvent(sid,0x3a6);CHECK(process);if (!process) return;
    const auto expected=static_cast<float>(40.0/static_cast<double>(scene.storage->frames[150].Codes().size()));const auto frame=RegionRead(scene.storage->pool.Slot(sid),36);
    CHECK(process->Count()==0 && process->Payload()==expected && !process->Shared() && scene.storage->PostPop().ActivationDepth()==0 && scene.aiNotices==1 && scene.postPopSelections==1);
    process->RunFrame();CHECK(process->Count()==1 && process->Payload()==expected && RegionRead(scene.storage->pool.Slot(sid),36)==frame && scene.players->Base()==0 && !scene.storage->Borrowed());
}

// 성장사건이아니면프레임/TYPE/권한을읽는하위없이원래payload비트만돌려준다.
TEST_CASE(ClientIce_UnknownEventPreservesPayloadAndCurrentState) {
    RegionScene scene;const auto sid=scene.Asset(0x50e888U);RegionPut(scene.storage->pool.AllocatedBytes(sid),36,0xffffffffU);const auto seed=scene.random->Game().State();
    const auto payload=std::bit_cast<float>(0x7fc12345U);const auto result=scene.storage->IceTower().Handle(sid,17,0xffffffffU,payload);
    CHECK(std::bit_cast<std::uint32_t>(result)==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==0xffffffffU && scene.random->Game().State()==seed && scene.players->Base()==0);
}

// 최종생성권한이없으면마지막프레임에도새타워를만들지않고현재프레임수의주기를읽는다.
TEST_CASE(ClientIce_CurrentFrameCountAndAuthorityGate) {
    RegionScene scene;const auto sid=scene.Asset(0x50e888U);const auto count=scene.storage->frames[150].Codes().size();RegionPut(scene.storage->pool.AllocatedBytes(sid),36,static_cast<std::uint32_t>(count-2));scene.gameplay->Boss()=false;
    const auto free=scene.storage->pool.FreeCount();const auto delay=scene.storage->IceTower().Handle(sid,0x3a6,9,0.5F);
    CHECK(delay==static_cast<float>(40.0/static_cast<double>(count)) && scene.storage->pool.FreeCount()==free && RegionRead(scene.storage->pool.Slot(sid),36)==count-2);
}

// 현재지면에받침이없으면성장형의드롭조건은본문을생략하고실제공통Post까지끝난다.
TEST_CASE(ClientIce_GrowingDeleteQueriesActualEmptySurfaceAndCompletes) {
    RegionScene scene;const auto sid=scene.Asset(0x50e888U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[40]=0;
    const auto before=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x200010U));
    CHECK(scene.storage->pool.FreeCount()==before+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.players->Base()==0 && !scene.sp->Store().Initialized());
}

// 실제받침을찾은성장형은선택된큰드롭미연결을숨기지않고앞선삭제접두만보존한다.
TEST_CASE(ClientIce_GrowingDeletePreservesSelectedDropFailure) {
    RegionScene scene;const auto surface=scene.storage->CreateAsset(94,2);scene.Position(surface);scene.storage->pool.AllocatedBytes(surface)[40]=0;
    const auto size=scene.storage->Display().FrameSize(OriginalEdition::Patch1078,scene.storage->pool.Slot(surface),0);const auto level=SquidHash::ObjectLevel(scene.storage->types[94].flags2,size[0],size[1]); //실제SHP와genus로검색대상의hash단계를선택한다.
    scene.storage->hash.Bucket(level,100,100)=surface.value;CHECK(level>0 && scene.storage->DamageablePost().GenusAt(100,100,0x1000000U)==surface);
    const auto sid=scene.Asset(0x50e888U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[40]=0;const auto before=scene.storage->pool.FreeCount();bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x200010U); }catch (const std::logic_error& error) { std::cerr<<"ICE DROP: "<<error.what()<<" type="<<unsigned(scene.storage->pool.Slot(sid)[10])<<" extra="<<unsigned(scene.storage->pool.Slot(surface)[40])<<'\n';rejected=std::string_view(error.what())=="영역 검사 금지 섬 드롭"; }
    CHECK(rejected && scene.storage->pool.FreeCount()==before && (scene.storage->pool.Slot(sid)[11]&3U)==2U && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==1);
}

// 현재성장형TYPE를선택한실제생성/WORD/QA/owner뒤금지된Native외부효과의부분상태를보존한다.
TEST_CASE(ClientIce_RegrowthCreatesCurrentRoleBeforeExternalFailure) {
    RegionScene scene;scene.PlayerBlock();scene.gameplay->Server()=true;scene.storage->GameAux()->GrowingRainBlockerType()=70;
    const auto sid=scene.storage->CreateAsset(scene.storage->GameAux()->RainBlockerType(),2);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);CHECK(raw[10]==147 && RegionRead(raw,0)==0x50e940U);raw[40]=0;raw[34]=1;raw[35]=0xe2;raw[12]=0x5a;raw[13]=0xa5;bool rejected{}; //VT를공유하는TYPE78과구별해현재역할147을실제생성한다.
    try { scene.storage->DestroyAsset(sid,0x10); }catch (const std::logic_error& error) { std::cerr<<"ICE GROW: "<<error.what()<<" type="<<unsigned(scene.storage->pool.Slot(sid)[10])<<" role="<<scene.storage->GameAux()->RainBlockerType()<<" depth="<<scene.storage->BaseDestroy().PreDepth()<<'/'<<scene.storage->BaseDestroy().PostDepth()<<'\n';rejected=std::string_view(error.what())=="영역 검사 금지 Native 방송"; }
    std::uint32_t created{};for (std::uint32_t value=5;value<scene.storage->pool.Capacity();++value) { //현재같은Pool에서실제새TYPE70만찾고성공결과를만들지않는다.
        const Sid current{static_cast<std::uint16_t>(value)};const auto object=scene.storage->pool.Slot(current);if (current!=sid && !(object[11]&1U) && object[10]==70) { ++created;CHECK(object[12]==0x5a && object[13]==0xa5 && object[35]==0xe2 && object[34]==1 && RegionRead(object,36)==0); }
    }
    CHECK(rejected && created==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==1 && !scene.storage->Borrowed());
}

// 재성장조건은전투모드와별개인실제서버전역이며클라이언트에서는현재풀을반납한다.
TEST_CASE(ClientIce_RegrowthServerGateIsIndependentOfBattleMode) {
    RegionScene scene;scene.PlayerBlock();scene.gameplay->Server()=false;scene.storage->ownerMode.battle=true;const auto sid=scene.storage->CreateAsset(scene.storage->GameAux()->RainBlockerType(),2);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);CHECK(raw[10]==147 && RegionRead(raw,0)==0x50e940U);raw[40]=0;raw[34]=1;
    const auto before=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10));CHECK(scene.storage->pool.FreeCount()==before+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 현재생성TYPE가신전이면가상74의실제Player등록/생산갱신뒤에만Pop외부효과를선택한다.
TEST_CASE(ClientIce_RegrowthUsesVirtualOwnerOfCurrentRole) {
    RegionScene scene;scene.PlayerBlock();scene.gameplay->Server()=true;scene.storage->ownerMode.battle=false;scene.storage->GameAux()->GrowingRainBlockerType()=124;
    const auto sid=scene.storage->CreateAsset(scene.storage->GameAux()->RainBlockerType(),2);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);CHECK(raw[10]==147 && RegionRead(raw,0)==0x50e940U);raw[40]=0;raw[34]=1;bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x10); }catch (const std::logic_error& error) { std::cerr<<"ICE OWNER: "<<error.what()<<" type="<<unsigned(scene.storage->pool.Slot(sid)[10])<<" role="<<scene.storage->GameAux()->RainBlockerType()<<" depth="<<scene.storage->BaseDestroy().PreDepth()<<'/'<<scene.storage->BaseDestroy().PostDepth()<<'\n';rejected=std::string_view(error.what())=="영역 검사 금지 Native 방송"; }
    const auto value=scene.PlayerRead(1,0x7c);CHECK(value && value<scene.storage->pool.Capacity());if (!value || value>=scene.storage->pool.Capacity()) return;const Sid born{static_cast<std::uint16_t>(value)};
    CHECK(rejected && scene.storage->pool.Slot(born)[10]==124 && scene.storage->pool.Slot(born)[34]==1 && scene.PlayerRead(1,0x80)==value && scene.refreshes==1 && scene.gameplay->MapRevision()==1);
}

// 전체펜스본체구성은현재역할/Player/SP/난수를조회하거나초기화하지않는다.
TEST_CASE(ClientFence_LongBodiesShareCurrentStateWithoutWorld) {
    RegionScene scene;const auto random=scene.random->Game().State();scene.storage->GameAux()->FenceMarkerType()=70;scene.storage->GameAux()->FenceShieldType()=171;
    auto* neighbors=&scene.storage->FenceNeighbors();auto* line=&scene.storage->FenceLine();auto* markers=&scene.storage->FenceMarkers();auto* shields=&scene.storage->FenceShields();auto* clear=&scene.storage->FenceMarkerClear();
    auto* rebuild=&scene.storage->FenceRebuild();auto* post=&scene.storage->FencePostPop();auto* regular=&scene.storage->FenceRegular();auto* lifetime=&scene.storage->FenceLifetime();
    CHECK(&scene.storage->FenceNeighbors()==neighbors && &scene.storage->FenceLine()==line && &scene.storage->FenceMarkers()==markers && &scene.storage->FenceShields()==shields && &scene.storage->FenceMarkerClear()==clear);
    CHECK(&scene.storage->FenceRebuild()==rebuild && &scene.storage->FencePostPop()==post && &scene.storage->FenceRegular()==regular && &scene.storage->FenceLifetime()==lifetime);
    CHECK(scene.storage->GameAux()->FenceMarkerType()==70 && scene.storage->GameAux()->FenceShieldType()==171 && scene.players->Base()==0 && !scene.sp->Store().Initialized() && scene.random->Game().State()==random && !scene.storage->Borrowed());
}

// 펜스 사거리 사본을 쓰던 주문 반경도 같은 현재 TYPE+44의 절삭값을 읽는다.
TEST_CASE(ClientFence_SharedRangeFeedsExistingBombRadius) {
    RegionScene scene;const auto sid=scene.Fence();BombTargetState state;RawBombTargets body(scene.storage->pool,scene.storage->hash,scene.storage->types,state,{});
    scene.storage->types[136].range=12.75F;CHECK(body.DefaultRadius(sid)==12);scene.storage->types[136].range=-7.5F;CHECK(body.DefaultRadius(sid)==-7 && scene.players->Base()==0 && !scene.storage->Borrowed());
}

// 같은일반finder가현재TYPE사거리와방향별동맹을다음조회에즉시반영한다.
TEST_CASE(ClientFence_CurrentRangeAndDirectionalAllianceBeforePlayers) {
    RegionScene scene;const auto parent=scene.Fence(100,100,1),other=scene.Fence(100,90,2);scene.Index(other);auto& body=scene.storage->FenceNeighbors();scene.storage->types[136].range=20;
    CHECK(body.Neighbor(parent,0)==other && body.Neighbor(parent,0,true)==Sid{});scene.players->Alliances()[2*9+1]=1;
    CHECK(body.Neighbor(parent,0,true)==other && body.Connections(parent)==1 && body.WithinRange(parent,{100,100,100,90}));
    scene.storage->types[136].range=4;CHECK(body.Neighbor(parent,0)==Sid{} && !body.WithinRange(parent,{100,100,100,90}));
    scene.storage->types[136].range=20;scene.players->Alliances()[2*9+1]=0;scene.players->Alliances()[1*9+2]=1;
    CHECK(body.Neighbor(parent,0,true)==Sid{} && scene.players->Base()==0 && !scene.sp->Store().Initialized());
}

// Storage가Player를강하게역참조하지않으며반납뒤선택된동맹은명시실패한다.
TEST_CASE(ClientFence_CurrentPlayerLeaseHasNoStorageCycle) {
    RegionScene scene;const auto parent=scene.Fence(100,100,1),other=scene.Fence(100,90,2);scene.storage->types[136].range=20;const std::array candidates{other};
    auto& body=scene.storage->FenceNeighbors();std::weak_ptr<ClientPlayerRecords> weak=scene.players;scene.owner.reset();scene.players.reset();CHECK(weak.expired());bool rejected{};
    try { static_cast<void>(body.Select(parent,candidates,true)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="검사 실제 Player 반납 뒤 동맹 선택"; }
    CHECK(rejected && !scene.storage->Borrowed());
}

// 실제VT457990전처리와Damageable후처리가월드없이같은공통삭제깊이를끝낸다.
TEST_CASE(ClientFence_ActualPrePostParentsBeforeWorld) {
    RegionScene scene;const auto sid=scene.Fence(100,100,0,9);const auto free=scene.storage->pool.FreeCount();CHECK(scene.Destroy(sid));
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.players->Base()==0 && scene.traces==1);
}

// 실제네방향검색후원문의처음두이웃에만같은Host의event2를예약한다.
TEST_CASE(ClientFence_PreDestroySchedulesOnlyFirstTwoNeighbors) {
    RegionScene scene;scene.storage->types[136].range=20;const auto parent=scene.Fence(100,100,1,9);
    const std::array others{scene.Fence(100,90,2),scene.Fence(110,100,2),scene.Fence(100,110,2),scene.Fence(90,100,2)};
    for (const auto sid:others) scene.Index(sid); //실제원문순서0/2/4/6의적대이웃을같은물리에입력한다.
    CHECK(scene.Destroy(parent));auto& host=scene.storage->Processes().Host();auto* north=host.FindEvent(others[0],2);auto* east=host.FindEvent(others[1],2);
    CHECK(north && east && !host.FindEvent(others[2],2) && !host.FindEvent(others[3],2));if (!north || !east) return;
    CHECK(north->Count()==0 && east->Count()==0 && north->Payload()==0.1F && east->Payload()==0.1F && !north->Shared() && !east->Shared() && scene.players->Base()==0 && !scene.storage->Borrowed());
}

// 실제최초Pop가현재theme3의event0과event2를예약하고Host도같은본체로재진입한다.
TEST_CASE(ClientFence_NoWorldFirstPopAndActualRegularHost) {
    RegionScene scene(true);scene.storage->types[136].theme=3;const auto sid=scene.Fence(128,128,0,0x20);CHECK(scene.storage->PopAsset(sid,128,128,0x10)==RawPopResult::Registered);
    auto& host=scene.storage->Processes().Host();auto* animation=host.FindEvent(sid,0);auto* rebuilding=host.FindEvent(sid,2);CHECK(animation && rebuilding);if (!animation || !rebuilding) return;
    const auto frame=RegionRead(scene.storage->pool.Slot(sid),36);animation->RunFrame();CHECK(animation->Count()==1 && animation->Payload()==0.1F && rebuilding->Count()==0 && RegionRead(scene.storage->pool.Slot(sid),36)==frame);
    CHECK(RegionRead(scene.storage->pool.Slot(sid),42)==0 && RegionRead(scene.storage->pool.Slot(sid),46)==0 && scene.storage->PostPop().ActivationDepth()==0 && scene.postPopSelections==1 && scene.players->Base()==0);
}

// 다른사건은현재캐시/프레임을바꾸지않고같은일반부모의payload비트를보존한다.
TEST_CASE(ClientFence_UnknownEventPreservesPayloadAndCaches) {
    RegionScene scene;const auto sid=scene.Fence();auto raw=scene.storage->pool.AllocatedBytes(sid);RegionPut(raw,42,0x12345678);RegionPut(raw,46,0x87654321);RegionPut(raw,36,0xffffffffU);
    const auto result=scene.storage->FenceRegular().Handle(sid,0x76543210,0xffffffffU,std::bit_cast<float>(0x7fc12345U));
    CHECK(std::bit_cast<std::uint32_t>(result)==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==0xffffffffU && RegionRead(scene.storage->pool.Slot(sid),42)==0x12345678 && RegionRead(scene.storage->pool.Slot(sid),46)==0x87654321);
}

// 현재역할참조를유지한실제표식생성은WORD/QB/owner/Pop40과같은해시까지끝난다.
TEST_CASE(ClientFence_MarkerCreateUsesCurrentRoleAndActualPop) {
    RegionScene scene(true);scene.PlayerBlock();const auto parent=scene.Fence(100,100,1),other=scene.Fence(100,110,2);scene.storage->GameAux()->FenceMarkerType()=70;auto& body=scene.storage->FenceMarkers();scene.storage->GameAux()->FenceMarkerType()=90;
    const auto free=scene.storage->pool.FreeCount();body.Create(parent,other);const auto markers=scene.OfType(90);CHECK(markers.size()==4 && scene.storage->pool.FreeCount()==free-4);
    for (const auto sid:markers) { const auto raw=scene.storage->pool.Slot(sid);CHECK((RegionRead(raw,12)&0xffffU)==parent.value && ((RegionRead(raw,26)<<8)|raw[30])==other.value && raw[34]==1 && !(raw[11]&5U)); } //실제생성된각표식의원문끝점태그와등록상태다.
    CHECK(scene.storage->PostPop().ActivationDepth()==0 && !scene.storage->Borrowed());
}

// 현재포스필드역할/J첫프레임/서명있는SID의owner선택과실제Pop를확인한다.
TEST_CASE(ClientFence_ShieldCreateUsesCurrentRoleFrameAndOwner) {
    RegionScene scene(true);scene.PlayerBlock();scene.storage->GameAux()->FenceShieldType()=70;auto& body=scene.storage->FenceShields();scene.storage->GameAux()->FenceShieldType()=170;
    const auto free=scene.storage->pool.FreeCount();body.Create({100,100,100,110},2,3,-1,5);const auto shields=scene.OfType(170);CHECK(shields.size()==8 && scene.storage->pool.FreeCount()==free-8);
    const auto expected=static_cast<std::uint32_t>(scene.storage->frames[170].Run('J').first);
    for (const auto sid:shields) { const auto raw=scene.storage->pool.Slot(sid);CHECK(raw[34]==3 && RegionRead(raw,36)==expected && !(raw[11]&5U)); } //같은현재프레임표와가상owner의실제결과만읽는다.
    CHECK(scene.storage->PostPop().ActivationDepth()==0);
}

// 같은이웃이라도현재사거리밖에서는생성하지않고범위확장뒤표식/포스필드를실제로만든다.
TEST_CASE(ClientFence_RebuildUsesLiveRangeAndBothActualCreators) {
    RegionScene scene(true);scene.PlayerBlock();const auto parent=scene.Fence(100,100,1),other=scene.Fence(100,110,2);scene.Index(parent);scene.Index(other);scene.players->Alliances()[2*9+1]=1;
    scene.storage->types[136].range=6;const auto free=scene.storage->pool.FreeCount();scene.storage->FenceRebuild().Rebuild(parent);CHECK(scene.storage->pool.FreeCount()==free);
    scene.storage->types[136].range=20;scene.storage->FenceRebuild().Rebuild(parent);CHECK(scene.OfType(90).size()==4 && scene.OfType(170).size()==8 && scene.storage->pool.FreeCount()==free-12);
}

// 현재역할이아닐때는남기고복원뒤실제생성표식의태그와가상삭제를사용한다.
TEST_CASE(ClientFence_MarkerClearReadsCurrentRoleAndActualTags) {
    RegionScene scene(true);scene.PlayerBlock();const auto parent=scene.Fence(100,100,1),other=scene.Fence(100,110,2);const auto free=scene.storage->pool.FreeCount();scene.storage->FenceMarkers().Create(parent,other);CHECK(scene.OfType(90).size()==4);
    scene.storage->GameAux()->FenceMarkerType()=170;scene.storage->FenceMarkerClear().Clear(parent,other);CHECK(scene.OfType(90).size()==4);
    scene.storage->GameAux()->FenceMarkerType()=90;scene.storage->FenceMarkerClear().Clear(parent,other);CHECK(scene.OfType(90).empty() && scene.storage->pool.FreeCount()==free && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 현재역할과실제J방향프레임으로만생성된연결선을찾아같은물리에서삭제한다.
TEST_CASE(ClientFence_LineClearReadsCurrentRoleAndFrameDirection) {
    RegionScene scene(true);scene.PlayerBlock();const auto free=scene.storage->pool.FreeCount();scene.storage->FenceShields().Create({100,100,100,110},1,1,5,6);CHECK(scene.OfType(170).size()==8);
    scene.storage->GameAux()->FenceShieldType()=90;scene.storage->FenceLine().Clear({100,100,100,110});CHECK(scene.OfType(170).size()==8);
    scene.storage->GameAux()->FenceShieldType()=170;scene.storage->FenceLine().Clear({100,100,100,110});CHECK(scene.OfType(170).empty() && scene.storage->pool.FreeCount()==free && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}
