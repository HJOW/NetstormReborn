// 두 판본의 float cost 차감·WORD 감김·난수 소비 및 고갈 효과 순서를 보존한다.
#include "o/RawGeyserMined.h"
#include "o/RawPacketDispatch.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kSoundChoices=6000,kSoundMinimum=1000,kGeyserDestroyFlags=0x1000; // 원본 난수 구간[1000,7000)과 고갈 삭제 flags다.
constexpr double kInt64Boundary=9223372036854775808.0; // __ftol이 불확정 signed64 값을 반환하는 범위 경계다.
}
// 입력 판본/필수 경계가 없으면 raw 수량이나 난수를 바꾸기 전에 거부한다.
RawGeyserMined::RawGeyserMined(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    GameRandom& random,GeyserMinedState& state,GeyserMinedHooks hooks)
    :pool_(pool),hash_(hash),types_(types),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.sound || !hooks_.destroy || !hooks_.create || !hooks_.pop)
        throw std::invalid_argument("가이저 채집 타입/효과 연결 오류");
}
// 비정렬 float 비트를 little endian 바이트 순서대로 조합한다.
float RawGeyserMined::Coordinate(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t bits=0;
    // 지정 좌표 DWORD 외의 필드는 읽지 않는다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 두 판본 모두 +12의 unsigned WORD를 경제 수량으로 읽는다.
std::uint16_t RawGeyserMined::Quantity(std::span<const std::uint8_t> bytes) { return static_cast<std::uint16_t>(bytes[12]|(static_cast<std::uint16_t>(bytes[13])<<8)); }
// float32 cost와 정확한 WORD 값의 double 차이를0방향으로 절삭한 뒤 하위16비트를 보존한다.
std::uint16_t RawGeyserMined::ReducedQuantity(std::uint16_t quantity,float cost) {
    const double result=static_cast<double>(quantity)-static_cast<double>(cost);
    if (!std::isfinite(result) || result>=kInt64Boundary || result< -kInt64Boundary) return 0;
    return static_cast<std::uint16_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(result)));
}
// 실제 __ftol 반환의 EAX만 읽는 좌표 caller를 보존한다. NaN/무한/범위 밖 값은 EAX0이다.
std::int32_t RawGeyserMined::CoordinateInteger(float coordinate) {
    const double value=coordinate;
    if (!std::isfinite(value) || value>=kInt64Boundary || value< -kInt64Boundary) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본 setter는 다른 수량/상태/소유자 필드를 변경하지 않는다.
void RawGeyserMined::SetQuantity(Sid sid,std::uint16_t quantity) const {
    auto bytes=pool_.AllocatedBytes(sid);bytes[12]=static_cast<std::uint8_t>(quantity);bytes[13]=static_cast<std::uint8_t>(quantity>>8);
    if (pool_.Edition()==OriginalEdition::Patch1078 && bytes[10]==state_.bridgeType && !(bytes[40]&8) && state_.checking && ((bytes[12]>>3)&15)>7)
        throw std::logic_error("Bridge 수량 num 원본 assert");
}
// 원본 island getter의 편향 없는 ftol 좌표와 같은 현재 표면 타입 비교다.
bool RawGeyserMined::OnIsle(float x,float y) const {
    const auto ix=CoordinateInteger(x),iy=CoordinateInteger(y);
    if (ix<0 || iy<0 || ix>=256 || iy>=256) return false;
    const Sid surface{hash_.Cell(0,static_cast<int>(ix),static_cast<int>(iy))};return surface.value && pool_.Slot(surface)[10]==state_.isleType;
}
// 음향→현재 수량/cost→setter→현재 boss/잔량0→삭제→현재 표면 검사→emptyGeyser 생성/Pop이다.
void RawGeyserMined::Handle(Sid geyser) const {
    const auto raw=pool_.Slot(geyser);if (raw[11]&1) return;
    const auto choice=(random_.Next(kSoundChoices)+kSoundMinimum)/1000;
    const auto sound="geyserMined"+std::to_string(choice)+".wav";
    hooks_.sound(Coordinate(raw,14),Coordinate(raw,18),sound,0);
    if (state_.nuggetType>=types_.size()) throw std::out_of_range("가이저 채집 현재 nugget 타입 범위");
    SetQuantity(geyser,ReducedQuantity(Quantity(pool_.Slot(geyser)),types_[state_.nuggetType].cost));
    if (!state_.boss || Quantity(pool_.Slot(geyser))!=0) return;
    const auto current=pool_.Slot(geyser);const float x=Coordinate(current,14),y=Coordinate(current,18);
    hooks_.destroy(geyser,kGeyserDestroyFlags);
    if (!OnIsle(x,y)) { const auto next=hooks_.create(state_.emptyGeyserType,0);hooks_.pop(next,x,y,0); }
}
// packet SID 검사는 byte 범위만 수행하고 free 여부는 실제 handler가 현재 raw에서 검사한다.
void RawGeyserMined::HandlePacket(std::span<const std::uint8_t> packet) const {
    if (packet.size()<6) throw std::out_of_range("가이저 채집 packet SID 범위");
    Handle(Sid{static_cast<std::uint16_t>(packet[4]|(static_cast<std::uint16_t>(packet[5])<<8))});
}
// 생성/packet 어댑터의 같은 월드 확인용 참조다.
const SidPool& RawGeyserMined::Pool() const { return pool_; }
// 실제 빈 원본 observer는 효과가 없으므로 별도 함수 없이 등록 표를 비운다. 단일 분배는 이 표를 호출하지 않는다.
void RegisterGeyserMined(RawPacketDispatch& dispatch,RawGeyserMined& handler) {
    dispatch.Observe(0x5b,{});dispatch.Version(0x5b,1,0);
    // 원본 handler는 sender/연결을 읽지 않고 packet+4 SID의 현재 raw 슬롯만 읽는다.
    dispatch.Register(0x5b,[&handler](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> packet) { handler.HandlePacket(packet); });
}
}
