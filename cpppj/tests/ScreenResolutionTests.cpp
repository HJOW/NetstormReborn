// 원본 세 행과1280 폭의 네 화면비 및FSR 전용 출력 정책을 검사한다.
#include "TestSupport.h"
#include "client/ScreenResolutions.h"
using namespace netstorm::client;
// 일반 화면은 최대1280 폭이며FSR 없이도 선택할 수 있다.
TEST_CASE(ScreenResolutions_Native1280AndFourAspects) {
    CHECK(kScreenResolutionChoices.size()==7 && kOriginalScreenResolutions.size()==3 && kCompatibilityScreenResolutions.size()==4);
    CHECK(kOriginalScreenResolutions[0][0]==640 && kOriginalScreenResolutions[1][0]==800 && kOriginalScreenResolutions[2][0]==1024);
    for (const auto& size:kScreenResolutionChoices) { // 모든 일반 크기는 실제 논리 크기와 같아야 한다.
        CHECK(size[0]<=1280 && size[1]<=1024 && IsSupportedScreenResolution(size[0],size[1]));
        CHECK(LogicalScreenSize(size[0],size[1])==size);
    }
    CHECK(IsSupportedScreenResolution(1280,720) && IsSupportedScreenResolution(1280,800) && IsSupportedScreenResolution(1280,960) && IsSupportedScreenResolution(1280,1024));
    CHECK(!IsSupportedScreenResolution(1366,768,true) && !IsSupportedScreenResolution(1600,900,true) && !IsSupportedScreenResolution(1280,1080,true));
}
// 세 출력 폭과 네 화면비는FSR 장치가 있어야 하며 같은1280 폭/화면비로 렌더링한다.
TEST_CASE(ScreenResolutions_FsrPreservesAllFourAspectsAt1280) {
    CHECK(kFsrScreenResolutionChoices.size()==12);
    for (const auto& size:kFsrScreenResolutionChoices) { // 실제 소스와 출력의 비율을 정수 교차곱으로 검사한다.
        CHECK(IsFsrScreenResolution(size[0],size[1]) && !IsSupportedScreenResolution(size[0],size[1]) && IsSupportedScreenResolution(size[0],size[1],true));
        const auto logical=LogicalScreenSize(size[0],size[1]);
        CHECK(logical[0]==1280 && IsSupportedScreenResolution(logical[0],logical[1]));
        CHECK(logical[0]*size[1]==logical[1]*size[0]);
        CHECK(NormalizeScreenResolution(size[0],size[1],false)==logical);
        CHECK(NormalizeScreenResolution(size[0],size[1],true)==size);
    }
    CHECK(!IsSupportedScreenResolution(3840,2560,true) && !IsSupportedScreenResolution(1920,1081,true));
}
// 오래된 설정·장치 미지원·손상된 값 모두 크기 할당 전에 제한한다.
TEST_CASE(ScreenResolutions_StartupMigrationDoesNotAllocateOldLargeSurfaces) {
    CHECK(NormalizeScreenResolution(1366,768,false)==(std::array<std::uint32_t,2>{1280,720}));
    CHECK(NormalizeScreenResolution(1600,900,true)==(std::array<std::uint32_t,2>{1280,720}));
    CHECK(NormalizeScreenResolution(1920,1200,false)==(std::array<std::uint32_t,2>{1280,800}));
    CHECK(NormalizeScreenResolution(0,0,true)==(std::array<std::uint32_t,2>{1024,768}));
    CHECK(NormalizeScreenResolution(-1,-1,true)==(std::array<std::uint32_t,2>{1024,768}));
}
