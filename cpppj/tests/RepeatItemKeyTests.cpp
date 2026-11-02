// D/d 원문452입력과 기존 최근 항목 소비자의 현재TYPE/내용물/serial 접합을 확인한다.
#include "TestSupport.h"
#include "client/ProductionShortcuts.h"
#include "client/InventoryItemGump.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client; // 실제 제품의 입력 큐와 생산항목 몸체를 사용한다.
namespace { // 원문TSV의 입력/관측을 변경 없이 해독하는 검사 전용 장치다.
std::vector<std::string> RepeatSplit(std::string_view text,char separator) { // 마지막 빈 필드도 보존한다.
    std::vector<std::string> fields;std::size_t begin{}; // 해독한 필드와 다음 검색 시작점이다.
    for (;;) { // 모든 명시 필드를 원시 순서대로 읽는다.
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return fields;begin=end+1;
    }
}
std::uint32_t RepeatNumber(std::string_view value) { return static_cast<std::uint32_t>(std::stoull(std::string(value))); } // 원문DWORD의 부호 비트를 유지한다.
std::string RepeatBits(std::int32_t value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); } // 실제 인자만 원시 관측 형식으로 바꾼다.
std::string RepeatQueueTrace(std::string_view text) { // 부모의 Guide/Gump/정지와 이 부분의 실제 예약만 구별한다.
    std::string result; // 원문queue CALL의 전체 순서다.
    for (const auto& item:RepeatSplit(text,';')) if (item.starts_with("queue:")) { if (!result.empty()) result+=';';result+=item; } // 기대 매핑을 계산하지 않고 관측한 호출만 선택한다.
    return result;
}
}

TEST_CASE(RepeatItemKey_Native452InputsAndRealQueue) { // 상위비트·뗌·정지·현재 명령·실제고리 결과를 독립PE 관측과 대조한다.
    std::ifstream input(NETSTORM_REPEAT_ITEM_KEY_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 원시 행과 실제로 대조한 입력 수다.
    while (std::getline(input,line)) { // 모든 동결 입력을 같은 제품 키/큐로 재생한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=RepeatSplit(line,'\t');CHECK(row.size()==17);if (row.size()!=17) continue;++count;
        InputQueue queue;std::string trace;const auto read=RepeatNumber(row[7]),filled=RepeatNumber(row[8]); // 관측 뒤 열은 초기화에 쓰지 않는다.
        for (std::uint32_t index=0;index<read;++index) { queue.Push(1,0,0);static_cast<void>(queue.Pop(false)); } // 실제 공개API로 두 커서를 전진한다.
        for (std::uint32_t index=0;index<filled;++index) { const auto physical=(read+index)%40U;queue.Push(0x02000300U+physical,static_cast<std::int32_t>(100+physical),static_cast<std::int32_t>(200+physical)); } // 원문 유효 슬롯 표식을 같은 순서로 채운다.
        ProductionShortcutHooks hooks;hooks.command=[]()->std::uint32_t { throw std::logic_error("최근항목이 다리슬롯명령을 조회했습니다"); };hooks.repeatCommand=[&] { return RepeatNumber(row[6]); };
        hooks.queue=[&](auto command,auto first,auto second) { if (!trace.empty()) trace+=';';trace+="queue:"+std::to_string(command)+':'+RepeatBits(first)+':'+RepeatBits(second);queue.PushCommand(command,first,second); };
        for (const auto& text:RepeatSplit(row[1],';')) { // 연속 사건 사이에 현재 큐/명령을 다시 만들지 않는다.
            const auto fields=RepeatSplit(text,':');CHECK(fields.size()==3);if (fields.size()!=3) continue;
            if (RepeatNumber(row[2]) || RepeatNumber(row[3]) || RepeatNumber(row[5])) continue; // 선행Guide/Gump/Console 소비는 부모의 별도 관문이다.
            static_cast<void>(QueueProductionShortcut({RepeatNumber(fields[0]),std::bit_cast<std::int32_t>(RepeatNumber(fields[1])),std::bit_cast<std::int32_t>(RepeatNumber(fields[2]))},std::bit_cast<std::int32_t>(RepeatNumber(row[4]))>0,hooks));
        }
        std::string active; // 비공개 물리슬롯 대신 같은 공개Pop의 활성FIFO를 관측한다.
        while (!queue.Empty()) { const auto event=queue.Pop(false);if (!active.empty()) active+=';';active+=std::to_string(event.code)+':'+RepeatBits(event.x)+':'+RepeatBits(event.y); } // 실제 예약 순서대로 읽는다.
        CHECK(trace==RepeatQueueTrace(row[14]));CHECK((active.empty() ? "-" : active)==row[12]);CHECK(row[9]=="1" && RepeatNumber(row[16])>0);
        for (const auto& delta:RepeatSplit(row[15],';')) CHECK(delta=="0"); // 원문 모든 사건의 스택 균형도 확인한다.
    }
    CHECK(count==452);
}

TEST_CASE(RepeatItemKey_ReleasePauseMaskAndLazyDevices) { // 정지/뗌/다른문자는 최근명령과 일반큐를 요구하지 않는다.
    CHECK(!QueueProductionShortcut({'D',0,0},true,{}));CHECK(!QueueProductionShortcut({'d'|InputCode::kRelease,0,0},false,{}));
    for (const auto code:{0x02000044U,0x00440000U,static_cast<std::uint32_t>('F'),static_cast<std::uint32_t>('f')}) CHECK(!QueueProductionShortcut({code,37,-19},false,{})); // 일반명령과 가상D를 글자D로 축소하지 않는다.
    std::uint32_t reads{},writes{};ProductionShortcutHooks hooks;hooks.repeatCommand=[&] { ++reads;return kProductionRepeatCommand; };hooks.queue=[&](auto command,auto first,auto second) { ++writes;CHECK(command==0x02000107U && !first && !second); };
    CHECK(QueueProductionShortcut({'D'|0x7c000000U,37,-19},false,hooks));CHECK(reads==1 && writes==1); // 모든수정키/outside/pressed에 별도거부를 추가하지 않는다.
}

TEST_CASE(RepeatItemKey_CurrentCommandAndPartialFailure) { // 매D/d가 현재 명령을 다시 읽고 실패 앞선조회 효과는 보존한다.
    std::uint32_t command=0x02000107U,reads{},writes{};ProductionShortcutHooks hooks;hooks.repeatCommand=[&] { ++reads;return command; };hooks.queue=[&](auto value,auto first,auto second) { CHECK(value==command && !first && !second);++writes;++command; };
    CHECK(QueueProductionShortcut({'D',0,0},false,hooks));CHECK(QueueProductionShortcut({'d',0,0},false,hooks));CHECK(reads==2 && writes==2 && command==0x02000109U);
    hooks.queue={};bool failed{};try { static_cast<void>(QueueProductionShortcut({'D',0,0},false,hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==3 && writes==2);
    hooks.repeatCommand={};failed=false;try { static_cast<void>(QueueProductionShortcut({'d',0,0},false,hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==3);
}

TEST_CASE(RepeatItemKey_ExistingItemConsumesCurrentTypeContentAndSerial) { // 키예약 뒤 현재일치TYPE의 기존항목이같은Choose/내용물/serial을 전달한다.
    netstorm::o::SidPool pool(netstorm::o::OriginalEdition::Patch1078,24000,true);const auto first=pool.Allocate(),second=pool.Allocate();auto raw=pool.AllocatedBytes(first);raw[18]=128;raw[19]=0;raw=pool.AllocatedBytes(second);raw[18]=129;raw[19]=0;
    InventoryItemNode node;node.content=first.value;node.serial=41;std::uint32_t owner{1},stock{},disabled{},last{},bridge{82},background{},frame{},knowledge{},forwards{};bool editor{};float cellX{16},cellY{11}; // 같은 실제 소비자의 현재 참조 전역이다.
    std::array<std::uint32_t,3> selected{};InventoryItemHooks itemHooks;itemHooks.node=[&](auto token) { CHECK(token==1);return &node; };itemHooks.local=[](auto,auto,auto,auto) { return std::array<float,2>{0,0}; };itemHooks.knowledgeOpen=[&] { return knowledge; };
    itemHooks.forward=[&](auto token,auto command,auto content,auto serial) { CHECK(token==1);++forwards;selected={command,content,serial};return 0x80000009U; };InventoryItemGump item(pool,{owner,editor,stock,disabled,last,bridge,cellX,cellY,background,frame},itemHooks);
    InputQueue queue;ProductionShortcutHooks keys;keys.repeatCommand=[] { return kProductionRepeatCommand; };keys.queue=[&](auto command,auto a,auto b) { queue.PushCommand(command,a,b); };
    const auto consume=[&] { const auto event=queue.Pop(false);return item.Handle(1,event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y)); }; // 같은 예약사건을 기존항목몸체에만 전달한다.
    CHECK(QueueProductionShortcut({'D',37,-19},false,keys));CHECK(!forwards);last=128;CHECK(consume()==0x80000009U);CHECK((selected==std::array<std::uint32_t,3>{0x02000008U,first.value,41}));
    CHECK(QueueProductionShortcut({'d',0,0},false,keys));last=129;CHECK(consume()==0 && forwards==1); // 현재 마지막TYPE가 달라지면 이전 항목은 취득하지 않는다.
    CHECK(QueueProductionShortcut({'D',0,0},false,keys));node.content=second.value;node.serial=43;CHECK(consume()==0x80000009U);CHECK((selected==std::array<std::uint32_t,3>{0x02000008U,second.value,43}));
    disabled=1;CHECK(QueueProductionShortcut({'D',0,0},false,keys));CHECK(consume()==0 && forwards==2);disabled=0;knowledge=1;CHECK(QueueProductionShortcut({'d',0,0},false,keys));CHECK(consume()==0 && forwards==2 && queue.Empty()); // 기존 항목의현재 입력/지식관문을 우회하지 않는다.
}
