// 실제 current type 첫 프레임과 프레임 뒤604 예약을 기존 모듈에서 실행한다.
#include "o/RawPriestRitualActions.h"
#include "o/SquidFrame.h"
#include "o/SquidProcess.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kRitualFollowEvent=0x25c,kLastRitualTableStage=5; // 후속604 사건과 K..P 첫 프레임 표의 마지막 단계다.
constexpr float kRitualFollowPayload=0.01F; // 원본604 SharedRegular payload의 float 비트3c23d70a다.
}
// 판본별 표 폭과 세 필수 경계를 첫 raw 접근 전에 확인한다.
RawPriestRitualActions::RawPriestRitualActions(const SidPool& pool,std::span<const RiftTypeFrames> frames,PriestRitualActionHooks hooks)
    :pool_(pool),frames_(frames),hooks_(std::move(hooks)) {
    if (frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.setFrame || !hooks_.reserveShared || !hooks_.addShared)
        throw std::invalid_argument("사제 의식 프레임/예약 연결 누락");
}
// 원본의 슬롯 주소가 pool+stride 이상이고 pool+capacity*stride 미만인 번호다.
bool RawPriestRitualActions::Valid(std::uint32_t sid) const { return sid && sid<pool_.Capacity() && sid<=std::numeric_limits<std::uint16_t>::max(); }
// 타입+0x158은 K 첫 프레임이며 stage마다 DWORD 하나씩 증가한다. 기존 LetterRun 결과를 다시 만들지 않는다.
void RawPriestRitualActions::Begin(std::uint32_t sid,std::uint32_t stage) const {
    if (!Valid(sid)) return;if (stage>kLastRitualTableStage) throw std::out_of_range("사제 의식 단계가 first-table 밖입니다");const Sid parent{static_cast<std::uint16_t>(sid)};const auto type=pool_.Slot(parent)[10];
    if (type>=frames_.size()) throw std::out_of_range("사제 의식 현재 raw 타입 범위");hooks_.setFrame(parent,frames_[type].Run(static_cast<std::uint8_t>('K'+stage)).first,0);
}
// frame callback 뒤 상태가 달라도 원본은 처음 유효했던 번호의 예약을 계속 요청한다.
void RawPriestRitualActions::Continue(std::uint32_t sid,std::uint32_t stage) const {
    if (!Valid(sid)) return;Begin(sid,stage);
    if (hooks_.reserveShared()) hooks_.addShared(Sid{static_cast<std::uint16_t>(sid)},kRitualFollowEvent,kRitualFollowPayload);
}
// 표시/예약 어댑터가 다른 풀로 연결되지 않도록 제공한다.
const SidPool& RawPriestRitualActions::Pool() const { return pool_; }
// 기존 프레임/공유예약 몸체를 정확한 현재 번호와 인자로 공급한다.
PriestRitualActionHooks MakePriestRitualActionHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,PriestRitualActionHooks hooks) {
    if (&frame.Pool()!=&pool || &host.Pool()!=&pool) throw std::invalid_argument("사제 의식 하위 SID 풀 불일치");
    hooks.setFrame=[&frame](Sid sid,std::int32_t index,std::uint32_t flags) { frame.Set(sid,index,flags); };
    hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
    return hooks;
}
}
