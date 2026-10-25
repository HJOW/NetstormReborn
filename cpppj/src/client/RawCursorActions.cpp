// 세 원본 몸체는 정수 DWORD 감김과 callback 뒤 재조회를 그대로 유지한다.
#include "client/RawCursorActions.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 도달한 필수 경계가 비었으면 계산을 대체하지 않고 해당 위치에서 거부한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("실제 커서 잠금/회전 경계 미연결");return hook; }
}
// 다른 타입 개수의 표를10.78 레이아웃에 섞지 않는다.
RawCursorActions::RawCursorActions(std::span<const o::RiftTypeRecord> types,CursorDropState& drop,CursorLifecycleState& lifecycle,
    const std::uint32_t& specialType,const std::uint32_t& rangeProcessType,CursorActionHooks hooks)
    :types_(types),drop_(drop),lifecycle_(lifecycle),specialType_(specialType),rangeProcessType_(rangeProcessType),hooks_(std::move(hooks)) {
    if (types_.size()!=188) throw std::invalid_argument("커서 동작10.78 TYPE 계약 오류");
}
// Activate가 깊이를 변경해도 처음 분기의 select 조건과 마지막 증가를 유지한다.
void RawCursorActions::Lock(std::uint32_t skipCursor) const {
    if (!drop_.lockDepth) { Need(hooks_.activate)(0);if (!skipCursor) Need(hooks_.selectCursor)(1); }
    ++drop_.lockDepth;
}
// Create가 바꾼 legalGump는 커서 번호를 고르는 다음 접근에서 읽는다.
void RawCursorActions::Unlock() const {
    --drop_.lockDepth;
    if (!drop_.lockDepth) { Need(hooks_.createPreviews)();Need(hooks_.selectCursor)(drop_.legalGump ? 3U : 2U); }
}
// signed remainder를 unsigned modulo8로 바꾸지 않고 음수/큰 DWORD 입력도 원문과 대조한다.
void RawCursorActions::Rotate(std::int32_t step) const {
    if (!drop_.lockDepth) { Need(hooks_.activate)(0);Need(hooks_.selectCursor)(1); }
    ++drop_.lockDepth;auto direction=(drop_.direction+8U+static_cast<std::uint32_t>(step))&0x80000007U;
    if (std::bit_cast<std::int32_t>(direction)<0) direction=((direction-1U)|0xfffffff8U)+1U;
    drop_.direction=direction;Need(hooks_.sound)("rotatepiece.wav");--drop_.lockDepth;
    if (!drop_.lockDepth) { Need(hooks_.createPreviews)();Need(hooks_.selectCursor)(drop_.legalGump ? 3U : 2U); }
    if (lifecycle_.abstractType>=types_.size()) throw std::out_of_range("커서 회전 현재 TYPE 범위");
    if (types_[lifecycle_.abstractType].group==1 || drop_.type==specialType_) {
        Need(hooks_.removeRange)(CursorPreviewSid(lifecycle_.previews[0]),rangeProcessType_);
        drop_.cursorGump=Need(hooks_.createRange)(CursorPreviewSid(lifecycle_.previews[0]),0);
    }
}
}
