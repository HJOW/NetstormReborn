// Pause 키의 원문246입력과 실제 일반 FIFO/기존 Pause 몸체의 지연 소비를 확인한다.
#include "TestSupport.h"
#include "client/PauseKeyInput.h"
#include "client/GameMenuActions.h"
#include "client/GameMenuCommands.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client; // 제품과 같은 입력 큐/일반 메뉴 동작을 사용한다.
namespace { // fixture의 원시 문자열만 해석하고 기대 분기/예약 결과는 계산하지 않는다.
std::vector<std::string> PauseSplit(std::string_view text,char separator) { // 마지막 빈 필드와 원래 순서를 보존한다.
    std::vector<std::string> fields;std::size_t begin{}; // 현재 필드와 다음 구분자 검색 위치다.
    for (;;) { // 모든 필드를 입력 순서대로 분리한다.
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return fields;begin=end+1;
    }
}
std::uint32_t PauseNumber(std::string_view value) { return static_cast<std::uint32_t>(std::stoull(std::string(value))); } // 원문DWORD의 전체 비트를 읽는다.
std::string PauseBits(std::int32_t value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); } // 실제 인자를 관측 문자열로만 바꾼다.
std::string PauseQueueTrace(std::string_view text) { // Guide/Gump/정지/공통 꼬리와 이 부분의 예약 효과를 구별한다.
    std::string result; // 실제queue CALL만 순서대로 보관한다.
    for (const auto& item:PauseSplit(text,';')) if (item.starts_with("queue:")) { if (!result.empty()) result+=';';result+=item; } // 원문이 관측한 호출만 선택한다.
    return result;
}
}

TEST_CASE(PauseKeyInput_Native246InputsAndRealQueue) { // 독립 PE가 결정한 상위비트/뗌/정지/선행 소비와 현재 명령/포화 결과다.
    std::ifstream input(NETSTORM_PAUSE_KEY_INPUT_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 원시 행과 실제 대조 수다.
    while (std::getline(input,line)) { // 모든 동결 입력을 같은 공개 제품 함수/큐로 재생한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=PauseSplit(line,'\t');CHECK(row.size()==17);if (row.size()!=17) continue;++count;
        InputQueue queue;std::string trace;const auto read=PauseNumber(row[7]),filled=PauseNumber(row[8]); // 관측 뒤 열을 초기화에 쓰지 않는다.
        for (std::uint32_t index=0;index<read;++index) { queue.Push(1,0,0);static_cast<void>(queue.Pop(false)); } // 실제 두 커서를 공개API로만 전진한다.
        for (std::uint32_t index=0;index<filled;++index) { // 원문 입력의 현재 유효 슬롯 표식을 같은 순서로 채운다.
            const auto physical=(read+index)%40U;queue.Push(0x02000300U+physical,static_cast<std::int32_t>(100+physical),static_cast<std::int32_t>(200+physical));
        }
        PauseKeyInputHooks hooks;hooks.command=[&] { return PauseNumber(row[6]); }; // 현재 명령은 원문 입력 열에서만 공급한다.
        hooks.queue=[&](auto command,auto first,auto second) { if (!trace.empty()) trace+=';';trace+="queue:"+std::to_string(command)+':'+PauseBits(first)+':'+PauseBits(second);queue.PushCommand(command,first,second); };
        for (const auto& text:PauseSplit(row[1],';')) { // 연속 사건 사이에 실제 고리나 명령을 초기화하지 않는다.
            const auto fields=PauseSplit(text,':');CHECK(fields.size()==3);if (fields.size()!=3) continue;
            if (PauseNumber(row[2]) || PauseNumber(row[3]) || PauseNumber(row[5])) continue; // 선행 Guide/Gump/Console 소비는 부모의 별도 관문이며 이 부분에 전달하지 않는다.
            static_cast<void>(DispatchPauseKeyInput({PauseNumber(fields[0]),std::bit_cast<std::int32_t>(PauseNumber(fields[1])),std::bit_cast<std::int32_t>(PauseNumber(fields[2]))},hooks));
        }
        std::string active; // 비공개 물리슬롯 대신 같은 공개Pop의 활성 FIFO를 읽는다.
        while (!queue.Empty()) { const auto event=queue.Pop(false);if (!active.empty()) active+=';';active+=std::to_string(event.code)+':'+PauseBits(event.x)+':'+PauseBits(event.y); } // 실제 예약 순서를 그대로 관측한다.
        CHECK(trace==PauseQueueTrace(row[14]));CHECK((active.empty() ? "-" : active)==row[12]);CHECK(row[9]=="1" && PauseNumber(row[16])>0);
        for (const auto& delta:PauseSplit(row[15],';')) CHECK(delta=="0"); // 원문 모든 사건의 스택 균형도 확인한다.
    }
    CHECK(count==246);
}

TEST_CASE(PauseKeyInput_NonPauseAndReleaseDoNotRequireDevices) { // 뗌·글자19·다른 가상키·일반명령은 현재 명령을 조회하지 않는다.
    for (const auto code:{0x80130000U,0xb8130000U,0x13U,0x02130000U,0x120000U,0x140000U}) CHECK(!DispatchPauseKeyInput({code,37,-19},{})); // 잘못된 사건을 키 누름으로 축소하지 않는다.
    std::uint32_t reads{},writes{};PauseKeyInputHooks hooks;hooks.command=[&] { ++reads;return 0x020000cfU; };hooks.queue=[&](auto code,auto first,auto second) { ++writes;CHECK(code==0x020000cfU && !first && !second); };
    CHECK(DispatchPauseKeyInput({0x7c130000U,37,-19},hooks));CHECK(reads==1 && writes==1); // outside/모든수정키/pressed는 원문에서 별도 거부하지 않는다.
}

TEST_CASE(PauseKeyInput_MissingQueuePreservesCurrentCommandLookup) { // 실제 선택된 누락 장치는 빈 성공 대신 진단하며 앞선 조회 효과는 유지한다.
    bool failed{};try { static_cast<void>(DispatchPauseKeyInput({0x130000U,0,0},{})); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    std::uint32_t reads{};PauseKeyInputHooks hooks;hooks.command=[&] { ++reads;return 0x020000cfU; };failed=false;
    try { static_cast<void>(DispatchPauseKeyInput({0x130000U,0,0},hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==1);
}

TEST_CASE(PauseKeyInput_QueuedCommandUsesCurrentPauseAndExistingActions) { // Pause 키는 시계/음향을 즉시 바꾸지 않고 같은 일반명령의 소비 시점 상태를 따른다.
    InputQueue queue;bool paused{};std::uint32_t hide{},redraw{};std::string trace; // 현재 상태와 같은 일반 큐다.
    PauseKeyInputHooks keys;keys.command=[] { return GameMenuCommandValue(0x542a30).value(); };keys.queue=[&](auto code,auto first,auto second) { queue.PushCommand(code,first,second); };
    GameMenuActionHooks effects;effects.command=[](auto command) { return GameMenuCommandValue(static_cast<std::uint32_t>(command)).value(); };
    effects.paused=[&] { trace+='P';return paused; };effects.status=[&](auto kind,auto owner,auto enabled,auto parameter,auto text) { CHECK(kind==11 && !owner && !parameter && text.empty() && enabled==!paused);trace+='S'; };
    effects.cancelCursor=[&] { trace+='C'; };effects.pauseClock=[&] { paused=true;trace+='T'; };effects.muteSound=[&] { trace+='M'; };effects.resumeClock=[&] { paused=false;trace+='R'; };effects.resumeSound=[&] { trace+='U'; };
    GameMenuActions actions({[](auto address) { return address==0x594fa4U ? 1U : 0U; },hide,redraw},effects);
    CHECK(DispatchPauseKeyInput({0x130000U,37,-19},keys));CHECK(!paused && trace.empty());paused=true; // 예약 뒤 다른 원문 효과가 현재 정지를 바꾼 입력이다.
    auto event=queue.Pop(false);CHECK(actions.Handle(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y))==0U);CHECK(!paused && trace=="PSRU");
    trace.clear();CHECK(DispatchPauseKeyInput({0x38130000U,41,-23},keys));CHECK(!paused && trace.empty());event=queue.Pop(false);
    CHECK(actions.Handle(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y))==0U);CHECK(paused && trace=="PSCTM" && queue.Empty());
}
