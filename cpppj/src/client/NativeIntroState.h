// 원본 10.78의 State39/40/41을 현재 Native 블록과 실제 영상 장치 경계로 실행한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 실제 Client의 인트로 준비와 공통 State 확정을 연결한다.
// 별도 상태 사본 없이 현재 전역과 pending Fort 이름의 물리를 빌린다.
struct NativeIntroStateGlobals {
    o::LocalMissionStateStorage state; // 다섯 144BYTE이며 current+8c가 인트로 준비 여부다.
    bool& fort;bool& server;bool& boss; // 00594fb8/00540bc0/00540bc4의 같은 실제 모드다.
    std::span<std::uint8_t> pendingName; // 0055b494가 가리키는 이름이며 State41은 첫BYTE만 지운다.
};
// 선택된 효과만 원문 위치에서 요구하며 영상 열기의 실패 반환도 정상 State40으로 잇는다.
struct NativeIntroStateHooks {
    std::function<void()> cleanup,hideChatInput,hideChat; // 004b2df0(0)/0042f270(0)/0042e440의 순서다.
    std::function<void()> core,renderer,hud; // Core(1,1)/Renderer(1)/HUD(3d,1,0,1)의 실제 장치다.
    std::function<std::string()> moviePath; // ConfigPath의 introMovie이며 빈 문자열도 보존한다.
    std::function<void()> markIntro; // 영상 이름이 있을 때만 ConfigSet(intro,1)을 호출한다.
    std::function<void(std::string_view)> openMovie; // 00484010의 실제 열기이며 원문은 성공 여부를 사용하지 않는다.
    std::function<bool()> movieActive; // 00483db0의 현재 MovieGump 존재 여부다.
    std::function<void(std::uint32_t)> request; // 같은 분배기의 immediate를 예약한다.
};
// 공통 commit은 RawLocalMissionState에 맡긴다. 39/40/41 외에는 장치 호출 전에 거부한다.
o::LocalMissionParentResult InstallNativeIntroState(NativeIntroStateGlobals globals,const NativeIntroStateHooks& hooks);
}
