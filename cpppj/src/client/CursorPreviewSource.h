// 실제 커서의 겹치는 전역을 수집·표시 양쪽에 복사 없이 제공한다.
#pragma once
#include "o/SidPool.h"
#include <array>

namespace netstorm::o { struct BridgeConnectState; }
namespace netstorm::client {
struct CursorDropState; // 잠금·선택 타입·플레이어·좌표를 소유하는 실제 커서 상태다.
struct CursorLifecycleState; // 부모·임시 SID 목록·비용과 경고 치수를 소유한다.
struct CursorPlacementState; // 표시 허용과 설명·화면 원점을 소유한다.
// 모든 참조 대상은 연결한 수집기·표시기보다 오래 살아야 한다. PATCH 전역 배치를 사용한다.
struct CursorPreviewSource {
    const o::SidPool& pool; // 미리보기 SID가 실제로 속한 풀이다. 다른 풀 연결은 거부한다.
    const CursorDropState& drop; // 최종 내려놓기 허용값은 표시 허용값으로 사용하지 않는다.
    const CursorLifecycleState& lifecycle; // 콜백 도중에도 현재 목록과 측정 결과를 다시 읽는다.
    const CursorPlacementState& placement; // 원본55a290·294·298·29c의 개별 판정이다.
    const o::BridgeConnectState& surfaces; // 실제 다리·섬·커넥터 타입 전역이다.
    const std::array<std::uint8_t,144>& grid; // 실제 Canon 배치 검사기가 기록한 열 우선 격자다.
};
}
