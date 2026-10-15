// 영역 번호/섬 목록을 거치지 않고 원본 표면 WORD→타입→소유자 BYTE를 읽는다.
#include "o/RawRegionOwner.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr float kSnapBias=0.99999F; // PATCH00502310/CD005017a0의 원본 snap 편향이다.
constexpr std::int32_t kSide=256; // 두 원본 표면 WORD 지도의 한 변이다.
}
// 원본과 같은 하나의 현재 표면 배열만 허용한다.
RawRegionOwner::RawRegionOwner(const SidPool& pool,std::span<const std::uint16_t> surfaces,const std::uint32_t& terrainType)
    :pool_(pool),surfaces_(surfaces),terrainType_(terrainType) {
    if (surfaces.size()!=static_cast<std::size_t>(kSide*kSide)) throw std::invalid_argument("영역 소유자 실제 표면 WORD 지도 크기 오류");
}
// 경계 검사를 통과한 WORD SID의 현재 타입과 소유자만 읽는다. dead/void/free 필터를 추가하지 않는다.
std::uint32_t RawRegionOwner::At(std::int32_t x,std::int32_t y) const {
    if (x<0 || y<0 || x>=kSide || y>=kSide) return 0;
    const auto sid=surfaces_[static_cast<std::size_t>(y*kSide+x)];if (!sid) return 0;
    if (sid>=pool_.Capacity()) throw std::out_of_range("영역 소유자 현재 표면 SID 범위 오류");
    const auto raw=pool_.Slot(Sid{sid});
    return raw[10]==terrainType_ ? raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U] : 0U;
}
// 원본 _ftol의 signed64 절삭과 하위 DWORD 감김을 보존한다.
std::int32_t RawRegionOwner::Truncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 첫 x87 합계를 float로 먼저 줄이지 않는다. 원본 int→float 저장은 두 번째 절삭 전에 적용한다.
std::int32_t RawRegionOwner::Snap(float value) { return Truncate(static_cast<float>(Truncate(static_cast<double>(value)+kSnapBias))); }
// 두 float 인자는 원본 helper와 같은 독립 순서로 snap한다.
std::uint32_t RawRegionOwner::At(float x,float y) const { const auto sx=Snap(x),sy=Snap(y);return At(sx,sy); }
// 실제 Session/미션 adapter의 다른 Pool 오연결을 차단한다.
const SidPool& RawRegionOwner::Pool() const { return pool_; }
}
