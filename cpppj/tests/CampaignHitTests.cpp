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
    scene.assetBindings->PrepareFrame(seconds);scene.combat->PrepareFrame(time);scene.soundState.frame=static_cast<std::uint32_t>(time.number);scene.world.RunFrame(time,false);
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
    const auto shield=scene.priests->Forcefield().Find(target);CHECK(shield.value!=0);
    CHECK(scene.core.processes.FindEvent(shield,1)!=nullptr && scene.core.processes.FindEvent(shield,2)!=nullptr);
    const auto lowFrame=frame;
    // HP1 이후에도 모든 현재 Kernel을12초 더 실행하여 발사 완료/보호막/소리/잔여효과 종료를 확인한다.
    for (;frame<lowFrame+720;++frame) CampaignHitFrame(scene,frame);
    CHECK(scene.priests->PriestState().CurrentHitPoints(target)==1 && scene.priests->Forcefield().Find(target)==shield);
    CHECK(!scene.playingSounds.empty());
    // 현재 모든 총의 표적 및 실제 양방향 감시가 HP1 사제를 더 이상 붙잡지 않아야 한다.
    for (std::uint32_t id=5;id<scene.core.pool.Capacity();++id) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};const auto raw=scene.core.pool.Slot(sid);if ((raw[11]&7) || raw[10]!=34) continue;
        auto* gun=dynamic_cast<GunProcess*>(scene.core.processes.FromForm(sid));CHECK(gun!=nullptr);
        CHECK(gun->Fields().Target()!=target && !scene.core.dependencies.Has(target,sid));
    }
    // 최초 교전의 잔여 투사체/충돌 시각 효과가 실제 삭제 경로를 끝냈는지 확인한다.
    for (const auto& object:scene.world.Objects()) CHECK(object.object.type!=83 && object.object.type!=160);
    CHECK(scene.core.bookkeeping.depth==0 && scene.core.destroy.PreDepth()==0 && scene.core.destroy.PostDepth()==0);
    CHECK(scene.priestState.regen.patch.sequence==scene.priestState.regen.patch.observedSequence);
    std::printf("실제 반복피격 HP1: frame%llu, 현재process%zu\n",static_cast<unsigned long long>(frame),scene.core.kernel.Size());
}

// 실제 같은 캠페인 GunHit→사제 body의 HP/WORD/피해통지48입력을 원본 사슬 결과와 직접 비교한다.
TEST_CASE(CampaignHit_PatchNativeGunToPriest) {
    std::size_t count=0;
    // 각 합성 WORD/시각/DPS/회복 입력은 새 실제 월드에서 재생하여 이전 보호막/감시 영향을 섞지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CAMPAIGN_HIT_FIXTURE).rows) {
        if (row[0]!="originals") continue;
        InstallScene scene;scene.runtime->Load();const auto target=MyCampaignPriest(scene);Sid firing{};
        // 실제 저장된 적 썬 디스크 발사기를 발사 부모로 선택한다.
        for (const auto& object:scene.world.Objects()) if (object.object.type==71 && object.object.owner==2) { firing=object.sid;break; }
        CHECK(firing.value!=0 && scene.priests->PriestState().CurrentHitPoints(target)==100);
        const auto now=std::stod(row[2]);scene.core.processState.now=now;scene.assetBindings->PrepareFrame(now);
        scene.combat->PrepareFrame({now,now,0,1});scene.soundState.frame=1;
        scene.hitTypes[71].damagePerSecond=std::stof(row[3]);Put(scene.core.pool.AllocatedBytes(firing),12,Number(row[1]),2);
        if (Number(row[4])) CHECK(scene.core.processes.AddRegular(target,0x25d,2.0f)!=nullptr);
        scene.external.clear();scene.gunHit->Hit(firing,target,now);
        CHECK(scene.priests->PriestState().CurrentHitPoints(target)==std::stoi(row[5]));
        CHECK(Get(scene.core.pool.Slot(firing),12,2)==Number(row[6]));
        const auto notices=std::count_if(scene.external.begin(),scene.external.end(),[](const std::string& event) { return event.starts_with("사제 피해 통지:"); });
        CHECK(static_cast<std::uint32_t>(notices)==Number(row[7]));
        // 피해 통지가 없는 원본 입력은 PriestHit을 선택하지 않으므로 기존 회복 예약도 그대로 남는다.
        CHECK(scene.core.bookkeeping.depth==0);
        CHECK((scene.core.processes.FindEvent(target,0x25d)!=nullptr)==(Number(row[4])!=0 && Number(row[7])==0));++count;
    }
    CHECK(count==48);
}
