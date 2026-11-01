// 실제 메인 부모의 벽시각/현재 포인터·모달 조건을 원문 순서로 처리한다.
#pragma once
#include "client/GumpCreate.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // 공통 Gump 부모와 독립 원문 검사가 같은 활동 상태를 사용한다.
inline constexpr std::uint32_t kMainMenuActivityEvent=0x020000acU,kMainMenuDemoCommand=0x020000c0U; // 보호005429a4/005429f4의 초기 DWORD다.
struct MainMenuActivity { // 원문 MainMenu +110/+118/+11c의 현재 필드이며 GameClock 정지와 무관하다.
    double deadline{}; // 마지막 활동의 실제 벽시각+45초다.
    ScreenPoint pointer{}; // 마지막 재설정 때 복사한005cae94/98이며 생성 시 둘 다0이다.
};
struct MainMenuActivityHooks { // 현재 외부 효과만 빌리고 HTML/모달/포인터 사본을 소유하지 않는다.
    std::function<bool()> popup; //004641d0의 실제 루트 직계 bit4000 검색이다.
    std::function<double()> now; //00460d70의 현재 벽시각이다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> queue; //00452fe0의 두 독립 문자열 인자를 같은 원본 입력 큐에 넣는다.
    std::function<void()> kill; //00466480의 실제 부모 Kill이며 scalar 반납은 후반 Cleanup이다.
};
void IdleMainMenuActivity(MainMenuActivity& state,const std::uint32_t& enabled,const std::int32_t& htmlInstances,const ScreenPoint& pointer,const std::uint32_t& currentDialog,std::uint32_t self,const MainMenuActivityHooks& hooks); //004cee80의 lazy 관문·두 시각 조회·부분 효과 순서를 보존한다.
bool HandleMainMenuActivity(MainMenuActivity& state,std::uint32_t event,const std::function<double()>& now); //004cebf0의 첫 분기는 autoDemo 값과 무관하게 현재 기한을 갱신한다.
struct RawMainMenuHooks { // 실제 Client의 장기 옵션/HTML 인스턴스와 원문 문자열 큐 경계다.
    std::function<std::uint32_t()> enabled; //0054db90의 현재 DWORD이며 누락 옵션을0으로 덮지 않는다.
    std::function<std::int32_t()> htmlInstances; //005cab2c의 현재 signed DWORD이며 visible 창 개수와 구별한다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> queue; // 같은 InputQueue/80칸 문자열 고리를 사용한다.
    std::function<void()> clearStart; //00436e40의 시작 전체화면 상태 표식을 원문 부모 click 진입에서 한 번 지운다.
};
}
