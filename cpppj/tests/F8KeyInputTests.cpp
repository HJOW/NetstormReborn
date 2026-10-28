// 일반/Editor F8의 독립558입력과 같은 FIFO·현재 모드 읽기·Tell 순서를 대조한다.
#include "TestSupport.h"
#include "client/ViewKeyInput.h"
#include <bit>
#include <fstream>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { // 이 파일의 장치는 앞13입력 열만 사용하며 관측으로 분기를 선택하지 않는다.
constexpr std::size_t kRows=558,kColumns=21; // 독립 입력 수와 물리 TSV 열 수다.
std::vector<std::string> Split(std::string_view text,char separator) { // 빈 마지막 필드도 보존하는 입력 분리기다.
    std::vector<std::string> output;std::size_t begin{}; // 읽은 필드와 현재 시작 위치다.
    for (;;) { // 원시 구분자 순서만 사용한다.
        const auto end=text.find(separator,begin);output.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return output;begin=end+1;
    }
}
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); } // 원문 DWORD의 전체 비트다.
std::string Bits(std::int32_t value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); } // 실제 signed 인자를 원시 DWORD 관측으로 표기한다.
std::string SelectedTrace(std::string_view text) { // Guide/Gump와 공통 tail은 이 부분 본체 계약에서 구별한다.
    std::string output; // 제품이 담당하는 직접 읽기/명시 효과의 순서다.
    for (const auto& item:Split(text,';')) { // 실제 CPU 관측에서 선택한 경계만 복사한다.
        if (!item.starts_with("fort:") && !item.starts_with("metamap:") && !item.starts_with("editor:") && !item.starts_with("highlight:") && !item.starts_with("readInt:") && !item.starts_with("tell:") && !item.starts_with("queue:") && !item.starts_with("paused:")) continue;
        if (!output.empty()) output+=';';output+=item;
    }
    return output;
}
}

TEST_CASE(F8KeyInput_Native558InputsAndCurrentModeReads) { // 최초 원본 관측의 상태/호출을 실제 제품 함수와 비교한다.
    std::ifstream input(NETSTORM_F8_KEY_INPUT_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 현재 행과 비교한 독립 입력 수다.
    while (std::getline(input,line)) { // 관측 열은 실행이 끝난 뒤에만 대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=Split(line,'\t');CHECK(row.size()==kColumns);if (row.size()!=kColumns) continue;++count;
        CHECK(row[9]=="0");InputQueue queue;std::uint32_t hide=9,redraw=17;std::string trace; // metamap 새 실행 없이 같은 고리/미변경 전역을 제공한다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 실제 제품 호출만 기록한다.
        const auto read=Number(row[6]),filled=Number(row[7]); // 원문이 받은 시작 커서와 활성 수다.
        for (std::uint32_t index=0;index<read;++index) { queue.Push(1,0,0);static_cast<void>(queue.Pop(false)); } // 공개 API로 빈 고리를 전진한다.
        for (std::uint32_t index=0;index<filled;++index) { // 원시 입력의 기존 활성 슬롯을 그대로 공급한다.
            const auto physical=(read+index)%40U;queue.Push(0x02000300U+physical,static_cast<std::int32_t>(100+physical),static_cast<std::int32_t>(200+physical));
        }
        ViewKeyInputHooks hooks; // 선택되지 않은 다른 키 장치는 설치하지 않는다.
        hooks.fort=[&] { append("fort:"+row[8]);return Number(row[8]); };
        hooks.metamap=[&] { append("metamap:"+row[9]);return Number(row[9]); };
        hooks.editor=[&] { append("editor:"+row[10]);return Number(row[10]); };
        hooks.highlight=[&](auto mode) { append("highlight:"+std::to_string(mode)); };
        hooks.readInt=[&](auto key,auto& value) { append("readInt:"+std::string(key)+':'+Bits(value));if (Number(row[11])) value=std::bit_cast<std::int32_t>(Number(row[12]));return Number(row[11])!=0; };
        hooks.tell=[&](auto section) { append("tell:"+std::string(section)); };
        hooks.queue=[&](auto command,auto first,auto second) { append("queue:"+std::to_string(command)+':'+Bits(first)+':'+Bits(second));queue.PushCommand(command,first,second); };
        hooks.paused=[&] { append("paused:"+row[4]);return std::bit_cast<std::int32_t>(Number(row[4]))>0; };
        for (const auto& item:Split(row[1],';')) { // 선행 장치 소비는 해당 입력 열로만 판단한다.
            const auto event=Split(item,':');CHECK(event.size()==3);if (event.size()!=3 || Number(row[2]) || Number(row[3]) || Number(row[5])) continue;
            static_cast<void>(DispatchViewKeyInput({Number(event[0]),std::bit_cast<std::int32_t>(Number(event[1])),std::bit_cast<std::int32_t>(Number(event[2]))},{hide,redraw},hooks));
        }
        std::string active; // 실제 활성 FIFO만 공개Pop으로 읽으며 비공개 물리 슬롯 동일성을 주장하지 않는다.
        while (!queue.Empty()) { const auto event=queue.Pop(false);if (!active.empty()) active+=';';active+=std::to_string(event.code)+':'+Bits(event.x)+':'+Bits(event.y); } // 같은 실제 예약 순서다.
        CHECK(trace==SelectedTrace(row[18]));CHECK((active.empty() ? "-" : active)==row[16]);CHECK(hide==9 && redraw==17);CHECK(row[13]=="1" && Number(row[20])>0);
    }
    CHECK(count==kRows);
}

TEST_CASE(F8KeyInput_LazyDevicesAndLateEditorRead) { // 뗌과 Shift에는 모드 getter를 요구하지 않고 Editor는 예약 뒤 현재 값을 읽는다.
    std::uint32_t hide{},redraw{},editor{};std::string trace; // 동기 외부 효과의 현재 관측이다.
    CHECK(!DispatchViewKeyInput({0x80770000U,0,0},{hide,redraw},{}));
    ViewKeyInputHooks hooks;hooks.fort=[&] { trace+='F';return 1U; }; // Fort1이면 metamap 장치가 없어도 된다.
    hooks.queue=[&](auto code,auto first,auto second) { CHECK(code==0x02000091U && !first && !second);trace+='Q';editor=7; };
    hooks.editor=[&] { trace+='E';return editor; };hooks.highlight=[&](auto mode) { CHECK(mode==1);trace+='H'; };
    hooks.paused=[&] { trace+='P';return true; };
    CHECK(DispatchViewKeyInput({0x770000U,0,0},{hide,redraw},hooks));CHECK(trace=="FQEHP");
    trace.clear();hooks.fort={};hooks.editor={};hooks.queue={};hooks.readInt=[&](auto key,auto& value) { CHECK(key=="theammode" && value==0);trace+='R';return false; };hooks.tell=[&](auto name) { CHECK(name=="SetIslands(1)");trace+='T'; };
    CHECK(DispatchViewKeyInput({0x20770000U,0,0},{hide,redraw},hooks));CHECK(trace=="RTP");
}
