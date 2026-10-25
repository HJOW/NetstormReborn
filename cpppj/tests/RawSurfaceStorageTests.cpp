// 실제 자산과 기존 월드 모듈로 장기 물리의 대여·주소·초기화 범위를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
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
