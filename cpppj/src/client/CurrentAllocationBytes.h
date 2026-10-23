// 기존CurrentAllocationBytes의본문과호출계약을그대로공유한다.기본0초기화나새Clientowner를추가하지않는다.
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>

namespace netstorm::client {
// 원본 생성자가 초기화하지 않는 바이트는 현재 CRT 할당의 실제 입력을 복사한다. 할당은 복사 뒤 즉시 반환한다.
inline void CurrentAllocationBytes(std::span<std::uint8_t> bytes) {
    if (bytes.empty()) return;
    void* allocation=std::malloc(bytes.size()); // 원본 malloc 계층처럼 zero 플래그 없는 실제 힙을 사용한다.
    if (!allocation) throw std::bad_alloc();
    std::memcpy(bytes.data(),allocation,bytes.size());std::free(allocation);
}
}
