// 원문 BackgroundGif308행의 모든 본체/현재 필드 재조회/명시 중단을 독립 입력으로 대조한다.
#include "TestSupport.h"
#include "client/BackgroundGifGump.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace netstorm::client;
namespace { // 원시 필드/장치 관측만 처리하며 게임 계산의 기대값을 만들지 않는다.
std::vector<std::string> GifSplit(std::string_view text,char separator) { std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } } // 빈 끝 열도 보존한다.
std::uint32_t GifNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); } // 원문 signed 입력도 하위DWORD를 보존한다.
std::vector<std::uint32_t> GifNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& item:GifSplit(text,':')) result.push_back(GifNumber(item));return result; } // 입력 원시 목록만 읽는다.
std::vector<std::uint32_t> GifFields(const BackgroundGifGumpNode& n) { return {n.vtable,n.flags,n.command,std::bit_cast<std::uint32_t>(n.scale),std::bit_cast<std::uint32_t>(n.baseWidth),std::bit_cast<std::uint32_t>(n.baseHeight),std::bit_cast<std::uint32_t>(n.width),std::bit_cast<std::uint32_t>(n.height),std::bit_cast<std::uint32_t>(n.x),std::bit_cast<std::uint32_t>(n.y),n.event,n.first,n.second,n.bitmap}; } // 같은14필드의 현재 비트다.
void GifSet(BackgroundGifGumpNode& n,const std::vector<std::uint32_t>& f) { n.vtable=f[0];n.flags=f[1];n.command=f[2];n.scale=std::bit_cast<float>(f[3]);n.baseWidth=std::bit_cast<float>(f[4]);n.baseHeight=std::bit_cast<float>(f[5]);n.width=std::bit_cast<float>(f[6]);n.height=std::bit_cast<float>(f[7]);n.x=std::bit_cast<float>(f[8]);n.y=std::bit_cast<float>(f[9]);n.event=f[10];n.first=f[11];n.second=f[12];n.bitmap=f[13]; } // 원문 입력/장치 돌연변이만 같은 필드에 쓴다.
std::string GifPack(const std::vector<std::uint32_t>& values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(value); }return result; } // 원시DWORD 관측 목록이다.
struct GifStop {}; // 명시 장치 중단을 정상 RET와 구별한다.
}

// Native 기본/파일/픽셀/수명 장치 계약만 공급하고 실제 C++ 본문을 같은 입력으로 실행한다.
TEST_CASE(BackgroundGifGump_IndependentNativeAllBodiesAndCurrentEffects) {
    std::ifstream input(NETSTORM_BACKGROUND_GIF_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    const std::array<std::string_view,14> names{"vtable","flags","kind","scale","bw","bh","width","height","x","y","event","first","second","bitmap"}; // Native14필드의 선언 순서다.
    while (std::getline(input,line)) { // 결과 행마다 새 실제 본체/장치 문맥을 구성한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=GifSplit(line,'\t');CHECK(f.size()==19);if (f.size()!=19) continue;
        BackgroundGifGumpNode node;GifSet(node,GifNumbers(f[3]));BackgroundGifGumpState state;state.kind=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(GifNumber(f[4])));std::vector<std::string> trace;bool poisoned{};
        const auto event=[&](std::string_view name,std::vector<std::uint32_t> args=std::vector<std::uint32_t>{}) { // 실제 장치 인자 뒤에만 외부 입력의 변경/중단을 수행한다.
            trace.push_back(std::string(name)+(args.empty() ? "" : ':'+GifPack(args)));
            if (f[11]!="-") { const auto mutation=GifSplit(f[11],'.');if (mutation[0]==name) { if (mutation[1]=="kind") state.kind=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(GifNumber(mutation[2])));else if (mutation[1]=="poison") { GifSet(node,std::vector<std::uint32_t>(14,0x5c5c5c5c));poisoned=true; }else { auto values=GifFields(node);const auto found=std::find(names.begin(),names.end(),mutation[1]);CHECK(found!=names.end());values[static_cast<std::size_t>(found-names.begin())]=GifNumber(mutation[2]);GifSet(node,values); } } }
            if (f[12]==name) throw GifStop{};
        };
        BackgroundGifGumpHooks h;h.node=[&](auto token) { if (poisoned) throw std::logic_error("dispatch 뒤 원문이 읽지 않는 노드를 재조회함");return token==1 ? &node : nullptr; };h.base=[&](auto token)->GumpCreateNode* { return token==1 ? &node : nullptr; };
        h.constructBase=[&](auto) { node.vtable=0x501b60;node.flags=1;node.baseWidth=node.baseHeight=node.width=node.height=0;event("base"); };h.destructBase=[&](auto) { event("destructBase"); };
        h.pathSpec=[&](auto name) { CHECK(name=="title");event("spec");return std::string("spec"); };h.exists=[&](auto path) { CHECK(path=="spec");event("exists");return f[5]=="1"; };
        h.load=[&](auto path,std::int32_t& width,std::int32_t& height) { CHECK(path=="spec");const auto size=GifNumbers(f[7]);width=std::bit_cast<std::int32_t>(size[0]);height=std::bit_cast<std::int32_t>(size[1]);event("load");return GifNumber(f[6]); };h.releaseBitmap=[&](auto bitmap) { event("free",{bitmap}); };
        h.position=[&](auto) { const auto point=GifNumbers(f[9]);event("position");return ScreenPoint{std::bit_cast<std::int32_t>(point[0]),std::bit_cast<std::int32_t>(point[1])}; };
        h.copy=[&](auto request) { event("copy",{std::uint32_t(request.position.x),std::uint32_t(request.position.y),request.bitmap,std::uint32_t(request.stride),std::uint32_t(request.source.left),std::uint32_t(request.source.top),std::uint32_t(request.source.right),std::uint32_t(request.source.bottom)}); };
        h.attach=[&](auto,float x,float y,auto parent,auto mode) { event("attach",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode}); };h.dispatch=[&](auto code,auto first,auto second) { event("dispatch",{code,first,second}); };
        h.baseHandle=[&](auto,auto code,auto first,auto second) { event("baseHandle",{code,first,second});return GifNumber(f[10]); };BackgroundGifGump body(state,std::move(h));bool completed=true;std::uint32_t result{};
        try {
            if (f[1]=="construct") result=body.Construct(1,"title");else if (f[1]=="refresh") body.Refresh(1,"title");else if (f[1]=="setEvent") { const auto args=GifNumbers(f[8]);body.SetEvent(1,args[0],args[1],args[2]); }
            else if (f[1]=="handle") { const auto args=GifNumbers(f[8]);result=body.Handle(1,args[0],args[1],args[2]); }else if (f[1]=="draw") body.Draw(1);else if (f[1]=="destroy") body.Destruct(1);else { CHECK(f[1]=="reattach");body.Reattach(1); }
        } catch (const GifStop&) { completed=false; }
        std::string calls;for (const auto& value:trace) { if (!calls.empty()) calls+=';';calls+=value; } // 실제 장치 순서를 원문 관측과 대조한다.
        CHECK(completed==(f[13]=="1") && GifPack(GifFields(node))==f[14] && calls==f[15] && (completed ? result : 0)==GifNumber(f[16]));++rows;
    }
    CHECK(rows==308);
}
