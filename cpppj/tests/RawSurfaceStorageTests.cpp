#include "o/RawGameAuxState.h"
// 실제 자산과 기존 월드 모듈로 장기 물리의 대여·주소·초기화 범위를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientGameplayGlobals.h"
#include "o/RawTotalMade.h"
#include <algorithm>
#include <cstring>
#include <type_traits>

using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 기존 캠페인과 같은 실제 표면 타입 번호를 공급한다.
BridgeConnectState StorageLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;links.battle=true;links.mission=true;return links; }
// 같은 물리 배열만 기존 RawSurfaceWorld에 직접 빌린다.
std::unique_ptr<RawSurfaceWorld> StorageWorld(const std::shared_ptr<RawSurfaceStorage>& storage,std::uint32_t terrain=97) {
    return std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),terrain,nullptr,storage);
}
// 계약 실패가 기존 물리를 바꾸는 성공 경로로 처리되지 않는지 확인한다.
template<class Function> bool StorageRejected(Function&& function) { try { function();return false; } catch (const std::exception&) { return true; } }
// 고정1530BYTE 기록과4096DWORD를 전체 비교해 reserved와 비활성 stack의 소실도 검출한다.
bool SameStorageGraph(const RawGraphStorage& first,const RawGraphStorage& second) {
    static_assert(sizeof(first.records)==1530);return std::memcmp(first.records.data(),second.records.data(),sizeof(first.records))==0 && first.stack==second.stack;
}
// 세 배열의 값 전체를 비교하며 장부의 선택 포인터를 기대 snapshot으로 복사하지 않는다.
bool SameStorageTotalMade(const RawTotalMadeStorage& first,const RawTotalMadeStorage& second) {
    return first.localCounts==second.localCounts && first.localSecondaryCounts==second.localSecondaryCounts && first.globalCounts==second.globalCounts;
}
}

// 두 판본 모두 같은 실제 Pool/해시/TYPE/spot/셰이프 주소를 월드 모듈이 읽는다.
TEST_CASE(RawSurfaceStorage_ActualBothEditionsAndSamePhysicalArrays) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 실제 보호 자산만 읽으며 파일을 변경하지 않는다.
        const auto& assets=netstorm::test::dudespawn::Assets(edition);auto storage=MakeRawSurfaceStorage(assets.game);CHECK(!storage->Borrowed() && storage->Generation()==0 && storage->Source()==&assets.game);
        auto world=StorageWorld(storage);const auto core=world->Core();CHECK(storage->Borrowed() && storage->Generation()==1 && world->Storage()==storage);
        CHECK(&core.pool==&storage->pool && &core.hash==&storage->hash && &core.ownerMode==&storage->ownerMode);
        CHECK(core.spots.data()==storage->spots.data() && core.types.data()==storage->types.data() && core.frames.data()==storage->frames.data() && core.shapes.data()==storage->shapes.data());
        world.reset();CHECK(!storage->Borrowed() && storage->Generation()==1);
    }
}

// 다음 월드의 실제 Reset(0)이 기존 주소를 유지하며 TYPE 전역은 파일 사본으로 되돌리지 않는다.
TEST_CASE(RawSurfaceStorage_NextWorldResetsPoolAndSpaceWithoutReplacingArrays) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto world=StorageWorld(storage);const auto* poolBytes=storage->pool.Bytes().data();const auto* spots=storage->spots.data();const auto* types=storage->types.data(); // 고정된 실제 배열 주소다.
    const auto sid=world->Create(94);CHECK(sid.value && storage->pool.FreeCount()<storage->pool.Capacity()-5);storage->hash.Cell(0,1,1)=sid.value;storage->spots[257]=0x71;storage->types[0].level=0x13579; // 초기화할 물리와 보존할 TYPE의 명시 입력이다.
    world.reset();CHECK(!storage->Borrowed() && storage->pool.Slot(sid)[10]==94);
    world=StorageWorld(storage);CHECK(storage->Generation()==2 && storage->pool.Bytes().data()==poolBytes && storage->spots.data()==spots && storage->types.data()==types);
    CHECK(storage->pool.FreeCount()==storage->pool.Capacity()-5 && (storage->pool.Slot(sid)[11]&1));CHECK(storage->hash.Cell(0,1,1)==0 && storage->spots[257]==0 && storage->types[0].level==0x13579);
}

// 활성 대여와 잘못된 타입 요청은 실제 풀 초기화보다 먼저 실패한다.
TEST_CASE(RawSurfaceStorage_DuplicateAndInvalidWorldKeepCurrentPhysicalBytes) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto world=StorageWorld(storage);const auto sid=world->Create(94); // 실제 기존 객체가 있는 물리다.
    const auto before=std::vector<std::uint8_t>(storage->pool.Bytes().begin(),storage->pool.Bytes().end());CHECK(StorageRejected([&] { StorageWorld(storage); }));
    CHECK(storage->Borrowed() && storage->Generation()==1 && std::equal(before.begin(),before.end(),storage->pool.Bytes().begin()) && storage->pool.Slot(sid)[10]==94);
    world.reset();CHECK(StorageRejected([&] { StorageWorld(storage,999); }));CHECK(!storage->Borrowed() && storage->Generation()==1 && std::equal(before.begin(),before.end(),storage->pool.Bytes().begin()));
}

// 외부 소유 포인터를 반납해도 실제 월드 대여와 모든 참조가 끝날 때까지 물리가 유지된다.
TEST_CASE(RawSurfaceStorage_LeaseRetainsDataUntilModulesAreGone) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);std::weak_ptr<RawSurfaceStorage> watch=storage;auto world=StorageWorld(storage);const auto* bytes=storage->pool.Bytes().data(); // 관측은 별도 강한 소유자를 만들지 않는다.
    storage.reset();CHECK(!watch.expired() && world->Core().pool.Bytes().data()==bytes);world.reset();CHECK(watch.expired());
}

// Native 정리 호출자가 준비한 물리의 세대가 증가해도 두 번째 초기화 없이 같은 배열/모드를 보존한다.
TEST_CASE(RawSurfaceStorage_PreparedWorldPreservesActualMemoryAndModes) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);
    { auto first=storage->Acquire(); } // 기존 대여를 한 번 거쳐 일반 경로라면 Reset이 선택되는 두 번째 세대로 만든다.
    storage->pool.Reset(0);storage->hash.Reset();std::fill(storage->spots.begin(),storage->spots.end(),std::uint8_t{}); // 앞선 명시 정리의 실제 하위를 호출한다.
    storage->ownerMode={true,false,false};storage->links=StorageLinks();storage->links.battle=false;storage->links.mission=false;
    storage->hash.Cell(0,1,1)=0x1234;storage->spots[257]=0x71;storage->types[0].level=0x13579; // 암묵 재초기화 감지를 위한 물리 입력이며 Native 완료 증거로 쓰지 않는다.
    const auto bytes=std::vector<std::uint8_t>(storage->pool.Bytes().begin(),storage->pool.Bytes().end());const auto* base=storage->pool.Bytes().data();
    auto world=std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true);
    CHECK(storage->Generation()==2 && storage->Borrowed() && world->Core().pool.Bytes().data()==base && std::equal(bytes.begin(),bytes.end(),storage->pool.Bytes().begin()));
    CHECK(storage->hash.Cell(0,1,1)==0x1234 && storage->spots[257]==0x71 && storage->types[0].level==0x13579);
    CHECK(storage->ownerMode.fort && !storage->ownerMode.battle && !storage->ownerMode.challenge && !storage->links.battle && !storage->links.mission);
    CHECK(StorageRejected([&] { storage->AcquirePrepared(); }) && StorageRejected([&] { storage->Acquire(); }));
    world.reset();CHECK(!storage->Borrowed() && storage->hash.Cell(0,1,1)==0x1234 && storage->spots[257]==0x71 && std::equal(bytes.begin(),bytes.end(),storage->pool.Bytes().begin()));
    auto ordinary=storage->Acquire();CHECK(storage->Generation()==3 && storage->hash.Cell(0,1,1)==0 && storage->spots[257]==0); // 기존 독립 설치의 명시 초기화 계약도 유지한다.
}

// World 실행기를 반납해도 실제 기록/stack을 유지하고 다음 준비 대여도 같은 주소를 빌린다.
TEST_CASE(RawSurfaceStorage_GraphPhysicalArraysOutliveWorldAndPreparedLease) {
    static_assert(!std::is_copy_constructible_v<RawSurfaceStorage> && !std::is_move_constructible_v<RawSurfaceStorage>); // 장기 물리 참조의 주소를 복제/이동하지 않는다.
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto first=StorageWorld(storage);const auto* records=storage->graph.records.data();const auto* stack=storage->graph.stack.data(); // 최초 실제 대여의 고정 주소다.
    CHECK(first->Core().graph.Records().data()==records && first->Core().graph.FloodStack().data()==stack);
    storage->graph.records[1]={9,1,0x4567};storage->graph.records[252]={3,4,0x2345};storage->graph.records[254].reserved=0x3456;storage->graph.stack[0]=0x10203040U;storage->graph.stack.back()=0xa1b2c3d4U;const auto current=storage->graph; // Native 완료를 가장하지 않는 비영 보존 입력이다.
    first.reset();CHECK(!storage->Borrowed() && SameStorageGraph(storage->graph,current));
    auto next=std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true); // 준비 대여에는 Graph reset이 없다.
    CHECK(next->Core().graph.Records().data()==records && next->Core().graph.FloodStack().data()==stack && SameStorageGraph(storage->graph,current));
    CHECK(StorageRejected([&] { storage->Acquire(); }) && StorageRejected([&] { storage->AcquirePrepared(); }) && SameStorageGraph(storage->graph,current));
    std::weak_ptr<RawSurfaceStorage> lifetime=storage;storage.reset();CHECK(!lifetime.expired() && next->Core().graph.Records()[252].reserved==0x2345 && next->Core().graph.FloodStack().back()==0xa1b2c3d4U);
    next.reset();CHECK(lifetime.expired()); // 실행기/모듈의 참조와 Use가 끝난 뒤에만 같은 물리가 소멸한다.
}

// 준비 대여 뒤 실행기 생성이 실패해도 lease만 반납하고 이미 있던 기록/stack을 초기화하지 않는다.
TEST_CASE(RawSurfaceStorage_GraphPreparedConstructionFailureKeepsCurrentBytes) {
    std::vector<RiftTypeRecord> types(187);std::vector<RiftTypeFrames> frames(187,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(187); // RawGraph의 PATCH188 계약에서만 실패하도록 같은 크기의 물리를 준비한다.
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);storage->graph.records[0]={2,3,0x4567};storage->graph.records[254]={6,7,0x1234};storage->graph.stack[13]=0x89abcdefU;const auto before=storage->graph; // 생성자 전에 실제 공유 바이트를 고정한다.
    CHECK(StorageRejected([&] { RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true); }));
    CHECK(!storage->Borrowed() && storage->Generation()==1 && SameStorageGraph(storage->graph,before)); // 성공한 대여 세대는 남고 실패 뒤 암묵 rollback/reset은 없다.
}

// 기존 일반 World의 fresh 표/stack 의미는 유지하되 Native partial Reset 및 prepared 보존과 분리한다.
TEST_CASE(RawSurfaceStorage_GraphGeneralAcquireRetainsFreshCompatibility) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game); // 실제 같은 자산으로 기존 일반 설치를 준비한다.
    { auto lease=storage->Acquire(); }storage->graph.records[0].reserved=0x7b23;storage->graph.records[252]={3,4,5};storage->graph.stack[11]=0xa1b2c3d4U;
    ResetRawGraphTable(storage->graph);CHECK(static_cast<std::uint16_t>(storage->graph.records[0].reserved)==0x7b01 && storage->graph.records[252].surfaces==3 && storage->graph.stack[11]==0xa1b2c3d4U); // Native 작은 reset은 지정 범위 밖을 보존한다.
    const auto prepared=storage->graph;{ auto lease=storage->AcquirePrepared();CHECK(SameStorageGraph(storage->graph,prepared)); }CHECK(SameStorageGraph(storage->graph,prepared));
    auto ordinary=StorageWorld(storage);RawGraphStorage expected;ResetRawGraphTable(expected); // 종전 새 지역 Graph의 전체0 뒤 작은 reset과 같은 호환 결과다.
    CHECK(SameStorageGraph(storage->graph,expected) && ordinary->Core().graph.Records().data()==storage->graph.records.data());
    auto independent=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);CHECK(independent->graph.records.data()!=storage->graph.records.data() && independent->graph.stack.data()!=storage->graph.stack.data()); // 다른 Client/물리는 분리한다.
}

// 세 표는 World보다 오래 보존하되 새 장부의 지역 fallback/제어 상태는 이전 World에서 빌리지 않는다. 실제 공유 SID목록은 StateSidLists 검사에서 별도로 확인한다.
TEST_CASE(RawSurfaceStorage_TotalMadeLifetimeAndPreparedControlIsolation) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto first=StorageWorld(storage);const auto* local=storage->totalMade.localCounts.data();const auto* secondary=storage->totalMade.localSecondaryCounts.data();const auto* global=storage->totalMade.globalCounts.data(); // 단일 물리의 고정 주소다.
    for (std::size_t index=0;index<256;++index) { // 마지막 물리 칸까지 모두 비영 현재값을 보존 입력으로 준다.
        storage->totalMade.localCounts[index]=static_cast<std::uint32_t>(index+11);storage->totalMade.localSecondaryCounts[index]=static_cast<std::uint32_t>(index+101);storage->totalMade.globalCounts[index]=static_cast<std::uint32_t>(index+1001);
    }
    const auto current=storage->totalMade;auto firstCore=first->Core();firstCore.bookkeeping.graphsEnabled=true;firstCore.bookkeeping.suppressed=true;firstCore.bookkeeping.depth=7;firstCore.bookkeeping.TotalCost()=91;firstCore.bookkeeping.providers={{3,4},1};firstCore.bookkeeping.ownerFactories[1]={{5,6},1}; // 제어/독립 fallback은 지역이다. 실제 목록 getter와 장기 TotalCost는 별도 물리를 빌린다.
    CHECK(firstCore.bookkeeping.LocalCounts().data()==local && firstCore.bookkeeping.LocalSecondaryCounts().data()==secondary && firstCore.bookkeeping.GlobalCounts().data()==global);first.reset();CHECK(!storage->Borrowed() && SameStorageTotalMade(storage->totalMade,current));
    auto second=std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true);const auto secondCore=second->Core();
    CHECK(secondCore.bookkeeping.LocalCounts().data()==local && secondCore.bookkeeping.LocalSecondaryCounts().data()==secondary && secondCore.bookkeeping.GlobalCounts().data()==global && SameStorageTotalMade(storage->totalMade,current));
    CHECK(!secondCore.bookkeeping.graphsEnabled && !secondCore.bookkeeping.suppressed && secondCore.bookkeeping.depth==0 && secondCore.bookkeeping.TotalCost()==91 && secondCore.bookkeeping.localOwner==1 && secondCore.bookkeeping.providers.entries.empty());
    CHECK(&secondCore.bookkeeping.TotalCost()==&storage->GameAux()->TotalCost() && storage->GameAux()->TotalCost()==91); // prepared는 Native 부모가 직접 쓸 때까지 같은 비용 비트를 보존한다.
    for (const auto& list:secondCore.bookkeeping.ownerFactories) CHECK(!list.current.storage && list.entries.empty() && list.count==0); // Player callback은 이전 장부에서 이월하지 않는다.
    CHECK(StorageRejected([&] { storage->Acquire(); }) && StorageRejected([&] { storage->AcquirePrepared(); }) && SameStorageTotalMade(storage->totalMade,current));
    std::weak_ptr<RawSurfaceStorage> lifetime=storage;storage.reset();CHECK(!lifetime.expired() && secondCore.bookkeeping.GlobalCounts().back()==1256);second.reset();CHECK(lifetime.expired()); // 마지막 실행기/Use가 끝난 뒤만 실제 물리가 소멸한다.
}

// 준비 대여 후 타입 계약에서 실패해도 세 표를 지우거나 성공 이전 값으로 rollback하지 않는다.
TEST_CASE(RawSurfaceStorage_TotalMadePreparedConstructionFailureKeepsAllTables) {
    std::vector<RiftTypeRecord> types(187);std::vector<RiftTypeFrames> frames(187,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(187);auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes); // Graph의 PATCH188 계약에서만 실패하는 동일 크기 입력이다.
    storage->totalMade.localCounts.fill(0x12345678U);storage->totalMade.localSecondaryCounts.fill(0x87654321U);storage->totalMade.globalCounts.fill(0xa1b2c3d4U);const auto before=storage->totalMade; // 세 배열 모두 독립 값 snapshot이다.
    CHECK(StorageRejected([&] { RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true); }));
    CHECK(!storage->Borrowed() && storage->Generation()==1 && SameStorageTotalMade(storage->totalMade,before));
    { auto lease=storage->AcquirePrepared();CHECK(storage->Borrowed() && SameStorageTotalMade(storage->totalMade,before)); }CHECK(!storage->Borrowed() && SameStorageTotalMade(storage->totalMade,before));
}

// Native255 reset/준비 대여 보존과 기존 일반 World의 전체256칸 fresh 준비를 구별한다.
TEST_CASE(RawSurfaceStorage_TotalMadeGeneralFreshIsSeparateFromNativeReset255) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);{ auto lease=storage->Acquire(); }storage->totalMade.localCounts.fill(11);storage->totalMade.localSecondaryCounts.fill(13);storage->totalMade.globalCounts.fill(17); // 초기 대여 뒤의 명시 현재값 입력이다.
    ResetTotalMadeCounts(storage->totalMade);CHECK(storage->totalMade.localCounts[254]==0 && storage->totalMade.localSecondaryCounts[254]==0 && storage->totalMade.globalCounts[254]==0 && storage->totalMade.localCounts[255]==11 && storage->totalMade.localSecondaryCounts[255]==13 && storage->totalMade.globalCounts[255]==17);
    const auto prepared=storage->totalMade;{ auto lease=storage->AcquirePrepared();CHECK(SameStorageTotalMade(storage->totalMade,prepared)); }CHECK(SameStorageTotalMade(storage->totalMade,prepared));
    auto world=StorageWorld(storage);const RawTotalMadeStorage fresh;CHECK(SameStorageTotalMade(storage->totalMade,fresh) && world->Core().bookkeeping.GlobalCounts().data()==storage->totalMade.globalCounts.data());
    auto independent=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);CHECK(independent->totalMade.localCounts.data()!=storage->totalMade.localCounts.data() && independent->totalMade.localSecondaryCounts.data()!=storage->totalMade.localSecondaryCounts.data() && independent->totalMade.globalCounts.data()!=storage->totalMade.globalCounts.data()); // 서로 다른 Client 물리는 alias하지 않는다.
}

// 같은 장기 표를 빌리는 다음 World도 새 ownerFactories에 현재 Player9를 다시 연결할 수 있다.
TEST_CASE(RawSurfaceStorage_TotalMadeSharingLeavesPlayerFactoryBindingsWorldLocal) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);ClientPlayerRecords records; // Player는 두 World가 끝날 때까지 살아 있는 별도 실제 소유자다.
    for (std::uint32_t pass=0;pass<2;++pass) { // 이전 World 반납 뒤만 Player 실제 재할당과 다음9개 바인딩을 수행한다.
        auto players=records.Acquire();players->Initialize([](std::uint32_t,std::uint32_t) {}); // 이 검사의 비선택 동맹 장치이며 Player/factory 물리는 기존 Native 부모가 만든다.
        auto world=pass ? std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true) : StorageWorld(storage);const auto core=world->Core();
        for (std::uint32_t owner=0;owner<9;++owner) { // 매 World의 목록은 새 바인딩을 받아 같은 현재 Player+0/+4/+8을 직접 읽는다.
            auto& list=core.bookkeeping.ownerFactories[owner];CHECK(!list.current.storage && list.entries.empty() && list.count==0);players->BindFactory(list,owner);CHECK(list.Count()==0 && list.Capacity()==30 && netstorm::test::rawscene::Get(records.Records(),owner*180+8)==0);
            const auto value=0x12340000U+owner;list.Append(value);CHECK(list.Count()==1 && list.Storage()[0]==value && list.Items().size()==1 && netstorm::test::rawscene::Get(records.Records(),owner*180+8)==1); // 공개 현재Player BYTE와 실제Append store로 바인딩을 확인한다.
        }
        CHECK(core.bookkeeping.GlobalCounts().data()==storage->totalMade.globalCounts.data() && core.bookkeeping.GlobalCounts()[84]==(pass ? 77U : 0U));core.bookkeeping.GlobalCounts()[84]=77;world.reset();CHECK(!storage->Borrowed() && storage->totalMade.globalCounts[84]==77);
    }
}

// 실제 World 교체와 prepared 대여는 같은3204BYTE/256×12BYTE·mode를 소유하며 대여중의 일반 fresh를 거부한다.
TEST_CASE(RawSurfaceStorage_IslandAndChunkPreparedLeaseKeepsPhysicalBytes) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto world=StorageWorld(storage);
    const auto* islands=storage->islands.records.data();const auto* chunks=storage->chunks.Entries().data();const auto* mode=&storage->chunks.Mode(); // 세 실제 물리 주소를 전후 비교한다.
    storage->islands.records.fill(0xa5);storage->chunks.Mode()=0x89abcdefU;storage->chunks.At(3,4)={7,'P',{1,2,3,4,5,6},71};const auto before=storage->islands.records;const auto entry=storage->chunks.At(3,4); // Native 완료를 가장하지 않는 보존 입력이다.
    world.reset();CHECK(!storage->Borrowed() && storage->islands.records==before && storage->chunks.Mode()==0x89abcdefU);
    world=std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true);
    CHECK(world->Storage()->islands.records.data()==islands && world->Storage()->chunks.Entries().data()==chunks && &world->Storage()->chunks.Mode()==mode);
    CHECK(storage->islands.records==before && storage->chunks.Mode()==0x89abcdefU && storage->chunks.At(3,4).record==entry.record && storage->chunks.At(3,4).ordinal==71);
    CHECK(StorageRejected([&] { storage->Acquire(); }) && StorageRejected([&] { storage->AcquirePrepared(); }) && storage->islands.records==before && storage->chunks.At(3,4).record==entry.record);
    world.reset();CHECK(!storage->Borrowed() && storage->islands.records==before && storage->chunks.Mode()==0x89abcdefU);
}

// prepared 대여 뒤 실제 Graph188종류 계약이 실패해도 앞서 준비한 섬/청크 물리는 그대로 남는다.
TEST_CASE(RawSurfaceStorage_IslandAndChunkPreparedFailureDoesNotReset) {
    std::vector<RiftTypeRecord> types(187);std::vector<RiftTypeFrames> frames(187,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(187); // 기존 Graph 실패와 같은 실제 미충족 입력이다.
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);storage->islands.records.fill(0x71);storage->chunks.Mode()=0x12345678;storage->chunks.At(2,3)={9,'F',{6,5,4,3,2,1},37};const auto before=storage->islands.records;
    CHECK(StorageRejected([&] { RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true); }));
    CHECK(!storage->Borrowed() && storage->Generation()==1 && storage->islands.records==before && storage->chunks.Mode()==0x12345678 && storage->chunks.At(2,3).ordinal==37 && storage->chunks.At(2,3).record[0]==6);
}

// 원문 partial reset의 padding 보존과 기존 일반 Session의 전체 fresh 입력을 같은 주소에서 구별한다.
TEST_CASE(RawSurfaceStorage_IslandAndChunkGeneralAcquireRetainsFreshCompatibility) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);const auto* islands=storage->islands.records.data();const auto* chunks=storage->chunks.Entries().data();
    { auto use=storage->Acquire(); }storage->islands.records.fill(0xa5);storage->chunks.Mode()=0x89abcdefU;storage->chunks.At(3,4).record.fill(0x71);
    ResetRawIslandRecords(storage->islands.records);storage->chunks.Reset();CHECK(storage->islands.records[18]==0xa5 && storage->islands.records[19]==0xa5 && storage->chunks.Mode()==0x89abcdefU);
    const auto before=storage->islands.records;{ auto use=storage->AcquirePrepared();CHECK(storage->islands.records==before && storage->chunks.Mode()==0x89abcdefU); }
    { auto use=storage->Acquire();CHECK((storage->islands.records.data()==islands && storage->chunks.Entries().data()==chunks && storage->islands.records[18]==0 && storage->islands.records[19]==0 && storage->chunks.Mode()==1 && storage->chunks.At(3,4).record==std::array<std::uint8_t,6>{})); }
}

// 같은9DWORD는 World 반납·다음prepared 대여에서 유지되고 일반 Acquire만 종전 지역fresh0 의미를 유지한다.
TEST_CASE(RawSurfaceStorage_PriestOwnerStateHasLongLifetimeAndSeparateFreshPolicy) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);const auto* address=storage->priestOwnerState.data();CHECK((storage->priestOwnerState==std::array<std::uint32_t,kPlayerCount+1>{}));
    auto first=StorageWorld(storage);for (std::size_t index=0;index<storage->priestOwnerState.size();++index) storage->priestOwnerState[index]=static_cast<std::uint32_t>(0x1000+index); // 실제9칸의 현재 값을 첫 World 수명에 쓴다.
    auto expected=storage->priestOwnerState;first.reset();CHECK(!storage->Borrowed() && storage->priestOwnerState==expected);
    auto next=std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true);CHECK(storage->priestOwnerState.data()==address && storage->priestOwnerState==expected);
    ResetPriestOwnerState(storage->priestOwnerState,8);expected[8]=0;CHECK(storage->priestOwnerState==expected && storage->priestOwnerState[0]==0x1000U);
    CHECK(StorageRejected([&] { storage->Acquire(); }) && StorageRejected([&] { storage->AcquirePrepared(); }) && storage->priestOwnerState==expected);next.reset();CHECK(storage->priestOwnerState==expected);
    auto ordinary=StorageWorld(storage);CHECK(storage->priestOwnerState.data()==address && (storage->priestOwnerState==std::array<std::uint32_t,kPlayerCount+1>{})); // 일반 fresh0은 Native 지정1..8 reset이 아니므로0번도 종전처럼0이다.
}

// prepared 실행기 생성 실패/선행 타입 거부는 이미 있던9DWORD를 바꾸거나 별도 표로 교체하지 않는다.
TEST_CASE(RawSurfaceStorage_PriestOwnerStateSurvivesPreparedConstructionFailure) {
    std::vector<RiftTypeRecord> types(187);std::vector<RiftTypeFrames> frames(187,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(187);auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);
    storage->priestOwnerState.fill(0x76543210U);const auto expected=storage->priestOwnerState;const auto* address=storage->priestOwnerState.data();
    CHECK(StorageRejected([&] { RawSurfaceWorld invalid(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),999,nullptr,storage,true); }));CHECK(storage->Generation()==0 && !storage->Borrowed() && storage->priestOwnerState==expected);
    CHECK(StorageRejected([&] { RawSurfaceWorld partial(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true); }));CHECK(storage->Generation()==1 && !storage->Borrowed() && storage->priestOwnerState==expected && storage->priestOwnerState.data()==address);
}

// 같은 실제 Gameplay/Storage/Process/Pool 주소가 prepared 대여와 World 소멸 뒤에도 살아 있다.
TEST_CASE(RawSurfaceStorage_AuthorityOwnerSharesCurrentValuesAndOutlivesWorld) {
    auto globals=std::make_shared<ClientGameplayGlobals>();auto owner=globals->Authority();auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,owner,globals->GameAux());auto first=StorageWorld(storage);
    auto core=first->Core();CHECK(owner->server && owner->boss && storage->Authority()==owner && &core.processState.Server()==&globals->Server() && &core.processState.Boss()==&globals->Boss());
    globals->Server()=false;globals->Boss()=false;core.processState.server=core.processState.boss=true;CHECK(!core.processState.Server() && !core.processState.Boss() && !core.pool.IsServer()); // 반대 fallback을 owner에 복사하지 않는다.
    first.reset();CHECK(!storage->Borrowed() && !globals->Server() && !globals->Boss());auto lease=globals->AcquirePrepared();CHECK(!globals->Server() && !globals->Boss());
    auto second=std::make_unique<RawSurfaceWorld>(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,storage->links,97,nullptr,storage,true,nullptr,owner);const auto next=second->Core();CHECK(!next.processState.Server() && !next.processState.Boss() && !next.pool.IsServer());
    std::weak_ptr<RawAuthorityState> lifetime=owner;owner.reset();lease.reset();globals.reset();storage.reset();CHECK(!lifetime.expired()); // 살아 있는 World의 Storage만 작은 owner를 계속 소유한다.
    next.processState.Server()=true;next.processState.Boss()=false;CHECK(next.pool.IsServer() && !next.processState.Boss());second.reset();CHECK(lifetime.expired());
}

// 일반 설치만 종전 fresh true/true로 시작하고 prepared/실패/중복은 현재 권한을 보존한다.
TEST_CASE(RawSurfaceStorage_AuthorityFreshPreparedAndIdentityFailureAreDistinct) {
    auto globals=std::make_shared<ClientGameplayGlobals>();auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,globals->Authority(),globals->GameAux());globals->Server()=false;globals->Boss()=false;
    auto other=std::make_shared<RawAuthorityState>();CHECK(StorageRejected([&] { RawSurfaceWorld wrong(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,storage->links,97,nullptr,storage,false,nullptr,other); }));
    CHECK(!storage->Borrowed() && storage->Generation()==0 && !globals->Server() && !globals->Boss());auto first=StorageWorld(storage);CHECK(globals->Server() && globals->Boss());globals->Server()=false;globals->Boss()=true;
    CHECK(StorageRejected([&] { StorageWorld(storage); }));CHECK(!globals->Server() && globals->Boss());first.reset();auto prepared=storage->AcquirePrepared();CHECK(!globals->Server() && globals->Boss());prepared.reset();CHECK(!globals->Server() && globals->Boss());
    auto ordinary=storage->Acquire();CHECK(globals->Server() && globals->Boss());ordinary.reset();globals->Server()=globals->Boss()=false;auto preparedGlobals=globals->AcquirePrepared();preparedGlobals.reset();CHECK(!globals->Server() && !globals->Boss());
    auto freshGlobals=globals->Acquire();CHECK(globals->Server() && globals->Boss());globals->Server()=false;freshGlobals.reset();CHECK(!globals->Server() && globals->Boss()); // 반납은 권한 초기화가 아니다.
    std::vector<RiftTypeRecord> types(187);std::vector<RiftTypeFrames> frames(187,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(187);auto failing=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr,globals->Authority(),globals->GameAux());globals->Boss()=false;
    CHECK(StorageRejected([&] { RawSurfaceWorld invalid(OriginalEdition::Patch1078,types,frames,shapes,StorageLinks(),97,nullptr,failing,true,nullptr,globals->Authority(),globals->GameAux()); }));CHECK(!failing->Borrowed() && !globals->Server() && !globals->Boss()); // prepared 실행기 생성 실패는 앞선 실제 두 bool을 복구/초기화하지 않는다.
}

// 두 PE의 Allocate/Release 관문은 생성 때 bool 사본이 아니라 각 호출의 같은 현재 서버값을 읽는다.
TEST_CASE(RawSurfaceStorage_AuthorityCurrentPoolGatesAndIndependentOwners) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별 서버 첫 번호와 같은 실제 장치를 검증한다.
        auto first=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(edition).game);auto second=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(edition).game);CHECK(first->Authority()!=second->Authority());
        auto& authority=*first->Authority();authority.server=false;authority.boss=false;CHECK(!first->pool.IsServer() && second->pool.IsServer() && second->Authority()->boss);
        const auto before=first->pool.FreeCount();CHECK(StorageRejected([&] { first->pool.Allocate(0); }));CHECK(first->pool.FreeCount()==before);const auto client=first->pool.Allocate(2);CHECK(client==Sid{5});
        authority.server=true;const auto server=first->pool.Allocate(0);CHECK(server.value==first->pool.Layout().serverFirst);const auto free=first->pool.FreeCount();const auto tail=first->pool.Tail(false);
        authority.server=false;first->pool.Release(server);CHECK(first->pool.FreeCount()==free && first->pool.Tail(false)==tail && (first->pool.Slot(server)[11]&1U)); // 비서버의 server 슬롯 반납은 FIFO/freeCount를 늘리지 않는다.
        authority.server=true;const auto next=first->pool.Allocate(0);CHECK(next.value==server.value+1);first->pool.Release(next);CHECK(first->pool.FreeCount()==free && first->pool.Tail(false)==next);
        first->pool.Release(client);CHECK(first->pool.FreeCount()==free+1 && first->pool.Tail(true)==client);CHECK(!authority.boss && second->Authority()->server && second->Authority()->boss);
        SidPool standalone(edition,32768,false);CHECK(!standalone.IsServer() && StorageRejected([&] { standalone.Allocate(0); })); // 미연결 순수 fallback은 종전 동작을 유지한다.
    }
}


namespace { // 서로 떨어진17주소를 packed image나 같은 자료형 배열로 가장하지 않는다.
// 각 실제 폭의 비트만 모아 생성/반납 전후 전체 필드를 대조한다.
std::array<std::uint64_t,17> GameAuxValues(const RawGameAuxState& owner) {
    return {owner.OrdinaryGeysers(),owner.GeyserFrameFlag(),owner.SavedGeyserCount(),std::bit_cast<std::uint64_t>(owner.GeyserDeadline()),std::bit_cast<std::uint64_t>(owner.CleanupDeadline()),std::bit_cast<std::uint64_t>(owner.SecondaryGeyserDeadline()),std::bit_cast<std::uint64_t>(owner.ScheduledDrawDeadline()),std::bit_cast<std::uint32_t>(owner.TotalCost()),owner.ResourcePoolBits(),owner.RefundPercent(),std::bit_cast<std::uint32_t>(owner.AllowAnyCapture()),owner.Archipelago(),std::bit_cast<std::uint32_t>(owner.LostX()),std::bit_cast<std::uint32_t>(owner.LostY()),std::bit_cast<std::uint32_t>(owner.DefaultSp()),owner.GeyserType(),owner.SubscriberMarker()};
}
// 17개 모두 구별되는 현재 입력이며 실제 World/Session 계산의 결과를 합성하지 않는다.
void SetGameAuxValues(RawGameAuxState& owner) {
    owner.OrdinaryGeysers()=17;owner.GeyserFrameFlag()=19;owner.SavedGeyserCount()=23;owner.GeyserDeadline()=-0.0;owner.CleanupDeadline()=29.25;owner.SecondaryGeyserDeadline()=31.5;owner.ScheduledDrawDeadline()=37.75;owner.TotalCost()=-41;owner.StoreResourcePoolBits(0xfedcba98U);owner.RefundPercent()=43;owner.AllowAnyCapture()=-47;owner.Archipelago()=0x12345678U;owner.LostX()=std::bit_cast<float>(0xc61c4000U);owner.LostY()=-0.0F;owner.DefaultSp()=-53;owner.GeyserType()=123;owner.SubscriberMarker()=59;
}
}
// 보호PE의 실제 세 명시 초기값과 나머지BSS+0 및 서로 다른 Client 주소를 확인한다.
TEST_CASE(GameAux_PeDefaultsAndIndependentTypedOwner) {
    ClientGameplayGlobals first,second;const auto owner=first.GameAux();const auto values=GameAuxValues(*owner);
    for (std::size_t index=0;index<values.size();++index) { // uint32/int32/float/double을 같은ref로 alias하지 않고 관측한 비트를 비교한다.
        const auto expected=index==9 ? 25U : index==14 ? 2000U : index==15 ? 122U : 0U;CHECK(values[index]==expected);
    }
    CHECK(first.GameAux()!=second.GameAux() && &first.GeyserType()==&owner->GeyserType());SetGameAuxValues(*owner);CHECK(second.GameAux()->ResourcePool()==0 && second.GeyserType()==122 && second.GameAux()->DefaultSp()==2000);
    owner->ResetLostPositionForCleanup();CHECK(std::bit_cast<std::uint32_t>(owner->LostY())==0xc61c4000U && std::bit_cast<std::uint32_t>(owner->LostX())==0xc61c4000U); // cleanup의Y→X -10000은 최초/fresh+0과 별개다.
}
// 실제 World의 준비 대여와 반납은17필드를 보존하고 일반fresh만 종전 지역 입력을 준비한다.
TEST_CASE(GameAux_PreparedWorldAliasesCostAndLossAndGeneralFreshIsExplicit) {
    auto gameplay=std::make_shared<ClientGameplayGlobals>();const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux());auto owner=gameplay->GameAux();SetGameAuxValues(*owner);const auto before=GameAuxValues(*owner);
    { RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,true,nullptr,gameplay->Authority(),owner);auto core=world.Core();
      CHECK(storage->GameAux()==owner && &storage->GeyserType()==&gameplay->GeyserType() && &core.bookkeeping.TotalCost()==&owner->TotalCost());CHECK(GameAuxValues(*owner)==before && &world.DeletionState().LostX()==&owner->LostX() && &world.DeletionState().LostY()==&owner->LostY());
      CHECK(StorageRejected([&] { storage->Acquire(); }));CHECK(GameAuxValues(*owner)==before); }
    CHECK(!storage->Borrowed() && GameAuxValues(*owner)==before);
    { auto use=storage->Acquire();CHECK(owner->ResourcePool()==0 && owner->TotalCost()==0 && owner->RefundPercent()==25 && owner->OrdinaryGeysers()==0 && owner->AllowAnyCapture()==0 && owner->Archipelago()==0 && owner->GeyserType()==122);CHECK(std::bit_cast<std::uint32_t>(owner->LostX())==0 && std::bit_cast<std::uint32_t>(owner->LostY())==0);
      CHECK(owner->GeyserFrameFlag()==19 && owner->SavedGeyserCount()==23 && std::bit_cast<std::uint64_t>(owner->GeyserDeadline())==0x8000000000000000ULL && owner->CleanupDeadline()==29.25 && owner->SecondaryGeyserDeadline()==31.5 && owner->ScheduledDrawDeadline()==37.75 && owner->DefaultSp()==-53 && owner->SubscriberMarker()==59); }
}
// 두 서비스의 다른 게임owner는 대여/기존값초기화 전에 거부하며 같은 권한만으로 통과하지 않는다.
TEST_CASE(GameAux_WorldIdentityFailurePrecedesBorrowAndReset) {
    ClientGameplayGlobals gameplay;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game,gameplay.Authority(),gameplay.GameAux());SetGameAuxValues(*gameplay.GameAux());const auto before=GameAuxValues(*gameplay.GameAux());auto foreign=std::make_shared<RawGameAuxState>();
    CHECK(StorageRejected([&] { RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,StorageLinks(),97,nullptr,storage,false,nullptr,gameplay.Authority(),foreign); }));CHECK(!storage->Borrowed() && storage->Generation()==0 && GameAuxValues(*gameplay.GameAux())==before && foreign->ResourcePool()==0);
}
