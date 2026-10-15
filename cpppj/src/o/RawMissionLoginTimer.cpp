// 원본 로그인 종료는 missionPost이면 ready 장벽을 추가하지 않고 사제/타이머 정리를 실행한다.
#include "o/RawMissionLoginTimer.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerStride=0xb4,kPlayerCount=9; // 같은 실제9 Player의 물리 폭/개수다.
constexpr std::array<std::uint8_t,8> kBeginPacket{1,0x6e,8,0,0,0,0,0x80}; // 보호 PE0052ee72의type/version을 실제version/type배치로 작성하고DWORD80000000을 붙인다.
// 실제로 선택된 경계만 확인한다. inactive 타이머에서 사용하지 않는 효과를 호출하거나 성공으로 채우지 않는다.
template<class Function> const Function& Need(const Function& function,const char* message) {
    if (!function) throw std::logic_error(message);return function;
}
}
// 다른 판본이나 잘린 Player 기록을 원본 정상 입력으로 간주하지 않는다.
MissionLoginPlayerCounts::MissionLoginPlayerCounts(OriginalEdition edition,std::span<const std::uint8_t> players):players_(players) {
    if (edition!=OriginalEdition::Patch1078 || players.size()!=kPlayerCount*kPlayerStride)
        throw std::invalid_argument("로그인 집계의 실제 PATCH9 Player 기록 누락");
}
// 원본 DWORD를 정렬 가정 없이 낮은BYTE부터 읽는다.
std::uint32_t MissionLoginPlayerCounts::Read(std::uint32_t owner,std::size_t offset) const {
    std::uint32_t value{};
    // 네BYTE를 그대로 조립하며 현재 기록에 숨은 부호/참여 추정을 덧붙이지 않는다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(players_[owner*kPlayerStride+offset+byte])<<(byte*8);
    return value;
}
// flags를 읽지 않는 원본 subscriber 집계다. Player0은 포함하지 않는다.
std::uint32_t MissionLoginPlayerCounts::Subscribers() const {
    std::uint32_t count{};
    // 현재1..8의 subscriber만 방문한다.
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner) if (Read(owner,0x2c)!=0) ++count;
    return count;
}
// 로그인된 것과 별개인 flags2 참여 집계다.
std::uint32_t MissionLoginPlayerCounts::Participating() const {
    std::uint32_t count{};
    // subscriber가 없는 기록의 stale flags는 집계하지 않는다.
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner) if (Read(owner,0x2c)!=0 && (Read(owner,0x50)&2U)!=0) ++count;
    return count;
}
// disconnected100은 subscriber와flags2가 모두 있을 때만 센다.
std::uint32_t MissionLoginPlayerCounts::Disconnected() const {
    std::uint32_t count{};
    // flags100만 있는 기록을 접속/참여로 추정하지 않는다.
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner)
        if (Read(owner,0x2c)!=0 && (Read(owner,0x50)&2U)!=0 && (Read(owner,0x50)&0x100U)!=0) ++count;
    return count;
}
// flags2/1/40의 세 조건은 서로 다른 원본 상태다.
std::uint32_t MissionLoginPlayerCounts::Ready() const {
    std::uint32_t count{};
    // Ready40이 켜졌어도 실제 subscriber/참여/로그인이 없으면 제외한다.
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner)
        if (Read(owner,0x2c)!=0 && (Read(owner,0x50)&2U)!=0 && (Read(owner,0x50)&1U)!=0 && (Read(owner,0x50)&0x40U)!=0) ++count;
    return count;
}
// timeout 뒤 최소 두 명 검사는 Ready와 별개인 flags2/1을 읽는다.
std::uint32_t MissionLoginPlayerCounts::LoggedIn() const {
    std::uint32_t count{};
    // Player0과 subscriber0의 stale 로그인표시는 제외한다.
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner)
        if (Read(owner,0x2c)!=0 && (Read(owner,0x50)&2U)!=0 && (Read(owner,0x50)&1U)!=0) ++count;
    return count;
}
// 값형 사본 대신 실제 기존 상태 참조와 현재 모드를 빌린다.
RawMissionLoginTimer::RawMissionLoginTimer(OriginalEdition edition,MissionLoginTimerState state,const MissionLoginPlayerCounts& players,
    MissionLoginTimerReads reads,MissionLoginTimerHooks hooks):state_(state),players_(players),reads_(std::move(reads)),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || !reads_.savedGame || !reads_.server || !reads_.mission || !reads_.missionPost ||
        !reads_.initialGeysers || !reads_.checkingAssertions) throw std::invalid_argument("로그인 타이머의 PATCH 현재 모드 조회 누락");
}
// 정지된 게임 시각과 무관한 현재 wall을 별도로 읽는다.
bool RawMissionLoginTimer::Expired() const {
    const auto now=Need(hooks_.wallSeconds,"로그인 실제 벽시각 미연결")(); // 원본은 wall 효과 뒤 같은end를 다시 읽는다.
    return state_.end<=now;
}
// 실제 메세지8BYTE와 쓰기/송신 순서를 유지한다. 송신 효과 뒤의 begin1을 앞당기지 않는다.
void RawMissionLoginTimer::Begin() const {
    if (reads_.checkingAssertions() && !reads_.server()) Need(hooks_.serverAssertion,"로그인 Begin 서버 진단 미연결")();
    state_.end=0.0;Need(hooks_.broadcast,"로그인 Begin 실제 송신 미연결")(kBeginPacket);state_.begin=1;
}
// 요청 phase는 실제 하위004b6350이 다시 바꿀 수 있으므로 마지막에4로 덮어쓰지 않는다.
void RawMissionLoginTimer::Abort() const {
    state_.begin=0;state_.end=0.0;state_.scriptBreak=1;state_.requestPhase=4;
    Need(hooks_.abortBattle,"로그인 Abort 실제 상태 정리 미연결")();Need(hooks_.closeProgress,"로그인 진행 창 해제 미연결")();
}
// missionPost와 Begin은 별개이며 미ready/미만료가 항상 진행 창 대기를 뜻하지 않는다.
void RawMissionLoginTimer::Run(bool start) const {
    if (start) {
        Need(hooks_.log,"로그인 시작 로그 미연결")(MissionLoginTimerLog::TimerStart,"login status timer start\n");
        const auto timeout=Need(hooks_.configInt,"로그인 현재 설정 미연결")("loginTimeout"); // 원본 stack local의 선택된 정수값이다.
        state_.timeout=timeout;const auto now=Need(hooks_.wallSeconds,"로그인 실제 벽시각 미연결")(); // 설정 저장 뒤 실제 wall 효과를 수행한다.
        state_.end=static_cast<double>(timeout)+now;return;
    }
    if (state_.end==0.0) return;
    if (!reads_.savedGame()) state_.clock=Need(hooks_.clock,"로그인 실제 CRT 시각 미연결")();
    bool serverBranch=true; // 연결 조회 효과가 서버 모드를 바꾸면 원문 재조회 결과로 서버 사슬을 선택한다.
    if (!reads_.server()) {
        if (!Need(hooks_.connection,"로그인 현재 서버 연결 조회 미연결")(9)) { Abort();return; }
        if (!reads_.server()) { Need(hooks_.remoteStatus,"로그인 실제 원격 상태 요청 미연결")(9,0,0);serverBranch=false; }
    }
    if (serverBranch) {
        bool begin=true; // 미ready/미만료이면 Begin만 생략하고 아래 실제 missionPost 종료 판정으로 간다.
        if (state_.begin==0) {
            const auto dropped=players_.Disconnected();const auto subscribers=players_.Subscribers();const auto ready=players_.Ready(); // 원본 bc0→ab0→b20 순서다.
            if (subscribers-dropped!=ready) {
                if (!Expired()) begin=false;
                else if (players_.LoggedIn()<2 && !reads_.savedGame()) { Abort();return; }
            }
        }
        if (begin) Begin();
    }
    if (reads_.missionPost() || state_.begin!=0) {
        if (!reads_.savedGame()) {
            if (!reads_.mission()) throw std::logic_error("로그인 일반 전투 초기화004b4a20 사슬은 아직 복원되지 않음");
            if (reads_.initialGeysers() && !reads_.mission()) throw std::logic_error("로그인 일반 랜덤 간헐천 초기화는 아직 복원되지 않음");
        }
        Need(hooks_.log,"로그인 사제 상태 로그 미연결")(MissionLoginTimerLog::Status,"Seting Priest State\n");
        // 같은 현재 Player/사제 상태의 실제 owner1..8을 원본 순서로 재조회한다.
        for (std::uint32_t owner=1;owner<9;++owner) Need(hooks_.priestRefresh,"로그인 실제 사제 상태 재설정 미연결")(owner);
        Need(hooks_.log,"로그인 상태 완료 로그 미연결")(MissionLoginTimerLog::Status,"Done\n");
        state_.begin=0;state_.scriptBreak=0;Need(hooks_.saveReconnect,"로그인 실제 재접속 저장 경계 미연결")();
        state_.end=0.0;Need(hooks_.closeProgress,"로그인 진행 창 해제 미연결")();
        Need(hooks_.log,"로그인 카메라 로그 미연결")(MissionLoginTimerLog::Status,"Moveing Camera\n");
        if (!reads_.missionPost()) Need(hooks_.resetCamera,"로그인 실제 카메라 초기화 미연결")();
        Need(hooks_.log,"로그인 상태 완료 로그 미연결")(MissionLoginTimerLog::Status,"Done\n");return;
    }
    Need(hooks_.progress,"로그인 일반 진행 창 꼬리004b68d6은 아직 미연결")();
}
}
