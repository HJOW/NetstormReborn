// 같은 Client의 원문 State 다섯 블록과 로그인/기한 전역을 메뉴/Editor/로그인이 함께 빌린다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <array>

namespace netstorm::client {
// Session 교체에서도 같은 State 블록과 로그인/메뉴의 현재 전역 주소를 유지한다.
class ClientLocalStateGlobals {
public:
    ClientLocalStateGlobals()=default; // 새 Client에서만 원문 BSS0과 명령표의 실제 초기값을 만든다.
    ClientLocalStateGlobals(const ClientLocalStateGlobals&)=delete; // 같은 블록을 빌리는 State/패킷 소비자에게 사본을 만들지 않는다.
    ClientLocalStateGlobals& operator=(const ClientLocalStateGlobals&)=delete; // 현재 사용 중인 블록 주소를 대입으로 바꾸지 않는다.
    ClientLocalStateGlobals(ClientLocalStateGlobals&&)=delete; // 장기 참조가 이전 객체에 남는 이동을 금지한다.
    ClientLocalStateGlobals& operator=(ClientLocalStateGlobals&&)=delete; // 실제 Client 수명 동안 주소를 고정한다.
    std::array<o::LocalMissionStateBlock,5>& Blocks() { return blocks_; } // old/phase/immediate/current/next 순서의 물리 배열 한 개다.
    const std::array<o::LocalMissionStateBlock,5>& Blocks() const { return blocks_; } // 진단도 같은720BYTE를 읽는다.
    o::LocalMissionStateStorage State() { return {blocks_[0],blocks_[1],blocks_[2],blocks_[3],blocks_[4]}; } // 기존 원문 분배기에 같은 블록 참조를 제공한다.
    double& ActivityEnd() { return activityEnd_; } // 기존 Login과 Editor가00544d58의 같은 벽시각 기한을 직접 쓴다.
    const double& ActivityEnd() const { return activityEnd_; } // 기한 사본이나 별도 Editor 타이머를 만들지 않는다.
    std::uint32_t& ZonePending() { return zonePending_; } // 005c85c0이며 실제 Fort 요청 작성과 State 정리가 같은 DWORD를 쓴다.
    const std::uint32_t& ZonePending() const { return zonePending_; } // 요청 캡처의 sequence와 구별하는 현재값이다.
    std::uint32_t& SourceType() { return sourceType_; } // 0055a420을 State와 Reply 수신이 직접 공유한다.
    const std::uint32_t& SourceType() const { return sourceType_; } // Session 밖에서도 마지막 값을 보존한다.
    bool& SavedGame() { return savedGame_; } // 005c85d4의 현재 정상0/1 모델을 로그인/State가 함께 빌린다.
    const bool& SavedGame() const { return savedGame_; } // 임의 DWORD 상위 비트 보존을 bool 참조로 가장하지 않는다.
    std::uint32_t& Begin() { return begin_; } // 005c7a74의 Login/Ready/타이머가 같은 시작 DWORD를 쓴다.
    const std::uint32_t& Begin() const { return begin_; } // 세션 반납은 시작값을 암묵 초기화하지 않는다.
    std::uint32_t& CentralCreated() { return centralCreated_; } // 005c85a0의 실제 metadata writer와 cleanup이 공유할 DWORD다.
    const std::uint32_t& CentralCreated() const { return centralCreated_; } // 현재 중앙섬 상태를 복제하지 않는다.
    double& CaptureEnd() { return captureEnd_; } // 005c8618의 Reply/Zone/LoginBegin 벽시각 기한이다.
    const double& CaptureEnd() const { return captureEnd_; } // 연결/로그인/활동 기한과 별개다.
    double& ConnectEnd() { return connectEnd_; } // 005c85f8의 State8/9 연결 만료 시각이다.
    const double& ConnectEnd() const { return connectEnd_; } // 시계 getter나 프레임 시각으로 대체하지 않는다.
    double& LoginEnd() { return loginEnd_; } // 005c85f0의 로그인/Dais/충돌 효과가 같은 double을 빌린다.
    const double& LoginEnd() const { return loginEnd_; } // 효과 억제 관문도 현재 double의 실제 주소를 사용한다.
    std::uint32_t& HudModeCommand() { return hudModeCommand_; } // 005427ec의 현재 명령을 호출 직전에 읽고 변경도 보존한다.
    std::uint32_t& AddIslandCommand() { return addIslandCommand_; } // 00542780의 현재 명령을 빈섬 예약 직전에 읽는다.
private:
    std::array<o::LocalMissionStateBlock,5> blocks_{}; //005c8630/8758/86c8/87e8/8878의 독립144BYTE 다섯 개다.
    double activityEnd_{}; //00544d58의 최초 BSS0이며 게임 정지 가능한 시계로 바꾸지 않는다.
    std::uint32_t zonePending_{},sourceType_{},begin_{},centralCreated_{}; // 순서가 다른 Native 주소들의 명시 소유자이며 주소 산술 배열로 쓰지 않는다.
    bool savedGame_{}; // 기존 실제 소비자의 정상0/1 표현을 유지하며 임의 Native DWORD 모델은 후속이다.
    double captureEnd_{},connectEnd_{},loginEnd_{}; // 각 Native 주소의 최초 BSS0과 서로 다른 수명을 유지한다.
    std::uint32_t hudModeCommand_{0x0200003dU}; //보호PE005427ec의 실제 명령 초기값이다.
    std::uint32_t addIslandCommand_{0x02000022U}; //보호PE00542780의 실제 Add Island 명령 초기값이다.
};
}
