// 일반 분배의 현재 시각/기록/콜백 순서와 버전 차이를 무시하는 원본 정책을 유지한다.
#include "o/RawPacketDispatch.h"
#include "o/RawPacketLengths.h"
#include "o/PacketRegistrationTable.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
//원문함수주소표식과선택된실제본문을한번구성한뒤바꾸지않는다.
CanonicalPacketReceiver::CanonicalPacketReceiver(std::uint32_t entry,Receive body):nativeEntry(entry),receive(std::move(body)) { if (!nativeEntry || !receive) throw std::invalid_argument("고정 Native receiver 정체성/본문 누락"); }
//등록표가F에서G로바뀌어도이객체의저장된F만호출한다. 현재자료/Runtime선택은F본문의원래위치에남긴다.
void CanonicalPacketReceiver::Invoke(std::uint32_t sender,PacketConnection* connection,std::span<const std::uint8_t> packet) const { receive(sender,connection,packet); }
bool PacketPlayerActive(std::span<const std::uint8_t> fallback,const std::function<std::span<const std::uint8_t>()>& current,std::uint32_t owner) { const auto players=current ? current():fallback;if (players.size()!=9U*180U || owner>=9) throw std::out_of_range("송신의 실제 현재 Player9 물리 부족");return (players[owner*180U+0x50U]&1U)!=0; } //현재 base를 한 번 조회하고 active의 낮은BYTE만 읽으며 별도 known/phase 조건을 더하지 않는다.
namespace {
// 단일 패킷 헤더의 선언 길이 WORD다. caller가 최소 4바이트를 확인한 뒤 읽는다.
std::uint16_t Length(std::span<const std::uint8_t> packet) { return static_cast<std::uint16_t>(packet[2]|(static_cast<std::uint16_t>(packet[3])<<8)); }
}
RawPacketDispatch::RawPacketDispatch(OriginalEdition edition,PacketDispatchHooks hooks,std::shared_ptr<PacketRegistrationTable> registrations)
    :registrations_(registrations ? std::move(registrations):std::make_shared<PacketRegistrationTable>(edition)),lastType_(edition==OriginalEdition::Patch1078 ? 137U : 126U),hooks_(std::move(hooks)) {
    if (!hooks_.now || !hooks_.record || !hooks_.terminate || !hooks_.close || !hooks_.release || registrations_->LastType()!=lastType_) throw std::invalid_argument("패킷 분배 경계/공유 등록표 판본 불일치");
}
// 등록표만 공유하며 실제 분배의 시계/record/close/free는 backend 고유 장치를 사용한다.
const std::shared_ptr<PacketRegistrationTable>& RawPacketDispatch::Registrations() const { return registrations_; }
void RawPacketDispatch::CheckType(std::uint32_t type) const { if (!type || type>lastType_) throw std::out_of_range("패킷 등록 타입 범위 오류"); }
void RawPacketDispatch::Register(std::uint32_t type,decltype(PacketRegistration::receive) receive) { CheckType(type);registrations_->Touch(type);auto& entry=registrations_->entries_[type];entry.receive=std::move(receive);entry.directReceive={};entry.canonicalReceive.reset(); } // 일반 재등록도 같은 이전 nullable 수신기를 제거한다.
void RawPacketDispatch::RegisterDirect(std::uint32_t type,decltype(PacketRegistration::directReceive) receive) {
    CheckType(type);if (!receive) { registrations_->Touch(type);auto& entry=registrations_->entries_[type];entry.receive={};entry.directReceive={};entry.canonicalReceive.reset();return; }
    const auto shared=std::make_shared<decltype(receive)>(std::move(receive)); // 두 입구가 mutable callable의 같은 실제 상태를 사용하며 dispatch 객체 this를 빌리지 않는다.
    decltype(PacketRegistration::directReceive) direct=[shared](auto sender,auto* connection,auto packet) { (*shared)(sender,connection,packet); };
    decltype(PacketRegistration::receive) framed=[shared](auto sender,auto& connection,auto packet) { (*shared)(sender,&connection,packet); };
    registrations_->Touch(type);auto& entry=registrations_->entries_[type];entry.directReceive=std::move(direct);entry.receive=std::move(framed);entry.canonicalReceive.reset(); // 모든 할당을 준비한 뒤 같은 lease/등록을 noexcept 이동으로 교체한다.
}
//알려진고정본문값을만든뒤같은현재슬롯의즉시입구와정체성을함께등록한다.
void RawPacketDispatch::RegisterCanonicalDirect(std::uint32_t type,std::uint32_t nativeEntry,CanonicalPacketReceiver::Receive receive) { RegisterCanonical(type,std::make_shared<CanonicalPacketReceiver>(nativeEntry,std::move(receive))); }
//인자가같은현재slot의참조여도선택된F를먼저복사해등록교체와분리한다.
void RawPacketDispatch::RegisterCanonical(std::uint32_t type,const std::shared_ptr<const CanonicalPacketReceiver>& receive) {
    CheckType(type);const auto selected=receive;if (!selected) { RegisterDirect(type,{});return; } //원문의NULL함수와미전환shortclosure를구별한다.
    RegisterDirect(type,[selected](auto sender,auto* connection,auto packet) { selected->Invoke(sender,connection,packet); });registrations_->entries_[type].canonicalReceive=selected;
}
//큐는이값만보관해야하며이전PacketRegistration/lease를대기수명동안보유하지않는다.
std::shared_ptr<const CanonicalPacketReceiver> RawPacketDispatch::SnapshotReceiver(std::uint32_t type) const {
    CheckType(type);const auto& entry=registrations_->entries_[type];if (!entry.receive && !entry.directReceive) return {}; //원문NULLreceiver의delay0자연반환을유지한다.
    if (!entry.canonicalReceive) throw std::logic_error("지연 receiver의 장기 고정본문 전환 미완료");return entry.canonicalReceive; //짧은oldSessionclosure를안전한F로포장하지않는다.
}
void RawPacketDispatch::InvokeDirect(std::uint32_t type,std::uint32_t sender,PacketConnection* connection,std::span<const std::uint8_t> packet) const {
    CheckType(type);const auto& entry=registrations_->entries_[type];if (!entry.receive && !entry.directReceive) return;const auto pin=entry.Pin();const auto direct=entry.directReceive;
    if (direct) { direct(sender,connection,packet);return; }const auto receive=entry.receive;if (!connection) throw std::invalid_argument("NULL 연결을 지원하지 않는 일반 수신기 직접 호출");if (receive) receive(sender,*connection,packet);
}
void RawPacketDispatch::Observe(std::uint32_t type,decltype(PacketRegistration::observer) observer) { CheckType(type);registrations_->Touch(type);registrations_->entries_[type].observer=std::move(observer);registrations_->entries_[type].packetObserver={}; }
// 두 호스트 표현은 같은 원문 슬롯이므로 새 등록이 이전 장치를 완전히 교체한다.
void RawPacketDispatch::ObservePacket(std::uint32_t type,decltype(PacketRegistration::packetObserver) observer) { CheckType(type);registrations_->Touch(type);registrations_->entries_[type].packetObserver=std::move(observer);registrations_->entries_[type].observer={}; }
// 실제 this를 사용하는 장치와 기존 방향 전용 장치를 중복 실행하지 않는다.
void PacketRegistration::NotifyObserver(std::span<std::uint8_t> packet,std::uint32_t sending) const { const auto pin=Pin();const auto withPacket=packetObserver;if (withPacket) withPacket(packet,sending);else { const auto directionOnly=observer;if (directionOnly) directionOnly(sending); } } // callback이 같은 등록표를 교체해도 이미 읽은 함수의 수명을 유지한다.
void RawPacketDispatch::Version(std::uint32_t type,std::uint8_t version,std::uint8_t mismatch) { CheckType(type);registrations_->Touch(type);registrations_->entries_[type].version=version;registrations_->entries_[type].mismatch=mismatch; }
void RawPacketDispatch::RequireLength(std::uint32_t type,std::uint16_t length) { CheckType(type);registrations_->Touch(type);registrations_->entries_[type].length=length; }
const PacketRegistration& RawPacketDispatch::Registration(std::uint32_t type) const { CheckType(type);return registrations_->entries_[type]; }
bool RawPacketDispatch::Validate(std::span<const std::uint8_t> packet) const {
    if (packet.size()<4) throw std::invalid_argument("패킷 헤더 길이 부족");const auto type=packet[1];
    if (!type || type>lastType_) return false;const auto& entry=registrations_->entries_[type];
    return (type<=9 || entry.receive) && (entry.length==0xffff || entry.length==Length(packet));
}
bool RawPacketDispatch::Dispatch(PacketConnection& connection,std::span<const std::uint8_t> packet) const {
    if (connection.flags&0x40) return false;
    if (!Validate(packet)) return true;const auto type=packet[1];
    if (type==4 || connection.delayed) throw std::logic_error("집합 패킷/지연 분배는 아직 지원하지 않습니다");
    const auto incoming=packet[0]&0x7f;const auto& entry=registrations_->entries_[type];
    if (incoming!=entry.version) {
        const bool lower=incoming<entry.version;
        if (entry.mismatch&(lower ? 8 : 4)) { hooks_.terminate(connection);return false; }
        if (entry.mismatch&(lower ? 2 : 1)) return true;
    }
    const auto recordPin=entry.Pin(); // record/시계 효과 안의 State 전환도 같은 전체 Runtime 정리 관문을 거친다.
    const auto now=hooks_.now();if (type==6) connection.heartbeat=now;else connection.received=now;
    if (type>9) hooks_.record(connection,packet,Length(packet));
    // 기록 경계가 등록을 바꾸면 현재 처리기를 사용한다. 콜백의 재등록 중에도 실행 중인 함수 저장소를 유지한다.
    const auto pin=entry.Pin();const auto receive=entry.receive; // 실제 Session 전체와 함수 사본을 함께 유지한다.
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
