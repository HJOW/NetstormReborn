// 원본 Player의 현재 필드 재조회와 두 metadata 수신/송신 분기 순서를 보존한다.
#include "o/RawMissionServerPackets.h"
#include "o/RawMissionClientPackets.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::size_t kStride=180,kCount=9,kPlayerPacketReserve=100; // 실제 Player와 송신 스택 예약 폭이다.
// 선택된 실제 경계가 없으면 이전 부분 효과를 보존한 채 오류를 반환한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("미션 서버 패킷의 실제 하위 미연결");return function; }
// 현재 물리 원문의 지정 폭만 읽는다.
std::uint32_t Get(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("미션 서버 패킷 읽기 물리 범위");std::uint32_t result=0;
    // 낮은 바이트부터 실제 little endian 비트를 결합한다.
    for (std::size_t index=0;index<width;++index) result|=std::uint32_t(bytes[offset+index])<<(index*8);return result;
}
// 주변 stale 바이트를 보존하며 실제 지정 BYTE/WORD/DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("미션 서버 패킷 쓰기 물리 범위");
    // 원본 지정 폭의 낮은 바이트만 변경한다.
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// NUL까지 복사하되 잘못된 이름이 실제 host 저장소 밖에 쓰는 것은 거부한다.
void Copy(std::span<std::uint8_t> bytes,std::size_t offset,std::string_view value) {
    value=value.substr(0,value.find('\0'));if (offset>=bytes.size() || value.size()>=bytes.size()-offset) throw std::out_of_range("미션 서버 이름 복사 물리 범위");
    std::copy(value.begin(),value.end(),bytes.begin()+static_cast<std::ptrdiff_t>(offset));bytes[offset+value.size()]=0;
}
// 원본004df590의 전체ff 예약과 실제version/type/초기헤더길이다.
std::vector<std::uint8_t> Packet(std::uint8_t type,std::size_t size,std::uint16_t length) {
    std::vector<std::uint8_t> packet(size,0xff);packet[0]=1;packet[1]=type;Put(packet,2,length,2);return packet;
}
}
// 모든 실제 참가자가 공유하는 동일9기록/Zone 주소만 허용한다.
RawMissionServerPackets::RawMissionServerPackets(std::span<std::uint8_t> players,MissionServerPacketState state,MissionServerPacketHooks hooks)
    :players_(players),state_(state),hooks_(std::move(hooks)) { if (players.size()!=kStride*kCount || state.zoneAddress.size()!=6) throw std::invalid_argument("미션 서버의 같은 Player9/Zone6BYTE 누락"); }
//0048f550의 현재 metamap 진단은 각 번호 조회마다 다시 실행한다.
std::uint32_t RawMissionServerPackets::Owner(std::uint32_t owner) const {
    if (state_.metamap) Need(hooks_.assertion)("!inMetamapMode");if (owner>=kCount) throw std::out_of_range("미션 서버 현재 Player 번호 범위");return owner&0xffU;
}
// 원본의 현재 Player 저장소를 복제하지 않고 필요한 폭만 읽는다.
std::uint32_t RawMissionServerPackets::Read(std::uint32_t owner,std::size_t offset,std::size_t width) const {
    if (owner>=kCount || offset+width>kStride) throw std::out_of_range("미션 서버 현재 Player 필드 범위");return Get(players_,owner*kStride+offset,width);
}
// 이름 정제/현재 주소 하위가 바꾼 flags/동맹/필드를 후속 원본 위치에서 다시 읽는다.
void RawMissionServerPackets::SendPlayer(std::uint32_t owner,std::int32_t target) const {
    if (!state_.server) Need(hooks_.assertion)("server");auto packet=Packet(0x2b,kPlayerPacketReserve,4);packet[0x32]=static_cast<std::uint8_t>(Owner(owner));
    auto name=players_.subspan(owner*kStride+12);SanitizeMissionPlayerName(Read(owner,0x2c),name);
    const auto end=std::find(name.begin(),name.end(),std::uint8_t{});if (end==name.end()) throw std::out_of_range("미션 서버 현재 Player 이름 NUL 누락");
    const auto length=static_cast<std::size_t>(end-name.begin());packet[0x33]=static_cast<std::uint8_t>(length+1);Copy(packet,0x3b,{reinterpret_cast<const char*>(name.data()),length});Put(packet,0x26,Read(owner,0x2c));
    if (!(Read(owner,0x50,1)&1U)) packet[4]=0;
    else { const auto address=Need(hooks_.peerAddress)(Owner(owner));Copy(packet,4,address); }
    Put(packet,0x2a,Read(owner,0x50));Put(packet,0x22,Read(owner,0x54));packet[0x34]=static_cast<std::uint8_t>(Read(owner,0x5c,1));Put(packet,0x35,Read(owner,0x60,2),2);
    packet[0x37]=static_cast<std::uint8_t>(Read(owner,0xa0,1));packet[0x38]=static_cast<std::uint8_t>(Read(owner,0xa4,1));packet[0x39]=static_cast<std::uint8_t>(Read(owner,0xa5,1));packet[0x3a]=static_cast<std::uint8_t>(Read(owner,0x71,1));
    Put(packet,2,0x3bU+packet[0x33],2);Need(hooks_.send)(target,std::span<const std::uint8_t>(packet).first(Get(packet,2,2)));
}
// 본문 계산을 생략한 성공을 만들지 않고 미션 Zone의 실제 자연 반환만 제공한다.
void RawMissionServerPackets::ReceiveMetadata(std::span<const std::uint8_t> packet) const {
    if (!(Get(state_.zoneAddress,0)&0x100U)) { Need(hooks_.log)("No Central Island.\n");return; }
    if (packet.size()<=10) throw std::out_of_range("중앙섬 metadata 원문 부족");if (packet[10]=='$') throw std::logic_error("실제 중앙섬 Archipelago 생성 미복원");
}
// 실제$Random 작성→현재 자기 처리→현재 로컬 번호 비교→다른 대상 송신을 유지한다.
void RawMissionServerPackets::SendMetadata(std::int32_t target) const {
    constexpr std::string_view name="$Random"; // 원본00511790의 실제 고정 좁은문자열이다.
    auto packet=Packet(0x72,name.size()+11,4);packet[4]=0;Put(packet,6,static_cast<std::uint32_t>(name.size()+1));Copy(packet,10,name);packet[5]=static_cast<std::uint8_t>(state_.archipelago);
    if (!state_.centralCreated) { if (!state_.server) Need(hooks_.assertion)("server");ReceiveMetadata(packet); }
    if (static_cast<std::uint32_t>(target)!=state_.localOwner) { Put(packet,2,Get(packet,6,2)+10,2);Need(hooks_.send)(target,std::span<const std::uint8_t>(packet).first(Get(packet,2,2))); }
}
// 원본이 실제 등록한0x72 observer는0040b110이며 길이는현재+6 WORD+10이다.
void RegisterMissionMetadata(RawPacketDispatch& dispatch,RawMissionServerPackets& packets) {
    dispatch.Version(0x72,1,0);dispatch.Observe(0x72,[](std::uint32_t sending) {
        if (sending) throw std::logic_error("metadata 송신 observer는 실제 패킷을 가진 writer에서 실행해야 합니다"); // 현재 등록표에는 packet 인자가 없어 길이 쓰기를 생략한 성공을 반환하지 않는다.
    });
    dispatch.Register(0x72,[&packets](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> bytes) { packets.ReceiveMetadata(bytes); });
}
}
