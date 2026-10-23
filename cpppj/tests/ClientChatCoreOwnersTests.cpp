// 실제Client에서시작한장기Core/Chat소유권만검사한다.Run/Session자동접합완료를주장하지않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientRandomGlobals.h"
#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>
using namespace netstorm::client;
namespace {
// 기존headlessClient검사와같이보호archive를읽기만하며창/장치/옵션파일쓰기를실행하지않는다.
ClientOptions ChatCoreClientOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options; }
}
// 새Client의한번생성/고정getter와설정변경뒤동일소유권을확인한다.
TEST_CASE(ClientChatCoreOwners_ActualClientStableOwnersWithoutStartup) {
    Client client(ChatCoreClientOptions());const auto core=client.CoreDataGlobals();const auto chat=client.ChatManagerGlobals();CHECK(core && chat && client.CoreDataGlobals()==core && client.ChatManagerGlobals()==chat);CHECK(core->Current().empty() && core->Players().empty() && !core->Active() && !chat->Current());CHECK(std::all_of(core->Cached().begin(),core->Cached().end(),[](auto value) { return value==0; }));
    const auto token=chat->Allocate(netstorm::o::kChatManagerBytes);auto bytes=chat->Bytes(token);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x5a});chat->Current()=token;core->Active()=7;const auto address=bytes.data();const auto random=client.RandomGlobals()->Game().State();client.Configuration().SetInt("readOnlyForts",1);
    CHECK(client.CoreDataGlobals()==core && client.ChatManagerGlobals()==chat && core->Active()==7 && chat->Current()==token && chat->Bytes(token).data()==address && chat->Bytes(token)[13]==0x5a && client.RandomGlobals()->Game().State()==random);CHECK(&std::as_const(*core).Active()==&core->Active() && &std::as_const(*chat).Current()==&chat->Current()); //미작성할당을읽지않고명시fill한물리만검증한다.
}
// 실제NativeSelectCore0→cache→Player반납계약을Client의같은현재슬롯으로실행한다.
TEST_CASE(ClientChatCoreOwners_SelectZeroCachesBeforeBorrowedPlayerRelease) {
    Client client(ChatCoreClientOptions());const auto core=client.CoreDataGlobals();std::vector<std::uint8_t> players(1620);std::array<std::uint8_t,10> expected{};
    for (std::size_t index=0;index<expected.size();++index) { expected[index]=static_cast<std::uint8_t>(index*17+3);players[180+0x6c+index]=expected[index]; } //원문현재Core가읽을명시10BYTE입력이다.
    netstorm::o::PlayerWorldState player;std::uint32_t metamap{};core->Players()=players;core->Select(player,metamap,1,1);core->Active()=1;CHECK(core->Current().data()==players.data()+180+0x6c);
    player.playerKnown=0;core->Select(player,metamap,0,1);CHECK(core->Current().empty() && !core->Active() && player.battleOwner==0 && player.templateOwner==1 && !player.playerKnown);CHECK(std::equal(expected.begin(),expected.end(),core->Cached().begin()));
    core->Players()={};std::vector<std::uint8_t>().swap(players);CHECK(core->Current().empty() && core->Players().empty() && std::equal(expected.begin(),expected.end(),core->Cached().begin())); //테스트가빌린Player를먼저해제한것이아니다.실제Session자동해제는후속이다.
}
// Client반납뒤남은명시장치/공유사용자는같은할당을보유하고다음새Client의BSS초기값과분리된다.
TEST_CASE(ClientChatCoreOwners_RetainedHooksAndNextClientAreSeparate) {
    std::shared_ptr<ClientCoreDataGlobals> core;std::weak_ptr<ClientCoreDataGlobals> weakCore;std::weak_ptr<ClientChatManagerGlobals> weakChat;netstorm::o::ChatManagerPrefixHooks hooks;std::uint32_t token{};const std::uint8_t* address{};
    { Client client(ChatCoreClientOptions());core=client.CoreDataGlobals();weakCore=core;weakChat=client.ChatManagerGlobals();core->Cached()[2]=91;token=client.ChatManagerGlobals()->Allocate(netstorm::o::kChatManagerBufferBytes);auto bytes=client.ChatManagerGlobals()->Bytes(token);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x6b});address=bytes.data();hooks=MakeClientChatManagerHooks(client.ChatManagerGlobals()); }
    CHECK(!weakCore.expired() && !weakChat.expired() && core->Cached()[2]==91 && hooks.memory(token).data()==address && hooks.memory(token)[127]==0x6b);
    { Client next(ChatCoreClientOptions());CHECK(next.CoreDataGlobals()!=core && !next.ChatManagerGlobals()->Current() && next.CoreDataGlobals()->Cached()[2]==0); } //새Client는새최초상태이며전Client의정의된실제할당을바꾸지않는다.
    core.reset();hooks={};CHECK(weakCore.expired() && weakChat.expired());
}
