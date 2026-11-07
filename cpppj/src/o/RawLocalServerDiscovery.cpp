// Native004b3fc0/004b4050/State34·35/004b8a10의 순서와 서로 다른 전역 수명을 보존한다.
#include "o/RawLocalServerDiscovery.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o { // OS 없는 검색 몸체이며 실제 전송/창 존재/상태 실행은 호출자의 장치다.
namespace { // 이 번역 단위의 비정렬 DWORD와 CRT 변환만 처리한다.
// 선택된 장치만 필수로 요구하며 미선택 분기의 callback을 호출하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("로컬 서버 검색의 실제 장치 미연결");return function; }
// 원문_ftol의signed64절삭 하위DWORD이며 무효FISTP의하위값은0이다.
std::uint32_t Seed(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
// 원문 현재15비트rand를 한 번만 소비하고 원래double 저장 경계의 지연을 반환한다.
double RetryDelay(CrtRandom& random) { return static_cast<double>(random.Next()%100U)*0.001+0.5; }
// 실제 패킷의little endian DWORD를 읽으며 선언 길이와 물리 길이를 혼동하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> packet,std::size_t at) {
    if (at>packet.size() || packet.size()-at<4) throw std::out_of_range("66h 검색 수신의 물리 DWORD 부족");
    return std::uint32_t(packet[at])|(std::uint32_t(packet[at+1])<<8)|(std::uint32_t(packet[at+2])<<16)|(std::uint32_t(packet[at+3])<<24);
}
}
// packet 생성자는Native0052ee56..59의실제header상수를쓰며payload두DWORD는caller값이다.
std::array<std::uint8_t,12> MakeLocalServerDiscoveryPacket(std::uint32_t command,std::uint32_t value) {
    std::array<std::uint8_t,12> packet{1,0x66,12,0}; //원문 header 뒤 두DWORD만 아래에서 덮는다.
    for (std::size_t byte=0;byte<4;++byte) { //각payload DWORD의낮은BYTE부터같은순서로기록한다.
        packet[4+byte]=static_cast<std::uint8_t>(command>>(byte*8));packet[8+byte]=static_cast<std::uint8_t>(value>>(byte*8));
    }
    return packet;
}
// 원본R은한번작성한같은서버발견패킷을현재UDP노드의가상SendTo로세번전달한다.
void AnnounceLocalServer(std::uint32_t basePort,const std::function<void(std::span<std::uint8_t>)>& broadcast) {
    auto packet=MakeLocalServerDiscoveryPacket(1,basePort); //명령1의+8은클라이언트검색토큰이아니라서버TCPbasePort다.
    for (std::size_t attempt=0;attempt<3;++attempt) Need(broadcast)(packet); //Native00475eb7/00475eca/00475eda의연속세송신호출이다.
}
// 초기 검색과 이후 재시도가 같은 현재 토큰을 다시 읽는다.
void BroadcastLocalServerDiscovery(LocalServerDiscoveryGlobals globals,const LocalServerDiscoveryHooks& hooks) {
    auto packet=MakeLocalServerDiscoveryPacket(0,globals.token);Need(hooks.broadcast)(packet); //실제 송신 동안만 원문12BYTE를 빌린다.
}
// 실제 열기 뒤 srand/네번rand를 소비하며 최종 토큰 저장 전까지 기존 전역을 유지한다.
void BeginLocalServerDiscovery(LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks) {
    Need(hooks.open)();random.SetState(Seed(Need(hooks.wall)()*1000.0));std::uint32_t token{}; //Native는wall×1000의_ftol하위DWORD를srand에전달한다.
    for (std::size_t byte=0;byte<4;++byte) token=(token<<8)|(random.Next()&0xffU); //첫rand의lowBYTE가최상위BYTE가된다.
    globals.token=token;BroadcastLocalServerDiscovery(globals,hooks);
}
// 5초검색기한과0.5초대재송신은각각별도의새벽시각을읽는다.
void StartLocalServerSearch(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks) {
    Need(hooks.progress)();BeginLocalServerDiscovery(globals,random,hooks);globals.searchEnd=Need(hooks.wall)()+5.0;
    const auto delay=RetryDelay(random);globals.retryEnd=delay+Need(hooks.wall)();Need(hooks.palette)();state.immediate[0]=35; //원문은delay를double에먼저저장하고별도Request진단없이직접즉시상태를쓴다.
}
// 재송신을 먼저 검사하고 만료 때만 실제 창을 찾으며 다음35 예약은 모든 정상 분기에 남긴다.
void ContinueLocalServerSearch(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks) {
    if (globals.retryEnd<=Need(hooks.wall)()) { BroadcastLocalServerDiscovery(globals,hooks);const auto delay=RetryDelay(random);globals.retryEnd=delay+Need(hooks.wall)(); }
    if (globals.searchEnd!=0.0 && globals.searchEnd<=Need(hooks.wall)()) {
        globals.searchEnd=0.0;
        if (!Need(hooks.serverWindowExists)()) { state.immediate[0]=37;state.next[0]=35;return; }
        globals.searchEnd=4.0+Need(hooks.wall)();
    }
    state.next[0]=35;
}
// 서버발견의현재블록직접쓰기와경쟁검색의signed비교를원문그대로구별한다.
void ReceiveLocalServerDiscovery(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,std::uint32_t sender,std::span<const std::uint8_t> packet,const LocalServerDiscoveryReceiveHooks& hooks) {
    if (Read(packet,4)==1) {
        state.current[0]=36;const auto address=Need(hooks.peerAddress)(sender); //현재36쓰기뒤에현재sender주소를조회한다.
        auto bytes=std::span<std::uint8_t>(reinterpret_cast<std::uint8_t*>(state.current.data()),sizeof(state.current)); //원문현재144BYTE의같은실제저장소다.
        if (address.size()>=bytes.size()-8) throw std::out_of_range("66h 검색 주소의 현재 상태 물리 초과");
        std::copy(address.begin(),address.end(),bytes.begin()+8);bytes[8+address.size()]=0;state.current[0x30/4]=Read(packet,8);Need(hooks.processState)();return;
    }
    if (std::bit_cast<std::int32_t>(Read(packet,8))<std::bit_cast<std::int32_t>(globals.token)) globals.searchEnd=Need(hooks.wall)()+12.0;
}
}
