// 실제 원본 토큰/명령 큐/Config·문자열 수명을 기대 fixture와 대조한다.
#include "TestSupport.h"
#include "client/ScriptCommand.h"
#include "client/DialogScript.h"
#include "o/RawTextToken.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace netstorm::client;
namespace {
// fixture의원본바이트를UTF-8치환없이복원한다. '-'는빈문자열이다.
std::string Bytes(std::string_view encoded) {
    if (encoded=="-") return {};std::string result;
    // 원본의각BYTE를정확한16진수두자리에서읽는다.
    for (std::size_t i=0;i<encoded.size();i+=2) result+=static_cast<char>(std::stoul(std::string(encoded.substr(i,2)),nullptr,16));return result;
}
// TSV빈필드와원본명령인자의쉼표를혼동하지않는다.
std::vector<std::string> Fields(std::string_view line,char delimiter='\t') {
    std::vector<std::string> values;std::size_t begin=0;
    // 마지막항목까지같은순서로복사한다.
    for (;;) {
        const auto end=line.find(delimiter,begin);values.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));
        if (end==line.npos) return values;begin=end+1;
    }
}
}
// native의소비포인터/전체문자열/예약코드·인자/고리위치를모두확인한다.
TEST_CASE(ScriptCommand_IndependentPe262) {
    std::ifstream input(NETSTORM_SCRIPT_COMMAND_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    ScriptStringRing continuous;std::vector<std::uint32_t> tokens;std::vector<std::string> stored;
    // 기계어의출력값을읽으며토큰정답이나큐정답을이검사에서다시계산하지않는다.
    while (std::getline(input,line)) {
        if (line.empty() || line.starts_with("operation\t")) continue;
        const auto row=Fields(line);CHECK(row.size()==9);const auto source=Bytes(row[1]);
        if (row[0]=="Token") {
            const auto delimiter=Bytes(row[2]);const auto token=delimiter=="DEFAULT" ? netstorm::o::ReadRawTextToken(source) : netstorm::o::ReadRawTextToken(source,0,delimiter);
            CHECK(token.text==Bytes(row[4]));CHECK(token.next==std::stoul(row[5]));
        } else if (row[0]=="Config") {
            std::vector<std::pair<std::string,std::string>> assignments;std::size_t missing=0;
            CHECK(ApplyScriptConfig(source,[&](std::string_view key,std::string_view value) { assignments.emplace_back(key,value);return row[3]=="1"; },[&] { ++missing; })==static_cast<std::int32_t>(std::stol(row[4])));
            const auto events=Fields(row[8],';');bool found=false;
            // 설정효과의실제키/값을읽어현재Config와알림순서를확인한다.
            for (const auto& event:events) if (event.starts_with("S,")) {
                const auto values=Fields(event,',');CHECK(values.size()==3);CHECK(assignments.size()==1);
                if (!assignments.empty()) { CHECK(assignments[0].first==Bytes(values[1]));CHECK(assignments[0].second==Bytes(values[2])); }found=true;
            }
            CHECK(assignments.size()==(found ? 1U : 0U));CHECK(missing==(found ? 0U : 1U));
        } else if (row[0]=="Command") {
            InputQueue queue;ScriptStringRing strings;std::vector<std::size_t> diagnostics;
            ScriptCommandHooks hooks;hooks.lookup=[&](std::string_view name) {
                CHECK(strings.Index()==0);const auto lower=netstorm::o::AsciiLower(name);
                return lower=="config" ? (row[3]=="1" ? 0U : kScriptConfig) : lower=="warning" ? kScriptWarning : 0xffffffffU;
            };
            hooks.unknown=[&](std::string_view) { diagnostics.push_back(strings.Index()); };
            QueueScriptCommand(source,queue,strings,hooks);CHECK(strings.Index()==std::stoul(row[7]));
            if (row[6]=="-") CHECK(queue.Empty());
            else {
                CHECK(!queue.Empty());const auto actual=queue.Pop(false);const auto split=row[6].find(',');CHECK(actual.code==std::stoul(row[6].substr(0,split)));CHECK(actual.x==actual.y);
                const auto expected=row[6].substr(split+1);
                if (expected.front()=='s') CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(actual.x))==Bytes(expected.substr(1)));
                else CHECK(std::bit_cast<std::uint32_t>(actual.x)==static_cast<std::uint32_t>(std::stoul(expected.substr(1))));
                CHECK(queue.Empty());
            }
            const auto logCount=static_cast<std::size_t>(std::count(row[8].begin(),row[8].end(),'L'));CHECK(diagnostics.size()==logCount);
            if (!diagnostics.empty()) CHECK(diagnostics.front()==(row[3]=="1" ? 0U : strings.Index()));
        } else if (row[0]=="Ring") {
            const auto token=continuous.Store(source);tokens.push_back(token);stored.push_back(source);CHECK(continuous.Resolve(token)==Bytes(row[4]));CHECK(continuous.Index()==std::stoul(row[7]));
            // 기존80수명이보존되고81번째복사부터정확히이전수명만무효화되는지확인한다.
            for (std::size_t i=0;i<tokens.size();++i) {
                bool rejected=false;try { CHECK(continuous.Resolve(tokens[i])==stored[i]); }catch (const std::out_of_range&) { rejected=true; }
                CHECK(rejected==(tokens.size()-i>80));
            }
        } else CHECK(false);
        ++count;
    }
    CHECK(count==262);
}
// 완료값은기존정의를덮어쓰고같은입력큐내다른명령을버리지않는다.
TEST_CASE(ScriptCommand_ConfigOverwriteAndQueueLifetime) {
    netstorm::o::ConfigInterface config({});config.Configuration().AppendSection("END");config.Set("DoneTutorial1","0");std::size_t changed=0;config.onChanged=[&] { ++changed; };
    ScriptStringRing strings;InputQueue queue;ScriptCommandHooks hooks{[](std::string_view) { return kScriptConfig; },{}};
    QueueScriptCommand("Config,DoneTutorial1=1",queue,strings,hooks);QueueScriptCommand("Config,DoneTutorial2=1",queue,strings,hooks);
    CHECK(config.GetInt("DoneTutorial1")==0 && changed==0);
    // 실제입력순서로예약값을적용하며첫State전환만보존하는별도큐를사용하지않는다.
    while (!queue.Empty()) { const auto event=queue.Pop(false);CHECK(event.code==kScriptConfig);ApplyScriptConfig(strings.Resolve(std::bit_cast<std::uint32_t>(event.x)),config); }
    CHECK(config.GetInt("DoneTutorial1")==1 && config.GetInt("DoneTutorial2")==1 && changed==2);
    CHECK(netstorm::o::ConfigText(config.SaveText("options.cfg")).GetRaw("DoneTutorial1")=="1");
    bool rejected=false;try { strings.Resolve(0); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    const auto page=ParseDialogPage("before <$Config,DoneTutorial1=1><$Warning,\"A, B > C\"> after\n$Button=OK,DoNothing,0"); // 혼합명령/인용구분자/본문과버튼을서로침범하지않는입력이다.
    CHECK(page.commands==std::vector<std::string>({"Config,DoneTutorial1=1","Warning,\"A, B > C\""}));
    CHECK(page.htmlBody=="before   after\n" || page.htmlBody=="before  after\n");CHECK(page.items.size()==1);
    CHECK(ParseDialogPage("<?0><$Config,Hidden=1></?>").commands.empty());
}
