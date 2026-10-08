// 세 실제 PE의 사제 배치 미리보기/표면/관계 관찰을 접두와 함께 재생한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacementPreview.h"
#include "o/SquidFactory.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 사각형 12종·지도 5종·전역/소유자 9종·조회 중 변화 3종의 독립 입력 수다.
constexpr std::size_t kRows=1620,kTotalRows=3*kRows,kCells=65536;
// 실제 실행기에 넣은 네 표면 SID/타입/genus/소유자 값이다.
struct PreviewSurface { Sid sid;std::uint32_t type,genus;std::uint8_t owner; };
constexpr std::array<PreviewSurface,4> kSurfaces{{{{50},83,2,1},{{51},84,2,2},{{52},85,4,0},{{53},86,2,8}}};
// 원본/Python 없는 검사에서도 저장 관찰만 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTPREVIEW_FIXTURE);return data.rows; }
struct MapInput { std::vector<std::uint8_t> spots;std::vector<std::uint16_t> surface; };
// 지도 입력만 구성한다. 기대 차단/허용 결과는 계산하지 않는다.
const auto& Maps() {
    static const auto inputs=[] {
        std::array<MapInput,5> result;
        // 다섯 지도는 원본 실행기에 공급한 주기와 같은 입력이다.
        for (std::size_t variant=0;variant<result.size();++variant) {
            auto& map=result[variant];map.spots.resize(kCells);map.surface.resize(kCells);
            // 비대칭 주기를 사용해 x/y 전치와 경계 칸을 구별한다.
            for (std::size_t index=0;index<kCells;++index) {
                const auto x=index%256,y=index/256,choice=(x+2*y)%5;std::uint16_t sid{};std::uint8_t spot{};
                if (variant==1) { sid=50;spot=6; }
                if (variant==2 || variant==4) { const std::array<std::uint16_t,5> ids{0,50,51,52,53};sid=ids[choice];spot=variant==4 && (3*x+y)%7==0 ? 16 : 6; }
                if (variant==3) { sid=choice!=3 ? 0xffff : 52;spot=choice!=3 ? 16 : 6; }
                map.spots[index]=spot;map.surface[index]=sid;
            }
        }
        return result;
    }();return inputs;
}
struct PreviewScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(kCells);
    std::vector<std::uint16_t> surfaceMap=std::vector<std::uint16_t>(kCells);
    std::array<std::span<std::uint8_t>,4> raw;
    PriestPlacementState placementState;
    PriestPlacementPreviewState previewState;
    PriestPlacementQuery query;
    PriestPlacementRect shape;
    SquidSearchArea bounds;
    std::uint32_t lower{},mutation{};
    std::string events;
    // 표면 raw 참조를 확보한 뒤 free/void/dead 비트를 입력으로 자유롭게 구성한다.
    explicit PreviewScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 독립 실행기와 같은 슬롯 번호까지 확보한다.
        for (std::uint16_t sid=5;sid<=53;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 이후 재생에서는 할당 상태로 표면을 필터링하지 않는다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=pool.AllocatedBytes(kSurfaces[index].sid);
    }
    // 외부 모양/충돌 경계의 사건만 원본과 같은 형식으로 기록한다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // 원본 실행기와 같은 조회 경계의 입력 변화를 공급한다.
    void Mutate() {
        if (mutation==1) {
            const auto index=static_cast<std::int64_t>(bounds.bottom+1)*256+bounds.right+1;
            if (index>=0 && index<static_cast<std::int64_t>(spots.size())) spots[static_cast<std::size_t>(index)]=16;
        }
        if (mutation==2) {
            previewState.editor=0;previewState.useAlliances=1;previewState.alliances[19]=0;types[83].flags2=4;
            raw[0][pool.Edition()==OriginalEdition::Patch1078 ? 34 : 32]=8;
        }
    }
    // fixture의 입력 열만 사용해 지도/타입/전역/배열을 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==36);events.clear();query={Number(row[1]),std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3])),Number(row[5]),Number(row[4]),Number(row[6])};
        types[query.type].flags2=Number(row[7]);placementState={Number(row[8]),std::stoi(row[9]),0xffffffff};
        shape={std::bit_cast<float>(Number(row[10])),std::bit_cast<float>(Number(row[11])),std::bit_cast<float>(Number(row[12])),std::bit_cast<float>(Number(row[13]))};
        lower=Number(row[14]);CHECK(Number(row[15])==0);bounds={std::stoi(row[16]),std::stoi(row[17]),std::stoi(row[18]),std::stoi(row[19])};
        const auto variant=Number(row[20]);spots=Maps()[variant].spots;surfaceMap=Maps()[variant].surface;
        previewState.blocked.fill(0xa5);previewState.editor=Number(row[21]);previewState.useAlliances=Number(row[22]);previewState.alliances.fill(0);previewState.alliances[19]=Number(row[23]);mutation=Number(row[24]);
        if ((query.owner&255)==255) previewState.alliances[8]=Number(row[23]);
        // 같은 raw/타입 입력을 매 행 복구한다. 실제 구현에서 free/dead 필터를 추가하면 결과가 달라진다.
        for (std::size_t index=0;index<raw.size();++index) {
            const auto& surface=kSurfaces[index];std::fill(raw[index].begin(),raw[index].end(),std::uint8_t{0xcd});raw[index][10]=static_cast<std::uint8_t>(surface.type);
            raw[index][11]=static_cast<std::uint8_t>((variant+surface.sid.value)%16);raw[index][pool.Edition()==OriginalEdition::Patch1078 ? 34 : 32]=surface.owner;types[surface.type].flags2=surface.genus;
        }
    }
    // 미복원된 모양과 충돌 경계만 제공한다.
    PriestPlacementPreviewHooks PreviewHooks() {
        return {[this](const PriestPlacementQuery& request) {
                CHECK(request.type==query.type && request.x==query.x && request.y==query.y && request.flags==query.flags);
                CHECK(std::all_of(previewState.blocked.begin(),previewState.blocked.end(),[](std::uint8_t value) { return value==0; }));
                Record("B:"+std::to_string(request.type)+':'+std::to_string(std::bit_cast<std::uint32_t>(request.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(request.y))+':'+std::to_string(request.flags));
                Record("R:"+std::to_string(bounds.left)+':'+std::to_string(bounds.top)+':'+std::to_string(bounds.right)+':'+std::to_string(bounds.bottom));Mutate();return bounds; },
            [this](const PriestPlacementQuery& request,bool localOwner) {
                CHECK(request.type==query.type && request.x==query.x && request.y==query.y);
                Record("C:"+std::to_string(localOwner ? 1 : 0)+':'+std::to_string(request.owner)+':'+std::to_string(request.flags)+':'+std::to_string(request.mode)+':'+std::to_string(lower));return lower!=0; }};
    }
    // 원본 접두의 모양 조회를 같은 기록 경계로 구성한다.
    PriestPlacementHooks PlacementHooks() {
        return {[this](std::uint32_t type,std::uint32_t arg,std::uint32_t flags) {
            CHECK(type==query.type && arg==query.type && flags==query.flags);Record("H:"+std::to_string(type)+':'+std::to_string(arg)+':'+std::to_string(flags));return shape; },{}};
    }
};
// 현재 미리보기 전체/지도/표면 raw/관계와 반환/사건을 독립 관찰과 대조한다.
void Replay(std::string_view edition) {
    CHECK(Fixture().size()==kTotalRows);PreviewScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 기대 mask를 구현으로 계산하지 않고 실제 PE가 저장한 전체 배열과 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;scene.Prepare(row);
        RawPriestPlacementPreview preview(scene.pool,scene.types,scene.spots,scene.surfaceMap,scene.previewState,scene.PreviewHooks());
        RawPriestPlacement placement(scene.pool,scene.types,scene.placementState,MakePriestPlacementPreviewHooks(scene.pool,preview,scene.PlacementHooks()));
        const auto mapBytes=std::span(reinterpret_cast<const std::uint8_t*>(scene.surfaceMap.data()),scene.surfaceMap.size()*sizeof(std::uint16_t));
        const bool allowed=placement.MayPlace(scene.query);
        const bool same=allowed==(Number(row[25])!=0) && scene.placementState.blockedRelation==Number(row[26]) && scene.previewState.editor==Number(row[27]) &&
            scene.previewState.useAlliances==Number(row[28]) && scene.previewState.alliances[19]==Number(row[29]) && scene.types[83].flags2==Number(row[30]) &&
            Hex(scene.previewState.blocked)==row[31] && Hex(scene.pool.Slot(Sid{50}))==row[32] && Adler(scene.spots)==Number(row[33]) && Adler(mapBytes)==Number(row[34]) && scene.events==row[35];
        CHECK(same);
        if (!same) { std::printf("Priest preview %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[35].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치의 실제 미리보기/표면/관계와 가장자리 거부를 대조한다.
TEST_CASE(priest_preview_patch_x86) { Replay("originals"); }
// CD의 인라인 표면/관계와 x=256 linear 공간 읽기 및 y=254/255 처리를 대조한다.
TEST_CASE(priest_preview_cd_x86) { Replay("originalCD"); }
// 추가 10.37의 실제 본문도 별도로 대조한다.
TEST_CASE(priest_preview_1037_x86) { Replay("original1037"); }
// 자료 누락과 원본 assert/정의되지 않은 읽기를 C++에서 안전하게 진단한다.
TEST_CASE(priest_preview_guards) {
    // 두 판본의 지도/관계/산술 범위와 연결 계약을 확인한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PreviewScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.PreviewHooks();hooks.bounds={};
        CHECK(Throws([&] { RawPriestPlacementPreview missing(scene.pool,scene.types,scene.spots,scene.surfaceMap,scene.previewState,hooks); }));
        CHECK(Throws([&] { RawPriestPlacementPreview shortMap(scene.pool,scene.types,scene.spots,std::span(scene.surfaceMap).first(1),scene.previewState,scene.PreviewHooks()); }));
        RawPriestPlacementPreview preview(scene.pool,scene.types,scene.spots,scene.surfaceMap,scene.previewState,scene.PreviewHooks());
        scene.bounds={1,1,11,1};CHECK(Throws([&] { preview.Inspect(scene.query,true); }));
        scene.bounds.right=std::numeric_limits<int>::max();CHECK(Throws([&] { preview.Inspect(scene.query,true); }));
        scene.bounds={20,255,20,255};
        if (edition==OriginalEdition::Patch1078) CHECK(!preview.Inspect(scene.query,true));
        else CHECK(Throws([&] { preview.Inspect(scene.query,true); }));
        scene.bounds={20,21,20,21};std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{});std::fill(scene.surfaceMap.begin(),scene.surfaceMap.end(),std::uint16_t{50});
        scene.previewState.editor=0;scene.previewState.useAlliances=1;scene.query.owner=128;CHECK(Throws([&] { preview.Inspect(scene.query,true); }));
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakePriestPlacementPreviewHooks(other,preview,{}); }));
    }
}
// 미리보기의 차단 표시가 원본처럼 실제 배치 허용 결과와 독립인지 생성까지 연결한다.
TEST_CASE(priest_preview_display_mask_does_not_replace_collision_result) {
    // 사제 생성자는 실제 실행하고 모양/충돌/일반 Pop은 기록 경계다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[kPriestType].constructorAddress=TypeConstructorAddress(edition,kPriestType);types[kPriestType].flags2=0x210000;SquidFactory factory(pool,types);
        std::vector<std::uint8_t> spots(kCells,6);std::vector<std::uint16_t> surfaceMap(kCells);PriestPlacementPreviewState previewState;PriestPlacementState placementState{0,1,0xffffffff};
        int collisions=0,pops=0;Sid born{};
        RawPriestPlacementPreview preview(pool,types,spots,surfaceMap,previewState,{
            [](const PriestPlacementQuery& query) { const auto x=static_cast<int>(query.x),y=static_cast<int>(query.y);return SquidSearchArea{x,y,x,y}; },
            [&](const PriestPlacementQuery&,bool local) { CHECK(local);++collisions;return true; }});
        RawPriestPlacement placement(pool,types,placementState,MakePriestPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t,std::uint32_t,std::uint32_t) { return PriestPlacementRect{0,0,16,11}; },{}}));
        PriestSpawnState spawnState;RawPriestSpawn spawn(pool,types,spots,spawnState,MakePriestPlacementHooks(pool,placement,{
            {},[&](std::uint32_t type,std::uint32_t flags) { born=factory.Create(type,flags);return born; },[](Sid,std::uint32_t) {},
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && x==21 && y==22 && flags==0);++pops; },[](Sid) { CHECK(false); },[&](Sid sid) { CHECK(sid==born); }}));
        spawn.Spawn(20.75f,21.9f,kPriestType,0x8101);
        CHECK(born.value==(patch ? 15000 : 6000) && collisions==1 && pops==1 && Get(pool.Slot(born),12,2)==0x8101);
        CHECK(std::count(previewState.blocked.begin(),previewState.blocked.end(),std::uint8_t{1})==9);
    }
}
