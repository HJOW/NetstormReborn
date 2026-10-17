// 좌표를 캡처한 뒤 양축을 거의 올리고 현재 포획물의 이전 소유자에 쓴다.
#include "o/RawCarrierFallStart.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 현재 raw의 비정렬 DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { std::uint32_t value{};
    // 정확히 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[at+i])<<(8*i);return value;
}
// Player의 한 좌표를 원본 DWORD 폭으로 저장한다.
void Write(std::span<std::uint8_t> raw,std::size_t at,float value) { const auto bits=std::bit_cast<std::uint32_t>(value);
    // 인접 Player 필드를 보존한다.
    for (std::size_t i=0;i<4;++i) raw[at+i]=static_cast<std::uint8_t>(bits>>(8*i));
}
// 41d7a0은0.99999F이며 ftol64 하위 signed DWORD를 다시 float에 저장한다.
float AlmostCeil(float value) {
    const double adjusted=static_cast<double>(value)+static_cast<double>(0.99999F);
    if (!std::isfinite(adjusted) || adjusted<-9223372036854775808.0 || adjusted>=9223372036854775808.0) return 0.0F;
    return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(adjusted))));
}
// 선택된 호출이 필요해질 때만 미연결을 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("캐리어 낙하 시작 실제 하위 미연결");return function; }
}
RawCarrierFallStart::RawCarrierFallStart(const SidPool& pool,std::span<std::uint8_t> players,const bool& boss,const std::uint32_t& priestType,CarrierFallStartHooks hooks)
    :pool_(pool),players_(players),boss_(boss),priestType_(priestType),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || players.size()%0xb4) throw std::invalid_argument("캐리어 낙하 시작 PATCH Player 자료 오류");
}
bool RawCarrierFallStart::Begin(Sid parent) const {
    if (Need(hooks_.hasRegular)(parent,0x5105U)) return true;
    if (boss_) { const auto movement=Need(hooks_.movementType)(pool_.Slot(parent)[10]);Need(hooks_.removeMovement)(parent,movement); }
    if (pool_.Slot(parent)[40]&4U) Need(hooks_.clearSelection)();
    const auto form=Need(hooks_.findKind)(parent,priestType_);
    if (form.value) {
        const auto raw=pool_.Slot(parent);const auto x=AlmostCeil(std::bit_cast<float>(Read(raw,14))),y=AlmostCeil(std::bit_cast<float>(Read(raw,18)));
        const auto at=static_cast<std::size_t>(pool_.Slot(form)[12]&0x7fU)*0xb4U;
        if (at>players_.size() || players_.size()-at<0xb4U) throw std::out_of_range("캐리어 사제 복귀 Player 범위 오류");
        Write(players_,at+0xa8,x);Write(players_,at+0xac,y);
        if (boss_) Need(hooks_.releasePriest)(parent);
    }
    Need(hooks_.task)(parent,0x5105U,0.0F);return true;
}
const SidPool& RawCarrierFallStart::Pool() const { return pool_; }
}
