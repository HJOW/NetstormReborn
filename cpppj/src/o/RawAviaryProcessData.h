// Sun/Rain 아비어리가 공유하는0045b0f0의 선택 필드 decoder다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o { // 현재 outer 이미지와 패킷만 빌리며 별도 예약/Kernel 객체를 만들지 않는다.
std::uint32_t ReadAviaryProcessData(std::span<std::uint8_t> fields,std::span<const std::uint8_t> packet); // 부모DWORD·dirty2의16..23/dirty4의24..27만 처리한다. 빈 fields도 실제 저장이 없으면 Native NULL this처럼 반환한다.
}
