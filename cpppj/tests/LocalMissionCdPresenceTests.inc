// 같은 Client CD 캐시/결과를 원문의 State7 효과 순서와 직접 접합한다. 실제 게임/통신은 실행하지 않는다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "o/RawBaseFileReadPolicy.h"

TEST_CASE(CdPresence_StateSevenStoresSameClientResultAfterCleanup) {
    using namespace netstorm::client;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));StateScene scene("Run",7,4,0);auto g=scene.Globals();
    LocalMissionStateGlobals globals{g.server,g.boss,g.inBattle,g.missionPost,g.savedGame,g.missionWorld,g.simulconnect,g.metamap,g.player,g.zonePending,g.gameTicks,g.sourceType,client.LocalStateGlobals()->CdPresent(),g.menuWindow,g.metaRefresh,g.metaRebuild,g.titleInitial,g.battleByte,g.connectEnd,g.activityEnd,g.gameSeconds,g.wallCached};
    auto hooks=scene.Hooks();bool cleaned{},queried{},commands{};const auto cleanup=hooks.cleanupBattle;hooks.cleanupBattle=[&](auto reason) { CHECK(!queried);cleanup(reason);cleaned=true; };
    CdPresenceHooks presence;presence.path=[](auto root,auto marker) { return JoinBaseFileRoot(root,marker); };presence.exists=[](auto) { return true; };presence.roots=[](auto output) { const std::array<char,5> data{'D',':','\\',0,0};std::copy(data.begin(),data.end(),output.begin());return 5U; };
    hooks.cdPresent=[&] { CHECK(cleaned && !commands);queried=true;return client.CdGlobals()->Check(presence); };hooks.refreshCommands=[&] { CHECK(queried && client.LocalStateGlobals()->CdPresent()==1);commands=true; };
    RawLocalMissionState state(OriginalEdition::Patch1078,scene.Storage(),globals,std::move(hooks));state.Run();CHECK(cleaned && queried && commands && client.LocalStateGlobals()->CdPresent()==1 && std::string(client.CdGlobals()->CachedRoot().data())=="D:\\");
}
