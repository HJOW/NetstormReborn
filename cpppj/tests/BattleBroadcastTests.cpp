// 실제 송신 분배의 독립 원본 입력과 상태 변경/NULL 연결 규칙을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBattleBroadcast.h"
#include <sstream>
using namespace netstorm::test::rawscene;
namespace {
// 원본 가상 송신은 명시 관찰 경계이며 같은 packet 인자와 후보 순서를 기록한다.
void CheckBroadcast(OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;const std::size_t stride=patch ? 0xb4U : 0xacU;
    std::vector<std::uint8_t> players(9*stride);BattlePacketPeers peers;std::uint32_t local{};bool server=true;
    RawBattleBroadcast logic(edition,players,peers,local,server);std::array<std::uint8_t,32> packet{};std::size_t count=0;
    // 명시 packet 바이트는 보내는 동안 변경되면 안 된다.
    for (std::size_t i=0;i<packet.size();++i) packet[i]=static_cast<std::uint8_t>(i);
    // 실제 PE의 새 판본별678개 입력을 동일 표/콜백 변경으로 재생한다.
    for (const auto& row:LoadFixture(NETSTORM_BATTLE_BCAST_FIXTURE).rows) {
        if (row[0]!=(patch ? "originals" : "originalCD")) continue;
        const auto active=std::stoul(row[2]),connected=std::stoul(row[3]);local=static_cast<std::uint32_t>(std::stoul(row[4]));
        const auto action=std::stoul(row[6]);std::vector<std::uint32_t> sent;
        // Player0과 bit0 외 비트는 원본과 같은 입력이다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            players[owner*stride+0x50]=static_cast<std::uint8_t>(0xfe|((active>>owner)&1));peers[owner]={};
            if (connected&(1U<<owner)) peers[owner]=[&,owner](std::span<const std::uint8_t> bytes) {
                CHECK(bytes.data()==packet.data() && bytes.size()==packet.size());sent.push_back(owner);
                if (sent.size()!=1) return;
                if (action==1) local=8;
                if (action==2 && owner<8) peers[owner+1]={};
                if (action==3 && owner<8) players[(owner+1)*stride+0x50]=0xfe;
            };
        }
        if (row[1]=="A") logic.Send(packet);else logic.SendExcept(std::stoi(row[5]),packet);
        std::ostringstream observed;
        // 실제 요청 순서를 보존하여 독립 원본 결과와 비교한다.
        for (std::size_t i=0;i<sent.size();++i) { if (i) observed<<',';observed<<sent[i]; }
        CHECK((sent.empty() ? "-" : observed.str())==row[7]);++count;
    }
    CHECK(count==678);
}
}
// PATCH의 현재 localOwner 재조회와 시작 시 제외 송신 localOwner 보관을 대조한다.
TEST_CASE(BattleBroadcast_PatchNative) { CheckBroadcast(OriginalEdition::Patch1078); }
// CD의 Player0xac 폭/동일+50 접속 BYTE와 실제 후보 순서를 대조한다.
TEST_CASE(BattleBroadcast_CdNative) { CheckBroadcast(OriginalEdition::Cd1072); }
// NULL 연결과 비접속은 실제 조건에서 생략하며 제외 송신의 서버 계약을 보존한다.
TEST_CASE(BattleBroadcast_EmptyPeersAndServerGuard) {
    std::vector<std::uint8_t> players(9*0xb4);BattlePacketPeers peers;std::uint32_t local=1;bool server=true;
    // 접속 플래그가 모두 켜져 있어도 NULL 연결에는 가상 송신을 호출하지 않는다.
    for (std::uint32_t owner=0;owner<9;++owner) players[owner*0xb4+0x50]=0xff;
    RawBattleBroadcast logic(OriginalEdition::Patch1078,players,peers,local,server);std::array<std::uint8_t,4> packet{1,2,3,4};
    logic.Send(packet);logic.SendExcept(2,packet);server=false;bool rejected=false;
    try { logic.SendExcept(2,packet); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);
}
// 실제 명령2/3 caller의 독립30입력과 전체20BYTE·미정 필드 보존을 대조한다.
TEST_CASE(BattleBroadcast_FlyerCommandNativePacket) {
    std::size_t count{}; // 원본 구간의 고유 입력 수이며 두 정밀도를 중복 합산하지 않는다.
    // 실제 원문 패킷은 제품 구현으로 만들거나 수정하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FLYER_COMMAND_FIXTURE).rows) {
        const auto input=Split(row[0],','); // SID/명령/기존 미정 바이트다.
        std::array<std::uint8_t,24> packet;packet.fill(static_cast<std::uint8_t>(Number(input[2]))); // caller의 기존 바이트와 뒤쪽 canary를 보존한다.
        WriteFlyerCommandPacket(packet,Sid{static_cast<std::uint16_t>(Number(input[0]))},Number(input[1]));
        CHECK(row[1]==row[2] && Hex(std::span<const std::uint8_t>(packet).first(20))==row[1]);
        // 원래20BYTE 밖도 쓰지 않는다.
        for (std::size_t i=20;i<packet.size();++i) CHECK(packet[i]==Number(input[2]));
        ++count;
    }
    CHECK(count==30);
}
// 너무 짧은 출력은 쓰기 전에 거부하고 실제 방송은 원격 연결에만 전달한다.
TEST_CASE(BattleBroadcast_FlyerCommandPeersAndCapacity) {
    std::array<std::uint8_t,19> shortPacket;shortPacket.fill(0xa5);bool rejected=false; // 손상 입력의 변경 전 계약이다.
    try { WriteFlyerCommandPacket(shortPacket,Sid{50},2); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);for (const auto byte:shortPacket) CHECK(byte==0xa5); // 거부 전에 일부 헤더를 쓰지 않는다.
    std::vector<std::uint8_t> players(9*0xb4);BattlePacketPeers peers;std::uint32_t local=1;bool server=true; // 실제 Player9·원격 표다.
    players[0xb4+0x50]=players[2*0xb4+0x50]=1;
    std::array<std::uint8_t,20> packet{};std::size_t localCalls{},remoteCalls{}; // 단독/연결 방송의 실제 효과 수다.
    peers[1]=[&](auto) { ++localCalls; };peers[2]=[&](auto bytes) { CHECK(bytes.size()==20 && bytes[1]==0x86 && bytes[4]==50 && bytes[12]==3);++remoteCalls; };
    WriteFlyerCommandPacket(packet,Sid{50},3);RawBattleBroadcast broadcast(OriginalEdition::Patch1078,players,peers,local,server);broadcast.Send(packet);
    CHECK(localCalls==0 && remoteCalls==1);
}
