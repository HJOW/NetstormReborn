// 실제439de0의 raw56BYTE×256 기록을 소유하고 최초 명도 double을 재계산 없이 빌린다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 실제 ColorSort 항목 수/항목 크기/전체 할당의 BYTE 수다.
inline constexpr std::size_t kColorSortPrimaryColors=256,kColorSortPrimaryRecordBytes=56,
    kColorSortPrimaryBytes=kColorSortPrimaryColors*kColorSortPrimaryRecordBytes;
// 원문에 없는 장치 성공을 추가하지 않는 선택적 관측 중단 경계다. 입력/기록 수정은 지원하지 않는다.
struct ColorSortPrimaryHooks {
    // 진입 때 (0,0,0), 각 store 완료 뒤 (누적수,offset,size)를 받는다. 예외는 부분 BYTE를 그대로 남긴다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> checkpoint;
};
// raw0x3800 자체와 host 준비 표식만 소유한다. 부모 primary DWORD/할당·검색객체 수명은 별도다.
class RawColorSortPrimary {
public:
    // 원본 malloc 직후의 실제 초기0x3800BYTE를 필수로 받는다. 미작성padding을 임의0으로 채우지 않는다.
    explicit RawColorSortPrimary(std::span<const std::uint8_t> allocationBytes);
    RawColorSortPrimary(const RawColorSortPrimary&)=delete; // 이미 빌린 raw 주소를 유지한다.
    RawColorSortPrimary& operator=(const RawColorSortPrimary&)=delete; // 서로 다른 primary의 암묵 사본을 만들지 않는다.
    RawColorSortPrimary(RawColorSortPrimary&&)=delete; // Client 수명의 고정 입력 주소를 이동하지 않는다.
    RawColorSortPrimary& operator=(RawColorSortPrimary&&)=delete; // 이미 반환한 BYTE view를 이전 객체에 남기지 않는다.
    // 기본53비트/nearest의439de0만 지원한다. 첫1024BYTE RGBX를 현재 읽으며 raw와의 실제 alias를 보존한다.
    void Construct(std::span<const std::uint8_t> logicalRgbx,const ColorSortPrimaryHooks& hooks={});
    std::span<std::uint8_t,kColorSortPrimaryBytes> Bytes(); // 실제 초기 할당·alias·부분효과를 위한 같은 raw 저장소다.
    std::span<const std::uint8_t,kColorSortPrimaryBytes> Bytes() const; // 기록 번호/미작성padding을 포함해 원문 물리를 빌린다.
    bool Constructed() const; // host 표식이며 raw payload에 추가 DWORD를 쓰지 않는다.
    double Lightness(std::size_t index) const; // 완료한 같은 primary+index*56+8의 double 비트를 읽는다. RGB 재계산이 아니다.
private:
    // host 타입 별칭 없이 원문 기록의 double을 현재 BYTE에서 복사해 읽는다.
    double ReadDouble(std::size_t offset) const;
    // 원문8BYTE store 하나를 완료한 뒤 현재 관측 경계를 호출한다.
    void StoreDouble(std::size_t offset,double value,std::uint32_t& writes,const ColorSortPrimaryHooks& hooks);
    // +30의 원래4BYTE 번호 store이며 인접 +34padding은 건드리지 않는다.
    void StoreIndex(std::size_t offset,std::uint32_t value,std::uint32_t& writes,const ColorSortPrimaryHooks& hooks);
    alignas(double) std::array<std::uint8_t,kColorSortPrimaryBytes> bytes_; // 정확0x3800 payload이며 생성자가 명시 초기 BYTE를 전부 복사한다.
    bool constructed_{}; // 부모가 성공 반환한 기록만 읽게 하는 host 표식이며 Native 필드로 세지 않는다.
};
}
