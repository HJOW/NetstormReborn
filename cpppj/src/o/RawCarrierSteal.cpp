// 원본의 역방향 동맹/원소유자/현재 내용물 조회와 로컬 선행 효과를 유지한다.
#include "o/RawCarrierSteal.h"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된 실제 경계가 없으면 빈 성공으로 대체하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("크리스탈 크랩 포획 실제 하위 미연결");return hook; }
// 비정렬WORD/DWORD를 원본 폭만큼 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 원본 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 지정WORD의 낮은 바이트를 기록한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); }
// 원본4202f0의41d770 signed64 절삭과 하위signed32를 보존한다.
std::int32_t Cell(float value) {
    if (!std::isfinite(value) || std::fabs(static_cast<double>(value))>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
//10.72의 다른 포획 본문을 이 몸체와 같다고 추정하지 않는다.
RawCarrierSteal::RawCarrierSteal(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const bool& boss,const std::uint32_t& editor,const std::uint32_t& localOwner,
    std::span<const std::uint32_t> alliances,const std::uint32_t& carrierType,
    const std::uint32_t& priestType,const FormPacketHeader& header,std::uint32_t& haltCounter,CarrierStealHooks hooks)
    :pool_(pool),hash_(hash),types_(types),boss_(boss),editor_(editor),localOwner_(localOwner),
    alliances_(alliances),carrierType_(carrierType),priestType_(priestType),header_(header),haltCounter_(haltCounter),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || alliances.size()!=81) throw std::invalid_argument("크리스탈 크랩 포획 판본/현재표 오류");
}
// 타입번호가 같아도 다른 가상 표를 이 전용 몸체로 자동 분배하지 않는다.
bool RawCarrierSteal::Handles(Sid parent) const { return Read(pool_.Slot(parent),0)==0x503f20U; }
// 최초의extra/flags 관문 뒤 현재boss를 읽으며 파생예약 뒤 공통 장부를 한 번 실행한다.
void RawCarrierSteal::PostPop(Sid parent,std::uint32_t flags) const {
    if (!(pool_.Slot(parent)[40]&9) && (flags&1) && Need(hooks_.allocateRegular)()) Need(hooks_.regular)(parent,0x492,1.0F);
    if (!(flags&1) && !boss_) Need(hooks_.check)(parent);
    Need(hooks_.damageablePostPop)(parent,flags);
}
// incDWORD 감김 뒤signed%4이며 모든 부모가 같은counter를 사용한다.
std::uint32_t RawCarrierSteal::Halt(Sid parent) const {
    haltCounter_=std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(haltCounter_+1U)%4);
    return haltCounter_==0 ? Try(parent) : Need(hooks_.commonHalt)(parent);
}
// free만 검사하고 dead/소유자/기증자 필터를 추가하지 않는다. 부모WORD는 효과 뒤 다시 읽는다.
void RawCarrierSteal::Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const {
    (void)owner;(void)flags;if (packet.size()<8) throw std::invalid_argument("크리스탈 크랩 포획 명령 길이 오류");
    const Sid content{static_cast<std::uint16_t>(Read(packet,6,2))};
    if ((pool_.Slot(content)[11]&1) || (pool_.Slot(Sid{static_cast<std::uint16_t>(Read(packet,4,2))})[11]&1)) return;
    Need(hooks_.detach)(content,0);
    Need(hooks_.attach)(content,Sid{static_cast<std::uint16_t>(Read(packet,4,2))},0);
    Need(hooks_.preview)(Sid{static_cast<std::uint16_t>(Read(packet,4,2))});
    Need(hooks_.sound)("crabSteal.wav",0,0,0,1,0);
}
// 현재 후보의genus210000/역방향 동맹/크랩 제외 뒤 첫 적합 내용물을 포획한다.
std::uint32_t RawCarrierSteal::Try(Sid parent) const {
    if (!boss_ || Need(hooks_.find)(parent,0x20,0,0).value || editor_) return 10;
    const auto raw=pool_.Slot(parent);const auto x=Cell(std::bit_cast<float>(Read(raw,14))),y=Cell(std::bit_cast<float>(Read(raw,18)));
    RawSquidFinder finder(pool_,hash_,types_);
    // finder의 현재 체인 순서를 유지하고 성공 뒤 다음 후보를 읽지 않는다.
    for (auto candidate=finder.Begin({x,y,x,y},0);candidate.value;candidate=finder.Next()) {
        const auto donor=pool_.Slot(candidate);if (donor[10]>=types_.size()) throw std::out_of_range("포획 현재 기증자TYPE 오류");
        if (!(types_[donor[10]].flags2&0x210000U)) continue;
        const auto first=donor[34],second=pool_.Slot(parent)[34];if (first>=9 || second>=9) throw std::out_of_range("포획 현재 동맹owner 오류");
        if (alliances_[first*9+second] || donor[10]==carrierType_) continue;
        const auto content=Need(hooks_.find)(candidate,0x20,0,0);if (!content.value) continue;
        const auto contained=pool_.Slot(content);
        if (Read(contained,18,2)==priestType_ && (contained[12]&0x7f)==pool_.Slot(parent)[34]) continue;
        std::array<std::uint8_t,10> packet{static_cast<std::uint8_t>(header_.protocol&0x7f),header_.message,static_cast<std::uint8_t>(header_.length),static_cast<std::uint8_t>(header_.length>>8)};
        Put(packet,4,parent.value);Put(packet,6,content.value);Put(packet,8,candidate.value);
        Receive(localOwner_,0,packet);Need(hooks_.broadcast)(packet);
        const auto movement=Need(hooks_.movementType)(pool_.Slot(candidate)[10]);Need(hooks_.removeMovement)(candidate,movement);
        const auto kind=Read(pool_.Slot(content),18,2);if (kind>=types_.size()) throw std::out_of_range("포획 뒤 현재 내용물TYPE 오류");
        if (types_[kind].flags2&0x8000U) Need(hooks_.goHome)(parent);return 12;
    }
    return 10;
}
// 실제 원본과 같은 풀만 반환한다.
const SidPool& RawCarrierSteal::Pool() const { return pool_; }
}
