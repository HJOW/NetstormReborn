// 기존 독립 x86 HP 관측을 보상 어댑터에 연결한다. 게임·의식·공격·MP는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/SquidReward.h"
#include "o/RawGameAuxState.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // HP 숫자 조회만 수행하는 실제 보상 구성이다.
struct RewardHpScene { // 풀/장부/SP 장치는 보상과 같은 수명이며 실제 게임 세션을 시작하지 않는다.
    SidPool pool; // 판본별 실제 raw 슬롯이다.
    GameRandom random; // HP 조회에서 소비되지 않아야 하는 난수다.
    ScrambledSpStore sp; // PATCH 보상 구성의 실제 SP 저장소다.
    SquidPostPopState book; // HP 조회가 쓰지 않아야 하는 등록 장부다.
    SquidRewardState mode; // 현재 checking/약화/특별 TYPE 입력을 빌린다.
    std::unique_ptr<SquidReward> reward; // 같은 실제 공유 getter를 호출하는 보상 처리기다.
    RewardHpScene(OriginalEdition edition,std::span<const RiftTypeRecord> types):pool(edition,32768),sp(random,[] { return 1U; }) { reward=std::make_unique<SquidReward>(pool,types,book,mode,edition==OriginalEdition::Patch1078 ? &sp : nullptr); } // 선택 표가 준비된 뒤 생성 시 사본을 구성한다.
};
}

TEST_CASE(RewardHitPointDiagnostics_Native1158AdapterAndCurrentGlobals) { // 초기 음수 번호2행은 번호형 API의 선택 범위 밖이며 기존 getter1160검사가 별도로 보존한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_REWARD_HP_DIAGNOSTIC_FIXTURE).rows) { // 원문 입력과 반환/진단만 읽으며 기대 HP를 계산하지 않는다.
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==9);if (std::stoi(input[7])!=0) continue;
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const auto count=edition==OriginalEdition::Patch1078 ? 188U : 171U;
        std::vector<RiftTypeRecord> physical(190);const auto number=Number(input[0]);auto& selected=physical[1+number];selected.maxHitPoints=std::stoi(input[1]);selected.flags1=Number(input[2]);
        const auto origin=std::span<const RiftTypeRecord>(physical).subspan(1,count);RewardHpScene scene(edition,origin);
        std::uint32_t checking=Number(input[3]),priest=Number(input[5]),dais=Number(input[6]);bool quarter=Number(input[4])!=0;const auto profile=Number(input[8]);std::string diagnostics;
        scene.mode.debugAsserts.Bind(checking);scene.mode.sharedPriestQuarterHp=&quarter;scene.mode.sharedPriestType=&priest;scene.mode.sharedDaisType=&dais;
        if (edition==OriginalEdition::Patch1078) scene.reward->BindCurrentTypes(origin);
        scene.reward->BindHitPointDiagnostics([&](auto expression,auto file,auto line) { // 외부 UI 대신 기존 원문 관측기의 명시 복귀 입력만 공급한다.
            if (!diagnostics.empty()) diagnostics+=';';diagnostics+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line);
            if (profile) { checking=0;quarter=true;selected.maxHitPoints=std::array<std::int32_t,3>{-13,-37,41}[profile-1];const auto shift=std::array<int,3>{0,1,-1}[profile-1];priest=static_cast<std::uint32_t>(74-shift);scene.reward->BindCurrentTypes(std::span<const RiftTypeRecord>(physical).subspan(static_cast<std::size_t>(1+shift),count)); }
        });
        const auto seed=scene.random.State(),free=scene.pool.FreeCount();const auto actual=scene.reward->TypeHitPoints(number);
        CHECK(actual==std::stoi(row[2]) && (diagnostics.empty() ? "-" : diagnostics)==row[3]);CHECK(scene.random.State()==seed && scene.pool.FreeCount()==free && scene.reward->VirtualCountCalls()==0);++rows;
    }
    CHECK(rows==1158);
}

TEST_CASE(RewardHitPointDiagnostics_MaximumKeepsSelectedAltarAfterDiagnosis) { // 다이스 최대 HP도 선택한 제단 TYPE this를 유지하고 현재 owner의 약화를 읽는다.
    std::vector<RiftTypeRecord> physical(189);physical[74].flags2=0x400000;physical[75].maxHitPoints=29;physical[76].maxHitPoints=999;RewardHpScene scene(OriginalEdition::Patch1078,std::span<const RiftTypeRecord>(physical).first(188));scene.reward->BindCurrentTypes(std::span<const RiftTypeRecord>(physical).first(188));
    RawGameAuxState owner;owner.AltarType()=75;owner.DaisType()=77;owner.PriestType()=158;scene.mode.sharedAltarType=&owner.AltarType();scene.mode.sharedDaisType=&owner.DaisType();scene.mode.sharedPriestType=&owner.PriestType();scene.mode.sharedPriestQuarterHp=&owner.PriestQuarterHp();std::uint32_t checking=0x80000000,calls{};scene.mode.debugAsserts.Bind(checking);
    const auto sid=scene.pool.Allocate();auto raw=scene.pool.AllocatedBytes(sid);raw[10]=74;raw[40]=1;Put(raw,26,123);
    scene.reward->BindHitPointDiagnostics([&](auto expression,auto file,auto line) { CHECK(expression=="(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType" && file=="\\Ns\\O\\Rifttype.cpp" && line==0x1e3);++calls;owner.AltarType()=76;owner.PriestType()=74;owner.PriestQuarterHp()=true;physical[75].maxHitPoints=-37;raw[10]=76;scene.reward->BindCurrentTypes(std::span<const RiftTypeRecord>(physical).subspan(1,188)); });
    CHECK(scene.reward->MaxHitPoints(sid)==-9 && calls==1 && Get(raw,26)==123 && raw[10]==76); // 원문 진단이 바꾼 raw TYPE나 새 제단의999로 다시 고르지 않는다.
}

TEST_CASE(RewardHitPointDiagnostics_MissingAndReplacedCallbackStayAtSelectedBoundary) { // 진단 장치의 교체/누락과 checking/dais에 따른 미선택을 구별한다.
    std::vector<RiftTypeRecord> types(188);types[74].maxHitPoints=-7;RewardHpScene scene(OriginalEdition::Patch1078,types);std::uint32_t checking=1;scene.mode.debugAsserts.Bind(checking);scene.mode.types.dais=75;
    CHECK(Throws([&] { scene.reward->TypeHitPoints(74); }));auto pin=std::make_shared<int>(17);std::weak_ptr<int> weak=pin;
    scene.reward->BindHitPointDiagnostics([&,keep=pin](auto,auto,auto) { scene.reward->BindHitPointDiagnostics({});CHECK(*keep==17);checking=0;scene.mode.priestQuarterHp=true;scene.mode.types.priest=74; });pin.reset();
    CHECK(scene.reward->TypeHitPoints(74)==-1 && weak.expired());CHECK(scene.reward->TypeHitPoints(74)==-1);checking=1;scene.mode.types.dais=74;CHECK(scene.reward->TypeHitPoints(74)==-1);
    scene.mode.types.dais=75;CHECK(Throws([&] { scene.reward->TypeHitPoints(74); }));
}
