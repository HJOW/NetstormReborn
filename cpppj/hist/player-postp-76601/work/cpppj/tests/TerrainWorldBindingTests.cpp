// 초기 도장의 외부 슬롯 모형을 실제 Factory/Owner/Pop/표시/지역 소비자로 교체해 같은 저장소를 확인한다.
#include "RawSceneSupport.h"
#include "client/RawTerrainWorldBinding.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 두 판본의 원본 isle/edge/fringe와 이번 연결 검사에서 생성할 한 지면의 좌표다.
constexpr std::uint32_t kBoundIsle=97,kBoundEdge=98,kBoundFringe=102;
constexpr std::int32_t kBoundX=50,kBoundY=53;
// 실제 월드와 초기 도장이 공유하는 입력 표/문자 지도/난수 및 진단 수명이다.
struct TerrainWorldScene {
    // types: 실제 타입97의 검증된 constructor 주소와 지면 속성이다.
    std::vector<RiftTypeRecord> types;
    // frames: patch quartile/CD 전체 후보가 조회하는 실제 P/P 코드 입력이다.
    std::vector<RiftTypeFrames> frames;
    // shapes: 실제 SquidDisplay/Pop의 발자국·그림 크기 입력이다. 이 검사는 합성 SHP를 쓴다.
    std::vector<SquidDisplayShape> shapes;
    // world: 단일 풀·공간 해시·Frame/Owner/PostPop/Graph/Kernel을 소유한다.
    std::unique_ptr<RawSurfaceWorld> world;
    // codes/regions: 원본 X열 우선 문자/지역BYTE 배열이다.
    std::array<std::uint8_t,65536> codes{},regions{};
    // mapping/aliases: 실제 초기 도장의 mutable 문자 대응 표다.
    std::array<std::int32_t,256> mapping{},aliases{};
    // map: 위 배열과 같은 저장소를 참조하는 도장 context다.
    TerrainStampMap map;
    // state: 초기 CRT99cache 및 현재 지역 없음·특수 타입 전역이다.
    TerrainStampState state;
    // random/crt: 게임 난수와 독립 CRT holdrand다.
    GameRandom random;
    std::uint32_t crt{7};
    // diagnostics: 정상 연결에서는 원본 assertion 요청이 없어야 한다.
    std::uint32_t diagnostics{};
    // 원본 지면 생성자와 합성 SHP를 같은 실제 월드에 공급한다.
    explicit TerrainWorldScene(OriginalEdition edition):types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size(),RiftTypeFrames({})),shapes(types.size()) {
        auto& terrain=types[kBoundIsle];terrain.constructorAddress=TypeConstructorAddress(edition,kBoundIsle);
        terrain.flags1=0x28000803;terrain.flags2=2;terrain.footX=terrain.footY=1;
        std::vector<FrameCode> codesForType;
        // 충분한8후보는 원본 patch quartile/CD 전체 선택을 모두 지원하며 fringe flags는0이다.
        for (std::uint8_t index=0;index<8;++index) codesForType.push_back({'P','P',index,0});
        frames[kBoundIsle]=RiftTypeFrames(std::move(codesForType));
        auto& shape=shapes[kBoundIsle];shape.loaded=true;shape.frameCount=8;
        shape.frames.assign(8,SquidDisplayFrame{20,16,8,12,1,1});
        BridgeConnectState bridges;bridges.bridgeType=82;bridges.islandType=94;bridges.stalagType=95;
        bridges.connectorType=155;bridges.noIslandType=157;bridges.battle=true;
        world=std::make_unique<RawSurfaceWorld>(edition,types,frames,shapes,bridges,kBoundIsle);
        codes.fill('.');regions.fill(127);const auto index=static_cast<std::size_t>(kBoundX)*256+kBoundY;
        codes[index]='x';regions[index]=5;mapping['x']=kBoundIsle;aliases['x']='x';
        map={codes,regions,mapping,aliases,{kBoundX,kBoundY,kBoundX+1,kBoundY+1},1,{}};
    }
    // 호출자에게 남은 원본 지역 조회/진단은 실제 지면 정상 분기에서 요청되면 실패한다.
    TerrainStampHooks Effects() {
        TerrainStampHooks hooks;
        hooks.chunkRegion=[](std::int32_t,std::int32_t)->std::int8_t { throw std::logic_error("지역5 초기 지면은 noRegion 조회를 요청하지 않는다"); };
        hooks.assertion=[this](TerrainStampAssertion) { ++diagnostics; };
        return hooks;
    }
};
}

// 실제 Factory 생성부터 표면 hash0/PostPop/지역 지정/현재 렌더 입력까지 같은SID를 유지한다.
TEST_CASE(TerrainWorldBinding_ActualFactoryOwnerPopAndRegion) {
    // 양판본의 원본 폭과 별도 선택 정책을 각각 같은 월드에서 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        TerrainWorldScene scene(edition);auto core=scene.world->Core();scene.world->BeginLoad();
        RawTerrainStamp stamp(core.pool,core.types,core.frames,scene.map,scene.random,scene.crt,scene.state,
            MakeTerrainStampWorldHooks(*scene.world,scene.state,[&](SquidRegionAssert) { ++scene.diagnostics; },scene.Effects()));
        stamp.Stamp('x');const auto sid=scene.map.lastSid;CHECK(sid.value!=0 && scene.diagnostics==0);
        CHECK(scene.world->Find(kBoundX,kBoundY,kBoundIsle)==sid);const auto raw=core.pool.Slot(sid);
        CHECK(Get(raw,8,2)==5 && raw[10]==kBoundIsle && raw[edition==OriginalEdition::Patch1078 ? 34 : 32]==0);
        CHECK(core.hash.Entries(0)[static_cast<std::size_t>(kBoundY)*256+kBoundX]==sid.value);
        CHECK(core.bookkeeping.globalCounts[kBoundIsle]==1 && core.bookkeeping.depth==0);
        const auto objects=scene.world->Objects();CHECK(objects.size()==1 && objects.front().sid==sid);
        CHECK(objects.front().object.x==kBoundX && objects.front().object.y==kBoundY && objects.front().object.owner==0);
        CHECK(&stamp.Pool()==&core.pool && scene.state.variants.has_value());scene.world->FinishLoad();
    }
}

// 현재 지역 없음 값과 필수 진단/ChunkMap callback을 묵시 기본값으로 바꾸지 않는다.
TEST_CASE(TerrainWorldBinding_RejectsMissingExternalEffects) {
    TerrainWorldScene scene(OriginalEdition::Patch1078);const auto before=scene.world->Report();
    CHECK(Throws([&] { MakeTerrainStampWorldHooks(*scene.world,scene.state,{},scene.Effects()); }));
    CHECK(Throws([&] { MakeTerrainStampWorldHooks(*scene.world,scene.state,[](SquidRegionAssert) {},{}); }));
    CHECK(scene.world->Report()==before && scene.world->Objects().empty());
}
