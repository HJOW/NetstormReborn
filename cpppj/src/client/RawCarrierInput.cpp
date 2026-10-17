// 원본의 조기 반환과 효과 뒤 현재 상태 재조회를 유지한다.
#include "client/RawCarrierInput.h"
#include "client/RawCarrierDropPoint.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 도달한 미연결 효과만 그 지점에서 거부한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("운반체 실제 사용자입력 하위 미연결");return hook; }
constexpr float kInsideExtent=0.9990000128746033F; //원본503b90의 자기 클릭 사각형 폭이다.
}
// 참조하는 원본 판본과 같은 TYPE/음향 저장소 폭을 확인한다.
RawCarrierInput::RawCarrierInput(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const CarrierInputType> audio,std::span<std::uint32_t> stamps,const std::uint32_t& frame,CarrierInputState& state,CarrierInputHooks hooks)
    :pool_(pool),types_(types),audio_(audio),stamps_(stamps),frame_(frame),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188 || audio.size()!=188 || stamps.size()<188) throw std::invalid_argument("운반체 입력의 같은 PATCH/TYPE/음향 장부 누락"); }
// free/dead 필터를 원본 앞에 추가하지 않는다.
o::Sid RawCarrierInput::Number(std::uint32_t value) const { if (!value || value>=pool_.Capacity()) throw std::out_of_range("운반체 입력 SID 물리 범위");return o::Sid{static_cast<std::uint16_t>(value)}; }
// 현재 물리 슬롯을 읽는다.
std::span<const std::uint8_t> RawCarrierInput::Raw(o::Sid parent) const { Number(parent.value);return pool_.Slot(parent); }
// 현재 타입과 실제 배열 범위를 검증한다.
std::uint32_t RawCarrierInput::Type(o::Sid parent) const { const auto type=Raw(parent)[10];if (type>=types_.size()) throw std::out_of_range("운반체 입력 TYPE 범위");return type; }
// 가상 효과 뒤 원본 위치에서 genus를 재조회한다.
std::uint32_t RawCarrierInput::Genus(o::Sid parent) const { return types_[Type(parent)].flags2; }
// 현재 typeFlags를 genus와 구별한다.
std::uint32_t RawCarrierInput::Flags(o::Sid parent) const { return types_[Type(parent)].flags1; }
// 작은 원문 값을 낮은 바이트부터 결합한다.
std::uint32_t RawCarrierInput::Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width) { std::uint32_t value{};
    // 지정한 원본 폭만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[at+i])<<(8*i);return value;
}
// 현재 위치 두 DWORD를 float 변환 없이 해석한다.
o::RawPathPoint RawCarrierInput::Position(o::Sid parent) const { const auto raw=Raw(parent);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 반환 코드가 전역 cursor에 기록되는 분기와 직접 반환되는 분기를 구별한다.
std::uint32_t RawCarrierInput::Evaluate(o::Sid parent,o::RawPathPoint& point) const {
    if (!(Genus(parent)&0x30000U)) Need(hooks_.assertion)(0x7b6);
    if (!(Genus(parent)&0x30000U) || (Raw(parent)[40]&0x20U)) { state_.cursor=6;return 6; }
    if (Need(hooks_.immobile)(parent)) { state_.cursor=6;return 6; }
    const auto position=Position(parent);const auto snapped=RawCarrierDropPoint::Snap({position.x,position.y});const float lowX=snapped[0]-kInsideExtent,lowY=snapped[1]-kInsideExtent; //원본 저장된 float 사각형이다.
    const auto contained=Need(hooks_.content)(parent,0x20);static_cast<void>(Need(hooks_.content)(parent,1));
    if (lowX<=point.x && point.x<=snapped[0] && lowY<=point.y && point.y<=snapped[1]) {
        state_.insidePoint={-10000,-10000};
        if (contained.value && !(Flags(parent)&0x1000000U)) { static_cast<void>(Need(hooks_.dropPoint)(parent,Read(Raw(contained),18,2)));return 6; }
        const auto candidate=Need(hooks_.candidate)(parent,Position(parent));
        if (!(Flags(parent)&0x1000000U) && candidate.first.value) { const auto graph=Need(hooks_.graph)(parent);if (Need(hooks_.graph)(candidate.first)==graph) return 8; }
        return 6;
    }
    const auto candidate=Need(hooks_.candidate)(parent,point);
    if (!(Genus(parent)&0x20000U)) {
        if (!candidate.first.value) {
            o::Sid dais{};
            if ((Genus(parent)&0x200000U) && (dais=Need(hooks_.daisAt)(point)).value && Need(hooks_.altarAllowed)(dais,parent)) { point=Position(dais);state_.cursor=0x11; }
            else if (!Need(hooks_.connected)(Position(parent),point)) {
                o::Sid geyser{},place{};
                if ((Need(hooks_.surface)(point)&2U) && (geyser=Need(hooks_.geyser)(point)).value && Need(hooks_.validGeyser)(parent,geyser)) { point=Position(geyser);state_.cursor=3; }
                else if ((Need(hooks_.surface)(point)&8U) && (place=Need(hooks_.returnPlace)(point)).value) { point=Position(place);state_.cursor=3; }
                else state_.cursor=6;
            } else {
                dais=Need(hooks_.daisAt)(point);
                if (!dais.value) state_.cursor=0x12;
                else { const auto p=Position(dais);point={p.x-3.0F,p.y-3.0F};state_.cursor=3; }
            }
        }
        if (!(Flags(parent)&0x1000000U) && candidate.first.value && (candidate.second || !contained.value)) { const auto graph=Need(hooks_.graph)(parent);if (Need(hooks_.graph)(candidate.first)==graph) state_.cursor=0xd; }
        return state_.cursor;
    }
    if (candidate.first.value && (candidate.second || !contained.value)) return 0xd;
    if ((Need(hooks_.surface)(point)&2U) && Need(hooks_.geyser)(point).value) return 3;
    if ((Need(hooks_.surface)(point)&8U) && Need(hooks_.returnPlace)(point).value) return 3;
    const auto dais=Need(hooks_.daisAt)(point);if (dais.value) { const auto p=Position(dais);point={p.x-3.0F,p.y-3.0F};return 3; }return 0x12;
}
// 실제 선택 모드5와 현재 마우스 폴링이 같은 평가기를 사용한다.
void RawCarrierInput::Highlight(std::uint32_t number,o::RawPathPoint point,std::uint32_t force) const {
    const auto p=RawCarrierDropPoint::Snap({point.x,point.y});point={p[0],p[1]};if (!number) { Need(hooks_.assertion)(0xce);return; }const auto parent=Number(number);
    if (state_.markers) {
        const auto snap=RawCarrierDropPoint::Snap({point.x,point.y});if (state_.markerTypes.empty()) throw std::logic_error("이동 진단 표시 타입 누락");const auto index=state_.markerCounter%static_cast<std::int32_t>(state_.markerTypes.size());if (index<0) throw std::out_of_range("이동 진단 표시 음수 나머지");state_.markerCounter=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.markerCounter)+1);Need(hooks_.marker)({snap[0],snap[1]},state_.markerTypes[static_cast<std::size_t>(index)]);Need(hooks_.refreshCursor)();
    }
    if (!force && !Need(hooks_.multiple)()) { const auto cursor=Evaluate(parent,point);Need(hooks_.selectCursor)(cursor);return; }Need(hooks_.selectCursor)(0x12);
}
// 원본이 현재TYPE를 다시 읽는 시점을 프레임 중복 억제와 구별한다.
void RawCarrierInput::Acknowledge(o::Sid parent,std::int32_t index) const {
    if (stamps_[Type(parent)]==frame_) return;stamps_[Type(parent)]=frame_;
    if (Raw(parent)[10]==state_.priestType) { if (index<0 || index>1) index=Need(hooks_.random)(0,2000)/1000;Need(hooks_.namedSound)(true,static_cast<std::uint32_t>(index));return; }
    if (Raw(parent)[10]==state_.golemType) { if (index<0 || index>4) index=Need(hooks_.random)(0,4000)/1000;Need(hooks_.namedSound)(false,static_cast<std::uint32_t>(index));return; }
    Need(hooks_.sound)(audio_[Type(parent)].moveSound);
}
// 클릭 전의TYPE 포인터와 클릭 후 현재owner/전역을 원문 시점에 각각 읽는다.
void RawCarrierInput::Commit(std::uint32_t number,o::RawPathPoint point) const {
    const auto snap=RawCarrierDropPoint::Snap({point.x,point.y});point={snap[0],snap[1]};if (!number) { Need(hooks_.highlight)(0);return; }const auto parent=Number(number);if (Raw(parent)[40]&1U) return;const auto type=Type(parent);
    if (types_[type].flags2&0x20000U) { if (point.x<=7.0F) point.x=7.0F;if (point.y<=5.0F) point.y=5.0F; }
    const auto cursor=Evaluate(parent,point);
    switch (cursor) {
    case 3:case 0xd:case 0xe:case 0x12:
        if (Raw(parent)[34]==Need(hooks_.localOwner)()) Acknowledge(parent,-1);Need(hooks_.request)(parent,point,0);return;
    case 8:case 9:Need(hooks_.pickup)(parent);Need(hooks_.select)(0,0,1);return;
    case 10:if (Raw(parent)[34]==Need(hooks_.localOwner)()) Need(hooks_.sound)(audio_[type].dropSound);Need(hooks_.dropCargo)(parent,0);[[fallthrough]];
    case 6:case 0xb:case 0xc:Need(hooks_.select)(0,0,1);return;
    case 0x11: {
        const auto dais=Need(hooks_.daisAt)(point);if (!dais.value) { Need(hooks_.assertion)(0x153);return; }
        if (Need(hooks_.checking)() && !(Genus(parent)&0x200000U)) Need(hooks_.assertion)(0x15a);
        if (Need(hooks_.altarAllowed)(dais,parent)) {
            if (Need(hooks_.earlySacrifice)()) { if (Raw(parent)[34]==Need(hooks_.localOwner)()) Need(hooks_.earlyLocal)();else Need(hooks_.earlyRemote)(Raw(parent)[34]);return; }
            Need(hooks_.request)(parent,point,1);if (Raw(parent)[34]==Need(hooks_.localOwner)()) { Acknowledge(parent,-1);state_.dais=dais.value;Need(hooks_.showSacrifice)(dais.value); }
        }return;
    }
    default:Need(hooks_.assertion)(0x179);return;
    }
}
}
