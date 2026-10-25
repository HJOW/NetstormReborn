// 실제 Editor 단일 zlocal 큐와 기존 분배기의 순서·재진입·부분반납을 창 없이 검사한다.
#include "TestSupport.h"
#include "client/EditorLocalTransport.h"
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { //원문에서확인한고정헤더입력과외부효과관찰장치만제공한다.
constexpr std::array<std::uint8_t,5> kMessage{1,0x24,5,0,77}; //원본길이표의0x24/version1/length5에명시payload77을공급한다.
// 같은외부장치의발화와현재입력만보관하며기대게임상태를계산하지않는다.
struct EditorWireEffects {
    std::array<std::uint8_t,1620> players{};bool server{true}; //양끝이빌리는현재Player9/서버입력이며이검사에서로그인성공을주입하지않는다.
    std::uint32_t now{100},records{},closes{},releases{},terminations{}; //매호출시계와실제장치호출수다.
    std::vector<std::string> trace; //observer·기록·처리·수명효과의실제순서다.
    PacketDispatchHooks Hooks() { //게임판단을대신하지않고같은연결에대한외부장치효과만관찰한다.
        PacketDispatchHooks hooks;hooks.now=[this] { return ++now; };
        hooks.record=[this](auto& connection,auto packet,auto length) { CHECK(length==5 && packet[1]==0x24);++records;trace.push_back("record:"+std::to_string(connection.sender)); };
        hooks.terminate=[this](auto& connection) { ++terminations;connection.flags|=0x40;trace.emplace_back("terminate"); };
        hooks.close=[this](auto& connection) { CHECK(connection.depth==0 && (connection.flags&0x40));++closes;trace.emplace_back("close"); };
        hooks.release=[this](auto&) { ++releases;trace.emplace_back("release"); };return hooks;
    }
};
template<class Function> bool EditorWireRejects(Function operation) { //예상한계약위반만인정하고다른예외를숨기지않는다.
    try { operation(); }catch (const std::logic_error&) { return true; }return false;
}
void RegisterEditorMessage(EditorLocalTransport& wire,decltype(PacketRegistration::receive) receive) { //실제기존분배기의등록표에현재처리기만연결한다.
    wire.Dispatch().Version(0x24,1,0);wire.Dispatch().Register(0x24,std::move(receive));
}
}

//004e22c0은listener부터두번씩시각을조회하며로그인/Player쓰기없이client9만먼저존재한다.
TEST_CASE(EditorLocalTransport_ListenerClientAndNegativePendingWithoutLogin) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());const auto players=effects.players;
    CHECK(!wire.ListenerAlive() && !wire.ClientAlive() && !wire.PendingAlive());wire.Open(6799);
    CHECK(wire.ListenerAlive() && wire.ClientAlive() && !wire.PendingAlive() && wire.Port()==6799);
    CHECK(effects.now==104 && wire.Client().sender==9 && wire.Client().flags==0x24 && wire.Client().received==103 && wire.Client().heartbeat==104);
    std::vector<std::uint32_t> connected; //처리기1이실제분배된원문sender만관찰한다.
    wire.Dispatch().Register(1,[&](auto sender,auto& connection,auto packet) { CHECK(connection.depth==1 && !(connection.flags&0x20) && packet[0]==0xff && packet[1]==1);connected.push_back(sender); });
    CHECK(wire.Poll()==0 && wire.PendingAlive() && effects.now==108);
    CHECK(wire.Pending().sender==0xffffffffU && wire.Pending().flags==4 && wire.Client().flags==4 && wire.Pending().received==107 && wire.Pending().heartbeat==106);
    CHECK(connected==std::vector<std::uint32_t>({0xffffffffU,9}) && effects.players==players && effects.records==0 && wire.QueuedFrames()==0);
    CHECK(wire.Poll()==0 && connected.size()==2); //다음폴링은type1/수락/초기시각을반복하지않는다.
}

//unbuffered송신도동기수신하지않고pending은한번만방문하며그응답은같은호출의등록9에서소비한다.
TEST_CASE(EditorLocalTransport_QueuedRequestReplyAndPendingPollOrder) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());std::uint32_t requests{},replies{};wire.Open(6801);
    wire.Dispatch().Observe(0x24,[&](auto sending) { effects.trace.push_back(sending ? "send" : "socket"); });
    RegisterEditorMessage(wire,[&](auto sender,auto& connection,auto packet) {
        CHECK(connection.depth==1 && packet[4]==77);
        if (sender==0xffffffffU) { ++requests;effects.trace.emplace_back("request");CHECK(wire.Send(-1,kMessage)); }
        else { CHECK(sender==9);++replies;effects.trace.emplace_back("reply"); }
    });
    CHECK(!wire.Send(-1,kMessage) && !wire.Send(1,kMessage)); //수락전pending과등록되지않은Player끝을성공시키지않는다.
    CHECK(wire.Send(9,kMessage) && wire.Send(9,kMessage) && wire.QueuedFrames()==2 && !requests && !replies);
    CHECK(effects.trace==std::vector<std::string>({"send","send"}));
    CHECK(wire.Poll()==2 && requests==1 && replies==1 && wire.QueuedFrames()==1);
    CHECK(effects.trace==std::vector<std::string>({"send","send","socket","record:4294967295","request","send","socket","record:9","reply"}));
    CHECK(wire.Poll()==2 && requests==2 && replies==2 && wire.QueuedFrames()==0 && effects.records==4);
    CHECK(wire.Pending().depth==0 && wire.Client().depth==0 && wire.Pending().sender==0xffffffffU);
}

//처리기내종료는depth1에서표식만쓰고기존분배기의최외곽복귀가각장치를한번씩반납한다.
TEST_CASE(EditorLocalTransport_CloseDuringReceiveAndReentrantPoll) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);bool nestedRejected{};
    RegisterEditorMessage(wire,[&](auto sender,auto& connection,auto) {
        CHECK(sender==0xffffffffU && connection.depth==1);nestedRejected=EditorWireRejects([&] { wire.Poll(); });
        wire.Disconnect(-1);CHECK(wire.PendingAlive() && effects.closes==0 && (connection.flags&0x40));
    });
    CHECK(wire.Send(9,kMessage) && wire.Poll()==1 && nestedRejected);
    CHECK(!wire.PendingAlive() && wire.ClientAlive() && effects.closes==1 && effects.releases==1 && !wire.Send(-1,kMessage));
    wire.Disconnect(-1);CHECK(effects.closes==1);wire.Disconnect(9);wire.CloseListener();
    CHECK(!wire.ListenerAlive() && !wire.ClientAlive() && effects.closes==3 && effects.releases==3 && wire.Poll()==0);
    wire.Open(6799);CHECK(wire.ClientAlive() && !wire.PendingAlive() && wire.Poll()==0 && wire.PendingAlive());
}

//버전정책은기존분배기가판정하며종료앞기록/receive를추가하지않고다음가상폴링에서닫힌끝을반납한다.
TEST_CASE(EditorLocalTransport_VersionTerminationUsesExistingDispatcher) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);std::uint32_t receives{};
    RegisterEditorMessage(wire,[&](auto,auto&,auto) { ++receives; });wire.Dispatch().Version(0x24,2,8);
    CHECK(wire.Send(9,kMessage) && wire.Poll()==1 && effects.terminations==1 && effects.records==0 && !receives);
    CHECK(wire.PendingAlive() && (wire.Pending().flags&0x40));CHECK(wire.Poll()==0 && !wire.PendingAlive() && effects.closes==1 && effects.releases==1);
}

//이미소비한요청/송신과같은연결필드는상위처리기예외에서도남고호스트폴링guard만복구된다.
TEST_CASE(EditorLocalTransport_HandlerFailurePreservesPartialQueue) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);
    RegisterEditorMessage(wire,[&](auto,auto& connection,auto) { CHECK(connection.depth==1);CHECK(wire.Send(-1,kMessage));throw std::logic_error("실제상위처리실패입력"); });
    CHECK(wire.Send(9,kMessage) && EditorWireRejects([&] { wire.Poll(); }));
    CHECK(wire.PendingAlive() && wire.ClientAlive() && wire.Pending().depth==0 && wire.QueuedFrames()==1 && effects.records==1);
    std::uint32_t reply{};RegisterEditorMessage(wire,[&](auto sender,auto&,auto) { CHECK(sender==9);++reply; });
    CHECK(wire.Poll()==1 && reply==1 && wire.QueuedFrames()==0); //실패뒤다음폴링이이미송신한실제응답을소비한다.
}

//실제포트/새수명/물리헤더와필수장치만검사하며이검사도Login등록이나GUI를만들지않는다.
TEST_CASE(EditorLocalTransport_SelectedContractsAndOwnedBytes) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());
    CHECK(EditorWireRejects([&] { wire.Open(0); }) && EditorWireRejects([&] { wire.Open(65536); }));wire.Open(6799);
    CHECK(EditorWireRejects([&] { wire.Open(6799); }) && EditorWireRejects([&] { wire.Send(-33,kMessage); }));
    std::array<std::uint8_t,6> packet{1,0x24,5,0,77,99};std::uint32_t received{};
    RegisterEditorMessage(wire,[&](auto sender,auto&,auto bytes) { CHECK(sender==0xffffffffU && bytes.size()==5 && bytes[4]==77);++received; });
    CHECK(wire.Send(9,packet));packet[4]=10;CHECK(wire.Poll()==1 && received==1); //선언길이만소유하므로운영중caller버퍼변경과stale마지막BYTE가섞이지않는다.
    packet[2]=7;CHECK(EditorWireRejects([&] { wire.Send(9,packet); }) && wire.QueuedFrames()==0);
    auto missing=effects.Hooks();missing.now={};CHECK(EditorWireRejects([&] { EditorLocalTransport invalid(effects.players,effects.server,missing); }));
}

//상대socket소멸은type2를한번분배할뿐이며server의등록9를임의로삭제하지않는다.
TEST_CASE(EditorLocalTransport_PeerCloseDispatchesTerminationOnce) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);CHECK(wire.Poll()==0);
    std::uint32_t notices{};wire.Dispatch().Register(2,[&](auto sender,auto& connection,auto packet) { CHECK(sender==9 && packet[0]==0xff && packet[1]==2 && connection.depth==1);++notices; });
    wire.Disconnect(-1);CHECK(!wire.PendingAlive() && wire.ClientAlive());CHECK(wire.Poll()==0 && notices==1 && (wire.Client().flags&0x10));
    CHECK(wire.Poll()==0 && notices==1 && !wire.Send(9,kMessage) && notices==1 && wire.QueuedFrames()==0);
    CHECK(wire.ClientAlive() && effects.closes==1 && effects.releases==1);wire.Disconnect(9);CHECK(effects.closes==2 && effects.releases==2);
}

//무효헤더의소비수와길이0sentinel을구별하여뒤의정상client9프레임을다음폴링에남긴다.
TEST_CASE(EditorLocalTransport_InvalidClientPacketStopsCurrentEndpointPoll) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);CHECK(wire.Poll()==0);
    std::uint32_t receives{},socketObservers{};RegisterEditorMessage(wire,[&](auto sender,auto&,auto) { CHECK(sender==9);++receives; });
    wire.Dispatch().Observe(0x24,[&](auto sending) { if (!sending) ++socketObservers; });
    const std::array<std::uint8_t,4> invalid{1,0x24,4,0};CHECK(wire.Send(-1,invalid) && wire.Send(-1,kMessage));
    CHECK(wire.Poll()==1 && wire.QueuedFrames()==1 && !receives && !socketObservers && !effects.records);
    CHECK(wire.Poll()==1 && wire.QueuedFrames()==0 && receives==1 && socketObservers==1 && effects.records==1);
}

//client9버전종료의길이0sentinel은같은폴링의재호출·조기Retire를막고다음폴링에서반납한다.
TEST_CASE(EditorLocalTransport_ClientVersionSentinelDefersRetirement) {
    EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);CHECK(wire.Poll()==0);
    RegisterEditorMessage(wire,[](auto,auto&,auto) { CHECK(false); });wire.Dispatch().Version(0x24,2,8);
    CHECK(wire.Send(-1,kMessage) && wire.Poll()==1 && effects.terminations==1 && wire.ClientAlive());
    CHECK((wire.Client().flags&0x40) && !effects.closes && !effects.releases && !effects.records);
    CHECK(wire.Poll()==0 && !wire.ClientAlive() && effects.closes==1 && effects.releases==1);
}

//정상수신protocol1뒤합성종료는같은끝의BYTE0을보존하여type2의현재버전정책도통과한다.
TEST_CASE(EditorLocalTransport_TerminationPreservesCurrentReceiveProtocolForBothEnds) {
    for (const bool receivingClient:std::array<bool,2>{false,true}) { //negative pending과등록9의서로다른현재수신버퍼를각각검사한다.
        EditorWireEffects effects;EditorLocalTransport wire(effects.players,effects.server,effects.Hooks());wire.Open(6799);CHECK(wire.Poll()==0);
        const std::uint32_t expectedSender=receivingClient ? 9U : 0xffffffffU;std::uint32_t received{},terminated{}; //실제수신끝의원문sender와발화수다.
        RegisterEditorMessage(wire,[&](auto sender,auto&,auto packet) { CHECK(sender==expectedSender && packet[0]==1);++received; });
        wire.Dispatch().Version(2,1,4); //stale ff를재주입하면높은버전127로판정해종료장치를호출하는구별입력이다.
        wire.Dispatch().Register(2,[&](auto sender,auto& connection,auto packet) { CHECK(sender==expectedSender && packet.size()==4 && packet[0]==1 && packet[1]==2 && packet[2]==4 && packet[3]==0 && connection.depth==1);++terminated; });
        CHECK(wire.Send(receivingClient ? -1 : 9,kMessage) && wire.Poll()==1 && received==1);
        wire.Disconnect(receivingClient ? -1 : 9);CHECK(wire.Poll()==0 && terminated==1 && effects.terminations==0);
        CHECK(wire.Poll()==0 && terminated==1 && effects.records==1); //type2는한번만직접분배되며일반패킷기록은추가하지않는다.
    }
}
