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
// 판본별 Start를 같은 미션 상태/실제 효과에 연결한다. 모든 참조 대상은 처리기보다 오래 살아야 한다.
RawMissionStart::RawMissionStart(OriginalEdition edition,MissionFrameGateState& gate,MissionStartState& state,MissionStartGlobals& globals,MissionStartHooks hooks)
    :edition_(edition),gate_(gate),state_(state),globals_(globals),hooks_(std::move(hooks)) { if (!hooks_.get) throw std::invalid_argument("MissionStart 실제 현재 설정 조회가 필요합니다"); }
// 부분 연결로 원본 전역 하나만 오래된 값에 남기지 않도록 세 조회기를 함께 확인한다.
void RawMissionStart::BindCurrentInputs(MissionStartReads reads) {
    if (!reads.localOwner || (edition_==OriginalEdition::Patch1078 && (!reads.playerKnown || !reads.editor))) throw std::invalid_argument("MissionStart 현재 owner/PlayerKnown/editor 조회 누락");
    reads_=std::move(reads);
}
// 현재 로컬 owner는 SP/기술/AI 순회의 서로 다른 원본 읽기 위치마다 조회한다.
std::uint32_t RawMissionStart::LocalOwner() const { return reads_.localOwner ? reads_.localOwner() : globals_.localOwner; }
// 현재 PlayerKnown이0이면 SP 쓰기와 해당 owner 조회를 모두 생략한다.
std::uint32_t RawMissionStart::PlayerKnown() const { return reads_.playerKnown ? reads_.playerKnown() : globals_.playerKnown; }
// 설정 조회 뒤 현재 editor를 읽어 원본의 고정 시작 금액 선택에 사용한다.
std::uint32_t RawMissionStart::Editor() const { return reads_.editor ? reads_.editor() : globals_.editor; }

// 원본004c3100은 첫 항목 예약을 위해 기존 index를 사용한다.
void RawMissionStart::RefreshTellAt() const {
    const auto text=hooks_.get("tellAtList");const auto source=edition_==OriginalEdition::Cd1072 ? std::string_view(text).substr(0,text.find('\0')):std::string_view(text); // CD00410300도 첫NUL까지인 현재C문자열을 읽는다.
    const auto value=ConfigListItem(source,std::bit_cast<std::int32_t>(state_.tellAtIndex));gate_.tellAtSeconds=value.empty() ? 0.0 : static_cast<double>(ConfigParseLong(std::string_view(value).substr(1)));
}

// 도중의 실제 효과가 현재 owner/시각을 변경하면 그 이후 원본 위치의 조회에 반영한다.
void RawMissionStart::Start() const {
    if (edition_==OriginalEdition::Cd1072) { StartCd();return; } // CD는 owner별SP/편집 자금/PlayerKnown 경로를 실행하지 않는다.
    state_.fieldC0=0;state_.fieldC4=0;
    globals_.DenySalvage()=ConfigParseLong(hooks_.get("denySalvage"));globals_.DenyAscend()=ConfigParseLong(hooks_.get("denyAscend"));globals_.AllowAnyCapture()=ConfigParseLong(hooks_.get("allowAnyCapture"));globals_.OrdinaryGeysers()=ConfigParseLong(hooks_.get("moreGeysers"))==0 ? 1U : 0U;
    const auto demoTimer=ConfigParseLong(hooks_.get("demoRestartTimer"));if (demoTimer!=0) gate_.demoEndSeconds=static_cast<double>(demoTimer)+Need(hooks_.gameSeconds,"MissionStart실제시계누락")();
    const auto names=Need(hooks_.sectionNames,"MissionStart실제섹션목록누락")("@");Need(hooks_.set,"MissionStart실제설정쓰기누락")("tellAtList",names);RefreshTellAt();++state_.sequence;state_.flags&=~1U;
    float money=static_cast<float>(ConfigParseLong(hooks_.get("myStartMoney")));if (Editor()!=0) money=194162.0F;
    if (PlayerKnown()!=0) Need(hooks_.setSp,"MissionStart실제SP지정누락")(LocalOwner(),money);
    Need(hooks_.refreshSp,"MissionStart실제SP화면갱신누락")();Need(hooks_.clearTechnology,"MissionStart실제기술제거누락")(LocalOwner());
    const auto techOwner=LocalOwner();const auto tech=hooks_.get("myTech");Need(hooks_.addTechnology,"MissionStart실제기술추가누락")(techOwner,tech);
    // 실제1..8 사제 존재를 각 owner 위치에서 검사하며 aiName 유무로 참가자를 대신 정하지 않는다.
    for (std::uint32_t owner=1;owner<9;++owner) if (owner!=LocalOwner() || gate_.demo!=0) if (Need(hooks_.hasPriest,"MissionStart실제사제조회누락")(owner)) Need(hooks_.createAi,"MissionStart실제AI생성누락")(owner);
    Need(hooks_.refreshCommandGump,"MissionStart실제명령Gump갱신누락")();if (ConfigParseLong(hooks_.get("randGeysers"))!=0) Need(hooks_.randomGeysers,"MissionStart실제랜덤간헐천생성누락")();
    Need(hooks_.resetCamera,"MissionStart실제카메라초기화누락")();gate_.startSeconds=Need(hooks_.gameSeconds,"MissionStart실제시계누락")();gate_.started=1;
}
// CD0040f840의 시작 순서다. 단일SP 장치와 현재 owner 조회 위치를 PATCH 계약으로 대체하지 않는다.
void RawMissionStart::StartCd() const {
    state_.fieldC0=0;state_.fieldC4=0;
    globals_.DenySalvage()=ConfigParseLong(hooks_.get("denySalvage"));globals_.DenyAscend()=ConfigParseLong(hooks_.get("denyAscend"));globals_.AllowAnyCapture()=ConfigParseLong(hooks_.get("allowAnyCapture"));globals_.OrdinaryGeysers()=ConfigParseLong(hooks_.get("moreGeysers"))==0 ? 1U:0U;
    const auto demoTimer=ConfigParseLong(hooks_.get("demoRestartTimer")); // CD는0이 아닌 정수에만 현재 double 게임 시각을 더한다.
    if (demoTimer!=0) gate_.demoEndSeconds=static_cast<double>(demoTimer)+Need(hooks_.gameSeconds,"CD MissionStart 실제 시계 누락")();
    const auto names=Need(hooks_.sectionNames,"CD MissionStart 실제 섹션 목록 누락")("@"); // CD00511da0의 실제 한 글자 접두다.
    const auto namesText=std::string_view(names).substr(0,names.find('\0')); // CD 원문이 지역256BYTE 배열에 쓴 C문자열의 실제 범위다.
    if (namesText.size()>=256) throw std::length_error("CD MissionStart 섹션 목록의 원문256BYTE 지역 버퍼 범위");
    Need(hooks_.set,"CD MissionStart 실제 설정 쓰기 누락")("tellAtList",namesText);RefreshTellAt();
    state_.flags&=~1U;++state_.sequence; // CD00480ce0은 flags 하위 bit를 먼저 지운 다음 sequence를 증가시킨다.
    const auto money=static_cast<float>(ConfigParseLong(hooks_.get("myStartMoney"))); // CD는 editor/PlayerKnown을 조회하지 않고 실제 정수→float 변환을 전달한다.
    Need(hooks_.setCdSp,"CD MissionStart 단일SP 지정 장치 누락")(money);
    Need(hooks_.clearTechnology,"CD MissionStart 실제 기술 제거 누락")(LocalOwner());
    const auto tech=hooks_.get("myTech"); // CD는 설정 조회가 끝난 뒤 현재 owner를 읽으며 PATCH의 선행 owner 캡처를 사용하지 않는다.
    Need(hooks_.addTechnology,"CD MissionStart 실제 기술 추가 누락")(LocalOwner(),std::string_view(tech).substr(0,tech.find('\0')));
    for (std::uint32_t owner=1;owner<9;++owner) { // CD0040fa22의 각 owner에서 현재 로컬 owner/demo와 실제 사제 존재를 순서대로 읽는다.
        if (owner!=LocalOwner() || gate_.demo!=0) if (Need(hooks_.hasPriest,"CD MissionStart 실제 사제 조회 누락")(owner)) Need(hooks_.createAi,"CD MissionStart 실제 AI 생성 경계 누락")(owner);
    }
    Need(hooks_.refreshCommandGump,"CD MissionStart 실제 명령 Gump 갱신 누락")();
    if (ConfigParseLong(hooks_.get("randGeysers"))!=0) Need(hooks_.randomGeysers,"CD MissionStart 실제 랜덤 간헐천 생성 누락")();
    Need(hooks_.resetCamera,"CD MissionStart 실제 카메라 초기화 누락")();
    const auto now=Need(hooks_.gameSeconds,"CD MissionStart 실제 시계 누락")(); // CD0040faa3의 현재 double 시각을 캡처한 뒤에만 시작 표식을 쓴다.
    gate_.started=1;gate_.startSeconds=now;
}
}
