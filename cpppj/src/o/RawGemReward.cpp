// 레벨98/99는 제외하고0/1/2/3의20/30/50/100 가중치를 그대로 사용한다.
#include "o/RawGemReward.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된 외부 효과가 없으면 아직 미복원된 호출로 보고한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("오벨리스크 보상의 실제 경계 미연결");return hook; }
// 비정렬 WORD/DWORD를 필요한 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{}; //폭이 제한된 little endian 반환이다.
    // 인접 필드를 함께 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
}
// 다른 판본의 보상 TYPE나 구조체를 PATCH와 같은 것으로 추정하지 않는다.
RawGemReward::RawGemReward(SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,const SquidPostPopList& excluded,GemRewardState& state,GemRewardHooks hooks)
    :pool_(pool),types_(types),random_(random),excluded_(excluded),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("오벨리스크 보상의 PATCH/188TYPE 연결 오류");
}
// QA 밖 TYPE 주소의 원본 무효 역참조는 호스트 범위 계약에서 거부한다.
const RiftTypeRecord& RawGemReward::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("오벨리스크 현재TYPE 범위");return types_[type]; }
// 원본41c7a0의 저장 항목은BYTE이며 인자 전체DWORD와 비교한다.
bool RawGemReward::Excluded(std::uint32_t type) const {
    // 같은 현재목록을 호출 때마다 읽고 최초 일치에서 반환한다.
    for (const auto entry:excluded_.Items()) if (std::uint32_t(static_cast<std::uint8_t>(entry))==type) return true;return false;
}
// 실시간 난수는 Deck Draw 안에서만 만들며 전역 난수를 추첨값으로 대신 쓰지 않는다.
std::uint32_t RawGemReward::Choose(std::uint32_t unused) const {
    (void)unused;std::array<std::uint8_t,kProductionDeckBytes> raw;Need(hooks_.fill)(raw);RawDeckDraw::Initialize(raw); //미정 영역을 지우지 않는 실제 임시 Deck이다.
    // 현재타입개수는 각 반복 끝에 다시 읽는다. 수준진단이 돌아오면 다음 현재TYPE를 계속 처리한다.
    for (std::int32_t type=0;type<state_.typeCount;++type) {
        if (!(Type(static_cast<std::uint32_t>(type)).flags2&0x100U) || Excluded(static_cast<std::uint32_t>(type))) continue;
        const auto level=Type(static_cast<std::uint32_t>(type)).level;std::int32_t chance{}; //제외목록 조회 뒤 다시 읽는 현재레벨이다.
        switch (level) { case 0:chance=20;break;case 1:chance=30;break;case 2:chance=50;break;case 3:chance=100;break;case 98:case 99:continue;default:Need(hooks_.assertion)(0xf6);continue; }
        RawDeckDraw::Append(raw,static_cast<std::uint32_t>(type),chance,hooks_.deckAssertion);
    }
    std::uint32_t type{}; //빈Deck/실패진단 복귀의 원본 반환0이다.
    if (Read(raw,0x14c)!=0) { RawDeckDraw deck(raw,random_,hooks_.deck);deck.Open(random_.Next(10000));type=deck.Draw();deck.Close();if (type) return type; }
    Need(hooks_.assertion)(0x104);return type;
}
// 오벨리스크 QA의유효genus는제외목록/현재레벨과무관하게 그대로 사용한다.
Sid RawGemReward::Reveal(Sid buried) const {
    auto raw=pool_.Slot(buried);if (std::uint32_t(raw[10])!=state_.buriedType || (raw[11]&1U)) Need(hooks_.assertion)(0x10a);
    std::uint32_t type=Read(pool_.Slot(buried),12,2);if (!type || !(Type(type).flags2&0x100U)) type=Choose(5);
    const auto created=Need(hooks_.create)(type,0);raw=pool_.Slot(buried);Need(hooks_.pop)(created,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),0);
    Need(hooks_.destroy)(buried,0);return created;
}
// 실제 풀 정체성만 빌리며 대상이나 TYPE를 복사하지 않는다.
const SidPool& RawGemReward::Pool() const { return pool_; }
}
