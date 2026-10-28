// 독립 PE310입력의 실제 F2/F3 관측과 같은 제품 FIFO/외부 장치 호출을 대조한다.
#include "TestSupport.h"
#include "client/ViewKeyInput.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { // 관측 TSV를 입력/출력 계산 없이 읽는 검사 전용 장치다.
constexpr std::size_t kColumns=23,kRows=310; // 새 독립 관측의 물리 열/입력 수다.
std::vector<std::string> Split(std::string_view text,char separator) { // 마지막 빈 필드도 그대로 보존한다.
    std::vector<std::string> fields;std::size_t begin{}; // 원시 필드와 현재 시작 위치다.
    for (;;) { // 원문 순서의 구분자만 해석한다.
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return fields;begin=end+1;
    }
}
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); } // signed 입력도 원래 DWORD로 제공한다.
std::string Bits(std::int32_t value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); } // 실제 인자의 비트를 십진 관측으로 바꾼다.
std::string SelectedTrace(std::string_view text) { // 외부 Guide/Gump/공통 tail은 이 부분 본체의 계약과 구별한다.
    std::string output; // 실제 ReadInt/Tell/정지/큐 순서만 보존한다.
    for (const auto& item:Split(text,';')) { // 관측 문자열의 선택된 CALL만 복사한다.
        if (!item.starts_with("readInt:") && !item.starts_with("tell:") && !item.starts_with("paused:") && !item.starts_with("queue:")) continue;
        if (!output.empty()) output+=';';output+=item;
    }
    return output;
}
}

TEST_CASE(ViewKeyInput_Native310InputsAndRealQueue) { // 관측 뒤 열을 입력 초기화나 키 분기 선택에 사용하지 않는다.
    std::ifstream input(NETSTORM_VIEW_KEY_INPUT_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 읽은 원문 행과 실제 대조 수다.
    while (std::getline(input,line)) { // 모든 동결 입력을 같은 실제 제품 함수로 대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=Split(line,'\t');CHECK(row.size()==kColumns);if (row.size()!=kColumns) continue;++count;
        auto hide=Number(row[9]),redraw=Number(row[10]);InputQueue queue;std::string trace; // 입력 전역과 실제40칸 고리다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 제품이 호출한 실제 경계만 기록한다.
        const auto read=Number(row[7]),filled=Number(row[8]); // oracle이 제공한 시작 커서와 활성 수다.
        for (std::uint32_t index=0;index<read;++index) { queue.Push(1,0,0);static_cast<void>(queue.Pop(false)); } // 공개 API만으로 빈 큐의 두 커서를 전진한다.
        for (std::uint32_t index=0;index<filled;++index) { // 원시 입력의 현재 활성 슬롯 표식을 그대로 공급한다.
            const auto physical=(read+index)%40U;queue.Push(0x02000300U+physical,static_cast<std::int32_t>(100+physical),static_cast<std::int32_t>(200+physical));
        }
        ViewKeyInputHooks hooks; // 값은 입력 앞13열에서만 제공한다.
        hooks.paused=[&] { append("paused:"+row[4]);return std::bit_cast<std::int32_t>(Number(row[4]))>0; };
        hooks.readInt=[&](auto key,auto& value) { append("readInt:"+std::string(key)+':'+Bits(value));if (Number(row[11])) value=std::bit_cast<std::int32_t>(Number(row[12]));return Number(row[11])!=0; };
        hooks.tell=[&](auto section) { append("tell:"+std::string(section)); };
        hooks.queue=[&](auto command,auto first,auto second) { append("queue:"+std::to_string(command)+':'+Bits(first)+':'+Bits(second));queue.PushCommand(command,first,second); };
        for (const auto& text:Split(row[1],';')) { // 사건열 사이에 실제 큐/표시 전역을 초기화하지 않는다.
            const auto fields=Split(text,':');CHECK(fields.size()==3);if (fields.size()!=3) continue;
            if (Number(row[2]) || Number(row[3]) || Number(row[5])) continue; // 선행 Guide/Gump/console 소비는 원문 별도 관문이며 이 함수에 전달하지 않는다.
            static_cast<void>(DispatchViewKeyInput({Number(fields[0]),std::bit_cast<std::int32_t>(Number(fields[1])),std::bit_cast<std::int32_t>(Number(fields[2]))},{hide,redraw},hooks));
        }
        std::string active; // 비공개 물리 슬롯 대신 실제 공개Pop의 활성 FIFO만 관측한다.
        while (!queue.Empty()) { const auto event=queue.Pop(false);if (!active.empty()) active+=';';active+=std::to_string(event.code)+':'+Bits(event.x)+':'+Bits(event.y); } // 실제 예약된 순서 그대로 읽는다.
        CHECK(hide==Number(row[21]));CHECK(redraw==Number(row[22]));CHECK(trace==SelectedTrace(row[18]));CHECK((active.empty() ? "-" : active)==row[16]);
        CHECK(row[13]=="1" && Number(row[20])>0); // 원문이 실제 종료한 관측인지도 확인한다.
    }
    CHECK(count==kRows);
}

TEST_CASE(ViewKeyInput_LazyDevicesAndTellBeforePause) { // 미선택 장치와 Tell 뒤 현재 정지 조회 순서를 검증한다.
    std::uint32_t hide{},redraw=9;bool paused{};std::string order; // 같은 장치가 현재 상태를 바꿀 수 있는 입력이다.
    CHECK(!DispatchViewKeyInput({0x80710000U,0,0},{hide,redraw},{}));CHECK(!DispatchViewKeyInput({'Q',0,0},{hide,redraw},{}));
    ViewKeyInputHooks hooks; // F2에는 설정/Tell/큐 장치를 설치하지 않는다.
    hooks.paused=[&] { order+='P';return paused; };
    CHECK(DispatchViewKeyInput({0x710000U,0,0},{hide,redraw},hooks));CHECK(hide==1 && redraw==1 && order=="P");
    order.clear();hooks.readInt=[&](auto,auto&) { order+='R';return false; };hooks.tell=[&](auto name) { CHECK(name=="SetIslands(1)");order+='T';paused=true; };
    CHECK(DispatchViewKeyInput({0x20720000U,0,0},{hide,redraw},hooks));CHECK(order=="RTP" && paused);
}
