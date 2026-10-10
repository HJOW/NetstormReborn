// 원본 초기화/종료/500ms worker 정책과 실제 Windows 협조 종료를 분리해 검증한다.
#include "client/SoundMusicRuntime.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// CreateThread에 넘기는 callback은 원본의 사라지는 지역 변수 주소 대신 힙에서 소유한다.
struct WorkerEntry { std::function<std::uint32_t()> callback; };
// 실제 Windows stdcall 진입점이다. callback/예외를 프로세스 밖으로 내보내지 않는다.
DWORD WINAPI WorkerMain(void* parameter) {
    std::unique_ptr<WorkerEntry> entry(static_cast<WorkerEntry*>(parameter));
    try { return entry->callback(); } catch (...) { return 0; }
}
// 한 Runtime의 실제 이벤트/스레드다. 핸들 닫기는 Runtime 수명 정책에서만 수행한다.
struct Win32Worker {
    HANDLE event{},thread{};std::atomic<bool> stopping{false};
};
}
// 원본 API 인자/반환을 Windows에 연결하되 TerminateThread 대신 정상 종료 요청/합류를 사용한다.
MusicRuntimeHooks MakeWin32MusicRuntimeHooks() {
    auto owner=std::make_shared<Win32Worker>();MusicRuntimeHooks h;
    h.createEvent=[owner]() {
        owner->stopping=false;owner->event=CreateEventW(nullptr,TRUE,FALSE,nullptr);return reinterpret_cast<MusicRuntimeHandle>(owner->event);
    }; // 익명 수동 리셋 이벤트다.
    h.createThread=[owner](std::function<std::uint32_t()> callback,std::uint32_t& id) {
        auto* entry=new(std::nothrow) WorkerEntry{std::move(callback)};if (!entry) return MusicRuntimeHandle{};
        DWORD nativeId=id;owner->thread=CreateThread(nullptr,0,WorkerMain,entry,0,&nativeId);id=nativeId;
        if (!owner->thread) delete entry;return reinterpret_cast<MusicRuntimeHandle>(owner->thread);
    }; // 스레드 생성 실패에서 callback 소유권을 되돌린다.
    h.wait=[owner](MusicRuntimeHandle handle,std::uint32_t milliseconds) {
        const auto native=reinterpret_cast<HANDLE>(handle);
        if (native==owner->event && owner->stopping.load()) return std::uint32_t{WAIT_OBJECT_0};
        const auto result=WaitForSingleObject(native,milliseconds);
        return native==owner->event && owner->stopping.load() ? std::uint32_t{WAIT_OBJECT_0} : static_cast<std::uint32_t>(result);
    }; // 협조 종료 요청은 실패한 SetEvent도 다음 500ms 대기 뒤 빠져나오게 한다.
    h.signal=[](MusicRuntimeHandle handle) { return SetEvent(reinterpret_cast<HANDLE>(handle))!=FALSE; }; // 실제 종료 이벤트를 신호한다.
    h.terminate=[owner](MusicRuntimeHandle) { owner->stopping=true;if (owner->event) SetEvent(owner->event); }; // mutex/COM/파일을 소유한 스레드는 강제로 중단하지 않는다.
    h.close=[owner](MusicRuntimeHandle handle) {
        const auto native=reinterpret_cast<HANDLE>(handle);
        if (native==owner->thread) {
            owner->stopping=true;if (owner->event) SetEvent(owner->event);WaitForSingleObject(native,INFINITE);CloseHandle(native);owner->thread=nullptr;
        } else if (native==owner->event) { CloseHandle(native);owner->event=nullptr; }
    };return h; // callback 종료 전에는 이벤트/파일/채널 수명을 끝내지 않는다.
}
// 입력 music의 공유 상태/기본 채널을 검사해 worker가 다른 상태를 갱신하지 않게 한다.
MusicRuntime::MusicRuntime(SoundState& sound,MusicRuntimeState& state,MusicChannel& primary,MusicChannel& secondary,SoundMusic& music,
    MusicBufferHooks buffers,MusicRuntimeHooks hooks):sound_(sound),state_(state),primary_(primary),secondary_(secondary),music_(music),buffers_(std::move(buffers)),hooks_(std::move(hooks)) {
    if (&music_.state_!=&sound_ || &music_.channel_!=&primary_ || &primary_==&secondary_) throw std::invalid_argument("음악 Runtime의 상태/두 채널 연결이 올바르지 않습니다");
    if (!hooks_.createEvent || !hooks_.createThread || !hooks_.wait || !hooks_.signal || !hooks_.terminate || !hooks_.close) throw std::invalid_argument("음악 Runtime의 필수 커널 경계 누락");
}
// 정상 종료 후 정지 콜백 예외도 객체 소멸 밖으로 던지지 않는다. 실패는 조회할 수 있도록 보관한다.
MusicRuntime::~MusicRuntime() { try { Shutdown(); } catch (...) { std::lock_guard guard(failureMutex_);failure_=std::current_exception(); } }
// 원본은 +0x14/+0x3a 패딩과 CRITICAL_SECTION 공간을 직접 0으로 덮지 않는다.
void MusicRuntime::Reset(MusicChannel& channel,std::uint32_t index) {
    std::lock_guard guard(channel.mutex_);
    auto raw=channel.state_.Raw();std::fill(raw.begin(),raw.begin()+0x14,std::uint8_t{});std::fill(raw.begin()+0x18,raw.begin()+0x3a,std::uint8_t{});
    std::fill(raw.begin()+0x3c,raw.begin()+0x44,std::uint8_t{});if (hooks_.section) hooks_.section(index,true);
}
// 선택 로그 경계에 원본 채널과 문장을 그대로 전달한다.
void MusicRuntime::Log(std::string_view text) { if (hooks_.log) hooks_.log(9,text); }
// 원본은 이벤트/스레드 실패를 보고한 뒤에도 준비 플래그를 세운다. 소리 미준비는 세우지 않는다.
void MusicRuntime::Initialize() {
    std::lock_guard lifecycle(lifecycleMutex_);std::unique_lock commands(music_.mutex_);
    if (sound_.musicInitialized) return;
    { std::lock_guard failure(failureMutex_);failure_=nullptr; }
    Reset(primary_,0);Reset(secondary_,1);music_.stopping_=false;
    if (!sound_.initialized) { Log("Attempting to initialize music with no sound.\n");return; }
    try {
        state_.shutdown=hooks_.createEvent();state_.thread=hooks_.createThread([this] { return RunWorker(); },state_.threadId); // worker는 초기화 완료의 음악 명령 잠금을 기다린다.
        const bool patch=music_.edition_==o::OriginalEdition::Patch1078;
        if (!state_.thread) {
            Log("Failed to create music update thread.\n");
            if (!state_.thread && hooks_.report) hooks_.report("hMusicThread != NULL",patch ? 0x6da : 0x6d5);
        } else if (!state_.shutdown && hooks_.report) hooks_.report("hMusicShutdown != NULL",patch ? 0x6e0 : 0x6db);
        sound_.musicInitialized=true;
    } catch (...) {
        sound_.musicInitialized=true;music_.stopping_=true;commands.unlock();Shutdown();throw; // 호스트 진단 예외도 이미 만든 worker를 반납한다.
    }
}
// 이벤트/스레드 대기 동안 음악 명령 잠금을 풀어 worker가 정상 반환할 수 있게 한다.
void MusicRuntime::Shutdown() {
    std::lock_guard lifecycle(lifecycleMutex_);std::exception_ptr stopFailure;
    {
        std::lock_guard commands(music_.mutex_);if (!sound_.musicInitialized) return;music_.stopping_=true;
        try { primary_.Stop(); } catch (...) { stopFailure=std::current_exception(); }
    }
    if (state_.thread && hooks_.wait(state_.thread,0)!=0) {
        if (!hooks_.signal(state_.shutdown)) { Log("Failed to signal music thread shutdown. Terminating thread.\n");hooks_.terminate(state_.thread); }
        std::uint32_t attempts=0;auto result=hooks_.wait(state_.thread,kMusicShutdownMilliseconds);
        // TIMEOUT에서만 재시도하며 다른 실패/포기 반환은 원본처럼 반복하지 않는다.
        while (result==kMusicWaitTimeout && attempts<2) {
            Log("The music update has not died. Attempting to terminate it.\n");hooks_.terminate(state_.thread);++attempts;
            result=hooks_.wait(state_.thread,kMusicShutdownMilliseconds);
        }
        if (attempts>1) Log("The music update thread will not die!!\n");
    }
    if (hooks_.section) { hooks_.section(0,false);hooks_.section(1,false); }
    if (state_.thread) { hooks_.close(state_.thread);state_.thread=0; }
    if (state_.shutdown) { hooks_.close(state_.shutdown);state_.shutdown=0; }
    { std::lock_guard commands(music_.mutex_);sound_.musicInitialized=false; }
    if (stopFailure) std::rethrow_exception(stopFailure);
}
// 첫 갱신은 대기 전이다. 준비 전/비활성 채널도 같은 이벤트 대기 정책을 따른다.
std::uint32_t MusicRuntime::RunWorker() noexcept {
    try {
        const auto event=state_.shutdown;if (!event) return 0;
        // 종료 이벤트가 TIMEOUT일 때만 기본 채널을 다시 갱신한다.
        do { music_.Update(buffers_); } while (hooks_.wait(event,kMusicUpdateMilliseconds)==kMusicWaitTimeout);
    } catch (...) { std::lock_guard failure(failureMutex_);failure_=std::current_exception(); }
    return 0;
}
// worker와 진단 조회가 서로 예외 포인터를 동시에 수정/읽지 않게 한다.
std::exception_ptr MusicRuntime::WorkerFailure() const { std::lock_guard guard(failureMutex_);return failure_; }
}
