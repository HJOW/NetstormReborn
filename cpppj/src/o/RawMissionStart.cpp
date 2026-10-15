// Start의 시간 정밀도·현재 owner 재조회와 모든 부수효과의 원본 순서를 보존한다.
#include "o/RawMissionStart.h"
#include "o/ConfigInterface.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된 실제 효과만 필수로 요구하며 빈 callback을 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function,const char* name) { if (!function) throw std::logic_error(name);return function; }
}
// 이 복원 단위는 PATCH전용이다. 모든 참조 대상은 처리기보다 오래 살아야 한다.
RawMissionStart::RawMissionStart(OriginalEdition edition,MissionFrameGateState& gate,MissionStartState& state,MissionStartGlobals& globals,MissionStartHooks hooks)
    :gate_(gate),state_(state),globals_(globals),hooks_(std::move(hooks)) { if (edition!=OriginalEdition::Patch1078 || !hooks_.get) throw std::invalid_argument("MissionStart는PATCH현재설정조회가필요합니다"); }

// 원본004c3100은 첫 항목 예약을 위해 기존 index를 사용한다.
void RawMissionStart::RefreshTellAt() const {
    const auto value=ConfigListItem(hooks_.get("tellAtList"),std::bit_cast<std::int32_t>(state_.tellAtIndex));gate_.tellAtSeconds=value.empty() ? 0.0 : static_cast<double>(ConfigParseLong(std::string_view(value).substr(1)));
}

// 도중의 실제 효과가 현재 owner/시각을 변경하면 그 이후 원본 위치의 조회에 반영한다.
void RawMissionStart::Start() const {
    state_.fieldC0=0;state_.fieldC4=0;
    globals_.denySalvage=ConfigParseLong(hooks_.get("denySalvage"));globals_.denyAscend=ConfigParseLong(hooks_.get("denyAscend"));globals_.allowAnyCapture=ConfigParseLong(hooks_.get("allowAnyCapture"));globals_.ordinaryGeysers=ConfigParseLong(hooks_.get("moreGeysers"))==0 ? 1U : 0U;
    const auto demoTimer=ConfigParseLong(hooks_.get("demoRestartTimer"));if (demoTimer!=0) gate_.demoEndSeconds=static_cast<double>(demoTimer)+Need(hooks_.gameSeconds,"MissionStart실제시계누락")();
    const auto names=Need(hooks_.sectionNames,"MissionStart실제섹션목록누락")("@");Need(hooks_.set,"MissionStart실제설정쓰기누락")("tellAtList",names);RefreshTellAt();++state_.sequence;state_.flags&=~1U;
    float money=static_cast<float>(ConfigParseLong(hooks_.get("myStartMoney")));if (globals_.editor!=0) money=194162.0F;
    if (globals_.playerKnown!=0) Need(hooks_.setSp,"MissionStart실제SP지정누락")(globals_.localOwner,money);
    Need(hooks_.refreshSp,"MissionStart실제SP화면갱신누락")();Need(hooks_.clearTechnology,"MissionStart실제기술제거누락")(globals_.localOwner);
    const auto techOwner=globals_.localOwner;const auto tech=hooks_.get("myTech");Need(hooks_.addTechnology,"MissionStart실제기술추가누락")(techOwner,tech);
    // 실제1..8 사제 존재를 각 owner 위치에서 검사하며 aiName 유무로 참가자를 대신 정하지 않는다.
    for (std::uint32_t owner=1;owner<9;++owner) if (owner!=globals_.localOwner || gate_.demo!=0) if (Need(hooks_.hasPriest,"MissionStart실제사제조회누락")(owner)) Need(hooks_.createAi,"MissionStart실제AI생성누락")(owner);
    Need(hooks_.refreshCommandGump,"MissionStart실제명령Gump갱신누락")();if (ConfigParseLong(hooks_.get("randGeysers"))!=0) Need(hooks_.randomGeysers,"MissionStart실제랜덤간헐천생성누락")();
    Need(hooks_.resetCamera,"MissionStart실제카메라초기화누락")();gate_.startSeconds=Need(hooks_.gameSeconds,"MissionStart실제시계누락")();gate_.started=1;
}
}
