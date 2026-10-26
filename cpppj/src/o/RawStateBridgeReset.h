// State 정리의 소유자 색 표와 다리 붕괴 스캔을 원문의 현재 물리에서 초기화한다.
#pragma once
#include "o/Bridge.h"
#include <cstdint>
#include <span>

namespace netstorm::o { //Native 주소의 원시 저장 폭을 사용하는 작은 정리 본체다.
void ResetStateOwnerColors(std::span<std::int32_t> colors); //0043b540: 앞9DWORD만0..8로 쓰고 이웃/현재 TYPE/프레임은 보존한다.
void ResetStateBridgeDecayScan(BridgeDecayScanState& scan,float period,double now); //00422490: 현재 float 지연+double 게임 시각을 먼저 계산한 뒤 세 DWORD/기한을 원문 순서로 쓴다.
}
