// 실제 원문 확인 관문과 상위 사건 분배의 별도 책임을 대조한다.
#include "RawSceneSupport.h"
#include "client/RawBattleLeave.h"
#include "client/MissionGuideInput.h"

using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { // Tell 진입 중단을 정상 안내 복귀와 구별한다.
struct RetreatTellStop {}; // 원문 큰 하위의 선택된 중단 표식이다.
}

// 양수 본문은 원문도 관측 경계에서 멈춘다. 실제 이탈/희생/승리나 MP를 재현하지 않는다.
TEST_CASE(RetreatConfirmation_IndependentNativeSignedBoundary18) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_RETREAT_CONFIRMATION_FIXTURE).rows) { // 최초 CPU 출력만 기대값으로 읽는다.
        CHECK(row.size()==5);if (row.size()!=5) continue;
        const auto reason=static_cast<std::int32_t>(std::stoll(row[0]));
        std::string section;bool stopped=false,handled=false;
        try {
            handled=DispatchBattleRetreatConfirmation({kLeaveBattleCommand,77,reason},[&](auto name) { section=name;if (Number(row[1])) throw RetreatTellStop{}; });
        } catch (const RetreatTellStop&) { stopped=true; }
        CHECK(stopped==(Number(row[2])==0));
        CHECK(stopped || handled==(Number(row[3])==0));
        CHECK((section.empty() ? "-" : section)==row[4]);++count;
    }
    CHECK(count==18);
}

// 뗌/다른 명령은 Tell 장치를 요구하지 않는다. 첫 인수나 modifier를 이유로 해석하지 않는다.
TEST_CASE(RetreatConfirmation_EventRoutingAndRequiredTell) {
    for (const auto reason:{INT32_MIN,-1,0,1,INT32_MAX}) { // signed 전체 경계에서 상위 release를 먼저 적용한다.
        CHECK(DispatchBattleRetreatConfirmation({kLeaveBattleCommand|InputCode::kRelease,91,reason},{}));
    }
    CHECK(!DispatchBattleRetreatConfirmation({kMissionAbortCommand,0,0},{}));
    CHECK(!DispatchBattleRetreatConfirmation({kLeaveBattleCommand,0,1},{}));
    bool missing=false;
    try { static_cast<void>(DispatchBattleRetreatConfirmation({kLeaveBattleCommand,1,0},{})); } catch (const std::logic_error&) { missing=true; }
    CHECK(missing);
    std::size_t calls{};
    CHECK(DispatchBattleRetreatConfirmation({kLeaveBattleCommand|InputCode::kControl|InputCode::kShift,INT32_MAX,-1},[&](auto section) { CHECK(section=="ConfirmRetreat");++calls; }));
    CHECK(calls==1);
}
