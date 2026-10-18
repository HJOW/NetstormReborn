// 원본의BYTE 중복 카운터·삭제 전에 다음 후보 읽기·현재 부모 재조회와 생성 인자를 유지한다.
#include "o/RawProductionSync.h"
#include "o/RawProductionDeck.h"
#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kCount=0x14c,kFlags=20,kKind=18; // 현재 Deck개수/내용물flags·kind의 실제 위치다.
// 비정렬 원문의 필요한 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 인접한 다른 필드를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
// 원본의 signed 개수는 음수이면 빈 순회다. 호스트 물리66개 한계는 넘지 못한다.
std::int32_t Count(std::span<const std::uint8_t> deck) {
    if (deck.size()<kProductionDeckBytes) throw std::out_of_range("생산 동기화 실제 Deck 크기 부족");
    const auto count=std::bit_cast<std::int32_t>(Read(deck,kCount));if (count>static_cast<std::int32_t>(kProductionEntryCapacity)) throw std::out_of_range("생산 동기화 Deck 물리 개수 초과");return count;
}
// 실제44f4d0처럼 첫 type BYTE 일치 항목의 signed power만 반환한다.
std::optional<std::int8_t> Power(std::span<const std::uint8_t> deck,std::uint32_t kind) {
    // 같은 종류가 중복되어도 첫 물리 항목만 사용한다.
    for (std::int32_t i=0;i<Count(deck);++i) { const auto offset=static_cast<std::size_t>(i)*kProductionEntryBytes;if (deck[offset]==kind) return std::bit_cast<std::int8_t>(deck[offset+2]); }
    return std::nullopt;
}
// 호출한 실제 하위의 누락만 그 위치에서 거부한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("생산 내용물 동기화 실제 하위 미연결");return callback; }
}
// 판본/현재 자료를 확인하며 생성 시 게임 상태는 변경하지 않는다.
RawProductionSync::RawProductionSync(const SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& finder,ProductionSyncState state,ProductionSyncHooks hooks)
    :pool_(pool),types_(types),finder_(finder),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("생산 내용물 동기화 PATCH 자료 오류");
}
// pointer 산술 전에 원문DWORD를 검사하며0을 임의 부모로 바꾸지 않는다.
Sid RawProductionSync::Parent(std::uint32_t value) const {
    if (!value || value>=pool_.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("생산 동기화 현재 기술 부모 범위 오류");return {static_cast<std::uint16_t>(value)};
}
// 삭제되는 기존 내용물도 seen을 먼저 증가시키므로 같은 호출 안에서는 다시 만들지 않는다.
void RawProductionSync::Synchronize() const {
    const auto deck=Need(hooks_.deck)();std::array<std::uint8_t,255> seen{};RawContainedFinder finder(pool_,finder_);
    auto current=finder.Begin(Parent(Need(hooks_.technology)()));
    // 원본은 다음 후보를 먼저 찾은 뒤 이전 후보의 현재kind/flags를 읽는다.
    while (current.value) {
        const auto form=current;current=finder.Next();const auto raw=pool_.Slot(form);const auto flags=Read(raw,kFlags,2),kind=Read(raw,kKind,2);
        if (kind>=types_.size() || kind>=seen.size()) throw std::out_of_range("생산 동기화 내용물 현재 TYPE 범위 오류");
        if (flags&8U) ++seen[kind];bool remove{};
        if (types_[kind].flags2&4U) remove=!Need(hooks_.temple)(state_.localOwner);
        else if (flags&8U) { const auto power=Power(deck,kind);if (!power || *power<1) remove=state_.fullDeck==0; }
        if (remove) { if (flags&0x100U) Need(hooks_.cancelCursor)();Need(hooks_.destroy)(form,0); }
    }
    const auto parent=Parent(Need(hooks_.technology)());
    // 같은 Deck 포인터의 현재 개수를 매번 읽고 새 생성 뒤 seen을 올리지 않는다.
    for (std::int32_t i=0;i<Count(deck);++i) {
        const auto kind=static_cast<std::uint32_t>(deck[static_cast<std::size_t>(i)*kProductionEntryBytes]);
        if (kind>=seen.size()) throw std::out_of_range("생산 동기화 Deck type BYTE 범위 오류");
        if (!seen[kind]) {
            if (Need(hooks_.findKind)(parent,8,kind,0)) Need(hooks_.assertion)("!me.findContainedType( CFF_TELEPORT, type )");
            static_cast<void>(Need(hooks_.create)(parent,kind,0,8,16,3));
        }
    }
}
// 단일 월드의 실제 저장소임을 검증한다.
const SidPool& RawProductionSync::Pool() const { return pool_; }
}
