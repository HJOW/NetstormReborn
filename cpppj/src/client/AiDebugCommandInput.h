// 실제413d50의현재brain reporting 토글과알려진진단DWORD주소해석을공유한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 원문임의32비트포인터를host주소로역참조하지않는다.
inline constexpr std::uint32_t kAiReportingCommand=0x020000d0U,kDebugSwitchCommand=0x020000d1U; //00542a34/38의보호PE초기값이다.
struct AiDebugInputState {
    std::function<std::uint32_t()> metamap; //AIReporting만현재MP관문을읽으며양성새검사는하지않는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> player; //현재owner의Player1620BYTE중한행이다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> body; //같은생존brain토큰의본문이며새사본을만들지않는다.
    std::function<std::uint32_t(std::uint32_t)> readDebug; //알려진안정전역의현재DWORD를주소식별자로읽는다.
    std::function<void(std::uint32_t,std::uint32_t)> writeDebug; //동일주소식별자의실제저장소에원시29/99를쓴다.
};
std::uint32_t AiDebugCommandScriptCode(std::string_view name); //실제등록두이름만ASCII대소문자무시로조회한다.
bool IsAiDebugCommand(std::uint32_t code); //원래event/수식비트포함값도동일두명령으로분류한다.
void ApplyAiDebugCommandState(InputEvent event,const AiDebugInputState& state); //413d50rawchild이며release거부는앞공통입력부모의책임이다.
bool DispatchAiDebugCommandInput(InputEvent event,const AiDebugInputState& state); //일반Guide/Gump뒤공통release를보존하는현재클라이언트접합부다.
}
