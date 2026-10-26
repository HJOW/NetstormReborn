// 원문004b6dd0의 State3 선택 부모를 같은 Native 블록/현재 장치로 실행한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { //호스트 메뉴 표시와 Native State 공통 확정을 분리한다.
// 실제 Client가 장기 소유하는 각 전역/이름 물리의 참조다. 이 부모는 별도 복원값을 소유하지 않는다.
struct NativeMainMenuStateGlobals {
    o::LocalMissionStateStorage state; //같은 다섯144BYTE이며 현재 State3/이전phase를 원문 위치마다 읽는다.
    std::uint32_t& sourceType;std::uint32_t& outcome;std::uint32_t& firstMenu; //0055a420/005c85ac/00542530의 현재 DWORD다.005c8754는 같은 state.immediate[35]를 직접 쓴다.
    const bool& battle;std::uint32_t& editor; //00594fbc/005c85a4이며 prefix와 Tell 뒤 현재값을 읽는다.
    std::span<char> background; //00565e70의 현재 NUL 문자열 물리이며 첫BYTE 쓰기와 전체 이름 복사를 구별한다.
    std::span<std::uint8_t> currentName,pendingName; //0055b490/494의 실제 할당이다. pending NULL은 빈span으로 표현한다.
    std::function<std::uint32_t()> userUpdated,tellTips; //0054da8c/0053190c의 현재 직접 읽기이며 Config 재조회로 바꾸지 않는다.
};
// 선택된 큰 하위만 필수로 요구한다. 공통 prefix/tail은 기존 실제 부모를 호출한다.
struct NativeMainMenuStateHooks {
    std::function<std::int32_t(std::string_view)> configInt; //intro와 WelcomeGiven의 같은 ConfigInt다.
    std::function<void(std::uint32_t)> request; //004b2bc0의 실제 immediate 예약이며 phase를 직접 쓰지 않는다.
    std::function<void()> deleteSavedState; //승패/소스 배경이 선택한004b5470→DeleteFile 경계다.
    std::function<void(std::string_view)> requestMusic; //00469db0의 현재 음악 요청이다.
    std::function<std::optional<std::string>(std::uint32_t)> sourceBackground; //현재 TYPE 이름/.gif 실파일 존재를 판정하여 존재할 때만 원래 이름을 반환한다.
    std::function<void()> leaveBattle,prefix,hudPrefix; //원문의 이전 Battle14 통지/전체 공통준비/첫 HUD1이다.
    std::function<void()> title,closeGuide; //현재/pending 첫BYTE0 뒤 제목 갱신과 그 뒤 Guide 소멸이다.
    std::function<void(std::string_view)> tell; //004cf960의 실제 고정 Tell 섹션이며 임시 Host MainMenu를 호출하지 않는다.
    std::function<std::uint32_t()> startup; //004b5ba0의 실제 reader/벽시각/질문 부모다.
    std::function<void()> editorPlayers,tail,changedState; //Tell 뒤 실제 Editor 분기/공통 후반/제목 callback이 State를 바꾼 경우의 원문 다른 경로다.
};
// 모든 선택 효과가 정상 복귀한 경우만 공통 commit으로 반환한다. intro0은 실제 immediate39를 예약한다.
o::LocalMissionParentResult InstallNativeMainMenuState(NativeMainMenuStateGlobals globals,const NativeMainMenuStateHooks& hooks);
}
