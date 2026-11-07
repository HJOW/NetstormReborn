// 로컬 로그인과 Root의 원격 요약이 같은 실제 CCh 참가자 생성자를 사용한다.
#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::root { //0044c7b0의 순수 기록/CRT 시계 경계를 공유한다.
void InitializeParticipant(std::span<std::uint8_t> player,std::uint32_t id,std::uint32_t subscriber,std::optional<std::string_view> name,std::span<const std::uint8_t,6> identity,std::span<const std::uint8_t,10> core,std::span<const std::uint8_t,8> position,std::uint32_t flags); // NULL 이름은 선두NUL만 쓰고 현재 나머지 바이트를 보존한다. 모든 입력은 호출 중 실제 바이트를 빌린다.
}
