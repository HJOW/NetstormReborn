// 밀어내기/섬 정리의 독립 원본 관찰과 서버 요청·실제 SID 수명 연결을 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionEffects.h"
#include "o/RawSquidDestroy.h"
#include "o/SquidFactory.h"
#include <cmath>

namespace {
using namespace netstorm::test::construction;
// 경로 타입 경계의 토큰과 원본 입력에 쓰는 다른 타입 번호다.
constexpr std::uint32_t kPathType=0x12345678,kOtherType=102;
// 판본별 저장 입력 개수다. 빠뜨린 행이 있으면 재생 검사를 실패시킨다.
constexpr std::size_t kRows=1178;
// 저장된 관찰만 읽는다. 기본 CTest에서 게임/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONEFFECTS_FIXTURE).rows;return rows; }
// fixture 행마다 공유 타입/지도/풀과 효과 기록을 재설정한다.
struct EffectsScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidHash hash;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> spots;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    ConstructionEffectsState state;
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch;
    // input: 처리할 입력 또는 입력 분배기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint32_t> input;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string events;
    // 원본 서버 SID까지 포함하는 기존 검사 풀을 쓴다.
    explicit EffectsScene(OriginalEdition edition):pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        spots(65536),patch(edition==OriginalEdition::Patch1078) {}
    // 실행기의 합성 입력을 공급한다. 기대 효과를 계산하지 않는다.
    void Prepare(const std::vector<std::uint32_t>& v) {
        input=v;types.assign(types.size(),RiftTypeRecord{});hash.Reset();events.clear();state=ConstructionEffectsState{};
        types[v[0]].footX=static_cast<int>(v[3]);types[v[0]].footY=static_cast<int>(v[4]);
        types[kTypeIsland].footX=static_cast<int>(v[3]);types[kTypeIsland].footY=static_cast<int>(v[4]);
        state.blocked=v[7]!=0;state.server=v[8]!=0;state.alliances.fill(v[12]);
        const auto profile=v[9];std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(profile>=7 && profile<=13 ? 0 : profile));
        // 한 칸 spot 입력으로 검색 순서와 두 판본의 탐색 거리를 구분한다.
        if (profile>=7 && profile<=13) {
            const std::array<std::array<int,2>,7> positions{{{20,22},{19,22},{20,24},{21,21},{23,21},{19,23},{1,2}}};
            const auto& p=positions[profile-7];spots[static_cast<std::size_t>(p[1])*256+static_cast<std::size_t>(p[0])]=2;
        }
    }
    // 후보를 fixture에 지정된 해시 단계의 머리에 등록한다.
    void Node(const std::vector<std::uint32_t>& n) {
        auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(n[0])});std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(n[1]);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(n[2]);
        Put(raw,14,n[3]);Put(raw,18,n[4]);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(n[5]);
        Put(raw,patch ? 36 : 34,n[6],patch ? 4 : 1);types[n[1]].flags2=n[8];
        if (n[1]!=kTypeIsland) types[n[1]].footX=types[n[1]].footY=1;
        auto& head=hash.Bucket(static_cast<int>(n[7]),Float(n[3]),Float(n[4]));Put(raw,4,head,2);head=static_cast<std::uint16_t>(n[0]);
    }
    // 경계 인자의 DWORD 비트를 순서대로 기록한다.
    void Append(char letter,std::initializer_list<std::uint32_t> values={}) {
        if (!events.empty()) events+=';';events+=letter;
        // 입력 순서대로 각 인자를 붙인다.
        for (const auto value:values) events+=':'+std::to_string(value);
    }
    // 원본 실행기에서 대체한 효과만 재생한다. finder Begin은 인자만 관찰한다.
    ConstructionEffectsHooks Hooks() {
        ConstructionEffectsHooks hooks;
        hooks.pathType=[this](std::uint32_t type) { Append('R',{type});return kPathType; };
        hooks.clearPath=[this](Sid sid,std::uint32_t type) { Append('C',{sid.value,type,0,1}); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Append('U',{sid.value,flags}); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Append('P',{sid.value,Bits(x),Bits(y),flags}); };
        hooks.fall=[this](Sid sid) { Append('F',{sid.value});if (input[14]==1) { auto raw=InputRaw(pool,sid);Put(raw,14,Bits(1.25F));Put(raw,18,Bits(2.5F)); } };
        hooks.sound=[this](float x,float y) { Append('S',{Bits(x),Bits(y),0}); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Append('K',{sid.value,flags});if (input[14]==2) types[pool.Slot(sid)[10]].flags2=0x10000; };
        hooks.notify=[this](Sid sid) { Append('N',{sid.value}); };
        hooks.broadcast=[this](std::uint32_t x,std::uint32_t y,std::uint32_t flags,std::uint32_t player) { Append('I');Append('B',{x,y,flags,player}); };
        hooks.assertion=[this](std::string_view expression,int line) {
            CHECK(expression==(line==0x7b ? "noIslandTypeCount == 9" : line==0x7c ? "islandTypeCount == 1" : "islandStalagTypeCount == 1"));Append('E',{static_cast<std::uint32_t>(line)});
        };
        hooks.scan=[this](const SquidSearchArea& a) { Append('A',{std::bit_cast<std::uint32_t>(a.left),std::bit_cast<std::uint32_t>(a.top),std::bit_cast<std::uint32_t>(a.right),std::bit_cast<std::uint32_t>(a.bottom)}); };
        return hooks;
    }
};
// 세 PE마다 전체 fixture를 실제 C++ 탐색/분기로 재생한다. 기대값을 별도로 계산하지 않는다.
void Replay(std::string_view edition) {
    EffectsScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRows);
    // 관찰 행의 모든 입력과 효과 순서를 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==6);const auto v=Inputs(row[2]);CHECK(v.size()==15);scene.Prepare(v);
        if (row[3]!="-") {
            // 입력 후보를 동일 순서로 등록한다.
            for (const auto& entry:Split(row[3],';')) { const auto n=Inputs(entry);CHECK(n.size()==9);scene.Node(n); }
        }
        RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,scene.Hooks());bool result=true;
        if (row[1]=="Displace") result=effects.Displace(Float(v[1]),Float(v[2]),v[0],v[5]);
        else effects.DropIsland(Float(v[1]),Float(v[2]),v[6],v[5]);
        const bool same=(row[4]=="-" || result==(Number(row[4])!=0)) && scene.events==(row[5]=="-" ? "" : row[5]);CHECK(same);
        if (!same) { std::printf("건설 효과 %s %s 행 %zu 불일치\n 입력 %s | %s\n 반환 %d/%s\n 기대 %s\n 실제 %s\n",std::string(edition).c_str(),row[1].c_str(),count,row[2].c_str(),row[3].c_str(),result,row[4].c_str(),row[5].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 10.78의 관계 거부/세 칸 탐색과 섬 정리 전체 관찰이다.
TEST_CASE(ConstructionEffects_ReplaysOriginals) { Replay("originals"); }
// CD의 두 칸 탐색/void 반환과 섬 정리 전체 관찰이다.
TEST_CASE(ConstructionEffects_ReplaysCd) { Replay("originalCD"); }
// 10.37을 별도의 실행 파일 근거로 비교한다.
TEST_CASE(ConstructionEffects_ReplaysExtra1037) { Replay("original1037"); }

// 누락 경계/자료 오류와 다른 풀 어댑터를 첫 효과 전에 거부한다.
TEST_CASE(ConstructionEffects_RejectsInvalidBindings) {
    EffectsScene scene(OriginalEdition::Patch1078);scene.Prepare({100,Bits(20),Bits(21),3,3,3,0x1000,0,1,2,0x10000,3,0,0,0});
    CHECK(Throws([&] { RawConstructionEffects bad(scene.pool,scene.hash,scene.types,scene.spots,scene.state,{}); }));
    CHECK(Throws([&] { RawConstructionEffects bad(scene.pool,scene.hash,scene.types,std::span<const std::uint8_t>{},scene.state,scene.Hooks()); }));
    RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,scene.Hooks());SidPool other(OriginalEdition::Patch1078,kCapacity,true);
    CHECK(Throws([&] { MakeConstructionEffectsRequestHooks(other,effects,{}); }));
    CHECK(Throws([&] { effects.Displace(20,21,300,3); }));CHECK(Throws([&] { effects.Displace(Float(0x7fc01234),21,100,3); }));CHECK(scene.events.empty());
    scene.state.blocked=true;effects.DropIsland(Float(0x7fc01234),21,0,3);CHECK(scene.events.empty());
}

// 실제 GUI에서 Dude의 실패 경로가 요청한 foot0을 검사한다. 원본443800→49ae80의 min/max 범위이며 게임 입력을 바꾸지 않는다.
TEST_CASE(ConstructionEffects_DudeZeroFootprint) {
    EffectsScene scene(OriginalEdition::Patch1078);scene.Prepare({70,Bits(20),Bits(21),0,0,3,0,0,1,2,0,3,0,0,0});
    RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,scene.Hooks());
    CHECK(effects.Displace(20,21,70,3));CHECK(scene.events=="A:20:21:21:22");
    scene.events.clear();scene.types[70].footX=-1;CHECK(Throws([&] { effects.Displace(20,21,70,3); }));CHECK(scene.events.empty());
}

// fall 뒤 현재 좌표, 삭제 뒤 현재 genus, 전파 뒤 현재 섬 타입/발자국을 사용하는지 검사한다.
TEST_CASE(ConstructionEffects_ReadsCurrentInputsAfterBoundaries) {
    EffectsScene scene(OriginalEdition::Patch1078);scene.Prepare({100,Bits(20),Bits(21),3,3,3,0,0,1,2,0,3,0,0,0});
    scene.Node({50,157,0,Bits(20),Bits(21),3,0,0,0});auto hooks=scene.Hooks();std::size_t called=0;
    hooks.broadcast=[&](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { scene.types[94].footX=4; };
    hooks.fall=[&](Sid sid) { auto raw=InputRaw(scene.pool,sid);Put(raw,14,Bits(1.25F));Put(raw,18,Bits(2.5F)); };
    hooks.sound=[&](float x,float y) { CHECK(x==1.25F && y==2.5F);++called; };
    hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(flags==0x48);scene.types[scene.pool.Slot(sid)[10]].flags2=0x10000; };
    hooks.notify=[&](Sid sid) { CHECK(sid.value==50);++called; };
    RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,hooks);effects.DropIsland(20,21,0,3);
    CHECK(called==2);CHECK(scene.events.starts_with("A:17:19:20:21"));
}

// 실제 요청→밀어내기→본 확정 실패→섬 정리→공통 삭제/풀 반납을 두 판본에서 연결한다.
TEST_CASE(ConstructionEffects_ConnectsRequestMovementAndFailureCleanup) {
    // 동일한 풀/해시를 공유하며 CD의 적대 후보 허용 차이도 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        EffectsScene scene(edition);scene.Prepare({100,Bits(20),Bits(21),3,3,3,0x1000,0,1,2,0x10000,3,0,0,0});
        scene.types[100].flags1=0x400;scene.types[100].flags2=0x10000;scene.types[100].group=1;
        // 실제 factory가 생성하는 섬 조각의 생성자/공통 삭제 자료다.
        for (const auto type:{94U,95U,157U,kOtherType}) { scene.types[type].footX=scene.types[type].footY=1;scene.types[type].group=1; }
        scene.types[94].footX=scene.types[94].footY=3;scene.types[kOtherType].flags2=0x10000;
        SquidFactory factory(scene.pool,scene.types);std::vector<Sid> pieces;
        // 생성한 자산을 공통 float 위치/해시 입력에 놓는다. 실제 타입별 Pop은 명시 경계다.
        const auto create=[&](std::uint32_t type,std::uint8_t owner) {
            const Sid sid=factory.Create(type,2);auto raw=scene.pool.AllocatedBytes(sid);raw[11]&=static_cast<std::uint8_t>(~4U);
            raw[scene.patch ? 34 : 32]=owner;Put(raw,14,Bits(20));Put(raw,18,Bits(21));auto& head=scene.hash.Cell(0,20,21);Put(raw,4,head,2);head=sid.value;return sid;
        };
        // 원본 섬 정리의 9/1/1 조각 수 조건을 만족하는 자료다.
        for (int i=0;i<9;++i) pieces.push_back(create(157,3));
        pieces.push_back(create(94,3));pieces.push_back(create(95,3));const Sid unit=create(kOtherType,4);
        SquidUnpop unpop(scene.pool,scene.hash,scene.spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);SquidDestroyHooks deletion;
        deletion.selected=[] { return Sid{}; };deletion.emit=[&](const SquidDestroyEvent& e) {
            if (e.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
            if (e.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        };
        auto hooks=scene.Hooks();std::size_t moved=0,deleted=0,assertions=0,confirmed=0;hooks.scan={};
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,scene.types[scene.pool.Slot(sid)[10]],flags); };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(sid==unit && x==20 && y==22 && flags==0);auto raw=scene.pool.AllocatedBytes(sid);Put(raw,14,Bits(x));Put(raw,18,Bits(y));raw[11]&=static_cast<std::uint8_t>(~4U);
            auto& head=scene.hash.Bucket(0,x,y);Put(raw,4,head,2);head=sid.value;++moved;
        };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(flags==(scene.pool.Slot(sid)[10]==94 ? 0x1000U : 0x1048U));destroy.Destroy(sid,flags,deletion);++deleted; };
        hooks.assertion=[&](std::string_view,int) { ++assertions; };
        RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,hooks);
        ConstructionRequestHooks requestHooks;requestHooks.mayPlace=[](const CanonPlacementQuery&) { return true; };
        requestHooks.confirm=[&](const ConstructionConfirmRequest& r) { CHECK(r.type==100);++confirmed;return false; };
        requestHooks=MakeConstructionEffectsRequestHooks(scene.pool,effects,requestHooks);
        std::vector<PriestPlainCanonType> frames(scene.types.size());frames[100]={InputFrames(0),3};ConstructionRequestState state;state.displaceGenus=0x10000;
        RawConstructionRequest request(scene.pool,scene.hash,scene.types,frames,scene.spots,state,requestHooks);
        if (scene.patch) {
            CHECK(!request.Request({100,20,21,3}));CHECK(moved==0 && deleted==0 && confirmed==0);
            scene.state.alliances[3*9+4]=1;
        }
        CHECK(!request.Request({100,20,21,3}));CHECK(moved==1 && confirmed==1 && deleted==11 && assertions==0);
        CHECK(!(scene.pool.Slot(unit)[11]&1));
        // 같은 finder의 next 저장 때문에 삭제 중에도 모든 섬 조각을 정확히 한 번 반납한다.
        for (const Sid sid:pieces) CHECK((scene.pool.Slot(sid)[11]&1)!=0);
        CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0);
    }
}

// 섬 정리 송신은 독립 server의 현재값을 읽고 빈 검색의 후속 반환도 유지한다.
TEST_CASE(ConstructionEffects_SharedServerControlsIslandBroadcast) {
    // 두 판본의 빈 검색 입력에는 섬 조각 개수 assert가 발생하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        EffectsScene scene(edition);std::vector<std::uint32_t> input(15);input[0]=kTypeIsland;input[3]=input[4]=1;input[8]=1;scene.Prepare(input);bool server=false;scene.state.sharedServer=&server;auto hooks=scene.Hooks();std::size_t broadcasts{},diagnostics{};hooks.broadcast=[&](std::uint32_t x,std::uint32_t y,std::uint32_t,std::uint32_t player) { CHECK(x==20*65536U && y==21*65536U && player==1);++broadcasts; };hooks.assertion=[&](std::string_view,int) { ++diagnostics; };RawConstructionEffects effects(scene.pool,scene.hash,scene.types,scene.spots,scene.state,std::move(hooks));effects.DropIsland(20,21,0,1);CHECK(broadcasts==0 && diagnostics==0);server=true;scene.state.server=false;effects.DropIsland(20,21,0,1);CHECK(broadcasts==1 && diagnostics==0);
    }
}
