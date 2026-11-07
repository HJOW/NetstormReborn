// 원문 메인 루프의 현재값 재조회/중복 호출과 실제 LAN 프로세스 통지 순서를 복원한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 구 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <windows.h>
#include "root/MainLoop.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <system_error>

namespace netstorm::root { // 실제 장치 호출은 Run/Step가 선택된 경우에만 수행한다.
namespace { // 선택된 상위 장치와 현재 실행 모듈의 실제 wide 경로다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R 메인 루프의 선택된 실제 상위 장치 미연결");return function; } // 빈 초기화/목록/보조 객체를 성공으로 대신하지 않는다.
std::string ModuleDirectory() {
    std::vector<wchar_t> buffer(260);
    for (;;) { // Windows10의 긴 경로도 실제 반환 크기로 확장하며 현재 실행 파일의 폴더를 구한다.
        const auto count=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));if (!count) throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"R 실행 모듈 경로 조회 실패");if (count<buffer.size()) return platform::PathToUtf8(std::filesystem::path(std::wstring(buffer.data(),count)).parent_path())+"\\";if (buffer.size()>=32768) throw std::length_error("R 실행 모듈의 Windows 경로 범위");buffer.resize(std::min(buffer.size()*2,std::size_t{32768}));
    }
} // 원문 Running 진단의 경로만 소유 복사하며 프로세스 CWD를 변경하지 않는다.
}
MainLoop::MainLoop(ConnectionState& connections,Clock& clock,TransceiverTransport& transport,ChallengeStore& store,ChallengeRuntime& challenges,Routing& routing,SharedFiles& files,MainLoopState& state,MainLoopHooks hooks)
    :connections_(connections),clock_(clock),transport_(transport),store_(store),challenges_(challenges),routing_(routing),files_(files),state_(state),hooks_(std::move(hooks)) {}
std::int32_t MainLoop::Run(bool lanArgument) {
    std::fputs("Netstorm Root Server 11.01\n",stdout);if (lanArgument) connections_.localLaunch=1;
    if (connections_.localLaunch) {
        std::fputs("Lan Launch Set True\n",stdout);const auto title=L"Netstorm Root Server "+std::to_wstring(GetCurrentProcessId());SetConsoleTitleW(title.c_str());Sleep(40);const auto window=FindWindowW(nullptr,title.c_str());ShowWindow(window,SW_MINIMIZE);SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);const auto message=RegisterWindowMessageW(L"NETSTORMOOGABOOGA");SendMessageW(HWND_BROADCAST,message,0,0x500); // 원문의 초기화 이전500 통지이며 서버 준비 성공을 합성하지 않는다.
    }
    bool first=true;
    while (!state_.stopRequests) { // 원문은 현재 stop 플래그만으로 최외곽 재초기화를 끝낸다.
        Need(hooks_.initialize)(first);Need(hooks_.log)(0,"Running in "+ModuleDirectory()+"\n");first=false;
        while (!state_.resetRequests && !state_.stopRequests) Step(); // reset/stop은 현재 frame 전체가 끝난 뒤 다음 경계에서 다시 읽는다.
        Need(hooks_.shutdown)();state_.resetRequests=0;
    }
    return 0;
}
void MainLoop::Step() {
    clock_.Tick();std::int32_t wait=60;
    for (std::size_t index=0;index<51;++index) { // 매 슬롯의 현재 token을 한 번 읽고 두 Tick에서는 같은 snapshot을 사용한다.
        const auto challenge=connections_.challenges[index];if (!challenge) continue;const auto next=challenges_.Tick(challenge);if (!(wait<next)) wait=challenges_.Tick(challenge); // 원문 최소값 식에 있는 실제 조건부 두 번째 호출을 제거하지 않는다.
    }
    static_cast<void>(transport_.Pump(wait,store_.BindNetwork(hooks_.network)));PollFiles();PollSchedule();
}
void MainLoop::PollFiles() {
    if (!(state_.watcherFlags&1U)) { state_.watcherFlags|=1U;state_.lastFilePoll=0.0; }if (!(60.0<clock_.State().game-state_.lastFilePoll)) return;state_.lastFilePoll=clock_.State().game;files_.CheckNews();Need(hooks_.reloadRootServers)();files_.CheckBlacklist(); // 중간 호출은 rootservers.dat 목록 재로드이며 Config 설정 재로드가 아니다.
}
void MainLoop::PollSchedule() {
    if (state_.scheduledAt==0.0) return;if (state_.scheduledAt<=clock_.State().game) { state_.scheduledAt=0.0;if (state_.goingDown) ++state_.stopRequests;else ++state_.resetRequests;return; }const auto scheduled=state_.scheduledAt;const auto remaining=scheduled-clock_.State().game;const auto interval=remaining<=300.0 ? 60.0:300.0;if (!(interval<clock_.State().game-state_.lastWarning)) return;
    const auto minutes=std::bit_cast<std::int32_t>(Clock::Truncate((scheduled-clock_.State().game)/60.0)+1U);const auto text=std::string("~r~3~EServer is ")+(state_.goingDown ? "going down":"resetting")+" in "+std::to_string(minutes)+" minutes.\n";routing_.BroadcastText(text);Need(hooks_.log)(29,text);state_.lastWarning=clock_.State().game;
}
}
