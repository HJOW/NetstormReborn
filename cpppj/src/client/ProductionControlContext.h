// 생산 UI의 실제 장기 물리/현재 전역/Player와 초기 옵션을 값 사본 없이 묶는다.
#pragma once
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawSurfaceWorld.h"
#include <array>

namespace netstorm::client {
class ClientSpGlobals;class OwnerColorTableState; // 예산과 현재 색표는 World 유무와 무관한 실제 Client owner다.
struct ProductionControlContext {
    std::shared_ptr<RawSurfaceStorage> storage; // 실제 Client Pool/TYPE/공간의 고정 물리다.
    std::shared_ptr<ClientGameplayGlobals> gameplay; // 실제 현재 Player/커서/검색/editor이며 Session 없는 생성도 이를 읽는다.
    std::shared_ptr<ClientPlayerRecords> players; // PlayerKnown일 때만 실제 현재 Player 기록을 조회한다.
    const std::array<std::uint8_t,87>& options; // 실제 현재 BYTE1의 초기 슬롯 수다. UI보다 오래 살아야 한다.
    std::shared_ptr<ClientSpGlobals> sp{}; // 실제 Client는 같은00531890 분산표를 제공한다. 기존 독립 Session 문맥만 생략하여 종전 Runtime 경로를 쓴다.
    std::shared_ptr<OwnerColorTableState> ownerColorTables{}; // 최초 HUD도 Session 없이 같은 현재557940을 빌린다.
};
}
