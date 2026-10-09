// 독립 원본 투표 관찰과 실제 지역 getter/outpost 수명 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawRegionOwnership.h"
#include "o/RawCanonPlacementTerrain.h"
#include "o/RawDamageablePreDestroy.h"
#include "o/SquidFactory.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 각 PE의 독립 관찰 수와 wrapper 초기 DWORD 값이다.
constexpr std::size_t kRows=440;
constexpr std::uint32_t kRevision=0xffffffff,kDirty=0x12345678;
// 기대값을 포함하지 않는 목록·raw 좌표/소유자·타입 테마 입력이다.
const std::array<std::pair<std::vector<std::uint32_t>,std::vector<std::uint32_t>>,12> kScenes{{
    {{},{}},{{50},{}},{{50,53},{60}},{{50},{60,63}},{{50,51,53},{60,61}},
    {{50,51,52,53,54,55},{60,61,62,63,64,65}},{{50,50,50},{61,64}},{{51,54},{60,63,64}},
    {{},{60,60,63,61}},{{52,55},{62,65}},{{50,51},{60,61,63,64,65}},{{55,50,53,50},{65,60,60,63}}}};
constexpr std::array<std::pair<float,float>,6> kCoords{{{10.2F,20.3F},{30.000001F,21.00002F},{40.5F,22.5F},
    {10.00002F,20.000001F},{30.5F,21.3F},{40.000001F,22.000001F}}};
constexpr std::array<std::array<std::uint8_t,6>,3> kOwners{{{1,2,0,1,2,8},{3,3,3,3,3,3},{0,0,8,2,2,8}}};
constexpr std::array<std::array<std::uint8_t,6>,3> kWorkshopOwners{{{1,2,8,1,2,8},{3,4,3,3,3,3},{2,0,8,2,0,8}}};
constexpr std::array<std::uint32_t,6> kThemes{0,1,2,3,0xffffffff,0};
// fixture의 독립 raw 입력을 쓰며 할당/일반 Pop이 완료됐다는 뜻은 아니다.
std::span<std::uint8_t> InputRaw(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// 누적 입력 기록을 만들며 중복 지역을 지우지 않는다.
std::vector<OwnershipAffectedRegion> Affected(std::uint32_t variant) {
    const std::vector<std::uint32_t> regions=variant==0 ? std::vector<std::uint32_t>{7,9} :
        variant==1 ? std::vector<std::uint32_t>{7,7,9} : std::vector<std::uint32_t>{9,9,7};
    std::vector<OwnershipAffectedRegion> result;
    // 원본이 투표에 사용하지 않는 좌표도 같은 float 기록으로 공급한다.
    for (std::size_t i=0;i<regions.size();++i) result.push_back({90.5F+static_cast<float>(i),91.25F+static_cast<float>(i),regions[i]});return result;
}
// 기대 출력은 저장된 기계어 관찰에서 한 번만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_REGIONOWNERSHIP_FIXTURE).rows;return rows;
}
// 모든 getter·옵션·도장 사건과 원본 wrapper 상태를 같은 실제 지역 지도에 재생한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());std::vector<std::uint32_t> themes(types.size());
    std::vector<std::uint16_t> islands(65536);CanonPlacementTerrainState terrainState;
    // 합성 고정 지도는 두 섬·다리·빈 칸의 실제 SID를 가리킨다.
    for (std::size_t y=0;y<256;++y) {
        // 원본 실행기와 동일한 x 구간을 공급하며 결과는 계산하지 않는다.
        for (std::size_t x=0;x<256;++x) islands[y*256+x]=static_cast<std::uint16_t>(x<20 ? 100 : x<40 ? 101 : x<60 ? 102 : 0);
    }
    RawCanonPlacementTerrain terrain(pool,types,frames,islands,terrainState,{[](const CanonPlacementQuery&,Sid) { return false; },
        [](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },[](const CanonPlacementQuery&,CanonPlacementTerrainState&) { return false; }});
    CHECK(Fixture().size()==3*kRows);std::size_t count=0;
    // 입력별 새 목록/전역을 준비하며 이전 행의 테마나 누적 지역을 재사용하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==13);const auto scene=Number(row[1]),variant=Number(row[2]);
        // 실제 raw 목록 항목을 공급하며 원본 owner/type getter의 오프셋을 유지한다.
        for (std::size_t i=0;i<kCoords.size();++i) {
            themes[83+i]=kThemes[(i+variant)%kThemes.size()];
            // outpost 추가 목록과 작업장 목록의 소유자 입력은 독립적이다.
            for (const bool second:{false,true}) {
                auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>((second ? 60 : 50)+i)});std::fill(raw.begin(),raw.end(),std::uint8_t{});
                raw[10]=static_cast<std::uint8_t>(83+i);raw[patch ? 34 : 32]=(second ? kWorkshopOwners : kOwners)[variant][i];
                Put(raw,14,std::bit_cast<std::uint32_t>(kCoords[i].first));Put(raw,18,std::bit_cast<std::uint32_t>(kCoords[i].second));
            }
        }
        // 지역 getter는 raw +8과 현재 genus를 읽으며 bridge는 sentinel을 반환한다.
        for (const auto sid:{100,101,102}) {
            const auto type=static_cast<std::uint8_t>(sid-17);auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});
            std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;Put(raw,8,sid==100 ? 7U : sid==101 ? 9U : 42U,2);
            types[type].flags2=sid==100 ? 2U : sid==101 ? 0x1000002U : 4U;
        }
        terrainState.emptyRegion=Number(row[8]);SquidPostPopList additional{kScenes[scene].first,0},workshops{kScenes[scene].second,0};
        additional.count=static_cast<std::uint32_t>(additional.entries.size());workshops.count=static_cast<std::uint32_t>(workshops.entries.size());
        additional.entries.resize(8,0xabababab);workshops.entries.resize(8,0xabababab);const auto beforeAdditional=additional.entries,beforeWorkshops=workshops.entries;
        RegionOwnershipState state{Affected(variant),kRevision,kDirty};std::string events;std::uint32_t paints=0;
        // 호출 순서를 저장하며 외부 도장 후의 누적 기록만 입력으로 제공한다.
        const auto append=[&](const std::string& event) { if (!events.empty()) events+=';';events+=event; };
        auto hooks=MakeTerrainRegionOwnershipHooks(pool,terrain,{{},[&](const RegionOwnershipPaint& paint) {
            append("P:"+std::to_string(paint.x)+":"+std::to_string(paint.y)+":"+std::to_string(paint.owner)+":"+std::to_string(paint.theme));
            ++paints;if (patch && Number(row[5]) && paints==2) state.affected=Affected(variant);
        },[&] { append("C:"+row[4]);return Number(row[4]); }});
        const auto actualRegion=hooks.regionAt;
        // 실제 terrain 어댑터 진입 전에 동일한 raw float 인자 비트를 관찰한다.
        hooks.regionAt=[&](float x,float y) {
            append("R:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y)));return actualRegion(x,y);
        };
        RawRegionOwnership ownership(pool,additional,workshops,themes,state,std::move(hooks));
        ownership.Update(std::bit_cast<float>(Number(row[6])),std::bit_cast<float>(Number(row[7])),Number(row[3]));
        std::string regions;
        // 패치만 누적 전역을 가지며 기대 기록은 독립 fixture에서 읽는다.
        if (patch) for (const auto& value:state.affected) { if (!regions.empty()) regions+=',';regions+=std::to_string(value.region); }
        if (regions.empty()) regions="-";
        const bool same=events==row[9] && regions==row[10] && state.revision==Number(row[11]) && state.dirty==Number(row[12]);
        CHECK(same);CHECK(additional.entries==beforeAdditional && workshops.entries==beforeWorkshops);
        if (!same) { std::printf("지역 소유 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
}

// 패치의 두 도장·테마·재귀·DWORD 감김을 전체 원본 관찰에 대조한다.
TEST_CASE(RegionOwnership_ReplaysOriginals) { Replay("originals"); }
// CD의 단일 도장과 테마/누적 지역이 없는 동작을 별도로 대조한다.
TEST_CASE(RegionOwnership_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 원본 관찰도 같은 구현으로 대조한다.
TEST_CASE(RegionOwnership_ReplaysExtra1037) { Replay("original1037"); }

// 실제 생성자·등록·삭제 준비·Damageable을 거쳐 즉시 지역 소유 투표가 바뀌는지 검사한다.
TEST_CASE(RegionOwnership_OutpostLifecycleImmediatelyChangesVotes) {
    // 두 판본의 outpost/소유자 필드와 실제 지역 지도 getter를 함께 연결한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        std::vector<PriestPlainCanonType> frames(types.size());std::vector<std::uint32_t> themes(types.size());themes[84]=1;types[83].flags2=2;
        std::vector<std::uint16_t> islands(65536,100);CanonPlacementTerrainState terrainState;
        // outpost(50)와 작업장(60)은 같은 지역에 있지만 서로 다른 소유자다.
        for (const auto sid:{50,60,100}) {
            auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[10]=sid==50 ? 126 : sid==60 ? 84 : 83;raw[patch ? 34 : 32]=sid==50 ? 1 : 2;
            Put(raw,8,7,2);Put(raw,14,std::bit_cast<std::uint32_t>(10.2F));Put(raw,18,std::bit_cast<std::uint32_t>(20.3F));
        }
        types[126].constructorAddress=TypeConstructorAddress(edition,126);SquidFactory factory(pool,types);factory.Construct(126,kSource);
        RawCanonPlacementTerrain terrain(pool,types,frames,islands,terrainState,{[](const CanonPlacementQuery&,Sid) { return false; },
            [](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },[](const CanonPlacementQuery&,CanonPlacementTerrainState&) { return false; }});
        SquidPostPopList additional{std::vector<std::uint32_t>(4),0},nearest{std::vector<std::uint32_t>(4),0},workshops{{60},1};
        RegionOwnershipState state;std::vector<RegionOwnershipPaint> paints;
        RawRegionOwnership ownership(pool,additional,workshops,themes,state,MakeTerrainRegionOwnershipHooks(pool,terrain,{
            {},[&](const RegionOwnershipPaint& paint) { paints.push_back(paint); },[] { return 1U; }}));
        std::vector<std::uint8_t> spots(65536);ContainedFinderState contained;DamageablePreDestroyState damageableState;std::uint32_t posts=0,pres=0;
        RawDamageablePreDestroy damageable(pool,spots,contained,damageableState,{[](Sid) {},[](Sid) {},[](const DamageableSoundEvent&) {},
            [](Sid) {},[](Sid,std::uint32_t) {},[&](Sid,std::uint32_t) { ++pres; }});
        RawOutpostLifecycle lifecycle(pool,additional,nearest,MakeOutpostRegionOwnershipHooks(pool,ownership,
            MakeOutpostDamageablePreDestroyHooks(pool,damageable,{{},[](Sid,std::uint32_t) {},[&](Sid,std::uint32_t) { ++posts; },{},{}})));
        ownership.Update(10.2F,20.3F);CHECK(paints.back().owner==2 && paints.back().theme==(patch ? 3U : 0U));
        lifecycle.PostPop(kSource,1);CHECK(additional.count==1 && nearest.count==1 && paints.back().owner==0 && paints.back().theme==0);
        lifecycle.PreDestroy(kSource,0x200000);CHECK(additional.count==0 && nearest.count==0 && paints.back().owner==2 && paints.back().theme==(patch ? 3U : 0U));
        CHECK(posts==1 && pres==1 && paints.size()==(patch ? 6U : 3U));CHECK(state.revision==(patch ? 3U : 0U) && state.dirty==(patch ? 1U : 0U));
    }
}

// 부정확한 연결/좌표를 묵시적인 기본 판정으로 바꾸지 않도록 검사한다.
TEST_CASE(RegionOwnership_RejectsMissingBoundariesAndForeignPools) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false),otherPool(OriginalEdition::Patch1078,kCapacity,false);
    SquidPostPopList additional{{50},1},workshops;std::vector<std::uint32_t> themes(188);RegionOwnershipState state{{{1,2,7}},5,9};std::uint32_t paints=0;
    RegionOwnershipHooks hooks{[](float,float) { return 7U; },[&](const RegionOwnershipPaint&) { ++paints; },[] { return 1U; }};
    auto missing=hooks;missing.paint={};CHECK(Throws([&] { RawRegionOwnership bad(pool,additional,workshops,themes,state,missing); }));
    missing=hooks;missing.themeMode={};CHECK(Throws([&] { RawRegionOwnership bad(pool,additional,workshops,themes,state,missing); }));
    CHECK(Throws([&] { RawRegionOwnership bad(pool,additional,additional,themes,state,hooks); }));
    RawRegionOwnership ownership(pool,additional,workshops,themes,state,hooks);
    CHECK(Throws([&] { ownership.Update(std::numeric_limits<float>::quiet_NaN(),1); }));CHECK(paints==0 && state.affected.size()==1 && state.revision==5);
    CHECK(Throws([&] { static_cast<void>(MakeOutpostRegionOwnershipHooks(otherPool,ownership,{})); }));
    std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(types.size());std::vector<std::uint16_t> islands(65536);CanonPlacementTerrainState terrainState;
    RawCanonPlacementTerrain terrain(pool,types,frames,islands,terrainState,{[](const CanonPlacementQuery&,Sid) { return false; },
        [](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },[](const CanonPlacementQuery&,CanonPlacementTerrainState&) { return false; }});
    CHECK(Throws([&] { static_cast<void>(MakeTerrainRegionOwnershipHooks(otherPool,terrain,hooks)); }));
    auto raw=InputRaw(pool,kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[34]=9;
    CHECK(Throws([&] { ownership.Update(1,2); }) && paints==0);additional.count=2;CHECK(Throws([&] { ownership.Update(1,2); }) && paints==0);
}
