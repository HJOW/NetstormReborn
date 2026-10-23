// 원본 현재 conversion DWORD와 실제255BYTE 할당을 Client 수명에 유지하는 호스트 소유자다.
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <span>

namespace netstorm::client {
inline constexpr std::uint32_t kFortTypeConversionBytes=255; // 원본004bd800의 실제 operator new 요청 폭이다.
// 포인터를 DWORD로 자르지 않고 비영 토큰을 같은 실제 할당 주소에 대응시킨다.
class FortTypeConversionStorage {
public:
    FortTypeConversionStorage()=default; // 최초 current0이며 아직 원본 할당이나 초기화는 수행하지 않는다.
    FortTypeConversionStorage(const FortTypeConversionStorage&)=delete; // 이미 빌린 할당과 전역 주소의 복제를 막는다.
    FortTypeConversionStorage& operator=(const FortTypeConversionStorage&)=delete; // 실제 current DWORD의 소유자를 바꾸지 않는다.
    FortTypeConversionStorage(FortTypeConversionStorage&&)=delete; // 외부에서 빌린 DWORD 참조를 유지한다.
    FortTypeConversionStorage& operator=(FortTypeConversionStorage&&)=delete; // 할당/참조 수명을 이동으로 끊지 않는다.
    std::uint32_t& Current(); // TypeNames·Deck·Technology가 같은 현재 DWORD를 직접 읽고 쓴다.
    const std::uint32_t& Current() const; // 같은 현재 DWORD를 복제 없이 조회한다.
    std::uint32_t Allocate(std::uint32_t bytes); // 실제255BYTE를 할당만 하고 본문의 zero나 current 대입을 앞당기지 않는다.
    std::span<std::uint8_t> Bytes(std::uint32_t token); // 해제되지 않은 같은 물리 할당을 빌리며 없는 주소는 명시 거부한다.
    void Release(std::uint32_t token); // 지정 할당만 반납하고 current0 쓰기는 원문 부모가 별도로 수행한다.
private:
    // 발급한 한 토큰의 실제 힙 주소와 물리 폭을 함께 보존한다.
    struct Allocation {
        std::unique_ptr<std::uint8_t[]> bytes; // 실제 힙 블록이며 원문 로더가 초기화하기 전에는 읽지 않는다.
        std::uint32_t size; // 물리 접근 경계를 확인하는 실제 할당 폭이다.
    };
    std::uint32_t current_{},nextToken_{}; // 최초 BSS0과 호스트 토큰 발급 상태이며 서로 다른 값이다.
    std::map<std::uint32_t,Allocation> allocations_; // 빈 TypeNames가 덮어쓴 옛 포인터도 Client 반납까지 실제 할당을 유지한다.
};
}
