// 보호 PATCH의 실제 Chat Add/Clear와100기록전체/하위진입순서를 독립 관찰 fixture에 대조한다.
#include "RawSceneSupport.h"
#include "client/RawChatChannels.h"
#include "client/CampaignClientServices.h"
#include "client/ClientMain.h"
#include <optional>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// native 관찰과 같은 명시 초기 입력만 만든다. 기록 선택이나 기대 결과는 계산하지 않는다.
std::array<std::uint8_t,RawChatChannels::kBytes> ChatInput(std::uint32_t scene) {
    std::array<std::uint8_t,RawChatChannels::kBytes> bytes{};
    // 원본이 건드리지 않아야 하는 stale BYTE를 전체 저장소에 먼저 넣는다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*37+scene*11);
    // 각 입력 기록의 이름/식별자/flags를 native 준비와 같은 위치에 쓴다.
    for (std::size_t index=0;index<RawChatChannels::kCount;++index) {
        auto record=std::span(bytes).subspan(index*RawChatChannels::kStride,RawChatChannels::kStride);
        std::fill_n(record.begin(),40,static_cast<std::uint8_t>(65+index%26));
        constexpr std::string_view name="OldName"; // 모든 기록에 넣는 독립 입력 이름이다.
        std::copy(name.begin(),name.end(),record.begin());record[7]=0;
        record[40]=static_cast<std::uint8_t>(index*13+0x95);Put(record,44,scene==0 || scene==5 ? 0U : static_cast<std::uint32_t>(1000+index));
    }
    if (scene==1) Put(bytes,7*48+44,77);
    if (scene==2) { Put(bytes,2*48+44,0);Put(bytes,7*48+44,77); }
    if (scene==3) Put(bytes,99*48+44,0);
    if (scene==4) { Put(bytes,7*48+44,77);Put(bytes,98*48+44,77); }
    if (scene==5) Put(bytes,44,0xffffffffU);
    return bytes;
}
// native 문자열 입력 hex를 원본 바이트로 읽는다. UTF-8 재해석을 하지 않는다.
std::string ChatUnhex(std::string_view hex) {
    std::string result;
    // 두 hex글자가 한 BYTE다.
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));
    return result;
}
// 원본 전체4800바이트 관찰의 변경 위치를 초기 입력에 적용한다. 나머지 모든 바이트도 그대로 비교한다.
void ApplyChatObservation(std::span<std::uint8_t> expected,std::string_view changes) {
    if (changes=="-") return;
    // fixture는 손실 없는 offset:byte이며 임의 체크섬 충돌을 동일성 근거로 사용하지 않는다.
    for (const auto& change:Split(changes,',')) {
        const auto colon=change.find(':');if (colon==std::string::npos) throw std::runtime_error("Chat native 변경 위치 형식 오류");
        const auto offset=std::stoul(change.substr(0,colon));if (offset>=expected.size()) throw std::out_of_range("Chat native 변경 위치 범위 오류");
        expected[offset]=static_cast<std::uint8_t>(std::stoul(change.substr(colon+1),nullptr,16));
    }
}
}
// 자기/0/-1/음수 식별자·만료flags·이름경계·기존/신규/가득찬100기록을 실제 PE 출력과 비교한다.
TEST_CASE(ChatChannels_Native2448) {
    std::size_t compared=0;
    // 기대 출력은 C++ 동작으로 계산하지 않고 독립 native fixture에서 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_CHAT_CHANNELS_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==9);
        auto records=ChatInput(Number(row[1])),expected=records;std::uint32_t current=Number(row[3]);std::string trace;
        RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,[&] { return current; },
            [&](ChatChannelCall call,std::uint32_t subscriber) {
                if (!trace.empty()) trace+=';';trace+=call==ChatChannelCall::NameChanged ? "N,"+std::to_string(subscriber) : "R";
            });
        if (row[0]=="A") {
            const auto bytes=row[5]=="-" ? std::string{} : ChatUnhex(row[5]);
            const auto name=row[5]=="-" ? std::optional<std::string_view>{} : std::optional<std::string_view>{bytes};
            CHECK(channels.Add(Number(row[2]),name,Number(row[4]))==std::stoi(row[6]));
        } else if (row[0]=="C") channels.Clear(Number(row[4]));else throw std::runtime_error("Chat native 입력 종류 오류");
        ApplyChatObservation(expected,row[7]);CHECK(records==expected);CHECK((trace.empty() ? "-" : trace)==row[8]);++compared;
    }
    CHECK(compared==2448);
}
// 같은 클라이언트의 현재 subscriber 변경과 원본 자기 반환·이름정제/두refresh 시점을 확인한다.
TEST_CASE(ChatChannels_CurrentSubscriberAndEffectOrder) {
    auto records=ChatInput(0);std::uint32_t current=77;std::size_t reads=0;std::vector<ChatChannelCall> calls;
    RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,[&] { ++reads;return current; },
        [&](ChatChannelCall call,std::uint32_t subscriber) {
            calls.push_back(call);
            if (call==ChatChannelCall::NameChanged) {
                CHECK(subscriber==77 && records[40]==0x40 && records[0]=='A' && records[1]==' ');
            } else CHECK(records[0]=='A' && records[1]=='B' && records[2]==0);
        });
    const auto before=records;CHECK(channels.AddToChannel(77,"A B",0x40)==-1);CHECK(records==before && calls.empty() && reads==1);
    current=1;CHECK(channels.AddToChannel(77,"A B",0x140)==0);CHECK(reads==2);
    CHECK(calls==std::vector<ChatChannelCall>({ChatChannelCall::NameChanged,ChatChannelCall::Refresh,ChatChannelCall::Refresh}));
    channels.Clear(0x140);CHECK(records[40]==0 && calls.size()==3);
}
// 판본/물리 저장소/현재getter 누락은 생성 시 거부하며 자기 경로에는 별도 UI 소비자가 필요하지 않다.
TEST_CASE(ChatChannels_StorageAndEditionContracts) {
    auto records=ChatInput(0);
    CHECK(Throws([&] { RawChatChannels channels(netstorm::o::OriginalEdition::Cd1072,records,[] { return 1U; }); }));
    CHECK(Throws([&] { RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,std::span(records).first(records.size()-1),[] { return 1U; }); }));
    CHECK(Throws([&] { RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,{}); }));
    RawChatChannels channels(netstorm::o::OriginalEdition::Patch1078,records,[] { return 1U; });
    const auto before=records;CHECK(channels.Add(1,std::nullopt,0xffffffffU)==-1);CHECK(records==before);
}
// 실제 Client는 Run 없이 장치/창을 만들지 않으며 여러 AI 서비스가 같은 장기 기록에 직접 쓴다.
TEST_CASE(ChatChannels_ClientLifetimeAndSharedAiBinding) {
    ClientOptions options; // 보호 아카이브는 읽기 전용 등록만 하며 실제 게임/GUI를 시작하지 않는다.
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    Client client(std::move(options));
    CHECK(client.Menu()==nullptr && client.CurrentSubscriber()==0xffffffffU);
    const auto records=client.ChatRecords(); // 이 주소는 각 binder가 생성한 서비스보다 오래 유지되는 실제4800바이트다.
    CHECK(records.size()==RawChatChannels::kBytes);
    CHECK(std::all_of(records.begin(),records.end(),[](std::uint8_t byte) { return byte==0; }));
    CampaignAiServices first;BindCampaignAiClientServices(client,first);
    first.participation.addChat(client.CurrentSubscriber(),"현재자기",0x40); // 미설치 subscriber-1의 자연 반환은 기록을 쓰지 않는다.
    CHECK(std::all_of(records.begin(),records.end(),[](std::uint8_t byte) { return byte==0; }));
    first.participation.addChat(77,"A B",0x140);
    CHECK(records[0]=='A' && records[1]=='B' && records[2]==0 && records[0x28]==0x40);
    CHECK(Get(records,0x2c)==77);
    CampaignAiServices second;BindCampaignAiClientServices(client,second);
    CHECK(client.ChatRecords().data()==records.data());
    second.participation.addChat(77,"Changed",0x80);CHECK(records[0x28]==0xc0 && Get(records,0x2c)==77);
    first.participation.clearChat(0x140);CHECK(records[0x28]==0x80 && Get(records,0x2c)==77);
    second.participation.addChat(88,"Next",0x40);CHECK(Get(records,RawChatChannels::kStride+0x2c)==88);
    CHECK(client.ChatRecords().data()==records.data() && client.CurrentSubscriber()==0xffffffffU);
    CHECK(!first.allocationBytes && !first.changeColor && !first.localTransport);
    CHECK(!first.participation.assignConnection && !first.participation.rejectLogin && !first.participation.disconnect);
    CHECK(!first.participation.sendAlliance && !first.participation.destructBrain && !first.participation.freeBrain);
}
// 실제 언어 Config의 변경/치환을 요청마다 읽고 독일어를 UTF-8이 아닌 원본 CP1252 바이트로 전달한다.
TEST_CASE(ChatChannels_ClientDefaultAiNameUsesCurrentPathSpec) {
    ClientOptions options; // Run을 호출하지 않아 장치·설정 저장·게임 실행은 없다.
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    Client client(std::move(options));CampaignAiServices ai;BindCampaignAiClientServices(client,ai);
    client.Configuration().Configuration().Assign("[END]\r\n"); // Run의 Startup 대신 실제 전역 Config 버퍼를 설치해 원본 Get의 널 관문을 통과한다.
    client.Configuration().Language().Assign("defaultAiName = \"{aiTitle}\"\naiTitle = \"Fürst des Bösen\"\n");
    CHECK(ai.defaultName()==std::string("F\xfcrst des B\xf6sen"));
    client.Configuration().Language().Assign("defaultAiName = \"Evil Duke\"\n");
    CHECK(ai.defaultName()=="Evil Duke");
    client.Configuration().Set("defaultAiName","{liveAiName}");client.Configuration().Set("liveAiName","Changed Name");
    CHECK(ai.defaultName()=="Changed Name");
}
