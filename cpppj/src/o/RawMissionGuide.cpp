// Guide의 단계 변경 뒤 현재 포인터 재조회와 같은 호출 내 반복을 보존한다.
#include "o/RawMissionGuide.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 미연결 가상 단계/해제를 정상 시작으로 간주하지 않는다.
RawMissionGuide::RawMissionGuide(OriginalEdition edition,MissionGuideHooks hooks):hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || !hooks_.current || !hooks_.pending || !hooks_.dialogsClear ||
        !hooks_.stage || !hooks_.postpone || !hooks_.release || !hooks_.invalidStage)
        throw std::invalid_argument("Guide의 PATCH 현재 미션/상태/UI/가상 단계 경계 누락");
}
// 실제 원문 역참조의 잘못된 NULL 입력만 호스트 예외로 보고한다.
MissionStartState& RawMissionGuide::RequireCurrent() const {
    auto* current=hooks_.current();if (!current) throw std::logic_error("Guide 가상 단계의 현재 미션 NULL");return *current;
}
// unsigned DWORD는 원본 add의 감김을 정의된 C++ 연산으로 유지한다.
void RawMissionGuide::Advance(MissionStartState& state) { ++state.sequence;state.flags&=~1U; }
// 상위 State/창이 막히면 가상+20만 실행하고 시작 가상 함수를 앞당기지 않는다.
void RawMissionGuide::Run() const {
    // 원본 do 루프는 현재 단계가 바뀐 경우에만 다음 가상 단계까지 이어 간다.
    for (;;) {
        auto* current=hooks_.current();if (!current) return;
        const auto before=current->sequence; // 외부 효과 전 현재 단계의 비교용 DWORD다.
        if (current->flags&8U) { if (hooks_.current()) hooks_.release();return; }
        if (!hooks_.pending() && hooks_.dialogsClear()) {
            auto& selected=RequireCurrent();if (selected.flags&1U) Advance(selected);
            const auto sequence=RequireCurrent().sequence; // advance 이후 실제 switch가 읽는 현재 단계 DWORD다.
            // 원본은 switch의 단계 번호 조회와 가상 호출 대상 조회 사이에도 현재 포인터를 다시 읽는다.
            if (sequence<=6U) { hooks_.stage(RequireCurrent(),sequence);if (sequence==6U) hooks_.release(); }
            else hooks_.invalidStage();
        } else hooks_.postpone(RequireCurrent());
        current=hooks_.current();if (!current || current->sequence==6U || current->sequence==before) return;
    }
}
}
