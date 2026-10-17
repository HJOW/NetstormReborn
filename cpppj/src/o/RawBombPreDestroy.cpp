// 폭발 삭제 플래그가 없는 포획에서는 공통 전처리만 직접 호출한다.
#include "o/RawBombPreDestroy.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 비정렬DWORD만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; //원문필드의 조립값이다.
    // 지정한 네 바이트만 결합한다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 선택되지 않은 효과는 요구하지 않으며 빈 성공은 공급하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Bomb 실제전처리 경계미연결");return hook; }
}
// CD의 다른전처리와 같은 것으로 추정하지 않는다.
RawBombPreDestroy::RawBombPreDestroy(const SidPool& pool,BombPreDestroyHooks hooks):pool_(pool),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Bomb 전처리 PATCH 판본오류"); }
// 입력TYPE 이름으로 가상 표를 넓히지 않는다.
bool RawBombPreDestroy::Handles(Sid sid) const { return Read(pool_.Slot(sid),0)==0x502370U; }
// 최초extra9/flags100000 관문은 한 번만 검사하고 다음 큰 효과 사이에 현재 raw를 다시 읽는다.
void RawBombPreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    if (!Handles(sid)) throw std::invalid_argument("Bomb 고유전처리 VT불일치");
    if (!(pool_.Slot(sid)[40]&9U) && (flags&0x100000U)) { Need(hooks_.explosion)(sid);const auto raw=pool_.Slot(sid);Need(hooks_.sound)(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))); }
    Need(hooks_.base)(sid,flags);
}
}
