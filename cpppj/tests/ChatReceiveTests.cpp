// 수신 부모·현재 Chat wrapper의 원문238행과 실제 장치 누락의 부분효과를 대조한다.
#include "TestSupport.h"
#include "client/RawChatReceive.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // fixture 입력과 실제 출력의 직렬화만 수행하는 검사 부품이다.
using namespace netstorm::client;
constexpr std::uint32_t kChat=0x12008000; // 원문 수신의 동일한 Chat 주소 별칭이다.
constexpr std::array<std::string_view,10> kKeys{"ShowBattle","ShowEveryone","ShowAllies","ShowObserver","ShowWisper","ShowMyAllies","ShowMyEmenys","ShowMyNeutrals","ShowChatWindow","PlaySoundMessage"}; // 독립 입력 비트와 설정 장치를 잇는 원문 키 순서다.
struct ReceiveStop { std::string reason; }; // 해당 장치 뒤에는 실행하지 않되 선행 패킷 변경은 유지한다.
// 마지막 빈 열도 유지하며 독립 TSV를 읽는다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 실제 열 순서를 바꾸지 않는다.
}
// 원문 비정렬 필드의 입력을 지정 폭으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); // 주변 BYTE는 유지한다.
}
// 원시 입력은 UTF8/CP1252 해석 없이 같은 BYTE로 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view hex) {
    std::vector<std::uint8_t> result;for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));return result; // 한 쌍씩 읽는다.
}
// 미작성 꼬리와 내부 NUL도 모두 소문자 hex로 비교한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 원문 도구와 같은 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 실제 전체 물리를 출력한다.
}
// 문자열 장치의 원문 바이트도 같은 hex로 직렬화한다.
std::string Hex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 같은8DWORD를 원문 little endian32BYTE 문맥으로 변환한다.
std::string ContextHex(std::span<const std::uint32_t> context) {
    std::array<std::uint8_t,32> bytes{};CHECK(context.size()==8);
    for (std::size_t index=0;index<context.size();++index) Put(bytes,index*4,context[index]);return Hex(bytes); // 각 현재DWORD의 비트를 유지한다.
}
// 실제 외부 호출 순서를 고정 fixture의 한 열로 만든다.
std::string Events(const std::vector<std::string>& trace) {
    std::string result;for (const auto& event:trace) { if (!result.empty()) result+=';';result+=event; }return result.empty() ? "-" : result; // 빈 효과는 별도 기호다.
}
}

// 버전/ASCII·무시·채널/관계 옵션과 현재값 변경·중단을 원문 전체 패킷과 함께 비교한다.
TEST_CASE(ChatReceive_IndependentNative238WholePacketAndEffects) {
    std::ifstream stream(NETSTORM_CHAT_RECEIVE_FIXTURE);CHECK(stream.good());std::string line;std::getline(stream,line);std::size_t rows{};
    while (std::getline(stream,line)) { // 제품보다 먼저 고정한 원문 입력/출력을 순서대로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);CHECK(row.size()==18);if (row.size()!=18) continue;
        const auto number=[&](auto index) { return static_cast<std::uint32_t>(std::stoul(row[index])); }; // fixture의 명시 입력 DWORD를 읽는다.
        std::uint32_t battle=number(3),subscriber=100,color=44;const auto mask=number(4),sender=number(5),flags=number(6),allied=number(7),neutral=number(8);const bool ignored=number(9)!=0;const auto& mode=row[10];
        const auto source=Bytes(row[11]);constexpr std::string_view name="Sender";std::array<std::uint8_t,256> packet;packet.fill(0xd3);std::vector<std::string> trace;
        Put(packet,0,number(1),1);Put(packet,1,0x41,1);Put(packet,2,static_cast<std::uint32_t>(15+name.size()+1+source.size()+4),2);Put(packet,5,number(2),1);Put(packet,6,sender);Put(packet,10,7,2);Put(packet,12,static_cast<std::uint32_t>(source.size()),2);Put(packet,14,1,1);
        std::copy(name.begin(),name.end(),packet.begin()+15);packet[21]=0;std::copy(source.begin(),source.end(),packet.begin()+22);Put(packet,22+source.size(),100);
        ChatReceiveHooks hooks;hooks.read=[&](auto at)->std::uint32_t {
            if (at==0x540c74) return subscriber;if (at==0x594fbc) return battle;if (at==0x594fc8) return 0;if (at==0x548710) return mask;if (at==0x548748) return 7;if (at==0x540c70) return 2;if (at==0x5ca92c) return 0x1234;if (at==0x5b5dd8) return color;
            if (at==0x548740) return row[0]=="noRoot" ? 0 : kChat;if (at==0x595200+(2*9+3)*4) return allied;if (at==0x52f81e+3) return neutral;throw std::out_of_range("Chat 수신 검사 전역 없음");
        };
        hooks.ignored=[&](auto actual) { trace.push_back("ignored:"+std::to_string(actual));return ignored; };
        hooks.config=[&](auto key) { trace.push_back("config:"+std::string(key));if (mode=="stopConfig") throw ReceiveStop{"config"};if (mode=="configBattleOff" && key==kKeys.back()) battle=0;const auto found=std::find(kKeys.begin(),kKeys.end(),key);CHECK(found!=kKeys.end());return std::int32_t((flags&(1U<<(found-kKeys.begin())))!=0); };
        hooks.owner=[&](auto actual) { trace.push_back("owner:"+std::to_string(actual));return 3U; };
        hooks.wake=[&](auto target) { trace.push_back("wake:"+std::to_string(target));if (mode=="wakeSwitch") { subscriber=sender;color=99; }if (mode=="stopWake") throw ReceiveStop{"wake"}; };
        hooks.sound=[&](auto text) { trace.push_back("sound:"+std::string(text)); };
        hooks.append=[&](auto target,auto text,auto context) { CHECK(target==kChat);trace.push_back("append:"+Hex(text)+':'+ContextHex(context));if (mode=="stopAppend") throw ReceiveStop{"append"}; };
        hooks.show=[&] { trace.emplace_back("show");if (mode=="stopShow") throw ReceiveStop{"show"}; };
        hooks.log=[&](auto channel,auto text) { trace.push_back("log:"+std::to_string(channel)+':'+Hex(text)); };
        std::string status="normal";try { if (row[0]=="direct") ReceiveChatPacket(kChat,packet,hooks);else DispatchChatPacket(packet,hooks); }catch (const ReceiveStop& stop) { status=stop.reason; }
        CHECK(status==row[12] && Hex(packet)==row[13] && row[14]=="7" && subscriber==number(15) && battle==number(16) && Events(trace)==row[17]);++rows;
    }
    CHECK(rows==238);
}

// 현재 Chat0의 wrapper와 선택한 append 누락 때의 선행NUL 변경을 구별한다.
TEST_CASE(ChatReceive_CurrentZeroAndMissingAppendKeepEarlierEffects) {
    ChatReceiveHooks hooks;hooks.read=[](auto at) { CHECK(at==0x548740);return 0U; };DispatchChatPacket({},hooks);
    std::array<std::uint8_t,30> packet{};packet[0]=3;packet[5]=1;Put(packet,6,100);Put(packet,10,2,2);Put(packet,12,4,2);packet[15]='N';packet[17]='h';packet[18]='i';packet[19]='!';packet[20]='\n';
    hooks.read=[](auto at)->std::uint32_t { if (at==0x540c74) return 100;return 0; };hooks.ignored=[](auto) { return false; };std::size_t wakes{};hooks.wake=[&](auto) { ++wakes; };bool missing{};
    try { ReceiveChatPacket(kChat,packet,hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing && wakes==1 && packet[20]==0 && packet[19]=='!');
}
