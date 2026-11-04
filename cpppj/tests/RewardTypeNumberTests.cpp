// 단위869: 보상/일반 삭제의 현재 TYPE 번호와 가상Count 전후의 판본별 저장 순서를 게임 없이 검사한다.
#include "RawSceneSupport.h"
#include "o/RawGameAuxState.h"
#include "o/RawDamageablePostDestroy.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 합성 객체/장기 번호 owner와 명시 외부 경계만 이 파일에서 소유한다.
struct RewardNumberScene { // 비용/HP 조회만 실행하며 실제 지급/공격/포획/희생은 요청하지 않는다.
    SidPool pool; // 대상50과 검색 후보60까지 할당한 실제 풀이다.
    GameRandom random{12345}; // SP 저장 장치가 참조하지만 Plan/조회에서는 소비하지 않는다.
    ScrambledSpStore store; // PATCH 보상 처리기의 필수 실제 장치다.
    SquidPostPopState book; // 별도 비용 누적/AI 재고를 가진 독립 장부다.
    SquidRewardState mode; // 최초 번호 사본과 실제 번호 포인터를 구별한다.
    RawGameAuxState owner; // 실제 Client와 동일한 최초 다섯 DWORD를 소유한다.
    std::vector<RiftTypeRecord> types; // 처음 로딩 비용과 현재 HP/부류 입력이다.
    std::unique_ptr<SquidReward> reward; // 원본 Plan/비용/HP 본체를 직접 실행한다.
    explicit RewardNumberScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :pool(edition,kCapacity,false),store(random,[] { return 1U; }),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        for (std::uint16_t sid=5;sid<=60;++sid) { CHECK(pool.Allocate(2)==Sid{sid});auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // 합성 대상의 raw 상태를 실제 할당 후에만 준비한다.
        types[70].cost=40;types[71].cost=80;types[72].cost=120;types[73].cost=200;types[147].cost=300;types[150].cost=500;types[154].cost=600;types[164].cost=1000;types[165].cost=1200;
        types[70].maxHitPoints=100;types[71].maxHitPoints=200;types[73].maxHitPoints=700;pool.AllocatedBytes(kSource)[10]=70;
    }
    void Bind() { mode.sharedAltarType=&owner.AltarType();mode.sharedDaisType=&owner.DaisType();mode.sharedSilentType=&owner.RainBlockerType();mode.sharedAliasType=&owner.GrowingRainBlockerType();mode.sharedNuggetType=&owner.NuggetType(); } // 제품 Session과 같은 실제 owner 주소를 빌린다.
    void Make(SquidRewardHooks hooks={}) { reward=std::make_unique<SquidReward>(pool,types,book,mode,pool.Edition()==OriginalEdition::Patch1078 ? &store : nullptr,std::move(hooks));reward->BindCurrentTypes(pool.Edition()==OriginalEdition::Patch1078 ? std::span<const RiftTypeRecord>(types) : std::span<const RiftTypeRecord>{}); } // 비용은 최초 사본, PATCH HP/부류만 현재 표를 쓴다.
};
}
TEST_CASE(RewardTypeNumber_OwnerDefaultsResetAndUnboundCompatibility) { // 초기 PE 값과 일반 월드 준비의 보존·독립 state 사본을 구별한다.
    RewardNumberScene scene;CHECK(scene.owner.AltarType()==164 && scene.owner.DaisType()==165 && scene.owner.RainBlockerType()==147 && scene.owner.GrowingRainBlockerType()==150 && scene.owner.NuggetType()==154);
    scene.owner.AltarType()=70;scene.owner.DaisType()=71;scene.owner.RainBlockerType()=72;scene.owner.GrowingRainBlockerType()=73;scene.owner.NuggetType()=74;scene.owner.ResetForGeneralWorld();
    CHECK(scene.owner.AltarType()==70 && scene.owner.DaisType()==71 && scene.owner.RainBlockerType()==72 && scene.owner.GrowingRainBlockerType()==73 && scene.owner.NuggetType()==74);
    scene.Make();CHECK(scene.reward->Plan(kSource,1,false).cost==40 && scene.reward->Plan(kSource,1,true).refund==10); // 명시 Bind 없는 독립 입력은 외부 owner를 자동 조회하지 않는다.
    scene.Bind();CHECK(scene.reward->SalvageRefund(kSource,70,0)==0 && !scene.reward->Plan(kSource,1,true).applied);
}
TEST_CASE(RewardTypeNumber_CurrentAltarDaisCountAndHp) { // 같은 현재 번호가 개수−1·HP 예외·다이스 최대HP 원천을 선택한다.
    RewardNumberScene scene;scene.Bind();scene.owner.AltarType()=70;scene.owner.DaisType()=71;scene.types[70].flags1=scene.types[73].flags1=0x10;scene.types[72].flags2=0x400000;scene.Make();
    auto raw=scene.pool.AllocatedBytes(kSource);raw[10]=74;CHECK(scene.reward->SalvageRefund(kSource,70,0)==0 && scene.reward->SalvageRefund(kSource,70,1)==10 && scene.reward->SalvageRefund(kSource,71,0)==0 && scene.reward->SalvageRefund(kSource,71,1)==20);
    raw[10]=72;raw[40]=1;CHECK(scene.reward->MaxHitPoints(kSource)==100);scene.owner.AltarType()=73;CHECK(scene.reward->MaxHitPoints(kSource)==700);raw[10]=74;CHECK(scene.reward->SalvageRefund(kSource,70,0)==10);
    scene.mode.debugAsserts=1U;CHECK(scene.reward->TypeHitPoints(71)==200);scene.owner.DaisType()=74;CHECK(Throws([&] { scene.reward->TypeHitPoints(71); })); // 미복원 HP 진단 호스트 경계 자체는 이번 단위에서 바꾸지 않는다.
}
TEST_CASE(RewardTypeNumber_CurrentRainAliasesAndFullNuggetRefund) { // 삭제 제외·성장 비용 별칭·성장 환불0·너겟 전액을 독립해서 바꾼다.
    RewardNumberScene scene;scene.Bind();scene.Make();scene.owner.RainBlockerType()=70;CHECK(!scene.reward->Plan(kSource,1,false).applied && scene.reward->Plan(kSource,1,true).refund==10);
    scene.owner.NuggetType()=70;CHECK(scene.reward->Plan(kSource,1,true).refund==40);scene.owner.GrowingRainBlockerType()=70;CHECK(scene.reward->Plan(kSource,1,true).refund==0);
    scene.owner.RainBlockerType()=71;const auto result=scene.reward->Plan(kSource,1,false);CHECK(result.applied && result.cost==80 && result.refund==20);
    scene.owner.GrowingRainBlockerType()=72;CHECK(scene.reward->Plan(kSource,1,false).cost==40);
}
TEST_CASE(RewardTypeNumber_AliasReadsCurrentRainAfterSecondVirtualCount) { // 양 PE의 Count→현재 silent TYPE 로드 순서를 명시 변경 입력으로 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별 비용 계산은 같은 단가40/80 정의역에서 비교한다.
        RewardNumberScene scene(edition);scene.Bind();std::uint32_t calls=0;SquidRewardHooks hooks;
        hooks.virtualCount=[&](Sid sid) { CHECK(sid==kSource);if (++calls==1) scene.owner.GrowingRainBlockerType()=70;else scene.owner.RainBlockerType()=71;return 0; };scene.Make(hooks);
        const auto result=scene.reward->Plan(kSource,1,false);CHECK(calls==2 && result.cost==80 && result.refund==20 && scene.reward->VirtualCountCalls()==2);
    }
}
TEST_CASE(RewardTypeNumber_PatchSavesRefundTypeBeforeCountAndCdReadsAfter) { // PATCH0044c37f와CD00462784의 서로 다른 TYPE 저장 순서를 보존한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 가상Count 두 번째 호출이 현재 슬롯 TYPE만 바꾼다.
        RewardNumberScene scene(edition);scene.Bind();scene.owner.NuggetType()=71;std::uint32_t calls=0;SquidRewardHooks hooks;
        hooks.virtualCount=[&](Sid) { if (++calls==2) scene.pool.AllocatedBytes(kSource)[10]=71;return 0; };scene.Make(hooks);const auto result=scene.reward->Plan(kSource,1,true);
        CHECK(calls==2 && result.cost==40 && result.refund==(edition==OriginalEdition::Patch1078 ? 10 : 80));CHECK(scene.pool.Slot(kSource)[10]==71);
    }
}
TEST_CASE(RewardTypeNumber_CurrentNumberBindingPreservesLoadedCostSnapshot) { // 번호 변경과 TYPE float 비용 변경은 별도이며 최초 정수 캐시를 다시 만들지 않는다.
    RewardNumberScene scene;scene.Bind();scene.Make();scene.types[71].cost=9001;scene.owner.GrowingRainBlockerType()=70;scene.owner.RainBlockerType()=71;
    CHECK(scene.reward->Plan(kSource,1,false).cost==80 && scene.reward->TypeCostUnits(71)==80 && scene.types[71].cost==9001);
}
TEST_CASE(RewardTypeNumber_GeneralPostSharesCurrentRainExclusionAndOverride) { // 실제 일반 finder가 발견한 rubble 위의 섬 삭제 제외만 검사하며 피해/다리 효과는 선택하지 않는다.
    RewardNumberScene scene;scene.Bind();scene.Make();SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeFrames> frames(scene.types.size(),RiftTypeFrames({}));
    scene.types[70].flags1=0x400;scene.types[70].footX=scene.types[70].footY=1;scene.types[90].flags2=0x1000000;scene.types[90].footX=scene.types[90].footY=1;
    auto raw=scene.pool.AllocatedBytes(kSource);raw[11]=2;raw[34]=3;Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(21.0F));auto rubble=scene.pool.AllocatedBytes(Sid{60});rubble[10]=90;Put(rubble,14,std::bit_cast<std::uint32_t>(20.0F));Put(rubble,18,std::bit_cast<std::uint32_t>(21.0F));hash.Bucket(1,20,21)=60;
    RawSquidNeighbors neighbors(scene.pool,hash,spots,scene.types,frames);DamageablePostDestroyState state;state.sharedSilentSurfaceType=&scene.owner.RainBlockerType();std::uint32_t drops=0,bases=0;DamageablePostDestroyHooks hooks;
    hooks.dropDude=[](Sid,auto,auto) { throw std::logic_error("현재 번호 검사에서 선택하지 않은 Dude 드롭"); };hooks.dropIsland=[&](float x,float y,auto,auto owner) { CHECK(x==20 && y==21 && owner==3);++drops; };hooks.base=[&](Sid sid,auto) { CHECK(sid==kSource);++bases; };
    hooks.regular=[](Sid,auto,auto,auto) { throw std::logic_error("현재 번호 검사에서 선택하지 않은 피해 예약"); };hooks.hasBridgeFall=[](Sid)->bool { throw std::logic_error("현재 번호 검사에서 선택하지 않은 다리 조회"); };hooks.notifySurface=hooks.displayUpdate=[](Sid) { throw std::logic_error("현재 번호 검사에서 선택하지 않은 다리 변경"); };hooks.destroy=[](Sid,auto) { throw std::logic_error("현재 번호 검사에서 선택하지 않은 다리 삭제"); };
    RawDamageablePostDestroy post(scene.pool,hash,scene.types,frames,*scene.reward,neighbors,state,hooks);scene.owner.RainBlockerType()=70;post.PostDestroy(kSource,0);CHECK(drops==0 && bases==1);
    scene.owner.RainBlockerType()=71;post.PostDestroy(kSource,0);CHECK(drops==1 && bases==2);scene.owner.RainBlockerType()=70;post.PostDestroy(kSource,0x200000);CHECK(drops==2 && bases==3);
}
