// 동일한 원본 기본 프레임/빈 추가 Draw를 공유하는 유닛의 정확한VT를 판별한다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o {
bool IsPlainUnitCapture(std::span<const std::uint8_t> raw); //10.78의TYPE/VT 두 값이 모두 일치할 때만 공통 캡처를 사용한다.
}
