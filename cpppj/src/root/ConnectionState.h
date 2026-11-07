// 제공된 R.exe의 일반/대기 연결과 Challenge 복합 번호를 클라이언트 연결표와 분리한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::root { //c:\ns\r\r.cpp와R전용Zacket의현재전역을복원한다.
inline constexpr std::uint32_t kDiagReceive=1,kRootMode=2,kChallengeMode=4,kLocalMode=8,kDiagTransmit=16; //00477950/00477e10의실제비트다.오래된모드출력문구의번호로대체하지않는다.
class ConnectionState final {
public:
    ConnectionState()=default; //새R프로세스의BSS0과별도data초기값pendingCount1을한번구성한다.
    ConnectionState(const ConnectionState&)=delete;ConnectionState& operator=(const ConnectionState&)=delete; //현재표를빌린장치와숫자토큰의주소수명을복제하지않는다.
    std::array<std::uint32_t,256> transceivers{}; //004d1b88의일반256칸이며첫빈칸검색은1..255만선택한다.
    std::array<std::uint32_t,32> pending{}; //004d8a00의하위Zacket32칸이며0은별도보존칸이다.
    std::array<std::uint32_t,51> challenges{}; //004d05f0의서로다른51개Challenge상태토큰이다.대기연결표가아니다.
    std::uint32_t pendingCount{1},pendingInitialized{}; //004cd908의보호PE초기1과004d8a9c의BSS0을구별한다.
    std::uint32_t modeFlags{},localLaunch{},assertions{},exitRequested{}; //004d21b8/004d21cc/004d8ac4/004d2204의별도현재DWORD다.
    std::uint32_t udpBroadcastId{},ipxBroadcastId{}; //004d21d8/dc의 현재 일반 연결 번호이며 노드 토큰이나 OS 핸들이 아니다. 최초BSS는0이다.
    std::int32_t challengeStateCount{}; //004d21e0의현재signed개수이며LAN모드해석은1로쓴뒤마지막증가로2를만든다.
};
struct ConnectionHooks {
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //선택된원문표현식/파일/줄진단이며진단복귀뒤쓰기도보존한다.
    std::function<void(std::uint32_t)> close; //R00484820의같은실제노드닫힘/깊이지연반납장치다.
    std::function<void(std::uint32_t,std::uint32_t)> sender; //현재노드의+40 sender DWORD를쓰며Host포인터를절삭하지않는다.
    std::function<void(std::uint32_t,std::uint32_t)> endParticipant; //현재Challenge토큰/복합ID의lowBYTE를R0044e0d0에전달한다.
    std::function<void(std::int32_t)> exit; //004d2204비영이면선택되는실제_exit(0)이며정상복귀하지않는장치다.
};
std::uint32_t FindFreeTransceiver(const ConnectionState& state); //004748c0의현재1..255중첫0번호이며가득차면0이다.슬롯은예약하지않는다.
std::uint32_t PendingTransceiver(const ConnectionState& state,std::int32_t id); //00478ac0의현재pending[-id]이며32칸물리밖은명시거부한다.
std::uint32_t ReadTransceiver(const ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks); //00474830의일반조회와원문negative관문을그대로구별한다.유효pending -1..-31은이부모에서0이다.
void InitializePending(ConnectionState& state); //00485a20의최초listener에서만32칸0/초기화1을쓰며현재pendingCount를재설정하지않는다.
void RegisterPending(ConnectionState& state,std::uint32_t token,const ConnectionHooks& hooks); //00485970의첫빈1..31저장→실제sender음수쓰기→개수증가→상한진단순서다.
void UnregisterPending(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks); //00485800의진단→같은칸0→개수감소/하한진단이다.소켓을닫지않는다.
void ClosePending(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks); //004858d0의유효현재칸닫기/0/감소뒤무조건하한진단이다.
void CloseAllPending(ConnectionState& state,const ConnectionHooks& hooks); //00485790의0..31현재노드닫기/0만수행한다.원문처럼개수는여기서변경하지않는다.
void ReceiveConnectionEnded(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks); //00475170의exit/대기/일반1..255/Challenge복합ID종료부모다.
void SelectLanLaunch(ConnectionState& state,std::span<const std::string> arguments); //00479760의정확히argc2/argv1소문자lan일때만현재localLaunch1을쓴다.
void InterpretMode(ConnectionState& state,const std::function<void(std::string_view,std::int32_t&)>& readInt); //00477950후반의세0임시옵션/OR비트·LAN8/현재Chal개수마지막증가만실행한다.앞선서버옵션읽기는별도부모다.
}
