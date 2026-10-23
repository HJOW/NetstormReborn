// 독립 PE 관측과 실제 입력 큐로 편집 맵의 pending/next 예약 및 전투 상태 보존을 대조한다.
#include "TestSupport.h"
#include "client/FortSelection.h"
#include "o/RawLocalMissionState.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace netstorm::client;
namespace {
// TSV 마지막 빈 필드도 보존한다. trace 내부 세미콜론은 구분하지 않는다.
std::vector<std::string> Fields(std::string_view source) {
    std::vector<std::string> fields;std::size_t start=0;
    for (;;) { // 각 탭 경계의 원문 필드만 잘라낸다.
        const auto end=source.find('\t',start);fields.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));
        if (end==source.npos) return fields;start=end+1;
    }
}
// 원본과 제품의 전체 State 바이트를 같은 작은 hex 형식으로 비교한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; // 원문 fixture의 소문자 hex 표다.
    std::string result;
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; } // 앞뒤4비트를 차례로 기록한다.
    return result;
}
// 검사에서만 공급하는 외부 장치와 그 현재 전역이다. 제품 부모의 최종값을 장치에서 계산하지 않는다.
struct Devices {
    std::uint32_t mode{},battle{},state{},slot{},owner{}; // 선택 부모가 각 경계 직전에 다시 읽는 전역이다.
    netstorm::o::LocalMissionStateBlock next{}; // next DWORD 외의35개 부가 인자 보존도 확인한다.
    std::array<std::array<std::uint8_t,32>,2> cores{}; // 별개 슬롯의 실제32BYTE State 입력이다.
    double seconds{};std::int32_t islands{}; // 외부 시계/owner 조회의 명시 입력이다.
    std::string pending{"old"},mutation,trace; // pending은 current 파일명과 다른 저장소다.
    std::size_t coreCalls{}; // 원문 첫째/둘째 State 조회의 명시 변경을 구별한다.
    // 실제 다섯 블록 중 next의 첫 DWORD만 선택 부모에 빌린다.
    FortSelectionGlobals Globals() { return {mode,next[0],battle,state,slot,owner}; }
    // 외부 효과의 실제 호출 순서를 원문 trace와 같은 표현으로 기록한다.
    void Record(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 독립 원문과 동일한 명시 callback 변경만 공급한다. 제품의 후속 재조회를 대신하지 않는다.
    void Mutate(std::string_view boundary) {
        const auto prefix=std::string(boundary)+'-'; // 효과 이름과 독립 입력 suffix를 비교한다.
        if (mutation==prefix+"mode0") mode=0;else if (mutation==prefix+"mode2") mode=2;
        else if (mutation==prefix+"slot1") slot=1;else if (mutation==prefix+"owner2") owner=2;else if (mutation==prefix+"next99") next[0]=99;
    }
    // 각 외부 반환/변경은 원문과 같은 명시 장치 입력이며 분기/State 쓰기는 제품에 맡긴다.
    FortSelectionHooks Hooks() {
        FortSelectionHooks hooks;
        hooks.resolveString=[](auto token)->std::string_view { if (token!=202U) throw std::logic_error("ReadFort가 event.y 이외 토큰을 읽음");return "Selected"; };
        hooks.lastEditPath=[this] { Record("path:lastEditFort:0:0:0");Mutate("path");return "LastEdit"; };
        hooks.pending=[this](auto name) { Record("pending:"+std::string(name));SetPendingFortName(pending,name);Mutate("pending"); };
        hooks.refresh=[this](auto value) { Record("refresh:"+std::to_string(value));Mutate("refresh"); };
        hooks.tell=[this](auto section) { Record("tell:"+std::string(section));Mutate("tell"); };
        hooks.currentFortPath=[this] { Record("current-path");Mutate("path");return "CurrentPath"; };
        hooks.loadFort=[this](auto selected,auto name) { Record("load:"+std::to_string(selected)+':'+std::string(name));Mutate("load"); };
        hooks.core=[this](auto selected)->std::span<std::uint8_t> {
            Record("core:"+std::to_string(selected));++coreCalls;
            Mutate("core"+std::to_string(coreCalls));
            return cores.at(selected);
        };
        hooks.gameSeconds=[this] { Record("time");return seconds; };
        hooks.ownedIslands=[this](auto selected) { Record("islands:"+std::to_string(selected));Mutate("islands");return islands; };
        hooks.notify=[this](auto selected) { Record("notify:"+std::to_string(selected));Mutate("notify"); };return hooks;
    }
};
// 도중 실패를 정상 예약이나 원본 저장 완료로 세지 않는다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 원본 c9/ca 기본블록·3550/3560 본체 및 CRT가 실제 만든 관측만 기대값으로 사용한다.
TEST_CASE(FortSelection_IndependentNative) {
    std::ifstream input(NETSTORM_FORT_SELECTION_FIXTURE);CHECK(input.good());std::string line;std::size_t compared=0;
    while (std::getline(input,line)) { // 독립 원문 모든 입력을 같은 외부 장치로 공급한다.
        if (line.empty() || line.starts_with('#')) continue;
        const auto row=Fields(line);CHECK(row.size()==22);if (row.size()!=22) continue;
        Devices device;device.mode=std::stoul(row[2]);device.battle=std::stoul(row[4]);device.state=std::stoul(row[5]);device.slot=std::stoul(row[6]);device.owner=std::stoul(row[7]);
        device.next.fill(0xabcdef01U);device.next[0]=std::stoul(row[3]);device.seconds=std::stod(row[10]);device.islands=std::stoi(row[11]);device.mutation=row[12];
        for (std::size_t slot=0;slot<2;++slot) { // fixture에 지정된 두 State 입력을 준비한다.
            device.cores[slot].fill(0xa5);device.cores[slot][0]=static_cast<std::uint8_t>(std::stoul(row[8+slot]));
            device.cores[slot][12]=0x44;device.cores[slot][13]=0x33;device.cores[slot][14]=0x22;device.cores[slot][15]=0x11;
        }
        if (row[1]=="next6") RequestFortRead(device.next[0]);
        else if (row[1]=="next1") RequestFortEdit(device.Globals(),device.Hooks());
        else { CHECK(row[1]=="ask" || row[1]=="read");CHECK(DispatchFortSelectionCommand({row[1]=="ask" ? kAskEditFortCommand : kReadFortCommand,101,202},device.Globals(),device.Hooks())); }
        CHECK(device.mode==std::stoul(row[13]));CHECK(device.next[0]==std::stoul(row[14]));CHECK(device.slot==std::stoul(row[15]));CHECK(device.owner==std::stoul(row[16]));
        CHECK(Hex(device.cores[0])==row[17]);CHECK(Hex(device.cores[1])==row[18]);CHECK(device.pending==row[19]);CHECK(device.trace==row[20]);CHECK(row[21]=="0");
        CHECK(std::all_of(device.next.begin()+1,device.next.end(),[](auto value) { return value==0xabcdef01U; }));++compared;
    }CHECK(compared==3672); // 독립 두 정밀도 전체 행이 없으면 일부만 비교하고 통과하지 않는다.
}

// 실제 원본 입력 큐의 같은 묶음에서 마지막 선택이 앞선 next를 덮어쓴다.
TEST_CASE(FortSelection_QueueRingAndLastSelection) {
    Devices device;InputQueue queue;ScriptStringRing strings;auto hooks=device.Hooks();
    hooks.resolveString=[&](auto token) { return strings.Resolve(token); };
    hooks.pending=[&](auto name) { SetPendingFortName(device.pending,name); };
    CHECK(!QueueFortSelectionAction("EditFort","unused",queue,strings));CHECK(queue.Empty() && strings.Index()==0);
    CHECK(QueueFortSelectionAction("ReadFort","First",queue,strings));CHECK(QueueFortSelectionAction("rEaDfOrT","Second",queue,strings));
    device.next[0]=7;auto event=queue.Pop(false);event.x=-1;CHECK(DispatchFortSelectionCommand(event,device.Globals(),hooks));CHECK(device.pending=="First" && device.next[0]==6);
    device.mode=2;event=queue.Pop(false);event.x=-1;CHECK(DispatchFortSelectionCommand(event,device.Globals(),hooks));CHECK(device.pending=="Second" && device.next[0]==1 && queue.Empty());
    CHECK(QueueFortSelectionAction("AskEditFort","0",queue,strings));CHECK(DispatchFortSelectionCommand(queue.Pop(false),device.Globals(),hooks));CHECK(device.pending=="LastEdit" && device.mode==2 && device.next[0]==1);
}

// 부모의 Guide/Gump 소비 뒤 release는 무효이며 Control/Shift/Alt는 별도 거부하지 않는다.
TEST_CASE(FortSelection_InputFlagsAndUnselectedHooks) {
    Devices device;device.next[0]=7;
    CHECK(!DispatchFortSelectionCommand({kReadFortCommand|InputCode::kRelease,0,202},device.Globals(),{}));
    CHECK(!DispatchFortSelectionCommand({0x02000089U,0,202},device.Globals(),{}));CHECK(device.next[0]==7 && device.pending=="old");
    CHECK(DispatchFortSelectionCommand({kReadFortCommand|InputCode::kControl|InputCode::kShift|InputCode::kAlt|InputCode::kOutside,101,202},device.Globals(),device.Hooks()));CHECK(device.next[0]==6);
    device.mode=2;CHECK(DispatchFortSelectionCommand({kReadFortCommand,101,202},device.Globals(),device.Hooks()));CHECK(device.next[0]==1);
    device.battle=1;device.state=13;RequestFortEdit(device.Globals(),{});CHECK(device.next[0]==1); // state14가 아니면 파일/시계 장치도 선택하지 않는다.
}

// 진단이 복귀하면 원본처럼 점을 포함한 이름도 복사한다. 예외면 앞선 쓰기를 보존한다.
TEST_CASE(FortSelection_PendingDiagnosticAndPartialFailure) {
    std::string pending="old";std::size_t diagnostics{};
    SetPendingFortName(pending,std::string_view("Map\0.ignored",12));CHECK(pending=="Map");
    SetPendingFortName(pending,"map.fort",[&] { ++diagnostics; });CHECK(pending=="map.fort" && diagnostics==1);
    CHECK(Rejects([&] { SetPendingFortName(pending,"bad.name"); }));CHECK(pending=="map.fort");
    Devices device;device.next[0]=7;CHECK(Rejects([&] { DispatchFortSelectionCommand({kAskEditFortCommand,0,0},device.Globals(),{}); }));CHECK(device.mode==2 && device.next[0]==7);
    auto hooks=device.Hooks();hooks.pending=[](auto) { throw std::logic_error("명시 pending 실패"); };
    CHECK(Rejects([&] { DispatchFortSelectionCommand({kReadFortCommand,101,202},device.Globals(),hooks); }));CHECK(device.next[0]==7);
    device.battle=1;device.state=14;CHECK(Rejects([&] { RequestFortEdit(device.Globals(),{}); }));CHECK(device.next[0]==1);
    hooks=device.Hooks();hooks.core=[](auto)->std::span<std::uint8_t> { return {}; };
    CHECK(Rejects([&] { RequestFortEdit(device.Globals(),hooks); }));CHECK(device.trace=="current-path;load:0:CurrentPath" && device.next[0]==1);
}
