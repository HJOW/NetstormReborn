// 시간 동기화 경로004c1f20의 세 DWORD 쓰기만 같은 실제 저장소에 실행한다.
#pragma once
#include <cstdint>

namespace netstorm::o {
// 실제 Client의 연결·응답모드와 시계/State/저장 reader가 공유하는 DWORD를 빌린다.
struct ClockSyncResetGlobals {
    std::uint32_t& connection; //005c8bf4의 현재 동기화 연결 token이며 메모리를 소유하거나 해제하지 않는다.
    std::uint32_t& replyMode; //005c8c00의 게임 시각 응답모드이며 인접 송신·수신 카운터와 별도다.
    std::uint32_t& gameTicks; //0055b4ac의 같은 DWORD다. 기존 State의 gameTicks/저장 reader tail과 사본을 만들지 않는다.
};
//004c1f20: connection→replyMode→gameTicks 순으로 0을 쓴다. 큰 명령 큐·시계·시간 동기화 배열은 초기화하지 않는다.
void ResetClockSynchronization(const ClockSyncResetGlobals& globals);
}
