// State의 실제 Chat 전체 행 정리와 기존 채널 flags 정리를 같은 Client 저장소에서 구별한다.
#include "TestSupport.h"
#include "client/RawChatChannels.h"
#include "client/ClientMain.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/CampaignClientServices.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::client;
// 원시 마지막 칸도 유지하며 TSV와 입력의 구분자를 같은 방식으로 분리한다.
std::vector<std::string> ChatClearFields(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t first{}; // 현재 칸의 시작 위치다.
    for (;;) { // 마지막 칸까지 원문 순서로 읽는다.
        const auto end=text.find(delimiter,first);result.emplace_back(text.substr(first,end==text.npos?text.npos:end-first));
        if (end==text.npos) return result;
        first=end+1;
    }
}
// 명시 입력의 한 DWORD만 쓰며 기록 선택이나 기대 결과를 계산하지 않는다.
void ChatClearPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 낮은 BYTE부터 원문 폭으로 쓴다.
}
// Native 도구와 같은 비영 초기 패턴/명시 점유 입력이다. 정리 기대는 별도 after 칼럼에서만 읽는다.
std::array<std::uint8_t,RawChatChannels::kBytes> ChatClearInput(std::uint32_t seed,std::string_view occupied) {
    std::array<std::uint8_t,RawChatChannels::kBytes> bytes; // 원문과 같은100×48BYTE 물리다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(seed+index*37); // 모든 stale BYTE에 독립 입력을 넣는다.
    for (std::size_t index=0;index<RawChatChannels::kCount;++index) ChatClearPut(bytes,index*48+44,0); // 점유 입력을 쓰기 전 각 Subscriber만0으로 둔다.
    if (occupied!="-") {
        for (const auto& item:ChatClearFields(occupied,',')) { // Native fixture의 명시 위치/ID만 복원한다.
            const auto colon=item.find(':');if (colon==item.npos) throw std::runtime_error("Chat 정리 점유 입력 구분자");
            const auto index=std::stoul(item.substr(0,colon));if (index>=RawChatChannels::kCount) throw std::out_of_range("Chat 정리 입력 행 범위");
            ChatClearPut(bytes,index*48+44,static_cast<std::uint32_t>(std::stoul(item.substr(colon+1),nullptr,16)));
        }
    }
    return bytes;
}
// 원문 전체4800BYTE 출력을 손실 없이 읽으며 제품 알고리즘을 기대 생성에 사용하지 않는다.
std::vector<std::uint8_t> ChatClearUnhex(std::string_view hex) {
    if (hex.size()!=RawChatChannels::kBytes*2) throw std::runtime_error("Chat 정리 원문 출력 폭");
    std::vector<std::uint8_t> result(hex.size()/2); // 실제 원문 출력 폭이다.
    for (std::size_t index=0;index<result.size();++index) result[index]=static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index*2,2)),nullptr,16)); // 두 글자를 한 BYTE로 읽는다.
    return result;
}
}

// 제품 구현 전에 고정한108입력/두 정밀도216관측의 전체 BYTE를 각각 비교한다.
TEST_CASE(ChatStateClear_ReplaysIndependent216NativeObservations) {
    std::ifstream input(NETSTORM_CHAT_STATE_CLEAR_FIXTURE);if (!input) throw std::runtime_error("Chat 정리 최초 fixture 없음");
    std::string line;std::size_t rows{},defaultPrecision{},extendedPrecision{};std::set<std::string> names; // 두 정밀도와 고유 입력을 따로 센다.
    while (std::getline(input,line)) { // 헤더 외 모든 최초 관측을 원시 순서로 대조한다.
        if (line.starts_with("id\t")) continue;
        if (!line.empty() && line.back()=='\r') line.pop_back();
        const auto fields=ChatClearFields(line,'\t');if (fields.size()!=7) throw std::runtime_error("Chat 정리 원문 칼럼 수");
        auto records=ChatClearInput(static_cast<std::uint32_t>(std::stoul(fields[2])),fields[3]);const auto expected=ChatClearUnhex(fields[4]); // 기대값은 독립 PE after뿐이다.
        RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,[]()->std::uint32_t { throw std::logic_error("Chat 정리가 현재 My를 조회함"); },
            [](auto,auto) { throw std::logic_error("Chat 정리가 원문 없는 이름/Refresh 효과를 호출함"); });
        channels.ResetOccupied();CHECK(std::equal(records.begin(),records.end(),expected.begin()));
        CHECK(std::stoul(fields[5])%12==0 && std::stoul(fields[6])>0); // Native는 점유 행마다12DWORD를 실제로 썼다.
        if (fields[1]=="027f") ++defaultPrecision;else { CHECK(fields[1]=="037f");++extendedPrecision; }
        names.insert(fields[0]);++rows;
    }
    CHECK(rows==216 && names.size()==108 && defaultPrecision==108 && extendedPrecision==108);
}

// 빈 행의 stale/padding과 경계는 보존하며 mask 정리로 전체 행 정리를 대신할 수 없다.
TEST_CASE(ChatStateClear_PreservesEmptyRowsAndGuardBytesWithoutEffects) {
    constexpr std::size_t guard=16; // 기록 양쪽의 별도 호스트 물리 경계다.
    std::array<std::uint8_t,RawChatChannels::kBytes+guard*2> physical;physical.fill(0xa7); // 원문 저장소 앞뒤의 비영 guard 입력이다.
    const auto source=ChatClearInput(19,"0:80000000,99:ffffffff");auto records=std::span(physical).subspan(guard,RawChatChannels::kBytes);std::copy(source.begin(),source.end(),records.begin());
    RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,[]()->std::uint32_t { throw std::logic_error("전체 Chat 정리의 불필요 My 조회"); },
        [](auto,auto) { throw std::logic_error("전체 Chat 정리의 불필요 통지"); });
    channels.Clear(0xff);CHECK(records[0]==source[0] && records[44]==0 && records[47]==0x80 && records[40]==0); // flags만 바뀌었으며 이름/높은 Subscriber 비트는 남는다.
    channels.ResetOccupied();const auto once=physical;channels.ResetOccupied();CHECK(physical==once); // 정상 정리 뒤 추가 호출은 빈 행을 보존한다.
    CHECK(std::all_of(records.begin(),records.begin()+48,[](auto byte) { return byte==0; }));
    CHECK(std::all_of(records.end()-48,records.end(),[](auto byte) { return byte==0; }));
    CHECK(std::equal(records.begin()+48,records.end()-48,source.begin()+48));
    CHECK(std::all_of(physical.begin(),physical.begin()+guard,[](auto byte) { return byte==0xa7; }));
    CHECK(std::all_of(physical.end()-guard,physical.end(),[](auto byte) { return byte==0xa7; }));
}

// 실제 Client의 같은 Chat 기록을 비우고 AI 두 소비자가 그 저장소의 첫 행을 다시 사용한다.
TEST_CASE(ChatStateClear_ActualClientReusesRecordsAndKeepsSubscriberOwners) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"; // Run/GUI 없이 보호 자산만 읽는다.
    Client client(std::move(options));const auto owner=client.SubscriberGlobals();owner->MySubscriber()=71;owner->LoginSubscriber()=81; // 두 DWORD는 Chat 행 반납 대상이 아니다.
    const auto* address=client.ChatRecords().data();CampaignAiServices first,second;BindCampaignAiClientServices(client,first);BindCampaignAiClientServices(client,second); // 두 binder가 같은 실제 Client를 빌린다.
    first.participation.addChat(77,"First",0x40);second.participation.addChat(88,"Second",0x80);
    CHECK(client.ChatRecords()[0]=='F' && client.ChatRecords()[48]=='S');
    client.ChatChannels().ResetOccupied();CHECK(client.ChatRecords().data()==address);
    CHECK(std::all_of(client.ChatRecords().begin(),client.ChatRecords().end(),[](auto byte) { return byte==0; }));
    second.participation.addChat(99,"Next",0x40);CHECK(client.ChatRecords()[0]=='N' && client.ChatRecords()[44]==99 && client.ChatRecords()[40]==0x40);
    CHECK(client.ChatRecords()[48]==0 && owner->MySubscriber()==71 && owner->LoginSubscriber()==81 && &client.CurrentSubscriber()==&owner->MySubscriber());
}
