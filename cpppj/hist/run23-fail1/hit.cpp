// 보호 캠페인의 실제 Kernel에서 충돌 생성/현재HP/감시 및 공통 자산 삭제를 확인한다.
#include "CampaignInstallSupport.h"

namespace {
// 현재 저장된 내 사제를 선택하며 가짜 피해 표적을 생성하지 않는다.
Sid MyCampaignPriest(const InstallScene& scene) {
    // 실제 PostPop 목록의 현재 소유자를 조회한다.
    for (const auto id:scene.priestState.priests.priests.Items()) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};if (scene.core.pool.Slot(sid)[34]==1) return sid;
    }
    throw std::logic_error("보호 캠페인의 내 사제 없음");
}
// 같은 현재 게임 시각을 모든 공격/발사/프로세스에 먼저 공급한다.
void CampaignHitFrame(InstallScene& scene,std::uint64_t frame) {
    const auto seconds=static_cast<double>(frame)/60.0;const FrameTime time{seconds,seconds,frame ? 1.0/60.0 : 0.0,frame+1};
    scene.assetBindings->PrepareFrame(seconds);scene.combat->PrepareFrame(time);scene.world.RunFrame(time,false);
}
}

// 명시 재배치 뒤 최초실제 충돌은 같은 HP를 감소시키고 실제 anim160/표시자산 삭제를 수행한다.
TEST_CASE(CampaignHit_ActualWarBeginsImpactAndHealth) {
    InstallScene scene;scene.runtime->Load();const auto target=MyCampaignPriest(scene);
    const auto before=scene.priests->PriestState().CurrentHitPoints(target);
    scene.world.Unpop(target,0);scene.world.Pop(target,130,114,0);bool impact=false;std::uint64_t frame=0;
    // 첫 실제 HP감소까지 모든 현재 Kernel을 실행한다. 피해값을 검사에서 직접 쓰지 않는다.
    for (;frame<1200 && scene.priests->PriestState().CurrentHitPoints(target)==before;++frame) {
        CampaignHitFrame(scene,frame);
        // 현재 살아 있는 실제 anim을 관찰한다. fake 효과 관찰 목록을 만들지 않는다.
        for (const auto& object:scene.world.Objects()) if (object.object.type==160) impact=true;
    }
    const auto after=scene.priests->PriestState().CurrentHitPoints(target);
    CHECK(after<before && after>=1 && impact);
    CHECK(scene.core.bookkeeping.depth==0 && scene.core.destroy.PreDepth()==0 && scene.core.destroy.PostDepth()==0);
    std::printf("실제 충돌/피격: HP%d→%d frame%llu damageEffect%u\n",before,after,static_cast<unsigned long long>(frame),scene.core.types[158].damageEffect);
}

// 같은 전체 Kernel의 반복 실제 충돌로 HP1에 도달하고 감시 표적/투사체 수명 깊이가 정리된다.
TEST_CASE(CampaignHit_ActualWarBeginsRepeatedDamageToOne) {
    InstallScene scene;scene.runtime->Load();const auto target=MyCampaignPriest(scene);
    scene.world.Unpop(target,0);scene.world.Pop(target,130,114,0);std::uint64_t frame=0;
    // 실제 HP1까지 최대60초를 실행하며 현재 회복/SP/모든 공격 부모도 생략하지 않는다.
    for (;frame<3600 && scene.priests->PriestState().CurrentHitPoints(target)>1;++frame) CampaignHitFrame(scene,frame);
    CHECK(scene.priests->PriestState().CurrentHitPoints(target)==1);
    CHECK(scene.core.bookkeeping.depth==0 && scene.core.destroy.PreDepth()==0 && scene.core.destroy.PostDepth()==0);
    CHECK(scene.priestState.regen.patch.sequence==scene.priestState.regen.patch.observedSequence);
    std::printf("실제 반복피격 HP1: frame%llu, 현재process%zu\n",static_cast<unsigned long long>(frame),scene.core.kernel.Size());
}
