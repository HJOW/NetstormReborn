// 원본 전체 collector의 순서·출력 폭·부모 조회와 판본별 float 저장 경계를 유지한다.
#include "o/RawRenderCollection.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 좌표/head raw 위치와 표시 항목의 종류다.
constexpr std::size_t kHead=6,kX=14,kY=18;
constexpr std::uint8_t kRenderKind=3;
// 표시가 같은 부모의 자산보다 뒤에 오도록 y에 더하는 원본 float32다.
constexpr float kOrderBias=std::bit_cast<float>(0x3a83126fU);
// 정렬기와 원본 출력 폭에 맞는 16바이트 기록임을 빌드 때 확인한다.
static_assert(sizeof(RenderCollectionEntry)==16);
// 정렬되지 않은 부모의 DWORD를 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 입력 네 바이트만 읽고 주변 raw 필드는 건드리지 않는다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return value;
}
}
// 등록 목록과 부모 raw가 같은 풀인지 확인하며 실행/삭제 경계는 만들지 않는다.
RawRenderCollection::RawRenderCollection(const SidPool& pool,const ConstructionVisualState& state,const RawRenderRegistry& registry,RenderFieldsLookup lookup)
    :pool_(pool),state_(state),registry_(registry),lookup_(std::move(lookup)) {
    if (&pool_!=&registry_.Pool() || !lookup_) throw std::invalid_argument("표시 수집 연결 오류");
}
// 원본은 부모가 비어 있거나 dead/void인지를 검사하지 않고 head만 확인한다.
std::span<const std::uint8_t> RawRenderCollection::Parent(const RenderProcessFields& fields) const {
    if (fields.parent<0 || static_cast<std::uint32_t>(fields.parent)>=pool_.Capacity()) throw std::out_of_range("표시 수집 부모 범위 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(fields.parent)});
    if (!(raw[kHead]|raw[kHead+1])) throw std::logic_error("표시 수집 부모 head가 비었습니다");
    return raw;
}
// 역수를 float로 먼저 반올림하지 않는다. 패치의 곱셈과 CD의 나눗셈을 구별한다.
RenderWorldRect RawRenderCollection::Bounds(const RenderProcessFields& fields) const {
    static_cast<void>(Parent(fields));
    if (!std::isfinite(state_.scaleX) || !std::isfinite(state_.scaleY) || state_.scaleX==0 || state_.scaleY==0)
        throw std::invalid_argument("표시 수집 배율 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto x=1.0/static_cast<double>(state_.scaleX),y=1.0/static_cast<double>(state_.scaleY);
    return {static_cast<float>(patch ? x*fields.left : static_cast<double>(fields.left)/state_.scaleX),
        static_cast<float>(patch ? y*fields.top : static_cast<double>(fields.top)/state_.scaleY),
        static_cast<float>(patch ? x*fields.right : static_cast<double>(fields.right)/state_.scaleX),
        static_cast<float>(patch ? y*fields.bottom : static_cast<double>(fields.bottom)/state_.scaleY)};
}
// 유효한 점/선은 내부 교차를 허용하지만 네 변 중 하나라도 닿기만 하면 제외한다.
bool RawRenderCollection::Intersects(RenderWorldRect view,RenderWorldRect bounds,OriginalEdition edition) {
    if (edition!=OriginalEdition::Patch1078) {
        // CD의 test ah,1 / test ah,0x41 분기는 unordered에서 서로 다른 결과를 낸다.
        return view.left<=view.right && !(view.top>view.bottom) && bounds.left<=bounds.right && !(bounds.top>bounds.bottom) &&
            !(view.left>=bounds.right) && bounds.left<view.right && !(view.top>=bounds.bottom) && bounds.top<view.bottom;
    }
    return view.left<=view.right && view.top<=view.bottom && bounds.left<=bounds.right && bounds.top<=bounds.bottom &&
        view.left<bounds.right && bounds.left<view.right && view.top<bounds.bottom && bounds.top<view.bottom;
}
// 출력 개수를 다 채운 뒤 다음 슬롯의 부모를 조회하지 않는다. 기존 출력의 tail/미사용 항목도 보존한다.
std::int32_t RawRenderCollection::Collect(RenderWorldRect view,std::span<RenderCollectionEntry> output,std::int32_t limit) const {
    if (limit>0 && static_cast<std::size_t>(limit)>output.size()) throw std::length_error("표시 수집 출력 용량 오류");
    std::int32_t count=0;
    // 등록 배열의 0번부터 검사하며 빈 칸은 출력 항목을 소비하지 않는다.
    for (const auto token:registry_.Slots()) {
        if (count>=limit) return count;if (!token) continue;
        const auto* fields=lookup_(token);if (!fields) continue;
        if (!Intersects(view,Bounds(*fields),pool_.Edition())) continue;
        const auto parent=Parent(*fields);auto& entry=output[static_cast<std::size_t>(count++)];
        entry.x=std::bit_cast<float>(Read(parent,kX));entry.y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(parent,kY)))+kOrderBias);
        entry.token=token;entry.depth=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(fields->zOrder));entry.kind=kRenderKind;
    }
    return count;
}
}
