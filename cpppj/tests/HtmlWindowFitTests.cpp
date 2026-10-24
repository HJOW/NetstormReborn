// 원문 전체 Fit/실제_ftol의 독립160행과 같은 입력·명시 측정·현재 객체를 대조한다.
#include "TestSupport.h"
#include "client/HtmlWindowFit.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace netstorm::client;
namespace { // 기대 배치를 계산하지 않고 원시 입력/명시 장치/출력만 다룬다.
std::vector<std::string> FitSplit(std::string_view source,char separator) { std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=source.find(separator,begin);result.emplace_back(source.substr(begin,end==source.npos ? source.npos : end-begin));if (end==source.npos) return result;begin=end+1; } } // 빈 마지막 열도 보존한다.
std::vector<std::uint32_t> FitNumbers(std::string_view source) { std::vector<std::uint32_t> result;if (source.empty()) return result;for (const auto& part:FitSplit(source,':')) result.push_back(static_cast<std::uint32_t>(std::stoull(part)));return result; } // 입력 DWORD의 원시 비트다.
std::string FitPack(const std::vector<std::uint32_t>& values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(value); }return result; } // 실제 관측의 십진수 목록이다.
std::vector<std::uint32_t> FitFields(const GumpCreateNode& n) { return {n.flags,n.depth,std::bit_cast<std::uint32_t>(n.scale),std::bit_cast<std::uint32_t>(n.baseWidth),std::bit_cast<std::uint32_t>(n.baseHeight),std::bit_cast<std::uint32_t>(n.width),std::bit_cast<std::uint32_t>(n.height),std::bit_cast<std::uint32_t>(n.x),std::bit_cast<std::uint32_t>(n.y)}; } // 같은9개 현재 필드다.
void FitSet(GumpCreateNode& n,std::span<const std::uint32_t> f) { n.flags=f[0];n.depth=f[1];n.scale=std::bit_cast<float>(f[2]);n.baseWidth=std::bit_cast<float>(f[3]);n.baseHeight=std::bit_cast<float>(f[4]);n.width=std::bit_cast<float>(f[5]);n.height=std::bit_cast<float>(f[6]);n.x=std::bit_cast<float>(f[7]);n.y=std::bit_cast<float>(f[8]); } // 같은 실제 객체에 입력만 설치한다.
std::vector<std::uint32_t> FitRoot(const HtmlWindowGumpNode& n) { return {n.controlCount,n.body,n.requestedWidth,n.requestedHeight,n.menuColumns,n.auxiliarySerial}; } // 현재 부모의6부가 필드다.
void FitRootSet(HtmlWindowGumpNode& n,const std::vector<std::uint32_t>& f) { n.controlCount=f[0];n.body=f[1];n.requestedWidth=f[2];n.requestedHeight=f[3];n.menuColumns=f[4];n.auxiliarySerial=f[5]; } // 입력과 명시 하위 변경만 적용한다.
struct FitStop {}; // 선택 장치의 중단을 정상 반환과 구별한다.
}

// 독립 원문이 실제 계산한26객체/현재 필드/20child/메뉴 스택·순서를 C++와 대조한다.
TEST_CASE(HtmlWindowFit_IndependentNativeAllGeometryAndCurrentEffects) {
    std::ifstream input(NETSTORM_HTML_WINDOW_FIT_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    const std::array<std::uint32_t,9> addresses{0x5ca9b4,0x531860,0x531864,0x5c78f0,0x5c78f4,0x5ca9cc,0x5ca9d4,0x5ca9d8,0x5ca9d0};const std::array<std::string_view,6> rootNames{"count","body","rw","rh","columns","aux"}; // Native 실제 선언 순서다.
    while (std::getline(input,line)) { // 각 최초 행마다 새 실제 부모와25자식 저장소를 만든다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=FitSplit(line,'\t');CHECK(f.size()==24);if (f.size()!=24) continue;
        HtmlWindowGumpNode node;std::array<GumpCreateNode,25> children;const auto object=[&](std::uint32_t token)->GumpCreateNode& { if (token==1) return node;if (token<2 || token>26) throw std::logic_error("Fit 실제 관측 주소 없음");return children[token-2]; }; // 같은 현재 주소다.
        const auto fields=FitNumbers(f[2]);for (std::uint32_t token=1;token<=26;++token) FitSet(object(token),std::span(fields).subspan((token-1)*9,9));FitRootSet(node,FitNumbers(f[3]));const auto slots=FitNumbers(f[4]);for (std::size_t index=0;index<20;++index) node.controls[index]={slots[index],0,0}; // 모든 현재 입력 객체/행이다.
        auto globals=FitNumbers(f[5]);const auto natural=FitNumbers(f[6]),widths=FitNumbers(f[7]),heights=FitNumbers(f[8]);auto menus=FitNumbers(f[9]);std::size_t heightCalls{};std::vector<std::string> trace;
        const auto event=[&](std::string_view name,const std::vector<std::uint32_t>& args) { // 장치의 실제 효과 뒤에만 독립 입력의 변경/중단을 적용한다.
            trace.push_back(std::string(name)+(args.empty() ? "" : ':'+FitPack(args)));if (f[12]!="-") { const auto mutation=FitSplit(f[12],'.');if (name==mutation[0]) { const auto value=static_cast<std::uint32_t>(std::stoul(mutation[2]));if (mutation[1]=="view") globals[0]=value;else { const auto field=std::find(rootNames.begin(),rootNames.end(),mutation[1]);CHECK(field!=rootNames.end());auto values=FitRoot(node);values[static_cast<std::size_t>(field-rootNames.begin())]=value;FitRootSet(node,values); } } }if (f[13]==name) throw FitStop{};
        };
        const auto attach=[&](std::string_view name,auto token,float x,float y,auto parent,auto mode) { auto& target=object(token);target.x=x;target.y=y;target.flags&=~8U;event(name,{token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode}); }; // 받은 원시 좌표를 저장할 뿐 기대 배치를 계산하지 않는다.
        HtmlWindowFitHooks h;h.node=[&](auto token) { return token==1 ? &node : nullptr; };h.base=[&](auto token) { return &object(token); };h.fitButton=[&](auto token) { CHECK(token>=4 && token<=23);object(token).baseWidth=std::bit_cast<float>(widths[token-4]);event("fitButton",{token}); };h.push=[&](auto token) { object(token).flags|=8;event("push",{token}); };
        h.fitContent=[&](auto token,auto x,auto y) { CHECK(x==0 && y==0);object(token).baseWidth=std::bit_cast<float>(natural[0]);object(token).baseHeight=std::bit_cast<float>(natural[1]);event("fitContent",{token}); };h.fitHeight=[&](auto token) { CHECK(heightCalls<heights.size());object(token).baseHeight=std::bit_cast<float>(heights.at(heightCalls++));event("fitHeight",{token}); };
        h.slider=[&](auto token,auto value) { event("slider",{token,value}); };h.scrollable=[&](auto token,auto value) { event("scroll",{token,value}); };h.requestScroll=[&](auto token,auto value) { event("request",{token,static_cast<std::uint32_t>(value)}); };
        h.attach=[&](auto token,auto x,auto y,auto parent,auto mode) { attach("attach",token,x,y,parent,mode); };h.attachBase=[&](auto token,auto x,auto y,auto parent,auto mode) { attach("attachBase",token,x,y,parent,mode); };
        h.currentMenu=[&] { CHECK(!menus.empty());const auto token=menus.back();event("menuCurrent",{token});return token; };h.attachMenu=[&](auto x,auto y,auto parent) { CHECK(!menus.empty());const auto token=menus.back();auto& target=object(token);target.x=static_cast<float>(x);target.y=static_cast<float>(y);target.flags&=~8U;event("menuAttach",{token,static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y),parent});menus.pop_back(); };
        h.reference=[&](auto serial) { event("reference",{serial});return static_cast<std::uint32_t>(std::stoul(f[11])); };h.read=[&](auto address) { const auto found=std::find(addresses.begin(),addresses.end(),address);CHECK(found!=addresses.end());return globals[static_cast<std::size_t>(found-addresses.begin())]; };h.addDepth=[&](auto token,auto delta) { object(token).depth+=delta;event("depth",{token,delta}); };
        HtmlWindowFit fit(std::move(h));const auto args=FitNumbers(f[10]);bool completed=true;try { fit.Attach(1,std::bit_cast<float>(args[0]),std::bit_cast<float>(args[1]),args[2],args[3]); }catch (const FitStop&) { completed=false; }
        std::vector<std::uint32_t> out;for (std::uint32_t token=1;token<=26;++token) { const auto values=FitFields(object(token));out.insert(out.end(),values.begin(),values.end()); } // 실제 모든 객체의 현재 필드다.
        std::vector<std::uint32_t> outSlots;for (const auto& entry:node.controls) outSlots.push_back(entry.child);std::string calls;for (const auto& call:trace) { if (!calls.empty()) calls+=';';calls+=call; } // 실제20행/장치 순서다.
        const bool same=completed==(f[14]=="1") && FitPack(out)==f[15] && FitPack(FitRoot(node))==f[16] && FitPack(outSlots)==f[17] && FitPack(globals)==f[18] && FitPack(menus)==f[19] && heightCalls==std::stoul(f[20]) && calls==f[21];if (!same) std::fprintf(stderr,"HTML Fit mismatch %s actual=%s expected=%s\n",f[0].c_str(),calls.c_str(),f[21].c_str());CHECK(same);++rows;
    }
    CHECK(rows==160);
}
