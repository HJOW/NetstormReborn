// 원본 outpost 목록 접두의 전체 관찰과 실제 Player 기준점 조회 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawOutpostLifecycle.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/RawDamageablePreDestroy.h"
#include "o/SquidFactory.h"

namespace {
using namespace netstorm::test::rawscene;
// 원본 행 수/목록 용량과 outpost의 실제 타입 번호다.
constexpr std::size_t kRows=768,kListSize=8;
constexpr std::uint32_t kOutpost=126,kFill=0xabababab;
// 독립 fixture의 슬롯 입력을 공급한다. 원본 수신/할당 경로의 완료를 뜻하지 않는다.
std::span<std::uint8_t> InputRaw(SidPool& pool,Sid sid) {
    const auto slot=pool.Slot(sid);return {const_cast<std::uint8_t*>(slot.data()),slot.size()};
}
// 원본 행의 DWORD 목록을 원래 순서와 비활성 꼬리까지 읽는다.
std::vector<std::uint32_t> Numbers(const std::string& value) {
    std::vector<std::uint32_t> result;
    // 모든 필드를 숫자로 읽되 추가/삭제 결과를 계산하지 않는다.
    for (const auto& part:Split(value,',')) result.push_back(Number(part));return result;
}
// 원본 입력과 같은 초기 목록을 만들며 기대 출력은 독립 fixture에서 읽는다.
SquidPostPopList Input(std::uint32_t scene,bool second) {
    const std::array<std::vector<std::uint32_t>,8> lists{{{}, {60}, {50}, {60,50,61,50},
        {60,61,62,63,64,65,66,67},{50,60,50,61,50,62,50,63},{50,50,50,50,50,50,50},{61,60,50}}};
    SquidPostPopList result;result.entries=lists[second ? (scene+3)%lists.size() : scene];result.count=static_cast<std::uint32_t>(result.entries.size());
    result.entries.resize(kListSize,kFill);return result;
}
// 관찰 목록은 한 번만 읽고 원본 파일/실행기 없이 재생한다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_OUTPOSTLIFECYCLE_FIXTURE).rows;return rows;
}
// 파생 반환의 두 목록 전체·통지 인자/순서·원본 raw 쓰기를 판본별로 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    CHECK(Fixture().size()==3*kRows);std::size_t count=0;
    // 같은 입력 순서로 wrapper를 호출하며 기대 분기는 구현하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==14);auto raw=InputRaw(pool,kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});
        raw[10]=kOutpost;raw[11]=2;raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[2]));Put(raw,14,Number(row[6]));Put(raw,18,Number(row[7]));
        auto additional=Input(Number(row[4]),false),nearest=Input(Number(row[4]),true);std::string events;
        const auto base=[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==kSource && flags==Number(row[3]));if (!events.empty()) events+=';';
            events+="B:"+std::to_string(flags)+":"+std::to_string(additional.count)+":"+std::to_string(nearest.count);
        };
        RawOutpostLifecycle lifecycle(pool,additional,nearest,{
            [&](float x,float y) {
                events="R:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(additional.count)+":"+std::to_string(nearest.count);
                if (Number(row[5])) { raw[patch ? 40 : 35]=9;Put(raw,14,0x42c80000); }
            },[](Sid,std::uint32_t) {},base,[](Sid,std::uint32_t) {},base});
        if (row[1]=="P") lifecycle.PostPop(kSource,Number(row[3]));else lifecycle.PreDestroy(kSource,Number(row[3]));
        CHECK(additional.count==Number(row[8]) && additional.entries==Numbers(row[9]));
        CHECK(nearest.count==Number(row[10]) && nearest.entries==Numbers(row[11]));CHECK(events==row[12]);CHECK(Hex(raw)==row[13]);++count;
    }
    CHECK(count==kRows);
}
}

TEST_CASE(OutpostLifecycle_ReplaysOriginals) { Replay("originals"); }
TEST_CASE(OutpostLifecycle_ReplaysCd) { Replay("originalCD"); }
TEST_CASE(OutpostLifecycle_ReplaysExtra1037) { Replay("original1037"); }

TEST_CASE(OutpostLifecycle_RegistrationAndRemovalImmediatelyChangePlayerAnchor) {
    // 두 판본에서 같은 실제 종속/그래프/추가 목록을 순차 수명 경로에 연결한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[83].flags1=types[84].flags1=0x800;types[83].flags2=2;types[84].flags2=6;
        std::vector<std::uint16_t> surfaces(65536);surfaces[21*256+20]=60;SquidPostPopState books;books.ownerFactories[1].entries={51};books.ownerFactories[1].count=1;
        SquidPostPopList additional{std::vector<std::uint32_t>(4),0},nearest{std::vector<std::uint32_t>(4),0};ContainedFinderState contained;PlayerPlacementAnchorState state;state.graphReady=1;
        // 작업장의 종속은 linked를 켜지만 작업장 자체는 다른 그래프이므로 추가 목록만 기준점이 된다.
        for (const auto sid:{50,51,60,61}) {
            auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[10]=sid==50 ? kOutpost : sid==51 ? 84 : sid==60 ? 83 : 6;raw[patch ? 34 : 32]=1;raw[patch ? 30 : 28]=sid==51 ? 8 : 7;
            Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(21.0F));
        }
        // 실제 outpost 생성자 쓰기를 적용하되 전체 Pop/작업장 프로세스는 공급하지 않는다.
        types[kOutpost].constructorAddress=TypeConstructorAddress(edition,kOutpost);SquidFactory factory(pool,types);factory.Construct(kOutpost,kSource);
        Put(InputRaw(pool,Sid{51}),6,61,2);Put(InputRaw(pool,Sid{61}),18,80,2);Put(InputRaw(pool,Sid{61}),20,0x10,2);
        RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,additional,contained,state);std::vector<std::uint32_t> regionAnchors;std::uint32_t basePosts=0,basePres=0;
        std::vector<std::uint8_t> spots(65536);DamageablePreDestroyState damageableState;std::uint32_t collapses=0,sounds=0;
        RawDamageablePreDestroy damageable(pool,spots,contained,damageableState,{
            [&](Sid sid) { CHECK(sid==kSource);++collapses; },[](Sid) {},[&](const DamageableSoundEvent& event) { CHECK(event.sound==DamageableSound::Collapse);++sounds; },
            [](Sid) {},[](Sid,std::uint32_t) {},[&](Sid sid,std::uint32_t flags) { CHECK(sid==kSource && flags==0x200000);++basePres; }});
        RawOutpostLifecycle lifecycle(pool,additional,nearest,MakeOutpostDamageablePreDestroyHooks(pool,damageable,{
            [&](float x,float y) { regionAnchors.push_back(anchor.Locate(1,80,x,y)); },
            [](Sid,std::uint32_t) {},[&](Sid,std::uint32_t) { ++basePosts; },
            {},{}}));
        CHECK(anchor.Locate(1,80,20,21)==0);lifecycle.PostPop(kSource,0);CHECK(anchor.Locate(1,80,20,21)==0);
        lifecycle.PostPop(kSource,1);CHECK(anchor.Locate(1,80,20,21)==50);lifecycle.PostPop(kSource,1);
        CHECK(additional.count==1 && nearest.count==1 && regionAnchors==std::vector<std::uint32_t>({50,50}));
        lifecycle.PreDestroy(kSource,0x200000);CHECK(anchor.Locate(1,80,20,21)==0 && additional.count==0 && nearest.count==0);
        CHECK(regionAnchors==std::vector<std::uint32_t>({50,50,0}) && basePosts==3 && basePres==1 && collapses==1 && sounds==1);
        lifecycle.PostPop(kSource,1);CHECK(anchor.Locate(1,80,20,21)==50 && additional.count==1 && nearest.count==1);
    }
}

TEST_CASE(OutpostLifecycle_RejectsBrokenConnectionsBeforeChangingLists) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);auto raw=InputRaw(pool,kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=kOutpost;
    SquidPostPopList additional{{60,kFill},1},nearest{{61,kFill},3};std::uint32_t regions=0,bases=0;
    OutpostLifecycleHooks hooks{[&](float,float) { ++regions; },[](Sid,std::uint32_t) {},[&](Sid,std::uint32_t) { ++bases; },
        [](Sid,std::uint32_t) {},[&](Sid,std::uint32_t) { ++bases; }};
    CHECK(Throws([&] { RawOutpostLifecycle bad(pool,additional,additional,hooks); }));
    auto missing=hooks;missing.regionChanged={};CHECK(Throws([&] { RawOutpostLifecycle bad(pool,additional,nearest,missing); }));
    RawOutpostLifecycle lifecycle(pool,additional,nearest,hooks);const auto before=additional.entries;
    CHECK(Throws([&] { lifecycle.PostPop(kSource,1); }) && additional.entries==before && additional.count==1 && regions==0 && bases==0);
    CHECK(Throws([&] { lifecycle.PreDestroy(kSource,1); }) && additional.entries==before && additional.count==1 && regions==0 && bases==0);
    // 원본 분기에서 읽지 않는 손상 목록은 abstract 객체/두 번째 Pop의 base 호출을 막지 않는다.
    lifecycle.PostPop(kSource,0);raw[40]=9;lifecycle.PostPop(kSource,1);lifecycle.PreDestroy(kSource,1);CHECK(regions==0 && bases==3);
    raw[11]=1;CHECK(Throws([&] { lifecycle.PreDestroy(kSource,1); }));raw[11]=0;raw[40]=0;
    auto rejecting=hooks;rejecting.validatePostPop=[](Sid,std::uint32_t) { throw std::logic_error("원래 부모 경계의 사전 거부"); };
    nearest.count=1;RawOutpostLifecycle guarded(pool,additional,nearest,rejecting);
    CHECK(Throws([&] { guarded.PostPop(kSource,1); }) && additional.entries==before && additional.count==1 && regions==0);
    SidPool otherPool(OriginalEdition::Patch1078,kCapacity,false);std::vector<std::uint8_t> spots(65536);ContainedFinderState contained;DamageablePreDestroyState state;
    RawDamageablePreDestroy other(otherPool,spots,contained,state,{[](Sid) {},[](Sid) {},[](const DamageableSoundEvent&) {},[](Sid) {},[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {}});
    CHECK(Throws([&] { static_cast<void>(MakeOutpostDamageablePreDestroyHooks(pool,other,hooks)); }));
}
