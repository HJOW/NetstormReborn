// 실제 서버 writer/metadata의 전체 Player·wire bytes·효과 순서를 독립 PE와 비교한다.
#include "RawSceneSupport.h"
#include "o/RawMissionServerPackets.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::size_t kServerCases=187; // 독립 명시 입력의 고유 개수다.
// NUL을 포함하지 않는 fixture hex를 실제 입력 BYTE로 읽는다.
std::vector<std::uint8_t> ServerUnhex(std::string_view hex) {
    std::vector<std::uint8_t> result;
    // 두 hex글자가 하나의 원본 BYTE다.
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));return result;
}
// 원본에 공급한 명시 Player9 입력만 만들며 송신/정제 결과를 계산하지 않는다.
std::array<std::uint8_t,1620> ServerInput(std::uint32_t owner,std::span<const std::uint8_t> name,std::uint32_t flags) {
    std::array<std::uint8_t,1620> bytes{};
    // 전체 stale BYTE를 native와 같은 패턴으로 채운다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*37+11);
    const auto offset=owner*180; // 원본 Player의 같은 실제 물리 시작 위치다.
    std::fill_n(bytes.begin()+offset+12,32,std::uint8_t{});std::copy(name.begin(),name.end(),bytes.begin()+offset+12);bytes[offset+12+name.size()]=0;
    Put(bytes,offset+0x2c,0x12345680+owner);Put(bytes,offset+0x50,flags);return bytes;
}
// 내부 실제 이름 정제 진입은 제품의 외부 효과가 아니므로 외부 trace 비교에서만 제외한다.
std::string ServerExternalTrace(std::string_view native) {
    std::string result;
    // Player/packet 결과는 별도로 전체 비교하며 내부함수 진입 통계는 native report에 보존한다.
    for (const auto& event:Split(native,';')) if (!event.starts_with("N,")) { if (!result.empty()) result+=';';result+=event; }return result.empty() ? "-" : result;
}
}
// 원본187행의 전체 Player 원문/현재 전역/현재 wire와 외부 하위 호출 순서를 전수 대조한다.
TEST_CASE(MissionServerPackets_Native187) {
    std::size_t count=0;
    // 각 행에서 새 실제 저장소를 빌려 이름 정제/송신 효과가 다음 행에 누적되지 않게 한다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_SERVER_PACKETS_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==4);const auto fields=Split(row[0],',');CHECK(fields.size()==10);
        const auto owner=Number(fields[1]),flags=Number(fields[3]);const auto name=fields[2]=="-" ? std::vector<std::uint8_t>{} : ServerUnhex(fields[2]);auto players=ServerInput(owner,name,flags);
        bool server=Number(fields[4])!=0,metamap=Number(fields[5])!=0;std::uint32_t local=1,created=Number(fields[7]),archipelago=Number(fields[9]);std::array<std::uint8_t,6> zone{};Put(zone,0,Number(fields[8]));
        std::string trace; // 외부 진단/주소/로그/송신의 실제 요청 순서다.
        const auto effect=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 기대 결과를 계산하지 않는 단순 외부 관찰자다.
        MissionServerPacketHooks hooks;
        hooks.assertion=[&](std::string_view text) { effect("A,"+Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()))); };
        hooks.log=[&](std::string_view text) { effect("L,"+Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()))); };
        hooks.peerAddress=[&](std::uint32_t player) { effect("IP,"+std::to_string(player));return std::string("127.0.0.1"); };
        hooks.send=[&](std::int32_t target,std::span<const std::uint8_t> bytes) { effect("S,"+std::to_string(std::bit_cast<std::uint32_t>(target))+","+Hex(bytes)); };
        RawMissionServerPackets packets(players,{server,metamap,local,created,archipelago,zone},std::move(hooks));
        const auto target=std::bit_cast<std::int32_t>(Number(fields[6])); // 원본 signed destination의 DWORD 비트를 보존한다.
        if (fields[0]=="P") packets.SendPlayer(owner,target);
        else if (fields[0]=="M") packets.SendMetadata(target);
        else { std::array<std::uint8_t,32> bytes;bytes.fill(0xff);std::copy(name.begin(),name.end(),bytes.begin()+10);bytes[10+name.size()]=0;packets.ReceiveMetadata(bytes); }
        CHECK(Hex(players)==row[1]);CHECK(std::to_string(server ? 1 : 0)+","+std::to_string(local)+","+std::to_string(created)+","+std::to_string(Get(zone,0))+","+std::to_string(archipelago)==row[2]);
        if ((trace.empty() ? "-" : trace)!=ServerExternalTrace(row[3])) throw std::runtime_error("서버 패킷 native 행 "+std::to_string(count)+" 현재 "+trace+" 기대 "+ServerExternalTrace(row[3]));++count;
    }
    CHECK(count==kServerCases);
}
// 원본이 실제 도달한 누락과 미복원 중앙섬 생성은 빈 성공으로 바꾸지 않는다.
TEST_CASE(MissionServerPackets_SelectedBoundariesAndRegistration) {
    auto players=ServerInput(1,{},1);bool server=true,metamap=false;std::uint32_t local=1,created=1,archipelago=0;std::array<std::uint8_t,6> zone{};MissionServerPacketHooks hooks;
    RawMissionServerPackets packets(players,{server,metamap,local,created,archipelago,zone},hooks);CHECK(Throws([&] { packets.SendPlayer(1,2); }));
    CHECK(players[180+12]=='C'); // 실제 이름 정제는 도달한 주소 경계 누락 이전에 이미 수행된다.
    packets.SendMetadata(1); // 이미 생성한 자기 metadata는 실제 원문처럼 어떤 소비자도 호출하지 않는다.
    Put(zone,0,0x100);std::array<std::uint8_t,11> central{};central[10]='$';CHECK(Throws([&] { packets.ReceiveMetadata(central); }));
    PacketDispatchHooks dispatchHooks;dispatchHooks.now=[] { return 0U; };dispatchHooks.record=[](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) {};
    dispatchHooks.terminate=dispatchHooks.close=dispatchHooks.release=[](PacketConnection&) { throw std::logic_error("등록만 검사하며 연결 종료를 선택하지 않는다"); };
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,dispatchHooks);RegisterMissionMetadata(dispatch,packets);CHECK(dispatch.Registration(0x72).version==1 && bool(dispatch.Registration(0x72).receive) && bool(dispatch.Registration(0x72).observer));CHECK(Throws([&] { dispatch.Registration(0x72).observer(1); }));
}
