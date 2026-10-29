// 일반 본문 조합 부모524행의 전체 manager/입력/지역 이름과 선택 중단을 원문에 대조한다.
#include "TestSupport.h"
#include "client/RawChatMessage.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 입력 복원과 실제 출력 직렬화만 수행하는 검사 부품이다.
using namespace netstorm::client;
constexpr std::uint32_t kFirst=0x12001000,kSecond=0x12004000,kRecipients=0x12007000; // 독립 원문 CPU의 같은 두 manager/배열 토큰이다.
struct MessageStop { std::string reason; }; // 명시 장치 중단의 원인 문자열이며 앞선 쓰기를 유지한다.
// 끝 빈 열도 보존하는 원시 TSV 분리다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 입력/출력의 모든 열을 같은 순서로 읽는다.
}
// 입력 DWORD를 같은 실제 네 바이트에 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); } // little endian 입력 구성이다.
// hex 원시 입력을 UTF8 해석 없이 복원한다.
std::string Input(std::string_view hex) {
    std::string result;if (hex=="-") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); // 두 문자를 한 BYTE로 읽는다.
    return result;
}
// 내부 NUL과 그 뒤 입력까지 그대로 쓰고 마지막 NUL만 추가한다.
void Store(std::span<std::uint8_t> bytes,std::size_t at,std::string_view text) { std::copy(text.begin(),text.end(),bytes.begin()+at);bytes[at+text.size()]=0; }
// 전체 바이트를 소문자 hex로 직렬화해 미작성 꼬리도 비교한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 원문 직렬화와 같은 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 어떤 실제 바이트도 생략하지 않는다.
    return result;
}
// 외부 장치의 C문자열 인자를 같은 hex 직렬화에 넘긴다.
std::string Hex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
}

// 원문 세 호출 부모/이름 getter와 각 장치 경계의 전체 쓰기·반환·외부 순서를 대조한다.
TEST_CASE(ChatMessage_IndependentNative524WholeMemoryAndCurrentReads) {
    std::ifstream file(NETSTORM_CHAT_MESSAGE_FIXTURE);CHECK(file.good());std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 제품 작성 전에 동결한 실제 원문 입력/출력을 한 행씩 대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto fields=Fields(line);CHECK(fields.size()==20);if (fields.size()!=20) continue;
        const auto& kind=fields[0];const auto spec=std::stoul(fields[1]),mask=std::stoul(fields[2]),direct=std::stoul(fields[3]),count=std::stoul(fields[4]);
        const auto receiver=fields[5]=="1" ? kSecond : kFirst;const auto source=Input(fields[6]),name=Input(fields[7]);const auto& mode=fields[8];const bool lookup=fields[9]=="1";
        std::array<std::uint8_t,2712> first,second;std::array<std::uint8_t,128> recipients{};std::array<std::uint8_t,256> input,local;input.fill(0xd3);local.fill(0xa5);
        for (std::size_t index=0;index<first.size();++index) { first[index]=static_cast<std::uint8_t>((0xa5+index*17)&255);second[index]=static_cast<std::uint8_t>((0xb0+index*17)&255); } // 같은 두 원문 manager 비영 입력을 구성한다.
        for (const auto memory:{std::span(first),std::span(second)}) { // 두 manager의 입력 필드는 원문과 같은 DWORD/문자열만 쓴다.
            Put(memory,0,static_cast<std::uint32_t>(count));Put(memory,4,32);Put(memory,8,kRecipients);Put(memory,12,static_cast<std::uint32_t>(direct));Put(memory,16,static_cast<std::uint32_t>(mask));Store(memory,0x14,"~rBACK");Store(memory,0x50,"~gFRONT");
        }
        if (count==1) Put(recipients,0,200);else if (count==2) { Put(recipients,0,100);Put(recipients,4,200); }Store(input,0,source);
        std::uint32_t current=fields[10]=="1" ? kFirst : 0U,subscriber=100,currentName=1,result{};std::vector<std::string> trace;bool localUsed{};ChatMessageHooks hooks;
        hooks.memory=[&](auto token) -> std::span<std::uint8_t> { if (token==kFirst) return first;if (token==kSecond) return second;if (token==kRecipients) return recipients;throw std::out_of_range("본문 검사 실제 토큰 없음"); };
        hooks.currentName=[&]() -> std::string_view { return currentName==2 ? "Changed" : std::string_view(name); };
        hooks.nameForSubscriber=[&](auto destination,auto target) {
            localUsed=true;trace.push_back("lookup:"+std::to_string(target));if (mode=="stopResolve") throw MessageStop{"resolve"};
            if (mode=="resolveSwitch") { currentName=2;subscriber=7;Put(hooks.memory(receiver),12,0);Store(first,0x14,"~pLateBack");Store(first,0x50,"~uLateFront");Store(input,0,"new~0body"); }
            Store(destination,0,lookup ? "Ta%s" : "SOMEONE");return 1U; // 실제00484410은 성공 여부 대신 비영 목적 포인터를 반환한다.
        };
        hooks.formatName=[&](auto destination,auto pattern,auto actualName,auto other) {
            localUsed=true;trace.push_back("format:"+Hex(pattern)+':'+Hex(actualName)+':'+Hex(other)+':'+std::to_string(destination.data()-first.data()));WriteChatNameFormat(destination,pattern,actualName,other);
            if (mode=="renderSwitch") { Put(hooks.memory(receiver),12,0);Put(first,16,32);Store(first,0x50,"~lRender");Store(input,0,"render~1changed"); }
            if (mode=="stopFormat") throw MessageStop{"format"};
        };
        hooks.stripSlots=[&](auto text) { trace.push_back("strip");StripChatFormatSlots(text); };
        hooks.deliver=[&](auto manager,auto target,auto body) {
            CHECK(manager==kFirst);trace.push_back("deliver:"+std::string(target==kSecond ? "2" : "1")+':'+std::to_string(body.size())+':'+Hex(body));if (mode=="stopDeliver") throw MessageStop{"deliver"};return 0x55aacc33U;
        };
        hooks.inputVisible=[&](auto visible) { trace.push_back("visible:"+std::to_string(visible)); };
        const ChatMessageGlobals globals{current,subscriber};std::string status="normal";
        try {
            if (kind=="format") { const std::array<std::optional<std::string_view>,4> patterns{"%s> ","%s ","%s (to %s)> ",std::nullopt};result=FormatChatMessage(kFirst,receiver,input,patterns.at(spec),local,globals,hooks); }
            else if (kind=="wrap") result=FormatChatInput(kFirst,input,local,globals,hooks);else { CHECK(kind=="say");result=SayChatMessage(input,local,globals,hooks); }
        } catch (const MessageStop& stop) { status=stop.reason; }
        std::string events;for (const auto& event:trace) { if (!events.empty()) events+=';';events+=event; } // 실제 장치 호출 순서를 원문과 같은 문자열로 만든다.
        CHECK(status==fields[11] && (status!="normal" || result==std::stoul(fields[12])) && currentName==std::stoul(fields[13]) && subscriber==std::stoul(fields[14]));
        CHECK(Hex(first)==fields[15] && Hex(second)==fields[16] && Hex(input)==fields[17] && (localUsed ? Hex(local) : "-")==fields[18] && (events.empty() ? "-" : events)==fields[19]);++rows;
    }
    CHECK(rows==524);
}
