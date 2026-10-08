// 실제 원본 자산을 읽기만 하는 프레임 표시 연결 콘솔 검사다. 창/원본 게임을 실행하지 않는다.
#pragma once
#include "o/RiftType.h"
#include <filesystem>

namespace netstorm::app {
// 판본별 SHP 메타데이터 전체와 실제 연결 객체의 프레임 변경/Renderer 영역 전달을 검사한다.
void InspectFrameBinding(const std::filesystem::path& root,o::OriginalEdition edition);
}
