// 실제 보호 캠페인의 같은 Kernel에서 썬 디스크 발사기 각상태와 발사 생성 경계를 검사한다.
#include "CampaignInstallSupport.h"
#include "RawSceneSupport.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawCrossbowAnimation.h"
using namespace netstorm::test::rawscene;

// 현재 저장 총과 사제의 실제 위치를 확인하여 전투 입력을 같은 월드에서 선택한다.
TEST_CASE(CampaignFire_ActualInitialPositions) {
    InstallScene scene;scene.runtime->Load();std::size_t guns=0,priests=0;
    // 현재 raw 부모 위치를 읽으며 임시 객체 사본을 만들지 않는다.
    for (std::uint32_t id=5;id<scene.core.pool.Capacity();++id) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};const auto raw=scene.core.pool.Slot(sid);
        if ((raw[11]&7) || (raw[10]!=71 && raw[10]!=scene.priestState.movement.priest)) continue;
        std::printf("현재 전투 자산 SID%u TYPE%u owner%u (%.3f,%.3f)\n",id,raw[10],raw[34],std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)));
        if (raw[10]==71) ++guns;else ++priests;
    }
    CHECK(guns && priests==2);
}

// 명시 재배치 입력 뒤 실제 전체 Kernel이 표적 검색→Fire→Missile 생성/첫 이동을 수행한다.
TEST_CASE(CampaignFire_ActualWarBeginsLaunchAndFlight) {
    InstallScene scene;scene.runtime->Load();Sid target{};
    // 실제 저장된 내 사제를 선택한다. 새 가상 표적이나 가짜 GunProcess를 만들지 않는다.
    for (const auto id:scene.priestState.priests.priests.Items()) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};
        if (scene.core.pool.Slot(sid)[34]==1) target=sid;
    }
    CHECK(target.value!=0);
    // 접근 경로/플레이어 입력은 별도 범위다. 이 검사는 실제 공간 Unpop/Pop으로 근거리 입력을 만든다.
    scene.world.Unpop(target,0);scene.world.Pop(target,130,114,0);
    Sid projectile{};std::array<std::uint32_t,2> initial{};bool moved=false,sawFire=false;
    // 모든 현재 프로세스를 실행하되 첫 실제 비행 이동에서 끝내며 충돌/피해 성공으로 주장하지 않는다.
    for (std::uint64_t frame=0;frame<360 && !moved;++frame) {
        const double seconds=static_cast<double>(frame)/60.0;
        const FrameTime time{seconds,seconds,frame ? 1.0/60.0 : 0.0,frame+1};
        scene.assetBindings->PrepareFrame(seconds);scene.combat->PrepareFrame(time);scene.world.RunFrame(time,false);
        // 실제 등록 Form/PID를 조회하며 현재 살아 있는 Missile의 부모만 관찰한다.
        for (std::uint32_t id=5;id<scene.core.pool.Capacity();++id) {
            const auto sid=Sid{static_cast<std::uint16_t>(id)};const auto raw=scene.core.pool.Slot(sid);
            if (raw[11]&7) continue;
            if (raw[10]==35) sawFire=true;
            if (raw[10]!=28) continue;
            auto* flight=dynamic_cast<Missile*>(scene.core.processes.FromForm(sid));CHECK(flight!=nullptr);
            CHECK(flight->Fields().Target()==target && scene.core.pool.Slot(flight->Fields().Firing())[10]==71);
            const auto parent=flight->Fields().Parent();const auto asset=scene.core.pool.Slot(parent);
            CHECK(asset[10]==83 && asset[34]==2);
            const std::array<std::uint32_t,2> position{Get(asset,14),Get(asset,18)};
            if (!projectile.value) { projectile=parent;initial=position; }
            else if (parent==projectile && position!=initial) moved=true;
        }
    }
    CHECK(sawFire && projectile.value && moved && scene.core.bookkeeping.depth==0);
    std::printf("실제 Fire→Missile 첫 이동: 표적%u 투사체%u\n",target.value,projectile.value);
}

// 같은 보호 자원/Host에서 썬 캐논의 세 원본 factory와 현재 시계가 실제로 연결되어야 한다.
TEST_CASE(CampaignFire_ActualSunCannonFactoriesAndClock) {
    InstallScene scene;scene.runtime->Load();Sid target{};
    for (const auto id:scene.priestState.priests.priests.Items()) { // 실제 저장된 플레이어1 사제를 표적으로 사용한다.
        const Sid sid{static_cast<std::uint16_t>(id)};if (scene.core.pool.Slot(sid)[34]==1) target=sid;
    }
    CHECK(target.value!=0);const auto cannon=scene.core.factory.Create(128,0);scene.core.owner.Set(cannon,2);
    auto* gun=CreateGunProcess(scene.core.processes,scene.assetBindings->Gun(),cannon,scene.gunState.processType);CHECK(gun!=nullptr);if (!gun) return;const auto form=gun->Form(); // 이 검사는 void 부모의 실제 생성 분배만 확인하며 건설 완료/Pop을 대신하지 않는다.
    auto hooks=scene.combat->GunHooks(GunProcessHooks{});
    for (const auto stage:{2U,5U,8U}) { // 각 생성 뒤 같은 Host로 정리해 다른 애니메이션과 겹치지 않는다.
        const double now=40.0+stage;scene.combat->PrepareFrame({now,now,.025,stage});hooks.startAnimation(stage,form,stage==8 ? Sid{} : target);
        const auto animationForm=FindAttachedProcess(scene.core.pool,cannon,stage==5 ? 35U : 36U);auto* animation=animationForm.value ? dynamic_cast<GunAnimation*>(scene.core.processes.FromForm(animationForm)) : nullptr;
        CHECK(animation!=nullptr);if (!animation) continue;const auto& fields=animation->Fields();CHECK(fields.Parent()==cannon && fields.Form()==animationForm && fields.GunForm()==form && fields.Time()==now);CHECK(fields.Target()==(stage==8 ? Sid{} : target));
        scene.core.processes.Kill(*animation,0x10);
    }
    CHECK(scene.core.bookkeeping.depth==0);
}

// 같은 실제 캠페인 Core/Host가 크로스보우의 공유 회전과 별도 발사 클래스를 생성해야 한다.
TEST_CASE(CampaignFire_ActualCrossbowFactoriesAndClock) {
    InstallScene scene;scene.runtime->Load();Sid target{};
    for (const auto id:scene.priestState.priests.priests.Items()) { // 실제 저장된 플레이어1 사제를 표적으로 사용한다.
        const Sid sid{static_cast<std::uint16_t>(id)};if (scene.core.pool.Slot(sid)[34]==1) target=sid;
    }
    CHECK(target.value!=0);const auto parent=scene.core.factory.Create(140,0);scene.core.owner.Set(parent,2);auto* gun=CreateGunProcess(scene.core.processes,scene.assetBindings->Gun(),parent,scene.gunState.processType);CHECK(gun!=nullptr);if (!gun) return;auto hooks=scene.combat->GunHooks(GunProcessHooks{});
    for (const auto stage:{2U,5U}) { // void 부모의 생성 분배/시계만 검사하며 건설 완료·투사체 실행은 별도다.
        const double now=70.0+stage;scene.combat->PrepareFrame({now,now,.025,stage});hooks.startAnimation(stage,gun->Form(),target);const auto form=FindAttachedProcess(scene.core.pool,parent,stage==2 ? 36U : 35U);CHECK(form.value!=0);auto* process=scene.core.processes.FromForm(form);CHECK(process!=nullptr);if (!process) continue;
        if (stage==2) { auto* animation=dynamic_cast<ArcherAnimation*>(process);CHECK(animation!=nullptr);if (animation) CHECK(animation->Fields().Parent()==parent && animation->Fields().Target()==target && animation->Fields().Time()==now); }
        else { auto* animation=dynamic_cast<CrossbowAnimation*>(process);CHECK(animation!=nullptr);if (animation) CHECK(animation->Fields().Parent()==parent && animation->Fields().Target()==target && animation->Fields().Time()==now); }
        scene.core.processes.Kill(*process,0x10);
    }
    CHECK(scene.core.bookkeeping.depth==0);
}
