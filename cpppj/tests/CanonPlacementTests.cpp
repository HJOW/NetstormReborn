// 세 실제 PE의 배치 접두 관찰과 일반 패턴 인자를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacement.h"
#include "o/RawCanonPixelShape.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 모양 10종·좌표 16종·전역/소유자 7종을 세 PE에서 각각 관찰했다.
constexpr std::size_t kRows=1120,kTotalRows=3*kRows;
// 실행기 없는 회귀에서도 저장된 원본 관찰만 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_CANONPLACEMENT_FIXTURE);return data.rows; }
struct PlacementScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    PriestPlacementState state;
    PriestPlacementRect rect;
    CanonPlacementQuery query;
    std::uint32_t lower{},change{};
    std::string events;
    // 자료 참조와 판본을 실제 구현의 수명 동안 유지한다.
    explicit PlacementScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {}
    // 하위 모양/후반 경계만 기록하고 지도 판단은 구현에 맡긴다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // fixture의 입력 열만 읽어 원본에 공급한 상태를 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==24);events.clear();query={Number(row[1]),Number(row[2]),std::bit_cast<float>(Number(row[3])),std::bit_cast<float>(Number(row[4])),Number(row[6]),Number(row[5]),Number(row[7])};
        types[query.type].flags2=Number(row[8]);state={Number(row[9]),std::stoi(row[10]),0xffffffff};
        rect={std::bit_cast<float>(Number(row[11])),std::bit_cast<float>(Number(row[12])),std::bit_cast<float>(Number(row[13])),std::bit_cast<float>(Number(row[14]))};
        lower=Number(row[15]);change=Number(row[16]);
    }
    // 모양 조회 중 전역 변화와 후반 구간의 반환/전역 효과를 원본 입력과 같이 공급한다.
    CanonPlacementHooks Hooks() {
        return {[this](std::uint32_t type,std::uint32_t arg,std::uint32_t flags) {
                CHECK(type==query.type && arg==query.argument && flags==query.flags && state.blockedRelation==0);Record("H:"+std::to_string(type)+':'+std::to_string(arg)+':'+std::to_string(flags));
                if (change==1) { state.localPlayer=17;state.forcePlacement=1;types[type].flags2=0; }return rect; },
            [this](const CanonPlacementQuery& input,bool localOwner) {
                CHECK(input.type==query.type && input.argument==query.argument && input.x==query.x && input.y==query.y && state.blockedRelation==0);
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
        scene.Prepare(row);RawCanonPlacement placement(scene.pool,scene.types,scene.state,scene.Hooks());
        const bool allowed=placement.MayPlace(scene.query);
        const bool same=allowed==(Number(row[17])!=0) && scene.state.blockedRelation==Number(row[19]) && scene.state.forcePlacement==Number(row[20]) &&
            static_cast<std::uint32_t>(scene.state.localPlayer)==Number(row[21]) && scene.types[scene.query.type].flags2==Number(row[22]) && scene.events==row[23];
        CHECK(same);
        if (!same) { std::printf("Canon placement %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[23].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치판 여백과 지도 경계의 실제 판단을 재생한다.
TEST_CASE(canon_placement_patch_prefix_x86) { Replay("originals"); }
// CD판의 중간 float 저장과 별도 명령을 재생한다.
TEST_CASE(canon_placement_cd_prefix_x86) { Replay("originalCD"); }
// 추가 10.37 바이너리의 접두를 별도로 재생한다.
TEST_CASE(canon_placement_1037_prefix_x86) { Replay("original1037"); }
// 사제 플래그 없이 일반 타입을 처리하고 입력/로컬 비교를 값으로 캡처한다.
TEST_CASE(canon_placement_general_arguments_and_guards) {
    // 두 판본의 별도 인자/부호 BYTE/진입 초기화를 동일 계약으로 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        PriestPlacementState state{0,8,99};CanonPlacementQuery submitted{82,25,20,21,6,0x108,1};int calls=0;
        CanonPlacementHooks hooks{
            [&](std::uint32_t type,std::uint32_t argument,std::uint32_t flags) {
                CHECK(type==82 && argument==25 && flags==6 && state.blockedRelation==0);
                submitted={70,67,0,0,0,0,0};state.localPlayer=17;return PriestPlacementRect{0,0,16,11}; },
            [&](const CanonPlacementQuery& query,bool localOwner) {
                CHECK(query.type==82 && query.argument==25 && query.x==20 && query.y==21 && query.flags==6 && query.owner==0x108 && query.mode==1 && localOwner);
                ++calls;return false; }};
        RawCanonPlacement placement(pool,types,state,hooks);CHECK(!placement.MayPlace(submitted) && calls==1 && state.localPlayer==17);
        state.blockedRelation=99;state.forcePlacement=1;CHECK(placement.MayPlace({82,0xffffffff,0,0,0,255,0xffffffff}) && state.blockedRelation==0 && calls==1);
        state.forcePlacement=0;types[82].flags2=0x02000000;CHECK(placement.MayPlace({82,67,-1,-1,0,0,0}) && calls==1);
        CHECK(Throws([&] { placement.MayPlace({69,0,0,0,0,0,0}); }));
        CHECK(Throws([&] { placement.MayPlace({255,0,0,0,0,0,0}); }));
        CHECK(Throws([&] { placement.MayPlace({82,0,std::numeric_limits<float>::infinity(),0,0,0,0}); }));
        hooks.inspectGeometry={};CHECK(Throws([&] { RawCanonPlacement missing(pool,types,state,hooks); }));
        CHECK(Throws([&] { RawCanonPlacement missing(pool,std::span(types).first(1),state,hooks); }));
    }
}
// 실제 복원 패턴과 SHP를 접두에 연결하여 argument/방향 전달과 경계 거부를 검사한다.
TEST_CASE(canon_placement_connected_to_pattern_pixel_getter) {
    // 각 판본에서 서로 다른 다리 패턴을 계산하고 접두의 경계 판단에 사용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);const auto count=edition==OriginalEdition::Patch1078 ? 188U : 171U;
        std::vector<RiftTypeRecord> types(count);std::vector<PriestPlainCanonType> frames(count);
        std::vector<SquidDisplayShape> shapes(count);std::vector<PriestTypeHotspot> hotspots(count);
        PriestPlacementGeometryState geometry;PriestPlacementShapeState pixelState;PriestPlacementState state{0,8,99};
        geometry.patternTypes={107,82,94,157,131,129,140,142};std::vector<FrameCode> codes;
        // 유효 원본 패턴에 필요한 방향/프레임 라벨을 정상 메타 입력으로 제공한다.
        for (int side='A';side<='P';++side) {
            // 한 자리 프레임 라벨 전체를 물리 SHP 표에 대응시킨다.
            for (int number=0;number<=10;++number) codes.push_back({static_cast<std::uint8_t>(side),'P',static_cast<std::uint8_t>(number),0});
        }
        frames[82].frames=RiftTypeFrames(std::move(codes));const auto n=static_cast<int>(frames[82].frames.Codes().size());
        shapes[82]={n,true,std::vector<SquidDisplayFrame>(static_cast<std::size_t>(n),{16,11,8,5,0,0})};
        RawCanonPixelShape shape(pool,types,frames,shapes,hotspots,geometry,pixelState);int calls=0;
        RawCanonPlacement placement(pool,types,state,MakeCanonShapePlacementHooks(pool,shape,{{},[&](const CanonPlacementQuery& query,bool localOwner) {
            CHECK(query.type==82 && query.argument==25 && query.flags==6 && localOwner);++calls;return true; }}));
        CHECK(placement.MayPlace({82,25,20,21,6,8,0}) && calls==1);
        CHECK(!placement.MayPlace({82,25,0,0,6,8,0}) && calls==1 && state.blockedRelation==0);
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakeCanonShapePlacementHooks(other,shape,{}); }));
    }
}
