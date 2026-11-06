// 원본004b9010의 현재 연결 활동 송신·지연 안내·자동 서버 이전 관문이다.
#pragma once
#include "o/RawLocalMissionState.h"
#include "o/RawConnectionActivity.h"

namespace netstorm::o { // 실제 연결·벽시각·Player는 같은 Client 수명에서 빌린다.
struct ConnectionHealthGlobals {
    const LocalMissionStateBlock& phase;double& heartbeatEnd;const bool& server; //005c8758/005c8620/00540bc0의 서로 다른 현재 입력이다.
    const std::uint32_t& localOwner;const std::uint32_t& autoRestart;ConnectionActivityTables activity; // 현재 owner/00531908와22칸 두 표다.
    std::function<std::span<const std::uint8_t>()> players; // 매 반복 현재9행 flags를 읽는다.
};
struct ConnectionHealthHooks {
    std::function<bool(std::uint32_t)> connectionExists; // 실제 객체 표의 NULL 여부이며 open 성공으로 바꾸지 않는다.
    std::function<double()> wallSeconds,frameWallSeconds;std::function<std::uint32_t()> frameNumber; // 새 wall과 프레임 고정 wall/하위DWORD를 구별한다.
    std::function<void(bool)> sendHeartbeat; // 현재 서버면 원문 broadcast, 아니면9에 길이5의64h를 보낸다.
    std::function<bool()> battleFinished;std::function<void(std::uint32_t,std::uint32_t)> status; // 실제 종류1/2 안내 관문과 표시다.
    std::function<void()> saveServerFailure; // flags8행의 실제 비트 변경 뒤 같은 재접속 writer를 호출한다.
};
bool PollConnectionHealth(ConnectionHealthGlobals globals,const ConnectionHealthHooks& hooks); // 시간 초과의 저장 부모가 복귀했을 때만 참이며 caller가 자동004b8cb0을 이어 실행한다.
}
