// 실제 원본25바이트 요청과 서버444af0의 모든 관찰 입력/재조회 순서를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawConstructionCommand.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 관측의 hex 바이트를 기대 동작 계산 없이 읽는다.
std::vector<std::uint8_t> DecodeCommand(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 한 바이트에 해당하는 두 글자를 차례로 복원한다.
    for (std::size_t i=0;i<text.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(i,2)),nullptr,16)));return result;
}
// 원본 경계와 같은 순서로 실제 C++가 호출한 하위 인자를 기록한다.
void CommandEvent(std::string& text,std::string_view label,const ConstructionConfirmRequest& value,bool whole) {
    if (!text.empty()) text+=';';text+=label;
    const auto append=[&](std::uint32_t item) { text+=':';text+=std::to_string(item); }; // 한 원본 DWORD 인자다.
    append(value.type);
    if (!whole) append(value.player);
    append(std::bit_cast<std::uint32_t>(value.x));append(std::bit_cast<std::uint32_t>(value.y));
    if (whole) append(value.player);
    append(value.argument);append(value.direction);
    if (whole) { append(value.flags);append(value.timeLow);append(value.timeHigh);append(value.quality); }
}
}
// 로컬 배치의 실제 생성자/본문/길이와 호출자 뒤 버퍼 보존을 대조한다.
TEST_CASE(ConstructionCommand_WritesNative64Packets) {
    std::size_t count=0;
    // 두 정밀도에서 같았던 모든 원본 송신 버퍼를 직접 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_CONSTRUCTION_COMMAND_WRITE).rows) {
        if (!row.empty() && row[0]=="type") continue; // 원본 관측의 열 이름은 입력 행과 구별한다.
        CHECK(row.size()==7);ConstructionLocalMessage message;message.type=static_cast<std::uint8_t>(Number(row[0]));message.x=std::bit_cast<float>(Number(row[1]));message.y=std::bit_cast<float>(Number(row[2]));message.argument=static_cast<std::uint16_t>(Number(row[3]));message.direction=static_cast<std::uint8_t>(Number(row[4]));message.quality=static_cast<std::uint8_t>(Number(row[5]));std::array<std::uint8_t,32> output;output.fill(0xa5);
        CHECK(WriteConstructionCommand(message,output)==25);CHECK(Hex(std::span(output).first(25))==row[6]);CHECK(std::all_of(output.begin()+25,output.end(),[](auto byte) { return byte==0xa5; }));++count;
    }
    CHECK(count==64);
}
// 차단/로컬/부족/NaN/반환값과 경계 뒤 패킷/비용 변경을 원본 기록과 대조한다.
TEST_CASE(ConstructionCommand_ReplaysNative144Receives) {
    std::size_t count=0;
    // 원본은 Money/Request 뒤 같은 패킷 필드를 다시 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_CONSTRUCTION_COMMAND_RECEIVE).rows) {
        if (!row.empty() && row[0]=="blocked") continue; // 원본 관측의 열 이름은 입력 행과 구별한다.
        CHECK(row.size()==9);std::vector<RiftTypeRecord> types(188);ConstructionCommandState state{Number(row[0]),Number(row[1])!=0,Number(row[3])};types[100].cost=std::bit_cast<float>(Number(row[5]));auto packet=DecodeCommand("013e1900640000a4410000aa41efbefa80efcdab890000f87f");std::string events;const auto mutation=Number(row[7]);ConstructionCommandHooks hooks;
        hooks.money=[&](std::uint32_t owner) { if (!events.empty()) events+=';';events+="M:"+std::to_string(owner);if (mutation==1) { types[100].cost=1;packet[4]=101;Put(packet,5,0xc0500000); }return std::bit_cast<float>(Number(row[4])); };
        hooks.shortfall=[&](const ConstructionConfirmRequest& value) { CommandEvent(events,"S",value,false); };
        hooks.request=[&](const ConstructionConfirmRequest& value) { CommandEvent(events,"Q",value,true);if (mutation==2) { packet[4]=101;Put(packet,9,0xc0200000);Put(packet,13,0x1234,2);packet[15]=9; }return Number(row[6])!=0; };
        hooks.reject=[&](const ConstructionConfirmRequest& value) { CommandEvent(events,"R",value,false); };
        RawConstructionCommand command(types,state,std::move(hooks));command.Receive(Number(row[2]),packet);std::string expected;
        // C++의 직접 TYPE 조회에는 별도 외부 호출이 없으므로 그 경계 표식만 제외한다.
        for (const auto& event:Split(row[8],';')) if (event!="-" && !event.starts_with("T:")) { if (!expected.empty()) expected+=';';expected+=event; }
        CHECK(events==expected);++count;
    }
    CHECK(count==144);
}
// 현재 패킷 분배기의 등록·길이·버전·sender DWORD와 준비 순서를 같은 수신 몸체로 확인한다.
TEST_CASE(ConstructionCommand_RegistersActualDispatch) {
    std::vector<RiftTypeRecord> types(188);ConstructionCommandState state;std::size_t prepared{},requests{};PacketDispatchHooks device;device.now=[] { return 123U; };device.record=[](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) {};device.terminate=[](PacketConnection&) { throw std::logic_error("정책0은 종료하지 않는다"); };device.close=[](PacketConnection&) {};device.release=[](PacketConnection&) {};
    ConstructionCommandHooks hooks;hooks.money=[](std::uint32_t) { return 0.0f; };hooks.shortfall=[](const ConstructionConfirmRequest&) { throw std::logic_error("비용0은 부족하지 않다"); };hooks.reject=[](const ConstructionConfirmRequest&) { throw std::logic_error("성공 요청의 취소 금지"); };hooks.request=[&](const ConstructionConfirmRequest& value) { CHECK(value.player==0xffffffffU && value.type==100 && value.flags==0 && value.timeLow==0 && value.timeHigh==0);++requests;return true; };
    RawConstructionCommand body(types,state,std::move(hooks));RawPacketDispatch dispatch(OriginalEdition::Patch1078,std::move(device));RegisterConstructionCommand(dispatch,body,[&] { ++prepared; });const auto& registration=dispatch.Registration(0x3e);CHECK(registration.version==1 && registration.mismatch==0 && registration.length==25 && registration.observer);std::array<std::uint8_t,25> packet{};ConstructionLocalMessage message;message.type=100;WriteConstructionCommand(message,packet);PacketConnection connection;connection.sender=0xffffffffU;
    packet[2]=24;CHECK(dispatch.Dispatch(connection,packet));CHECK(!prepared && !requests);packet[2]=25;packet[0]=0x81;CHECK(dispatch.Dispatch(connection,packet));CHECK(prepared==1 && requests==1 && connection.received==123 && connection.depth==0);
}

// 준비 시점 사본 대신 실제 비용 관문에서 현재 전역을 읽고 음수 sender와 선행 TYPE/차단 순서를 보존한다.
TEST_CASE(ConstructionCommand_CurrentChargeGatePreservesNativeReadOrder) {
    std::vector<RiftTypeRecord> types(188);types[100].cost=100.0f;ConstructionCommandState state;bool current=true;std::uint32_t reads{},money{},requests{},shortfalls{};
    state.chargeBlockedRead=[&] { ++reads;return current; };ConstructionCommandHooks hooks;hooks.money=[&](auto sender) { CHECK(sender==0xffffffffU);++money;return 0.0f; };hooks.shortfall=[&](const auto& request) { CHECK(request.player==0xffffffffU);++shortfalls; };hooks.request=[&](const auto& request) { CHECK(request.player==0xffffffffU || request.player==1);++requests;return true; };hooks.reject=[](const auto&) { throw std::logic_error("성공 요청의 예상 밖 환불"); };
    RawConstructionCommand body(types,state,std::move(hooks));std::array<std::uint8_t,25> packet{};ConstructionLocalMessage message;message.type=100;WriteConstructionCommand(message,packet);
    body.Receive(0xffffffffU,packet);CHECK(reads==1 && !money && requests==1 && !shortfalls && !state.chargeBlocked);
    current=false;body.Receive(0xffffffffU,packet);CHECK(reads==2 && money==1 && requests==1 && shortfalls==1);
    body.Receive(1,packet);CHECK(reads==3 && money==1 && requests==2); // 명령 수신은 로컬 sender 비교보다 먼저 전역을 읽는다.
    packet[4]=255;bool invalid{};try { body.Receive(0xffffffffU,packet); } catch (const std::out_of_range&) { invalid=true; }CHECK(invalid && reads==3);
    state.blocked=1;body.Receive(0xffffffffU,{});CHECK(reads==3); // 첫 차단은 본문과 현재 전역 조회 모두를 자연 생략한다.
}
