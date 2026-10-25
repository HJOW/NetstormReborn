// 실제 자산과 기존 월드 모듈로 장기 물리의 대여·주소·초기화 범위를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>

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
