// 실제 보호 캠페인의 같은 Kernel에서 썬 아처 각상태와 발사 생성 경계를 검사한다.
#include "CampaignInstallSupport.h"
#include "RawSceneSupport.h"
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
