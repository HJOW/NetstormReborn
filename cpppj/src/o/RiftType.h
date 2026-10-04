// 원본 RiftType.cpp의 4바이트 프레임 코드와 검색 함수 복원.
#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace netstorm::o {
struct FrameCode {
    std::uint8_t side{}; // 원본 코드 +0, 방향 문자.
    std::uint8_t variant{}; // 원본 코드 +1, 변형 문자.
    std::uint8_t number{}; // 원본 코드 +2, 8비트 프레임 번호.
    std::uint8_t flags{}; // 원본 코드 +3. 검색에서 MSVC signed char로 부호 확장한다.
};
// 원본 배열 원소는 정확히 4바이트다. 게임 객체 자체의 32비트 포인터 배치는 복제하지 않는다.
static_assert(sizeof(FrameCode) == 4);

class RiftTypeFrames {
public:
    // 원본 type +0x114(개수), +0x124(코드 포인터)를 소유권 있는 배열로 바꾼다.
    explicit RiftTypeFrames(std::vector<FrameCode> frames);
    // 번호까지 같은 첫 프레임. 실패는 -1. 패치 0049a9a0 ↔ CD 00444350.
    int FindNumber(std::uint8_t side, std::uint8_t variant, std::uint8_t number) const;
    // 번호와 (부호 확장 플래그 & 마스크)가 맞는 첫 프레임. 패치 0049a9e0 ↔ CD 00444410.
    int FindMasked(std::uint8_t side, std::uint8_t variant, std::uint8_t number, std::uint32_t mask) const;
    // 방향·변형·플래그가 정확히 같은 첫 프레임. 패치 0049aa30 ↔ CD 00444460.
    int FindFlags(std::uint8_t side, std::uint8_t variant, std::int32_t flags) const;
    // 코드 배열을 검증·직렬화 도구에 공개한다.
    std::span<const FrameCode> Codes() const;
private:
    std::vector<FrameCode> frames_;
};
}
