// 대상 검색 중 최초59 유무를 보고2/4/6개 원주 생산기를 같은 운반체에 부착한다.
#include "o/RawWindSpell.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본이 선택한 실제 하위의 누락을 빈 성공으로 숨기지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Wind 주문 실제 하위 미연결");return hook; }
// 생성자가 쓰지 않은 바이트를 보존하면서 count/index DWORD만 기록한다.
void Put(WindAviaryProcessFields& fields,std::size_t at,std::uint32_t value) {
    for (std::size_t i=0;i<4;++i) fields.bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i)); // 원본 little endian 쓰기다.
}
}
RawWindSpell::RawWindSpell(const SidPool& pool,const bool& server,const double& now,WindSpellState& state,WindSpellHooks hooks)
    :pool_(pool),server_(server),now_(now),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || state.counts.size()!=188) throw std::invalid_argument("Wind 주문 PATCH/188TYPE 필요");
}
std::uint32_t RawWindSpell::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t result{};for (std::size_t i=0;i<width;++i) result|=std::uint32_t(raw[offset+i])<<(8*i);return result; // BYTE 폭을 확장하지 않는다.
}
bool RawWindSpell::MayTarget(Sid spell) const {
    const auto table=Read(spell,0);std::size_t variant{};
    if (table==0x5025d8U) variant=0;else if (table==0x502e68U) variant=1;else if (table==0x502f30U) variant=2;else throw std::invalid_argument("Wind 주문 현재VT 불일치");
    if (!server_) return false;
    if (Need(hooks_.exists)(Sid{static_cast<std::uint16_t>(Read(spell,12,2))},state_.processType)) return false;
    const auto kind=state_.spells[variant];if (kind>=state_.counts.size()) throw std::out_of_range("Wind 주문 현재TYPE 범위");
    for (std::int32_t index=0;index<state_.counts[kind];++index) { // 부착 뒤 변경된 같은TYPE의 현재 한도를 매번 읽는다.
        auto fields=Need(hooks_.allocate)();if (!fields) throw std::bad_alloc();RawWindAviaryState::Initialize(*fields,server_,now_);
        Put(*fields,68,std::bit_cast<std::uint32_t>(state_.counts[kind]));Put(*fields,72,std::bit_cast<std::uint32_t>(index));
        Need(hooks_.attach)(std::move(*fields),Sid{static_cast<std::uint16_t>(Read(spell,12,2))},state_.processType,8);
    }
    return false;
}
}
