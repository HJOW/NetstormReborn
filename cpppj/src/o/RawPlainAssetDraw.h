// 원본 TYPE 기본 프레임/빈 추가 그림을 공유하는 자산의 실제 Draw 분배 조건이다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o {
// 검증한 TYPE/VT 쌍만 일반 Draw로 연결한다. 다른 파생의 전용 그림을 생략하지 않는다.
bool IsPlainAssetDraw(std::span<const std::uint8_t> raw);
}
