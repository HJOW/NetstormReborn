// CD 00484140의 현재 Player 진단과 비전투/미션 반환 경계를 복원한다.
#pragma once
#include "o/RawBattleCompletion.h"

namespace netstorm::o { // CD의172BYTE 기록을 PATCH180BYTE 판정기에 전달하지 않는다.
void RunCdBattleCompletionPrefix(const BattleCompletionGlobals& globals,const BattleCompletionFrameGlobals& frame,
    const BattleCompletionHooks& playerHooks,const BattleCompletionFrameHooks& frameHooks); // 네 SID 쌍을 진단한 뒤 원문 inactive 관문에서 반환한다. 일반 전투 활성 본문은 명시 미복원이다.
}
