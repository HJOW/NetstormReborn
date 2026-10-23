// 같은 Client의 원문 State 다섯 블록과 활동 기한을 메뉴/Editor/로그인이 함께 빌린다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <array>

namespace netstorm::client {
// Session 교체에서 다시 초기화하지 않는 실제720BYTE와00544d58 double의 소유자다.
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
    std::uint32_t& HudModeCommand() { return hudModeCommand_; } // 005427ec의 현재 명령을 호출 직전에 읽고 변경도 보존한다.
    std::uint32_t& AddIslandCommand() { return addIslandCommand_; } // 00542780의 현재 명령을 빈섬 예약 직전에 읽는다.
private:
    std::array<o::LocalMissionStateBlock,5> blocks_{}; //005c8630/8758/86c8/87e8/8878의 독립144BYTE 다섯 개다.
    double activityEnd_{}; //00544d58의 최초 BSS0이며 게임 정지 가능한 시계로 바꾸지 않는다.
    std::uint32_t hudModeCommand_{0x0200003dU}; //보호PE005427ec의 실제 명령 초기값이다.
    std::uint32_t addIslandCommand_{0x02000022U}; //보호PE00542780의 실제 Add Island 명령 초기값이다.
};
}
