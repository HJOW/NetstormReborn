// 원본 GetTechnology/CheatAllTechnology 명령을 같은 현재 캠페인의 실제 기술 내용물로 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <functional>
namespace netstorm::client {
inline constexpr std::uint32_t kGetTechnologyCommand=0x0200009fU; // 원문542970의 실제 기술 지급 입력이다.
// release는 현재 Session/효과를 요구하지 않고 press만492a30의 같은 DWORD 인자를 전달한다.
bool DispatchGetTechnologyInput(const InputEvent& event,const std::function<void(std::uint32_t)>& acquire);
class RawCampaignSession; // 실제 Player/TYPE/내용물 풀의 수명을 소유하는 기존 세션이다.
inline constexpr std::uint32_t kCheatAllTechnologyCommand=0x02000028U; // 보호 명령표00542798의 원본 코드다.
// 명시 owner 기술 getter를 거치지 않고 원본 현재 부모 getter와 같은 실제 contained 몸체를 사용한다.
void ApplyThemeTechnology(RawCampaignSession& session,std::int32_t theme);
}
