// 원본0042e540의 장기 후보/접두/순환 상태를 실제 Chat 기록과 연결한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
namespace netstorm::client { //이름 자동완성은 Console의 단어 선택 상태와 별도로 Client 수명에 산다.
struct ChatNameCompletionState { //00548810..0054c89f의 실제 BSS 배치를 보존한다.
    std::array<std::uint8_t,0x4090> bytes{}; //index0/length4/prefix8/count88/unused8c/candidates90의 원시 상태다.
};
struct ChatNameCompletionHooks { //새 query일 때만 번역과 현재100행을 요구한다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 번역 바이트이며 후보 슬롯 폭으로 자르지 않는다.
    std::function<std::span<const std::uint8_t>()> records; //00558ed0..0055a18f의 실제 현재100×48BYTE다.
    std::function<void(std::uint32_t)> assertion; //길이127 이상인 원문의354 진단이며 복귀하면 복사를 계속한다.
};
//NULL query는 저장 후보의 다음/이전 순환이다. 새 query는 length BYTE만 strncpy하고 나머지 접두는 남긴다.
std::optional<std::string> CompleteChatName(ChatNameCompletionState& state,std::optional<std::string_view> query,std::int32_t length,bool backward,const ChatNameCompletionHooks& hooks);
}
