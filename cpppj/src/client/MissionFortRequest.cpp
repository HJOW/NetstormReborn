// 원본 주소의 마지막BYTE와 요청의 stale 꼬리를 서로 구별해 보존한다.
#include "client/MissionFortRequest.h"
#include "client/State.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 외부 효과가 누락된 요청을 성공으로 반환하지 않는다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("Fort 요청 실제 효과 누락");return effect; }
// 원본 주소의 비정렬DWORD 두 개만 낮은 바이트부터 읽는다.
std::uint32_t AddressWord(std::span<const std::uint8_t> address,std::size_t start) {
    std::uint32_t value{};
    // 지정된4BYTE만 읽으며 인접 요청 필드는 해석하지 않는다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(address[start+index])<<(index*8);return value;
}
}
// 선택 뒤 현재 주소/포트,복제 뒤 현재 순번을 읽는 원본 위치를 유지한다.
void WriteMissionFortRequest(std::string_view name,MissionFortRequestGlobals globals,const MissionFortRequestHooks& hooks) {
    if (globals.localAddress.size()<9) throw std::invalid_argument("Fort 요청 현재9BYTE 주소 누락");
    Need(hooks.selectFort)(name.substr(0,name.find('\0')));auto packet=zacket::MakeLocalMissionZone();packet.storage[2]=4;packet.storage[3]=0; // 원본 관찰기는 뒤의 duplicate 진입 때 길이를 확정한다.
    globals.next[0x54/4]=AddressWord(globals.localAddress,0);globals.next[0x58/4]=AddressWord(globals.localAddress,4);globals.next[0x5c/4]=(globals.next[0x5c/4]&0xffffff00U)|globals.localAddress[8];
    globals.next[0]=7;globals.next[0x7c/4]=globals.port;globals.next[0x80/4]=Need(hooks.duplicate)(packet);++globals.sequence;
}
// 호스트 포인터를DWORD로 자르지 않고 이 요청이 단독 소유한 실제 이미지의 핸들1을 사용한다.
std::uint32_t CaptureMissionFortZone(MissionFortLoadRequest& request,zacket::LocalMissionPacket& packet) {
    if (request.consumed || !request.zone.empty() || packet.storage.size()!=512) throw std::logic_error("Fort 요청 Zone 중복/예약 크기 오류");
    const auto length=15U+std::uint32_t(packet.storage[14])*71U;if (length>packet.storage.size()) throw std::out_of_range("Fort 요청 현재 Zone client 범위");packet.storage[2]=static_cast<std::uint8_t>(length);packet.storage[3]=static_cast<std::uint8_t>(length>>8);
    const auto bytes=packet.Bytes();request.zone.assign(bytes.begin(),bytes.end());return 1;
}
// Session이 아직 빌리는 이전 이미지의 실제 반납은 그 공유 소유자가 끝나는 시점에 맡긴다.
void ReleaseMissionFortLoad(State& pending,std::shared_ptr<MissionFortLoadRequest>& request) {
    pending.Cancel("LoadMissionFort");pending.Cancel("LoadMissionWorld");request.reset();
}
}
