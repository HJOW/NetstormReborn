// 원본 커서 내려놓기의 전체 분기와 효과 순서를 복원한다. 실제 게임/OS 호출은 경계 밖이다.
#include "client/RawCursorDrop.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 Form의 플래그 WORD 위치다. 품질 위치는 판본마다 다르다.
constexpr std::size_t kFlags=20;
// CRT ftol은 절삭한 64비트 정수의 하위 DWORD를 쓴다. NaN/64비트 범위 밖의 불확정값은 하위 DWORD가 0이다.
std::int32_t Ftol(float value) {
    const double number=value;
    if (!std::isfinite(number) || std::fabs(number)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(number))));
}
// 부호 있는 32비트 정수를 DWORD로 해석한다. 화면 좌표 산술은 원본처럼 넘침을 보존한다.
std::uint32_t Word(std::int32_t value) { return std::bit_cast<std::uint32_t>(value); }
}
RawCursorDrop::RawCursorDrop(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& state,CursorDropHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.empty() || !hooks_.queryCursor || !hooks_.prepareCursor || !hooks_.highlight || !hooks_.playDrop || !hooks_.previewSource ||
        !hooks_.destroy || !hooks_.pop || !hooks_.record || !hooks_.placeLocal || !hooks_.addLocalSp || !hooks_.showSp || !hooks_.battleRefresh ||
        !hooks_.invalidGui || !hooks_.guiFlags || !hooks_.guiParent || !hooks_.detach || !hooks_.attach || !hooks_.guiReceive || !hooks_.returnPiece ||
        !hooks_.activatePreview || !hooks_.selectCursor || !hooks_.refresh || !hooks_.closeGump) throw std::invalid_argument("커서 내려놓기 자료/효과 연결 오류");
}
float RawCursorDrop::Round(float value) {
    if (!std::isfinite(value)) throw std::invalid_argument("커서 좌표가 유한하지 않습니다");
    return static_cast<float>(std::floor(static_cast<double>(value)+0.5));
}
const o::RiftTypeRecord& RawCursorDrop::Type() const {
    if (state_.type>=types_.size()) throw std::out_of_range("커서 타입 번호 오류");
    return types_[state_.type];
}
o::Sid RawCursorDrop::SourceSid(std::uint32_t value) const {
    if (!value || value>=pool_.Capacity() || value>0xffff) throw std::out_of_range("커서 연관 SID 오류");
    return o::Sid{static_cast<std::uint16_t>(value)};
}
//원문의정수포인터계산은자료를읽지않는다. 실제호스트소비직전까지큰DWORD와선행효과를보존한다.
o::Sid RawCursorDrop::PreviewSid(std::uint32_t value) const {
    if (value<5 || value>=pool_.Capacity() || value>0xffffU) throw std::out_of_range("커서 미리보기 DWORD의 실제Pool/SID16범위");
    return o::Sid{static_cast<std::uint16_t>(value)};
}
void RawCursorDrop::Lock() {
    if (!state_.lockDepth) { hooks_.activatePreview(0);hooks_.selectCursor(1); }
    ++state_.lockDepth;
}
void RawCursorDrop::Charge() {
    if (!(state_.costFlags&0x40)) return;
    hooks_.addLocalSp(-Type().cost);
    // 가산 경계 뒤 비용/발자국/화면 좌표를 다시 읽는다. /2는 음수도 0쪽으로 절삭한다.
    const auto& type=Type();const auto pixels=std::bit_cast<std::int32_t>(Word(type.footX)<<4);
    const auto x=std::bit_cast<std::int32_t>(Word(state_.screenX)-Word(pixels/2)-Word(state_.cursorWidth/2));
    hooks_.showSp(Ftol(type.cost),x,state_.screenY,0);
    if (state_.battle && !state_.blocked) hooks_.battleRefresh();
}
void RawCursorDrop::Drop() {
    if (state_.lockDepth) return;
    hooks_.queryCursor();if (!state_.legal) return;
    const auto preview=state_.preview;
    hooks_.prepareCursor();state_.x=Round(state_.x);state_.y=Round(state_.y);hooks_.prepareCursor();
    hooks_.highlight(0);hooks_.playDrop();
    if (!state_.legalGump) {
        if (state_.inputFlags&0x80) {
            const auto captured=PreviewSid(preview);const auto source=SourceSid(hooks_.previewSource(captured));
            hooks_.pop(source,state_.x,state_.y,0);hooks_.destroy(captured,0);
        } else {
            if (state_.recording) hooks_.record(state_.recorder,Type().name,state_.x,state_.y,state_.argument,state_.direction);
            const float x=Round(state_.x),y=Round(state_.y);Lock();
            hooks_.placeLocal({PreviewSid(preview),x,y,state_.argument,state_.direction});
        }
        Charge();
    } else {
        const auto allowed=hooks_.guiFlags(state_.legalGump);
        const auto captured=PreviewSid(preview);const auto first=hooks_.previewSource(captured),second=hooks_.previewSource(captured);
        const auto raw=pool_.Slot(captured);
        const auto flags=static_cast<std::uint32_t>(raw[kFlags])|(static_cast<std::uint32_t>(raw[kFlags+1])<<8);
        const auto quality=static_cast<std::uint32_t>(raw[pool_.Edition()==o::OriginalEdition::Patch1078 ? 30 : 26]);
        if (allowed&flags) { hooks_.invalidGui();return; }
        hooks_.detach(captured,0);
        // detach 뒤 현재 슬롯에 WORD만 쓴다. detach의 다른 raw 변경은 보존한다.
        auto writable=pool_.AllocatedBytes(captured);
        writable[kFlags]=static_cast<std::uint8_t>(allowed);writable[kFlags+1]=static_cast<std::uint8_t>(allowed>>8);
        const auto parent=hooks_.guiParent(state_.legalGump);
        if (!parent) throw std::logic_error("cursorLegalGump->parentSid 원본 assert 계약 위반");
        hooks_.attach(captured,parent,0x10);hooks_.guiReceive(state_.legalGump,captured);
        if (state_.inputFlags&0x80) hooks_.destroy(SourceSid(first),0);
        else if (flags&8) hooks_.returnPiece(flags,second,quality);
    }
    Lock();hooks_.refresh(0,0,1);
    if (state_.cursorGump) { hooks_.closeGump(state_.cursorGump,3,0,0);state_.cursorGump=0; }
    state_.preview={};hooks_.activatePreview(1);
}
const o::SidPool& RawCursorDrop::Pool() const { return pool_; }
CursorDropHooks MakeCursorDropHooks(const o::SidPool& pool,o::RawConstructionLocal& local,o::SquidReward& reward,
    SoundPlayer& sound,const CursorDropState& state,CursorDropHooks hooks) {
    if (&pool!=&local.Pool() || &pool!=&reward.Pool()) throw std::invalid_argument("커서/로컬 배치/보상 SID 풀 연결 오류");
    hooks.placeLocal=[&local](const o::ConstructionLocalRequest& request) { local.Place(request); };
    hooks.addLocalSp=[&reward,&state](float delta) { reward.AddSp(state.localPlayer,delta); };
    hooks.playDrop=[&sound] { sound.PlayByName("droppiece.wav",0,0,0,0,0); };
    return hooks;
}
}
