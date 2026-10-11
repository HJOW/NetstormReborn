// 복원한 가시 표시 수집을 공유 Renderer 정렬 비교와 실제 표시 Draw에 연결하는 클라이언트 어댑터다.
#pragma once
#include "o/ConstructionVisualSystem.h"
#include "client/DrawSubmission.h"

namespace netstorm::client {
// 월드 영역의 가시 건설 표시를 작업 버퍼에 수집하고 선택적으로 깊이/y/x 정렬한 뒤 제출한다.
// 반환값은 수집 항목 수다. 공간이 모자라면 원본처럼 먼저 등록된 항목으로 제한하며 잘린 항목은 정렬에 참여하지 않는다.
// 작업 버퍼/월드/팔레트는 호출 동안 살아야 한다. 공통 병합/분배 경로를 건설 수집기만 연결하여 사용한다.
// 정렬은 유한 좌표만 지원한다. 완전 동률도 판본별 원본 CRT의 피벗/교환 순서로 처리한다.
std::int32_t DrawVisibleConstruction(o::ConstructionVisualSystem& system,o::RenderWorldRect view,
    std::span<o::RenderCollectionEntry> work,bool sort=true);
// 실제 건설 수집/토큰 Draw를 공통 목록에 연결하고 외부 UI/묶음/자산/미리보기와 함께 정렬·분배한다.
// 두 hooks의 construction은 system으로 연결한다. 반환값의 backgroundCovered는 호출자가 배경 그리기에 사용한다.
// 전체 Client/GameWorld 프레임 루프·외부 수집기의 실제 자산/SHP/장치 연결은 아직 후속이다.
DrawCollectionResult DrawVisibleScene(o::ConstructionVisualSystem& system,const DrawCollectionRequest& request,
    std::span<o::RenderCollectionEntry> work,DrawCollectionHooks collection,
    const DrawDispatchState& state,DrawDispatchHooks dispatch,bool sort=true);
}
