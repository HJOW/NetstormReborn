// 실제 수신 버퍼와 같은 전역의 부분 효과·현재 재조회 순서를 보존한다.
#include "o/RawStateBattleStart.h"
#include <array>
#include <cstdio>
#include <stdexcept>

namespace netstorm::o { //일반 State7 설치는 이미 연결된 같은 분배기에서 수행한다.
namespace { //선택된 접근의 물리만 검사하고 앞선 원문 쓰기를 되돌리지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("전투 시작 수신의 실제 장치 미연결");return function; } //비선택 장치는 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("전투 시작 수신의 실제 읽기 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //낮은 지정 BYTE부터 비정렬 x86 값을 조합한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("전투 시작 수신의 실제 쓰기 물리 부족");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //현재 WORD/BYTE만 쓰고 이웃 필드는 보존한다.
}
void ObserveBattleStartZone(std::span<std::uint8_t> packet,std::uint32_t sending,const StateBattleStartHooks& h) {
    if (!sending) return;if (Read(packet,14,1)==0xffU) Need(h.assertion)("numClients != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x48f);Put(packet,2,Read(packet,14,1)*71U+15U,2); //진단이 실제 패킷을 바꾸면 복귀 뒤 그 현재 BYTE를 읽는다.
}
void ReceiveBattleStartZone(std::span<std::uint8_t> packet,StateBattleStartGlobals g,const StateBattleStartHooks& h) {
    if (g.checking && g.state.phase[0]!=29 && g.state.phase[0]!=33) Need(h.assertion)("currState.state == S_META_CHAL_CONNECTED || currState.state == S_META_CHAL_CONNECTED_MONITOR",".\\State.cpp",0xdef);
    g.state.next[0x54/4]=0x636f6c7aU;g.state.next[0x58/4]=0x2a2c6c61U;g.state.next[0x5c/4]&=0xffffff00U; //PE00513870의 zlocal,*와 끝1BYTE만 쓰며 상위3BYTE는 남긴다.
    g.state.next[0]=7;g.state.next[0x7c/4]=g.gamePort;g.state.next[0x80/4]=Need(h.duplicate)(packet);++g.zonePending;
    if (g.recordRequested) { std::array<char,256> name{};std::snprintf(name.data(),name.size(),"%08X.%X.c.dat",Read(packet,10,4),g.subscriber);Need(h.openRecord)(name.data()); } //복제/observer 뒤의 현재 packet+10과 자기 subscriber로 같은 파일을 연다.
    Need(h.disconnect)(10);Need(h.disconnect)(11); //수신 중인 노드의 지연 반납/깊이는 같은 실제 TCP owner가 처리한다.
}
}
