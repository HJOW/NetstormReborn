// 원본 CPU의 실제32BYTE 문맥과 호출 순서를 제품 부모에 대조한다.
#include "TestSupport.h"
#include "client/ChatWindow.h"
#include "client/TraceChat.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 장치 입력/원시 관측만 보유하며 제품의 분기 결과를 기대값으로 만들지 않는다.
using namespace netstorm::client;
struct ChatAppendStop {}; // 선택한 원문 하위 진입에서만 중단하는 검사 신호다.
// TSV/문맥의 구분자만 읽고 빈 필드도 유지한다.
std::vector<std::string> ChatAppendSplit(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; // 앞선 필드 뒤 위치와 원시 필드 목록이다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? end : end-start));if (end==text.npos) break;start=end+1; } // 마지막 필드까지 순서대로 분리한다.
    return result;
}
// 주어진 원시 BYTE 전체를 fixture와 같은 소문자16진수로 표시한다.
std::string ChatAppendHex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 고정16진 글자 표와 전체 원시 출력이다.
    for (const auto value:bytes) { const auto byte=static_cast<unsigned char>(value);result+=digits[byte>>4];result+=digits[byte&15]; } // signed char의 상위 비트도 BYTE로 보존한다.
    return result;
}
//32BYTE 문맥을 원문의 little endian 순서로 변환한다.
std::string ChatAppendContext(const std::array<std::uint32_t,8>& context) {
    std::string bytes; //8DWORD의 원시32BYTE다.
    for (auto word:context) for (unsigned byte=0;byte<4;++byte) bytes+=static_cast<char>(word>>(byte*8)); // DWORD별 네BYTE를 그대로 기록한다.
    return ChatAppendHex(bytes);
}
// 앞쪽 입력 문자열만16진수에서 복원하며 기대관측은 입력에 사용하지 않는다.
std::string ChatAppendBytes(std::string_view hex) {
    std::string result; // NUL 뒤 바이트를 포함한 원문 입력이다.
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); // 두 글자씩 BYTE를 복원한다.
    return result;
}
}

// 현재 색/포인터 관문과 부모/두 자식의 문맥·alias·변경/부분중단을 전체 원문37입력과 대조한다.
TEST_CASE(TraceChat_Native37GateContextsCurrentChildScrollAndPartialEffects) {
    std::ifstream file(NETSTORM_TRACE_CHAT_FIXTURE);CHECK(file.good());std::string line;std::size_t cases{}; // 최초 원시 파일과 실제 재생 수다.
    while (std::getline(file,line)) { // 각 CPU 관측마다 별도 실제 노드와 제품 처리기를 만든다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=ChatAppendSplit(line,'\t');CHECK(fields.size()==14);
        HudChatNode root;ScrollingTextGumpNode first,second;ChatWindowState globals;std::array<std::uint32_t,8> supplied; // 서로 다른 부모/두자식/전달 문맥의 실제 물리다.
        for (std::size_t index=0;index<8;++index) { root.textContext[index]=static_cast<std::uint32_t>(100+index);first.textContext[index]=static_cast<std::uint32_t>(200+index);second.textContext[index]=static_cast<std::uint32_t>(300+index);supplied[index]=static_cast<std::uint32_t>(400+index); } // 원문 관측과 같은 구별값이다.
        root.text=10;std::string status="normal";std::vector<std::string> events; // 현재 자식 토큰과 원시 관측이다.
        const auto token=[](auto value) { CHECK(value==10 || value==20);return value==10 ? "1" : "2"; }; // fixture의 격리 주소를 의미 토큰으로 대조한다.
        const auto event=[&](std::string_view kind,std::uint32_t child,std::string_view text={}) {
            const auto& node=child==10 ? first : second;std::string row=std::string(kind)+':'+token(child)+':'+ChatAppendContext(root.textContext)+':'+ChatAppendContext(node.textContext); // 하위 진입 당시 실제 문맥이다.
            if (kind=="parse") row+=':'+ChatAppendHex(text.substr(0,text.find('\0')));events.push_back(std::move(row));
            if (fields[6]=="stop-"+std::string(kind)) { status="stop";throw ChatAppendStop{}; }
            if (fields[6]==std::string(kind)+"-child") root.text=20;
            if (fields[6]==std::string(kind)+"-context") for (std::size_t index=0;index<8;++index) { root.textContext[index]=static_cast<std::uint32_t>(700+index);first.textContext[index]=static_cast<std::uint32_t>(800+index); } // 명시 하위 변경이며 제품 후처리의 원문 재읽기를 확인한다.
        };
        ChatWindowHooks hooks;hooks.chat=[&](auto value) { return value==1 ? &root : nullptr; };hooks.base=[&](auto value)->GumpCreateNode* { return value==1 ? static_cast<GumpCreateNode*>(&root) : value==10 ? &first : value==20 ? &second : nullptr; };
        hooks.atBottom=[&](auto child) { event("at",child);return fields[5]!="0"; };hooks.appendLog=[&](auto child,auto text) { event("parse",child,text); };hooks.requestBottom=[&](auto child) { event("bottom",child); };ChatWindow window(globals,std::move(hooks));
        const auto text=ChatAppendBytes(fields[7]); // 원시 입력은 뒤의 기대 관측과 분리한다.
        try {
            if (fields[0]=="append") {
                std::span<const std::uint32_t> context;if (fields[4]=="supplied") context=supplied;else if (fields[4]=="root") context=root.textContext;else if (fields[4]=="child") context=first.textContext; // 실제32BYTE alias다.
                window.AppendLog(1,text,context);
            } else {
                TraceChatHooks trace;trace.color=[&] { events.emplace_back("color");return static_cast<std::uint32_t>(std::stoul(fields[2])); };trace.currentChat=[&] { events.emplace_back("current");return fields[3]=="0" ? 0U : 1U; };trace.append=[&](auto current,auto body,auto context) { window.AppendLog(current,body,context); };
                ForwardTraceToChat(static_cast<std::uint32_t>(std::stoul(fields[1])),text,trace);
            }
        } catch (const ChatAppendStop&) {}
        std::string trace; // 실제 호출 순서와 인자의 원시 비교 문자열이다.
        for (const auto& entry:events) { if (!trace.empty()) trace+=';';trace+=entry; } // 중복/재읽기 호출을 합치거나 정렬하지 않는다.
        if (trace.empty()) trace="-";
        CHECK(status==fields[8] && ChatAppendContext(root.textContext)==fields[9] && ChatAppendContext(first.textContext)==fields[10] && ChatAppendContext(second.textContext)==fields[11] && token(root.text)==fields[12] && trace==fields[13]);++cases;
    }
    CHECK(cases==37);
}

// 원문에서 추가 인자를 주지 않은 변환/스택 넘침을 무음 성공으로 바꾸지 않으며 NULL Chat도 먼저 서식을 처리한다.
TEST_CASE(TraceChat_AbsentGateNoDevicesAndDefinedFormatLimits) {
    ForwardTraceToChat(0,"%s",{});TraceChatHooks hooks;std::uint32_t calls{}; //0 관문은 미정의 format이나 장치를 조회하지 않는다.
    hooks.color=[&] { ++calls;return 9U; };hooks.currentChat=[&] { ++calls;return 0U; };
    ForwardTraceToChat(1,std::string(1023,'x'),hooks);CHECK(calls==2);bool rejected{}; //1023문자와 끝NUL은 원문1024BYTE 안이다.
    try { ForwardTraceToChat(1,std::string(1024,'x'),hooks); }catch (const std::length_error&) { rejected=true; }CHECK(rejected && calls==3);
    rejected=false;try { ForwardTraceToChat(1,"%s",hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && calls==4);
}
