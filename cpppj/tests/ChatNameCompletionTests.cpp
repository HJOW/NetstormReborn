// 원문189행의연속후보/접두상태를동일한물리배치에대조한다.
#include "TestSupport.h"
#include "client/ChatNameCompletion.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <vector>
using namespace netstorm::client;
namespace { //fixture해석과번역입력만제공하며기대상태를계산하지않는다.
std::vector<std::string> Fields(std::string_view text,char separator='\t') { //빈마지막필드를보존한다.
    std::vector<std::string> result;std::size_t begin{}; //현재필드시작과실제열순서다.
    for (;;) { //구분자위치만해석한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
std::string Decode(std::string_view value) { //NULL표시는호출자가처리하고빈BYTE열은'-'로표현한다.
    if (value=="-") return {};std::string result;for (std::size_t index=0;index<value.size();index+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(index,2)),nullptr,16));return result; //원시16진수두글자씩읽는다.
}
std::string Encode(std::span<const std::uint8_t> bytes,bool trim=false) { //물리상태의끝0만선택적으로줄이고중간0을남긴다.
    if (trim) while (!bytes.empty() && !bytes.back()) bytes=bytes.first(bytes.size()-1); //BSS뒤쪽0을fixture와같은방법으로표현한다.
    constexpr char digits[]="0123456789abcdef";std::string result; //실제BYTE의16진수표시표와결과다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } //순서와각BYTE를보존한다.
    return result.empty() ? "-" : result;
}
}
//번역중복/빈값/128BYTE넘침·100행·현재기록변경·접두잔류·정/역순환을전체상태로대조한다.
TEST_CASE(ChatNameCompletion_Native189PhysicalStateAndSequentialCycles) {
    std::ifstream file(NETSTORM_CHAT_NAME_COMPLETION_FIXTURE);CHECK(file.good());std::string line;ChatNameCompletionState state;std::size_t count{}; //행사이에원문BSS를유지한다.
    while (std::getline(file,line)) { //최초원문기대자료를그대로소비한다.
        if (line.starts_with("name\t")) continue;const auto f=Fields(line);CHECK(f.size()==11);if (f.size()!=11) continue;if (f[1]=="1") state={};std::array<std::uint8_t,4800> records{};const auto input=Decode(f[2]);std::copy(input.begin(),input.end(),records.begin());const auto mode=std::stoul(f[3]);std::string trace;
        const auto append=[&](std::string_view item) { if (!trace.empty()) trace+=';';trace+=item; }; //실제선택된외부경계의순서다.
        ChatNameCompletionHooks hooks;hooks.records=[&] { return std::span<const std::uint8_t>(records); };hooks.assertion=[&](auto line) { append("A:"+std::to_string(line)); };
        hooks.translate=[&](auto key) { append("T:"+Encode({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()}));if (mode==1) { if (key=="BATTLE") return std::string("Area");if (key=="EVERYONE") return std::string("Tous");if (key=="ALLIES") return std::string("Friends");return std::string("Eyes"); }if (mode==2) return std::string("dup");if (mode==3 && key=="ALLIES") return std::string();if ((mode==4 || mode==5) && key=="BATTLE") return std::string(mode==4 ? 127 : 128,'Z');return std::string(key); };
        const auto text=f[4]=="~" ? std::string() : Decode(f[4]);const std::optional<std::string_view> query=f[4]=="~" ? std::nullopt : std::optional<std::string_view>(text);const auto result=CompleteChatName(state,query,static_cast<std::int32_t>(std::stol(f[5])),f[6]=="1",hooks);
        CHECK((result ? Encode({reinterpret_cast<const std::uint8_t*>(result->data()),result->size()}) : "~")==f[7]);CHECK(Encode(state.bytes,true)==f[8]);CHECK((trace.empty() ? "-" : trace)==f[9]);CHECK(std::stoul(f[10])>0);++count;
    }
    CHECK(count==189);
}
//NULL순환은장치가없어도현재저장상태만사용하며새단어는실제기록을요구한다.
TEST_CASE(ChatNameCompletion_LazySnapshotAndPhysicalBoundary) {
    ChatNameCompletionState state;CHECK(!CompleteChatName(state,std::nullopt,0,false,{}));std::uint32_t reads{};std::array<std::uint8_t,4800> records{};ChatNameCompletionHooks hooks;hooks.translate=[](auto text) { return std::string(text); };hooks.records=[&] { ++reads;return std::span<const std::uint8_t>(records); };
    CHECK(CompleteChatName(state,"",0,false,hooks)=="BATTLE");CHECK(CompleteChatName(state,std::nullopt,0,false,{})=="EVERYONE");CHECK(reads==1);bool rejected{};try { CompleteChatName(state,"A",-1,false,hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && reads==2);
}
