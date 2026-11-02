// 원문 세 일반 명령을 기존 세계 간헐천 생성과 실제 AI 직접 상태 writer에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // Guide/Gump/뗌 뒤이며 Console/공통 정지보다 앞이다.
inline constexpr std::uint32_t kCreateInitialGeysersCommand=0x020000adU,kBrainCrankMoneyCommand=0x020000c1U,kAiParamSetCommand=0x02000078U; //5429a8/5429f8/5428d8의 원문 초기DWORD다.
std::uint32_t AiUtilityCommandScriptCode(std::string_view name); // 실제 세 이름만 ASCII 대소문자를 무시하고 조회한다.
bool IsAiUtilityCommand(std::uint32_t code); // 같은 이름·HTML 허용표와 일반 입력이 정규화된 명령값을 공유한다.
struct AiUtilityCommandHooks { // 선택한 분기에 도달한 실제 부모만 필수로 요구한다.
    std::function<void()> initialGeysers; //00486440의 기존 전체 GenerateWorldGeysers이며 local 생성 하나로 줄이지 않는다.
    std::function<void()> crankMoney; //00413c60의 현재 Player1..8과 실제 brain+2d8 직접 쓰기다.
    std::function<void(std::uint32_t)> parameter; //004143b0에 param2 전체DWORD를 전달하며 low3 owner를 바꾸지 않는다.
};
bool DispatchAiUtilityCommandInput(InputEvent event,const AiUtilityCommandHooks& hooks); // 대상 뗌은 효과 없이 소비하고 다른 명령은false다. 모드/정지/owner를 새 관문으로 읽지 않는다.
}
