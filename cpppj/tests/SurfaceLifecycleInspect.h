// 선택적인 원본 자산 검사는 일반 CTest와 분리하며 게임 실행 없이 읽기만 한다.
#pragma once
#include "o/RiftType.h"
#include <filesystem>

namespace netstorm::test {
// 두 판본의 실제 타입·SHP를 같은 Graph 활성 통합 검사에 공급한다.
void InspectSurfaceLifecycle(const std::filesystem::path& root,o::OriginalEdition edition);
}
