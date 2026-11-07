//004b6660의 현재 동기화·서버 실패·Player flags·활동 프레임과 메타 상태 요청을 연결한다.
#pragma once
#include "o/RawConnectionActivity.h"

namespace netstorm::o { // 호스트 연결 token은 같은 Registry의 실제 노드 정체성이다.
struct ConnectionEndedGlobals {
    const bool& server;std::uint32_t& synchronization; // 현재00540bc0/005c8bf4이며 callback 뒤 다시 읽는다.
    const std::uint32_t& phase;std::uint32_t& nextPhase; //005c8758/005c8878의 실제 현재/다음 상태다.
    std::span<std::uint32_t,kConnectionActivityEntries> frames; // 같은 활동22DWORD 중 선택된 양수 Player 한 칸만0쓴다.
    std::function<std::span<std::uint8_t>()> players; // flags의 세 store마다 현재 Player base를 다시 빌린다.
};
struct ConnectionEndedHooks {
    std::function<void()> saveServerFailure; //004b6560의 flags8 정리와 같은 저장 writer다.
    std::function<void(std::int32_t)> disconnect; //004e2940의 현재 연결 지연 종료다.
    std::function<void(std::uint32_t)> broadcastPlayer; //00491270의 현재 접속자별 Player 상태 송신이다.
};
// 동기화0/앞선 flags·종료/송신은 예외 때 되돌리지 않고 자연 반환에서 다음 상태도 쓰지 않는다.
void ReceiveConnectionEnded(std::int32_t sender,std::uint32_t connection,ConnectionEndedGlobals globals,const ConnectionEndedHooks& hooks);
}
