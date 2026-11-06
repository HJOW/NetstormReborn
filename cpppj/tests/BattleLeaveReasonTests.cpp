// 실제 양수 부모의 원문22행으로 이유별 예약과 두 삭제 사이의 부분 효과를 대조한다.
#include "RawSceneSupport.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/RawBattleLeave.h"

namespace { // 입력 배치와 결과 직렬화만 제공하며 기대 상태를 별도 모델로 계산하지 않는다.
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
// 외부 파일 장치의 선택된 경계에서 원래 부분 효과 관측을 끝낸다.
struct LeaveBoundaryStop { std::string event; }; //원본과 C++ 양쪽에서 기록하는 중단 위치다.
// 다섯 블록 전체를 원문 최초 관측과 같은 순서의720BYTE로 직렬화한다.
std::string LeaveStateBytes(ClientLocalStateGlobals& local) {
    std::string result;
    for (const auto& block:local.Blocks()) { //첫DWORD와+84 외의 미수정 물리도 검사한다.
        if (!result.empty()) result+=':';result+=Hex({reinterpret_cast<const std::uint8_t*>(block.data()),144});
    }
    return result;
}
}

// 선행삭제/공통삭제의 실패도 source/abort/State를 롤백하지 않는 원문 순서를 확인한다.
TEST_CASE(BattleLeaveReason_IndependentNative22) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_BATTLE_LEAVE_REASON_FIXTURE).rows) { //제품 구현 전 고정한 원문 출력만 기대값으로 읽는다.
        CHECK(row.size()==10);if (row.size()!=10) continue;
        ClientLocalStateGlobals local;
        for (auto& block:local.Blocks()) { //다른 인수와 네 상태 블록에 원문과 같은 입력 흔적을 남긴다.
            std::fill_n(reinterpret_cast<std::uint8_t*>(block.data()),144,static_cast<std::uint8_t>(0xa5));
        }
        std::uint32_t script=17,source=133,abort=19,outcome=2,editor=row[0]=="editor",known=row[0]=="mission";
        bool post=true,meta=false,battle=row[0]=="mission";
        std::size_t paths{};std::string cause="normal",trace;
        const auto boundary=[&](std::string event) { //실제 외부 장치 진입 순간의 전역도 기록해 쓰기 순서 차이를 검출한다.
            if (!trace.empty()) trace+=';';trace+=event;
            for (const auto value:{script,source,abort,outcome,local.State().next[0],local.State().current[33],local.State().immediate[33],local.State().next[33]}) { //PE 실행기의 같은8DWORD 순서다.
                trace+=','+std::to_string(value);
            }
            if (event==row[2]) throw LeaveBoundaryStop{std::move(event)};
        };
        const auto remove=[&] { //파일을 실제 삭제하지 않고 path/CRT remove 두 선택 장치를 원래 순서로 관측한다.
            ++paths;boundary("path"+std::to_string(paths));boundary("delete"+std::to_string(paths));
        };
        try { PrepareBattleLeaveState(static_cast<std::int32_t>(Number(row[1])),{{local.State(),abort,outcome,post,editor,meta,battle},script,source,known},remove); }
        catch (const LeaveBoundaryStop& stopped) { cause=stopped.event; }
        CHECK(cause==row[3] && script==Number(row[4]) && source==Number(row[5]) && abort==Number(row[6]) && outcome==Number(row[7]));
        CHECK(LeaveStateBytes(local)==row[8]);CHECK((trace.empty()?"-":trace)==row[9]);++count;
    }
    CHECK(count==22);
}

// 확인 요청은별도 관문에서 처리하므로 양수 함수는 잘못된 signed 입력과 선택된 장치 누락을 명시 거부한다.
TEST_CASE(BattleLeaveReason_PositiveContractAndSelectedDeletion) {
    ClientLocalStateGlobals local;std::uint32_t script=17,source=133,abort=19,outcome=2,editor{},known=1;
    bool post=true,meta=false,battle=true;
    const BattleLeaveStateGlobals globals{{local.State(),abort,outcome,post,editor,meta,battle},script,source,known};
    for (const auto reason:{INT32_MIN,-1,0}) { //음수와0은 기존 확인 안내의 영역이며 전역을 바꾸지 않는다.
        bool rejected=false;try { PrepareBattleLeaveState(reason,globals,{}); } catch (const std::invalid_argument&) { rejected=true; }
        CHECK(rejected && script==17 && source==133 && abort==19 && outcome==2 && local.State().next[0]==0);
    }
    PrepareBattleLeaveState(2,globals,{});CHECK(script==1 && source==133 && abort==19 && outcome==2 && local.State().next[0]==3);
    local.State().next[0]=0;script=17;
    bool missing=false;try { PrepareBattleLeaveState(3,globals,{}); } catch (const std::logic_error&) { missing=true; }
    CHECK(missing && script==1 && source==133 && abort==19 && outcome==2 && local.State().next[0]==0);
}
