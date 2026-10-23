// c79b0 실제 발화의 순서/정수 보존과 현재 입력 큐/지연 OnExit를 비교한다.
#include "RawSceneSupport.h"
#include "client/DialogCommandLifetime.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kParent=0x12401000U; // 독립 PE 관측이 현재 부모에 부여한 명시 토큰이다.
}
// 전체 258행 가운데 명령이 발화된 252행의 세 정수/예약→원래 부모 Kill 순서를 전부 대조한다.
TEST_CASE(DialogCommandLifetime_Native252) {
    std::size_t rows{},fired{},instant{},released{}; // 원문 비교 범위와 두 입력 축의 실제 비교 수다.
    for (const auto& row:LoadFixture(NETSTORM_DIALOG_COMMAND_LIFETIME_FIXTURE).rows) { // 기대값은 독립 원문 출력이며 실행 중 재작성하지 않는다.
        CHECK(row.size()==13);if (row.size()!=13) return;
        ++rows;if (!Number(row[10])) continue; // helper 앞의 선택/fallback은 이 core API의 범위가 아니다.
        DialogExitCommandState selected{Number(row[6]),Number(row[7])};std::string trace,queued; // 이미 메타 해석이 끝난 원문 정수 입력이다.
        FireDialogCommand(kParent,selected,[&](auto command,auto first,auto second) {
            queued=std::to_string(command)+','+std::to_string(first)+','+std::to_string(second);trace="Queue,"+queued;
        },[&](auto parent) { trace+=";Kill,"+std::to_string(parent); });
        CHECK(Number(row[9])==1 && Number(row[10])==1 && queued==row[11] && trace==row[12]);
        ++fired;if (Number(row[5])) ++instant;if (Number(row[1])&0x80000000U) ++released;
    }
    CHECK(rows==258 && fired==252 && instant==126 && released==126);
}
// 기존 큐의 선행 명령/선택 명령/나중 Cleanup의 OnExit가 같은 순서와 원래 DWORD를 유지한다.
TEST_CASE(DialogCommandLifetime_CurrentQueueDeferredExit) {
    InputQueue queue;queue.PushCommand(0x0200006dU,17,17);
    DialogExitCommandState selected{0x02000095U,0xffffffffU},exit{0x02000001U,99}; // 선택 인자는 문자열 토큰처럼 signed 음수 표현도 비트 보존한다.
    std::uint32_t liveParent=kParent,kills{};bool retired{}; // Kill은 현재 표시를 내리지만 OnExit를 직접 실행하지 않는다.
    FireDialogCommand(liveParent,selected,queue,[&](auto parent) {
        CHECK(parent==kParent);++kills;retired=true;liveParent=0;
        selected={0,0}; // Kill callback이 원래 저장소를 지워도 이미 예약된 선택 값은 바뀌지 않는다.
    });
    CHECK(kills==1 && retired && !liveParent && exit.command==0x02000001U);
    const auto first=queue.Pop(false),second=queue.Pop(false); // 실제 입력 드레인은 Cleanup 전에 현재 선택을 소비한다.
    CHECK(first.code==0x0200006dU && first.x==17 && first.y==17);
    CHECK(second.code==0x02000095U && std::bit_cast<std::uint32_t>(second.x)==0xffffffffU && second.x==second.y && queue.Empty());
    DisposeDialogExitCommand(exit,queue); // 같은 부모의 후반 Cleanup만 기존 OnExit 예약 입구를 부른다.
    const auto last=queue.Pop(false);CHECK(last.code==0x02000001U && last.x==99 && last.y==99 && queue.Empty());
}
// 원문처럼 예약 당시의 command/parameter와 원래 this를 보존하고 callback 이후 현재 페이지를 잘못 닫지 않는다.
TEST_CASE(DialogCommandLifetime_CallbackMutationKeepsSource) {
    DialogExitCommandState selected{0,0xffffffffU};std::uint32_t currentParent=kParent,queued{},killed{}; // code0도 실제 선택이면 큐에 남는다.
    FireDialogCommand(currentParent,selected,[&](auto command,auto first,auto second) {
        ++queued;CHECK(!command && first==0xffffffffU && second==first);selected={123,456};currentParent=777;
    },[&](auto parent) { ++killed;CHECK(queued==1 && parent==kParent && currentParent==777); });
    CHECK(queued==1 && killed==1 && selected.command==123 && currentParent==777);
    bool rejected{};queued=0;
    try { FireDialogCommand(kParent,selected,[&](auto,auto,auto) { ++queued; },{}); }
    catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && !queued); // 호스트의 미연결 장치는 부분 발화 전에 명시 거부한다.
}
