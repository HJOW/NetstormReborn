// 원문18BYTE의 직접 대입 순서를 유지하며 추가 하위 호출·난수·소유자 생성을 하지 않는다.
#include "o/RawClockSyncReset.h"

namespace netstorm::o {
// 빌린 저장소는 이 호출보다 오래 살아야 하며 바인딩 시 초기화 효과는 없다.
void ResetClockSynchronization(const ClockSyncResetGlobals& globals) {
    globals.connection=0;
    globals.replyMode=0;
    globals.gameTicks=0;
}
}
