// 실제 Handle은 count0에서 효과 없이 원래payload를 반환하며 새Regular 예약과 Templateowner 재조회를 보존한다.
#include "o/RawPlayerRegular.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kLogGroup=29,kFirstEvent=1,kSecondEvent=2; // 원본 로그그룹/소어루저 확인의두사건번호다.
constexpr float kSecondPayload=60.0F; // 원본42700000 비트의 새Regular2 payload이며 최초time 지연이 아니다.
}
// 최초 handler 호출 전에 actual Player와 빠짐없는 필수 하위 효과를 확인한다.
RawPlayerRegular::RawPlayerRegular(const RawPlayerWorld& player,const SquidOwnerMode& mode,PlayerRegularHooks hooks)
    :player_(player),mode_(mode),hooks_(std::move(hooks)) {
    if (!hooks_.assertion || !hooks_.log || !hooks_.reserveRegular || !hooks_.addRegular || !hooks_.templateState || !hooks_.templateChanged)
        throw std::invalid_argument("Player Regular 실제 로그/예약/현재Template 효과 누락");
}
// 실제 Player VT만 판정하며 다른 타입의 Handler를 기본본문으로 덮지 않는다.
bool RawPlayerRegular::Handles(Sid sid) const { return player_.Handles(sid); }
// count/event/payload를 같은원본폭으로 읽으며 firstevent/count0은 원래float를 그대로 돌려준다.
float RawPlayerRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::logic_error("Player Regular 미복원 가상Handler 표");
    return RunPlayerRegular(Pool(),player_.State(),mode_,sid,event,count,payload,hooks_);
}
float RunPlayerRegular(const SidPool& pool,const PlayerWorldState& state,const SquidOwnerMode& mode,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const PlayerRegularHooks& hooks) { // 현재 두 owner/battle과 원문 호출 순서/반환을 복사 없이 공유한다.
    const auto raw=pool.Slot(sid);const auto owner=raw[pool.Edition()==OriginalEdition::Patch1078 ? 34U : 32U];
    if (owner!=state.battleOwner || !count) return payload;
    if (event==kFirstEvent) {
        hooks.log(kLogGroup,"SORE LOSER TEST: ");
        if (mode.battle) {
            hooks.log(kLogGroup,"STARTING.\n");if (hooks.reserveRegular()) hooks.addRegular(sid,kSecondEvent,kSecondPayload);return 0.0F;
        }
        hooks.log(kLogGroup,"NOT STARTING YET.\n");return payload;
    }
    if (event==kSecondEvent) {
        if (mode.battle) {
            const auto pin=hooks.templatePin;const auto hold=pin ? pin():std::shared_ptr<void>{}; // Native의 실제 owner는 반환 span 쓰기와 현재 owner Notify까지 유지한다.
            auto bytes=hooks.templateState(state.templateOwner);if (bytes.empty()) throw std::out_of_range("Player 실제Template State BYTE 누락");
            bytes[0]|=4U;hooks.templateChanged(state.templateOwner);
        }
        return 0.0F;
    }
    hooks.assertion(PlayerRegularAssertion::UnknownEvent);return payload;
}
// 현재 Player/Post/Host와 동일한 풀을 상위 분배기에서 검증한다.
const SidPool& RawPlayerRegular::Pool() const { return player_.Pool(); }
}
