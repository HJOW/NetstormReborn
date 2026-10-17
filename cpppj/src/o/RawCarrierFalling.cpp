// 원본의 캡처 시점과 현재 가상 표 재조회를 보존한다.
#include "o/RawCarrierFalling.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 비정렬 원본 DWORD/WORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) {
    if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("캐리어 낙하 원본 기록 범위 오류");
    std::uint32_t result{};
    // 필요한 하위 바이트만 결합한다.
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(raw[at+i])<<(8*i);
    return result;
}
// 선택된 하위만 요구한다. 권한 없는 조기 반환에는 효과가 필요 없다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("캐리어 낙하 실제 하위 미연결");return function; }
// _ftol64의 무효 정수도 실제 사용한 하위 DWORD로 관찰한다.
std::int32_t Cell(float value) {
    const double adjusted=static_cast<double>(value)+static_cast<double>(0.9999F);
    if (!std::isfinite(adjusted) || adjusted<-9223372036854775808.0 || adjusted>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(adjusted)));
}
}
RawCarrierFalling::RawCarrierFalling(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint16_t> surface,std::span<const std::uint8_t> players,const bool& boss,const bool& checking,const std::uint32_t& priestType,const std::int32_t& mapLimit,CarrierFallingHooks hooks)
    :pool_(pool),types_(types),frames_(frames),surface_(surface),players_(players),boss_(boss),checking_(checking),priestType_(priestType),mapLimit_(mapLimit),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || surface.size()!=65536 || players.size()%0xb4) throw std::invalid_argument("캐리어 낙하 PATCH 자료 크기 오류");
}
std::uint32_t RawCarrierFalling::NeedsEscape(float x,float y) const {
    const auto ix=Cell(x),iy=Cell(y);if (ix<0 || iy<0 || ix>=256 || iy>=256) return 0;
    const Sid sid{surface_[static_cast<std::size_t>(iy*256+ix)]};if (!sid.value) return 0;
    const auto raw=pool_.Slot(sid);const auto type=raw[10];if (type>=types_.size()) throw std::out_of_range("캐리어 다리 TYPE 오류");
    if (!(types_[type].flags2&4U)) return 0;
    const auto frame=Read(raw,36);const auto codes=frames_[type].Codes();if (frame>=codes.size()) throw std::out_of_range("캐리어 다리 프레임 코드 범위 오류");
    return codes[frame].flags&0x20U;
}
bool RawCarrierFalling::ValidPoint(float x,float y) const {
    if (!(0.0F<x)) return false;
    const auto limit=static_cast<float>(mapLimit_);return x<limit && 0.0F<y && y<limit;
}
float RawCarrierFalling::Fall(Sid parent,std::uint32_t count) const {
    const auto signedCount=std::bit_cast<std::int32_t>(count);
    if (signedCount>100) {
        if (!boss_) return 1.0F;
        if (Need(hooks_.find)(parent,0x20U,priestType_,0).value) ReleasePriest(parent);
        Need(hooks_.destroy)(parent,0);return -1.0F;
    }
    if (signedCount==10) pool_.AllocatedBytes(parent)[35]=20;
    const auto raw=pool_.Slot(parent);const auto x=std::bit_cast<float>(Read(raw,14));
    float y=static_cast<float>((static_cast<double>(signedCount)*0.5+1.0)*0.0625+static_cast<double>(std::bit_cast<float>(Read(raw,18))));
    if (254.0F<y) y=254.0F; // unordered 분기는 NaN을 보존한다.
    Need(hooks_.unpop)(parent,0);Need(hooks_.pop)(parent,x,y,0x40U);return 0.05F;
}
void RawCarrierFalling::ReleasePriest(Sid parent) const {
    if (checking_ && !boss_) Need(hooks_.assertion)(0x3bfU);
    if (!boss_) return;
    const auto source=Need(hooks_.findKind)(parent,priestType_);const auto priest=Need(hooks_.create)(priestType_,0);
    Need(hooks_.setWord)(priest,Read(pool_.Slot(source),12,2));Need(hooks_.setOwner)(priest,0);
    const auto owner=pool_.Slot(source)[12]&0x7fU;const auto at=static_cast<std::size_t>(owner)*0xb4U;
    const auto x=std::bit_cast<float>(Read(players_,at+0xa8)),y=std::bit_cast<float>(Read(players_,at+0xac));
    if (!ValidPoint(x,y)) Need(hooks_.assertion)(0x3d0U);
    Need(hooks_.pop)(priest,x,y,0);static_cast<void>(Need(hooks_.fallWalker)(priest));Need(hooks_.destroy)(source,0);
}
const SidPool& RawCarrierFalling::Pool() const { return pool_; }
}
