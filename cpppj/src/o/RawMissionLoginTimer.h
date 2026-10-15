// PATCH 로그인 타이머의 같은 Player9 집계·미션 종료 선택·Begin/Abort 순서를 복원한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 원본1..8의 현재 subscriber/flags만 읽는다. 로그인/타이머가 같은 실제9 Player를 공유한다.
class MissionLoginPlayerCounts {
public:
    // PATCH의 정확한9×0xb4바이트를 빌린다. 저장소는 모든 로그인/타이머보다 오래 살아야 한다.
    MissionLoginPlayerCounts(OriginalEdition edition,std::span<const std::uint8_t> players);
    std::uint32_t Subscribers() const; // 00490ab0: subscriber!=0만 세며 flags를 요구하지 않는다.
    std::uint32_t Participating() const; // 00490a00: subscriber!=0 && flags2이며 상위 Options 수신과 공유한다.
    std::uint32_t Disconnected() const; // 00490bc0: subscriber!=0 && flags2 && flags100이다.
    std::uint32_t Ready() const; // 00490b20: subscriber!=0 && flags2 && flags1 && flags40이다.
    std::uint32_t LoggedIn() const; // 00490970: subscriber!=0 && flags2 && flags1이다.
private:
    // 정렬되지 않은 실제 Player의 little endian DWORD를 현재 기록에서 읽는다.
    std::uint32_t Read(std::uint32_t owner,std::size_t offset) const;
    std::span<const std::uint8_t> players_; // 현재 원문9기록이며 목록/flags 사본을 두지 않는다.
};
// 기존 게임 상태의 실제 저장소를 참조한다. bool 모드를 DWORD 참조로 재해석하지 않는다.
struct MissionLoginTimerState {
    double& end; // 005c85f0의 실제 벽시각 예약이다. 다른 모듈이 읽는 같은 double을 연결한다.
    std::int32_t& timeout; // 005c8938의 현재 loginTimeout 설정값이다.
    std::uint32_t& begin; // 005c7a74의 현재 Begin 수신/송신 표시다.
    std::uint32_t& scriptBreak; // 00542544이며 완료0/Abort1을 실제 입력 처리기가 공유한다.
    std::uint32_t& requestPhase; // Abort가4를 요청하는005c85ac이다. 후속004b6350의 실제 효과가 다시 바꿀 수 있다.
    std::uint32_t& clock; // savedGame0에서만 _clock 결과를 쓰는00594fb4다.
};
// 원문이 각 분기에서 다시 읽는 현재 모드다. Session의 기존 bool 저장소도 직접 조회할 수 있다.
struct MissionLoginTimerReads {
    std::function<bool()> savedGame,server,mission,missionPost; // 005c85d4/00540bc0/00594fa0/00594fa4의 서로 다른 현재 조건이다.
    std::function<bool()> initialGeysers,checkingAssertions; // 00540bc4와005e4794의 현재 조건이다.
};
// 서로 다른 원본 로그 몸체를 같은 성공 효과로 합치지 않도록 호출 종류를 전달한다.
enum class MissionLoginTimerLog {
    TimerStart, // 0045ad00("login status timer start\n")의 진입이다.
    Status // 004a68e0의 사제/카메라 진행 로그 진입이다.
};
// 실제 시간/전송/사제/UI 소비자만 외부 경계로 둔다. 선택되지 않은 경계를 가짜 성공으로 채우지 않는다.
struct MissionLoginTimerHooks {
    std::function<std::int32_t(std::string_view)> configInt; // 00441090의 현재 loginTimeout 정수 조회다.
    std::function<double()> wallSeconds; // 00460d70의 호출 시점마다 읽는 실제 벽시각이다.
    std::function<std::uint32_t()> clock; // 현재 CRT _clock의 DWORD 비트다.
    std::function<bool(std::int32_t)> connection; // 004e2b10(9)의 현재 실제 연결 객체 존재 여부다.
    std::function<void(std::int32_t,std::uint32_t,std::uint32_t)> remoteStatus; // 004b6180(9,0,0)의 실제 요청이다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; // 실제 Begin8BYTE를004e2e30과 같은 송신기로 전달한다.
    std::function<void(std::uint32_t)> priestRefresh; // 00495060(owner1..8)의 실제 사제 상태 재조회/설정이다.
    std::function<void()> saveReconnect; // 004b54e0이며 미션 자연 반환도 실제 해당 몸체가 결정한다.
    std::function<void()> closeProgress; // 004c8d90(&005c85b0)의 실제 진행 창 해제다.
    std::function<void()> resetCamera; // !missionPost 때만004340d0을 실행한다.
    std::function<void()> abortBattle; // 004b6350의 실제 상태 요청/정리다. Abort의 필수 하위다.
    std::function<void()> progress; // 아직 미복원인 일반 진행 UI 꼬리004b68d6..004b6ba1의 명시 경계다.
    std::function<void()> serverAssertion; // Begin의 checkingAssertions&&!server 진단 후 계속 실행한다.
    std::function<void(MissionLoginTimerLog,std::string_view)> log; // 원본 시작/상태 로그를 해당 위치에서 전달한다.
};
// 실제 타이머와 Begin/Abort는 직접 계산한다. 큰 일반 전투 초기화는 도달 시 명시 거부한다.
class RawMissionLoginTimer {
public:
    // 모든 참조/현재 조회 대상은 타이머보다 오래 살아야 한다. 필수 모드는 생성 시 확인한다.
    RawMissionLoginTimer(OriginalEdition edition,MissionLoginTimerState state,const MissionLoginPlayerCounts& players,
        MissionLoginTimerReads reads,MissionLoginTimerHooks hooks);
    // 004b67c0: start!=0이면 설정+현재 벽시각 예약,0이면 현재 예약의 실제 선택 분기를 실행한다.
    void Run(bool start) const;
    // 004180a0: end<=현재 wall이다. 게임 시각/프레임 캐시를 대신 사용하지 않는다.
    bool Expired() const;
    // 004b61f0: timer0→실제6e/1/len8/payload80000000 송신→begin1이다.
    void Begin() const;
    // 004b6780: begin0/timer0/scriptBreak1/phase4→실제004b6350→진행 창 해제다.
    void Abort() const;
private:
    MissionLoginTimerState state_; // 실제 기존 저장소에 대한 참조 묶음이며 DWORD/bool 복제를 만들지 않는다.
    const MissionLoginPlayerCounts& players_; // 같은 현재 Player9를 읽는 집계다.
    MissionLoginTimerReads reads_; // 각 원본 위치에서 다시 읽는 현재 모드다.
    MissionLoginTimerHooks hooks_; // 실제 선택된 외부 효과만 수행한다.
};
}
