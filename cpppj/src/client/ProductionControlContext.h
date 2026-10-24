// 생산 UI의 실제 장기 물리/현재 전역/Player와 초기 옵션을 값 사본 없이 묶는다.
#pragma once
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawSurfaceWorld.h"
#include <array>

namespace netstorm::client {
struct ProductionControlContext {
    std::shared_ptr<RawSurfaceStorage> storage; // 실제 Client Pool/TYPE/공간의 고정 물리다.
    std::shared_ptr<ClientGameplayGlobals> gameplay; // 실제 현재 Player/커서/검색/editor이며 Session 없는 생성도 이를 읽는다.
    std::shared_ptr<ClientPlayerRecords> players; // PlayerKnown일 때만 실제 현재 Player 기록을 조회한다.
    const std::array<std::uint8_t,87>& options; // 실제 현재 BYTE1의 초기 슬롯 수다. UI보다 오래 살아야 한다.
};
}
