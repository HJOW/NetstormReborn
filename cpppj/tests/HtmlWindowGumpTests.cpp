// 실제 원문 HTML 부모6본문/24DWORD/20행/8192BYTE와 부분 효과를 독립 최초 관측에 대조한다.
#include "TestSupport.h"
#include "client/HtmlWindowGump.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace netstorm::client;
namespace { // 원시 입력/관측과 명시 하위 장치만 공급한다.
std::vector<std::string> HtmlSplit(std::string_view text,char separator) { std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } } // 빈 입력 열도 보존한다.
std::vector<std::uint32_t> HtmlNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& item:HtmlSplit(text,':')) result.push_back(static_cast<std::uint32_t>(std::stoull(item)));return result; } // DWORD 원시 목록이다.
std::string HtmlPack(const std::vector<std::uint32_t>& values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(value); }return result; } // 실제 현재DWORD만 직렬화한다.
std::vector<std::uint32_t> HtmlFields(const HtmlWindowGumpNode& n) { const auto time=std::bit_cast<std::uint64_t>(n.timeoutSeconds);return {n.vtable,n.flags,n.command,n.secondaryDepth,std::bit_cast<std::uint32_t>(n.scale),std::bit_cast<std::uint32_t>(n.baseWidth),std::bit_cast<std::uint32_t>(n.baseHeight),std::bit_cast<std::uint32_t>(n.width),std::bit_cast<std::uint32_t>(n.height),std::bit_cast<std::uint32_t>(n.x),std::bit_cast<std::uint32_t>(n.y),n.controlCount,n.body,n.updateDepth,n.requestedWidth,n.requestedHeight,n.menuColumns,n.auxiliarySerial,static_cast<std::uint32_t>(time),static_cast<std::uint32_t>(time>>32),n.timeoutCommand,n.timeoutParameter,n.onExitCommand,n.onExitParameter}; } // 원문 같은24필드다.
void HtmlSet(HtmlWindowGumpNode& n,const std::vector<std::uint32_t>& f) { n.vtable=f[0];n.flags=f[1];n.command=f[2];n.secondaryDepth=f[3];n.scale=std::bit_cast<float>(f[4]);n.baseWidth=std::bit_cast<float>(f[5]);n.baseHeight=std::bit_cast<float>(f[6]);n.width=std::bit_cast<float>(f[7]);n.height=std::bit_cast<float>(f[8]);n.x=std::bit_cast<float>(f[9]);n.y=std::bit_cast<float>(f[10]);n.controlCount=f[11];n.body=f[12];n.updateDepth=f[13];n.requestedWidth=f[14];n.requestedHeight=f[15];n.menuColumns=f[16];n.auxiliarySerial=f[17];n.timeoutSeconds=std::bit_cast<double>(std::uint64_t(f[18])|(std::uint64_t(f[19])<<32));n.timeoutCommand=f[20];n.timeoutParameter=f[21];n.onExitCommand=f[22];n.onExitParameter=f[23]; } // 입력과 명시 장치 변경만 적용한다.
std::string HtmlHex(std::string_view bytes) { const char digits[]="0123456789abcdef";std::string result;for (const auto byte:bytes) { const auto value=static_cast<unsigned char>(byte);result+=digits[value>>4];result+=digits[value&15]; }return result; } // 실제 장치 인자의 바이트 관측이다.
std::string HtmlBytes(std::string_view text) { std::string result;for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; } // 독립 입력만 해석한다.
struct HtmlStop {}; // 명시 중단을 정상 반환과 구별한다.
}

// 기대 결과를 제품에서 만들지 않고 같은 원문 입력/하위 장치로 실제 C++ 본문을 실행한다.
TEST_CASE(HtmlWindowGump_IndependentNativeLifecycleControlsBodyAndInput) {
    std::size_t rows{};
    const std::array<std::string_view,24> names{"vtable","flags","kind","depth","scale","bw","bh","width","height","x","y","count","body","update","rw","rh","columns","aux","timeLo","timeHi","timeout","timeoutArg","exit","exitArg"}; // 같은 원시 필드 선언 순서다.
    for (const auto* path:{NETSTORM_HTML_WINDOW_FIXTURE,NETSTORM_HTML_WINDOW_IDLE_FIXTURE}) { // 최초250행과 별도 실제 Idle88행을 함께 읽기만 한다.
    std::ifstream input(path);CHECK(input.good());std::string line;std::getline(input,line);
    while (std::getline(input,line)) { // 각 입력은 새 실제 부모/자식/전역으로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=HtmlSplit(line,'\t');CHECK(f.size()==25);if (f.size()!=25) continue;
        HtmlWindowGumpNode node;HtmlSet(node,HtmlNumbers(f[3]));node.html.fill(static_cast<char>(std::stoul(f[5])));const auto slots=HtmlNumbers(f[4]);for (std::size_t i=0;i<20;++i) node.controls[i]={slots[i*3],slots[i*3+1],slots[i*3+2]}; // 현재20행 입력이다.
        std::array<ButtonGumpNode,24> children;for (std::size_t i=0;i<children.size();++i) { children[i].serial=static_cast<std::uint32_t>(1000+i);children[i].secondaryDepth=0x12345678;children[i].buttonFlags=0x2468; } // 명시 하위 장치의 현재 실제 노드다.
        HtmlWindowGumpState state;state.instances=static_cast<std::uint32_t>(std::stoul(f[6]));state.kind=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(std::stoul(f[7])));state.lastCloseTime=-7.5;std::vector<std::string> trace;
        const auto child=[&](std::uint32_t token)->ButtonGumpNode& { if (token<2 || token>25) throw std::logic_error("검사 실제 자식 주소 없음");return children[token-2]; }; // 동일한 실제 하위 주소다.
        const auto event=[&](std::string_view name,std::string arguments=std::string{}) { // 실제 호출 뒤에만 독립 장치 입력의 변경/중단을 적용한다.
            trace.push_back(std::string(name)+(arguments.empty() ? "" : ':'+arguments));if (f[12]!="-") { const auto mutation=HtmlSplit(f[12],'.');if (mutation[0]==name) { const auto field=std::find(names.begin(),names.end(),mutation[1]);CHECK(field!=names.end());auto values=HtmlFields(node);values[static_cast<std::size_t>(field-names.begin())]=static_cast<std::uint32_t>(std::stoul(mutation[2]));HtmlSet(node,values); } }if (f[13]==name) throw HtmlStop{};
        };
        HtmlWindowGumpHooks h;h.node=[&](auto token) { return token==1 ? &node : nullptr; };h.base=[&](auto token)->GumpCreateNode* { if (token==1) return &node;if (token>=2 && token<=25) return &child(token);return nullptr; };h.button=[&](auto token) { return &child(token); };
        h.constructDecoration=[&](auto) { node.vtable=0x50ac70;node.flags=9;node.command=57;node.secondaryDepth=0;node.scale=1;node.baseWidth=node.baseHeight=node.width=node.height=node.x=node.y=0;event("base"); };h.destructBase=[&](auto) { event("destruct"); };h.push=[&](auto token) { event("push",std::to_string(token)); };h.kill=[&](auto token) { event("kill",std::to_string(token)); };
        h.allocate=[&](auto size) { event("alloc",std::to_string(size));CHECK(size==0x170 || size==0xd8);return size==0x170 ? 24U : 25U; };h.constructButton=[&](auto token,auto label) { child(token).buttonFlags=0;event("button",std::to_string(token)+':'+HtmlHex(label)); };h.constructBody=[&](auto token) { child(token).secondaryDepth=0;event("body",std::to_string(token)); };
        h.parseBody=[&](auto token,auto source) { event("parse",std::to_string(token)+':'+HtmlHex(source)); };h.scrollable=[&](auto token,auto value) { event("scroll",std::to_string(token)+':'+std::to_string(static_cast<std::uint32_t>(value))); };h.attach=[&](auto token,float x,float y,auto parent,auto mode) { event("attach",HtmlPack({token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode})); };
        h.reference=[&](auto serial) { event("reference",std::to_string(serial));return static_cast<std::uint32_t>(std::stoul(f[10])); };h.wallSeconds=[&] { event("wall");return std::bit_cast<double>(std::stoull(f[11])); };h.queue=[&](auto code,auto first,auto second) { event("queue",HtmlPack({code,first,second})); };h.link=[&](auto token) { event("link",std::to_string(token)); };h.baseHandle=[&](auto,auto code,auto first,auto second) { event("baseHandle",HtmlPack({code,first,second}));return 0xfedcba98U; };
        HtmlWindowGump body(state,std::move(h));const auto args=HtmlNumbers(f[8]);const auto text=HtmlBytes(f[9]);bool completed=true;std::uint32_t result{};
        try { if (f[1]=="construct") result=body.Construct(1,args[0],args[1]);else if (f[1]=="destroy") body.Destruct(1);else if (f[1]=="clear") body.ClearButtons(1);else if (f[1]=="add") body.AddButton(1,text,args[1],args[2],args[3]);else if (f[1]=="body") body.SetBody(1,text);else if (f[1]=="idle") body.Idle(1);else { CHECK(f[1]=="handle");result=body.Handle(1,args[0],args[1],args[2]); } }catch (const HtmlStop&) { completed=false; }
        std::vector<std::uint32_t> outSlots,outChildren;for (const auto& entry:node.controls) { outSlots.push_back(entry.child);outSlots.push_back(entry.command);outSlots.push_back(entry.parameter); } // 실제20행 전체다.
        for (const auto& current:children) { outChildren.push_back(current.serial);outChildren.push_back(current.secondaryDepth);outChildren.push_back(current.buttonFlags); } // 실제 명시 자식의 변경을 읽는다.
        std::uint64_t fingerprint=14695981039346656037ULL;for (const auto byte:node.html) fingerprint=(fingerprint^static_cast<unsigned char>(byte))*1099511628211ULL;std::string calls;for (const auto& call:trace) { if (!calls.empty()) calls+=';';calls+=call; } // 실제8192BYTE와 장치 호출 순서다.
        const bool same=completed==(f[14]=="1") && HtmlPack(HtmlFields(node))==f[15] && HtmlPack(outSlots)==f[16] && fingerprint==std::stoull(f[17]) && HtmlPack(outChildren)==f[18] && state.instances==std::stoul(f[19]) && std::bit_cast<std::uint64_t>(state.lastCloseTime)==std::stoull(f[20]) && calls==f[21] && (completed ? result : 0)==std::stoul(f[22]);if (!same) std::fprintf(stderr,"HTML Native mismatch %s\nactual=%s\nexpected=%s\n",f[0].c_str(),calls.c_str(),f[21].c_str());CHECK(same);++rows;
    }
    }
    CHECK(rows==NETSTORM_HTML_WINDOW_ROWS);
}
