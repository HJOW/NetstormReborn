// 실제 Client의 기존 State 정리와 음악 Stop을 원문 메모리 실패의 고정 순서에 연결한다.
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/RawSurfaceWorld.h"
#include "client/UberGump.h"
#include "o/RawAllocationFailure.h"
#include <cstdlib>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace netstorm::client { // 실제 OS 경계는 이 연결에만 두며 공유본문 검사는 창 없이 실행한다.
bool Client::CleanupNativeGameForExit() {
    const o::SquidOwnerMode inactive{false,false,false};const auto& modes=surfaceStorage_ ? surfaceStorage_->ownerMode:inactive; // 초기 자산 할당 실패는 현재 세 모드가 모두0인 원문 초기 상태다.
    o::AllocationFailureHooks hooks;hooks.context=this;
    hooks.cleanup=[](void* context,std::uint32_t reason) { auto& client=*static_cast<Client*>(context);if (!client.menu_) throw std::logic_error("종료 정리의 현재 Uber 부모 미연결");client.menu_->CleanupNativeStateWorld(reason); }; // 이미 연결된 전체004b2df0을 재구현하지 않는다.
    return o::CleanupRawGameForExit({nativeStartupComplete_,modes.fort,modes.battle,modes.challenge,nativeExitGuard_},hooks);
}
[[noreturn]] void Client::HandleAllocationFailure() {
    const o::SquidOwnerMode inactive{false,false,false};const auto& modes=surfaceStorage_ ? surfaceStorage_->ownerMode:inactive; // 실패 중 새 Storage/Session을 만들지 않는다.
    o::AllocationFailureHooks hooks;hooks.context=this;
    hooks.message=[](void*,const char* body,const char* title,std::uint32_t flags) { static_cast<void>(::MessageBoxA(nullptr,body,title,flags)); }; // 원문 NULL HWND/고정 literal/MB_ICONHAND를 사용한다.
    hooks.cleanup=[](void* context,std::uint32_t reason) { auto& client=*static_cast<Client*>(context);if (!client.menu_) throw std::logic_error("메모리 실패 정리의 현재 Uber 부모 미연결");client.menu_->CleanupNativeStateWorld(reason); }; // 선택된 현재 모드에서만 실제 전체 정리를 호출한다.
    hooks.currentEvent=[](void*)->std::uintptr_t { return 0; }; //0054dbcc는 구 HTTP 서버의 종료 이벤트이며 현재 제품은 해당 서버를 만들지 않아NULL이다.
    hooks.signal=[](void*,std::uintptr_t event) { static_cast<void>(::SetEvent(reinterpret_cast<HANDLE>(event))); }; // 원문46d140처럼 BOOL 반환을 추가 성공 조건으로 쓰지 않는다.
    hooks.stopMusic=[](void* context) { auto& client=*static_cast<Client*>(context);if (client.audio_) client.audio_->StopMusic();else if (client.disabledAudio_) client.disabledAudio_->StopMusic(); }; // 준비 전/noAudio도 기존 음악 Stop 관문이 처리한다.
    hooks.terminate=[](void*,std::int32_t code) { std::_Exit(code); }; // 원문은0으로즉시종료하며C++소멸/새설정저장을추가하지않는다.
    o::HandleRawAllocationFailure({nativeStartupComplete_,modes.fort,modes.battle,modes.challenge,nativeExitGuard_},hooks);std::_Exit(0);
}
}
