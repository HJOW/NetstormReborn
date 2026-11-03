// 실제 CPU 시간/서식과 현재 Client 옵션/기한을 창·통신 없이 대조한다.
#include "RawSceneSupport.h"
#include "client/RawMonitorTime.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include <bit>
#include <map>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // Native 입력/출력만 fixture에서 읽고 외부 장치 입력을 기록한다.
const auto& TimerRows() { static const auto rows=LoadFixture(NETSTORM_MONITOR_TIME_FIXTURE).rows;return rows; } // 실제1176행이다.
const auto& TemporaryRows() { static const auto rows=LoadFixture(NETSTORM_TEMPORARY_CHAT_FIXTURE).rows;return rows; } // 실제12행이다.
std::string MonitorHex(std::span<const std::uint8_t> bytes) { constexpr char digits[]="0123456789abcdef";std::string text;for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; }return text; } // 원문 저장 순서를 고정16진수로 출력한다.
std::string TextHex(std::string_view text) { return MonitorHex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 개행을 TSV 구분자로 내보내지 않는다.
// Native와 같은 명시 외부 시계/달력/Config/제출 입력만 조립한다.
std::vector<std::string> ReplayTimer(const std::vector<std::string>& row) {
    MonitorTimeState state{Number(row[2]),std::bit_cast<double>(std::stoull(row[3])),std::stoi(row[4]),std::stoi(row[5])};double wall=std::bit_cast<double>(std::stoull(row[6]));const auto increment=std::bit_cast<double>(std::stoull(row[7]));const auto steps=Number(row[8]),changes=Number(row[9]);std::uint32_t calls{};std::string trace,name="Alpha";MonitorTimeHooks hooks;
    hooks.wallSeconds=[&] { const auto now=wall;wall+=increment;++calls;trace+="W,"+std::to_string(std::bit_cast<std::uint64_t>(now))+";";if ((changes&1U) && calls==1) state.deadline=10000;return now; };
    hooks.calendar=[&] { trace+="T;L;A;";if (changes&2U) name="Bravo";return "Wed Nov  4 01:00:00 2026\n"; };hooks.handle=[&]() -> std::string_view { return name; };
    hooks.line=[&](auto handle,auto calendar) { trace+="P,"+TextHex("%s: !%s\n")+','+TextHex(handle)+','+TextHex(calendar)+";";return FormatMonitorTimeLine(handle,calendar); };
    hooks.format=[&] { trace+="Q,"+TextHex("monitorFormat")+",0,0,0;";if (changes&4U) name="Bravo";return "~9~0"; };
    hooks.submit=[&](auto format,auto line) { trace+="E,"+TextHex(format)+','+TextHex(line)+";";if (changes&8U) { state.monitor=0;state.alert=0; }if (changes&16U) state.deadline=-4.25; };
    for (std::uint32_t step=0;step<steps;++step) StepMonitorTime(state,hooks); // 행 안에서는 같은 현재 상태를 재시드하지 않는다.
    return {std::to_string(state.initialized),std::to_string(std::bit_cast<std::uint64_t>(state.deadline)),std::to_string(state.monitor),std::to_string(state.alert),std::to_string(calls),trace.empty() ? "-" : trace}; // CD의 무효과 atexit 등록은 아래 별도 판본 경계로 검증한다.
}
// 실제 임시서식 부모의 두 manager 전체256BYTE를 비교하는 장면이다.
struct TemporaryScene {
    std::uint32_t current{0x601000},changes{}; // Native와 같은 토큰/현재 manager 변경 입력이다.
    bool patch{};std::map<std::uint32_t,std::vector<std::uint8_t>> memory;std::string trace; // 같은 실제 물리와 관측 요청이다.
    TemporaryChatFormatHooks hooks; // 위 저장소보다 먼저 소멸하는 실제 경계 뷰다.
    TemporaryScene(bool edition,std::uint32_t action):changes(action),patch(edition),memory{{0x601000,std::vector<std::uint8_t>(256,0xab)},{0x602000,std::vector<std::uint8_t>(256,0xcd)}} {
        const auto offset=patch ? 0x14U : 0x10U;const std::string first="saved-prefix",second="second-prefix";std::copy(first.begin(),first.end(),memory.at(0x601000).begin()+offset);memory.at(0x601000)[offset+first.size()]=0;std::copy(second.begin(),second.end(),memory.at(0x602000).begin()+offset);memory.at(0x602000)[offset+second.size()]=0;
        hooks.memory=[this](auto token) { return std::span<std::uint8_t>(memory.at(token)); };
        hooks.submit=[this,offset](auto manager,auto line) { const auto& bytes=memory.at(manager);const auto text=std::string_view(reinterpret_cast<const char*>(bytes.data()+offset));trace+="S,"+TextHex(text)+','+TextHex(line)+";";if (changes&1U) current=0x602000; };
    } // 초기 바이트와 외부 submit만 조립하며 기대 출력은 만들지 않는다.
};
}
// 현재 guard/기한/옵션/시각/본문/서식/요청순서1176행을 실제 CPU 결과와 대조한다.
TEST_CASE(MonitorTime_ReplaysNativeTimerAndCurrentCallbacks) {
    std::size_t count{};
    for (const auto& row:TimerRows()) { // 첫 불일치의 전체 필드를 남기며 모든 행을 비교한다.
        CHECK(row.size()==17);const auto actual=ReplayTimer(row);const std::vector<std::string> expected{row[10],row[11],row[12],row[13],row[14],row[16]};CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("시간 알림 %s 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),row[1].c_str(),index,expected[index].c_str(),actual[index].c_str()); } // Native 원시값을 고치지 않는다.
        if (row[0]=="originalCD") CHECK(Number(row[15])==((Number(row[2])&1U) ? 0U : 1U));else CHECK(Number(row[15])==0); // CD 무효과 정리함수 등록만 별도 판본 경계다.
        ++count;
    }
    CHECK(count==1176);
}
// 판본별60/16BYTE 절단/패딩·제출 후 현재 manager 재조회와 전체 물리12행을 대조한다.
TEST_CASE(MonitorTime_ReplaysNativeTemporaryFormatAndCurrentManager) {
    std::size_t count{};
    for (const auto& row:TemporaryRows()) { // 두 실제 원문 wrapper의 같은 입력을 독립 실행한다.
        CHECK(row.size()==8);TemporaryScene scene(row[0]=="originals",Number(row[3]));const auto pattern=Number(row[2]);const std::string format=pattern==0 ? "~9~0" : pattern==1 ? std::string(75,'Z') : std::string();
        SubmitTemporaryChatFormat(scene.current,format,"Alpha: !calendar\n\n",scene.patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,scene.hooks);
        CHECK(std::to_string(scene.current==0x602000 ? 1 : 0)==row[4] && MonitorHex(scene.memory.at(0x601000))==row[5] && MonitorHex(scene.memory.at(0x602000))==row[6] && scene.trace==row[7]);++count;
    }
    CHECK(count==12);
}
// 원문은 정상 submit 복귀에서만 복원한다. 실패 시 앞선 임시서식 쓰기를 지우지 않는다.
TEST_CASE(MonitorTime_TemporaryFailureKeepsOriginalPartialWrite) {
    TemporaryScene scene(true,0);scene.hooks.submit=[](auto,auto) { throw std::logic_error("명시 제출 장치 실패"); };bool rejected=false;
    try { SubmitTemporaryChatFormat(scene.current,std::string(75,'Z'),"x",OriginalEdition::Patch1078,scene.hooks); }catch (const std::logic_error&) { rejected=true; }
    const auto& rows=TemporaryRows();const auto native=std::find_if(rows.begin(),rows.end(),[](const auto& row) { return row[0]=="originals" && row[2]=="1" && row[3]=="1"; });CHECK(rejected && native!=rows.end());if (native!=rows.end()) CHECK(MonitorHex(scene.memory.at(0x601000))==(*native)[5]);CHECK(scene.current==0x601000);
}
// 실제 동기 Config writer가 두 현재 DWORD만 갱신하며 정지/옵션해제로 기존 기한을 지우지 않는다.
TEST_CASE(MonitorTime_ActualClientOptionsAndDeadlineLifetime) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));const auto owner=client.LocalStateGlobals();auto& state=owner->MonitorTime();state.initialized=0x80000000U;state.deadline=123;client.UpdateMonitorTime();CHECK(state.initialized==0x80000001U && std::bit_cast<std::uint64_t>(state.deadline)==0);
    client.Configuration().Set("monitor","-2147483648");client.Configuration().Set("alert","1");CHECK(state.monitor==(-2147483647-1) && state.alert==1);client.Pause(true);client.UpdateMonitorTime();const auto deadline=state.deadline;CHECK(deadline>0 && state.initialized==0x80000001U && &state==&owner->MonitorTime());
    client.Configuration().Set("monitor","0");client.Configuration().Set("alert","0");client.UpdateMonitorTime();CHECK(state.deadline==deadline);client.Configuration().Set("unrelatedMonitorTest","1");CHECK(!state.monitor && !state.alert && state.deadline==deadline);client.Pause(false);
    const auto calendar=MonitorCalendarText();CHECK(calendar.size()==25 && calendar.back()=='\n'); // 실제 CRT 달력 경계만 호출하며 GUI/통신은 실행하지 않는다.
}
