// State19의 실제 준비·CS 재사용·다음 상태 선택과 Fort/Core/UI 꼬리를 복원한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 원문의 현재 Client 저장소를 빌리는 큰 메타 진입 부모다.
class ClientChallengeGlobals; // 같은 CS 포인터/실제 할당/배치 좌표를 소유한다.
struct NativeMetaStateGlobals { // 다섯 상태 블록과 서로 다른 Native 전역의 실제 참조다.
    o::LocalMissionStateStorage state; // current19와 immediate+40을 같은144BYTE에서 읽고 쓴다.
    std::span<char> background; //00565e70의 첫BYTE만 지우며 뒤 문자열은 보존한다.
    std::uint32_t& cdPresent; //005c85bc이며 State7의005c85b8과 별개인 CD 결과다.
    bool& metamap;bool& server;bool& boss; //00594fc8/00540bc0/00540bc4의 현재 모드다.
    std::uint32_t& pending; //005c85e8의 현재 DWORD이며 renderer 다음에0을 쓴다.
    o::PlayerWorldState& player; //현재 templateOwner/PlayerKnown을 같은 원문 위치에서 조회한다.
    const std::uint32_t& hudCommand; //005427ec의 현재 명령을 HUD 호출 직전에 읽는다.
    double& activityEnd; //00544d58의 실제 주변음 기한이며 게임시각과 구별한다.
    ClientChallengeGlobals& challenge; //1053의 같은 Client CS 생성/재사용 장치다.
};
struct NativeMetaStateHooks { // 원문에 도달한 하위만 요구하고 없는 장치를 빈 성공으로 바꾸지 않는다.
    std::function<std::string()> fortPalette; //ConfigPath fortPal의 현재 결과 문자열이다.
    std::function<void(std::string_view)> applyPalette; //004b6100의 실제 팔레트 교체다.
    std::function<void(std::uint32_t)> cleanup; //004b2df0(0)의 실제 World/Player/연결 정리다.
    std::function<std::uint32_t()> checkCd; //00440c60의 실제 결과를 별도005c85bc에 쓴다.
    std::function<void()> initializeRenderer; //현재0059a8bc 장치의004993c0이다.
    std::function<void(std::uint32_t)> assertion; //CS->listsInitialized의 State.cpp1322 진단이며 복귀 뒤 계속한다.
    std::function<void(std::uint32_t)> request; //004b2bc0의 실제 즉시 요청이며 공통144BYTE 확정은 외부에 남긴다.
    std::function<void(std::uint32_t,std::uint32_t)> selectCore; //0048f3b0의 owner1/templateOwner1이다.
    std::function<std::string()> currentFortPath; //00460680의 현재 Fort 경로다.
    std::function<void(std::uint32_t,std::string_view)> loadFort; //004bf550의 현재 templateOwner/실제 파일 읽기다.
    std::function<void(std::uint32_t,std::uint32_t)> loadCore; //004beca0의 현재 templateOwner/최소 병합0이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> hud; //00466280(command,4,0,1)의 실제 자식 우선 분배다.
    std::function<double()> wallSeconds; //004180d0의 현재 벽시각 getter다.
    std::function<void()> loadNewsTime; //0042e4d0의 현재 서버별 설정 읽기다.
    std::function<void(std::uint32_t)> battleChannel; //0042f3d0(0)의 실제 BATTLE 접두/채널 갱신이다.
    std::function<void(std::uint32_t)> inputVisible; //0042f270(1)의 현재 Chat 입력줄 표시다.
};
o::LocalMissionParentResult InstallNativeMetaState(NativeMetaStateGlobals globals,const NativeMetaStateHooks& hooks); //004b6dd0 case19의 전체 몸체다. 요청25/34/21 소비자와 실제 장치는 호출자가 연결해야 한다.
}
