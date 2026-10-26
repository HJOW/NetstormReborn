// HUD의 실제43df80을 장기 Client 물리 또는 현재 Session의 한정 실행기에 연결한다.
#pragma once
#include "client/ProductionControlContext.h"

namespace netstorm::client { // World 수명과 독립인 생산 내용물 부모의 Client 접합이다.
void SynchronizeProductionControlContents(const ProductionControlContext& context); //44f760의 실제 Player Deck을 먼저 확보하고 SID0도 실제 finder로 순회한다.
}
