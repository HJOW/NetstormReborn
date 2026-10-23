// 실제 PE의 현재 사건 처리 관측 중 일반 키 구간에 도달한 행만 제품과 대조한다.
#include "TestSupport.h"
#include "client/ProductionShortcuts.h"
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::size_t kColumns=17,kNativeRows=8040,kComparableRows=4280; // 전체 원문 관측과 실제 키 구간 도달 행을 구별한다.
constexpr std::size_t kRingSlots=40; // 독립 원문의 명시 입력 고리 크기다.
using ShortcutRow=std::array<std::string,kColumns>; // 앞9개 입력과 뒤8개 관측을 보존하는 행이다.

// 마지막 빈 필드도 보존하며 TSV/사건/경계의 명시 구분자만 나눈다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> parts;std::size_t begin{};
    for (;;) { // 각 실제 필드의 바이트를 변경 없이 보존한다.
        const auto end=text.find(separator,begin);parts.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return parts;begin=end+1;
    }
}
// 원문 입력/관측의 전체 unsigned DWORD를 호스트 부호 변환 없이 읽는다.
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); }
// 실제 장치 경계가 받은 signed 인자를 동일 DWORD의 십진수로 기록한다.
std::string Bits(std::int32_t value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); }
// 원문 명시 사건열의 세 DWORD를 제품의 동일 InputEvent로 옮긴다.
std::vector<InputEvent> Events(std::string_view text) {
    std::vector<InputEvent> events;
    for (const auto& item:Split(text,';')) { // 입력 순서를 유지하며 큐를 사건 사이에 비우지 않는다.
        const auto fields=Split(item,':');if (fields.size()!=3) throw std::invalid_argument("생산 단축키 사건 열 오류");
        events.push_back({Number(fields[0]),std::bit_cast<std::int32_t>(Number(fields[1])),std::bit_cast<std::int32_t>(Number(fields[2]))});
    }
    return events;
}
// 각 Guide 진입을 사건 경계로 사용해 실제 queue CALL 인자와 사건별 횟수를 읽는다.
struct NativeAttempts {
    std::string trace; // CPU가 실제 push한 명령/슬롯/셋째값의 전체 순서다.
    std::vector<std::size_t> counts; // 입력 사건마다 실제452fe0에 진입한 횟수다.
    std::size_t pausedCalls{}; // 실제 일반 키 구간에 도달했는지를 보여 주는 경계 횟수다.
    // 원문 관측에서 사건 경계와 실제 예약만 읽으며 입력 키를 해석하지 않는다.
    explicit NativeAttempts(std::string_view text) {
        for (const auto& item:Split(text,';')) { // 이미 관측한 경계만 선택하며 키 매핑을 다시 계산하지 않는다.
            if (item.starts_with("guide:")) counts.push_back(0);
            else if (item.starts_with("paused:")) ++pausedCalls;
            else if (item.starts_with("queue:")) {
                if (counts.empty()) throw std::logic_error("Guide 사건 없는 원문 예약");++counts.back();
                if (!trace.empty()) trace+=';';trace+=item;
            }
        }
    }
};
// 실제 InputQueue의 공개 API로 독립 입력의 활성 고리와 현재 명령 장치를 준비한다.
struct ShortcutScene {
    InputQueue queue; // 실제40칸 큐의 FIFO/포화 정책을 그대로 사용한다.
    std::uint32_t command{}; // 원문이 매 예약 직전에 조회한 현재 명령 입력이다.
    std::string trace; // helper가 요청한 예약 인자이며 원문 CALL trace와 직접 대조한다.
    std::size_t commandReads{}; // 선택한 예약마다 현재 명령을 다시 조회하는지 확인한다.
    // 동일한 입력 고리 위치/활성 표식만 준비하며 새 예약은 실제 제품 함수에 맡긴다.
    explicit ShortcutScene(const ShortcutRow& row):command(Number(row[6])) {
        const auto read=Number(row[7]),filled=Number(row[8]);if (read>=kRingSlots || filled>=kRingSlots) throw std::out_of_range("원문 입력 고리 범위");
        for (std::uint32_t index=0;index<read;++index) { // 빈 큐의 두 커서를 공개Push/Pop만으로 독립 입력 위치까지 전진한다.
            queue.Push(0x2001000U+index,0,0);static_cast<void>(queue.Pop(false));
        }
        for (std::uint32_t index=0;index<filled;++index) { // oracle이 명시한 기존 활성 슬롯의 입력 표식을 그대로 공급한다.
            const auto physical=static_cast<std::uint32_t>((read+index)%kRingSlots);
            queue.Push(0x2000300U+physical,static_cast<std::int32_t>(100U+physical),static_cast<std::int32_t>(200U+physical));
        }
    }
    // 실제452fe0 대응 API만 호출하며 기대 슬롯이나 성공 결과를 대신 만들지 않는다.
    ProductionShortcutHooks Hooks() {
        return {[this] { ++commandReads;return command; },[this](std::uint32_t code,std::int32_t first,std::int32_t second) {
            if (!trace.empty()) trace+=';';trace+="queue:"+std::to_string(code)+':'+Bits(first)+':'+Bits(second);
            queue.PushCommand(code,first,second);
        }};
    }
    // 실제 두 커서 사이의 활성 사건만 공개Pop으로 관측한다. 비공개 물리 슬롯 동일성은 주장하지 않는다.
    std::string ActiveQueue() {
        std::string text;std::size_t count{};
        while (!queue.Empty()) { // 최대39개의 실제 활성 사건을 순서대로 꺼낸다.
            if (++count>=kRingSlots) throw std::logic_error("생산 단축키 활성 큐 상한 오류");const auto event=queue.Pop(false);
            if (!text.empty()) text+=';';text+=std::to_string(event.code)+':'+Bits(event.x)+':'+Bits(event.y);
        }
        return text.empty() ? "-" : text;
    }
};
}

// 전체8040 중 Guide/Gump/release/콘솔에서 끝나지 않은4280행만 같은 실제 helper/큐에 대조한다.
TEST_CASE(ProductionShortcuts_Native4280ReachableCases) {
    std::ifstream input(NETSTORM_PRODUCTION_SHORTCUTS_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t nativeRows{},compared{},gateRows{};
    while (std::getline(input,line)) { // 각 동결 행을 실제 본체의 도달 범위에 따라 검사한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;
        const auto fields=Split(line,'\t');CHECK(fields.size()==kColumns);if (fields.size()!=kColumns) continue;
        ShortcutRow row;for (std::size_t index=0;index<row.size();++index) row[index]=fields[index]; // 관측을 수정하지 않고 고정 열에 옮긴다.
        ++nativeRows;CHECK(row[9]=="1");CHECK(Number(row[16])>0);CHECK(row[13].size()==kRingSlots*12U*2U);
        const auto events=Events(row[1]);NativeAttempts expected(row[14]);CHECK(expected.counts.size()==events.size());
        bool reachable=Number(row[2])==0 && Number(row[3])==0 && Number(row[5])==0;
        for (const auto& event:events) { // 원본은release를 Guide/Gump 뒤, 일반 키 구간 앞에서 거부한다.
            if (event.code&InputCode::kRelease) reachable=false;
        }
        CHECK(reachable==(expected.pausedCalls==events.size()));
        if (!reachable) { ++gateRows;continue; } // 앞선 native 관문은 이 helper의 제품 대조로 계산하지 않는다.
        ++compared;ShortcutScene scene(row);const auto hooks=scene.Hooks();
        const bool paused=std::bit_cast<std::int32_t>(Number(row[4]))>0; // 실제460de0의 signed 깊이>0이다.
        for (std::size_t index=0;index<events.size();++index) { // 같은 큐를 유지하며 원문과 같은 사건 순서로 예약한다.
            const bool handled=QueueProductionShortcut(events[index],paused,hooks);CHECK(handled==(expected.counts[index]!=0));
        }
        CHECK(scene.trace==expected.trace);
        std::size_t attempted{};for (const auto count:expected.counts) attempted+=count; // 실제 관측한 예약 수와 현재 명령 조회 수를 대조한다.
        CHECK(scene.commandReads==attempted);CHECK(scene.ActiveQueue()==row[12]);
    }
    CHECK(nativeRows==kNativeRows);CHECK(compared==kComparableRows);CHECK(gateRows==kNativeRows-kComparableRows);
}

// 키 구간 앞의 뗌/정지 관문과 비선택 문자가 실제 예약 장치를 요구하지 않는지 확인한다.
TEST_CASE(ProductionShortcuts_ReleasePauseAndLazyDevices) {
    const ProductionShortcutHooks missing; // 관문에서 끝나는 입력에는 조회/예약 함수를 설치하지 않는다.
    for (const auto key:std::string_view("qWaSzXyE")) { // 여섯 슬롯과 대체/전체 예약 키를 각각 검사한다.
        CHECK(!QueueProductionShortcut({static_cast<std::uint32_t>(key)|InputCode::kRelease,0,0},false,missing));
        CHECK(!QueueProductionShortcut({static_cast<std::uint32_t>(key),0,0},true,missing));
    }
    CHECK(!QueueProductionShortcut({'?',0,0},false,missing));
    std::uint32_t command=0x020000f4U;std::size_t reads{},calls{};std::vector<std::uint32_t> observed; // 각 예약 직전 현재 명령과 중단 전 부분 효과다.
    ProductionShortcutHooks partial{[&] { ++reads;return command; },[&](auto code,auto slot,auto zero) { CHECK(slot==static_cast<std::int32_t>(calls) && zero==0);++calls;if (calls==3) throw std::runtime_error("명시 세 번째 큐 경계 중단");observed.push_back(code);++command; }};
    bool stopped{};try { QueueProductionShortcut({'E',0,0},false,partial); } catch (const std::runtime_error&) { stopped=true; }
    CHECK(stopped && reads==3 && calls==3 && observed==std::vector<std::uint32_t>({0x020000f4U,0x020000f5U}));
}
