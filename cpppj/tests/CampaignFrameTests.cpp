// 실제 저장 미션의 전체 현재 Kernel을 실행하여 미연결 첫 게임 루프 경계를 확인한다.
#include "CampaignInstallSupport.h"

// 각 프로세스를 선택적으로 실행하지 않고 같은 현재 Kernel의 등록 순서를 사용한다.
void CheckCampaignKernel(std::string_view mission) {
    InstallScene scene(mission);scene.runtime->Load();std::uint64_t frames=0;
    // 보호 썬 아처 TYPE은 range9/hpPerSec15이며 별도 발사 대기 대신 애니메이션이 간격을 제어한다.
    CHECK(scene.firingIntervals[71]==0 && scene.gunTypes[71].range==9 && scene.hitTypes[71].damagePerSecond==15);
    // 원본 같은 시계를60등분하여 실제 예약과 총/자산/사제 프로세스를 모두 실행한다.
    for (;frames<600;++frames) {
        const double time=static_cast<double>(frames)/60.0;scene.assetBindings->PrepareFrame(time);
        try { scene.world.RunFrame({time,time,frames ? 1.0/60.0 : 0.0,frames+1},false); }
        catch (...) { std::printf("전체 Kernel 오류 시각 %.9f, frame %llu\n",time,static_cast<unsigned long long>(frames));throw; }
    }
    CHECK(scene.core.bookkeeping.depth==0 && scene.priestState.regen.patch.sequence==scene.priestState.regen.patch.observedSequence);
    const auto player=Sid{static_cast<std::uint16_t>(scene.player->Technology(1))};
    CHECK(!scene.core.processes.FindEvent(player,1));const auto* later=scene.core.processes.FindEvent(player,2);CHECK(later && later->Count()==1);
    std::printf("전체 Kernel 실행: %llu 프레임, process %zu\n",static_cast<unsigned long long>(frames),scene.core.kernel.Size());
}
// 첫 캠페인의 총/신전/사제/가이저/레지던스/Player 예약을 동일 Kernel에서 실행한다.
TEST_CASE(CampaignFrame_ActualWarBeginsFullKernel) { CheckCampaignKernel("thewarbegins"); }
// 실제 둘째 캠페인의 초기 프로세스도 생략 없이 같은 Kernel에 전달한다.
TEST_CASE(CampaignFrame_ActualMasterFullKernel) { CheckCampaignKernel("masterofwhirligigs"); }
