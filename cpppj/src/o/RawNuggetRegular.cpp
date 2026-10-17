// count초기관문과원본프레임감김/게임난수/가속낙하순서를보존한다.
#include "o/RawNuggetRegular.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 정렬과무관하게원본DWORD를읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value{};
    // 낮은네바이트만합친다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[at+i])<<(8*i);
    return value;
}
// 선택된하위만요구하며count0/죽은객체의조기반환에는효과가필요없다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("너겟실제Regular하위몸체미연결");return function; }
// 실제_ftol64의정수무효값도하위DWORD0으로관찰한다. 중간합을float로저장하지않는다.
std::uint32_t Cell(float value) {
    const double adjusted=static_cast<double>(value)+static_cast<double>(0.9999F);
    if (!std::isfinite(adjusted) || adjusted<-9223372036854775808.0 || adjusted>=9223372036854775808.0) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(adjusted));
}
}
RawNuggetRegular::RawNuggetRegular(SidPool& pool,std::span<const std::int32_t> aFrameCounts,std::span<const std::uint8_t> spots,const bool& boss,NuggetRegularHooks hooks)
    :pool_(pool),aFrameCounts_(aFrameCounts),spots_(spots),boss_(boss),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || aFrameCounts.size()!=188 || spots.size()!=65536) throw std::invalid_argument("너겟Regular는PATCH188TYPE/65536spot이필요합니다");
}
bool RawNuggetRegular::Handles(Sid sid) const { const auto raw=pool_.Slot(sid);return raw[10]==154 && Read(raw,0)==0x50dc30U; }
bool RawNuggetRegular::CanFall(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto y=Cell(std::bit_cast<float>(Read(raw,18))),x=Cell(std::bit_cast<float>(Read(raw,14)));const auto index=(y<<8)+x;
    if (index>=spots_.size()) throw std::out_of_range("너겟지면조회가원본지도물리범위를벗어났습니다");
    return !(spots_[index]&6U);
}
float RawNuggetRegular::Animate(Sid sid,bool grow) const {
    const auto frame=Read(pool_.Slot(sid),36);bool endpoint=frame==0;
    if (grow) { const auto type=pool_.Slot(sid)[10];if (type>=aFrameCounts_.size()) throw std::out_of_range("너겟프레임TYPE범위오류");endpoint=frame==std::bit_cast<std::uint32_t>(aFrameCounts_[type])-1U; }
    if (endpoint) {
        if (Need(hooks_.allocate)()) { const auto delay=static_cast<float>(Need(hooks_.randomInteger)(2,8));Need(hooks_.construct)(sid,grow ? 0U : 1U,delay); }
        return 0.0F;
    }
    const auto next=Read(pool_.Slot(sid),36)+(grow ? 1U : 0xffffffffU);Need(hooks_.setFrame)(sid,std::bit_cast<std::int32_t>(next),0);
    if (grow) Need(hooks_.refresh)(sid);
    return 0.1F;
}
float RawNuggetRegular::Fall(Sid sid,std::int32_t count) const {
    if (pool_.Slot(sid)[11]&2U) return 0.0F;
    if (count>100) { if (boss_) Need(hooks_.destroy)(sid,0);return -1.0F; }
    if (count==10) pool_.AllocatedBytes(sid)[35]=20;
    const auto raw=pool_.Slot(sid);const auto x=std::bit_cast<float>(Read(raw,14));
    float y=static_cast<float>((static_cast<double>(count)*0.5+1.0)*0.0625+static_cast<double>(std::bit_cast<float>(Read(raw,18))));
    if (254.0F<y) y=254.0F; // 실제unordered분기는NaN을보존한다.
    Need(hooks_.unpop)(sid,0);Need(hooks_.pop)(sid,x,y,0x40U);
    return 0.05F;
}
float RawNuggetRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!count) return payload;
    if (event==0) return Animate(sid,false);
    if (event==1) return Animate(sid,true);
    if (event==2) { Need(hooks_.destroy)(sid,0x100000U);return -1.0F; }
    if (event==3 && CanFall(sid)) return Fall(sid,std::bit_cast<std::int32_t>(count));
    return Need(hooks_.damageable)(sid,event,count,payload);
}
const SidPool& RawNuggetRegular::Pool() const { return pool_; }
}
