// 일반 기본 그림과 빈 추가 그림을 공유하는 다섯 워크샾 자산의 정확한 분배 조건이다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o {
// TYPE129/136/140/143/148과 원본VT를 함께 확인한다. 다른 파생/128의 별도 처리기는 보존한다.
bool IsPlainWorkshopCapture(std::span<const std::uint8_t> raw);
}
