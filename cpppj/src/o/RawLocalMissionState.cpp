// 상태번호를callback안에서확정하지않고원본공통부에서전체블록을복사한다.
#include "o/RawLocalMissionState.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된실제소비자가없으면앞선원문쓰기만보존하고중단한다.
template<class Function> const Function& Need(const Function& function,const char* text) { if (!function) throw std::logic_error(text);return function; }
// 원본_ftol의정수부와하위DWORD다. 유효signed64밖은원본indefinite의하위0이다.
std::uint32_t TruncateDword(double value) { if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); }
// 정렬가정없이원문바이트를읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) { std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result; }
// 해당폭만수정해같은Template의다른필드를보존한다.
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) { for (std::size_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); }
}
// 다른판본/서로같은상태배열을실제다섯블록으로취급하지않는다.
RawLocalMissionState::RawLocalMissionState(OriginalEdition edition,LocalMissionStateStorage state,LocalMissionStateGlobals globals,LocalMissionStateHooks hooks):state_(state),globals_(globals),hooks_(std::move(hooks)) {
    const std::array<const LocalMissionStateBlock*,5> blocks{&state.old,&state.phase,&state.immediate,&state.current,&state.next};
    // 모든블록이서로다른현재저장소인지확인한다.
    for (std::size_t first=0;first<blocks.size();++first) for (std::size_t second=first+1;second<blocks.size();++second) if (blocks[first]==blocks[second]) throw std::invalid_argument("로컬미션상태블록중첩");
    if (edition!=OriginalEdition::Patch1078 || &globals.menuWindow!=&state.immediate[0x84/4]) throw std::invalid_argument("로컬미션상태PATCH/실제immediate+84별칭오류");
}
// 진단뒤에도원문은새요청으로덮어쓴다. callback뒤실제전달값을저장한다.
void RawLocalMissionState::Request(std::uint32_t state) const { if (state_.immediate[0]) Need(hooks_.assertion,"즉시상태중복진단미연결")();state_.immediate[0]=state; }
// next0은자연반환이며기존current/phase를임의0으로만들지않는다.
void RawLocalMissionState::Poll() {
    if (globals_.metaRebuild && !globals_.metaRefresh && globals_.metamap) { Need(hooks_.metaFirst,"메타맵첫효과미연결")();globals_.metaRebuild=0; }
    if (globals_.metaRefresh && globals_.metamap) { Need(hooks_.metaSecond,"메타맵둘째효과미연결")();Need(hooks_.metaThird,"메타맵셋째효과미연결")();globals_.metaRefresh=0; }
    if (state_.next[0]) { state_.current=state_.next;state_.next[0]=0;Run(); }
}
// original7은cleanup전seed를보존하고그후현재전역을각위치에서다시읽는다.
void RawLocalMissionState::Starting() {
    Need(hooks_.clearSelection,"선택정리미연결")();const auto zonePending=globals_.zonePending;globals_.titleInitial=0;std::uint32_t ticks=0,source=0;
    if (state_.current[0x84/4]) { ticks=globals_.gameTicks;source=globals_.sourceType; }
    Need(hooks_.cleanupBattle,"현재전투정리미연결")(1);globals_.randomSeed=Need(hooks_.seed,"현재난수시드조회미연결")();Need(hooks_.refreshCommands,"명령창갱신미연결")();
    if (state_.current[0x84/4]) { globals_.gameTicks=ticks;globals_.sourceType=source; }
    globals_.inBattle=true;Need(hooks_.stopMusic,"음악정리미연결")();
    if (globals_.missionWorld && globals_.missionPost) Need(hooks_.refreshMission,"현재미션가상갱신미연결")();
    if (globals_.simulconnect) throw std::logic_error("simulconnect서버분기는선택단독미션범위밖");
    if (zonePending) { Need(hooks_.installZone,"현재Zone설치미연결")(state_.current[0x80/4]);Need(hooks_.freeZone,"현재Zone해제미연결")(state_.current[0x80/4]);state_.current[0x80/4]=0;state_.next[0x80/4]=0;Need(hooks_.installMapHandlers,"실제맵처리기등록미연결")(); }
    Need(hooks_.refreshCursor,"현재커서갱신미연결")();globals_.player.playerKnown=0;Need(hooks_.uiMode,"현재UI모드미연결")(2);
    const auto now=Need(hooks_.wallSeconds,"현재벽시각미연결")();globals_.activityEnd=now+10.0;state_.immediate[1]=20;Request(8);
    if (state_.current[0x84/4]) { globals_.savedGame=true;Need(hooks_.loadReconnectFort,"재접속Fort읽기미연결")();Need(hooks_.progress,"재접속진행창미연결")(LocalMissionProgress::ReconnectStarting,0); }
    else if (globals_.missionPost) Need(hooks_.progress,"미션진행창미연결")(LocalMissionProgress::MissionStarting,0);
    else throw std::logic_error("일반전투연결UI분기는선택단독미션범위밖");
    const auto palette=Need(hooks_.battlePalette,"현재battlePal조회미연결")();Need(hooks_.applyPalette,"실제팔레트적용미연결")(palette);
}
// original13의Template작은필드는직접쓰고큰파일/Builder/UI몸체는필수소비자로남긴다.
void RawLocalMissionState::Connected() {
    globals_.connectEnd=0.0;Need(hooks_.closeProgress,"현재진행창전체해제미연결")();
    if (!globals_.server) { const auto connection=Need(hooks_.serverConnection,"현재서버연결객체미연결")();Need(hooks_.attachCommander,"실제Commander연결미연결")(connection); }
    if (!Need(hooks_.fortAvailable,"현재Fortwrapper조회미연결")(globals_.player.templateOwner)) Need(hooks_.loadCurrentFort,"현재Fort파일읽기미연결")();
    auto core=Need(hooks_.fortCore,"현재FortCore조회미연결")(globals_.player.templateOwner);if (core.size()<24) throw std::out_of_range("현재FortCore24BYTE누락");core[0]=static_cast<std::uint8_t>((core[0]&0xfbU)|2U);
    const auto address=Need(hooks_.localAddress,"현재로컬연결주소미연결")();Write(core,4,Read(address,4,4),4);const auto port=Read(address,Read(address,0,2)==6 ? 12U : 2U,2);Write(core,8,((port&255U)<<8)|(port>>8),2);Write(core,12,TruncateDword(globals_.gameSeconds),4);
    const auto battleByte=globals_.battleByte;Write(core,0x16,Read(core,0x16,2)+1U,2);core[0x10]=battleByte;Need(hooks_.touchFort,"현재Fort갱신미연결")(globals_.player.templateOwner);
    Need(hooks_.uiMode,"현재UI모드미연결")(3);Need(hooks_.refreshFort,"실제Fort설정갱신미연결")();globals_.menuWindow=0;state_.next[0x84/4]=0;Request(14);
    if (state_.current[0x84/4]) Need(hooks_.setReconnectSp,"실제재접속SP지정미연결")(static_cast<float>(std::bit_cast<std::int32_t>(state_.current[0x88/4])));
    if (!Need(hooks_.connectionOpen,"현재서버연결조회미연결")(9)) { globals_.sourceType=0;state_.next[0]=3; }
    else { Need(hooks_.loginTimer,"실제로그인타이머시작미연결")(true);Need(hooks_.inputMode,"실제입력모드전환미연결")(false); }
}
// 선택상태의일반반환은공통부로돌아가고원문timeout만early return한다.
bool RawLocalMissionState::Step() {
    switch (state_.current[0]) {
    case 7: Starting();break;
    case 8:
        if (globals_.connectEnd==0.0) { const auto now=Need(hooks_.wallSeconds,"연결현재벽시각미연결")();globals_.connectEnd=now+30.0;if (!globals_.missionPost) throw std::logic_error("일반전투연결문구는선택범위밖"); }
        Need(hooks_.connectServer,"실제서버연결미연결")(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(state_.current.data())+0x54,20),state_.current[0x7c/4]);state_.next[0]=9;break;
    case 9: {
        auto connected=Need(hooks_.connectionStatus,"현재연결상태미연결")(9);
        if (connected<1 && !globals_.server) if (Need(hooks_.connectionStatus,"현재pending연결미연결")(-1)!=0) { const auto status=hooks_.connectionStatus(-1);Need(hooks_.logConnection,"연결전환진단미연결")(0x401,status);Need(hooks_.closeConnection,"실제연결해제미연결")(9);Need(hooks_.renameConnection,"실제연결번호전환미연결")(-1,9);connected=1; }
        const auto now=Need(hooks_.wallSeconds,"연결만료벽시각미연결")();if (globals_.connectEnd<=now) { globals_.sourceType=0;state_.next[0]=3;return true; }
        const auto open=Need(hooks_.connectionOpen,"현재서버연결조회미연결")(9);
        if (!open || connected<0) { const auto currentOpen=hooks_.connectionOpen(9);Need(hooks_.logConnection,"연결실패진단미연결")(0x40f,currentOpen ? 1 : 0);Need(hooks_.closeConnection,"실제연결해제미연결")(9);Request(8); }
        else if (connected<1) { const auto remaining=std::bit_cast<std::int32_t>(TruncateDword(globals_.connectEnd-globals_.wallCached));Need(hooks_.progress,"실제연결대기창미연결")(globals_.savedGame ? LocalMissionProgress::ReconnectCountdown : LocalMissionProgress::ConnectingCountdown,remaining);state_.next[0]=9; }
        else { Need(hooks_.sendLogin,"실제로그인송신미연결")();Need(hooks_.connectedProgress,"로그인전송진행창미연결")();Request(10); }break;
    }
    case 10: if (!Need(hooks_.connectionOpen,"현재서버연결조회미연결")(9)) { Need(hooks_.progress,"실제연결실패창미연결")(LocalMissionProgress::ServerLost,0);globals_.sourceType=0;state_.next[0]=3; } else state_.next[0]=10;break;
    case 13: Connected();break;
    case 4: case 12: case 14: break; //004b874c의State4도004b86bd 공통확정으로 직행하며 다음Poll에서만 소비한다.
    case 1: case 2: case 3: case 39: case 40: case 41: return Need(hooks_.parentState,"큰메뉴부모상태미연결")(state_.current[0])==LocalMissionParentResult::ReturnWithoutCommit;
    default: Need(hooks_.invalidState,"미복원상태진단미연결")(state_.current[0]);break;
    }
    return false;
}
// callback실행중에는phase가이전상태이며모든선택효과가끝난뒤확정한다.
void RawLocalMissionState::Run() {
    if (running_) throw std::logic_error("상태실행중동기수신재진입: 실제수신큐를사용해야함");running_=true;
    try {
        // 즉시요청이있을때만같은호출에서다음상태를다시처리한다.
        for (;;) { state_.next=state_.current;state_.immediate=state_.current;state_.next[0]=0;state_.immediate[0]=0;Need(hooks_.logState,"현재상태로그미연결")(state_.current[0]);if (Step()) break;const bool again=state_.immediate[0]!=0;state_.old=state_.phase;state_.phase=state_.current;state_.current[0]=0;if (!again) break;state_.current=state_.immediate; }
    } catch (...) { running_=false;throw; }running_=false;
}
// 송신callback은실제수신기를이큐에넣고원문공통확정까지반환해야한다.
void RawLocalMissionState::EnqueueReceive(std::function<void()> receive) { if (!receive) throw std::invalid_argument("빈로컬수신예약");receives_.push_back(std::move(receive)); }
// 상태실행중분배를명시거부하여reply28이phase10이전에도착하지않게한다.
void RawLocalMissionState::DispatchQueued() { if (running_) throw std::logic_error("상태확정전수신분배금지");while (!receives_.empty()) { auto receive=std::move(receives_.front());receives_.pop_front();receive(); } }
}
