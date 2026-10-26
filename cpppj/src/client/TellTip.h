// 원본 Tips 버튼의 현재 번호 변경·설정·다음 안내 선택을 같은 Client 전역에 적용한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { // 첫 메뉴의 실제 Tips HTML 명령을 일반 입력 큐 뒤에서 처리한다.
inline constexpr std::uint32_t kTellTipCommand=0x020000ebU; // 보호 PE00542aa0의 TellTip 초기 명령값이다.
// 지역 정수 서식/Config/Tell 장치만 제공하며 창 Kill/OnExit는 기존 HTML 부모가 소유한다.
struct TellTipHooks {
    std::function<std::string(std::int32_t)> format; //004a89c0의 실제 %d 서식이며 현재 signed DWORD를 받는다.
    std::function<void(std::string_view,std::string_view)> setConfig; //004414d0("tipNumber",문자열)의 실제 설정 장치다.
    std::function<void(std::string_view)> tell; //y!=3일 때만 "Tips"를 선택한다. OK3에 Blank를 덧붙이지 않는다.
};
// 004d6fc9..004d7044의 선택 부모다. release/다른 명령은 번호·하위 장치를 실행하지 않는다.
bool DispatchTellTip(const InputEvent& event,std::uint32_t& tipNumber,const TellTipHooks& hooks);
}
