// 독립 원문 전체 생성106행의 현재 전역/부모8192BYTE/순서·반환을 대조한다.
#include "TestSupport.h"
#include "client/HtmlWindowFactory.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>
#include <map>
using namespace netstorm::client;
namespace { // 입력 디코드와 명시 하위 장치만 준비한다.
std::vector<std::string> FactoryFields(std::string_view text,char separator='\t') { std::vector<std::string> result;std::size_t start{};for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; } } // 마지막 빈 필드도 같은 순서로 읽는다.
std::string FactoryBytes(std::string_view text) { std::string result;for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; } // 원문 좁은 바이트를 정규화하지 않는다.
std::string FactoryHex(std::string_view text) { std::string result;char buffer[3];for (const unsigned char byte:text) { std::snprintf(buffer,sizeof(buffer),"%02x",byte);result+=buffer; }return result; } // Native와 같은 바이트 관측이다.
struct FactoryStop {}; // 명시 선택 장치 중단이며 일반 C++/원문 EH 검증과 구별한다.
}

// 상위 계산 결과는 fixture에서 읽고 하위 생성/파서/메타 장치에만 명시 입력을 공급한다.
TEST_CASE(HtmlWindowFactory_IndependentWholeNativeAndCurrentEffects) {
    std::ifstream file(NETSTORM_HTML_FACTORY_FIXTURE);CHECK(file.good());std::string line;std::size_t compared{};
    while (std::getline(file,line)) { // 최초 원문 행의 현재 필드/trace를 그대로 비교한다.
        if (line.empty() || line.starts_with("name\t")) continue;const auto f=FactoryFields(line);CHECK(f.size()==33);if (f.size()!=33) continue;
        HtmlWindowGumpState state;state.createdSerial=777;state.lastCloseTime=-88.5;HtmlWindowGumpNode node{};node.html.fill(static_cast<char>(0xa5));bool constructed{};std::uint32_t parses{},metas{};std::string trace,firstObserved,secondObserved;const auto geometry=FactoryFields(f[13],':'),screen=FactoryFields(f[14],':');std::map<std::uint32_t,std::uint32_t> globals{{0x5c78f0,static_cast<std::uint32_t>(std::stoul(screen[0]))},{0x5c78f4,static_cast<std::uint32_t>(std::stoul(screen[1]))},{0x563e38,static_cast<std::uint32_t>(std::stoul(f[15]))}}; // 같은 부모/명시 화면 입력과 관측만 유지한다.
        const auto event=[&](std::string_view phase,std::string arguments={}) { // Native와 같은 실제 인자 기록 뒤에만 명시 변경/중단을 적용한다.
            if (!trace.empty()) trace+=';';trace+=phase;if (!arguments.empty()) trace+=':'+arguments;
            if (f[20]!="-") { const auto change=FactoryFields(f[20],'.');if (phase==change[0]) { const auto value=static_cast<std::uint32_t>(std::stoul(change[2]));if (change[1]=="created") state.createdSerial=value;else if (change[1]=="serial") node.serial=value;else if (change[1]=="width") node.baseWidth=std::bit_cast<float>(value);else if (change[1]=="height") node.baseHeight=std::bit_cast<float>(value);else globals.at(change[1]=="screenW" ? 0x5c78f0 : 0x563e38)=value; } }
            if (phase==f[21]) throw FactoryStop{};
        };
        HtmlWindowFactoryHooks h;h.node=[&](auto token) { CHECK(token==1);return &node; };
        h.create=[&](auto width,auto height) { event("allocate","8680");if (std::stoul(f[9])) return 0U;event("construct",std::to_string(width)+':'+std::to_string(height));node.serial=static_cast<std::uint32_t>(std::stoul(f[12]));node.baseWidth=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(geometry[0])));node.baseHeight=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(geometry[1])));node.html[0]=0;constructed=true;return 1U; };
        h.substitute=[&](auto source,auto capacity) { event("substitute",FactoryHex(source)+':'+std::to_string(capacity)+":0:0:0");return FactoryBytes(f[3]); };
        h.parse=[&](auto& parser,auto root,auto source,auto capacity) { parser.afterHeading=0;event("bind",std::to_string(capacity)+':'+(root ? "2:8192" : "0:0"));++parses;event("parse"+std::to_string(parses),FactoryHex(source)+':'+(root ? "2" : "0")+':'+std::to_string(parser.shown)+':'+std::to_string(parser.hiddenDepth)+':'+std::to_string(parser.afterHeading));const auto output=FactoryBytes(f[parses==1 ? 4 : 5]);if (root) { const auto links=FactoryBytes(f[parses==1 ? 6 : 7]);std::copy(links.begin(),links.end(),node.html.begin()); }if (parses==1) firstObserved=output;else secondObserved=output;return output; };
        h.wallSeconds=[&] { event("clock");return std::stod(f[16]); };h.metadata=[&](auto root,std::span<char> buffer) { CHECK(root==1);++metas;event("meta",FactoryHex(buffer.data()));if (metas==1) { if (f[18]!="-") { const auto data=FactoryBytes(f[18]);std::copy(data.begin(),data.end(),buffer.begin());buffer[data.size()]=0; }firstObserved=buffer.data(); }return static_cast<std::uint32_t>(std::stoul(f[17])); };h.fallback=[&] { event("fallback","4f6b427574746f6e:0:0:0");return FactoryBytes(f[19]); };
        h.body=[&](auto root,auto text) { CHECK(root==1);if (parses==2) secondObserved=text;event("body",FactoryHex(text)); };h.read=[&](auto address) { return globals.at(address); };h.attach=[&](auto root,float x,float y,auto parent,auto mode) { CHECK(root==1);event("attach",std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(parent)+':'+std::to_string(mode)); };h.modal=[&](auto root,bool enabled) { CHECK(root==1);event("modal",enabled ? "1" : "0"); };h.kill=[&](auto root) { CHECK(root==1);event("kill"); };
        bool completed=true;std::uint32_t result{};try { result=HtmlWindowFactory(state,std::move(h)).Show(FactoryBytes(f[2]),static_cast<std::uint32_t>(std::stoul(f[8])),static_cast<std::uint32_t>(std::stoul(f[10])),static_cast<std::uint32_t>(std::stoul(f[11]))); }catch (const FactoryStop&) { completed=false; }
        std::uint64_t fingerprint=constructed ? 14695981039346656037ULL : 0; // 같은 실제8192BYTE를 다시 읽는다.
        if (constructed) for (const unsigned char byte:node.html) fingerprint=(fingerprint^byte)*1099511628211ULL;
        if (completed!=(f[22]=="1") || trace!=f[29]) std::printf("factory native mismatch case=%s control=%s trace=%s\n",f[0].c_str(),f[1].c_str(),trace.c_str());
        CHECK(completed==(f[22]=="1") && state.createdSerial==std::stoul(f[23]) && std::bit_cast<std::uint64_t>(state.lastCloseTime)==std::stoull(f[24]) && (constructed ? node.serial : 0)==std::stoul(f[25]) && fingerprint==std::stoull(f[26]) && FactoryHex(firstObserved)==f[27] && FactoryHex(secondObserved)==f[28] && trace==f[29] && result==std::stoul(f[30]));if (completed) CHECK(f[31]=="4");++compared;
    }
    CHECK(compared==106);
}
