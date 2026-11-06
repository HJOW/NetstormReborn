// 자산의 +74 재정의 중 생산 변경 접두와 고정 중립 소유자를 실제 현재 가상 표로 분배한다.
#pragma once
#include "o/RawBridgeConnect.h"

namespace netstorm::o { // 같은 SID 풀의 공통 소유자와 표면 재정의를 합성한다.
// productionDirty는 실제 생산 목록의 DWORD이며 반환한 함수보다 오래 살아야 한다.
// 신전/사제는 상위 전용 분배기가 먼저 처리하고, 나머지 알려진 자산은 이 함수를 사용한다.
std::function<void(Sid,std::uint32_t)> MakeAssetOwnerDispatch(SidPool& pool,const BridgeConnectState& state,
    std::uint32_t& productionDirty,std::function<void(Sid,std::uint32_t)> base);
}
