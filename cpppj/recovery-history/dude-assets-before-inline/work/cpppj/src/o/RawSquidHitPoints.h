// 공통 비가상 HP 쓰기를 같은 raw SID 풀에서 공유한다.
#pragma once
#include "o/SidPool.h"

namespace netstorm::o {
// 004adca0/CD004aed20: 현재 HP26에 패치DWORD/CD하위WORD만 쓴다. 옆필드는 보존한다.
// 사용: 공통 setter를 직접 부르는 건설/Rune에만 사용한다. 사제의 가상 HP 전환 효과는 호출자가 기존 몸체를 연결한다.
void SetRawSquidHitPoints(SidPool& pool,Sid sid,std::int32_t value);
}
