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
#include "o/RawDiscRegular.h" // 원문의 IDIV/감김은 새 실제 PE 경계 자료와 직접 대조한다.
#include "client/GumpFormBinding.h" // 실제 등록표 알림과 메뉴 Form 연결을 같은 Native 본체로 검사한다.
#include "client/RawButtonControls.h" // 실제 Gump 등록표·트리·지연 삭제 큐를 소유한다.
#include "client/RawMenuControls.h" // 실제 Floating SP의 Reference/등록ID/Form/가상 Kill을 사용한다.
#include "client/BitmapFont.h" // UI 통합은 보호 자산과 실제 글꼴 저장소를 사용한다.
#include <filesystem> // 새 경계 fixture를 이 검사 파일의 실제 위치에서 읽는다.
#include <bit>
#include <optional>
#include <iostream> //최초실패의실제삭제깊이/현재필드를보존한다.
#include <limits> // Native 조회가 실제로 소비한 NaN 좌표의 실패 위치를 확인한다.

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
    std::function<void()> onClear; // 실제 공통 Pre의 선택 해제 뒤 owner/base 변경 순서를 관찰한다.
    std::function<void()> onTrace; // 실제 공통 삭제 로그 뒤 현재 VT 재조회와 부분 실패 위치를 관찰한다.
    std::unique_ptr<RawAiWallet> wallet; //기존실제현재Player지갑본문이며비영AI선택은금지한다.
    RawAiBrainMemory aiMemory{0x26000000U,RegionForbidden<void,std::span<std::uint8_t>>("영역 검사 비영 AI 할당 금지")}; //빈실제메모리owner이며가짜brain을할당하지않는다.
    double now{};std::uint32_t outpostType{126}; //NULL brain통지가읽지않는현재시각/원문TYPE입력이다.
    std::uint32_t spRefreshes{},walletCalls{},aiNotices{},lostSounds{},postPopSelections{}; //실제공통환불/통지/표시장치의선택수를관찰한다.
    // 최초 결합은 원문 게임/Player 초기화·패킷·월드 대여를 실행하지 않는다.
    explicit RegionScene(bool observePostPop=false) {
        storage->BindTransmissionDevices({RegionForbidden<void,std::span<std::uint8_t>>("영역 검사 금지 Native 방송"),RegionForbidden<bool,std::int32_t>(),RegionForbidden<void,std::int32_t,std::span<std::uint8_t>>()});
        storage->BindFenceAlliances([weak=std::weak_ptr<ClientPlayerRecords>(players)](auto row,auto column) { const auto current=weak.lock();if (!current) throw std::logic_error("검사 실제 Player 반납 뒤 동맹 선택");return current->Alliances().at(row*9+column); }); //게임/Player초기화없이실제동맹배열만선택호출동안pin한다.
        storage->BindSerializationGlobals(std::make_shared<FortSerializationGlobals>());
        storage->BindDisplayDiagnostics(detail,[](auto expression,auto file,auto line) { throw std::logic_error("유효 영역 입력에서 원문 단언 발생: "+std::string(file)+":"+std::to_string(line)+" "+std::string(expression)); }); // 최초 실패의 실제 원문 위치/표현을 숨기지 않는다.
        storage->BindGraphTrace([this](auto,std::string_view) { ++traces;if (onTrace) onTrace(); });storage->EnablePatchDestroyDiagnostics();
        RawSurfaceDisplayBindings display;display.view=[] { return SquidDisplayView{0,0,65536,{0,0,1024,768}}; };display.suppressed=[] { return true; };display.invalidate=RegionForbidden<void,SquidDisplayRect,std::uint32_t>();storage->BindDisplayDevices(std::move(display));
        wallet=std::make_unique<RawAiWallet>(RegionForbidden<AiWalletMemoryAccess>("영역 검사 비영 AI 지갑 대여 금지"),[this]()->std::span<const std::uint8_t> { return players->Records(); },AiWalletReads{[this] { return storage->ownerMode.challenge; },RegionForbidden<bool>()});
        storage->BindRewardDevices({std::shared_ptr<ScrambledSpStore>(sp,&sp->Store()),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->world.editor),[this] { ++spRefreshes; },[this](auto player,auto amount) { ++walletCalls;wallet->Credit(player,amount); }});
        storage->BindPostPopDevices({[this](auto player,auto sid,auto added) { ++aiNotices;if (storage->ownerMode.challenge || std::bit_cast<std::int32_t>(player)<1) return;RawAiObjectNotice body(aiMemory,storage->pool,storage->types,players->Records(),now,outpostType,[this] { return storage->ownerMode.challenge; },{});body.Notice(player,sid,added); },[current=gameplay] { return current->player.battleOwner; },[this,observePostPop] { if (!observePostPop) RegionForbidden<void>("영역 검사 금지 PostPop UI")();CHECK(gameplay->selection.mouseType==0);++postPopSelections; }});
        storage->BindDeletionDevices({[](auto,auto,auto) {},[this](Sid sid) { ++lostSounds;CHECK(storage->pool.Slot(sid)[34]==gameplay->player.battleOwner); }});storage->BindProcessDevices({[this] { return gameplay->selection.selected; },[this] { ++clears;gameplay->selection.selected=0;if (onClear) onClear(); }});
        storage->BindPlayerRecords(players); // 본문은 각 주소 소비 때만 현재 실제 Player를 pin한다.
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

// Player 장기 장치는 배열·동맹·SP·난수를 조기 초기화하지 않고 한 번만 결합한다.
TEST_CASE(ClientRegistry_PlayerBindingDoesNotInitializeRecords) {
    RegionScene scene;const auto random=scene.random->Game().State();bool rejected{};
    try { scene.storage->BindPlayerRecords(scene.players); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && scene.players->Base()==0 && !scene.players->Initialized() && !scene.sp->Store().Initialized() && scene.random->Game().State()==random && !scene.storage->Borrowed());
}

// 실제 Player base가 없으면 원문의 첫 기록 접근 위치에서 실패하고 앞선 dead/로그/깊이를 보존한다.
TEST_CASE(ClientRegistry_PlayerWithoutBasePreservesDeletePrefix) {
    RegionScene scene;const auto sid=scene.Asset(0x50ee08U);const auto free=scene.storage->pool.FreeCount();bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x10U); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && scene.storage->pool.FreeCount()==free && scene.traces==1 && (scene.storage->pool.Slot(sid)[11]&3U)==2U && scene.storage->BaseDestroy().PreDepth()==1 && scene.storage->BaseDestroy().PostDepth()==0 && scene.players->Base()==0);
}

// 자신의 현재 Player+9c만 지우고 같은 flags DWORD의 다른 비트와 다른 행은 보존한다.
TEST_CASE(ClientRegistry_PlayerClearsMatchingSidAndUpdatesExactBits) {
    RegionScene scene;scene.PlayerBlock();const auto sid=scene.Asset(0x50ee08U);scene.storage->pool.AllocatedBytes(sid)[34]=1;
    scene.PlayerWrite(1,0x9c,sid.value);scene.PlayerWrite(1,0x50,0xa5a50003U);scene.PlayerWrite(2,0x9c,0x12345678U);scene.PlayerWrite(2,0x50,0x76543210U);
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10U));
    CHECK(scene.PlayerRead(1,0x9c)==0 && scene.PlayerRead(1,0x50)==0xa5a50102U && scene.PlayerRead(2,0x9c)==0x12345678U && scene.PlayerRead(2,0x50)==0x76543210U);
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 다른 Player SID를 자신으로 오인하지 않으며 owner0도 실제 배열의 원문 행을 사용한다.
TEST_CASE(ClientRegistry_PlayerKeepsOtherSidAndSupportsOwnerZero) {
    RegionScene scene;scene.PlayerBlock();const auto sid=scene.Asset(0x50ee08U);scene.storage->pool.AllocatedBytes(sid)[34]=0;
    scene.PlayerWrite(0,0x9c,0x10000U|sid.value);scene.PlayerWrite(0,0x50,1);CHECK(scene.storage->DestroyAsset(sid,0x10U));
    CHECK(scene.PlayerRead(0,0x9c)==(0x10000U|sid.value) && scene.PlayerRead(0,0x50)==0x100U);
}

// 공통 Pre가 Player 배열과 raw owner를 바꿔도 이후 두 flags 쓰기는 새 주소에만 적용한다.
TEST_CASE(ClientRegistry_PlayerRereadsBaseAndOwnerAfterCommonPre) {
    RegionScene scene;const auto oldBase=scene.PlayerBlock();const auto sid=scene.Asset(0x50ee08U);scene.storage->pool.AllocatedBytes(sid)[34]=1;scene.PlayerWrite(1,0x9c,sid.value);scene.gameplay->selection.selected=sid.value;
    scene.onClear=[&] { CHECK(scene.PlayerRead(1,0x9c)==0);scene.PlayerBlock();scene.storage->pool.AllocatedBytes(sid)[34]=2;scene.PlayerWrite(1,0x50,0x77U);scene.PlayerWrite(2,0x9c,0x5678U);scene.PlayerWrite(2,0x50,0x80000001U); };
    CHECK(scene.storage->DestroyAsset(sid,0x10U));
    CHECK(scene.players->Base()!=oldBase && scene.clears==1 && scene.PlayerRead(1,0x50)==0x77U && scene.PlayerRead(2,0x9c)==0x5678U && scene.PlayerRead(2,0x50)==0x80000100U && !scene.storage->Borrowed());
}

// 장기 Storage의 약한 기록 장치는 실제 owner를 반납한 뒤 가짜 배열이나 재초기화를 만들지 않는다.
TEST_CASE(ClientRegistry_PlayerOwnerLifetimeIsWeak) {
    RegionScene scene;const auto sid=scene.Asset(0x50ee08U);const auto weak=std::weak_ptr<ClientPlayerRecords>(scene.players);scene.owner.reset();scene.players.reset();CHECK(weak.expired());bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x10U); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="장기 Player 삭제의 실제 현재 기록 owner 미연결"; }
    CHECK(rejected && scene.storage->BaseDestroy().PreDepth()==1 && scene.traces==1);
}

// 두 고유 Post는 같은 장기 목록에서 중복 DWORD만 압축하고 비활성 꼬리와 높은 SID 비트는 보존한다.
TEST_CASE(ClientRegistry_ResidenceAndDudeRemoveExactDuplicates) {
    for (const auto table:{0x50f540U,0x508cc8U}) { // 실제 두 VT의 동일 제거 본체를 각각 독립 물리에서 실행한다.
        RegionScene scene;const auto sid=scene.Asset(table);scene.storage->pool.AllocatedBytes(sid)[40]=0;auto& list=table==0x50f540U ? scene.storage->sidLists.residences:scene.storage->sidLists.dudes;
        list.count=5;list.entries[0]=sid.value;list.entries[1]=0x10000U|sid.value;list.entries[2]=sid.value;list.entries[3]=7;list.entries[4]=sid.value;list.entries[5]=0xdeadbeefU;
        const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x200810U)); // 원문의 환급/Graph 정리 억제와 전송 억제로 실제 목록/부모 수명만 확인한다.
        CHECK(list.count==2 && list.entries[0]==(0x10000U|sid.value) && list.entries[1]==7 && list.entries[2]==sid.value && list.entries[3]==7 && list.entries[4]==sid.value && list.entries[5]==0xdeadbeefU);
        CHECK(scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// extra9는 목록 데이터와 count가 비정상이어도 원문처럼 읽기 전에 제거 접두를 생략한다.
TEST_CASE(ClientRegistry_SkippedPostDoesNotReadInvalidCount) {
    for (const auto table:{0x50f540U,0x508cc8U}) { // 두 고유 Post의 같은 자연 관문을 확인한다.
        RegionScene scene;const auto sid=scene.Asset(table);auto& list=table==0x50f540U ? scene.storage->sidLists.residences:scene.storage->sidLists.dudes;list.count=0xffffffffU;list.entries[0]=sid.value;
        CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(list.count==0xffffffffU && list.entries[0]==sid.value && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 원문0040ea00의 signed count가 음수이면 일반 객체도 배열을 읽지 않고 해당 DWORD를 보존한다.
TEST_CASE(ClientRegistry_NegativeSignedCountSurvivesActualPost) {
    for (const auto table:{0x50f540U,0x508cc8U}) { // Native는 독립 typed의 Items 사전 검사를 삽입하지 않는다.
        RegionScene scene;const auto sid=scene.Asset(table);scene.storage->pool.AllocatedBytes(sid)[40]=0;auto& list=table==0x50f540U ? scene.storage->sidLists.residences:scene.storage->sidLists.dudes;list.count=0x80000000U;list.entries[0]=sid.value;
        CHECK(scene.storage->DestroyAsset(sid,0x200810U));CHECK(list.count==0x80000000U && list.entries[0]==sid.value && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 목록은 생성 때 snapshot하지 않고 앞선 공통 Pre/UI가 바꾼 실제 현재 데이터에서만 제거한다.
TEST_CASE(ClientRegistry_PostReadsListChangedByEarlierCommonPre) {
    RegionScene scene;const auto sid=scene.Asset(0x508cc8U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.gameplay->selection.selected=sid.value;auto& list=scene.storage->sidLists.dudes;list.count=1;list.entries[0]=123;
    scene.onClear=[&] { list.count=3;list.entries[0]=9;list.entries[1]=sid.value;list.entries[2]=11; };
    CHECK(scene.storage->DestroyAsset(sid,0x200810U));CHECK(scene.clears==1 && list.count==2 && list.entries[0]==9 && list.entries[1]==11 && list.entries[2]==11);
}

// 공통 Pre가 extra를 바꾼 뒤 고유 Post는 그때의 새 관문을 읽어 목록을 남긴다.
TEST_CASE(ClientRegistry_PostRereadsExtraAfterEarlierCommonPre) {
    RegionScene scene;const auto sid=scene.Asset(0x50f540U);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.gameplay->selection.selected=sid.value;auto& list=scene.storage->sidLists.residences;list.count=1;list.entries[0]=sid.value;
    scene.onClear=[&] { scene.storage->pool.AllocatedBytes(sid)[40]|=9U; };
    CHECK(scene.storage->DestroyAsset(sid,0x200810U));CHECK(scene.clears==1 && list.count==1 && list.entries[0]==sid.value && scene.storage->BaseDestroy().PostDepth()==0);
}

// 장기 삭제/참여 본체는 현재 목록/원소유자/역할/SP/Player/난수를 초기화하지 않는다.
TEST_CASE(ClientDais_LongBodiesKeepActualOwners) {
    RegionScene scene;const auto random=scene.random->Game().State(),free=scene.storage->pool.FreeCount();scene.storage->priestOwnerState[3]=77;scene.storage->sidLists.priests.count=0;
    auto* body=&scene.storage->DaisDeletion();auto* participation=&scene.storage->PriestParticipation();CHECK(&scene.storage->DaisDeletion()==body && &scene.storage->PriestParticipation()==participation && &body->Pool()==&scene.storage->pool);
    CHECK(scene.storage->priestOwnerState[3]==77 && scene.storage->pool.FreeCount()==free && scene.random->Game().State()==random && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 실제 두 VT의 네 고유 Pre/Post가 World 없이 같은 전체 부모와 삭제 깊이를 끝낸다.
TEST_CASE(ClientDais_BothActualParentsBeforeWorld) {
    for (const auto table:{0x5076b8U,0x507950U}) { // 실제 제단과 다이스를 별도 현재 물리에서 확인한다.
        RegionScene scene;const auto sid=scene.Asset(table);scene.Position(sid);const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10U));
        CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && !scene.storage->Borrowed());
    }
}

// 전체 DWORD가 일치하는 primary와 mirror만 지우고 높은 비트가 다른 SID 쌍은 유지한다.
TEST_CASE(ClientDais_PostClearsOnlyMatchingDwords) {
    RegionScene scene;scene.PlayerBlock();const auto sid=scene.Asset(0x507950U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[34]=1;
    scene.PlayerWrite(1,0x94,sid.value);scene.PlayerWrite(1,0x98,0xaaaaaaaaU);scene.PlayerWrite(1,0x8c,0x10000U|sid.value);scene.PlayerWrite(1,0x90,0xbbbbbbbbU);
    CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(scene.PlayerRead(1,0x94)==0 && scene.PlayerRead(1,0x98)==0 && scene.PlayerRead(1,0x8c)==(0x10000U|sid.value) && scene.PlayerRead(1,0x90)==0xbbbbbbbbU);
}

// 서로 다른 SID 두 쌍과 mirror는 삭제 대상이 아니며 다른 Player 행도 그대로다.
TEST_CASE(ClientDais_PostKeepsUnmatchedPairs) {
    RegionScene scene;scene.PlayerBlock();const auto sid=scene.Asset(0x507950U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[34]=1;
    scene.PlayerWrite(1,0x94,7);scene.PlayerWrite(1,0x98,8);scene.PlayerWrite(1,0x8c,9);scene.PlayerWrite(1,0x90,10);scene.PlayerWrite(2,0x94,sid.value);
    CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(scene.PlayerRead(1,0x94)==7 && scene.PlayerRead(1,0x98)==8 && scene.PlayerRead(1,0x8c)==9 && scene.PlayerRead(1,0x90)==10 && scene.PlayerRead(2,0x94)==sid.value);
}

// owner0 관문은 현재 Player base가 아직 없어도 기록 owner나 배열을 읽지 않는다.
TEST_CASE(ClientDais_PostOwnerZeroNeedsNoRecords) {
    RegionScene scene;const auto sid=scene.Asset(0x507950U);scene.Position(sid);CHECK(scene.storage->pool.Slot(sid)[34]==0 && scene.players->Base()==0);
    CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(scene.players->Base()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// owner1의 미초기화 base는 Post의 실제 소비 위치에서 실패하며 완료된 Pre를 되돌리지 않는다.
TEST_CASE(ClientDais_PostWithoutRecordsKeepsEarlierEffects) {
    RegionScene scene;const auto sid=scene.Asset(0x507950U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[34]=1;const auto free=scene.storage->pool.FreeCount();bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x10U); }catch (const std::out_of_range& error) { rejected=std::string_view(error.what())=="다이스 삭제의 현재 Player 물리 범위"; }
    CHECK(rejected && scene.players->Base()==0 && scene.storage->pool.FreeCount()==free && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==1 && (scene.storage->pool.Slot(sid)[11]&2U));
}

// 공통 Pre가 실제 Player 배열/owner를 바꿔도 Post는 그때의 새 base/행에서 두 SID 쌍을 정리한다.
TEST_CASE(ClientDais_PostRereadsPlayersAfterCommonPre) {
    RegionScene scene;const auto old=scene.PlayerBlock();const auto sid=scene.Asset(0x507950U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[34]=1;scene.gameplay->selection.selected=sid.value;
    scene.onClear=[&] { scene.PlayerBlock();scene.storage->pool.AllocatedBytes(sid)[34]=2;scene.PlayerWrite(1,0x94,99);scene.PlayerWrite(2,0x94,sid.value);scene.PlayerWrite(2,0x98,1);scene.PlayerWrite(2,0x8c,sid.value);scene.PlayerWrite(2,0x90,2); };
    CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(scene.players->Base()!=old && scene.clears==1 && scene.PlayerRead(1,0x94)==99 && scene.PlayerRead(2,0x94)==0 && scene.PlayerRead(2,0x98)==0 && scene.PlayerRead(2,0x8c)==0 && scene.PlayerRead(2,0x90)==0);
}

// 제단은 flags400000이 없을 때만 살아 있는 현재 다이스를 같은 flags로 실제 가상 삭제한다.
TEST_CASE(ClientDais_AltarDeletesDaisUnlessSacrificeFlag) {
    for (const auto flags:{0x10U,0x400010U}) { // 단순 삭제/희생 삭제 구분만 확인하고 희생 동작이나 포획을 실행하지 않는다.
        RegionScene scene;const auto altar=scene.Asset(0x5076b8U),dais=scene.Asset(0x507950U);scene.Position(altar);scene.Position(dais);scene.storage->pool.AllocatedBytes(dais)[40]=1;scene.Index(dais);
        CHECK(scene.storage->DestroyAsset(altar,flags));CHECK(bool(scene.storage->pool.Slot(dais)[11]&1U)==!(flags&0x400000U));CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 제단의 extra9는 룬 정리를 생략하지 않으며 소수 우하단은 원문처럼 거의 올림한다.
TEST_CASE(ClientDais_AltarExtraKeepsFractionalRuneCleanup) {
    RegionScene scene;const auto altar=scene.Asset(0x5076b8U),rune=scene.Asset(0x5075d8U);scene.Position(altar,100.25F,100.25F);scene.Position(rune,101,101);scene.storage->pool.AllocatedBytes(rune)[40]=1;scene.Index(rune);
    CHECK(scene.storage->DestroyAsset(altar,0x10U));CHECK((scene.storage->pool.Slot(rune)[11]&1U) && scene.traces==2 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 비boss는 서버 SID 룬을 남기고 실제 client SID의 추상 룬만 지운다. 송신 장치는 금지 상태다.
TEST_CASE(ClientDais_AltarAuthoritySkipsServerRune) {
    RegionScene scene;const auto altar=scene.Asset(0x5076b8U),local=scene.Asset(0x5075d8U),remote=scene.storage->CreateAsset(scene.gameplay->GameAux()->RuneType(),0);scene.Position(altar);scene.Position(local);scene.Position(remote);
    scene.storage->pool.AllocatedBytes(local)[40]=1;scene.storage->pool.AllocatedBytes(remote)[40]=1;scene.Index(local);scene.Index(remote);scene.gameplay->Authority()->boss=false;
    CHECK(scene.storage->DestroyAsset(altar,0x10U));CHECK((scene.storage->pool.Slot(local)[11]&1U) && !(scene.storage->pool.Slot(remote)[11]&1U) && scene.traces==2);
}

// 앞선 다이스 삭제의 실제 선택 해제가 역할/후보 TYPE를 바꾸면 뒤 룬 정리는 새 현재 값을 쓴다.
TEST_CASE(ClientDais_AltarReadsRoleAfterNestedDaisDelete) {
    RegionScene scene;scene.PlayerBlock();const auto altar=scene.Asset(0x5076b8U),dais=scene.Asset(0x507950U),rune=scene.Asset(0x5075d8U);scene.Position(altar);scene.Position(dais);scene.Position(rune);
    scene.storage->pool.AllocatedBytes(dais)[40]=1;scene.storage->pool.AllocatedBytes(dais)[34]=1;scene.storage->pool.AllocatedBytes(rune)[40]=1;scene.Index(rune);scene.Index(dais);scene.gameplay->selection.selected=dais.value;
    scene.onClear=[&] { scene.gameplay->GameAux()->RuneType()=70;scene.storage->pool.AllocatedBytes(rune)[10]=70; };
    CHECK(scene.storage->DestroyAsset(altar,0x10U));CHECK(scene.clears==1 && scene.traces==3 && (scene.storage->pool.Slot(dais)[11]&1U) && (scene.storage->pool.Slot(rune)[11]&1U));
}

// 일반 다이스의1..8 갱신은 같은 장기 사제 목록과9DWORD 상태를 사용하며0번은 바꾸지 않는다.
TEST_CASE(ClientDais_OrdinaryPreRefreshesSameLongParticipation) {
    RegionScene scene;const auto dais=scene.Asset(0x507950U),priest=scene.Asset(0x50f210U);scene.Position(dais);scene.Position(priest);scene.storage->pool.AllocatedBytes(dais)[40]=0;scene.storage->pool.AllocatedBytes(priest)[34]=1;
    scene.storage->sidLists.priests.count=1;scene.storage->sidLists.priests.entries[0]=priest.value;scene.storage->priestOwnerState.fill(99);
    CHECK(scene.storage->DestroyAsset(dais,0x810U));CHECK(scene.storage->priestOwnerState[0]==99 && scene.storage->priestOwnerState[1]==0 && scene.storage->PriestParticipation().Find(1)==priest.value);
    for (std::uint32_t owner=2;owner<=8;++owner) CHECK(scene.storage->priestOwnerState[owner]==2); // 원문의 현재 owner별 조회 결과를 확인한다.
}

// 강한 같은 owner 사제의 첫604를 실제 Host에서 제거한 뒤 현재 E 프레임을 지정한다.
TEST_CASE(ClientDais_PreStopsActualPriest604) {
    RegionScene scene;scene.PlayerBlock();const auto dais=scene.Asset(0x507950U),priest=scene.Asset(0x50f210U);scene.Position(dais);scene.Position(priest);auto d=scene.storage->pool.AllocatedBytes(dais),p=scene.storage->pool.AllocatedBytes(priest);d[40]=0;d[34]=1;p[40]=1;p[34]=1;
    RegionPut(p,26,static_cast<std::uint32_t>(scene.storage->Reward().MaxHitPoints(priest)));RegionPut(p,36,static_cast<std::uint32_t>(scene.storage->frames[p[10]].Run('K').first));scene.storage->Frame().StoreLevel(priest);scene.Index(priest);auto& host=scene.storage->Processes().Host();CHECK(host.AddRegular(priest,604,0.1F)!=nullptr);const auto frame=scene.storage->frames[p[10]].Run('E').first; // 수동 해시 입력도 실제 K프레임의 저장 단계와 일치시켜 불필요한 void 재Pop/self-next 입력을 만들지 않는다.
    CHECK(scene.storage->DestroyAsset(dais,0x810U));CHECK(host.FindEvent(priest,604)==nullptr && RegionRead(scene.storage->pool.Slot(priest),36)==static_cast<std::uint32_t>(frame) && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제 Form type BYTE와 전체 Regular 역할 DWORD가 다르면24여도 희생 완료를 선택하지 않는다.
TEST_CASE(ClientDais_FinishFormComparesFullCurrentRoles) {
    RegionScene scene;const auto dais=scene.Asset(0x507950U),altar=scene.Asset(0x5076b8U);scene.Position(dais);scene.Position(altar);scene.storage->pool.AllocatedBytes(dais)[40]=0;scene.storage->pool.AllocatedBytes(altar)[40]=1;scene.Index(dais);scene.Index(altar);
    CHECK(scene.storage->Processes().Host().AddRegular(altar,24,0.1F)!=nullptr);scene.gameplay->GameAux()->RegularProcessType()+=256;scene.gameplay->GameAux()->SharedRegularProcessType()+=256;
    CHECK(scene.storage->DestroyAsset(dais,0x400810U));CHECK((scene.storage->pool.Slot(altar)[11]&1U) && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 장기 삭제의 Player 연결은 weak이며 owner 반납 뒤 Post에서 가짜 배열을 만들지 않는다.
TEST_CASE(ClientDais_CurrentPlayerOwnerIsWeak) {
    RegionScene scene;scene.PlayerBlock();const auto sid=scene.Asset(0x507950U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[34]=1;static_cast<void>(scene.storage->DaisDeletion());const auto weak=std::weak_ptr<ClientPlayerRecords>(scene.players);scene.owner.reset();scene.players.reset();CHECK(weak.expired());bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x10U); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="장기 다이스 삭제의 현재 Player owner 미연결"; }
    CHECK(rejected && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==1);
}

// 장기 사제 본체 조립은 목록/역할/난수/SP/Player를 초기화하거나 World를 대여하지 않는다.
TEST_CASE(ClientPriest_LongBodiesKeepActualOwners) {
    RegionScene scene;const auto random=scene.random->Game().State();auto& list=scene.storage->sidLists.priests;list.count=1;list.entries[0]=123;scene.gameplay->GameAux()->PriestForcefieldType()=0x1234U;
    const auto free=scene.storage->pool.FreeCount();auto* pre=&scene.storage->PriestPre();auto* lookup=&scene.storage->PriestForcefield();
    CHECK(&pre->Pool()==&scene.storage->pool && &lookup->Pool()==&scene.storage->pool && &scene.storage->PriestPre()==pre && &scene.storage->PriestForcefield()==lookup);
    CHECK(list.count==1 && list.entries[0]==123 && lookup->Type()==0x1234U && scene.storage->pool.FreeCount()==free && scene.random->Game().State()==random && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 실제 VT4919b0은 같은16칸 목록에서 전체 DWORD 일치만 압축한 뒤 Carrier/공통 부모로 끝난다.
TEST_CASE(ClientPriest_ActualDeleteCompressesLongList) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[40]=0;auto& list=scene.storage->sidLists.priests;
    list.count=5;list.entries[0]=sid.value;list.entries[1]=0x10000U|sid.value;list.entries[2]=sid.value;list.entries[3]=7;list.entries[4]=sid.value;list.entries[5]=0xdeadbeefU;
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x810U)); // 실제 해방/Graph 억제 인자와 client SID로 파생 생성·송신 없이 같은 부모를 확인한다.
    CHECK(list.count==2 && list.entries[0]==(0x10000U|sid.value) && list.entries[1]==7 && list.entries[2]==sid.value && list.entries[3]==7 && list.entries[4]==sid.value && list.entries[5]==0xdeadbeefU);
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.traces==1);
}

// 추상/매장 관문은 손상 count와 NaN 좌표보다 먼저 적용하고 실제 Carrier 부모는 호출한다.
TEST_CASE(ClientPriest_SkippedPrefixReadsNeitherListNorCoordinate) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid,std::numeric_limits<float>::quiet_NaN());auto& list=scene.storage->sidLists.priests;list.count=0xffffffffU;list.entries[0]=sid.value;
    CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(list.count==0xffffffffU && list.entries[0]==sid.value && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 조회/가상 분배는 raw TYPE/상태를 사제 번호로 제한하지 않고 현재 VT만 분배에 사용한다.
TEST_CASE(ClientPriest_NativeLookupDoesNotPreflightSource) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[10]=70;raw[11]=9;
    CHECK(scene.storage->PriestPre().Handles(sid));CHECK(scene.storage->PriestForcefield().Find(sid)==Sid{});
    RegionPut(raw,0,0x12345678U);CHECK(!scene.storage->PriestPre().Handles(sid));CHECK(scene.storage->PriestForcefield().Find(sid)==Sid{});
}

// 역할 비교는 BYTE 절삭 없이 현재 DWORD를 매 조회에서 읽고 후보/부모의 현재 owner를 비교한다.
TEST_CASE(ClientPriest_LookupUsesCurrentFullRoleAndOwner) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U),field=scene.Asset(0x507770U);scene.Position(sid,100.75F,100.9F);scene.Position(field);auto source=scene.storage->pool.AllocatedBytes(sid),candidate=scene.storage->pool.AllocatedBytes(field);
    candidate[40]=1;source[34]=candidate[34]=3;scene.storage->hash.Bucket(1,100,100)=field.value;auto& lookup=scene.storage->PriestForcefield();
    CHECK(lookup.Find(sid)==field);scene.gameplay->GameAux()->PriestForcefieldType()=0x100U+candidate[10];CHECK(lookup.Find(sid)==Sid{});
    scene.gameplay->GameAux()->PriestForcefieldType()=candidate[10];source[34]=4;CHECK(lookup.Find(sid)==Sid{});candidate[34]=4;CHECK(lookup.Find(sid)==field);
}

// Native 조회기를 먼저 구성해도 나중에 바뀐 실제 TYPE 발자국을 현재 finder가 읽는다.
TEST_CASE(ClientPriest_LookupBorrowsCurrentTypeFootprint) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U),field=scene.Asset(0x507770U);scene.Position(sid);scene.Position(field,101,100);scene.storage->pool.AllocatedBytes(field)[40]=1;
    scene.storage->hash.Bucket(1,101,100)=field.value;auto& type=scene.storage->types[scene.storage->pool.Slot(field)[10]];type.footX=1;type.footY=1;auto& lookup=scene.storage->PriestForcefield();
    CHECK(lookup.Find(sid)==Sid{});type.footX=2;CHECK(lookup.Find(sid)==field);type.footX=1;CHECK(lookup.Find(sid)==Sid{});
}

// 후보 free/dead/void/contained는 추가로 걸러내지 않고 원문 finder의 extra8만 생략한다.
TEST_CASE(ClientPriest_LookupKeepsOriginalCandidateFilters) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U),field=scene.Asset(0x507770U);scene.Position(sid);scene.Position(field);auto candidate=scene.storage->pool.AllocatedBytes(field);candidate[40]=1;scene.storage->hash.Bucket(1,100,100)=field.value;
    for (const auto state:{1,2,4,8}) { candidate[11]=static_cast<std::uint8_t>(state);CHECK(scene.storage->PriestForcefield().Find(sid)==field); } // 이미 확보한 물리 view에서 상태 입력만 바꾸며 재할당하지 않는다.
    candidate[40]=8;CHECK(scene.storage->PriestForcefield().Find(sid)==Sid{});
}

// 실제 보호막 가상 destroy(flags0)가 끝난 뒤에만 같은 사제의 Carrier/공통 Pre가 선택을 해제한다.
TEST_CASE(ClientPriest_ActualForcefieldDeletePrecedesCarrier) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U),field=scene.Asset(0x507770U);scene.Position(sid);scene.Position(field);scene.storage->pool.AllocatedBytes(sid)[40]=0;scene.storage->pool.AllocatedBytes(field)[40]=1;
    auto& list=scene.storage->sidLists.priests;list.count=1;list.entries[0]=sid.value;scene.storage->hash.Bucket(1,100,100)=field.value;scene.gameplay->selection.selected=sid.value;
    scene.onClear=[&] { CHECK(list.count==0 && (scene.storage->pool.Slot(field)[11]&1U) && scene.traces==2); };
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x810U));
    CHECK(scene.clears==1 && scene.traces==2 && scene.storage->pool.FreeCount()==free+2 && list.count==0 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// Native signed count는 음수를 그대로 보존한 뒤에도 실제 lookup을 실행한다.
TEST_CASE(ClientPriest_NegativeCountStillReachesLookup) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid,std::numeric_limits<float>::quiet_NaN());scene.storage->pool.AllocatedBytes(sid)[40]=0;auto& list=scene.storage->sidLists.priests;list.count=0x80000000U;list.entries[0]=sid.value;bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x810U); }catch (const std::out_of_range& error) { rejected=std::string_view(error.what())=="사제 보호막 조회 좌표 절삭 범위 오류"; }
    CHECK(rejected && list.count==0x80000000U && list.entries[0]==sid.value && scene.storage->BaseDestroy().PreDepth()==1 && scene.traces==1);
}

// lookup 실패 앞에 완료된 목록 압축은 보존하고 Carrier/공통 깊이는 아직 끝내지 않는다.
TEST_CASE(ClientPriest_LookupFailurePreservesEarlierCompression) {
    RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid,std::numeric_limits<float>::quiet_NaN());scene.storage->pool.AllocatedBytes(sid)[40]=0;auto& list=scene.storage->sidLists.priests;list.count=3;list.entries[0]=sid.value;list.entries[1]=77;list.entries[2]=sid.value;bool rejected{};
    try { scene.storage->DestroyAsset(sid,0x810U); }catch (const std::out_of_range& error) { rejected=std::string_view(error.what())=="사제 보호막 조회 좌표 절삭 범위 오류"; }
    CHECK(rejected && list.count==1 && list.entries[0]==77 && list.entries[1]==77 && list.entries[2]==sid.value && scene.storage->BaseDestroy().PreDepth()==1 && (scene.storage->pool.Slot(sid)[11]&2U));
}

// VT가 사제인 실제 공통 삭제는 TYPE70으로 바뀌어도 고유 접두와 같은 장기 부모를 선택한다.
TEST_CASE(ClientPriest_ActualDispatchKeepsVtableWithChangedType) {
    for (const bool wrapped:{false,true}) { // NoWorld 직접 VT 분배와 Native Session이 실제로 사용하는 사제 래퍼를 각각 독립 물리에서 확인한다.
        RegionScene scene;const auto sid=scene.Asset(0x50f210U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[10]=70;raw[40]=0;auto& list=scene.storage->sidLists.priests;list.count=1;list.entries[0]=sid.value;const auto free=scene.storage->pool.FreeCount();
        if (wrapped) {
            auto fallback=scene.storage->CommonDeletion().Hooks();const auto previous=fallback.emit; // 같은 실제 공통 선택/전송 경계와 파생44b9e0을 사용하며 성공 효과를 합성하지 않는다.
            fallback.emit=[&scene,previous](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::PostDestroy) scene.storage->DamageablePost().PostDestroy(event.sid,event.flags);else previous(event); };
            const auto hooks=MakePriestPreDestroyHooks(scene.storage->pool,scene.storage->PriestPre(),std::move(fallback));scene.storage->BaseDestroy().Destroy(sid,0x810U,hooks);
        } else CHECK(scene.storage->DestroyAsset(sid,0x810U));
        CHECK(list.count==0 && scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 전체 벌룬 본체를 조립해도 현재 물리·Player·SP·난수와 대여 상태를 초기화하지 않는다.
TEST_CASE(ClientBalloon_LongBodyDoesNotResetOwners) {
    RegionScene scene;const auto random=scene.random->Game().State();const auto free=scene.storage->pool.FreeCount();
    auto* body=&scene.storage->BalloonBody();CHECK(&scene.storage->BalloonBody()==body && &body->Pool()==&scene.storage->pool);
    CHECK(scene.storage->pool.FreeCount()==free && scene.random->Game().State()==random && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 실제 두 생성자 VT의 고유 Pre와 공통 Post가 World 없이 같은 풀을 반납한다.
TEST_CASE(ClientBalloon_TwoNoWorldDeleteParents) {
    for (const auto table:{0x5040c0U,0x504198U}) { // 그림자가 없는 두 실제 부모를 독립 저장소에서 확인한다.
        RegionScene scene;const auto sid=scene.Asset(table);const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->BalloonBody().Handles(sid));
        CHECK(scene.storage->DestroyAsset(sid,0x10U));CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 현재 WORD 그림자를 실제 가상 삭제한 뒤 부모를 삭제하며 TYPE/상태의 높은 WORD를 대상에 섞지 않는다.
TEST_CASE(ClientBalloon_ShadowIsDeletedBeforeParent) {
    RegionScene scene;const auto parent=scene.Asset(0x5040c0U),shadow=scene.Asset(0x504198U);auto raw=scene.storage->pool.AllocatedBytes(parent);raw[8]=static_cast<std::uint8_t>(shadow.value);raw[9]=static_cast<std::uint8_t>(shadow.value>>8);
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(parent,0x10U));
    CHECK(scene.storage->pool.FreeCount()==free+2 && scene.traces==2 && (scene.storage->pool.Slot(shadow)[11]&1U) && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 자기 WORD 대상의 재진입은 원래 dead 관문으로 반환하고 부모 깊이와 풀 반납을 중복하지 않는다.
TEST_CASE(ClientBalloon_SelfShadowUsesExistingDeadGuard) {
    RegionScene scene;const auto sid=scene.Asset(0x5040c0U);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[8]=static_cast<std::uint8_t>(sid.value);raw[9]=static_cast<std::uint8_t>(sid.value>>8);
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(sid,0x10U));
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 모든 알려진 Pre 연결 뒤에도 그림자의 미지 VT가 실패하면 두 실제 삭제 깊이와 부모 선택을 보존한다.
TEST_CASE(ClientBalloon_ShadowFailurePreservesBothDepths) {
    RegionScene scene;const auto parent=scene.Asset(0x5040c0U),shadow=scene.Asset(0x5076b8U);auto raw=scene.storage->pool.AllocatedBytes(parent);raw[8]=static_cast<std::uint8_t>(shadow.value);raw[9]=static_cast<std::uint8_t>(shadow.value>>8);scene.gameplay->selection.selected=parent.value;
    scene.onTrace=[&] { if (scene.traces==2) RegionPut(scene.storage->pool.AllocatedBytes(shadow),0,0x12345678U); }; // 실제 child 가상10/로그가 시작된 뒤 VT를 바꿔 현재 Pre 재조회의 실패를 선택한다.
    const auto free=scene.storage->pool.FreeCount();bool rejected{};
    try { scene.storage->DestroyAsset(parent,0x10U); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="PATCH 현재 삭제 가상 효과의 미지 VT"; }
    CHECK(rejected && scene.storage->pool.FreeCount()==free && scene.traces==2 && scene.clears==0 && scene.gameplay->selection.selected==parent.value);
    CHECK((scene.storage->pool.Slot(parent)[11]&3U)==2U && (scene.storage->pool.Slot(shadow)[11]&3U)==2U && scene.storage->BaseDestroy().PreDepth()==2 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제 최초 Pop은 현재 TYPE로 그림자를 만들고 같은 장기 재귀 PostPop/공통 깊이를 끝낸다.
TEST_CASE(ClientBalloon_FirstPopCreatesCurrentActualShadow) {
    RegionScene scene(true);scene.gameplay->Server()=true;const auto sid=scene.Asset(0x5040c0U);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=0;RegionPut(raw,36,0);const auto type=raw[10];
    CHECK((scene.storage->types[type].flags1&0x400000U)!=0);const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->PopAsset(sid,128,128,0x10U)==RawPopResult::Registered);
    const auto current=scene.storage->pool.Slot(sid);const Sid shadow{static_cast<std::uint16_t>(current[8]|std::uint16_t(current[9])<<8)};CHECK(shadow.value!=0 && scene.storage->pool.FreeCount()==free-1);if (!shadow.value) return;
    const auto image=scene.storage->pool.Slot(shadow);CHECK(image[10]==type && (image[40]&8U) && !(image[11]&1U) && scene.storage->PostPop().ActivationDepth()==0 && !scene.storage->Borrowed());
}

// Cloud Floater의 최초 Pop은 실제 동일 Host에 사건3/0.1초 주기를 부착한다.
TEST_CASE(ClientBalloon_CloudFirstPopUsesRealRegular) {
    RegionScene scene(true);scene.gameplay->Server()=true;const auto sid=scene.Asset(0x504198U);scene.storage->pool.AllocatedBytes(sid)[40]&=0xf6U; // 검사용 추상/그림자 비트9만 끄고 원문의 일반 최초 Pop 관문을 사용한다.
    CHECK(scene.storage->PopAsset(sid,128,128,0x10U)==RawPopResult::Registered);
    auto* process=scene.storage->Processes().Host().FindEvent(sid,3);CHECK(process);if (!process) return;
    CHECK(process->Count()==0 && process->Payload()==0.1F && !process->Shared() && scene.storage->PostPop().ActivationDepth()==0 && !scene.storage->Borrowed());
    const auto frame=RegionRead(scene.storage->pool.Slot(sid),36);process->RunFrame(); // 실제 Host의 NoWorld 분배가 사건3·프레임·그림자 재귀 Pop까지 수행한다.
    CHECK(process->Count()==1 && process->Payload()==0.1F && RegionRead(scene.storage->pool.Slot(sid),36)==frame+1 && scene.storage->PostPop().ActivationDepth()==0);
}

// 사건3은 큰 Carrier Runtime 없이 실제 프레임을 진행하며 payload의 NaN 비트도 보존한다.
TEST_CASE(ClientBalloon_CloudEvent3UsesActualFrameAdvance) {
    RegionScene scene;scene.gameplay->Server()=true;const auto sid=scene.Asset(0x504198U);RegionPut(scene.storage->pool.AllocatedBytes(sid),36,0);const auto type=scene.storage->pool.Slot(sid)[10];CHECK(scene.storage->frames[type].Codes().size()>1);
    const auto value=scene.storage->BalloonRegular(sid,3,17,std::bit_cast<float>(0x7fc12345U));
    CHECK(std::bit_cast<std::uint32_t>(value)==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==1 && !scene.storage->Borrowed());
}

// 다른 사건은 실제 Carrier Runtime이 없을 때 payload 성공으로 대신하지 않는다.
TEST_CASE(ClientBalloon_NonFrameEventRequiresActualCarrierRuntime) {
    RegionScene scene;const auto sid=scene.Asset(0x504198U);const auto before=RegionRead(scene.storage->pool.Slot(sid),36);bool rejected{};
    try { scene.storage->BalloonRegular(sid,2,17,0.25F); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="장기 벌룬 현재 Carrier Regular 미연결"; }
    CHECK(rejected && RegionRead(scene.storage->pool.Slot(sid),36)==before && !scene.storage->Borrowed());
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


// 실제 세 자기삭제 가상34는 두 인수를 무시하고 flags0의 현재 가상10을 실행한다.
TEST_CASE(ClientAssetNotice_ThreeSelfDeleteVtables) {
    for (const auto table:{0x509148U,0x509210U,0x50eb98U}) { // 공중3종이 아닌 세 일반 자산만 실제 client SID로 확인한다.
        RegionScene scene;const auto sid=scene.Asset(table);scene.Position(sid);const auto free=scene.storage->pool.FreeCount();
        CHECK(scene.storage->NotifyAssetDependency(sid,Sid{65535},0xffffffffU));
        CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
        CHECK(scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
    }
}

// 자기삭제 통지에는 서버 관문이 없으며 client SID는 원문의 송신 억제를 유지한다.
TEST_CASE(ClientAssetNotice_SelfDeleteAlsoOnNonServer) {
    RegionScene scene;const auto sid=scene.Asset(0x50eb98U);scene.Position(sid);scene.gameplay->Authority()->server=false;
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->NotifyAssetDependency(sid,{},0));
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 이미 dead인 수신자는 실제 가상10의 기존 관문으로 돌아가며 통지가 풀/선택을 중복 처리하지 않는다.
TEST_CASE(ClientAssetNotice_SelfDeleteRetainsDeadGuard) {
    RegionScene scene;const auto sid=scene.Asset(0x50eb98U);scene.storage->pool.AllocatedBytes(sid)[11]|=2U;scene.gameplay->selection.selected=sid.value;
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->NotifyAssetDependency(sid,Sid{65535},0xffffffffU));
    CHECK(scene.storage->pool.FreeCount()==free && scene.traces==0 && scene.clears==0 && scene.gameplay->selection.selected==sid.value);
}

// 빈 원문 ret8은 TYPE/owner/두 통지 인자를 해석하지 않고 물리 필드를 보존한다.
TEST_CASE(ClientAssetNotice_EmptyBodyDoesNotReadTypeOrArguments) {
    RegionScene scene;const auto sid=scene.Asset(0x502370U);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[10]=255;raw[34]=255;
    const std::vector<std::uint8_t> before(raw.begin(),raw.end());const auto free=scene.storage->pool.FreeCount();
    CHECK(scene.storage->NotifyAssetDependency(sid,Sid{65535},0xffffffffU));
    CHECK(std::equal(before.begin(),before.end(),scene.storage->pool.Slot(sid).begin()) && scene.storage->pool.FreeCount()==free && scene.traces==0 && scene.players->Base()==0);
}

// 등록되지 않은 Form VT는 자산 성공으로 삼지 않고 기존 Host가 처리하도록 false로 돌린다.
TEST_CASE(ClientAssetNotice_UnknownVtableIsNotConsumed) {
    RegionScene scene;const auto sid=scene.Asset(0x50eb98U);RegionPut(scene.storage->pool.AllocatedBytes(sid),0,0x12345678U);
    const auto free=scene.storage->pool.FreeCount();CHECK(!scene.storage->NotifyAssetDependency(sid,Sid{65535},0xffffffffU));
    CHECK(scene.storage->pool.FreeCount()==free && scene.traces==0 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 현재 VT가 가상 선택을 결정하며 다른 유효 TYPE로 바꿔도 자기삭제 통지 본문을 유지한다.
TEST_CASE(ClientAssetNotice_CurrentVtableSelectsBody) {
    RegionScene scene;const auto sid=scene.Asset(0x509148U);scene.Position(sid);scene.storage->pool.AllocatedBytes(sid)[10]=70;
    const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->NotifyAssetDependency(sid,{},0));
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제 가상10 진입 뒤 현재 VT가 바뀌면 실패 접두/깊이를 보존하고 풀을 거짓 반납하지 않는다.
TEST_CASE(ClientAssetNotice_DeleteFailureRetainsActualPrefix) {
    RegionScene scene;const auto sid=scene.Asset(0x50eb98U);scene.Position(sid);scene.gameplay->selection.selected=sid.value;
    scene.onTrace=[&] { RegionPut(scene.storage->pool.AllocatedBytes(sid),0,0x12345678U); };
    const auto free=scene.storage->pool.FreeCount();bool rejected{};
    try { scene.storage->NotifyAssetDependency(sid,{},0); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="PATCH 현재 삭제 가상 효과의 미지 VT"; }
    CHECK(rejected && scene.storage->pool.FreeCount()==free && scene.traces==1 && scene.clears==0 && scene.gameplay->selection.selected==sid.value);
    CHECK((scene.storage->pool.Slot(sid)[11]&3U)==2U && scene.storage->BaseDestroy().PreDepth()==1 && scene.storage->BaseDestroy().PostDepth()==0);
}


// 장기 다리 본문 조회는 기존 물리/난수/Player/SP/World 대여를 초기화하지 않는다.
TEST_CASE(ClientBridgeRegular_LongBodyDoesNotResetOwners) {
    RegionScene scene;const auto seed=scene.random->Game().State(),free=scene.storage->pool.FreeCount();
    auto* body=&scene.storage->BridgeRegular();CHECK(&scene.storage->BridgeRegular()==body);
    CHECK(scene.random->Game().State()==seed && scene.storage->pool.FreeCount()==free && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 다른 사건은 원문처럼 수신자 TYPE/프레임/owner를 읽지 않고 payload의 비트를 반환한다.
TEST_CASE(ClientBridgeRegular_UnknownEventPreservesPayload) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[10]=255;raw[34]=255;RegionPut(raw,36,0xffffffffU);
    const auto seed=scene.random->Game().State();const auto result=scene.storage->BridgeRegular().Handle(sid,17,0xffffffffU,std::bit_cast<float>(0x7fc12345U));
    CHECK(std::bit_cast<std::uint32_t>(result)==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==0xffffffffU && scene.random->Game().State()==seed && scene.traces==0);
}

// 현재 boss0은 두 원문 사건/끝 칸 변환을 TYPE/시각/공간/삭제 전에 끝낸다.
TEST_CASE(ClientBridgeRegular_CurrentAuthorityZeroGuardsBothEvents) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);auto& body=scene.storage->BridgeRegular();scene.gameplay->Authority()->boss=false;
    auto raw=scene.storage->pool.AllocatedBytes(sid);raw[10]=255;RegionPut(raw,36,0xffffffffU);const auto free=scene.storage->pool.FreeCount();
    CHECK(body.Handle(sid,kBridgeDestroyEvent,0xffffffffU,std::bit_cast<float>(0x7fc12345U))==0.0F);
    CHECK(body.Handle(sid,kBridgeFallEvent,0xffffffffU,std::bit_cast<float>(0x7fc12345U))==0.0F);body.ConvertEnd(sid,0,0);
    CHECK(scene.storage->pool.FreeCount()==free && scene.traces==0 && RegionRead(scene.storage->pool.Slot(sid),36)==0xffffffffU && scene.players->Base()==0);
}

// 본문 생성 후 바뀐 현재 boss를 읽고 실제 client SID의 flags0 가상 삭제를 선택한다.
TEST_CASE(ClientBridgeRegular_AuthorityChangeUsesActualVirtualDelete) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);scene.Position(sid);auto& body=scene.storage->BridgeRegular();scene.gameplay->world.editor=1;scene.gameplay->Authority()->boss=false;
    const auto free=scene.storage->pool.FreeCount();CHECK(body.Handle(sid,kBridgeDestroyEvent,99,0.25F)==0.0F && scene.storage->pool.FreeCount()==free);
    scene.gameplay->Authority()->boss=true;CHECK(body.Handle(sid,kBridgeDestroyEvent,99,0.25F)==-1.0F);
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// J/K 외 실제 L 글자는 끝 칸 변환을 하지 않고 현재 부모/프레임을 보존한다.
TEST_CASE(ClientBridgeRegular_FallWithOtherLetterDoesNotChangeParent) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);scene.Position(sid);const auto frame=scene.storage->frames[82].Run('L').first;CHECK(frame>=0);if (frame<0) return;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),36,static_cast<std::uint32_t>(frame));const auto free=scene.storage->pool.FreeCount();
    CHECK(scene.storage->BridgeRegular().Handle(sid,kBridgeFallEvent,0xffffffffU,123.0F)==-1.0F);
    CHECK(scene.storage->pool.FreeCount()==free && scene.traces==0 && RegionRead(scene.storage->pool.Slot(sid),36)==static_cast<std::uint32_t>(frame));
}

// 실제 J 프레임/빈 이웃 공간은 임시 프레임을 복구한 뒤 원문 flags0 자기삭제를 실행한다.
TEST_CASE(ClientBridgeRegular_FallWithoutNeighborDeletesActualParent) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);scene.Position(sid);scene.gameplay->world.editor=1;const auto frame=scene.storage->frames[82].Run('J').first;CHECK(frame>=0);if (frame<0) return;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),36,static_cast<std::uint32_t>(frame));const auto free=scene.storage->pool.FreeCount();
    CHECK(scene.storage->BridgeRegular().Handle(sid,kBridgeFallEvent,0xffffffffU,0.0F)==-1.0F);
    CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제 client ProcessForm/Kernel의 기본 Regular 분배도 같은 현재 boss0을 읽고 예약을 종료한다.
TEST_CASE(ClientBridgeRegular_ActualHostEndsOnCurrentAuthorityZero) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);scene.gameplay->Authority()->boss=false;auto& host=scene.storage->Processes().Host();const auto free=scene.storage->pool.FreeCount();
    auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=8U;raw[11]&=static_cast<std::uint8_t>(~4U); // 실제 Host 관문을 통과하는 live/buried 부모 입력이며 boss0 사건은 공간을 소비하지 않는다.
    auto* process=host.AddRegular(sid,kBridgeDestroyEvent,0.25F,0x52U);CHECK(process);if (!process) return;process->RunFrame();
    CHECK(host.FindEvent(sid,kBridgeDestroyEvent)==nullptr && scene.storage->pool.FreeCount()==free && !(scene.storage->pool.Slot(sid)[11]&1U) && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && !scene.storage->Borrowed());
}

// Host는 호출 시 현재 VT를 선택하므로 TYPE82여도 base VT로 바뀌면 payload 예약을 유지한다.
TEST_CASE(ClientBridgeRegular_ActualHostReadsCurrentVtable) {
    RegionScene scene;const auto sid=scene.Asset(kPatchBridgeVtable);scene.gameplay->Authority()->boss=false;auto& host=scene.storage->Processes().Host();
    auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=8U;raw[11]&=static_cast<std::uint8_t>(~4U); // 원문의 abstract/void 관문을 지난 현재 VT 분배 입력을 준비한다.
    auto* process=host.AddRegular(sid,kBridgeDestroyEvent,0.25F,0x52U);CHECK(process);if (!process) return;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),0,0x50eb98U);process->RunFrame();
    CHECK(host.FindEvent(sid,kBridgeDestroyEvent)==process && process->Count()==1 && process->Payload()==0.25F && !scene.storage->Borrowed());
}


// 두 프레임 본문은 같은 현재 장기 상태를 읽고 Player/SP/난수/풀을 초기화하지 않는다.
TEST_CASE(ClientFrameRegular_ImpactUsesCurrentFrameTableAndPayload) {
    RegionScene scene;const auto sid=scene.Asset(0x509758U);scene.Position(sid);const auto type=scene.storage->pool.Slot(sid)[10];const auto run=scene.storage->frames[type].Run('A');CHECK(run.count>1);if (run.count<2) return;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),36,static_cast<std::uint32_t>(run.first));scene.storage->Frame().StoreLevel(sid);const auto seed=scene.random->Game().State(),free=scene.storage->pool.FreeCount();
    const auto result=scene.storage->ImpactRegular(sid,0,1,std::bit_cast<float>(0x7fc12345U));
    CHECK(std::bit_cast<std::uint32_t>(result)==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==static_cast<std::uint32_t>(run.first+1));
    CHECK(scene.storage->pool.FreeCount()==free && scene.random->Game().State()==seed && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 범위 끝과 signed 음수 count는 같은 실제 client SID를 flags0으로 삭제하고-1을 반환한다.
TEST_CASE(ClientFrameRegular_ImpactBothExpiryBranchesUseActualDelete) {
    for (const bool negative:{false,true}) { // 원문의 signed 비교 양쪽 종료를 독립 Pool에서 확인한다.
        RegionScene scene;const auto sid=scene.Asset(0x509758U);scene.Position(sid);const auto type=scene.storage->pool.Slot(sid)[10];const auto count=negative ? 0xffffffffU:static_cast<std::uint32_t>(scene.storage->frames[type].Run('A').count);const auto free=scene.storage->pool.FreeCount();
        CHECK(scene.storage->ImpactRegular(sid,0,count,0.125F)==-1.0F);
        CHECK(scene.storage->pool.FreeCount()==free+1 && scene.traces==1 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
    }
}

// 실제 Host 기본 분배도 현재 anim VT를 선택하고 원래 payload/횟수로 다시 예약한다.
TEST_CASE(ClientFrameRegular_ImpactActualHostUsesSameBody) {
    RegionScene scene;const auto sid=scene.Asset(0x509758U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=8;raw[11]&=static_cast<std::uint8_t>(~4U);scene.storage->Frame().StoreLevel(sid);
    scene.Index(sid); // live 부모의 실제 현재 저장 단계/같은 SHP/TYPE에 맞는 공간 해시를 준비한다.
    auto& host=scene.storage->Processes().Host();auto* process=host.AddRegular(sid,0,0.125F,0x52U);CHECK(process);if (!process) return;process->RunFrame();
    CHECK(host.FindEvent(sid,0)==process && process->Count()==1 && process->Payload()==0.125F && !scene.storage->Borrowed());
}

// 종료 중 부모가 자기 Regular도 삭제하면 기존 Host의 실제 PID 소멸 관문으로 돌아간다.
TEST_CASE(ClientFrameRegular_ImpactExpiryRemovesActualParentAndProcess) {
    RegionScene scene;const auto sid=scene.Asset(0x509758U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=8;raw[11]&=static_cast<std::uint8_t>(~4U);
    scene.storage->Frame().StoreLevel(sid);scene.Index(sid); // 실제 삭제의 Unpop found 관문이 요구하는 live 부모 공간 입력이다.
    auto& host=scene.storage->Processes().Host();auto* process=host.AddRegular(sid,0,0.125F,0x52U);CHECK(process);if (!process) return;
    process->SetInitialCount(static_cast<std::uint32_t>(scene.storage->frames[raw[10]].Run('A').count));const auto free=scene.storage->pool.FreeCount();process->RunFrame();
    CHECK(host.FindEvent(sid,0)==nullptr && scene.storage->pool.FreeCount()==free+2 && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 현재 방향/클러스터 DWORD를 바꾸면 다음 호출이 새 값으로 modulo를 계산하며 소스 사본을 만들지 않는다.
TEST_CASE(ClientFrameRegular_DiscReadsBothCurrentTypeFields) {
    RegionScene scene;const auto sid=scene.Asset(0x50b570U);scene.Position(sid);const auto type=scene.storage->pool.Slot(sid)[10];auto& record=scene.storage->types[type];record.frameCount=4;record.directions=1;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),36,0);scene.storage->Frame().StoreLevel(sid);const auto seed=scene.random->Game().State();
    CHECK(std::bit_cast<std::uint32_t>(scene.storage->DiscRegular(sid,0xffffffffU,0xffffffffU,std::bit_cast<float>(0x7fc12345U)))==0x7fc12345U && RegionRead(scene.storage->pool.Slot(sid),36)==1);
    record.directions=4;scene.storage->DiscRegular(sid,17,99,0.25F);CHECK(RegionRead(scene.storage->pool.Slot(sid),36)==1);
    record.frameCount=8;scene.storage->DiscRegular(sid,17,99,0.25F);CHECK(RegionRead(scene.storage->pool.Slot(sid),36)==0 && scene.random->Game().State()==seed && scene.players->Base()==0 && !scene.sp->Store().Initialized() && !scene.storage->Borrowed());
}

// 음수 방향 수는 원문의 정상 IDIV 피연산자이며 Native에서 typed 사전 거부로 바꾸지 않는다.
TEST_CASE(ClientFrameRegular_DiscNativeSignedDirectionsReachActualFrame) {
    RegionScene scene;const auto sid=scene.Asset(0x50b570U);scene.Position(sid);const auto type=scene.storage->pool.Slot(sid)[10];scene.storage->types[type].frameCount=4;scene.storage->types[type].directions=-1;
    RegionPut(scene.storage->pool.AllocatedBytes(sid),36,0);scene.storage->Frame().StoreLevel(sid);CHECK(scene.storage->DiscRegular(sid,0,0,0.5F)==0.5F && RegionRead(scene.storage->pool.Slot(sid),36)==1);
}

// 원문 IDIV 오류는 프레임 지정 전에 명시 예외로 나타나며 기존 물리/난수/풀을 유지한다.
TEST_CASE(ClientFrameRegular_DiscDivideErrorPreservesPrefix) {
    RegionScene scene;const auto sid=scene.Asset(0x50b570U);const auto type=scene.storage->pool.Slot(sid)[10];scene.storage->types[type].frameCount=4;scene.storage->types[type].directions=0;
    const auto frame=RegionRead(scene.storage->pool.Slot(sid),36),free=scene.storage->pool.FreeCount(),seed=scene.random->Game().State();bool rejected{};
    try { scene.storage->DiscRegular(sid,0,0,0.5F); }catch (const std::domain_error&) { rejected=true; }
    CHECK(rejected && RegionRead(scene.storage->pool.Slot(sid),36)==frame && scene.storage->pool.FreeCount()==free && scene.random->Game().State()==seed && scene.traces==0);
}

// 실제 Host의 썬 디스크 분배는 현재 TYPE를 쓰고 event/count 종류와 무관하게 payload를 유지한다.
TEST_CASE(ClientFrameRegular_DiscActualHostUsesSameCurrentType) {
    RegionScene scene;const auto sid=scene.Asset(0x50b570U);scene.Position(sid);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=8;raw[11]&=static_cast<std::uint8_t>(~4U);scene.storage->types[raw[10]].frameCount=4;scene.storage->types[raw[10]].directions=1;RegionPut(raw,36,0);scene.storage->Frame().StoreLevel(sid);
    scene.Index(sid); // 실제 Host의 live 부모는 현재 Frame 저장 단계와 같은 해시에도 존재한다.
    auto& host=scene.storage->Processes().Host();auto* process=host.AddRegular(sid,99,0.25F,0x52U);CHECK(process);if (!process) return;process->RunFrame();
    CHECK(host.FindEvent(sid,99)==process && process->Count()==1 && process->Payload()==0.25F && RegionRead(scene.storage->pool.Slot(sid),36)==1 && !scene.storage->Borrowed());
}

// 새 기대값은 실제 PATCH Handle의 독립 명령 실행70입력만 읽으며 산술식을 검사 안에 복제하지 않는다.
TEST_CASE(ClientFrameRegular_DiscNativeIntegerBoundaryGolden) {
    const auto fixture=std::filesystem::path(__FILE__).parent_path()/"fixtures/disc-regular-boundary-1219-x86.tsv";std::size_t count{},faults{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(fixture.string().c_str()).rows) { // 원문 정상47/IDIV오류23 모두 같은 입력으로 대조한다.
        CHECK(row.size()==8);const auto current=static_cast<std::int32_t>(std::stoll(row[0])),frames=static_cast<std::int32_t>(std::stoll(row[1])),directions=static_cast<std::int32_t>(std::stoll(row[2]));
        if (row[3]=="divide-error") { bool rejected{};try { RawDiscRegular::NextFrameNative(current,frames,directions); }catch (const std::domain_error&) { rejected=true; }CHECK(rejected);++faults; }
        else { const auto frame=RawDiscRegular::NextFrameNative(current,frames,directions);CHECK(static_cast<std::uint32_t>(frame)==std::stoul(row[4]));CHECK(std::stoul(row[5])==0 && std::stoul(row[6])==0x3d800000U); }
        ++count;
    }
    CHECK(count==70 && faults==23);
}

namespace { // 감시 연결의 입력은 실제 슬롯의 WORD만 바꾸며 인접 상대 번호를 보존한다.
void DependWord(SidPool& pool,Sid sid,std::size_t offset,std::uint16_t value) {
    auto raw=pool.AllocatedBytes(sid);raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); // 원문 비정렬 WORD 쓰기다.
}
}
TEST_CASE(ClientDependOwner_GetterKeepsCurrentPoolTypeAndFactory) { // getter는 대여·생성·초기화 없이 같은 주소를 반환한다.
    RegionScene scene;scene.storage->GameAux()->DependFormType()=8;const auto free=scene.storage->pool.FreeCount();auto* factory=&scene.storage->Factory();auto* first=&scene.storage->DependForms();
    CHECK(first==&scene.storage->DependForms() && &first->Pool()==&scene.storage->pool && factory==&scene.storage->Factory());CHECK(scene.storage->pool.FreeCount()==free && scene.storage->GameAux()->DependFormType()==8 && !scene.storage->Borrowed());
}
TEST_CASE(ClientDependOwner_ActualPairCreateFindAndRemoveWithoutWorld) { // 실제 공장·두 부착·검색·현재 가상 삭제가 같은 풀을 쓴다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);const auto free=scene.storage->pool.FreeCount();auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);
    CHECK(body.Has(watched,watcher) && body.Watcher(pair.watchedForm)==watcher);CHECK(CommonContainedParent(scene.storage->pool,pair.watchedForm)==watched && CommonContainedParent(scene.storage->pool,pair.watcherForm)==watcher && scene.storage->pool.FreeCount()==free-2);
    body.Remove(watcher,watched,0x10U);CHECK(!body.Has(watched,watcher) && scene.storage->pool.FreeCount()==free && !scene.storage->Borrowed());CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0 && scene.storage->postPopState.depth==0);
}
TEST_CASE(ClientDependOwner_CurrentTypesUseActualConstructorAndOldClass) { // 두 현재 번호가 같은 감시 생성자를 사용해도 조회/제거는 현재 전체 번호를 읽는다.
    RegionScene scene;scene.storage->types[7]=scene.storage->types[1];scene.storage->types[8]=scene.storage->types[1];const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);const auto free=scene.storage->pool.FreeCount();auto& body=scene.storage->DependForms();
    scene.storage->GameAux()->DependFormType()=7;const auto old=body.Create(watched,watcher);scene.storage->GameAux()->DependFormType()=8;CHECK(!body.Has(watched,watcher));const auto current=body.Create(watched,watcher);CHECK(body.Has(watched,watcher));
    CHECK(scene.storage->pool.Slot(old.watchedForm)[10]==7 && scene.storage->pool.Slot(current.watchedForm)[10]==8);body.Remove(watcher,watched,0x10U);CHECK(scene.storage->pool.FreeCount()==free-2 && body.IsDependForm(old.watchedForm));scene.storage->GameAux()->DependFormType()=7;body.Remove(watcher,watched,0x10U);CHECK(scene.storage->pool.FreeCount()==free);
}
TEST_CASE(ClientDependOwner_CurrentTypeDoesNotNarrowToByte) { // 현재 DWORD257을 TYPE BYTE1 또는 초기1로 바꾸지 않는다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);const auto free=scene.storage->pool.FreeCount();scene.storage->GameAux()->DependFormType()=257;
    CHECK(!body.Has(watched,watcher));body.Remove(watcher,watched,0x10U);CHECK(scene.storage->pool.FreeCount()==free && body.IsDependForm(pair.watchedForm));scene.storage->GameAux()->DependFormType()=1;body.Remove(watcher,watched,0x10U);CHECK(scene.storage->pool.FreeCount()==free+2);
}
TEST_CASE(ClientDependOwner_FinderAndParentDoNotAddFreeDeadFilters) { // 원문 FormFinder/현재 부모 getter는 후보·상대의 free/dead를 추가 필터로 쓰지 않는다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);auto first=scene.storage->pool.AllocatedBytes(pair.watchedForm),second=scene.storage->pool.AllocatedBytes(pair.watcherForm);const auto firstState=first[11],secondState=second[11];first[11]|=3U;second[11]|=1U;
    CHECK(body.Has(watched,watcher) && body.Watcher(pair.watchedForm)==watcher);first[11]=firstState;second[11]=secondState;body.Remove(watcher,watched,0x10U);
}
TEST_CASE(ClientDependOwner_FinderExcludesAssetByteBeforeDependComparison) { // FormFinder는 현재 Depend 번호와 같아도 TYPE70 이상을 후보로 반환하지 않는다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);scene.storage->pool.AllocatedBytes(pair.watchedForm)[10]=70;scene.storage->GameAux()->DependFormType()=70;
    CHECK(!body.Has(watched,watcher));scene.storage->pool.AllocatedBytes(pair.watchedForm)[10]=1;scene.storage->GameAux()->DependFormType()=1;body.Remove(watcher,watched,0x10U);
}
TEST_CASE(ClientDependOwner_PreDestroyUsesActualProcessPeerVirtual) { // 실제 감시+14의 상대는 현재 ProcessForm 가상10으로 삭제한다.
    RegionScene scene;const auto parent=scene.Asset(0x5092d0U);auto& host=scene.storage->Processes().Host();auto* process=host.AddRegular(parent,99,0.25F,0x52U);CHECK(process);if (!process) return;const auto processForm=process->Form();const auto source=scene.storage->Factory().Create(1,2);scene.storage->Factory().AttachForm(source,parent,0x10U);DependWord(scene.storage->pool,source,20,processForm.value);const auto free=scene.storage->pool.FreeCount();
    scene.storage->DependForms().Destroy(source,0x10U);CHECK(host.FindEvent(parent,99)==nullptr && (scene.storage->pool.Slot(source)[11]&3U)==3U && (scene.storage->pool.Slot(processForm)[11]&3U)==3U && scene.storage->pool.FreeCount()==free+2);CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}
TEST_CASE(ClientDependOwner_PreDestroyReadsSecondPeerAfterFirstEffect) { // 첫 상대 삭제의 실제 trace 효과가 바꾼 두 번째 WORD/현재 VT를 다시 읽는다.
    RegionScene scene;const auto parent=scene.Asset(0x5092d0U);auto& host=scene.storage->Processes().Host();auto* first=host.AddRegular(parent,91,0.25F,0x52U);auto* second=host.AddRegular(parent,92,0.25F,0x52U);auto* third=host.AddRegular(parent,93,0.25F,0x52U);CHECK(first && second && third);if (!first || !second || !third) return;const auto firstSid=first->Form(),secondSid=second->Form(),thirdSid=third->Form();const auto source=scene.storage->Factory().Create(1,2);scene.storage->Factory().AttachForm(source,parent,0x10U);DependWord(scene.storage->pool,source,20,firstSid.value);DependWord(scene.storage->pool,source,18,secondSid.value);bool changed{};
    scene.onTrace=[&] { if (!changed && (scene.storage->pool.Slot(firstSid)[11]&2U)) { changed=true;DependWord(scene.storage->pool,source,18,thirdSid.value); } };scene.storage->DependForms().Destroy(source,0x10U);CHECK(changed && host.FindEvent(parent,91)==nullptr && host.FindEvent(parent,93)==nullptr && host.FindEvent(parent,92)==second && !(scene.storage->pool.Slot(secondSid)[11]&3U));scene.onTrace={};CHECK(scene.storage->DestroyAsset(secondSid,0x10U));
}
TEST_CASE(ClientDependOwner_CommonSelectionKeepsFullDword) { // 현재 전체 DWORD 비교가 일치할 때만 실제 Client 선택 정리를 실행한다.
    RegionScene scene;const auto first=scene.storage->Factory().Create(1,2);scene.gameplay->selection.selected=0x10000U|first.value;scene.storage->DependForms().Destroy(first,0x10U);CHECK(scene.clears==0 && scene.gameplay->selection.selected==(0x10000U|first.value));const auto second=scene.storage->Factory().Create(1,2);scene.gameplay->selection.selected=second.value;scene.storage->DependForms().Destroy(second,0x10U);CHECK(scene.clears==1 && scene.gameplay->selection.selected==0);
}
TEST_CASE(ClientDependOwner_ThreeEmptyVirtualNoticesIgnoreOtherFields) { // 실제1/3/6의+34는 TYPE·권한·인수를 읽지 않는 같은 원문 빈 함수다.
    RegionScene scene;
    for (const auto type:{1U,3U,6U}) { // 실제 공장의 세 생성자 VT를 사용하며 삭제 미구현과 구별한다.
        const auto form=scene.storage->Factory().Create(type,2);scene.storage->pool.AllocatedBytes(form)[10]=255;scene.storage->NotifyDependency(form,Sid{65535},Sid{65534});
    }
}
TEST_CASE(ClientDependOwner_ProcessNoticeServerZeroStopsBeforePid) { // 실제 Process+34는 server0이면 없는 PID·인수·전송 장치를 선택하지 않는다.
    RegionScene scene;const auto form=scene.storage->Factory().Create(2,2);scene.storage->Authority()->server=false;scene.storage->NotifyDependency(form,Sid{65535},Sid{65534});CHECK(RegionRead(scene.storage->pool.Slot(form),18)==0);
}
TEST_CASE(ClientDependOwner_CurrentNoticePinsOwnerAndExpires) { // 알림 중 대여/외부 소유자를 반납해도 실제 pin·함수 사본이 반환까지 살아 있다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.storage->Factory().Create(3,2);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);std::size_t calls{};auto owner=std::make_shared<int>(7);std::weak_ptr<int> weak=owner;auto binding=std::make_shared<RawSurfaceDependBindings>();
    binding->pin=[weak]()->std::shared_ptr<void> { const auto pin=weak.lock();if (!pin) throw std::logic_error("검사 실제 알림 owner 반납 뒤 pin");return pin; };
    binding->notice=[&](auto receiver,auto parent,auto form) { ++calls;owner.reset();binding.reset();CHECK(!weak.expired());scene.storage->NotifyDependency(receiver,parent,form); };scene.storage->BindDependRuntime(binding);body.Notify(pair.watchedForm,watched);CHECK(calls==1 && weak.expired());body.Notify(pair.watchedForm,watched);CHECK(calls==1);body.Remove(watcher,watched,0x10U);
}
TEST_CASE(ClientDependOwner_NotifyAssertionCanReturnNormally) { // 통지 뒤 현재 watchedFid 진단은 실제 Client assertion 정상 복귀를 보존한다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.storage->Factory().Create(3,2);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);std::size_t assertions{};scene.storage->BindDisplayDiagnostics(scene.detail,[&](auto expression,auto file,auto line) { CHECK(expression=="watchedFid == 0" && file=="\\Ns\\O\\Squid.cpp" && line==0xadf);++assertions; });DependWord(scene.storage->pool,pair.watchedForm,20,pair.watcherForm.value);body.Notify(pair.watchedForm,watched);CHECK(assertions==1);DependWord(scene.storage->pool,pair.watchedForm,20,0);body.Remove(watcher,watched,0x10U);
}
TEST_CASE(ClientDependOwner_RemoveAssertionsReadCurrentFieldsAndContinue) { // 원문 제거 진단 둘이 정상 복귀하면 빈 head에서 정상 종료한다.
    RegionScene scene;const auto parent=scene.Asset(0x5092d0U);scene.storage->GameAux()->DependFormType()=scene.storage->pool.Slot(parent)[10];std::vector<std::uint32_t> lines;scene.storage->BindDisplayDiagnostics(scene.detail,[&](auto,auto file,auto line) { CHECK(file=="\\Ns\\O\\Squid.cpp");lines.push_back(line); });scene.storage->DependForms().Remove(parent,parent,0x10U);CHECK(lines==std::vector<std::uint32_t>({0xb4b,0xb4e}));
}
TEST_CASE(ClientDependOwner_FinderReadsHeadAfterCurrentCheckingAssertion) { // checking 진단의 정상 복귀가 바꾼 현재 head로 탐색을 시작한다.
    RegionScene scene;const auto watched=scene.Asset(0x5092d0U),watcher=scene.Asset(0x5092d0U);auto& body=scene.storage->DependForms();const auto pair=body.Create(watched,watcher);DependWord(scene.storage->pool,watched,6,0);auto raw=scene.storage->pool.AllocatedBytes(watched);const auto state=raw[11];raw[11]|=2U;scene.storage->Authority()->boss=false;scene.storage->BindChecking(RawCheckingFlag{0x80000000U});std::size_t assertions{};
    scene.storage->BindDisplayDiagnostics(scene.detail,[&](auto expression,auto file,auto line) { CHECK(expression=="inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy" && file=="\\Ns\\O\\Squidfinder.cpp" && line==0x158);++assertions;DependWord(scene.storage->pool,watched,6,pair.watchedForm.value); });CHECK(body.Has(watched,watcher) && assertions==1);raw[11]=state;scene.storage->Authority()->boss=true;body.Remove(watcher,watched,0x10U);
}
TEST_CASE(ClientDependOwner_NativeWorldBorrowsSameBodyAndReturnsIt) { // Native World의 Core도 같은 장기 감시 본체를 빌리고 반납 시 교체하지 않는다.
    RegionScene scene;auto* body=&scene.storage->DependForms();BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    { RawSurfaceWorld world(OriginalEdition::Patch1078,scene.storage->types,scene.storage->frames,scene.storage->shapes,links,94,nullptr,scene.storage);CHECK(&world.Core().dependencies==body && scene.storage->Borrowed()); }
    CHECK(&scene.storage->DependForms()==body && !scene.storage->Borrowed());
}

namespace { // 실제 UI 등록표/지연 삭제를 소유하며 게임·Session·통신을 생성하지 않는다.
struct GumpUiScene {
    RegionScene& scene; // 같은 Native 물리/공장/현재 전역이다.
    std::shared_ptr<RawButtonControls> controls; // 실제 등록표/노드/지연 삭제의 선택 pin 대상이다.
    std::shared_ptr<RawSurfaceGumpBindings> binding; // Storage에 약하게 빌린 실제 UI 알림/owner 장치다.
    explicit GumpUiScene(RegionScene& owner):scene(owner) {
        RawButtonHooks hooks;hooks.deleteForm=[storage=scene.storage](auto form,auto flags) { storage->GumpForms().Destroy(Sid{static_cast<std::uint16_t>(form)},flags); };hooks.project=[storage=scene.storage,gameplay=scene.gameplay](auto object) { return ProjectGumpObject(storage->pool,gameplay->Camera().Snapshot(),object); };
        controls=std::make_shared<RawButtonControls>(OriginalEdition::Patch1078,std::move(hooks));Page();binding=std::make_shared<RawSurfaceGumpBindings>();binding->notice=controls->ObjectNotice();binding->pin=[weak=std::weak_ptr<RawButtonControls>(controls)]()->std::shared_ptr<void> { const auto pin=weak.lock();if (!pin) throw std::logic_error("검사 실제 UI 반납 뒤 pin");return pin; };scene.storage->BindGumpUiRuntime(binding);
    }
    void Page() { const std::array<GumpControl,1> items{{{7,{10,20,85,40},false,true,false}}};const std::array<std::string,8> labels{{"","","","","","","","Menu"}};controls->SetControls(items,labels,640,480,RawButtonAssets{90,0,20,16,{7,255,13}}); } // 기존 실제 돌 버튼으로 비영 등록 ID를 만든다.
    std::uint32_t Owner(std::uint32_t token) const {
        for (std::uint32_t id=1;id<1000;++id) if (controls->RegisteredGump(id)==token) return id; // 실제 현재 등록 표에서만 ID를 얻는다.
        throw std::logic_error("검사 실제 UI 등록 없음");
    }
};
}
TEST_CASE(ClientGumpOwner_GetterKeepsPoolCurrentTypeAndFactory) { // getter는 생성·대여·상태 복사 없이 같은 장기 본체를 반환한다.
    RegionScene scene;scene.storage->containedGumpType=7;const auto free=scene.storage->pool.FreeCount();auto* factory=&scene.storage->Factory();auto* body=&scene.storage->GumpForms();CHECK(body==&scene.storage->GumpForms() && &body->Pool()==&scene.storage->pool && &scene.storage->Factory()==factory && body->CurrentType()==7 && scene.storage->pool.FreeCount()==free && !scene.storage->Borrowed());
}
TEST_CASE(ClientGumpOwner_CurrentConstructorAndWholeOwnerDword) { // 현재7의 실제 Gump 생성자와 전체 owner DWORD를 사용하고 별도 TYPE 사본을 쓰지 않는다.
    RegionScene scene;scene.storage->types[7]=scene.storage->types[3];scene.storage->containedGumpType=7;auto& body=scene.storage->GumpForms();const auto free=scene.storage->pool.FreeCount();const auto form=body.Create(2);body.SetOwner(form,0xfedc1234U);CHECK(scene.storage->pool.Slot(form)[10]==7 && RegionRead(scene.storage->pool.Slot(form),0)==0x512188U && body.Owner(form)==0xfedc1234U && body.Parent(form).value==0);body.Destroy(form,0x10U);CHECK(scene.storage->pool.FreeCount()==free && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}
TEST_CASE(ClientGumpOwner_TakeUsesCurrentTypeAndActualUnpop) { // 현재 번호로 같은 지정 Take를 실행해 부모 해제와 owner/payload 보존을 확인한다.
    RegionScene scene;scene.storage->types[7]=scene.storage->types[3];scene.storage->containedGumpType=7;auto& body=scene.storage->GumpForms();const auto parent=scene.Asset(0x5092d0U),form=body.Create(0);body.SetOwner(form,0x12345678U);body.Attach(form,parent,0x10U);CHECK(form.value>=scene.storage->pool.Layout().serverFirst); // 실제 Take의 FIRST_SERVER_SID 이상 지정 번호와 일치하는 기존 TYPE7을 사용한다.
    CHECK(body.Take(form)==form && scene.storage->pool.Slot(form)[10]==7 && body.Owner(form)==0x12345678U && body.Parent(form)==parent);CHECK((RegionRead(scene.storage->pool.Slot(parent),6)&0xffffU)==0 && (scene.storage->pool.Slot(form)[11]&12U)==4U);body.Destroy(form,0x10U); // 원문600입력/기존 연결 검사처럼 부모 WORD는 보존하고 contained/부모 head만 분리한다.
}
TEST_CASE(ClientGumpOwner_OutOfRangeCurrentTypeKeepsAllocationPrefix) { // 현재 DWORD를 BYTE나 고정3으로 줄이지 않고 원문 공통 할당 뒤 범위 실패를 유지한다.
    RegionScene scene;scene.storage->containedGumpType=0xffffffffU;auto& body=scene.storage->GumpForms();const auto free=scene.storage->pool.FreeCount();bool failed{};try { body.Create(2); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && scene.storage->pool.FreeCount()==free-1 && body.CurrentType()==0xffffffffU);
}
TEST_CASE(ClientGumpOwner_ZeroOwnerSkipsStateTypeAndAllUiDevices) { // 실제4ae9d0은 owner0이면 TYPE/free/부모/등록표를 읽지 않고 반환한다.
    RegionScene scene;auto& body=scene.storage->GumpForms();const auto form=body.Create(2);auto raw=scene.storage->pool.AllocatedBytes(form);const auto state=raw[11];raw[10]=255;raw[11]|=1U;body.Notify(form);raw[10]=3;raw[11]=state;body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_NotifySavesFullOwnerAndCurrentParentBeforeCallback) { // 원문 통지는 전체 owner와 부모 WORD를 저장하고 callback 후 Form을 다시 읽지 않는다.
    RegionScene scene;auto& body=scene.storage->GumpForms();const auto form=body.Create(2);body.SetOwner(form,0x80010001U);DependWord(scene.storage->pool,form,16,65534);auto binding=std::make_shared<RawSurfaceGumpBindings>();std::size_t calls{};
    binding->notice=[&](auto owner,auto parent) { CHECK(owner==0x80010001U && parent.value==65534);++calls;body.Destroy(form,0x10U); };scene.storage->BindGumpRuntime(binding);body.Notify(form);CHECK(calls==1 && (scene.storage->pool.Slot(form)[11]&3U)==3U);
}
TEST_CASE(ClientGumpOwner_ActualUiRegistryKillIsDeferred) { // 같은 실제 등록표가 비영 ID를 조회해 Kill하고 scalar 반납은 기존 큐 정리에서 수행한다.
    RegionScene scene;GumpUiScene ui(scene);auto& body=scene.storage->GumpForms();const auto parent=scene.Asset(0x5092d0U),form=body.Create(2);const auto token=ui.controls->ControlGump(7),id=ui.Owner(token);body.SetOwner(form,id);body.Attach(form,parent,0x10U);body.Notify(form);CHECK(ui.controls->RegisteredGump(id)==token);ui.controls->CleanupGumps();CHECK(ui.controls->RegisteredGump(id)==0);body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_RegistryLookupUsesCurrentReusedEntry) { // Form은 최초 UI 포인터를 보관하지 않고 재사용된 현재 등록 슬롯을 조회한다.
    RegionScene scene;GumpUiScene ui(scene);auto& body=scene.storage->GumpForms();const auto form=body.Create(2);const auto old=ui.controls->ControlGump(7),id=ui.Owner(old);body.SetOwner(form,id);ui.Page();const auto current=ui.controls->ControlGump(7);CHECK(current!=old && ui.controls->RegisteredGump(id)==current);body.Notify(form);ui.controls->CleanupGumps();CHECK(ui.controls->RegisteredGump(id)==0);body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_UiPinKeepsActualRegistryUntilNoticeReturns) { // 실제 알림 중 UI 소유자/대여를 반납해도 선택 pin이 실제 트리와 큐를 유지한다.
    RegionScene scene;GumpUiScene ui(scene);auto& body=scene.storage->GumpForms();const auto form=body.Create(2);body.SetOwner(form,ui.Owner(ui.controls->ControlGump(7)));std::weak_ptr<RawButtonControls> weak=ui.controls;const auto actual=ui.controls->ObjectNotice();bool called{};
    ui.binding->notice=[&](auto owner,auto parent) { ui.controls.reset();ui.binding.reset();CHECK(!weak.expired());actual(owner,parent);called=true; };body.Notify(form);CHECK(called && weak.expired());body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_MissingUiIsExplicitAndOwnerZeroIsNotRequired) { // 비영 소유자의 미연결을 성공으로 바꾸지 않으며 owner0은 실제 원문 반환이다.
    RegionScene scene;auto& body=scene.storage->GumpForms();const auto form=body.Create(2);body.Notify(form);body.SetOwner(form,1);bool failed{};try { body.Notify(form); }catch (const std::logic_error&) { failed=true; }CHECK(failed && !(scene.storage->pool.Slot(form)[11]&1U));body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_CurrentWorldNoticePinsAndExpiresToUiRegistry) { // 명시 World 소비자와 실제 UI의 수명을 보유하고 대여 만료 뒤 장기 실제 등록표를 사용한다.
    RegionScene scene;GumpUiScene ui(scene);auto& body=scene.storage->GumpForms();const auto form=body.Create(2);const auto token=ui.controls->ControlGump(7),id=ui.Owner(token);body.SetOwner(form,id);auto owner=std::make_shared<int>(1);std::weak_ptr<int> weak=owner;auto binding=std::make_shared<RawSurfaceGumpBindings>();std::size_t calls{};
    binding->pin=[weak]()->std::shared_ptr<void> { const auto pin=weak.lock();if (!pin) throw std::logic_error("검사 실제 World owner 반납 뒤 pin");return pin; };binding->notice=[&](auto actual,auto parent) { CHECK(actual==id && parent.value==0);++calls;owner.reset();binding.reset();CHECK(!weak.expired()); };scene.storage->BindGumpRuntime(binding);body.Notify(form);CHECK(calls==1 && weak.expired() && ui.controls->RegisteredGump(id)==token);body.Notify(form);ui.controls->CleanupGumps();CHECK(calls==1 && ui.controls->RegisteredGump(id)==0);body.Destroy(form,0x10U);
}
TEST_CASE(ClientGumpOwner_CommonSelectionUsesFullDword) { // 현재 전체 선택 DWORD가 일치할 때만 실제 Client 선택 정리로 이어진다.
    RegionScene scene;auto& body=scene.storage->GumpForms();const auto first=body.Create(2);scene.gameplay->selection.selected=0x10000U|first.value;body.Destroy(first,0x10U);CHECK(scene.clears==0);const auto second=body.Create(2);scene.gameplay->selection.selected=second.value;body.Destroy(second,0x10U);CHECK(scene.clears==1 && scene.gameplay->selection.selected==0);
}
TEST_CASE(ClientGumpOwner_DestroyUsesCurrentDifferentFormVirtual) { // UI에 보관된 실제 SID의 현재 Depend VT를 고정 Gump 몸체로 삭제하지 않는다.
    RegionScene scene;const auto form=scene.storage->Factory().Create(1,2);const auto free=scene.storage->pool.FreeCount();scene.storage->GumpForms().Destroy(form,0x10U);CHECK((scene.storage->pool.Slot(form)[11]&3U)==3U && scene.storage->pool.FreeCount()==free+1 && scene.storage->BaseDestroy().PreDepth()==0);
}
TEST_CASE(ClientGumpOwner_DependantPeerUsesActualGumpDeletion) { // 감시 Pre의 현재 상대 Gump+10도 장기 기본 혼합 분배를 통과한다.
    RegionScene scene;const auto parent=scene.Asset(0x5092d0U),gump=scene.storage->GumpForms().Create(2),depend=scene.storage->Factory().Create(1,2);scene.storage->GumpForms().Attach(gump,parent,0x10U);scene.storage->Factory().AttachForm(depend,parent,0x10U);DependWord(scene.storage->pool,depend,20,gump.value);const auto free=scene.storage->pool.FreeCount();scene.storage->DependForms().Destroy(depend,0x10U);CHECK(scene.storage->pool.FreeCount()==free+2 && (scene.storage->pool.Slot(gump)[11]&3U)==3U && scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}
TEST_CASE(ClientGumpOwner_ParentDeletionReachesRealUiAndReleasesForm) { // 실제 부모 삭제의 종속 통지가 같은 UI 등록표와 Gump 반납을 모두 수행한다.
    RegionScene scene;GumpUiScene ui(scene);auto& body=scene.storage->GumpForms();const auto parent=scene.Asset(0x5092d0U),form=body.Create(2);const auto token=ui.controls->ControlGump(7),id=ui.Owner(token);body.SetOwner(form,id);body.Attach(form,parent,0x10U);const auto free=scene.storage->pool.FreeCount();CHECK(scene.storage->DestroyAsset(parent,0x10U));ui.controls->CleanupGumps();CHECK(ui.controls->RegisteredGump(id)==0 && scene.storage->pool.FreeCount()==free+2 && (scene.storage->pool.Slot(form)[11]&3U)==3U);
}
TEST_CASE(ClientGumpOwner_ActualFloatingMenuUsesCurrentTypeAndUiFormReentry) { // 실제 Reference→owner/Attach→부모 삭제 통지→UI Kill의 Form 재삭제를 같은 본체에서 수행한다.
    RegionScene scene;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");fonts.SetUserCacheDirectory(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-resume-1222/font-cache"); // 새 글꼴 캐시는 보호 원본 밖의 이번 단위 임시 폴더에만 쓴다.
    GumpUiScene ui(scene);RawMenuHooks hooks;hooks.command=RegionForbidden<void,std::uint32_t,std::uint32_t,std::uint32_t>();hooks.readInt=[](auto,std::int32_t&) {};hooks.sound=RegionForbidden<void,std::string_view>();hooks.poll=RegionForbidden<InputEvent,std::uint32_t>();hooks.cursor=RegionForbidden<void,std::uint32_t>();hooks.objectForms=[&]()->GumpFormHost& { return scene.storage->GumpForms(); };hooks.gameSeconds=[] { return 0.0; };auto& menus=ui.controls->EnableMenus(assets.game,fonts,std::move(hooks));
    scene.storage->types[7]=scene.storage->types[3];scene.storage->containedGumpType=7;const auto parent=scene.Asset(0x5092d0U);const auto popup=menus.ShowSp(100,10,20,parent.value);const Sid form{static_cast<std::uint16_t>(RegionRead(scene.storage->pool.Slot(parent),6)&0xffffU)};CHECK(popup && form.value && scene.storage->pool.Slot(form)[10]==7 && scene.storage->GumpForms().Owner(form)>0);const auto free=scene.storage->pool.FreeCount();scene.storage->GumpForms().Notify(form);CHECK((scene.storage->pool.Slot(form)[11]&3U)==3U && scene.storage->pool.FreeCount()==free+1);ui.controls->CleanupGumps();CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}
