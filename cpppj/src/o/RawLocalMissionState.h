// 단독 미션 상태7~10/13/14의 현재 블록 복사·확정·전송 수신 시점을 복원한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include <array>
#include <deque>
#include <functional>
#include <span>

namespace netstorm::o {
// 원본 상태와 부가 인자 전체0x90BYTE다. 첫DWORD만 상태번호다.
using LocalMissionStateBlock=std::array<std::uint32_t,36>;
// 실제 다섯 상태 블록을 같은 저장소로 빌린다.
struct LocalMissionStateStorage {
    LocalMissionStateBlock& old; // 005c8630: 마지막 확정 때의 이전phase 전체다.
    LocalMissionStateBlock& phase; // 005c8758: 마지막으로 공통부가 확정한 상태 전체다.
    LocalMissionStateBlock& immediate; // 005c86c8: 같은Run에서 이어서 처리할 요청이다.
    LocalMissionStateBlock& current; // 005c87e8: 현재 실행 중인 명령 전체다.
    LocalMissionStateBlock& next; // 005c8878: 다음Poll이 소비할 명령 전체다.
};
// 원본의 서로 다른 현재 모드/타이머/메타맵 표식을 빌린다.
struct LocalMissionStateGlobals {
    bool& server; bool& boss; bool& inBattle; bool& missionPost; bool& savedGame; // 00540bc0/bc4/00594fbc/fa4/005c85d4다.
    const bool& missionWorld; const bool& simulconnect; const bool& metamap; // 00594fa0/0054db50/00594fc8이다.
    PlayerWorldState& player; // 같은 현재PlayerKnown과두owner다.
    std::uint32_t& zonePending; std::uint32_t& gameTicks; std::uint32_t& sourceType; std::uint32_t& randomSeed; // 005c85c0/0055b4ac/0055a420/005c85b8이다.
    std::uint32_t& menuWindow; std::uint32_t& metaRefresh; std::uint32_t& metaRebuild; // 005c874c/005c85d8/dc다.
    std::uint8_t& titleInitial; const std::uint8_t& battleByte; // 00565e70의첫BYTE와005c85d0의현재BYTE다.
    double& connectEnd; double& activityEnd; // 005c85f8과00544d58의독립벽시각예약이다.
    const double& gameSeconds; const double& wallCached; // 0055b4d0/d8의서로다른현재시계다.
};
// 큰부모상태의원문early return은공통phase확정과명확히구분한다.
enum class LocalMissionParentResult { Commit,ReturnWithoutCommit };
// 원본의큰진행창문구선택이다. 실제표시/번역/창수명은필수소비자가처리한다.
enum class LocalMissionProgress { MissionStarting,ReconnectStarting,ConnectingCountdown,ReconnectCountdown,ServerLost };
// 작은상태알고리즘이선택하는외부몸체다. 도달한경계의누락은성공이아니다.
struct LocalMissionStateHooks {
    std::function<void(std::uint32_t)> logState; // 0045ad00의현재상태진입로그다.
    std::function<void()> assertion; // 004b2bc0의이미요청된immediate진단이다.
    std::function<void(std::uint32_t)> invalidState; // 원본default진단이다.
    std::function<LocalMissionParentResult(std::uint32_t)> parentState; // 1/2/3큰메뉴부모전체와실제반환종류다.
    std::function<void()> clearSelection; std::function<void(std::uint32_t)> cleanupBattle; // 004978d0/004b2df0의실제정리다.
    std::function<std::uint32_t()> seed; std::function<void()> refreshCommands; std::function<void()> stopMusic; // 00440c60/0042e440/00469fc0다.
    std::function<void()> refreshMission; // missionWorld&&missionPost의00463a40가상+28이다.
    std::function<void(std::uint32_t)> installZone,freeZone; std::function<void()> installMapHandlers; // 004b4be0/free/004c1e70이다. ZoneDWORD는호스트가해석하는핸들이다.
    std::function<void()> refreshCursor; std::function<void(std::uint32_t)> uiMode; // 004993c0/00466280(mode,0,1)의실제UI효과다.
    std::function<double()> wallSeconds; // 00460d70의현재벽시각이다.
    std::function<void()> loadReconnectFort; // 큰00460980의현재재접속파일읽기다.
    std::function<void(LocalMissionProgress,std::int32_t)> progress; // 실제진행창을만들거나갱신한다.
    std::function<std::uint32_t()> battlePalette; std::function<void(std::uint32_t)> applyPalette; // 004410f0("battlePal")→004b6100이다.
    std::function<void(std::span<const std::uint8_t>,std::uint32_t)> connectServer; // 004e2620(9,current+54,current+7c,0)이다.
    std::function<std::int32_t(std::int32_t)> connectionStatus; std::function<bool(std::int32_t)> connectionOpen; // 004e2b70/004e2b10의현재조회다.
    std::function<void(std::int32_t)> closeConnection; std::function<void(std::int32_t,std::int32_t)> renameConnection; // 004e2940/004e27b0이다.
    std::function<void(std::int32_t,std::int32_t)> logConnection; // 401/40f분기의실제현재연결진단이다.
    std::function<void()> sendLogin,connectedProgress,closeProgress; // 004b63d0/004b6090/004c8dc0이다. 송신은수신큐에예약해야한다.
    std::function<std::uint32_t()> serverConnection; std::function<void(std::uint32_t)> attachCommander; // 비서버004e2220(9)→004c1eb0이다.
    std::function<bool(std::uint32_t)> fortAvailable; std::function<void()> loadCurrentFort; // 현재Templatewrapper검사와필수파일경계다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> fortCore; // 004bd550의실제현재CoreData최소24BYTE다.
    std::function<std::array<std::uint8_t,20>()> localAddress; // 004e2a50(out,9)의전체주소기록이다.
    std::function<void(std::uint32_t)> touchFort; std::function<void()> refreshFort; // 004bd540/00460730이다.
    std::function<void(float)> setReconnectSp; std::function<void(bool)> loginTimer,inputMode; // 004477d0/004b67c0(1)/0042f270(0)이다.
    std::function<void()> metaFirst,metaSecond,metaThird; // Poll의00479760/0047dd60/0047cab0이다.
};
// 실제상태확정과비동기수신경계를같은수명으로소유한다.
class RawLocalMissionState {
public:
    // 블록/전역/소비자는이모듈보다오래살아야하며PATCH만지원한다.
    RawLocalMissionState(OriginalEdition edition,LocalMissionStateStorage state,LocalMissionStateGlobals globals,LocalMissionStateHooks hooks);
    void Request(std::uint32_t state) const; // 004b2bc0: 현재immediate만쓰고phase를앞당기지않는다.
    void Poll(); // 004b88c0: 메타맵선행효과뒤next가있을때만current로복사하고Run한다.
    void Run(); // 004b6dd0: 작은선택상태와전체0x90BYTE공통확정을실행한다.
    void EnqueueReceive(std::function<void()> receive); // 실제수신처리기호출을예약하며즉시실행하지않는다.
    void DispatchQueued(); // Run이끝난뒤현재큐의실제수신기를순서대로실행한다.
private:
    bool Step(); // true이면원문공통부없이반환하는선택경로다.
    void Starting(); void Connected(); // 상태7/13의현재조회와원문효과순서다.
    LocalMissionStateStorage state_; LocalMissionStateGlobals globals_; LocalMissionStateHooks hooks_; // 같은원문블록/전역/효과다.
    bool running_{}; std::deque<std::function<void()>> receives_; // 재진입오류와실제비동기수신순서다.
};
}
