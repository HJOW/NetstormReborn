// 원본 고정 길이 요청과 직접 서버 수신의 재조회/NaN 비교/환불 순서를 보존한다.
#include "o/RawConstructionCommand.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::size_t kCommandBytes=25,kReadBytes=17; // 전체 송신25와 수신이 실제 읽는 마지막 품질BYTE까지의 폭이다.
// 비정렬 원문을 지정 폭만 little endian으로 읽는다.
std::uint32_t Get(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 각 BYTE의 원래 자리를 보존한다.
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(bytes[offset+i])<<(i*8);return result;
}
// 비정렬 원문을 지정 폭만 쓰고 인접 필드는 유지한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본이 쓰는 BYTE/WORD/DWORD만 낮은 자리부터 기록한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
}
// 생성자의0시각과 로컬 배치의 나머지 필드를 같은 바이트 위치에 기록한다.
std::size_t WriteConstructionCommand(const ConstructionLocalMessage& message,std::span<std::uint8_t> packet,ConstructionCommandHeader header) {
    if (packet.size()<kCommandBytes) throw std::out_of_range("건설 요청25바이트 저장소 부족");
    packet[0]=header.protocol&0x7f;packet[1]=header.message;Put(packet,2,header.length,2);packet[4]=message.type;
    Put(packet,5,std::bit_cast<std::uint32_t>(message.x));Put(packet,9,std::bit_cast<std::uint32_t>(message.y));Put(packet,13,message.argument,2);packet[15]=message.direction;packet[16]=message.quality;
    Put(packet,17,message.timeLow);Put(packet,21,message.timeHigh);return kCommandBytes;
}
// 수신 계산에 필요한 실제 하위와 같은 PATCH 타입 표를 먼저 확인한다.
RawConstructionCommand::RawConstructionCommand(std::span<const RiftTypeRecord> types,const ConstructionCommandState& state,ConstructionCommandHooks hooks)
    :types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=188 || !hooks_.money || !hooks_.shortfall || !hooks_.request || !hooks_.reject) throw std::invalid_argument("실제 건설 요청 수신 연결 오류");
}
// 원본 sender DWORD/WORD 인자/BYTE 방향·품질을 유지하며 packet의 시각 자리는 사용하지 않는다.
ConstructionConfirmRequest RawConstructionCommand::Read(std::uint32_t sender,std::span<const std::uint8_t> packet) {
    return {packet[4],std::bit_cast<float>(Get(packet,5)),std::bit_cast<float>(Get(packet,9)),sender,Get(packet,13,2),packet[15],0,0,0,packet[16]};
}
// NaN 잔액/비용의 원본 선검사는 ordered less이며 후속 Place의 부족 판단과 구별한다.
void RawConstructionCommand::Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    if (state_.blocked) return;
    if (packet.size()<kReadBytes) throw std::out_of_range("건설 요청 수신 본문 부족");
    const auto type=packet[4];if (type>=types_.size()) throw std::out_of_range("건설 요청 현재 TYPE 범위 오류");
    if (!state_.chargeBlocked && sender!=state_.localPlayer) {
        const auto money=hooks_.money(sender); // 실제 잔액 경계 뒤 현재 TYPE 비용을 읽는다.
        if (money<types_[type].cost) { hooks_.shortfall(Read(sender,packet));return; }
    }
    if (!hooks_.request(Read(sender,packet))) hooks_.reject(Read(sender,packet));
}
// 실제 빈 observer는 종류만 등록하며 단일/집합 분배기의 원본 차이를 보존한다.
void RegisterConstructionCommand(RawPacketDispatch& dispatch,RawConstructionCommand& command,std::function<void()> prepare) {
    dispatch.Observe(0x3e,[](std::uint32_t) {});dispatch.Version(0x3e,1,0);dispatch.RequireLength(0x3e,25);
    dispatch.Register(0x3e,[&command,prepare=std::move(prepare)](std::uint32_t sender,PacketConnection&,std::span<const std::uint8_t> packet) { if (prepare) prepare();command.Receive(sender,packet); });
}
}
