// 단독 자기 중계의 독립 원문132행과 같은 패킷의 부분효과를 대조한다.
#include "TestSupport.h"
#include "client/RawChatRelay.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace { // 원문 fixture 입출력 직렬화만 수행한다.
using namespace netstorm::client;
struct RelayStop { std::string reason; }; // 선택한 외부 장치에서 중단한 원인이며 선행 변경은 유지한다.
// 끝 빈 열도 유지하며 고정 TSV를 읽는다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> values;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);values.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return values;begin=end+1; } // 모든 열의 원문 순서를 보존한다.
}
// 명시 입력의 실제 비정렬 BYTE/WORD/DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); // 주변 꼬리는 보존한다.
}
// 원시 바이트를 같은 소문자 hex로 직렬화한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string text; // 독립 관측과 같은 숫자표다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; }return text; // 미작성 물리도 생략하지 않는다.
}
// 고정 hex 입력을 문자 인코딩 해석 없이 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view text) {
    std::vector<std::uint8_t> bytes;for (std::size_t at=0;at<text.size();at+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));return bytes; // 한 쌍을 한 BYTE로 읽는다.
}
}

// 원문 자기 경로만 실행하며 원격 Player/다른 연결 송신을 새로 검사하지 않는다.
TEST_CASE(ChatRelay_IndependentNative132WholePacketAndMask) {
    std::ifstream stream(NETSTORM_CHAT_RELAY_FIXTURE);CHECK(stream.good());std::string line;std::getline(stream,line);std::size_t rows{};
    while (std::getline(stream,line)) { // 제품보다 먼저 고정한 원문 전체 출력과 비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);CHECK(row.size()==9);if (row.size()!=9) continue;
        std::uint32_t battle=static_cast<std::uint32_t>(std::stoul(row[0])),mask=0xaabbccdd;const auto server=static_cast<std::uint32_t>(std::stoul(row[1])),count=static_cast<std::uint32_t>(std::stoul(row[2]));const auto body=Bytes(row[3]);const auto& mode=row[4];std::vector<std::string> trace;
        std::array<std::uint8_t,256> packet;packet.fill(0xd3);Put(packet,0,3,1);Put(packet,1,0x41,1);Put(packet,2,static_cast<std::uint32_t>(22+body.size()+count*4),2);Put(packet,5,1,1);Put(packet,6,100);Put(packet,10,7,2);Put(packet,12,static_cast<std::uint32_t>(body.size()),2);Put(packet,14,count,1);
        constexpr std::array<std::uint8_t,7> name{'L','o','c','a','l',0,0};std::copy(name.begin(),name.end(),packet.begin()+15);std::copy(body.begin(),body.end(),packet.begin()+22);
        for (std::uint32_t index=0;index<count;++index) Put(packet,22+body.size()+index*4,100); // 단독 자기 subscriber만 사용한다.
        ChatRelayHooks hooks;hooks.read=[&](auto at)->std::uint32_t { if (at==0x594fbc) return battle;if (at==0x540bc0) return server;if (at==0x540c74) return 100;throw std::logic_error("Chat 중계 검사 전역 없음"); };
        hooks.address=[&](auto sender) { trace.push_back("address:"+std::to_string(sender));std::array<std::uint8_t,20> address;for (std::size_t index=0;index<address.size();++index) address[index]=static_cast<std::uint8_t>(index); // 원문 명시 주소 장치와 같은 전체20BYTE다.
            if (mode=="stopAddress") throw RelayStop{"address"};if (mode=="addressBattleOff") battle=0;return address;
        };
        hooks.receive=[&](auto bytes,auto address) { trace.push_back("receive:"+Hex(bytes)+':'+Hex(address));if (mode=="stopReceive") throw RelayStop{"receive"}; };
        std::string status="normal";try { RelayChatPacket(9,packet,mask,hooks); }catch (const RelayStop& stop) { status=stop.reason; }std::string events;
        for (const auto& event:trace) { if (!events.empty()) events+=';';events+=event; } // 실제 주소/수신 효과 순서를 비교한다.
        CHECK(status==row[5] && mask==std::stoul(row[6]) && Hex(packet)==row[7] && (events.empty() ? "-" : events)==row[8]);++rows;
    }
    CHECK(rows==132);
}
