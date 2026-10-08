// 세 실제 PE의 배치 접두 관찰과 사제 생성 연결을 재생한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacement.h"
#include "o/SquidFactory.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 모양 9종·좌표 16종·전역/소유자 7종을 세 PE에서 각각 관찰했다.
constexpr std::size_t kRows=1008,kTotalRows=3*kRows;
// 실행기 없는 회귀에서도 저장된 원본 관찰만 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTPLACEMENT_FIXTURE);return data.rows; }
struct PlacementScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    PriestPlacementState state;
    PriestPlacementRect rect;
    PriestPlacementQuery query;
    std::uint32_t lower{},change{};
    std::string events;
    // 자료 참조와 판본을 실제 구현의 수명 동안 유지한다.
    explicit PlacementScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {}
    // 하위 모양/후반 경계만 기록하고 지도 판단은 구현에 맡긴다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // fixture의 입력 열만 읽어 원본에 공급한 상태를 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==23);events.clear();query={Number(row[1]),std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3])),Number(row[5]),Number(row[4]),Number(row[6])};
        types[query.type].flags2=Number(row[7]);state={Number(row[8]),std::stoi(row[9]),0xffffffff};
        rect={std::bit_cast<float>(Number(row[10])),std::bit_cast<float>(Number(row[11])),std::bit_cast<float>(Number(row[12])),std::bit_cast<float>(Number(row[13]))};
        lower=Number(row[14]);change=Number(row[15]);
    }
    // 모양 조회 중 전역 변화와 후반 구간의 반환/전역 효과를 원본 입력과 같이 공급한다.
    PriestPlacementHooks Hooks() {
        return {[this](std::uint32_t type,std::uint32_t arg,std::uint32_t flags) {
                CHECK(type==query.type && arg==query.type && flags==query.flags && state.blockedRelation==0);Record("H:"+std::to_string(type)+':'+std::to_string(arg)+':'+std::to_string(flags));
                if (change==1) { state.localPlayer=17;state.forcePlacement=1;types[type].flags2=0; }return rect; },
            [this](const PriestPlacementQuery& input,bool localOwner) {
                CHECK(input.type==query.type && input.x==query.x && input.y==query.y && state.blockedRelation==0);
                Record("G:"+std::to_string(localOwner ? 1 : 0)+':'+std::to_string(input.owner)+':'+std::to_string(input.flags)+':'+std::to_string(input.mode)+':'+std::to_string(lower));
                state.blockedRelation=0x77;if (change==2) { state.forcePlacement=9;state.localPlayer=-7; }return lower!=0; }};
    }
};
// 실제 접두가 하위 구간에 진입했는지와 반환/전역/사건을 대조한다.
void Replay(std::string_view edition) {
    CHECK(Fixture().size()==kTotalRows);PlacementScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 구현으로 기대 여백이나 허용 여부를 계산하지 않고 원본 관찰만 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawPriestPlacement placement(scene.pool,scene.types,scene.state,scene.Hooks());
        const bool allowed=placement.MayPlace(scene.query);
        const bool same=allowed==(Number(row[16])!=0) && scene.state.blockedRelation==Number(row[18]) && scene.state.forcePlacement==Number(row[19]) &&
            static_cast<std::uint32_t>(scene.state.localPlayer)==Number(row[20]) && scene.types[scene.query.type].flags2==Number(row[21]) && scene.events==row[22];
        CHECK(same);
        if (!same) { std::printf("Priest placement %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[22].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치판 여백과 지도 경계의 실제 판단을 재생한다.
TEST_CASE(priest_placement_patch_prefix_x86) { Replay("originals"); }
// CD판의 중간 float 저장과 별도 명령을 재생한다.
TEST_CASE(priest_placement_cd_prefix_x86) { Replay("originalCD"); }
// 추가 10.37 바이너리의 접두를 별도로 재생한다.
TEST_CASE(priest_placement_1037_prefix_x86) { Replay("original1037"); }
// 누락 자료·비사제·비유한 입력/잘못된 연결과 원본 값 인자의 보존을 검사한다.
TEST_CASE(priest_placement_guards_and_argument_snapshot) {
    // 두 판본에 같은 C++ 입력 계약을 적용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PlacementScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.shape={};
        CHECK(Throws([&] { RawPriestPlacement missing(scene.pool,scene.types,scene.state,hooks); }));
        hooks=scene.Hooks();hooks.inspectGeometry={};CHECK(Throws([&] { RawPriestPlacement missing(scene.pool,scene.types,scene.state,hooks); }));
        CHECK(Throws([&] { RawPriestPlacement shortTypes(scene.pool,std::span(scene.types).first(1),scene.state,scene.Hooks()); }));
        RawPriestPlacement placement(scene.pool,scene.types,scene.state,scene.Hooks());scene.query.type=255;
        CHECK(Throws([&] { placement.MayPlace(scene.query); }));scene.query.type=158;scene.types[158].flags2=0;
        CHECK(Throws([&] { placement.MayPlace(scene.query); }));scene.types[158].flags2=0x210000;scene.query.x=std::numeric_limits<float>::infinity();
        CHECK(Throws([&] { placement.MayPlace(scene.query); }));CHECK(scene.events.empty());
        scene.query.x=20;scene.rect.right=std::numeric_limits<float>::quiet_NaN();CHECK(Throws([&] { placement.MayPlace(scene.query); }));
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakePriestPlacementHooks(other,placement,{}); }));
        // 모양 조회 중 호출자의 요청을 바꾸어도 원본 스택 값 인자/로컬 비교 결과는 유지한다.
        PriestPlacementQuery submitted{158,20,21,7,0x108,1};scene.state={0,8,0xffffffff};int geometryCalls=0;
        RawPriestPlacement copied(scene.pool,scene.types,scene.state,{
            [&](std::uint32_t type,std::uint32_t arg,std::uint32_t flags) {
                CHECK(type==158 && arg==158 && flags==7);submitted={82,256,256,0,0,0};scene.state.localPlayer=0;return PriestPlacementRect{0,0,16,11}; },
            [&](const PriestPlacementQuery& request,bool localOwner) {
                CHECK(request.type==158 && request.x==20 && request.y==21 && request.flags==7 && request.owner==0x108 && request.mode==1 && localOwner);++geometryCalls;return true; }});
        CHECK(copied.MayPlace(submitted));CHECK(geometryCalls==1 && submitted.type==82 && scene.state.localPlayer==0);
    }
}
// 지도 공간이 있는 생성 경로에서 실제 배치 접두와 실제 사제 생성자를 연결한다.
TEST_CASE(priest_placement_prefix_connected_to_spawn_and_factory) {
    // 후반 충돌/관계 본체와 일반 Pop은 명시한 기록 경계다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[kPriestType].constructorAddress=TypeConstructorAddress(edition,kPriestType);types[kPriestType].flags2=0x210000;types[kPriestType].maxHitPoints=200;
        SquidFactory factory(pool,types);PriestPlacementState state{0,1,0xffffffff};int shapeCalls=0,geometryCalls=0,pops=0;Sid born{};
        RawPriestPlacement placement(pool,types,state,{
            [&](std::uint32_t type,std::uint32_t arg,std::uint32_t flags) { CHECK(type==kPriestType && arg==kPriestType && flags==0);++shapeCalls;return PriestPlacementRect{0,0,16,11}; },
            [&](const PriestPlacementQuery& query,bool localOwner) { CHECK(localOwner && query.owner==1 && query.flags==0 && query.mode==0);++geometryCalls;return geometryCalls==2; }});
        std::vector<std::uint8_t> spots(65536,6);PriestSpawnState spawnState;
        RawPriestSpawn spawn(pool,types,spots,spawnState,MakePriestPlacementHooks(pool,placement,{
            {},[&](std::uint32_t type,std::uint32_t flags) { born=factory.Create(type,flags);return born; },
            [&](Sid sid,std::uint32_t owner) { CHECK(sid==born && owner==1 && Get(pool.Slot(sid),12,2)==0x8101); },
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && x==21 && y==21 && flags==0);++pops; },
            [](Sid) { CHECK(false); },[&](Sid sid) { CHECK(sid==born); }}));
        spawn.Spawn(20.75f,21.9f,kPriestType,0x8101);
        CHECK(born.value==(patch ? 15000 : 6000) && shapeCalls==2 && geometryCalls==2 && pops==1 && state.blockedRelation==0);
    }
}
