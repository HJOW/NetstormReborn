// 실제 저장 미션의 전체 현재 Kernel을 실행하여 미연결 첫 게임 루프 경계를 확인한다.
#include "CampaignInstallSupport.h"

// 각 프로세스를 선택적으로 실행하지 않고 같은 현재 Kernel의 등록 순서를 사용한다.
TEST_CASE(CampaignFrame_ActualWarBeginsFullKernel) {
    InstallScene scene;scene.runtime->Load();std::uint64_t frames=0;
    // 원본 같은 시계를60등분하여 실제 예약과 총/자산/사제 프로세스를 모두 실행한다.
    for (;frames<600;++frames) {
        const double time=static_cast<double>(frames)/60.0;scene.assetBindings->PrepareFrame(time);
        try { scene.world.RunFrame({time,time,frames ? 1.0/60.0 : 0.0,frames+1},false); }
        catch (...) { std::printf("전체 Kernel 오류 시각 %.9f, frame %llu\n",time,static_cast<unsigned long long>(frames));throw; }
    }
    CHECK(scene.core.bookkeeping.depth==0);std::printf("전체 Kernel 실행: %llu 프레임, process %zu\n",static_cast<unsigned long long>(frames),scene.core.kernel.Size());
}
