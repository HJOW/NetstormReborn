// 원본 format/back/front의 고정 스타일 검색·공통 캡처·manager/Config 쓰기를 복원한다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 현재 Client의 원문 전역과 같은 캡처 물리를 빌리는 서식 명령 계층이다.
inline constexpr std::size_t kChatFormatCaptureBytes=81; //00495a10의 최대80BYTE와 끝 NUL이 접근하는 공통 캡처의 앞부분이다.
struct ChatFormatGlobals { // 외부 callback 뒤 원문이 사용하는 시점에 같은 현재 슬롯을 다시 읽는다.
    std::uint32_t& current; //00548714의 현재 manager 토큰이다.
    const std::uint32_t& subscriber; //00540c74의 현재 signed 설정 키 ID다.
};
void StripChatFormatSlots(std::span<std::uint8_t> text); //0042d160처럼 ~0~6 두 바이트를 순방향 제자리 제거하고 새 NUL 뒤 꼬리를 보존한다.
struct ChatFormatHooks { // 선택된 원문 경계만 요구하며 진단·복사·설정의 부분효과를 보존한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; // 토큰의 실제 manager 물리이며 순수 조회여야 한다.
    std::function<void(std::uint32_t)> assertion; // manager0의 Chat.cpp157/184 진단이다. 복귀하면 현재 전역으로 계속한다.
    std::function<void(std::span<std::uint8_t>)> stripSlots{StripChatFormatSlots}; // 실제0042d160 본체를 기본 사용하며 같은 캡처 물리의 호출 관측도 허용한다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view,std::uint32_t)> copyFormat; // 원문 strncpy의 목적 토큰/offset/공통 캡처/60 또는500BYTE다.
    std::function<void(std::string_view,std::string_view)> writeConfig; //004414d0의 지역 키와 같은 현재 공통 캡처 문자열이다.
    std::function<void(std::string_view)> illegalFormat; // 검색 실패의 level29 Illegal format 진단이며 입력 전체를 전달한다.
};
std::uint32_t ApplyChatFormat(bool front,ChatFormatGlobals globals,std::string_view input,
    std::span<std::uint8_t> capture,const ChatFormatHooks& hooks); //0042ddf0/0042def0: 검색→80BYTE 캡처/정리→현재 manager 복사/강제NUL→현재 ID 키→Config 순서다.
}
