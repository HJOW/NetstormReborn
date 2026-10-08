// 다리/섬 연결·연결 객체 생성·소유자 전파를 원본 기계어 기대값으로 검사하고 실제 raw 모듈에 이어 본다.
#include "RawSceneSupport.h"
#include "o/SquidFactory.h"
#include "o/SquidFrame.h"
#include "o/SquidOwner.h"
#include "o/SquidPop.h"
#include "o/SquidPostPop.h"
#include "o/SquidUnpop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 판본마다 기대하는 관찰 행 수다(Walk는 장면 232개 × 플래그 8개, Connect는 장면마다 등록/미리보기 두 번).
const std::map<std::string,std::size_t> kExpectedRows{{"Walk",1856},{"Connect",464},{"Link",392},{"Owner",330},{"FindAt",270},
    {"Color",42},{"IslandOwner",96},{"StalagOwner",96},{"IslandPost",54}};
// 저장된 독립 기계어 출력을 한 번만 읽는다.
const FixtureData& Fixture() {
    static const auto data=LoadFixture(NETSTORM_BRIDGECONNECT_FIXTURE);
    return data;
}
// 미리보기 목록을 실행기의 표기로 만든다.
std::string Listed(const std::vector<Sid>& preview,bool used) {
    if (!used) return "-";
    std::string text;
    // 목록 순서대로 쉼표로 잇는다.
    for (const auto sid:preview) { if (!text.empty()) text+=',';text+=std::to_string(sid.value); }
    return text.empty() ? "empty" : text;
}
// 한 판본의 모든 관찰 행을 재생한다. 이전 행의 효과를 다음 행에 섞지 않는다.
void Replay(const char* edition) {
    Scene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::map<std::string,std::size_t> counts;int failures=0;
    // 행 종류마다 같은 입력으로 C++을 실행해 원본 관찰과 비교한다.
    for (const auto& row:Fixture().rows) {
        if (row[1]!=edition) continue;
        const auto& kind=row[0];bool same=false;std::string actual;
        try {
            if (kind=="Color") {
                BridgeConnectState state;state.battle=row[2]=="1";
                const auto colors=Split(row[3],',');
                // 색 표의 아홉 칸을 넣는다. 열째 칸은 표 밖의 메모리다.
                for (std::size_t i=0;i<state.ownerColors.size();++i) state.ownerColors[i]=std::stoi(colors[i]);
                const int owner=std::stoi(row[4]);
                // 전투 중의 표 밖 번호는 원본이 표 뒤의 메모리를 읽는다. C++은 거부한다. 전투가 아니면 표를 읽지 않는다.
                if (state.battle && owner>=static_cast<int>(kOwnerColorCount)) same=Throws([&] { OwnerColorFrame(state,owner); });
                else { actual=std::to_string(OwnerColorFrame(state,owner));same=actual==row[5]; }
            } else {
                scene.Prepare(Fixture().scenes.at(row[2]));
                if (kind=="Walk") {
                    RawSquidNeighborWalk walk(*scene.neighbors,kSource,Number(row[3]));
                    // 0이 나올 때까지 이웃 번호를 모은다.
                    for (Sid current=walk.Current();current.value!=0;current=walk.Next()) { if (!actual.empty()) actual+=',';actual+=std::to_string(current.value); }
                    if (actual.empty()) actual="-";
                    same=actual==row[4];
                } else if (kind=="FindAt") {
                    RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,scene.Hooks());
                    actual=std::to_string(connect.FindTypeAt(std::bit_cast<float>(Number(row[3])),std::bit_cast<float>(Number(row[4])),Number(row[5])).value);
                    same=actual==row[6];
                } else if (kind=="IslandOwner" || kind=="StalagOwner") {
                    SetIslandOwner(scene.pool,scene.state,kSource,Number(row[3]),[&](Sid sid,std::uint32_t owner) {
                        scene.pool.AllocatedBytes(sid)[scene.OwnerOffset()]=static_cast<std::uint8_t>(owner);
                        scene.events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(owner));
                    });
                    actual=scene.Events()+'|'+std::to_string(Get(scene.pool.Slot(kSource),scene.FrameOffset(),scene.FrameWidth()));
                    same=actual==row[4]+'|'+row[5];
                } else {
                    RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,scene.Hooks());
                    std::vector<Sid> preview;std::string expected;
                    if (kind=="Connect") {
                        const bool listed=row[3]=="1";
                        connect.Connect(kSource,listed ? &preview : nullptr);
                        actual=scene.Events()+'|'+scene.Born()+'|'+std::to_string(scene.PoolAdler())+'|'+Listed(preview,listed);
                        expected=row[4]+'|'+row[5]+'|'+row[6]+'|'+row[7];
                    } else if (kind=="Link") {
                        const bool listed=row[5]=="1";
                        connect.Link(Sid{static_cast<std::uint16_t>(Number(row[3]))},Sid{static_cast<std::uint16_t>(Number(row[4]))},Number(row[6]),
                            listed ? &preview : nullptr);
                        actual=scene.Events()+'|'+scene.Born()+'|'+std::to_string(scene.PoolAdler())+'|'+Listed(preview,listed);
                        expected=row[7]+'|'+row[8]+'|'+row[9]+'|'+row[10];
                    } else if (kind=="Owner") {
                        connect.PropagateOwner(Sid{static_cast<std::uint16_t>(Number(row[3]))},Sid{static_cast<std::uint16_t>(Number(row[4]))});
                        actual=scene.Events()+'|'+std::to_string(scene.PoolAdler());
                        expected=row[5]+'|'+row[6];
                    } else if (kind=="IslandPost") {
                        connect.IslandPostPop(kSource,Number(row[3]),[&](Sid sid,std::uint32_t flags) {
                            scene.events.push_back("T:"+std::to_string(sid.value)+':'+std::to_string(flags));
                        });
                        actual=scene.Events()+'|'+scene.Born()+'|'+std::to_string(scene.PoolAdler());
                        expected=row[4]+'|'+row[5]+'|'+row[6];
                    } else {
                        throw std::runtime_error("알 수 없는 행 종류");
                    }
                    same=actual==expected;
                }
            }
        } catch (const std::exception& error) { actual=std::string("예외: ")+error.what(); }
        CHECK(same);
        if (!same) {
            std::printf("  %s %s 행 %zu(장면 %s): 실제 %.400s\n",edition,kind.c_str(),counts[kind],kind=="Color" ? "-" : row[2].c_str(),actual.c_str());
            if (++failures>=6) break;
        }
        ++counts[kind];
    }
    CHECK(failures==0 && counts==kExpectedRows);
}
// 실제 bridge.type의 프레임 코드 60개. 다리 붕괴 기대값 파일의 "Frames 0" 행에 원본 자산에서 읽은 값이 있다.
RiftTypeFrames RealBridgeFrames() {
    std::ifstream input(NETSTORM_BRIDGEDECAY_FIXTURE);
    if (!input) throw std::runtime_error("다리 붕괴 fixture 없음");
    std::string line;
    // 첫 "Frames 0" 행만 쓴다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        const auto row=Split(line,'\t');
        if (row.size()!=3 || row[0]!="Frames" || row[1]!="0") continue;
        std::vector<FrameCode> codes;
        // 16진수 여덟 글자가 프레임 코드 한 칸(방향 글자, 변형 글자, 번호, 플래그)이다.
        for (std::size_t i=0;i+8<=row[2].size();i+=8) {
            const auto byte=[&](std::size_t at) { return static_cast<std::uint8_t>(std::stoul(row[2].substr(i+at*2,2),nullptr,16)); };
            codes.push_back({byte(0),byte(1),byte(2),byte(3)});
        }
        return RiftTypeFrames(std::move(codes));
    }
    throw std::runtime_error("다리 프레임 행 없음");
}
}
// 세 PE는 같은 장면이라도 각 실행 파일에서 직접 얻은 관찰을 사용한다.
TEST_CASE(bridge_connect_patch_x86_walk_link_owner) { Replay("originals"); }
// CD 판본의 바이트 프레임·인라인된 사각형 탐색 경로도 검사한다.
TEST_CASE(bridge_connect_cd_x86_walk_link_owner) { Replay("originalCD"); }
// 추가 10.37은 별도 PE 입력 SHA를 가진 관찰이다.
TEST_CASE(bridge_connect_1037_x86_walk_link_owner) { Replay("original1037"); }

// 훅 누락·다른 풀·표면 지도 순회 flag·색 표 밖 번호·소유자 없는 다리의 전파는 상태를 바꾸기 전에 거부한다.
TEST_CASE(bridge_connect_rejects_missing_hooks_and_invalid_inputs) {
    Scene scene(OriginalEdition::Patch1078);
    // 다리(50, 소유자 0) 서쪽에 표면 칸(70), 그 칸을 덮는 주인 없는 섬 받침(71)을 둔다.
    scene.Prepare({"50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;"
        "71,91,0,0,1100480512,1101529088,3,3,671088643,16777216,8,3","-","0","50,0;71,0","82,1,0,0,1,2,3,4,5,6,7,8,1"});
    auto hooks=scene.Hooks();
    CHECK(Throws([&] { auto broken=hooks;broken.setFrame={};RawBridgeConnect invalid(scene.pool,*scene.neighbors,scene.state,broken); }));
    CHECK(Throws([&] { auto broken=hooks;broken.create={};RawBridgeConnect invalid(scene.pool,*scene.neighbors,scene.state,broken); }));
    Scene other(OriginalEdition::Patch1078);
    CHECK(Throws([&] { RawBridgeConnect invalid(other.pool,*scene.neighbors,scene.state,hooks); }));
    CHECK(Throws([&] { RawSquidNeighborWalk invalid(*scene.neighbors,kSource,8); }));
    CHECK(Throws([&] { OwnerColorFrame(scene.state,9); }));
    CHECK(Throws([&] { SetIslandOwner(scene.pool,scene.state,kSource,1,{}); }));
    CHECK(Throws([&] { MakeOwnerDispatch(scene.pool,scene.state,{}); }));
    RawBridgeConnect connect(scene.pool,*scene.neighbors,scene.state,hooks);
    const std::vector<std::uint8_t> before(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    // 원본 assert("newPlayerId != INVALID_PLAYER_ID") 경로: 훅을 부르거나 풀을 바꾸기 전에 예외다.
    CHECK(Throws([&] { connect.PropagateOwner(kSource,Sid{70}); }));
    CHECK(scene.events.empty() && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    CHECK(Throws([&] { connect.IslandPostPop(kSource,1,{}); }));
    CHECK(Throws([&] { connect.FindTypeAt(20.0f,21.0f,5000); }));
    // 소유자가 있으면 같은 장면에서 받침의 소유자와 색 프레임(전투 모드, 소유자 4 → 색 표 4 - 1)이 바뀐다.
    scene.pool.AllocatedBytes(kSource)[scene.OwnerOffset()]=4;
    connect.PropagateOwner(kSource,Sid{70});
    CHECK(scene.Events()=="O:71:4;S:71:3:0");
}

// 순회 도중 호출자가 뒤쪽 버킷에 새 객체를 등록하면 원본처럼 그 뒤의 Next가 그 객체를 읽는다(여기서는 표면 타입이라 이웃이 된다).
// 이미 지나간 버킷에 넣은 객체는 보이지 않는다.
TEST_CASE(bridge_connect_walk_reads_live_chain_between_next_calls) {
    Scene scene(OriginalEdition::Patch1078);
    // 다리(50) 서쪽 (19,21)과 동쪽 (21,21)에 표면 칸이 있다. 0단계에서 y, x 순서라 서쪽이 먼저다.
    scene.Prepare({"50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;"
        "71,83,0,0,1101529088,1101529088,1,1,671090691,2,2,0","-","0","-","82,1,0,0,1,2,3,4,5,6,7,8,1"});
    RawSquidNeighborWalk walk(*scene.neighbors,kSource,NeighborFlag::kSkipAbstract);
    CHECK(walk.Current()==Sid{70});
    // 첫 이웃을 받은 뒤 남쪽 (20,22)과 이미 지나간 북쪽 (20,20)에 표면 칸을 직접 등록한다.
    const auto add=[&](std::uint16_t id,float x,float y) {
        auto raw=scene.pool.AllocatedBytes(Sid{id});
        Put(raw,0,kVtable);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));Put(raw,36,2);
        auto& head=scene.hash.Bucket(0,x,y);Put(raw,4,head,2);head=id;
    };
    add(72,20.0f,22.0f);add(73,20.0f,20.0f);
    CHECK(walk.Next()==Sid{71});
    CHECK(walk.Next()==Sid{72});
    CHECK(walk.Next()==Sid{} && walk.Next()==Sid{});
}

// 기록 훅으로 남았던 생성·소유자 지정·Pop을 실제 raw 모듈에 연결한다: 중립 섬 받침 옆에 다리를 놓으면
// 다리 postPop 접두 → 연결 순회 → 연결 객체 생성/등록 → 섬 받침·종유석·표면 칸으로 소유자와 색이 전파된다.
// 프레임 지정도 실제 복원 함수(SquidFrame)를 거친다. 표시 갱신과 noIsland의 파생 postPop은 여전히 외부 경계이며
// 비전투·Graph 비활성 공간에서의 통합 검사다.
TEST_CASE(bridge_connect_real_raw_modules_capture_neutral_island) {
    // 같은 타입 번호와 생성자 배치를 쓰는 두 판본에서 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 실제 raw 생성자가 요구하는 원본 전체 슬롯 수다.
        constexpr std::uint32_t capacity=32768;
        // 실제 타입 번호: bridge 82, island 94, islandStalag 95, bridgeConnector 155, noIsland 157.
        constexpr std::uint32_t bridgeType=82,islandType=94,stalagType=95,connectorType=155,noIslandType=157;
        const bool patch=edition==OriginalEdition::Patch1078;
        const std::size_t frameOffset=patch ? 36 : 34,frameWidth=patch ? 4 : 1,extraOffset=patch ? 40 : 35,ownerOffset=patch ? 34 : 32;
        SidPool pool(edition,capacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        // 10.78 타입 표의 플래그·발자국과 판본별 실제 생성자 주소를 넣는다.
        const auto define=[&](std::uint32_t number,std::uint32_t flags1,std::uint32_t flags2,int foot) {
            auto& type=types[number];type.constructorAddress=TypeConstructorAddress(edition,number);
            type.flags1=flags1;type.flags2=flags2;type.footX=type.footY=foot;
        };
        define(bridgeType,0x00000802,0x00000004,1);define(islandType,0x28000003,0x01000000,3);define(stalagType,0x28000002,0,3);
        define(connectorType,0x28000002,0x02000000,1);define(noIslandType,0x08000803,0x01000002,1);
        // 다리는 실제 프레임 표, 나머지는 색 프레임 9개(방향 글자 A)의 합성 표다.
        const auto bridgeFrames=RealBridgeFrames();
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));
        frames[bridgeType]=bridgeFrames;
        for (const auto number:{islandType,stalagType,connectorType,noIslandType})
            frames[number]=RiftTypeFrames(std::vector<FrameCode>(9,FrameCode{'A','P',1,0}));
        SquidPostPopState bookkeeping;
        RawBridgeConnect* linker=nullptr;
        SquidPostPop postPop(pool,types,bookkeeping,nullptr,[&](Sid sid) { linker->Connect(sid); });
        postPop.SetIslandPrefix([&](Sid sid,std::uint32_t flags) { linker->IslandPostPopPrefix(sid,flags); });
        SquidUnpop unpop(pool,hash,spots);SquidPop pop(pool,hash,spots,nullptr,&postPop);
        SquidFactory factory(pool,types,false,&unpop);
        SquidOwnerMode ownerMode;ownerMode.battle=true;SquidOwner owner(pool,types,bookkeeping,ownerMode);
        RawSquidNeighbors neighbors(pool,hash,spots,types,frames);
        BridgeConnectState state;
        state.connectorType=connectorType;state.islandType=islandType;state.stalagType=stalagType;state.noIslandType=noIslandType;
        state.bridgeType=bridgeType;state.battle=true;
        std::vector<Sid> created;
        BridgeConnectHooks hooks;
        hooks.create=[&](std::uint32_t type,std::uint32_t flags) { created.push_back(factory.Create(type,flags));return created.back(); };
        hooks.setOwner=MakeOwnerDispatch(pool,state,[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); });
        // 해시 단계는 타입의 발자국 크기를 프레임 크기로 넣어 정한다(섬 받침·종유석 2단계, 연결 객체 1단계).
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            const auto& type=types[pool.Slot(sid)[10]];
            CHECK(pop.Pop(sid,type,static_cast<float>(type.footX),static_cast<float>(type.footY),x,y,flags)==RawPopResult::Registered);
        };
        // 프레임 지정(004acee0)도 실제 복원 함수에 잇는다. 프레임 크기는 타입의 발자국 크기로 주고 표시 갱신은 연결하지 않는다.
        int frameUpdates=0;
        SquidFrame frameSetter(pool,types,SquidFrameHooks{
            [&](Sid sid,std::int32_t) { const auto& type=types[pool.Slot(sid)[10]];return std::array<float,2>{static_cast<float>(type.footX),static_cast<float>(type.footY)}; },
            [&](Sid,std::uint32_t) { ++frameUpdates; },
            [&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); },
            [&](Sid sid,float x,float y,std::uint32_t flags) {
                const auto& type=types[pool.Slot(sid)[10]];
                CHECK(pop.Pop(sid,type,static_cast<float>(type.footX),static_cast<float>(type.footY),x,y,flags)==RawPopResult::Registered);
            }});
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { frameSetter.Set(sid,frame,flags); };
        // 연결 객체의 타입은 다리 타입 전역과 다르므로 표면 알림은 불리지 않아야 한다.
        int notified=0;
        hooks.notifySurface=[&](Sid) { ++notified; };
        RawBridgeConnect connect(pool,neighbors,state,std::move(hooks));linker=&connect;
        // 1. 중립 섬 받침(소유자 0, 프레임 8)을 (30,30)에 등록한다. 파생 postPop이 같은 자리에 종유석을 만든다.
        const Sid island=factory.Create(islandType);
        Put(pool.AllocatedBytes(island),frameOffset,8,frameWidth);
        CHECK(pop.Pop(island,types[islandType],3,3,30.0f,30.0f)==RawPopResult::Registered);
        CHECK(created.size()==1);
        const Sid stalag=connect.FindTypeAt(30.0f,30.0f,stalagType);
        CHECK(stalag.value!=0 && stalag==created[0]);
        CHECK(pool.Slot(stalag)[ownerOffset]==0 && Get(pool.Slot(stalag),frameOffset,frameWidth)==8 && (pool.Slot(stalag)[11]&4)==0);
        CHECK(connect.FindTypeAt(29.0f,28.0f,islandType)==island);
        // 2. 받침 위의 표면 칸(noIsland) 둘은 파생 postPop이 미복원이라 기존 배치 상태를 직접 입력한다.
        const auto surface=[&](float x,float y) {
            const Sid sid=factory.Create(noIslandType);auto raw=pool.AllocatedBytes(sid);
            raw[11]=static_cast<std::uint8_t>(raw[11]&~4);
            Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
            auto& head=hash.Bucket(0,x,y);Put(raw,4,head,2);head=sid.value;
            spots[static_cast<std::size_t>(static_cast<int>(y)*256+static_cast<int>(x))]|=2;
            return sid;
        };
        const Sid east=surface(30.0f,29.0f),inner=surface(29.0f,30.0f);
        // 3. 소유자 3의 다리(동서 방향 K 프레임)를 받침 동쪽 바깥 (31,29)에 등록한다.
        const Sid bridge=factory.Create(bridgeType);owner.Set(bridge,3);
        Put(pool.AllocatedBytes(bridge),frameOffset,static_cast<std::uint32_t>(bridgeFrames.Run('K').first),frameWidth);
        CHECK(pop.Pop(bridge,types[bridgeType],1,1,31.0f,29.0f)==RawPopResult::Registered);
        // 다리 postPop 접두가 extra 비트 2를 켜고 연결 순회를 불렀다. 연결 객체가 하나 생겼다.
        CHECK((pool.Slot(bridge)[extraOffset]&2)!=0 && created.size()==2);
        if (created.size()!=2) continue;
        const Sid link=created[1];const auto raw=pool.Slot(link);
        // 연결 객체: 다리에서 서쪽으로 한 칸((30,29)), 프레임 3(서쪽 6 / 2), 소유자 3, 첫 참조 다리·둘째 참조 표면 칸.
        CHECK(raw[10]==connectorType && raw[ownerOffset]==3 && Get(raw,frameOffset,frameWidth)==3);
        CHECK(std::bit_cast<float>(Get(raw,14))==30.0f && std::bit_cast<float>(Get(raw,18))==29.0f && (raw[11]&4)==0);
        CHECK(Get(raw,12,2)==bridge.value && Get(raw,8,2)==east.value);
        CHECK(hash.Bucket(1,30.0f,29.0f)==link.value);
        // 소유자 전파: 섬 받침의 소유자와 색 프레임(소유자 3 → 2), 종유석 프레임, 발자국 안의 두 표면 칸의 소유자.
        CHECK(pool.Slot(island)[ownerOffset]==3 && Get(pool.Slot(island),frameOffset,frameWidth)==2);
        CHECK(Get(pool.Slot(stalag),frameOffset,frameWidth)==2 && pool.Slot(stalag)[ownerOffset]==0);
        CHECK(pool.Slot(east)[ownerOffset]==3 && pool.Slot(inner)[ownerOffset]==3);
        // 받침과 종유석의 프레임 지정은 해시 단계가 그대로라 제자리 경로(표시 갱신 두 번씩)이고 등록 위치는 바뀌지 않는다.
        CHECK(frameUpdates==4 && connect.FindTypeAt(30.0f,30.0f,islandType)==island && connect.FindTypeAt(30.0f,30.0f,stalagType)==stalag);
        // 4. 이미 주인이 있는 받침은 다른 소유자의 다리가 닿아도 바뀌지 않는다. 연결 객체만 더 생긴다.
        const Sid second=factory.Create(bridgeType);owner.Set(second,5);
        Put(pool.AllocatedBytes(second),frameOffset,static_cast<std::uint32_t>(bridgeFrames.Run('J').first),frameWidth);
        CHECK(pop.Pop(second,types[bridgeType],1,1,29.0f,31.0f)==RawPopResult::Registered);
        CHECK(created.size()==3 && pool.Slot(island)[ownerOffset]==3 && pool.Slot(inner)[ownerOffset]==3);
        if (created.size()==3) {
            const auto other=pool.Slot(created[2]);
            // 남쪽 다리에서 북쪽(0)으로 한 칸: (29,30), 프레임 0, 소유자 5.
            CHECK(other[ownerOffset]==5 && Get(other,frameOffset,frameWidth)==0 && Get(other,12,2)==second.value && Get(other,8,2)==inner.value);
            CHECK(std::bit_cast<float>(Get(other,14))==29.0f && std::bit_cast<float>(Get(other,18))==30.0f);
        }
        CHECK(bookkeeping.depth==0 && notified==0);
    }
}
