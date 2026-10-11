// 원본 CRT의 판본별 16바이트 표시 목록 정렬을 복원한다. 동일 키도 원본 교환 순서를 사용한다.
#pragma once
#include "o/RawRenderCollection.h"

namespace netstorm::client {
struct RenderSprite;
// 원본 목록의 전체 16바이트 항목을 제자리 정렬한다. 키는 signed 깊이 내림차순/y·x 오름차순이다.
// 패치의 중앙값/피벗 이동과 CD의 첫 칸 피벗을 구별한다. 같은 키를 비교해도 1인 원본 비교를 그대로 사용한다.
// 유한 x/y만 받으며 잘못된 키/32비트 원본 범위를 넘는 개수는 변경 전에 거부한다. 게임/OS/CRT 함수는 호출하지 않는다.
void SortDrawEntries(std::span<o::RenderCollectionEntry> entries,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
// 실제 SHP 스프라이트의 키/원래 인덱스로 같은 목록을 만들고 모든 스프라이트 필드를 함께 옮긴다.
// 항목은 자산/이미지를 소유하지 않는다. 원본 포인터 대신 인덱스를 사용하며 종류는 비교 키에 참여하지 않는다.
void SortRenderSprites(std::vector<RenderSprite>& sprites,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
}
