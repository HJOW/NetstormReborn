// Carrier의 교대 표시와 현재 raw 재조회 순서를 보호10.78의42a750에 맞춘다.
#include "o/RawCarrierRegular.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::uint32_t,6> kCarrierRegularVtables{0x503d80U,0x503e50U,0x503f20U,0x503ff0U,0x5040c0U,0x50e568U}; // 보호 PE의 실제+5c=42a750 여섯 VT다.
// 비정렬 원문 WORD/DWORD만 낮은 바이트부터 합친다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) {
    std::uint32_t value{};
    // 요청한 원문 폭만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[at+i])<<(8*i);
    return value;
}
// 직접 프레임 쓰기는 SquidFrame의 추가 표시 효과를 호출하지 않는다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) {
    // 정확히 한 DWORD를 little endian으로 쓴다.
    for (std::size_t i=0;i<4;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 선택한 외부 효과만 필수다. 조기 반환 경로에 없는 효과를 미리 요구하지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Carrier 실제 Regular 하위 몸체 미연결");
    return function;
}
// 실제41d0d0의 unordered 분기는 NaN을 보존한다. 유한 좌표만1..255로 제한한다.
float Clamp(float value) { const float lower=value<1.0F ? 1.0F : value;return 255.0F<lower ? 255.0F : lower; }
}
RawCarrierRegular::RawCarrierRegular(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const CarrierRegularType> details,const bool& boss,CarrierRegularHooks hooks)
    :pool_(pool),types_(types),details_(details),boss_(boss),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || details.size()!=types.size()) throw std::invalid_argument("Carrier Regular는 같은 PATCH188 TYPE가 필요합니다");
}
bool RawCarrierRegular::Handles(Sid parent) const {
    const auto table=Read(pool_.Slot(parent),0);
    // 가상 표만 비교하며 genus/타입 번호로 파생 범위를 넓히지 않는다.
    for (const auto known:kCarrierRegularVtables) if (table==known) return true;
    return false;
}
float RawCarrierRegular::Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const {
    if (std::bit_cast<std::int32_t>(event)<0x494) {
        if (event==0x493U) {
            const auto form=Need(hooks_.find)(parent,1,0,0);const auto kind=Read(pool_.Slot(form),18,2); // 원본은 빈 결과도 물리0번 슬롯에서 kind를 읽는다.
            if (kind>=details_.size()) throw std::out_of_range("Carrier 시전 내용물 TYPE 범위 오류");
            const auto limit=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(details_[kind].castTime)*10U);
            if (limit<std::bit_cast<std::int32_t>(count)) return 0.0F;
        } else if (event==0x491U) {
            const float limit=1.5F/payload;const float elapsed=static_cast<float>(std::bit_cast<std::int32_t>(count));std::uint32_t found{}; // 두 값 모두 실제 float 저장 경계를 거친다.
            // mask20/1 순서로 조회하며 각각 조회 뒤 기한을 검사한다.
            for (std::uint32_t index=0;index<2;++index) {
                const auto form=Need(hooks_.find)(parent,index==0 ? 0x20U : 1U,0,0);
                if (limit<elapsed) return 0.0F;
                if (!form.value) continue;
                const auto kind=Read(pool_.Slot(form),18,2);
                if (kind>=types_.size()) throw std::out_of_range("Carrier 표시 내용물 TYPE 범위 오류");
                if (!(types_[kind].flags2&0x208100U)) return 0.0F;
                ++found;
                if ((index==0 && (count&1U)) || (index==1 && !(count&1U))) {
                    const auto ghost=Need(hooks_.create)(kind,2);auto raw=pool_.AllocatedBytes(ghost);
                    if (raw[10]>=details_.size()) throw std::out_of_range("Carrier 새 표시 TYPE 범위 오류");
                    Put(raw,36,std::bit_cast<std::uint32_t>(details_[raw[10]].defaultFrame));raw[35]=static_cast<std::uint8_t>(pool_.Slot(parent)[35]-1U);raw[40]|=8;
                    Need(hooks_.setWord)(ghost,Read(pool_.Slot(form),12,2));Need(hooks_.setOwner)(ghost,pool_.Slot(parent)[34]);
                    const auto current=pool_.Slot(parent);const float x=std::bit_cast<float>(Read(current,14));const float y=std::bit_cast<float>(Read(current,18))-(index==0 ? 0.8F : 1.2F);
                    Need(hooks_.pop)(ghost,Clamp(x),Clamp(y),0);
                    if (Need(hooks_.allocateTimer)()) Need(hooks_.constructTimer)(ghost,payload);
                }
            }
            if (!found) return 0.0F;
            Need(hooks_.refresh)(parent);
        } else if (event==0x492U && boss_) {
            const auto raw=pool_.Slot(parent);
            if (Need(hooks_.needsEscape)(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)))) {
                const auto type=Need(hooks_.movementType)(pool_.Slot(parent)[10]);
                if (!Need(hooks_.hasPath)(parent,type)) Need(hooks_.escape)(parent,1.0F,0xffffffffU);
            }
        }
    } else if (event==0x5105U) return Need(hooks_.falling)(parent,count);
    return Need(hooks_.damageable)(parent,event,count,payload);
}
RegularHandler RawCarrierRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid parent,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(parent)) return Handle(parent,event,count,payload);
        if (!fallback) throw std::logic_error("다른 실제 Carrier 파생 Regular 미연결");
        return fallback(parent,event,count,payload);
    };
}
const SidPool& RawCarrierRegular::Pool() const { return pool_; }
}
