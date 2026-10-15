// payload 순서와 다른 실제 전역 조회 순서 및 원본 DWORD 비트를 보존한다.
#include "o/RawMissionSettingsPacket.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// packet의 비정렬 DWORD를 원본 little endian으로 쓴다.
void SettingsDword(std::span<std::uint8_t> packet,std::size_t offset,std::uint32_t value) {
    // 각 현재 DWORD의 네 BYTE를 낮은 위치부터 기록한다.
    for (std::size_t byte=0;byte<4;++byte) packet[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 수신의 현재 원본 DWORD를 읽되 실제 물리 부족을 범위 밖 읽기로 바꾸지 않는다.
std::uint32_t SettingsRead(std::span<const std::uint8_t> packet,std::size_t offset) {
    if (offset>packet.size() || packet.size()-offset<4) throw std::out_of_range("미션 설정 패킷의 현재 DWORD 물리 범위 부족");std::uint32_t value{};
    // 현재 네 BYTE를 낮은 자리부터 조립한다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(packet[offset+byte])<<(byte*8);return value;
}
}
// 실제 현재 설정이 누락된 구성을 성공상수로 채우지 않는다.
RawMissionSettingsPacket::RawMissionSettingsPacket(OriginalEdition edition,MissionSettingsPacketHooks hooks):hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || !hooks_.current) throw std::invalid_argument("미션 설정 패킷의 PATCH 현재 설정 조회 누락");
}
// 원문 asm의 load 순서는1,2,0,4,5,3,6이며 읽은 값은 다음 callback 변화에도 보존된다.
void RawMissionSettingsPacket::Send(std::int32_t target) const {
    std::array<std::uint8_t,32> packet{};packet[0]=1;packet[1]=0x32;packet[2]=32;
    const auto hack=hooks_.current(MissionSettingsField::HackMover);
    const auto noDamage=hooks_.current(MissionSettingsField::NoDamageDebug);
    const auto generator=hooks_.current(MissionSettingsField::GeneratorTimer);
    SettingsDword(packet,8,hack);
    const auto fill=hooks_.current(MissionSettingsField::FillMyTank);
    SettingsDword(packet,12,noDamage);
    const auto columns=hooks_.current(MissionSettingsField::CombatGumpColumns);
    SettingsDword(packet,4,generator);
    const auto shoot=hooks_.current(MissionSettingsField::ShootMyFriends);
    SettingsDword(packet,20,fill);SettingsDword(packet,24,columns);SettingsDword(packet,16,shoot);
    const auto rows=hooks_.current(MissionSettingsField::CombatGumpRows);SettingsDword(packet,28,rows);
    if (!hooks_.send) throw std::logic_error("미션 설정 패킷의 실제 현재 연결 송신 미연결");hooks_.send(target,packet);
}
// 각 원본 store 뒤 외부효과가 나머지packet을 바꾸더라도 다음 DWORD는 현재bytes에서 읽는다.
void RawMissionSettingsPacket::Receive(std::span<const std::uint8_t> packet) const {
    // 실제payload순서0..6으로 쓰며 송신의 서로 다른 read순서를 수신에 적용하지 않는다.
    for (std::uint8_t index=0;index<7;++index) {
        const auto value=SettingsRead(packet,4+std::size_t(index)*4);
        if (!hooks_.setCurrent) throw std::logic_error("미션 설정 패킷의 실제 현재 설정 쓰기 미연결");hooks_.setCurrent(static_cast<MissionSettingsField>(index),value);
    }
}
// 원본observer0040b380은 인자를 버리는ret4뿐이며 송신/수신 모두 추가효과 없이 자연반환한다.
void RegisterMissionSettingsPacket(RawPacketDispatch& dispatch,RawMissionSettingsPacket& packet) {
    dispatch.Observe(0x32,[](std::uint32_t) {});dispatch.Version(0x32,1,0);
    dispatch.Register(0x32,[&packet](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> bytes) { packet.Receive(bytes); });
}
}
