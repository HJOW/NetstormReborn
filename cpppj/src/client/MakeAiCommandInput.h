// 원문 MakeAi 이름과 Editor OR missionPost 일반 입력 관문을 실제 현재 부모에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // Guide/Gump/뗌 뒤, Console/공통 정지보다 앞인 원문 명령이다.
inline constexpr std::uint32_t kMakeAiCommand=0x020000b7U; //005429d0의 실제 초기 명령DWORD다.
std::uint32_t MakeAiCommandScriptCode(std::string_view name); //ASCII 대소문자를 무시하는 같은 이름표다.
bool IsMakeAiCommand(std::uint32_t code); //분류 전에 수식 비트를 제거한 정확한 명령값만 받는다.
struct MakeAiCommandHooks { // 선택 분기에서 현재 모드를 읽고 원문param2 전체를 전달한다.
    std::function<std::uint32_t()> editor,missionPost; //Editor 비영이면missionPost는 읽지 않는 short circuit다.
    std::function<void(std::uint32_t)> makeAi; //417fa0의 실제 부모이며 자체battle 관문과 현재AI 수명을 유지한다.
};
bool DispatchMakeAiCommandInput(InputEvent event,const MakeAiCommandHooks& hooks); //release는 효과 없이 소비하고 다른 명령은false다.
}
