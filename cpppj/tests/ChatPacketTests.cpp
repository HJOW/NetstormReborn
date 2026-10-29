// 송신 부모/길이 observer의 독립 원문256행과 실제 소유자의 조합→추가→송신을 대조한다.
#include "TestSupport.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/RawChatMessage.h"
#include "client/RawChatPacket.h"
#include "client/RawChatSubmit.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 fixture 입출력과 실제 같은 물리 구성에만 쓰는 검사 부품이다.
using namespace netstorm::client;
constexpr std::uint32_t kManager=0x12001000,kRecipients=0x12004000; // 원문 CPU에서 사용한 manager와 수신자 배열 주소다.
struct PacketStop { std::string reason; }; // 선택한 장치의 중단 원인이며 앞선 쓰기를 되돌리지 않는다.
// 끝 빈 열도 유지하는 TSV 분리다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 모든 열을 원문 순서대로 읽는다.
}
// 원문 입력의 비정렬 BYTE/WORD/DWORD를 같은 little endian으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 지정한 폭 밖의 BYTE는 보존한다.
}
// 검사 출력의 실제 DWORD/WORD/BYTE를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    std::uint32_t result{};for (std::size_t index=0;index<width;++index) result|=std::uint32_t(bytes[at+index])<<(index*8);return result; // 같은 비정렬 폭을 조립한다.
}
// fixture hex는 인코딩 변환 없이 원시 BYTE로 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view hex) {
    std::vector<std::uint8_t> result;if (hex=="-") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16))); // 한 쌍을 한 BYTE로 쓴다.
    return result;
}
// 내부 NUL 뒤를 포함하는 전체 물리를 같은 소문자 hex로 출력한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 독립 관측과 동일한 숫자 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 미작성 꼬리도 생략하지 않는다.
}
// 명시 C문자열을 NUL까지 같은 실제 저장소에 쓴다.
void Store(std::span<std::uint8_t> bytes,std::size_t at,std::string_view text) { std::copy(text.begin(),text.end(),bytes.begin()+at);bytes[at+text.size()]=0; }
// 실제 장치 효과를 원문과 같은 순서의 한 열로 직렬화한다.
std::string Events(const std::vector<std::string>& trace) {
    std::string result;for (const auto& event:trace) { if (!result.empty()) result+=';';result+=event; }return result.empty() ? "-" : result; // 선택되지 않은 장치는 기록하지 않는다.
}
}

// 단독9/fallback 부모와 송수신 observer의 전체 물리·현재 전역·부분효과를 원문에 대조한다.
TEST_CASE(ChatPacket_IndependentNative256WholeMemoryAndObserver) {
    std::ifstream stream(NETSTORM_CHAT_PACKET_FIXTURE);CHECK(stream.good());std::string line;std::getline(stream,line);std::size_t rows{};
    while (std::getline(stream,line)) { // 제품보다 먼저 고정한 각 원문 입력/출력을 같은 메모리에 적용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);CHECK(row.size()==18);if (row.size()!=18) continue;
        const auto direct=static_cast<std::uint32_t>(std::stoul(row[2])),mask=static_cast<std::uint32_t>(std::stoul(row[3])),count=static_cast<std::uint32_t>(std::stoul(row[4]));
        std::uint32_t battle=static_cast<std::uint32_t>(std::stoul(row[1])),subscriber=100,result{};const auto& mode=row[8];const auto source=Bytes(row[6]),nameInput=Bytes(row[7]);
        std::array<std::uint8_t,2712> manager;std::array<std::uint8_t,1200> recipients;std::array<std::uint8_t,256> name{};recipients.fill(0xa5);std::vector<std::string> trace;std::vector<std::uint8_t> packet;
        for (std::size_t index=0;index<manager.size();++index) manager[index]=static_cast<std::uint8_t>((0xa5+index*17)&255); // 원문과 같은 미작성 manager 꼬리를 둔다.
        Put(manager,0,count);Put(manager,4,300);Put(manager,8,kRecipients);Put(manager,12,direct);Put(manager,16,mask);std::copy(nameInput.begin(),nameInput.end(),name.begin());
        for (std::uint32_t index=0;index<count;++index) Put(recipients,std::size_t(index)*4,row[5]=="1" && index==count/2 ? 100 : 200+index); // 중간 자기 항목과 실제 원문 순서를 재현한다.
        ChatPacketHooks hooks;hooks.memory=[&](auto token)->std::span<std::uint8_t> { if (token==kManager) return manager;if (token==kRecipients) return recipients;throw std::out_of_range("Chat 패킷 검사 물리 토큰 없음"); };
        hooks.currentName=[&]()->std::span<const std::uint8_t> { return name; };
        hooks.read=[&](auto at)->std::uint32_t { if (at==0x540c74) return subscriber;if (at==0x594fbc) return battle;if (at==0x594fb8 || at==0x594fc8 || at==0x544c5c) return 0;if (at==0x52edf6) return 0x41;throw std::out_of_range("Chat 패킷 검사 원문 전역 없음"); };
        hooks.addRecipient=[&](auto target,auto added) {
            CHECK(target==kManager);trace.push_back("add:"+std::to_string(added));if (mode=="stopAdd") throw PacketStop{"add"};
            const auto current=Read(manager,0);Put(recipients,std::size_t(current)*4,added);Put(manager,0,current+1);
            if (mode=="addSwitch") { subscriber=7;battle=1-battle;Put(manager,16,128);Put(manager,12,1);Store(name,0,"Modified"); }
        };
        hooks.send=[&](auto target,auto bytes) { CHECK(target==9);packet.assign(bytes.begin(),bytes.end());trace.push_back("send:9:"+Hex(bytes));if (mode=="stopSend") throw PacketStop{"send"}; };
        hooks.log=[&](auto channel,auto text) { trace.push_back("log:"+std::to_string(channel)+':'+Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}));if (mode=="stopLog") throw PacketStop{"log"}; };
        hooks.zeroAddress=[&] { trace.emplace_back("address:0.0.0.0:0"); };
        hooks.localReceive=[&](auto bytes) { packet.assign(bytes.begin(),bytes.end());trace.push_back("receive:"+Hex(bytes));if (mode=="stopReceive") throw PacketStop{"receive"}; };
        std::string status="normal";
        try {
            if (row[0]=="send") result=SendChatPacket(kManager,kManager,source,hooks);
            else { CHECK(row[0]=="observe");packet.assign(32,0xd3);Put(packet,10,direct,2);Put(packet,12,mask,2);Put(packet,14,count,1);ObserveChatPacket(packet,static_cast<std::uint32_t>(std::stoul(row[9])),[&](auto lineNumber) { trace.push_back("assert:"+std::to_string(lineNumber)); }); }
        }catch (const PacketStop& stop) { status=stop.reason; }
        CHECK(status==row[10] && (row[11]=="-" || result==std::stoul(row[11])) && subscriber==std::stoul(row[12]) && battle==std::stoul(row[13]));
        CHECK(Hex(manager)==row[14] && Hex(recipients)==row[15] && (packet.empty() ? "-" : Hex(packet))==row[16] && Events(trace)==row[17]);++rows;
    }
    CHECK(rows==256);
}

// 실제 본문 부모와 소유자의32→64 수신자 확장/반납·자기 중복·전송 observer를 함께 사용한다.
TEST_CASE(ChatPacket_ActualOwnerMessageGrowthAndWireLength) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();auto prefix=MakeClientChatManagerHooks(owner);const auto manager=owner->Allocate(netstorm::o::kChatManagerBytes);netstorm::o::ConstructChatManager(manager,prefix);owner->Current()=manager;
    Put(owner->Bytes(manager),12,0);Put(owner->Bytes(manager),16,32);owner->Bytes(manager)[0x14]=0;owner->Bytes(manager)[0x50]=0;auto add=MakeClientChatSubmitHooks(owner);
    for (std::uint32_t index=0;index<32;++index) AddChatRecipient(manager,200+index,add); // 초기32슬롯을 실제 추가 몸체로 채운다.
    const auto old=Read(owner->Bytes(manager),8);auto hooks=MakeClientChatPacketHooks(owner);const std::array<std::uint8_t,6> name{'L','o','c','a','l',0};std::vector<std::uint8_t> sent;std::uint32_t subscriber=100;
    hooks.currentName=[&]()->std::span<const std::uint8_t> { return name; };hooks.read=[&](auto at)->std::uint32_t { if (at==0x540c74) return subscriber;if (at==0x594fbc) return 1;if (at==0x52edf6) return 0x41;return 0; };
    hooks.send=[&](auto channel,auto bytes) { CHECK(channel==9 && Read(bytes,2,2)==4);ObserveChatPacket(bytes,1,{});sent.assign(bytes.begin(),bytes.end());CHECK(Read(bytes,2,2)==bytes.size()); };
    ChatMessageHooks message;message.memory=hooks.memory;message.currentName=[]()->std::string_view { return "Local"; };
    message.deliver=[&](auto output,auto targets,auto body) { return SendChatPacket(output,targets,body,hooks); };
    const std::array<std::uint8_t,6> input{'h','e','l','l','o',0};std::array<std::uint8_t,256> local{};
    CHECK(FormatChatInput(manager,input,local,{owner->Current(),subscriber},message)==1);CHECK(Read(owner->Bytes(manager),0)==33 && Read(owner->Bytes(manager),4)==64);
    bool released{};try { static_cast<void>(owner->Bytes(old)); }catch (const std::out_of_range&) { released=true; }CHECK(released);
    CHECK(sent.size()>21 && sent[0]==3 && sent[1]==0x41 && sent[4]==0xff && sent[5]==1 && sent[14]==33 && Read(sent,6)==100);
    const std::string expected="032~[Iicon.D41]Local> hello";const auto offset=15+Read(sent,10,2);CHECK(Read(sent,12,2)==expected.size());CHECK(std::equal(expected.begin(),expected.end(),sent.begin()+offset));
    CHECK(Read(sent,offset+expected.size()+32*4)==100);const auto expanded=Read(owner->Bytes(manager),8);CHECK(SendChatPacket(manager,manager,input,hooks)==1 && Read(owner->Bytes(manager),8)==expanded && Read(owner->Bytes(manager),0)==33);
}

// 전송 누락은 성공으로 반환하지 않으며 먼저 실행한 자기 추가는 실제로 유지된다.
TEST_CASE(ChatPacket_SelectedMissingSendRetainsRecipient) {
    auto owner=std::make_shared<ClientChatManagerGlobals>();auto prefix=MakeClientChatManagerHooks(owner);const auto manager=owner->Allocate(netstorm::o::kChatManagerBytes);netstorm::o::ConstructChatManager(manager,prefix);Put(owner->Bytes(manager),12,0);Put(owner->Bytes(manager),16,32);
    auto hooks=MakeClientChatPacketHooks(owner);const std::array<std::uint8_t,2> name{'N',0},body{'x',0};hooks.currentName=[&]()->std::span<const std::uint8_t> { return name; };
    hooks.read=[](auto at)->std::uint32_t { if (at==0x540c74) return 100;if (at==0x594fbc) return 1;if (at==0x52edf6) return 0x41;return 0; };bool missing{};
    try { SendChatPacket(manager,manager,body,hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing && Read(owner->Bytes(manager),0)==1 && Read(owner->Bytes(Read(owner->Bytes(manager),8)),0)==100);
}
