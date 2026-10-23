// Client장기owner의같은144BYTE와lastKnown10을분리해검사하는사본초안이다.
#include "TestSupport.h"
#include "client/ClientCoreDataGlobals.h"
#include <algorithm>
#include <array>
#include <utility>

using namespace netstorm::client;
// 최초CRT뒤실제장부/lastKnown은별도고정주소이고metadata가같은owner장부를직접갱신한다.
TEST_CASE(ClientCoreRecords_OneOwnerHasSeparateRecordsAndLastKnown) {
    ClientCoreDataGlobals owner;const auto& constant=std::as_const(owner);const auto address=owner.Records().data();
    CHECK(owner.Records().size()==144 && owner.Cached().size()==10 && owner.Records().data()==constant.Records().data() && owner.Cached().data()!=address);
    CHECK(std::all_of(owner.Records().begin(),owner.Records().end(),[](auto byte) { return byte==0; }) && std::all_of(owner.Cached().begin(),owner.Cached().end(),[](auto byte) { return byte==0; }));
    owner.Records()[142]=0x5a;owner.Records()[143]=0x6b;owner.Cached()[0]=0x7c;std::array<std::uint8_t,10> core{1,2,3,4,5,6,7,8,9,10};owner.StoreMetadata(8,0x12345678,core);core.fill(0);
    CHECK(owner.Records().data()==address && constant.Records()[128]==0x78 && constant.Records()[131]==0x12 && constant.Records()[132]==1 && constant.Records()[141]==10 && constant.Records()[142]==0x5a && constant.Records()[143]==0x6b && owner.Cached()[0]==0x7c);
}
// Current/Select0는다른lastKnown10을갱신하지만9기록을복제/reset하지않는다.
TEST_CASE(ClientCoreRecords_CoreSelectionDoesNotResetSavedRecords) {
    ClientCoreDataGlobals owner;std::array<std::uint8_t,10> core{10,9,8,7,6,5,4,3,2,1};owner.StoreMetadata(1,9,core);std::array<std::uint8_t,144> before{};std::copy(owner.Records().begin(),owner.Records().end(),before.begin());
    std::array<std::uint8_t,1620> players{};owner.Players()=players;owner.Current()=core;netstorm::o::PlayerWorldState player;std::uint32_t metamap{};owner.Select(player,metamap,0,1);
    CHECK(owner.Current().empty() && std::equal(core.begin(),core.end(),owner.Cached().begin()) && std::equal(before.begin(),before.end(),owner.Records().begin()));
    ClientCoreDataGlobals next;CHECK(next.Records().data()!=owner.Records().data() && next.Records()[16]==0 && owner.Records()[16]==9); // 새Client만초기화하고이전owner를다시만들지않는다.
}
