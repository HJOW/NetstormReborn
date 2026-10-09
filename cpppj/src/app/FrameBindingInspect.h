// 실제 원본 자산을 읽기만 하는 프레임 표시 연결 콘솔 검사다. 창/원본 게임을 실행하지 않는다.
#pragma once
#include "o/RiftType.h"
#include <filesystem>

namespace netstorm::app {
// 판본별 SHP 메타데이터 전체와 실제 연결 객체의 프레임 변경/Renderer 영역 전달을 검사한다.
void InspectFrameBinding(const std::filesystem::path& root,o::OriginalEdition edition);
// 실제 배치 메타 자료와 사제 기본 프레임의 픽셀 범위를 JSON으로 내보낸다. GUI는 만들지 않는다.
void InspectPriestAssets(const std::filesystem::path& root,o::OriginalEdition edition);
// 실제 자산과 현재 타입 전역으로 모든 패턴/짝수 방향을 콘솔에서 순회한다.
void InspectCanonPatterns(const std::filesystem::path& root,o::OriginalEdition edition);
}
