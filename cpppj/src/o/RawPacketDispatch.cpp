// 일반 분배의 현재 시각/기록/콜백 순서와 버전 차이를 무시하는 원본 정책을 유지한다.
#include "o/RawPacketDispatch.h"
#include "o/RawPacketLengths.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 단일 패킷 헤더의 선언 길이 WORD다. caller가 최소 4바이트를 확인한 뒤 읽는다.
std::uint16_t Length(std::span<const std::uint8_t> packet) { return static_cast<std::uint16_t>(packet[2]|(static_cast<std::uint16_t>(packet[3])<<8)); }
}
RawPacketDispatch::RawPacketDispatch(OriginalEdition edition,PacketDispatchHooks hooks)
    :lastType_(edition==OriginalEdition::Patch1078 ? 137U : 126U),hooks_(std::move(hooks)) {
    if (!hooks_.now || !hooks_.record || !hooks_.terminate || !hooks_.close || !hooks_.release) throw std::invalid_argument("패킷 분배 경계 누락");
    // 최초 길이 표는 원본 PE의 WORD를 그대로 복사하고 함수/버전 표는 0으로 둔다.
    for (std::size_t i=0;i<=lastType_;++i) entries_[i].length=edition==OriginalEdition::Patch1078 ? kPatchPacketLengths[i] : kCdPacketLengths[i];
}
void RawPacketDispatch::CheckType(std::uint32_t type) const { if (!type || type>lastType_) throw std::out_of_range("패킷 등록 타입 범위 오류"); }
void RawPacketDispatch::Register(std::uint32_t type,decltype(PacketRegistration::receive) receive) { CheckType(type);entries_[type].receive=std::move(receive); }
void RawPacketDispatch::Observe(std::uint32_t type,decltype(PacketRegistration::observer) observer) { CheckType(type);entries_[type].observer=std::move(observer);entries_[type].packetObserver={}; }
// 두 호스트 표현은 같은 원문 슬롯이므로 새 등록이 이전 장치를 완전히 교체한다.
void RawPacketDispatch::ObservePacket(std::uint32_t type,decltype(PacketRegistration::packetObserver) observer) { CheckType(type);entries_[type].packetObserver=std::move(observer);entries_[type].observer={}; }
// 실제 this를 사용하는 장치와 기존 방향 전용 장치를 중복 실행하지 않는다.
void PacketRegistration::NotifyObserver(std::span<std::uint8_t> packet,std::uint32_t sending) const { const auto withPacket=packetObserver;if (withPacket) withPacket(packet,sending);else { const auto directionOnly=observer;if (directionOnly) directionOnly(sending); } } // callback이 같은 등록표를 교체해도 이미 읽은 함수의 수명을 유지한다.
void RawPacketDispatch::Version(std::uint32_t type,std::uint8_t version,std::uint8_t mismatch) { CheckType(type);entries_[type].version=version;entries_[type].mismatch=mismatch; }
void RawPacketDispatch::RequireLength(std::uint32_t type,std::uint16_t length) { CheckType(type);entries_[type].length=length; }
const PacketRegistration& RawPacketDispatch::Registration(std::uint32_t type) const { CheckType(type);return entries_[type]; }
bool RawPacketDispatch::Validate(std::span<const std::uint8_t> packet) const {
    if (packet.size()<4) throw std::invalid_argument("패킷 헤더 길이 부족");const auto type=packet[1];
    if (!type || type>lastType_) return false;const auto& entry=entries_[type];
    return (type<=9 || entry.receive) && (entry.length==0xffff || entry.length==Length(packet));
}
bool RawPacketDispatch::Dispatch(PacketConnection& connection,std::span<const std::uint8_t> packet) const {
    if (connection.flags&0x40) return false;
    if (!Validate(packet)) return true;const auto type=packet[1];
    if (type==4 || connection.delayed) throw std::logic_error("집합 패킷/지연 분배는 아직 지원하지 않습니다");
    const auto incoming=packet[0]&0x7f;const auto& entry=entries_[type];
    if (incoming!=entry.version) {
        const bool lower=incoming<entry.version;
        if (entry.mismatch&(lower ? 8 : 4)) { hooks_.terminate(connection);return false; }
        if (entry.mismatch&(lower ? 2 : 1)) return true;
    }
    const auto now=hooks_.now();if (type==6) connection.heartbeat=now;else connection.received=now;
    if (type>9) hooks_.record(connection,packet,Length(packet));
    // 기록 경계가 등록을 바꾸면 현재 처리기를 사용한다. 콜백의 재등록 중에도 실행 중인 함수 저장소를 유지한다.
    const auto receive=entry.receive;
    if (receive) {
        ++connection.depth;
        try { receive(connection.sender,connection,packet); }
        catch (...) { --connection.depth;throw; }
        --connection.depth;
    }
    if (!connection.depth && (connection.flags&0x40)) { hooks_.close(connection);hooks_.release(connection);return false; }
    return true;
}
void RegisterConstructionData(RawPacketDispatch& dispatch,RawConstructionData& data) {
    dispatch.Observe(0x37,{});dispatch.Version(0x37,1,0);
    // 원본 처리기의 sender/연결 인자는 건설 수신 몸체에서 사용하지 않는다.
    dispatch.Register(0x37,[&data](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> packet) { data.Receive(packet); });
}
}
