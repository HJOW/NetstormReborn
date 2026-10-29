// 원본0042da00의 대상/본문 해석과0042d1b0의 실제 수신자 배열을 복원한다.
#pragma once
#include "o/RawPlayerIdentity.h"
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 같은 manager 물리와 현재 Identity/Chat 행을 빌리는 채팅 제출 부품이다.
inline constexpr std::size_t kChatSubmitWorkBytes=4097; // 인접한 본문2048/접두2048과 마지막 캡처 NUL의 실제 접근 폭이다.
// 메모리 getter는 동일 토큰의 순수 조회다. 선택한 장치만 해당 원문 위치에서 요구한다.
struct ChatSubmitHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; // manager와 수신자 할당의 같은 현재 물리다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 수신자 확장의 실제 BYTE 요청을 받아 비영 토큰 또는 실패0을 반환한다.
    std::function<void(std::uint32_t)> release; // 확장 복사 뒤 이전 수신자 할당을 실제로 반납한다.
    std::function<void(std::uint32_t)> assertion; // 새 할당 실패의 Chat.cpp274 진단이며 돌아오면 원문 접근을 계속한다.
    std::function<bool(o::PlayerIdentityRecord&)> resolve; // 같은60BYTE 지역 이름을 현재 Player/Chat 정보로 조회한다.
    std::function<std::string(std::string_view)> translate; // 실패한 Identity 다음의 여덟 채널 키를 현재 언어로 조회한다.
    std::function<std::span<const std::uint8_t>()> records; // 현재100×48BYTE Chat 기록을 읽는다.
    std::function<void(std::uint32_t)> inputVisible; // 빈 본문의0042f270(0)이며 수신자 갱신 뒤에만 실행한다.
    std::function<void(std::span<std::uint8_t>)> dispatch; // 비어 있지 않은 실제 본문을0043f6f0 분배에 넘긴다.
};
// 같은 manager의 중복 DWORD를 먼저 찾고, 필요하면32슬롯을 늘린 뒤 현재 count에 추가한다.
void AddChatRecipient(std::uint32_t manager,std::uint32_t subscriber,const ChatSubmitHooks& hooks);
// identity와 work는 명시 지역 입력이다. 초기화하지 않은 꼬리는 부모가 쓰는 범위 외에는 유지한다.
// work의 두2048BYTE 캡처는 인접하므로 본문 상한의 NUL을 접두 캡처가 덮는 원문 효과도 보존한다.
// 지원 물리 밖/NUL 없는 입력은 명시 예외다. 전체 Regexp/네트워크 전송을 대신하지 않는다.
void SubmitChatInput(std::uint32_t manager,std::string_view input,o::PlayerIdentityRecord& identity,
    std::span<std::uint8_t> work,const ChatSubmitHooks& hooks);
}
