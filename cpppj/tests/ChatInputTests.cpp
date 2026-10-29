// 원본 Enter 부모162행과 전환/조회/프레임말216행의 독립 관측을 제품에 대조한다.
#include "TestSupport.h"
#include "client/HudChatGeometry.h"
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace netstorm::client;
namespace { // 원문 fixture의 입출력 바이트와 명시 가상 장치만 제공한다.
std::vector<std::string> Fields(std::string_view text,char separator='\t') { //빈 마지막 필드도 보존한다.
    std::vector<std::string> out;std::size_t begin{}; //현재 필드 시작과 순서 그대로의 결과다.
    for (;;) { //분리 외에는 원시 문자열을 해석하지 않는다.
        const auto end=text.find(separator,begin);out.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return out;begin=end+1;
    }
}
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); } //원시 DWORD만 읽는다.
struct Scene { //root1/3과 Console2/4는 원문 장치와 같은 현재 포인터를 공유한다.
    HudChatNode first,second;std::array<GumpCreateNode,2> consoles;std::uint32_t current{},swap{};std::string trace; //가상 호출의 실제 변경/관측이다.
    GumpCreateNode* Node(std::uint32_t token) { return token==1 ? &first : token==3 ? &second : token==2 ? &consoles[0] : token==4 ? &consoles[1] : nullptr; } //같은 노드 주소만 반환한다.
    void Append(std::string_view value) { if (!trace.empty()) trace+=';';trace+=value; } //원문 경계 순서다.
    HudChatGeometryHooks Hooks() { //Push 뒤 현재 root 또는 현재 Console만 교체한다.
        HudChatGeometryHooks h;h.chat=[this](auto token) { return token==1 ? &first : token==3 ? &second : nullptr; };h.node=[this](auto token) { return Node(token); };h.current=[this] { return current; };
        h.push=[this](auto token) { Append("push:"+std::to_string(token));if (swap==1) current=3;else if (swap==2) first.console=4; };
        h.attach=[this](auto token,float x,float y,auto parent,auto mode) { Append("attach:"+std::to_string(token)+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(parent)+":"+std::to_string(mode)); };return h;
    }
};
}
//가상 Push의 재진입 변경과 원본의 다른 플래그/입력/루트 크기 보존을 대조한다.
TEST_CASE(ChatInput_Native216ToggleVisibleAndFrameRequest) {
    std::ifstream file(NETSTORM_CHAT_INPUT_TOGGLE_FIXTURE);CHECK(file.good());std::string line;std::size_t count{}; //독립 입력 행 수다.
    while (std::getline(file,line)) { //최초 관측 열의 입력과 결과를 바꾸지 않는다.
        if (line.starts_with("name\t")) continue;const auto f=Fields(line);CHECK(f.size()==16);if (f.size()!=16) continue;
        Scene s;s.current=Number(f[2]) ? 1U : 0U;s.first.console=2;s.second.console=4;s.swap=Number(f[5]);s.consoles[0].flags=Number(f[3]);s.consoles[1].flags=Number(f[4]);s.consoles[0].x=12;s.consoles[0].y=-14;s.consoles[1].x=14;s.consoles[1].y=-16;HudChatGeometry geometry(s.Hooks());
        std::int32_t request=std::bit_cast<std::int32_t>(Number(f[6]));std::uint32_t result{}; //signed 요청은 비트 그대로 보존한다.
        if (f[1]=="visible") result=geometry.InputVisible();else if (f[1]=="toggle") geometry.ToggleInputVisible();else { CHECK(f[1]=="frame");ConsumeChatInputToggle(request,[&] { geometry.ToggleInputVisible(); }); }
        CHECK(f[7]=="1" && f[14]=="0" && Number(f[15])>0);CHECK(s.consoles[0].flags==Number(f[8]) && s.consoles[1].flags==Number(f[9]));CHECK(s.current==Number(f[10]) && std::bit_cast<std::uint32_t>(request)==Number(f[11]));CHECK(result==Number(f[12]));CHECK((s.trace.empty() ? "-" : s.trace)==f[13]);++count;
    }
    CHECK(count==216);
}
//Guide/Gump와 모달의 외부 관문 뒤 제품 Enter의 효과만 원문 부모 관측과 비교한다.
TEST_CASE(ChatInput_Native162EnterGuideGumpModalReleaseAndPause) {
    std::ifstream file(NETSTORM_CHAT_INPUT_KEY_FIXTURE);CHECK(file.good());std::string line;std::size_t count{}; //부모 관측 행 수다.
    while (std::getline(file,line)) { //정지 상태는 표에 남기며 Enter 앞에 추가 관문을 만들지 않는다.
        if (line.starts_with("name\t")) continue;const auto f=Fields(line);CHECK(f.size()==18);if (f.size()!=18) continue;std::string expected,actual;
        for (const auto& item:Fields(f[15],';')) if (item=="visible" || item=="show:1" || item=="open") { if (!expected.empty()) expected+=';';expected+=item; } //명시 장치 세 개의 실제 호출만 추출한다.
        const auto append=[&](std::string_view item) { if (!actual.empty()) actual+=';';actual+=item; }; //제품 효과의 호출 순서다.
        if (!Number(f[2]) && !Number(f[3]) && !Number(f[5])) DispatchChatInputKey({Number(Fields(f[1],':')[0]),71,83},[&] { append("visible");return Number(f[9]); },[&] { append("show:1"); },[&] { append("open"); });
        CHECK(actual==expected);CHECK(f[10]=="1" && f[16]=="0" && Number(f[17])>0);++count;
    }
    CHECK(count==162);
}
//실패한 프레임 callback은 요청을 보존하고 성공한 한 번만0으로 소비한다.
TEST_CASE(ChatInput_FrameRequestExceptionAndNoRequest) {
    std::int32_t request{};ConsumeChatInputToggle(request,{});request=-1;bool failed{};
    try { ConsumeChatInputToggle(request,[] { throw std::runtime_error("명시 프레임 장치 실패"); }); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && request==-1);
    std::uint32_t calls{};ConsumeChatInputToggle(request,[&] { ++calls; });ConsumeChatInputToggle(request,[&] { ++calls; });CHECK(!request && calls==1);
}
