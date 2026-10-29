// 선택 trace와 개발자 F7 덤프가 공유하는 Client 수명의 원문 진단 전역이다.
#pragma once
#include "client/RawTraceLog.h"
#include "client/InputEvent.h"

namespace netstorm::client { // 장기 로그 상태와 실제 입력 관문만 선언하며 월드를 별도 소유하지 않는다.
struct ClientTraceGlobals {
    TraceLogState log; //00542658/005ca8d0 이후의 같은 FILE/버퍼/들여쓰기/필터다.
    std::uint32_t depth{},suppress{},traceToChat{}; //0055a4a0/005c8454/00531854의 최초BSS0이며 미션 전환 때 보존한다.
};
// Guide/Gump 뒤·console/정지/일반F7 전에 원래 세 호출 경계를 실행한다. 단일RET은 별도 효과가 없다.
bool DispatchDiagnosticKey(InputEvent event,const std::function<void()>& dump);
}
