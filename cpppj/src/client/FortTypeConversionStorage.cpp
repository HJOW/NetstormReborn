// 현재 포인터 대입/해제 순서는 원본 부모가 소유하고 이 계층은 주소와 할당 수명만 제공한다.
#include "client/FortTypeConversionStorage.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 같은 Client 전역의 수정 가능한 참조다.
std::uint32_t& FortTypeConversionStorage::Current() { return current_; }
// 읽기 소비자도 별도 캐시 없이 같은 DWORD 주소를 사용한다.
const std::uint32_t& FortTypeConversionStorage::Current() const { return current_; }
// 메모리를 미리0으로 채우지 않고 원본 로더의 실제 DWORD/WORD/BYTE zero에 맡긴다.
std::uint32_t FortTypeConversionStorage::Allocate(std::uint32_t bytes) {
    if (bytes!=kFortTypeConversionBytes) throw std::invalid_argument("Fort 변환표의 실제255BYTE 할당 폭");
    if (nextToken_==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Fort 변환표 수명 토큰 소진");
    const auto token=nextToken_+1U; // 할당 성공 전에는 현재 전역이나 발급 상태를 바꾸지 않는다.
    Allocation allocation{std::unique_ptr<std::uint8_t[]>(new std::uint8_t[bytes]),bytes}; // 본문이 초기화할 실제 미초기화 블록이다.
    allocations_.emplace(token,std::move(allocation));nextToken_=token;return token;
}
// 원문이 보관한 이전 할당도 현재 포인터 값과 별개로 해석한다.
std::span<std::uint8_t> FortTypeConversionStorage::Bytes(std::uint32_t token) {
    const auto found=allocations_.find(token); // 실제로 소유한 주소만 빌려 물리 경계를 유지한다.
    if (!token || found==allocations_.end()) throw std::out_of_range("Fort 변환표의 해제되거나 없는 실제 주소");
    return {found->second.bytes.get(),found->second.size};
}
// free(nullptr)는 허용하지만 반납 이후의 현재 전역 쓰기를 대신 수행하지 않는다.
void FortTypeConversionStorage::Release(std::uint32_t token) {
    if (!token) return;
    if (!allocations_.erase(token)) throw std::out_of_range("Fort 변환표의 이미 반납한 실제 주소");
}
}
