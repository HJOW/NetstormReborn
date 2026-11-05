// 월드 설치 전후의 UI도 같은 현재 TYPE HP getter를 사용할 수 있게 연결한다.
#pragma once
#include "o/RawRiftHitPoints.h"

namespace netstorm::client { // 원문 TYPE base/전역의 수명은 호출자가 Reader보다 오래 유지한다.
std::function<std::int32_t(std::uint32_t)> MakeCurrentTypeHitPoints(o::OriginalEdition edition,std::function<std::span<const o::RiftTypeRecord>()> currentTypes,o::RiftHitPointGlobals globals,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion); // 진단 뒤 현재 표 기준을 읽되 처음 선택한 TYPE this는 보존한다.
}
