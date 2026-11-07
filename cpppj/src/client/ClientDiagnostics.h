// 선택 trace와 개발자 F7 덤프가 공유하는 Client 수명의 원문 진단 전역이다.
#pragma once
#include "client/RawTraceLog.h"
#include "client/InputEvent.h"
#include <filesystem>
#include <memory>

namespace netstorm::o { class Kernel; } // 실제 Client Kernel을 빌리며 새 실행기를 만들지 않는다.

namespace netstorm::client { // 장기 로그 상태와 실제 입력 관문만 선언하며 월드를 별도 소유하지 않는다.
class RawSurfaceStorage; // 현재 Pool/TYPE/진단 장치의 실제 소유자다.
class ClientGameplayGlobals; // 상세 문자열의 장기 소유권을 Kernel 장치 수명까지 유지한다.
// 시작/창 없는 집중 검사 모두 같은 요약 본문·사용자 경로·현재 단언을 연결한다. Kernel/Surface는 약한 참조로 순환 소유를 피한다.
void BindClientKernelDiagnostics(const std::shared_ptr<o::Kernel>& kernel,const std::shared_ptr<RawSurfaceStorage>& surface,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::filesystem::path& directory,bool userPolicy);
struct ClientTraceGlobals {
    std::uint32_t heapTrace{},heapAllocated{}; //0054dab0/00557dfc의 BSS0이며 Session 교체와 무관한 원문 프레임 진단이다. 빈437f0에 호스트 힙 집계를 추가하지 않는다.
    TraceLogState log; //00542658/005ca8d0 이후의 같은 FILE/버퍼/들여쓰기/필터다.
    std::uint32_t depth{},suppress{},traceToChat{}; //0055a4a0/005c8454/00531854의 최초BSS0이며 미션 전환 때 보존한다.
};
// Guide/Gump 뒤·console/정지/일반F7 전에 원래 세 호출 경계를 실행한다. 단일RET은 별도 효과가 없다.
bool DispatchDiagnosticKey(InputEvent event,const std::function<void()>& dump);
}
