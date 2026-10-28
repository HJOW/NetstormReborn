// 원본 로그의 필터/접두/개행과현재공유버퍼·스트림재조회를검사한다.
#include "TestSupport.h"
#include "client/RawTraceLog.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 기대값은원문fixture에서만읽고입력/외부장치변형만구성한다.
using namespace netstorm::client;
// 원시바이트를문자인코딩변환없이단일16진수열로표현한다.
std::string Hex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string text; // 낮은8비트값의두자리표다.
    for (const unsigned char value:bytes) { text+=digits[value>>4];text+=digits[value&15]; } // 전체원시바이트를기록한다.
    return text;
}
// '-'는빈입력이고그밖의16진수쌍은NUL을포함해그대로복원한다.
std::string Unhex(const std::string& text) {
    if (text=="-") return {};std::string bytes; // 실제입력C문자열의전후바이트다.
    for (std::size_t i=0;i<text.size();i+=2) bytes+=static_cast<char>(std::stoul(text.substr(i,2),nullptr,16)); // 두자리씩복원한다.
    return bytes;
}
// 원문입력/기대값의모든탭열을빠짐없이읽는다.
std::vector<std::string> Columns(const std::string& line) {
    std::stringstream stream(line);std::string field;std::vector<std::string> result; // 현재열과원문순서의전체결과다.
    while (std::getline(stream,field,'\t')) result.push_back(field); // 모든열을분리한다.
    return result;
}
// 실제공유상태·명시로그입력·외부효과변경을보유하는독립장면이다.
struct LogScene {
    TraceLogState state; // 원문현재전역과1024BYTE물리버퍼다.
    std::int32_t level{};std::size_t prefix{};std::string text; // 함수의level/format접두수와서식완료바이트다.
    std::uint32_t mutation{},writes{}; // 명시외부변형번호와fprintf호출횟수다.
    std::string events; // 장치의실제진입순서/인자를보존한다.
    // 현재장치기록앞에필요한구분자만추가한다.
    void Event(const std::string& value) { if (!events.empty()) events+=';';events+=value; }
    // 콜백의공유버퍼변경은NUL까지만쓰고나머지stale값은남긴다.
    void Replace(std::string_view value) { std::copy(value.begin(),value.end(),state.buffer.begin());state.buffer[value.size()]=0; }
    // 실제원문입력만설치하고예상결과는읽지않는다.
    explicit LogScene(const std::vector<std::string>& columns) {
        level=std::stoi(columns.at(0));state.level=std::stoi(columns.at(1));state.indent=static_cast<std::uint32_t>(std::stoul(columns.at(2)));
        state.stream=static_cast<TraceLogStream>(std::stoull(columns.at(3)));state.mirror=static_cast<std::uint32_t>(std::stoul(columns.at(4)));state.flush=static_cast<std::uint32_t>(std::stoul(columns.at(5)));
        prefix=static_cast<std::size_t>(std::stoull(columns.at(6)));text=Unhex(columns.at(7));mutation=static_cast<std::uint32_t>(std::stoul(columns.at(8)));
        state.buffer.fill(static_cast<char>(0xa5));Replace("old");
    }
    // 각원문출력경계가돌아오는시점에만같은전역/버퍼를변경한다.
    TraceLogHooks Hooks() {
        TraceLogHooks hooks;
        hooks.console=[this](std::string_view value) {
            Event("C:"+Hex(value));
            if (mutation==1) state.stream=2000;
            else if (mutation==2) { state.mirror=1;state.flush=1; }
            else if (mutation==3) { state.stream=0;Replace("changed\n"); }
            else if (mutation==4) Replace("nested replacement");
            else if (mutation==8) { state.level=0;state.indent=99; }
        };
        hooks.write=[this](TraceLogStream stream,std::string_view value) {
            Event("W:"+std::to_string(stream)+":"+Hex(value));++writes;
            if (writes==1 && mutation==5) { state.stream=3000;state.flush=1;Replace("after mirror"); }
            else if (writes==1 && mutation==6) state.flush=0;
        };
        hooks.flush=[this](TraceLogStream stream) { Event("F:"+std::to_string(stream));if (mutation==7) state.stream=0; };
        return hooks;
    }
    // 외부장치/현재전역/1024BYTE모두를원문과같은열형식으로기록한다.
    std::string Result() const {
        return events+"|"+std::to_string(state.stream)+","+std::to_string(static_cast<std::uint32_t>(state.level))+","+std::to_string(state.indent)+","+std::to_string(state.mirror)+","+std::to_string(state.flush)+"|"+Hex({state.buffer.data(),state.buffer.size()});
    }
};
}

// 원문81개입력의전체장치인자/최종전역/공유버퍼staleBYTE가모두같아야한다.
TEST_CASE(RawTraceLog_IndependentNative81FilterMarkupLinesAndCurrentState) {
    std::ifstream input(NETSTORM_TRACE_LOG_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{}; // 실제고유입력을센다.
    while (std::getline(input,line)) { // 두정밀도에서일치한모든원문행을비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Columns(line);LogScene scene(columns);WriteRawTraceLog(scene.state,scene.level,scene.text,scene.prefix,scene.Hooks());
        CHECK(scene.Result()==columns.at(9));++rows;
    }
    CHECK(rows==81);
}

// 필터된호출은장치/입력을검사하지않고실제도달한누락장치는앞선효과를보존한다.
TEST_CASE(RawTraceLog_HostErrorsAndFilteredCallsKeepPriorEffects) {
    TraceLogState state;WriteRawTraceLog(state,-1,"bad",999,{});CHECK(state.stream==0 && state.buffer[0]==0);
    TraceLogHooks hooks;std::uint32_t console{};hooks.console=[&console](auto) { ++console; };bool threw=false; // write만누락시켜앞선console/버퍼효과를검사한다.
    try { WriteRawTraceLog(state,29,"line\r\n",0,hooks); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && console==1 && state.stream==1 && std::string_view(state.buffer.data())=="line\n");
    const auto before=state.buffer;threw=false;
    try { WriteRawTraceLog(state,29,std::string(1024,'x'),0,hooks); }catch (const std::length_error&) { threw=true; }
    CHECK(threw && state.buffer==before);
}

// 직접flush는현재FILE0을건너뛰고비0이면콜백이바꾼최종상태를유지한다.
TEST_CASE(RawTraceLog_FlushUsesCurrentStreamAndZeroNeedsNoDevice) {
    TraceLogState state;FlushRawTraceLog(state,{});state.stream=1000;TraceLogHooks hooks;std::uint32_t calls{}; // 실제선택된FILE을관측한다.
    hooks.flush=[&](auto stream) { CHECK(stream==1000);++calls;state.stream=0; };FlushRawTraceLog(state,hooks);CHECK(calls==1 && state.stream==0);
}

// 같은로그의실제중첩호출이공유버퍼를바꾸면바깥출력도그현재버퍼를읽는다.
TEST_CASE(RawTraceLog_ReentrantConsoleUsesOneActualBuffer) {
    TraceLogState state;state.stream=1000;state.indent=3;state.mirror=state.flush=1;TraceLogHooks hooks;std::uint32_t console{},writes{},flushes{}; // 한물리상태의모든호출수를센다.
    hooks.console=[&](auto) { if (++console==1) WriteRawTraceLog(state,29,"~1~ynested\r\n",4,hooks); };
    hooks.write=[&](auto stream,auto text) { CHECK(stream==2 || stream==1000);CHECK(text=="~1~y   nested\n");++writes; };
    hooks.flush=[&](auto stream) { CHECK(stream==1000);++flushes; };
    WriteRawTraceLog(state,29,"outer\n",0,hooks);CHECK(console==2 && writes==4 && flushes==2);
    CHECK(std::string_view(state.buffer.data())=="~1~y   nested\n");
}
