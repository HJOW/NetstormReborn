// Factory 업그레이드 요청/수신의 원문·SharedRegular·비용·현재 전역 순서를 합성으로 검사한다. 작성·컴파일만 한다.
#include "o/RawFactoryUpgradeCommand.h"
#include "RawSceneSupport.h"
#include <limits>
#include <tuple>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 Factory 원문과 명시 비용/예약/전송 장치를 조립한다.
struct UpgradeInput {
    SidPool pool{OriginalEdition::Patch1078,32768,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //같은 PATCH 현재 원문과 TYPE다.
    Sid factory{};bool boss{true},checking{},reserve{true};double now{16777216.25}; //현재 부모/권한/검사/할당 결과/게임 시각이다.
    FactoryUpgradeCommandHooks hooks;std::vector<std::string> trace; //선택 효과와 실제 호출 순서다.
    std::vector<std::tuple<Sid,std::uint32_t,float>> timers;std::vector<std::pair<std::uint32_t,float>> money;std::vector<std::vector<std::uint8_t>> sent,broadcasts; //실제 전달된 인자/원문이다.
    // 원본 부모의 실제 BYTE/가상 표와 선택 효과를 준비한다.
    UpgradeInput() {
        factory=pool.Allocate(2);auto raw=pool.AllocatedBytes(factory);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x509148U);raw[10]=85;raw[34]=2;types[85].flags2=0x4000;
        hooks.cost=[this](auto type) { CHECK(type==85);trace.push_back("cost");return 500; };hooks.addSp=[this](auto owner,float delta) { trace.push_back("money");money.emplace_back(owner,delta); };hooks.reserveShared=[this] { trace.push_back("reserve");return reserve; };hooks.addShared=[this](Sid parent,auto event,float delay) { CHECK(parent==factory);trace.push_back("timer");timers.emplace_back(parent,event,delay); };hooks.send=[this](auto target,auto packet) { CHECK(target==9);trace.push_back("send");sent.emplace_back(packet.begin(),packet.end()); };hooks.broadcast=[this](auto packet) { trace.push_back("broadcast");broadcasts.emplace_back(packet.begin(),packet.end()); };hooks.log=[this](auto kind,std::string_view text) { CHECK(kind==4 && text=="new ftUPGRADE sharedRegPro\n");trace.push_back("log"); };hooks.assertion=[this](std::string_view text) { CHECK(text=="factory.getGenus() & gFACTORY");trace.push_back("assert"); };
    }
    // 구현 helper와 별도로 같은 wire 필드를 직접 쓴 독립 입력/기대 원문이다.
    std::array<std::uint8_t,16> Packet(double due) const { std::array<std::uint8_t,16> raw{};raw[0]=1;raw[1]=0x58;Put(raw,2,16,2);Put(raw,4,factory.value,2);raw[7]=1;const auto bits=std::bit_cast<std::uint64_t>(due);Put(raw,8,static_cast<std::uint32_t>(bits));Put(raw,12,static_cast<std::uint32_t>(bits>>32));return raw; }
    // 같은 현재 시각/권한/실제 슬롯을 빌리는 요청/수신 몸체다.
    RawFactoryUpgradeCommand Make() { return RawFactoryUpgradeCommand(pool,types,FactoryUpgradeCommandState{boss,checking,now},hooks); }
};

// 요청 대기20초와 wire 완료 시각15초를 구분하며 정확히16바이트를 송신한다.
TEST_CASE(FactoryUpgradeCommand_RequestWaitAndPacketDeadline) {
    UpgradeInput input;input.Make().Request(input.factory);const auto expected=input.Packet(input.now+15);CHECK(input.trace==std::vector<std::string>({"reserve","timer","send"}) && input.timers.size()==1 && std::get<1>(input.timers[0])==4 && std::get<2>(input.timers[0])==20.0F && input.sent.size()==1 && input.sent[0]==std::vector<std::uint8_t>(expected.begin(),expected.end()) && input.money.empty());
}
// 요청 할당 실패도 송신은 계속하고, 예약/부착 효과 뒤 현재 시각으로 메시지를 만든다.
TEST_CASE(FactoryUpgradeCommand_RequestAllocationFailureAndCurrentClock) {
    UpgradeInput missing;missing.reserve=false;missing.hooks.addShared={};missing.Make().Request(missing.factory);CHECK(missing.timers.empty() && missing.sent.size()==1 && missing.trace==std::vector<std::string>({"reserve","send"}));
    UpgradeInput changed;changed.hooks.addShared=[&](Sid parent,auto event,float delay) { changed.timers.emplace_back(parent,event,delay);changed.now+=3.25; };changed.Make().Request(changed.factory);const auto expected=changed.Packet(changed.now+15);CHECK(changed.sent[0]==std::vector<std::uint8_t>(expected.begin(),expected.end()));
}
// 수신은 실제 비용→로그→할당/서버event3→원문 방송이며 뒤의 호스트 버퍼는 방송하지 않는다.
TEST_CASE(FactoryUpgradeCommand_ReceiveCostTimerAndExactBroadcast) {
    UpgradeInput input;const auto first=input.Packet(input.now+15);std::vector<std::uint8_t> packet(first.begin(),first.end());packet.resize(24,0xa7);input.Make().Receive(packet);CHECK(input.trace==std::vector<std::string>({"cost","money","log","reserve","timer","broadcast"}) && input.money.size()==1 && input.money[0]==std::pair(2U,-500.0F) && input.timers.size()==1 && std::get<1>(input.timers[0])==3 && std::get<2>(input.timers[0])==15.0F && input.broadcasts.size()==1 && input.broadcasts[0]==std::vector<std::uint8_t>(first.begin(),first.end()));
}
// 실제 x87 test ah,41은 기한 전/같음/미래와 unordered를 구별하며 NaN을1초로 바꾸지 않는다.
TEST_CASE(FactoryUpgradeCommand_ReceiveDelayAndUnordered) {
    // 유한 시간의 최소1초와 NaN 전파를 각각 검사한다.
    for (double delta:{-1.0,0.0,1.0,2.25,std::numeric_limits<double>::quiet_NaN()}) { UpgradeInput input;input.Make().Receive(input.Packet(input.now+delta));CHECK(input.timers.size()==1);const auto actual=std::get<2>(input.timers[0]);if (std::isnan(delta)) CHECK(std::isnan(actual));else CHECK(actual==static_cast<float>(delta<1 ? 1 : delta)); }
}
// 상태/모드/genus 관문은 선택되지 않은 효과를 요구하지 않고 진단 뒤에는 현재 genus를 다시 읽는다.
TEST_CASE(FactoryUpgradeCommand_StateAndCheckingGates) {
    UpgradeInput dead;dead.pool.AllocatedBytes(dead.factory)[11]=3;dead.hooks={};dead.Make().Receive(dead.Packet(dead.now+15));CHECK(dead.trace.empty());UpgradeInput plain;plain.types[85].flags2=0;plain.hooks={};plain.Make().Receive(plain.Packet(plain.now+15));
    UpgradeInput checked;checked.checking=true;checked.types[85].flags2=0;checked.hooks.assertion=[&](std::string_view) { checked.trace.push_back("assert");checked.types[85].flags2=0x4000; };checked.Make().Receive(checked.Packet(checked.now+15));CHECK(checked.trace.front()=="assert" && checked.money.size()==1 && checked.timers.size()==1);
}
// 비용·할당·부착 효과 뒤의 현재 owner/시각/boss를 원본 위치에서 다시 읽는다.
TEST_CASE(FactoryUpgradeCommand_CurrentOwnerClockAndBossAfterChildren) {
    UpgradeInput input;input.hooks.cost=[&](auto type) { CHECK(type==85);input.pool.AllocatedBytes(input.factory)[34]=3;return 500; };input.hooks.reserveShared=[&] { input.now+=1;return true; };input.hooks.addShared=[&](Sid parent,auto event,float delay) { input.timers.emplace_back(parent,event,delay);input.boss=false; };const auto packet=input.Packet(input.now+15);input.Make().Receive(packet);CHECK(input.money[0]==std::pair(3U,-500.0F) && std::get<2>(input.timers[0])==14.0F && input.broadcasts.empty());
}
// 할당 실패는 비용/방송을 되돌리지 않으며 선택된 누락도 앞선 실제 효과를 보존한다.
TEST_CASE(FactoryUpgradeCommand_AllocationFailureAndSelectedMissingEffects) {
    UpgradeInput failed;failed.reserve=false;failed.hooks.addShared={};failed.Make().Receive(failed.Packet(failed.now+15));CHECK(failed.money.size()==1 && failed.timers.empty() && failed.broadcasts.size()==1);UpgradeInput missing;missing.hooks.reserveShared={};CHECK(Throws([&] { missing.Make().Receive(missing.Packet(missing.now+15)); }));CHECK(missing.money.size()==1 && missing.trace==std::vector<std::string>({"cost","money","log"}));UpgradeInput shortPacket;const auto packet=shortPacket.Packet(shortPacket.now+15);CHECK(Throws([&] { shortPacket.Make().Receive(std::span(packet).first(8)); }));CHECK(shortPacket.money.size()==1 && shortPacket.timers.empty());
}
// 같은58h 연구 부모는 kind0/mode1을 owner/sender 조건 없이 실제 수신으로 넘기며 미연결 호출은 거부한다.
TEST_CASE(FactoryUpgradeCommand_ResearchDispatchKeepsUpgradeBranch) {
    UpgradeInput input;auto command=input.Make();std::uint32_t loading{},dirty{},outpost{126};FactoryResearchHooks hooks;hooks.upgradeReceive=[&](auto packet) { command.Receive(packet); };RawFactoryResearch research(input.pool,input.types,FactoryResearchState{loading,dirty,input.boss,input.now,outpost},hooks);const auto packet=input.Packet(input.now+15);research.Receive(0xffffffffU,packet);CHECK(input.money.size()==1 && input.timers.size()==1);hooks.upgradeReceive={};RawFactoryResearch absent(input.pool,input.types,FactoryResearchState{loading,dirty,input.boss,input.now,outpost},hooks);CHECK(Throws([&] { absent.Receive(2,packet); }));
}
}
