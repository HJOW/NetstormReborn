// 단위 테스트 실행 파일의 진입점
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string_view>

#include "TestSupport.h"
#include "SurfaceLifecycleInspect.h"

namespace netstorm::test {
// 실제 원본 WAV 전수 읽기의 선택 검사다. 소리 장치와 창을 열지 않는다.
void InspectSoundFiles(const std::filesystem::path& root);
// 열린 원본 음악 파일의 헤더/처음·끝 부분 읽기/닫기를 확인한다. 장치·창은 열지 않는다.
void InspectMusicFiles(const std::filesystem::path& root);
// 사용자가 명시적으로 선택하는 실제 장치 검사다. 기본 CTest에서 실행하지 않는다.
void InspectSoundDevice(const std::filesystem::path& root);
}

// 등록된 테스트를 모두 실행한다. 실패한 검사가 하나라도 있으면 1 을 돌려준다.
int main(int argc,char** argv) {
    // 특정 복원 모듈만 빠르게 재검사할 때 --filter <이름 일부>를 지정한다. 기본 CTest는 전체를 실행한다.
    const bool filtered=argc==3 && std::string_view(argv[1])=="--filter";
    // 선택 검사는 원본 파일을 읽을 때만 실행한다. 일반 CTest에는 원본 자산이 필요 없다.
    if (argc!=1 && !filtered) {
        try {
            if (argc==3 && std::string_view(argv[1])=="--inspect-sound-files") {
                netstorm::test::InspectSoundFiles(argv[2]);return 0;
            }
            if (argc==3 && std::string_view(argv[1])=="--inspect-music-files") {
                netstorm::test::InspectMusicFiles(argv[2]);return 0;
            }
            if (argc==3 && std::string_view(argv[1])=="--inspect-sound-device") {
                netstorm::test::InspectSoundDevice(argv[2]);return 0;
            }
            if ((argc!=3 && argc!=4) || std::string_view(argv[1])!="--inspect-surface-graph" ||
                (argc==4 && std::string_view(argv[3])!="--cd")) throw std::runtime_error("--filter <name> | --inspect-sound-files <game-dir> | --inspect-music-files <game-dir> | --inspect-sound-device <game-dir> | --inspect-surface-graph <game-dir> [--cd]");
            netstorm::test::InspectSurfaceLifecycle(argv[2],argc==4 ? netstorm::o::OriginalEdition::Cd1072 : netstorm::o::OriginalEdition::Patch1078);
        } catch (const std::exception& error) { netstorm::test::ReportFailure(__FILE__,__LINE__,error.what()); }
        return netstorm::test::FailureCount()==0 ? 0 : 1;
    }
    const auto& cases = netstorm::test::Registry();
    std::size_t executed=0;
    // 등록 순서대로 실행한다. 한 테스트가 실패해도 나머지를 계속 실행한다.
    for (const auto& testCase : cases) {
        if (filtered && std::string_view(testCase.name).find(argv[2])==std::string_view::npos) continue;
        ++executed;
        const int failuresBefore = netstorm::test::FailureCount();
        // 파서 예외도 실패로 기록하여 뒤 테스트를 계속 확인한다.
        try { testCase.function(); }
        catch (const std::exception& error) {
            netstorm::test::ReportFailure(__FILE__, __LINE__, error.what());
        }
        const bool passed = netstorm::test::FailureCount() == failuresBefore;
        std::printf("[%s] %s\n", passed ? " OK " : "FAIL", testCase.name);
    }
    if (filtered && executed==0) netstorm::test::ReportFailure(__FILE__,__LINE__,"필터와 일치하는 검사가 없습니다");
    std::printf("%d test(s), %d failed check(s)\n",
                static_cast<int>(executed), netstorm::test::FailureCount());
    return netstorm::test::FailureCount() == 0 ? 0 : 1;
}
