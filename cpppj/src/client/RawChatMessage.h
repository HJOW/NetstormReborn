// 원본 일반 본문의 채널/대상 아이콘·이름/스타일 조합과 say/wrapper 호출 순서를 복원한다.
#pragma once
#include "client/RawChatFormat.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::client { // 패킷 작성과 분리된 원문0042f1e0/0042dc30/0042d640 부모다.
inline constexpr std::uint32_t kChatMessageOffset=0x298; // manager2712BYTE 안의2048BYTE 본문 출력 시작이다.
inline constexpr std::uint32_t kChatMessageNameBytes=256; //0042d640의 실제 대상 이름 지역 버퍼 폭이다.
struct ChatMessageGlobals { // 현재 슬롯은 원문이 사용하는 위치에서 다시 읽는다.
    const std::uint32_t& current; //00548714의 현재 manager이며 say의 선행 관문과 wrapper 호출에 쓴다.
    const std::uint32_t& subscriber; //00540c74의 현재 subscriber이며 첫 비자기 수신자를 찾을 때 읽는다.
};
void WriteChatNameFormat(std::span<std::uint8_t> output,std::string_view pattern,std::string_view name,std::string_view other); // 실제 세 고정 %s 서식만 NUL 포함 작성하며 이름 안의 %s를 다시 해석하지 않는다.
struct ChatMessageHooks { // 조회/CRT와 최종0042d280 전달은 같은 동기 호출 동안만 빌린다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; // manager/수신자 배열의 같은 실제 물리이며 조회 자체는 순수해야 한다.
    std::function<std::string_view()> currentName; // 현재00540ca8 포인터의 C문자열이며 빈 값은 ++Nameless++로 바뀐다.
    std::function<std::uint32_t(std::span<std::uint8_t>,std::uint32_t)> nameForSubscriber; //00484410처럼 현재 이름 또는 SOMEONE을 쓰고 목적 포인터의0/비영 토큰을 반환한다. 조회 성공 여부가 아니다.
    std::function<void(std::span<std::uint8_t>,std::string_view,std::string_view,std::string_view)> formatName{WriteChatNameFormat}; // 같은 목적 위치에 세 원문 CRT 서식을 작성한다.
    std::function<void(std::span<std::uint8_t>)> stripSlots{StripChatFormatSlots}; //0042d160의 실제 제자리 제거 본체다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> deliver; // 출력 this/대상 manager/NUL 포함 현재 본문을0042d280에 넘긴다. 패킷/전송을 빈 성공으로 대체하지 않는다.
    std::function<void(std::uint32_t)> inputVisible; // say 빈 본문의0042f270(0)이다.
};
std::uint32_t FormatChatMessage(std::uint32_t manager,std::uint32_t recipients,std::span<const std::uint8_t> body,
    std::optional<std::string_view> pattern,std::span<std::uint8_t> nameWork,ChatMessageGlobals globals,const ChatMessageHooks& hooks); //0042d640: 본문 빈 관문→현재 채널/대상·back→이름→현재 front/본문→슬롯 제거→전달 순서다.
std::uint32_t FormatChatInput(std::uint32_t manager,std::span<const std::uint8_t> input,std::span<std::uint8_t> nameWork,
    ChatMessageGlobals globals,const ChatMessageHooks& hooks); //0042dc30: 선행!은 이름+space, 직접 대상은 to 서식, 나머지는 일반 서식으로 같은 manager를 전달한다.
std::uint32_t SayChatMessage(std::span<const std::uint8_t> input,std::span<std::uint8_t> nameWork,
    ChatMessageGlobals globals,const ChatMessageHooks& hooks); //0042f1e0: 현재 manager0/앞 space/빈 본문 관문을 지키고 하위 전달값과 무관하게0을 반환한다.
}
