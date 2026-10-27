// 원본4d5840의 소유자/이동/깊이/좌표 순위와 마지막1.6 근접 후보 교체다.
#include "client/RawUserPick.h"
#include "o/RawAiDryGeometry.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 10.78 입력 선택 몸체이며 객체 상태를 쓰지 않는다.
namespace { // 원문 비정렬 필드와 도달한 하위 계약만 처리한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("선택기의 도달한 원본 하위 미연결");return function; } //미도달 경계는 요구하지 않는다.
float Position(std::span<const std::uint8_t> raw,std::size_t offset) { //비정렬float를 현재 raw 네 바이트에서 읽는다.
    std::uint32_t bits{}; //현재float의 little endian 원문이다.
    for (std::size_t byte=0;byte<4;++byte) bits|=std::uint32_t(raw[offset+byte])<<(8*byte); //주변 필드를 읽거나 정렬을 가정하지 않는다.
    return std::bit_cast<float>(bits);
}
}
RawUserPick::RawUserPick(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,UserPickHooks hooks):pool_(pool),types_(types),hooks_(std::move(hooks)) { //선택/장치 효과 없이 같은 참조만 설치한다.
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("원본4d5840 선택기는10.78 전용");
}
const o::RiftTypeRecord& RawUserPick::Type(o::Sid sid) const { //추상/소유자 등의 후속 읽기도 현재 TYPE를 재조회한다.
    const auto number=pool_.Slot(sid)[10];if (number>=types_.size()) throw std::out_of_range("선택기의 현재 TYPE 범위");return types_[number];
}
o::Sid RawUserPick::Pick(std::int32_t x,std::int32_t y,std::uint32_t force) const { //첫 관문·검색·가상 hit·마지막 근접 교체 순서를 유지한다.
    if (Need(hooks_.paused)()) return {};
    const auto point=Need(hooks_.project)(x,y);const bool meta=Need(hooks_.metamap)()!=0; //원문은 반경을 만들 때만 metamap을 읽는다.
    auto candidate=Need(hooks_.begin)(point,meta ? 8 : 4,meta ? 8 : 7);
    o::Sid best{},near{};int bestDepth=9999;float bestX{},bestY{},nearest=1.6F;bool bestOwned{},bestMobile{}; //원래 초기값이며 근접 거리는float 저장 경계를 보존한다.
    while (candidate>0 && candidate<pool_.Capacity()) { //free/dead/자산 여부를 새 필터로 추가하지 않고 물리 범위만 검사한다.
        if (candidate>65535) throw std::out_of_range("선택기의 WORD SID 범위");const o::Sid sid{static_cast<std::uint16_t>(candidate)};
        const bool owned=pool_.Slot(sid)[34]==Need(hooks_.localOwner)();const bool mobile=owned && (Type(sid).flags2&0x30000U); //소유 이동 여부는 후보 진입 시점 값이다.
        const auto& type=Type(sid);const auto raw=pool_.Slot(sid); //같은 현재 TYPE/extra의 원본 거부 관문이다.
        const bool hidden=Need(hooks_.hideBuildings)() && ((type.flags2&2421574144U) || raw[10]==164 || raw[10]==84);
        if (!hidden && (force || (!(type.flags2&0x2000000U) && !(type.flags1&0x8000000U) && (!(raw[40]&0x10U) || raw[34]==Need(hooks_.localOwner)())))) {
            if (Type(sid).flags2&0x30000U) { //가상 hit 실패와 무관하게 가까운 이동 후보를 먼저 기억한다.
                const auto center=Need(hooks_.center)(sid);const double distance=o::AiPointDistance({point[0],point[1]},{center[0],center[1]});
                if (distance<static_cast<double>(nearest)) { nearest=static_cast<float>(distance);near=sid; }
            }
            const auto current=pool_.Slot(sid);const float px=Position(current,14),py=Position(current,18),dy=py-bestY,dx=px-bestX; //원래 저장된float 차이이며 raw 좌표를 중심으로 바꾸지 않는다.
            const int dz=std::bit_cast<std::int8_t>(current[35])-bestDepth; //signed BYTE 깊이 차이다.
            bool allowed=owned || !bestOwned; //자기 소유 hit은 다른 소유자보다 우선한다.
            if (allowed && owned==bestOwned) {
                if (mobile && !bestMobile) allowed=true;
                else if (!mobile && bestMobile) allowed=false;
                else if (dz>=0 && (dz!=0 || (dy<=0.F && (dy!=0.F || dx<=0.F)))) allowed=false;
            }
            if (allowed && Need(hooks_.hit)(sid,x,y)) { //rank를 통과한 현재 후보에만+b0를 호출한다.
                bestY=dy+bestY;bestDepth+=dz;bestX=dx+bestX;best=sid;bestOwned=owned;bestMobile=mobile;
            }
        }
        candidate=Need(hooks_.next)();
    }
    if ((!best.value || (Type(best).flags2&0x80006U)) && near.value) best=near; //표면/다리/edgefarm의 hit보다 가까운 이동 유닛을 마지막에 고른다.
    return best;
}
}
