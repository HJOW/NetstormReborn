// 실제 일반 지형 소비자의 생성자·도장·군집·공간·지역·장부와 서로 다른 난수 저장소를 확인한다.
#include "RawSceneSupport.h"
#include "client/RawNormalTerrainWorldBinding.h"
#include <type_traits>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원본 타입 번호이며 프레임/SHP는 연결 검사에 명시한 합성 입력이다.
constexpr std::uint32_t kNormalIsle=97,kNormalFarm=121,kNormalPiece=107;
// 원본97의0생성자 계약이 쓰는 base표와121의실제Damageable표를 분리한다.
constexpr std::uint32_t kPatchNormalBase=0x501dd0,kCdNormalBase=0x5058d8,kPatchNormalFarm=0x50a8e8,kCdNormalFarm=0x500b10;
// 내부 콜백이 빌린 모듈 주소를 이동/복사하지 않는다.
static_assert(!std::is_copy_constructible_v<RawNormalTerrainWorldBinding> && !std::is_move_constructible_v<RawNormalTerrainWorldBinding>);
// 두 판본의 같은 실제 월드와 수명이 긴 원본 전역/섬/배치/HP/후처리를 소유한다.
struct NormalWorldScene {
    std::vector<RiftTypeRecord> types; // 실제 지면/121 생성자 주소와 합성 타입 속성 입력이다.
    std::vector<RiftTypeFrames> frames; // 모든 방향·변형 검색과 AA 직접 프레임을 지원하는 합성 코드다.
    std::vector<SquidDisplayShape> shapes; // 실제 Shape Pop/표시에 쓰는 합성 SHP 메타다.
    std::unique_ptr<RawSurfaceWorld> world; // Pool/Factory/hash/spots/장부/가상 공간은 한 월드가 소유한다.
    ChunkMap chunks; // 성장·도장·군집의 현재 청크 지역/seed/패턴 레코드다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> islandBytes{}; // 실제 mutable3204BYTE 원본 목록이다.
    FortFinishOwnerState modes; // 원래ownerBYTE와 현재ownerDWORD를 읽는 기존 모드다.
    SquidPostPopList residences; // owner setter가 현재 읽을 실제 레지던스 목록이다.
    std::uint32_t revision{},changed{},diagnostics{}; // 지역 revision/변경 전역과 진단 관찰 수다.
    std::unique_ptr<RawIslandRuntime> islands; // 같은 Pool/hash와 위 원본 바이트를 빌린다.
    std::vector<PriestPlainCanonType> plainFrames; // 기존 pipeline의 프레임 인수 형식으로 같은 불변 입력을 보관한다.
    std::vector<PriestTypeHotspot> hotspots; // 기존 실제 픽셀 모양 조회가 쓰는 타입별 기준점 입력이다.
    CanonPlacementPipelineState pipelineState; // 강제 허용 없이 실제 공간·지형·최종 관계를 실행한다.
    SquidPostPopList additional; // 현재 Player 기준점 순회가 읽는 추가 목록이다.
    ContainedFinderState contained; // 같은 Pool의 실제 내용물 finder 상태다.
    std::unique_ptr<RawCanonPlacementPipeline> pipeline; // 같은 types/SHP/hash/spots/장부의 실제 MayPlace다.
    GameRandom random{0x12345678}; // 프레임 선택이 소비하는 게임 난수 저장소다.
    std::uint32_t crt{7}; // 초기99cache만 소비하는 독립 CRT holdrand다.
    TerrainContextGlobals globals; // 실제 정상 문자 mapping과 지역 없음 전역이다.
    TerrainStampState stamp; // 두 번의 초기 지형 요청이 공유하는 수명이 긴99cache다.
    FortRandomPlacementState placement; // 실제 지면/edgeFarm 번호와 현재 ownerOverride다.
    FarmClusterState farm; // 정상 군집이며 군도 요청 검사는 별도다.
    std::uint32_t puzzlePiece{kNormalPiece}; // 패턴Count가 매번 읽는 현재 타입 번호다.
    SquidRewardState hpState; // 기존 HP getter의 실제 상태다. 경제 효과는 이 검사에서 호출하지 않는다.
    std::unique_ptr<ScrambledSpStore> sp; // 패치 HP 모듈이 요구하는 실제 SP 저장소다.
    std::unique_ptr<SquidReward> hp; // 같은 Pool/장부/타입의 실제 HP 조회다.
    CarrierPostPopState carrierState; // 실제 Damageable→base 후처리의 로딩 전역이다.
    std::unique_ptr<RawCarrierPostPop> carrier; // 실제121 가상 후처리를 명시 등록한다.
    std::unique_ptr<RawNormalTerrainWorldBinding> binding; // 새 소비자는 위 완료 모듈의 참조를 공유한다.

    // 실제 생성자와 합성 자산 입력을 같은 표면 월드에 넣고 모든 필수 효과를 연결한다.
    explicit NormalWorldScene(OriginalEdition edition,bool randomFarm=false):types(edition==OriginalEdition::Patch1078 ? 188U : 171U),
        frames(types.size(),RiftTypeFrames({})),shapes(types.size()),plainFrames(types.size()),hotspots(types.size()) {
        std::vector<FrameCode> terrainCodes;
        // 모든16방향×16변형은 검색 실패나 임의 프레임 fallback 없이 원본 Choose를 실행하게 한다.
        for (std::uint8_t side='A';side<='P';++side)
            // 프레임 수256 안에서 AA의 J후속36프레임도 물리 범위를 유지한다.
            for (std::uint8_t variant='A';variant<='P';++variant) terrainCodes.push_back({side,variant,1,0});
        frames[kNormalIsle]=RiftTypeFrames(terrainCodes);frames[kNormalFarm]=RiftTypeFrames(std::move(terrainCodes));
        std::vector<FrameCode> pieceCodes;
        // 영역 패턴 칸의 실제 회전된 방향·P변형·번호1 검색을 지원한다.
        for (std::uint8_t side='A';side<='P';++side) pieceCodes.push_back({side,'P',1,0});
        frames[kNormalPiece]=RiftTypeFrames(std::move(pieceCodes));
        // 지면과121은 실제 원본 생성자를 사용하며121의Damageable가상표를 base로 대신하지 않는다.
        for (const auto type:{kNormalIsle,kNormalFarm}) {
            auto& record=types[type];record.constructorAddress=TypeConstructorAddress(edition,type);
            record.flags1=type==kNormalIsle ? 0x28000803U : randomFarm ? 0x00100812U : 0x00200812U;
            record.flags2=type==kNormalIsle ? 2U : 0x80000U;record.footX=record.footY=1;record.maxHitPoints=73;
            shapes[type]={256,true,std::vector<SquidDisplayFrame>(256,{20,16,8,12,1,1})};
        }
        BridgeConnectState bridges;bridges.bridgeType=82;bridges.islandType=94;bridges.stalagType=95;
        bridges.connectorType=155;bridges.noIslandType=157;bridges.battle=true;
        world=std::make_unique<RawSurfaceWorld>(edition,types,frames,shapes,bridges,kNormalIsle);const auto core=world->Core();
        islands=std::make_unique<RawIslandRuntime>(core.pool,core.hash,islandBytes,modes,residences,revision,changed,
            IslandRuntimeHooks{[&owner=core.owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); },
                [this](IslandRuntimeAssertion) { ++diagnostics; }});
        islands->Reset();CHECK(islands->Add(2,3)==0 && islands->Add(3,4)==1);
        // 두 실제 섬은 서로 다른 청크/원래owner/현재owner를 유지한다.
        for (std::int32_t region=0;region<2;++region) {
            auto& chunk=chunks.At(3+region,3);chunk.island=static_cast<std::uint8_t>(region);chunk.side='A';
            chunk.record={3,1,0,static_cast<std::uint8_t>(7+region*13),0,0};
        }
        // 기존 pipeline의 래퍼형 프레임은 입력이 동일하며 현재 SHP/spots/hash/타입은 실제 Core span을 빌린다.
        for (std::size_t type=0;type<plainFrames.size();++type) plainFrames[type].frames=core.frames[type];
        pipelineState.geometry.patternTypes={107,82,94,157,131,129,140,142};pipelineState.placement.localPlayer=1;
        pipelineState.permission.graphReady=0;pipelineState.permission.alliances.fill(1);
        // 실제121은 특수 vortex 제한 목록을 조회하지 않는다. 필요 없는 island 사본을 만들지 않는다.
        pipeline=std::make_unique<RawCanonPlacementPipeline>(core.pool,core.hash,
            CanonPlacementPipelineData{core.types,plainFrames,core.shapes,hotspots,core.spots,core.hash.Entries(0),{}},
            core.bookkeeping,additional,contained,pipelineState);
        sp=std::make_unique<ScrambledSpStore>(random,[] { return 0x12345678U; });
        hp=std::make_unique<SquidReward>(core.pool,core.types,core.bookkeeping,hpState,
            edition==OriginalEdition::Patch1078 ? sp.get() : nullptr);
        carrierState.loadingDepth=1;
        carrier=std::make_unique<RawCarrierPostPop>(core.pool,core.types,carrierState,
            MakeCarrierPostPopHooks(core.pool,core.postPop,[](Sid) { throw std::logic_error("121은 Carrier +cc를 요청하지 않는다"); },
                [this](float x,float y,std::uint8_t owner) { world->SetSupportOwner(x,y,owner); }));
        binding=std::make_unique<RawNormalTerrainWorldBinding>(*world,chunks,*islands,random,crt,globals,stamp,
            placement,farm,puzzlePiece,*pipeline,*hp,*carrier,Diagnostics());
    }
    // 진단의 실제 요청 여부를 관찰하며 누락 효과를 기본값으로 채우지 않는다.
    NormalTerrainDiagnostics Diagnostics() {
        return {[this](TerrainContextAssertion) { ++diagnostics; },[this](TerrainStampAssertion) { ++diagnostics; },
            [this](SquidRegionAssert) { ++diagnostics; },[this](FarmClusterAssertion) { ++diagnostics; },
            [this](TerrainFarmPostpassAssertion) { ++diagnostics; }};
    }
    // 예상 결과를 생성하는 모형 없이 기존 FrameAt만 호출해 같은 게임/CRT 난수 소비를 사전 관찰한다.
    std::size_t ExpectedFrames(const NormalTerrainRequest& request,GameRandom& expected,std::uint32_t& expectedCrt,
        TerrainStampState& expectedState) {
        const auto core=world->Core();RawTerrainContext context(core.pool.Edition(),request.owner,request.selectedRegion,request.bounds,
            globals,[this](TerrainContextAssertion) { ++diagnostics; });
        RawTerrainGrowth growth(context,chunks,globals);growth.RunNormal();context.MarkEdges('x','=');
        TerrainStampHooks effects;effects.chunkRegion=[this](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(chunks.At(x,y).island); };
        effects.assertion=[this](TerrainStampAssertion) { ++diagnostics; };
        RawTerrainStamp chooser(core.pool,core.types,core.frames,context.Map(),expected,expectedCrt,expectedState,
            MakeTerrainStampWorldHooks(*world,expectedState,[this](SquidRegionAssert) { ++diagnostics; },std::move(effects)));
        std::size_t count=0;
        // 실제 초기 도장의 Y/X 순서에서 매 실제 지면 프레임 요청만 관찰한다.
        for (auto y=context.Map().bounds[1];y<context.Map().bounds[3];++y)
            // 생성/Pop 없이 실제 FrameAt이 게임/CRT 저장소만 갱신한다.
            for (auto x=context.Map().bounds[0];x<context.Map().bounds[2];++x) {
                const auto code=context.Codes()[static_cast<std::size_t>(x)*256+y];
                if (context.Map().aliases[code]=='x' && context.Map().typeMapping[code]) { chooser.FrameAt(x,y);++count; }
            }
        return count;
    }
};
}

// 같은 Factory/Pool/공간/지역/장부로 두 섬을 실제 성장·도장·군집 후처리하며 캐시는 첫 요청만 소비한다.
TEST_CASE(NormalTerrainWorldBinding_ActualNormalGenerationAndFarmRetype) {
    // 두 판본의 프레임 폭/선택 정책과 같은 현재 지역 바이트를 각각 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        NormalWorldScene scene(edition);const auto core=scene.world->Core();scene.world->BeginLoad();std::size_t total=0,farmCount=0;
        CHECK(&scene.binding->Pool()==&core.pool && &scene.hp->Bookkeeping()==&core.bookkeeping);
        // 첫/두 번째 요청의 CRT cache는 하나이며 선택 지역만 바뀐다.
        for (std::int32_t region=0;region<2;++region) {
            const NormalTerrainRequest request{1,{3,3,5,4},region,NormalTerrainBranch::Normal};
            GameRandom expected(scene.random.State());auto expectedCrt=scene.crt;auto expectedState=scene.stamp;
            const auto count=scene.ExpectedFrames(request,expected,expectedCrt,expectedState);CHECK(count>0);total+=count;
            const auto previousCrt=scene.crt;const auto last=scene.binding->Generate(request);CHECK(last.value!=0);
            CHECK(scene.random.State()==expected.State() && scene.crt==expectedCrt && scene.stamp.variants==expectedState.variants);
            CHECK(region==0 ? scene.crt!=previousCrt : scene.crt==previousCrt);
        }
        const auto objects=scene.world->Objects();CHECK(objects.size()==total && scene.diagnostics==0);
        // 실제 생성한 모든 SID의 지면/121 가상표와 현재 hash 단계·지역WORD·HP를 확인한다.
        for (const auto& object:objects) {
            const auto raw=core.pool.Slot(object.sid);const auto region=Get(raw,8,2);CHECK(region<2);
            CHECK(object.object.owner==0 && object.object.x>=48 && object.object.x<80 && object.object.y>=48 && object.object.y<64);
            const auto vtable=raw[10]==kNormalFarm ? (edition==OriginalEdition::Patch1078 ? kPatchNormalFarm : kCdNormalFarm) :
                (edition==OriginalEdition::Patch1078 ? kPatchNormalBase : kCdNormalBase);
            CHECK(Get(raw,0)==vtable);
            CHECK(scene.world->Find(object.object.x,object.object.y,raw[10])==object.sid);
            const auto level=raw[edition==OriginalEdition::Patch1078 ? 33 : 31];
            bool found=false;std::uint32_t inspected=0; // 실제 같은 버킷의 SID 체인 포함 여부와 순회 한도다.
            // 일반 해시는 여러 SID를 한 머리에 연결하므로 대상이 머리인지를 요구하지 않는다.
            for (Sid current{core.hash.Bucket(level,object.object.x,object.object.y)};
                 current.value && inspected++<core.pool.Capacity();current=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(current),4,2))}) {
                if (current==object.sid) { found=true;break; }
            }
            CHECK(found);
            if (raw[10]==kNormalFarm) { CHECK(Get(raw,26,edition==OriginalEdition::Patch1078 ? 4 : 2)==73);++farmCount; }
            else CHECK(raw[10]==kNormalIsle);
        }
        CHECK(farmCount>0 && core.bookkeeping.globalCounts[kNormalIsle]==static_cast<std::uint32_t>(total));
        CHECK(core.bookkeeping.depth==0 && scene.pipelineState.placement.forcePlacement==0);
        scene.world->FinishLoad();CHECK(scene.islands->Owner(0)==2 && scene.islands->Owner(1)==3);
    }
}

// 새 배치 분기는 실제121 생성/HP/게임 프레임 난수/현재 섬 owner를 같은 공간·장부에 쓴다.
TEST_CASE(NormalTerrainWorldBinding_ActualFarmCreateOwnerHpAndGameRandom) {
    // terrain-frame 교체와 구별한 synthetic random-frame 입력으로 양판본의 실제 생성 소비자를 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        NormalWorldScene scene(edition,true);const auto core=scene.world->Core();scene.world->BeginLoad();
        RawTerrainContext context(edition,1,0,{3,3,4,4},scene.globals,[](TerrainContextAssertion) {});
        context.Codes()[50*256+53]='x';context.Regions()[50*256+53]=0;
        TerrainStampHooks effects;effects.chunkRegion=[&scene](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(scene.chunks.At(x,y).island); };
        effects.assertion=[&scene](TerrainStampAssertion) { ++scene.diagnostics; };
        RawTerrainStamp stamp(core.pool,core.types,core.frames,context.Map(),scene.random,scene.crt,scene.stamp,
            MakeTerrainStampWorldHooks(*scene.world,scene.stamp,[&scene](SquidRegionAssert) { ++scene.diagnostics; },std::move(effects)));
        stamp.Stamp('x');CHECK(context.Map().lastSid.value!=0);
        RawFortRandomPlacement single(core.pool,core.hash,core.spots,core.types,core.frames,scene.random,scene.placement,
            MakeNormalTerrainFarmWorldHooks(*scene.world,scene.chunks,*scene.islands,*scene.pipeline,*scene.hp));
        GameRandom local(77),expected(scene.random.State());expected.Next(1);const auto previousCrt=scene.crt;
        const Sid created{static_cast<std::uint16_t>(single.Place(local,{kNormalFarm,50,53,0,0,0}))};CHECK(created.value!=0);
        const auto raw=core.pool.Slot(created);CHECK(scene.world->Find(50,53,kNormalFarm)==created);
        CHECK(scene.random.State()==expected.State() && local.State()==77 && scene.crt==previousCrt);
        CHECK(Get(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,edition==OriginalEdition::Patch1078 ? 4 : 1)==255);
        CHECK(raw[edition==OriginalEdition::Patch1078 ? 34 : 32]==2 && Get(raw,26,edition==OriginalEdition::Patch1078 ? 4 : 2)==73);
        CHECK(core.bookkeeping.globalCounts[kNormalFarm]==1 && core.bookkeeping.totalCost==0 && core.bookkeeping.depth==0);
        CHECK(scene.diagnostics==0 && &scene.pipeline->Pool()==&core.pool);scene.world->FinishLoad();
    }
}

// 필수 효과/다른 월드/미복원 분기를 생성 전 거부하며 원본을 기본 생성자/허용값으로 대신하지 않는다.
TEST_CASE(NormalTerrainWorldBinding_RejectsMissingEffectsAndUnrestoredBranches) {
    NormalWorldScene scene(OriginalEdition::Patch1078);const auto core=scene.world->Core();const auto before=scene.world->Report();
    const auto random=scene.random.State(),crt=scene.crt;
    CHECK(Throws([&] { scene.binding->Generate({1,{3,3,4,4},0,NormalTerrainBranch::Archipelago}); }));
    CHECK(Throws([&] { scene.binding->Generate({1,{3,3,4,4},0,NormalTerrainBranch::UnrestoredParent}); }));
    scene.farm.archipelago=true;CHECK(Throws([&] { scene.binding->PostprocessFarms(0); }));scene.farm.archipelago=false;
    scene.placement.edgeFarmType=122;CHECK(Throws([&] { scene.binding->PostprocessFarms(0); }));scene.placement.edgeFarmType=kNormalFarm;
    auto missing=scene.Diagnostics();missing.region={};
    CHECK(Throws([&] { RawNormalTerrainWorldBinding rejected(*scene.world,scene.chunks,*scene.islands,scene.random,scene.crt,scene.globals,
        scene.stamp,scene.placement,scene.farm,scene.puzzlePiece,*scene.pipeline,*scene.hp,*scene.carrier,missing); }));
    NormalWorldScene other(OriginalEdition::Patch1078);
    CHECK(Throws([&] { MakeNormalTerrainFarmWorldHooks(*scene.world,scene.chunks,*other.islands,*scene.pipeline,*scene.hp); }));
    CHECK(Throws([&] { MakeNormalTerrainFarmWorldHooks(*scene.world,scene.chunks,*scene.islands,*other.pipeline,*scene.hp); }));
    CHECK(scene.world->Report()==before && scene.random.State()==random && scene.crt==crt && !scene.stamp.variants);
    const std::array<std::uint8_t,6> record{3,1,0,7,0,0};CHECK(scene.binding->PatternCount(record)==3);
    auto rotated=record;rotated[0]|=0xc0;CHECK(scene.binding->PatternCount(rotated)==3);
    CHECK(&scene.binding->Pool()==&core.pool);
}
