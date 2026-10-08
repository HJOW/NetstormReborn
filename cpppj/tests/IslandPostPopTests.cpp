// noIsland 최초 등록/받침 소유자 전파를 독립 기계어 기대값과 대조하고 실제 생성·Pop·소유자 모듈에 연결한다.
#include "RawSceneSupport.h"
#include "o/RawIslandPostPop.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidPop.h"
#include "o/SquidPostPop.h"
#include "o/SquidUnpop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// noIsland의 실제 3×3 글자 순서다. 각 방향의 첫 프레임만 합성 입력으로 사용한다.
constexpr std::string_view kLetters="FCGBADIEH";
// 같은 순서의 P1 코드 표를 만든다. 기대 프레임은 원본 fixture에서 읽는다.
RiftTypeFrames SurfaceFrames() {
    std::vector<FrameCode> codes;
    // 방향마다 P 변형의 첫 프레임이다.
    for (const char letter:kLetters) codes.push_back({static_cast<std::uint8_t>(letter),'P',1,0});
    return RiftTypeFrames(std::move(codes));
}
// 저장된 입력/관찰은 한 번만 읽는다.
const FixtureData& Fixture() { static const auto data=LoadFixture(NETSTORM_ISLANDPOSTPOP_FIXTURE);return data; }
// 한 판본을 원본과 같은 초기 슬롯·지도·효과 경계로 재생한다.
void Replay(const char* edition) {
    Scene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::map<std::string,std::size_t> counts;int failures=0;
    // 각 행은 앞 행의 변경을 이어받지 않고 입력부터 다시 준비한다.
    for (const auto& row:Fixture().rows) {
        if (row[1]!=edition) continue;
        scene.Prepare(Fixture().scenes.at(row[2]));scene.frames[kNoIslandType]=SurfaceFrames();
        auto objects=scene.Hooks();RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,objects);
        IslandPostPopState state{Number(row[4]),row[5]=="1"};
        IslandPostPopHooks hooks{objects,
            [&](Sid sid) { connect.Connect(sid); },
            [&](float x,float y,std::uint32_t type) { return connect.FindTypeAt(x,y,type); },
            [&](Sid sid) { scene.events.push_back("U:"+std::to_string(sid.value)); },
            [&](bool,bool,bool) { scene.events.push_back("D"); }};
        RawIslandPostPop surface(scene.pool,scene.hash.Entries(0),scene.types,scene.frames,scene.state,state,std::move(hooks));
        if (row[0]=="SurfacePostPop") {
            surface.Prefix(kSource,Number(row[3]));
            scene.events.push_back("T:50:"+row[3]);
        } else {
            CHECK(row[0]=="SupportOwner");const auto raw=scene.pool.Slot(kSource);
            surface.SetSupportOwner(std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),Number(row[3]));
        }
        const bool same=scene.Events()==row[6] && scene.Born()==row[7] && scene.PoolAdler()==Number(row[8]);
        if (!same && failures++<8) std::fprintf(stderr,"islandpostpop %s %s scene %s: %s / %s, Adler %u / %s\n",
            edition,row[0].c_str(),row[2].c_str(),scene.Events().c_str(),row[6].c_str(),scene.PoolAdler(),row[8].c_str());
        CHECK(same);++counts[row[0]];
    }
    CHECK(counts["SurfacePostPop"]==402);CHECK(counts["SupportOwner"]==150);
}
}
// 서로 다른 실제 PE에서 저장한 기대값을 각각 대조한다.
TEST_CASE(island_postpop_matches_originals_x86) { Replay("originals"); }
TEST_CASE(island_postpop_matches_original_cd_x86) { Replay("originalCD"); }
TEST_CASE(island_postpop_matches_original_1037_x86) { Replay("original1037"); }

// 원본 지도 조회는 범위 밖/다른 타입을 0으로 돌려준다. 훅/지도 오류와 H의 패치 assert 경계도 확인한다.
TEST_CASE(island_postpop_validates_hooks_maps_and_missing_upper_left) {
    Scene scene(OriginalEdition::Patch1078);
    scene.Prepare({"50,93,0,0,1106247680,1106247680,1,1,134219779,16777218,8,0","-","0","-","82,1,0,0,1,2,3,4,5,6,7,8,1"});
    scene.frames[kNoIslandType]=SurfaceFrames();
    auto objects=scene.Hooks();RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,objects);
    IslandPostPopState state;
    IslandPostPopHooks hooks{objects,[&](Sid sid) { connect.Connect(sid); },
        [&](float x,float y,std::uint32_t type) { return connect.FindTypeAt(x,y,type); },
        [](Sid) {},[](bool,bool,bool) {}};
    CHECK(Throws([&] { auto broken=hooks;broken.updateDisplay={};RawIslandPostPop invalid(scene.pool,scene.hash.Entries(0),scene.types,scene.frames,scene.state,state,broken); }));
    CHECK(Throws([&] { RawIslandPostPop invalid(scene.pool,{},scene.types,scene.frames,scene.state,state,hooks); }));
    RawIslandPostPop surface(scene.pool,scene.hash.Entries(0),scene.types,scene.frames,scene.state,state,hooks);
    CHECK(surface.SurfaceLetter(-1,30)==0 && surface.SurfaceLetter(30,256)==0 && surface.SurfaceLetter(30,30)=='H');
    CHECK(Throws([&] { surface.Prefix(kSource,1); }));
    CHECK(scene.events.empty());
    // flag 비트가 없으면 현재 H여도 아무 파생 효과를 수행하지 않는다.
    surface.Prefix(kSource,4);CHECK(scene.events.empty());
    state.ownerPropagationSuppressed=true;surface.Prefix(kSource,1);CHECK(scene.events.empty());
}

// CD는 서/북·3×3 순회를 고정 지도에서, H의 왼쪽 위·대상은 현재 지도에서 읽는다.
TEST_CASE(island_postpop_cd_uses_separate_owner_surface_map) {
    Scene scene(OriginalEdition::Cd1072);
    scene.Prepare({"50,93,0,0,1106247680,1106247680,1,1,134219779,16777218,4,0;60,93,0,0,1105723392,1106247680,1,1,134219779,16777218,0,0",
        "-","0","-","82,1,0,0,1,2,3,4,5,6,7,8,0"});
    scene.frames[kNoIslandType]=SurfaceFrames();
    auto objects=scene.Hooks();RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,objects);
    IslandPostPopState state{1,true};
    auto ownerMap=std::vector<std::uint16_t>(scene.hash.Entries(0).begin(),scene.hash.Entries(0).end());
    ownerMap[30*256+29]=0;
    IslandPostPopHooks hooks{objects,[&](Sid sid) { connect.Connect(sid); },
        [&](float x,float y,std::uint32_t type) { return connect.FindTypeAt(x,y,type); },
        [](Sid) {},[](bool,bool,bool) {}};
    RawIslandPostPop surface(scene.pool,scene.hash.Entries(0),scene.types,scene.frames,scene.state,state,hooks,ownerMap);
    CHECK(surface.SurfaceLetter(29,30)==0);
    surface.Prefix(kSource,1);
    CHECK(Get(scene.pool.Slot(kSource),34,1)==0 && scene.born==1);
}

// 실제 raw 생성/Pop/소유자 모듈로 3×3 noIsland를 놓는다. 첫 F가 받침·종유석을 만들고 마지막 H가 전체 소유자를 맞춘다.
TEST_CASE(island_postpop_real_raw_modules_build_support_and_propagate_owner) {
    // 10.78과 CD의 프레임 폭/가상 표/생성자 차이를 모두 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 실제 원본 풀과 타입 번호다.
        constexpr std::uint32_t capacity=32768,bridgeType=82,islandType=94,stalagType=95,connectorType=155,noIslandType=157;
        const bool patch=edition==OriginalEdition::Patch1078;
        const std::size_t ownerOffset=patch ? 34 : 32,frameOffset=patch ? 36 : 34,frameWidth=patch ? 4 : 1;
        SidPool pool(edition,capacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        // 실제 타입의 생성자·발자국·플래그를 입력한다.
        const auto define=[&](std::uint32_t number,std::uint32_t flags1,std::uint32_t flags2,int foot) {
            auto& type=types[number];type.constructorAddress=TypeConstructorAddress(edition,number);
            type.flags1=flags1;type.flags2=flags2;type.footX=type.footY=foot;
        };
        define(bridgeType,0x802,4,1);define(islandType,0x28000003,0x01000000,3);define(stalagType,0x28000002,0,3);
        define(connectorType,0x28000002,0x02000000,1);define(noIslandType,0x08000803,0x01000002,1);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[noIslandType]=SurfaceFrames();
        // 받침·종유석의 색 프레임은 방향 A의 합성 표다.
        for (const auto number:{islandType,stalagType}) frames[number]=RiftTypeFrames(std::vector<FrameCode>(9,FrameCode{'A','P',1,0}));
        SquidPostPopState bookkeeping;RawBridgeConnect* linker=nullptr;RawIslandPostPop* surfaceLinker=nullptr;
        SquidPostPop postPop(pool,types,bookkeeping,nullptr,[&](Sid sid) { linker->Connect(sid); });
        postPop.SetIslandPrefix([&](Sid sid,std::uint32_t flags) { linker->IslandPostPopPrefix(sid,flags); });
        postPop.SetSurfacePrefix([&](Sid sid,std::uint32_t flags) { surfaceLinker->Prefix(sid,flags); });
        SquidUnpop unpop(pool,hash,spots);SquidPop pop(pool,hash,spots,nullptr,&postPop);SquidFactory factory(pool,types,false,&unpop);
        SquidOwnerMode mode;mode.battle=true;SquidOwner owner(pool,types,bookkeeping,mode);
        RawSquidNeighbors neighbors(pool,hash,spots,types,frames);
        BridgeConnectState typeState;typeState.bridgeType=bridgeType;typeState.islandType=islandType;typeState.stalagType=stalagType;
        typeState.connectorType=connectorType;typeState.noIslandType=noIslandType;typeState.battle=true;
        std::vector<Sid> created;int updates=0,diagnostics=0;
        BridgeConnectHooks objects;
        objects.create=[&](std::uint32_t type,std::uint32_t flags) { created.push_back(factory.Create(type,flags));return created.back(); };
        objects.setOwner=MakeOwnerDispatch(pool,typeState,[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); });
        objects.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            const auto& type=types[pool.Slot(sid)[10]];
            CHECK(pop.Pop(sid,type,static_cast<float>(type.footX),static_cast<float>(type.footY),x,y,flags)==RawPopResult::Registered);
        };
        objects.setFrame=[](Sid,std::int32_t,std::uint32_t) { throw std::logic_error("다리 없는 입력에서 프레임 지정이 불렸다"); };
        objects.notifySurface=[](Sid) { throw std::logic_error("다리 없는 입력에서 표면 알림이 불렸다"); };
        RawBridgeConnect connect(pool,neighbors,typeState,objects);linker=&connect;
        IslandPostPopState state{1,false};
        IslandPostPopHooks hooks{objects,[&](Sid sid) { connect.Connect(sid); },
            [&](float x,float y,std::uint32_t type) { return connect.FindTypeAt(x,y,type); },
            [&](Sid) { ++updates; },[&](bool,bool,bool) { ++diagnostics; }};
        RawIslandPostPop surface(pool,hash.Entries(0),types,frames,typeState,state,std::move(hooks));surfaceLinker=&surface;
        std::vector<Sid> cells;
        // 요새 로드와 같은 행 우선 순서로 9칸을 실제 Pop한다.
        for (int y=28;y<=30;++y) {
            // 처음 F의 소유자만 3, 나머지는 0으로 시작해 H의 전파를 확인한다.
            for (int x=28;x<=30;++x) {
                const Sid sid=factory.Create(noIslandType);cells.push_back(sid);
                owner.Set(sid,cells.size()==1 ? 3U : 0U);
                CHECK(pop.Pop(sid,types[noIslandType],1,1,static_cast<float>(x),static_cast<float>(y))==RawPopResult::Registered);
                CHECK(Get(pool.Slot(sid),frameOffset,frameWidth)==cells.size()-1);
            }
        }
        CHECK(created.size()==2 && updates==11 && diagnostics==0 && bookkeeping.depth==0);
        // 3×3 칸과 받침·종유석이 같은 주인/색을 얻었으며 공통 postPop 통계도 중복 없이 기록됐다.
        for (const Sid sid:cells) CHECK(pool.Slot(sid)[ownerOffset]==3);
        const Sid island=connect.FindTypeAt(30,30,islandType),stalag=connect.FindTypeAt(30,30,stalagType);
        CHECK(island.value!=0 && stalag.value!=0);
        if (island.value && stalag.value) {
            CHECK(pool.Slot(island)[ownerOffset]==3 && pool.Slot(stalag)[ownerOffset]==3);
            CHECK(Get(pool.Slot(island),frameOffset,frameWidth)==2 && Get(pool.Slot(stalag),frameOffset,frameWidth)==2);
        }
        CHECK(bookkeeping.globalCounts[noIslandType]==9 && bookkeeping.globalCounts[islandType]==1 && bookkeeping.globalCounts[stalagType]==1);
        // 접두 연결을 제거하면 다음 Pop은 공간 변경 전에 거부한다.
        postPop.SetSurfacePrefix({});const Sid rejected=factory.Create(noIslandType);
        const auto before=hash.Bucket(0,40,40);
        CHECK(Throws([&] { pop.Pop(rejected,types[noIslandType],1,1,40,40); }));
        CHECK(hash.Bucket(0,40,40)==before && (pool.Slot(rejected)[11]&4)!=0);
    }
}
