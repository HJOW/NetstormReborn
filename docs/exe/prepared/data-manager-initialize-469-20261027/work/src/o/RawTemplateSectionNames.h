// 원문004bccb0의 Client전역005c8aa4 lazy표 생성만 복원한다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
namespace netstorm::o { // 원문 token과 실제512BYTE owner를 분리한다.
inline constexpr std::uint32_t kTemplateSectionNamesCapacity=512; // 실제operator new의 크기다.
inline constexpr std::size_t kTemplateSectionNamesWrittenBytes=277; // 정상 신규표에서 실제 쓰는 접두부와 마지막NUL까지다.
struct TemplateSectionNamesHooks { // 새 할당의 미정 BYTE는 allocator가 소유하며 이 부모는 초기화하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 실제512BYTE의 불투명 token/0이다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> buffer; // 현재 token의같은물리범위이며조회자체는무효과다.0을정상표로만들지않는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> format; //004a89c0(local256,"Terr%02d",index)의같은NUL포함출력이다.반환뒤현재전역표를다시조회한다.
};
std::uint32_t GetTemplateSectionNames(std::uint32_t& current,const TemplateSectionNamesHooks& hooks); // 최초할당→현재전역공개→277BYTE쓰기이며 비영현재값이면 하위없이 반환한다.
}
