// 실제 세 PE의 사제 생성/나선 탐색/WORD/Carrier 검사 관찰을 재생한다.
#include "RawSceneSupport.h"
#include "o/RawPriestSpawn.h"
#include "o/RawPriestOwner.h"
#include "o/SquidFactory.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 세 PE의 독립 입력 수와 대체 생성 SID/가상 표 기록값이다.
constexpr std::size_t kPatchRows=408,kCdRows=414,kTotalRows=kPatchRows+2*kCdRows;
constexpr Sid kSpawned{100};
constexpr std::uint32_t kSpawnVtable=0x20000100;
// 원본/Python 없는 검사에서도 저장된 관찰만 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTSPAWN_FIXTURE);return data.rows; }
struct SpawnScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    PriestSpawnState state;
    std::span<std::uint8_t> raw;
    std::string events;
    std::int64_t pattern{};
    std::uint32_t queries{},change{},afterGenus{};
    // 후속 fixture의 합성 free 바이트도 직접 초기화할 수 있게 span을 보관한다.
    explicit SpawnScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 실행기와 같은 생성 SID까지 확보한다.
        for (std::uint16_t id=5;id<=kSpawned.value;++id) CHECK(pool.Allocate(2)==Sid{id});
        raw=pool.AllocatedBytes(kSpawned);
    }
    // 하위 호출을 원본 관찰 형식으로 기록한다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // 실제 배치 판단 대신 실행기에 공급한 반환 입력을 같은 순서로 제공한다.
    std::uint32_t Result() const {
        if (pattern==0xffffffff) return 0xffffffff;
        if (pattern==-2) return queries%2;
        return pattern>0 && queries>=pattern ? 1U : 0U;
    }
    // 원본에서 대체한 하위 효과만 공급하며 나선/조건/WORD 쓰기는 구현에 맡긴다.
    PriestSpawnHooks Hooks() {
        return {[this](const PriestPlacementQuery& query) {
                ++queries;const auto result=Result();CHECK(query.flags==0 && query.mode==0);
                Record("Q:"+std::to_string(query.type)+':'+std::to_string(std::bit_cast<std::uint32_t>(query.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(query.y))+':'+std::to_string(query.flags)+':'+std::to_string(query.owner)+':'+std::to_string(query.mode)+':'+std::to_string(result));
                if (change==1 && queries==1) {
                    const auto cell=static_cast<std::size_t>(query.y)*256+static_cast<std::size_t>(query.x);spots[cell]=0;spots[cell-256]=0;
                }
                if (change==2) state.bridgeType=query.type;
                return result!=0; },
            [this](std::uint32_t type,std::uint32_t flags) {
                CHECK(flags==0);std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});Put(raw,0,kSpawnVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;
                Record("N:"+std::to_string(type)+':'+std::to_string(flags)+':'+std::to_string(kSpawned.value));if (change==2) state.bridgeType=82;return kSpawned; },
            [this](Sid sid,std::uint32_t owner) { CHECK(sid==kSpawned);Record("O:"+std::to_string(sid.value)+':'+std::to_string(owner)); },
            [this](Sid sid,float x,float y,std::uint32_t flags) {
                CHECK(sid==kSpawned && flags==0);Record("L:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags));
                if (change==3) { raw[10]=82;types[82].flags2=afterGenus; } },
            [this](Sid sid) { CHECK(sid==kSpawned);Record("S:"+std::to_string(sid.value)); },
            [this](Sid sid) { CHECK(sid==kSpawned);Record("K:"+std::to_string(sid.value)); }};
    }
    // 입력 칸만으로 공간/반환/전역/초기 raw를 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==13);events.clear();queries=0;pattern=std::stoll(row[6]);change=Number(row[7]);afterGenus=Number(row[8]);
        state.bridgeType=82;state.checkingPlacement=false;types[158].flags2=types[82].flags2=0x210000;
        std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(row[5])));std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
    }
};
// 세 실제 PE의 전체 반환 이후 현재 전역/사건/raw/map을 비교한다.
void Replay(std::string_view edition) {
    SpawnScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==kTotalRows);
    // 기대 위치를 구현에서 계산하지 않고 저장된 실제 관찰과 대조한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawPriestSpawn spawn(scene.pool,scene.types,scene.spots,scene.state,scene.Hooks());
        spawn.Spawn(std::bit_cast<float>(Number(row[1])),std::bit_cast<float>(Number(row[2])),Number(row[3]),static_cast<std::uint16_t>(Number(row[4])));
        const bool same=scene.state.bridgeType==Number(row[9]) && scene.events==row[10] && Hex(scene.pool.Slot(kSpawned))==row[11] && Adler(scene.spots)==Number(row[12]);
        CHECK(same);
        if (!same) { std::printf("Priest spawn %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[10].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
}
// 패치 전체 나선·스냅·WORD·Carrier 래퍼를 대조한다.
TEST_CASE(priest_spawn_patch_x86) { Replay("originals"); }
// CD의 별도 명령과 Pop 뒤 비 Carrier 타입의 Carrier 검사 생략을 대조한다.
TEST_CASE(priest_spawn_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 관찰을 별도로 대조한다.
TEST_CASE(priest_spawn_1037_x86) { Replay("original1037"); }
// 잘못된 자료/비유한 좌표/타입과 실패 진단의 판본 차이를 검사한다.
TEST_CASE(priest_spawn_guards_and_failure_diagnostics) {
    // 누락 경계와 잘못된 입력은 생성 전에 거부한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SpawnScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.mayPlace={};
        CHECK(Throws([&] { RawPriestSpawn missing(scene.pool,scene.types,scene.spots,scene.state,hooks); }));
        CHECK(Throws([&] { RawPriestSpawn shortMap(scene.pool,scene.types,std::span(scene.spots).first(1),scene.state,scene.Hooks()); }));
        CHECK(Throws([&] { RawPriestSpawn shortTypes(scene.pool,std::span(scene.types).first(1),scene.spots,scene.state,scene.Hooks()); }));
        RawPriestSpawn spawn(scene.pool,scene.types,scene.spots,scene.state,scene.Hooks());
        CHECK(Throws([&] { spawn.Spawn(std::bit_cast<float>(0x7fc00000U),21,158,1); }));
        CHECK(Throws([&] { spawn.Spawn(20,std::numeric_limits<float>::infinity(),158,1); }));
        CHECK(Throws([&] { spawn.Spawn(20,21,6,1); }));CHECK(Throws([&] { spawn.Spawn(20,21,255,1); }));CHECK(scene.events.empty());
        std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{6});scene.pattern=0;scene.state.checkingPlacement=true;
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { spawn.Spawn(20,21,158,1); }));
        else spawn.Spawn(20,21,158,1);
        CHECK(scene.queries==132 && scene.events.find("N:")==std::string::npos);
        // 패치는 non-Carrier 검사가 항상 assert이며 CD는 가상 호출 없이 반환한다.
        scene.events.clear();std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{});scene.types[82].flags2=0;
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { spawn.Spawn(20,21,82,1); }));
        else spawn.Spawn(20,21,82,1);
        CHECK(scene.events.find("K:")==std::string::npos && scene.events.find("N:")!=std::string::npos);
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakePriestSpawnHooks(other,spawn,{}); }));
    }
}

// 종속 해방→나선→실제 서버 SID/사제 생성자→실제 사제 소유자까지 연결한다.
TEST_CASE(priest_spawn_release_uses_real_factory_and_priest_owner) {
    // 일반 Pop/가상 Carrier 검사는 기록 경계이며 두 판본의 실제 생성·소유자 쓰기를 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);const auto freeBefore=pool.FreeCount();
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);auto& priestType=types[kPriestType];
        priestType.constructorAddress=TypeConstructorAddress(edition,kPriestType);priestType.flags1=0x69012;priestType.flags2=0x210000;priestType.maxHitPoints=200;priestType.footX=priestType.footY=1;
        SquidFactory factory(pool,types);const Sid parent=factory.Create(kPriestType,2);const Sid contained=pool.Allocate(2);
        auto parentRaw=pool.AllocatedBytes(parent);auto childRaw=pool.AllocatedBytes(contained);
        Put(parentRaw,14,std::bit_cast<std::uint32_t>(20.75f));Put(parentRaw,18,std::bit_cast<std::uint32_t>(21.9f));Put(parentRaw,6,contained.value,2);
        childRaw[10]=6;childRaw[11]=8;Put(childRaw,4,0,2);Put(childRaw,12,0x8101,2);Put(childRaw,18,kPriestType,2);Put(childRaw,20,0x20,2);
        SquidPostPopState book;SquidOwnerMode mode;mode.battle=true;SquidOwner owner(pool,types,book,mode);
        PriestOwnerState ownerState;ownerState.battlePlayer=1;int notifications=0,pops=0,carrierChecks=0;Sid born{};
        RawPriestOwner priestOwner(pool,mode,ownerState,{[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); },[&](Sid) { ++notifications; }});
        std::vector<std::uint8_t> spots(65536);PriestSpawnState spawnState;
        RawPriestSpawn spawn(pool,types,spots,spawnState,{
            [](const PriestPlacementQuery&)->bool { CHECK(false);return false; },
            [&](std::uint32_t type,std::uint32_t flags) { CHECK(type==kPriestType && flags==0);born=factory.Create(type,flags);return born; },
            MakePriestOwnerDispatch(priestOwner,[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); }),
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && x==21 && y==22 && flags==0 && (pool.Slot(sid)[11]&4));++pops; },
            [](Sid) { CHECK(false); },[&](Sid sid) { CHECK(sid==born);++carrierChecks; }});
        ContainedFinderState finderState;DamageableReleaseState releaseState;releaseState.battle=true;
        RawDamageableRelease release(pool,types,spots,finderState,releaseState,MakePriestSpawnHooks(pool,spawn,{
            {},[](std::uint32_t,std::uint32_t)->Sid { CHECK(false);return {}; },[](Sid,std::uint32_t) { CHECK(false); },
            [](Sid,float,float,std::uint32_t) { CHECK(false); },[](Sid) { CHECK(false); }}));
        release.Release(parent);
        CHECK(born.value==(patch ? 15000 : 6000) && pool.FreeCount()==freeBefore-3 && priestOwner.Handles(born));
        CHECK(Get(pool.Slot(born),12,2)==0x8181 && pool.Slot(born)[patch ? 34 : 32]==1 && Get(childRaw,12,2)==0x8101);
        CHECK(pops==1 && carrierChecks==1 && notifications==1);
    }
}
