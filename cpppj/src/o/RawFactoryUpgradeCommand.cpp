// 업그레이드 요청/수신의 현재 읽기와 할당 실패·단락 평가·원문 방송 순서를 보존한다.
#include "o/RawFactoryUpgradeCommand.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kRequestWait=20.0F; //00453bf0의 event4 SharedRegular payload다.
constexpr double kUpgradeDuration=15.0,kMinimumWait=1.0; //00508ed0의 요청 완료 간격과00501580의 수신 최소 지연이다.
constexpr std::size_t kPacketBytes=16; //같은58h 메시지의 실제 선언/전송 길이다.
// 패킷에서 선택된 필드의 실제 바이트만 물리 범위 검사 뒤 읽는다.
std::uint64_t Packet(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width) {
    if (at>raw.size() || raw.size()-at<width) throw std::out_of_range("Factory 업그레이드 수신 필드 범위");std::uint64_t value{};
    // 원본 little endian 필드만 낮은 자리부터 합친다.
    for (std::size_t index=0;index<width;++index) value|=std::uint64_t(raw[at+index])<<(index*8);return value;
}
// 실제 선택된 하위만 요구하며 누락을 빈 성공으로 처리하지 않는다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 생성 시 게임 효과나 장치 호출을 하지 않는다.
RawFactoryUpgradeCommand::RawFactoryUpgradeCommand(const SidPool& pool,std::span<const RiftTypeRecord> types,FactoryUpgradeCommandState state,FactoryUpgradeCommandHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.empty()) throw std::invalid_argument("Factory 업그레이드 같은 PATCH/TYPE 계약 누락"); }
// 같은 현재 슬롯의 지정BYTE를 읽는다.
std::uint32_t RawFactoryUpgradeCommand::Read(Sid factory,std::size_t offset) const { const auto raw=pool_.Slot(factory);if (offset>=raw.size()) throw std::out_of_range("Factory 업그레이드 현재 raw 범위");return raw[offset]; }
// 진단 효과 뒤 다른 TYPE가 되었으면 그 현재 genus로 다시 판정한다.
std::uint32_t RawFactoryUpgradeCommand::Genus(Sid factory) const { const auto type=Read(factory,10);if (type>=types_.size()) throw std::out_of_range("Factory 업그레이드 현재 TYPE 범위");return types_[type].flags2; }
// 원본 자체에는 비용/소유자/이미 진행 중인지 검사가 없다. 그런 관문은 입력 부모에 있다.
void RawFactoryUpgradeCommand::Request(Sid factory) const {
    if (Need(hooks_.reserveShared,"Factory 업그레이드 SharedRegular 예약 미연결")()) Need(hooks_.addShared,"Factory 업그레이드 SharedRegular 실제 부착 미연결")(factory,4,kRequestWait);
    std::array<std::uint8_t,kPacketBytes> packet{};WriteFactoryResearchCommand(factory,0,state_.now+kUpgradeDuration,packet);packet[7]=1;Need(hooks_.send,"Factory 업그레이드 실제 요청 송신 미연결")(9,packet);
}
// 유효한업그레이드 분기는 sender/owner 일치를 추가하지 않는다. 원본 연구 분기와 구별한다.
void RawFactoryUpgradeCommand::Receive(std::span<const std::uint8_t> packet) const {
    const auto factory=Sid{static_cast<std::uint16_t>(Packet(packet,4,2))};if (Read(factory,11)&3U) return;if (Packet(packet,6,1) || Packet(packet,7,1)!=1) return;
    if (state_.checking && !(Genus(factory)&0x4000U)) Need(hooks_.assertion,"Factory 업그레이드 genus 진단 미연결")("factory.getGenus() & gFACTORY");if (!(Genus(factory)&0x4000U)) return;
    const auto cost=Need(hooks_.cost,"Factory 업그레이드 현재 비용 조회 미연결")(Read(factory,10));const auto delta=static_cast<float>(-static_cast<double>(cost));Need(hooks_.addSp,"Factory 업그레이드 같은 실제 SP 가산 미연결")(Read(factory,34),delta);Need(hooks_.log,"Factory 업그레이드 원본 예약 로그 미연결")(4,"new ftUPGRADE sharedRegPro\n");
    // 실제 x87 test ah,41 분기는 unordered에서도 원래 차이를 선택하므로 NaN을1초로 바꾸지 않는다.
    if (Need(hooks_.reserveShared,"Factory 업그레이드 수신 SharedRegular 예약 미연결")()) { const auto due=std::bit_cast<double>(Packet(packet,8,8));const auto difference=due-state_.now;const auto wait=static_cast<float>(difference<kMinimumWait ? kMinimumWait : difference);Need(hooks_.addShared,"Factory 업그레이드 수신 SharedRegular 실제 부착 미연결")(factory,3,wait); }
    if (state_.boss) { if (packet.size()<kPacketBytes) throw std::out_of_range("Factory 업그레이드 원문16바이트 방송 범위");Need(hooks_.broadcast,"Factory 업그레이드 실제 원문 방송 미연결")(packet.first(kPacketBytes)); }
}
// 실제 Session 조립의 동일 풀이다.
const SidPool& RawFactoryUpgradeCommand::Pool() const { return pool_; }
}
