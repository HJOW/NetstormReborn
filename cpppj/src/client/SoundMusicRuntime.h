// 원본 두 음악 채널·이벤트/작업 스레드 수명과 Windows의 협조 종료 경계를 연결한다.
#pragma once
#include "client/SoundMusic.h"
#include <exception>

namespace netstorm::client {
// 원본 500ms 갱신 주기와 종료 때의 5000ms 대기/최대 두 번 중단 요청이다.
inline constexpr std::uint32_t kMusicUpdateMilliseconds=500,kMusicShutdownMilliseconds=5000,kMusicWaitTimeout=0x102;
// OS 핸들은 호스트 포인터 폭으로 보관하며 x86 채널 raw에는 넣지 않는다.
using MusicRuntimeHandle=std::uintptr_t;
// 원본 스레드/수동 리셋 종료 이벤트/스레드 번호 전역이다. 초기 배치 외에는 Runtime이 관리한다.
struct MusicRuntimeState { MusicRuntimeHandle thread{},shutdown{};std::uint32_t threadId{}; };
// 원본 커널 경계다. 상태 전이는 Runtime이 수행하며 생성/대기/신호/중단/닫기/section/log는 예외를 던지지 않아야 한다. report의 호스트 진단 예외는 자원 정리 후 전달한다.
// 사용: createThread는 callback을 보관해 별도 스레드에서 실행한다. 실패 시 0이며 같은 스레드에서 실행하면 안 된다.
struct MusicRuntimeHooks {
    // 두 채널의 CRITICAL_SECTION 초기화/삭제를 관찰한다. 호스트 채널 mutex 자체는 채널 객체가 소유한다.
    std::function<void(std::uint32_t channel,bool initialize)> section;
    // 익명 수동 리셋/초기 비신호 이벤트를 만든다.
    std::function<MusicRuntimeHandle()> createEvent;
    // 정상 반환 시 callback과 그 대상이 스레드 종료까지 살아야 한다. 출력 ID도 원본처럼 보관한다.
    std::function<MusicRuntimeHandle(std::function<std::uint32_t()> callback,std::uint32_t& id)> createThread;
    // 원본 WAIT_OBJECT_0/WAIT_TIMEOUT/WAIT_FAILED 값을 반환한다. worker는 TIMEOUT에서만 반복한다.
    std::function<std::uint32_t(MusicRuntimeHandle,std::uint32_t milliseconds)> wait;
    // 종료 이벤트를 신호 상태로 만든다.
    std::function<bool(MusicRuntimeHandle)> signal;
    // 원본 강제 중단 요청 위치다. 실제 Windows 경계는 협조 종료 요청으로 바꾸며 스레드를 강제 삭제하지 않는다.
    std::function<void(MusicRuntimeHandle)> terminate;
    // 실제 스레드는 완전히 종료한 뒤 핸들을 반납해야 한다. 이벤트는 스레드 다음에 닫는다.
    std::function<void(MusicRuntimeHandle)> close;
    // 원본 채널 9의 진단과 실패 assert를 기록한다. assert 보고 후 초기화를 계속하는 원본 동작을 보존한다.
    std::function<void(std::uint32_t,std::string_view)> log;
    SoundAssertReport report;
};
// 실제 Windows 이벤트/스레드 경계다. 아직 장치/창은 열지 않으며 Initialize가 스레드를 만든다.
// 사용: 한 Runtime마다 별도로 만든다. 중단 요청은 협조 종료, close(thread)는 마지막 정상 종료까지 기다린다.
MusicRuntimeHooks MakeWin32MusicRuntimeHooks();
// 두 채널 초기화→기본 채널 worker→기본 채널 Stop/종료 대기/두 채널 잠금 삭제의 원본 순서를 복원한다.
// 사용: state/music/두 채널/콜백 대상은 Runtime보다 오래 살아야 한다. 먼저 Runtime을 종료하고 장치/파일 소유자를 종료한다.
class MusicRuntime {
public:
    // 참조/경계를 보관하며 OS 자원은 열지 않는다. music은 같은 sound/primary를 참조해야 한다.
    MusicRuntime(SoundState& sound,MusicRuntimeState& state,MusicChannel& primary,MusicChannel& secondary,SoundMusic& music,
        MusicBufferHooks buffers,MusicRuntimeHooks hooks);
    // 남은 worker를 협조 종료/대기한다. 객체 밖 콜백의 수명은 이 시점까지 유지해야 한다.
    ~MusicRuntime();
    // worker callback이 this를 참조하므로 복제/이동하지 않는다.
    MusicRuntime(const MusicRuntime&)=delete;
    MusicRuntime& operator=(const MusicRuntime&)=delete;
    // 이미 준비됐으면 아무 일도 하지 않는다. 소리 준비 전에도 두 채널의 알려진 필드를 초기화하지만 worker는 만들지 않는다.
    void Initialize();
    // 기본 채널 Stop→비동기 종료 요청/최대 세 번 대기→두 채널 관찰/핸들 닫기→준비 해제다. 재호출 가능하다.
    void Shutdown();
    // 종료 이벤트가 있을 때 기본 채널만 갱신하고 500ms 기다린다. kernel callback 및 독립 재생 검사가 사용한다.
    std::uint32_t RunWorker() noexcept;
    // worker 안의 호스트 예외를 조회한다. 초기화 때 비우고 종료 뒤에도 진단을 보존한다.
    std::exception_ptr WorkerFailure() const;
private:
    // 원본이 명시한 필드만 0으로 만들고 패딩/호스트 mutex는 건드리지 않는다.
    void Reset(MusicChannel& channel,std::uint32_t index);
    // 원본 진단을 채널 9에 전달한다.
    void Log(std::string_view text);
    // 참조/콜백 수명과 초기화/종료 직렬화다. 음악 명령 잠금은 SoundMusic과 공유하고 wait 동안 잡지 않는다.
    SoundState& sound_;MusicRuntimeState& state_;MusicChannel& primary_;MusicChannel& secondary_;SoundMusic& music_;
    MusicBufferHooks buffers_;MusicRuntimeHooks hooks_;std::recursive_mutex lifecycleMutex_;
    // worker 예외만 별도 잠금으로 보호해 종료 대기와 충돌하지 않게 한다.
    mutable std::mutex failureMutex_;std::exception_ptr failure_;
};
}
