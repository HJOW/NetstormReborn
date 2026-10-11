// 복원한 가시 표시 수집을 공유 Renderer 정렬 비교와 실제 표시 Draw에 연결하는 클라이언트 어댑터다.
#pragma once
#include "o/ConstructionVisualSystem.h"

namespace netstorm::client {
// 월드 영역의 가시 건설 표시를 작업 버퍼에 수집하고 선택적으로 깊이/y/x 정렬한 뒤 제출한다.
// 반환값은 수집 항목 수다. 공간이 모자라면 원본처럼 먼저 등록된 항목으로 제한하며 잘린 항목은 정렬에 참여하지 않는다.
// 작업 버퍼/월드/팔레트는 호출 동안 살아야 한다. 자산/구름/UI와의 공통 정렬 및 실제 프레임/장치 연결은 후속이다.
// 정렬은 유한 좌표만 지원한다. 완전 동률은 호스트의 안정 순서이며 원본 CRT qsort의 동률 순서를 보장하지 않는다.
std::int32_t DrawVisibleConstruction(o::ConstructionVisualSystem& system,o::RenderWorldRect view,
    std::span<o::RenderCollectionEntry> work,bool sort=true);
}
