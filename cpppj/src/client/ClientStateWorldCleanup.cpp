// 앞선 실제 효과는 실패 시에도 보존하고 원문 Core0 뒤 Pool 앞에서만 Runtime 고정뷰를 반납한다.
#include "client/ClientStateWorldCleanup.h"
#include "client/ClientStateRuntimeRetirement.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/ClientUserSelectionCleanup.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientSpGlobals.h"
#include "client/CampaignTemplateStorage.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/RawChatChannels.h"
#include "client/RawSurfaceWorld.h"
#include "client/Cursor.h"
#include "o/RawFortMoneyLoad.h"
#include "o/RawPriestPostPop.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 같은 Native 부모를 재구현하지 않고 실제 Client 장치를 그 원문 위치에 바인딩한다.
namespace { // 선택된 실제 외부 장치만 요구하며 미선택 선택지는 호출하지 않는다.
template<class F> const F& NeedState(const F& function) { if (!function) throw std::logic_error("전체 State 정리의 선택된 실제 외부 장치 누락");return function; }
}
void CleanupClientStateWorld(std::uint32_t reason,ClientStateWorldContext c,std::shared_ptr<RawSurfaceStorage> storage,const ClientStateWorldDevices& devices) {
    if (!storage || storage.get()!=&c.surface) throw std::invalid_argument("전체 State 정리의 같은 실제 표면 owner 필요");
    const auto globals=BindClientStateWorldGlobals(c);ClientStateRuntimeRetirement runtime(c.client,storage);auto& game=*c.client.GameplayGlobals();auto& local=*c.client.LocalStateGlobals();auto& sp=c.client.SpGlobals()->Store();
    o::StateWorldCleanupHooks hooks;
    hooks.disconnect=[&](auto owner) { c.client.ConnectionGlobals()->Disconnect(static_cast<std::int32_t>(owner)); };
    hooks.assertion=devices.assertion;hooks.pauseDepth=[&] { return c.client.Paused() ? 1U : 0U; }; //부모는 비영만 검사하고 매 반복의 실제 정지 상태를 다시 읽는다.
    hooks.saveFort=[&](auto slot) { game.RequireSession().SnapshotFort(slot); };hooks.saveBattle=devices.saveBattle;
    hooks.resetAi=[&](auto owner) { runtime.ResetAi(owner); };hooks.resetPriest=[&](auto owner) { o::ResetPriestOwnerState(storage->priestOwnerState,owner); };
    hooks.resetOtherPlayer=[&](auto owner,auto amount) { o::SetPlayerSpAmount(std::bit_cast<std::int32_t>(owner),std::bit_cast<std::int32_t>(amount),sp,{[&] { return game.player.playerKnown; },[&] { return game.player.battleOwner; },[&] { return game.world.editor; }},[&] { NeedState(devices.refreshSp)(); }); };
    hooks.setSp=[&](auto owner,float value) { sp.Set(owner,std::bit_cast<std::uint32_t>(value)); };
    hooks.selectCore=[&](auto owner,auto templateOwner) { c.client.CoreDataGlobals()->Select(game.player,c.surface.ownerMode.challenge?1U:0U,owner,templateOwner); };
    hooks.resetPool=[&](auto capacity) { runtime.Retire(devices.retireRuntime);ResetClientSquidPool(c.client,*storage,static_cast<int>(capacity)); };
    hooks.wallSeconds=[&] { return c.client.WallSeconds(); };
    hooks.effect=[&](o::StateWorldCleanupEffect effect) {
        using E=o::StateWorldCleanupEffect; //주소 식별값을 실제 기존 하위에만 분배한다.
        switch (effect) {
        case E::CloseAllConnections:c.client.ConnectionGlobals()->CloseAll();return;
        case E::EndConnections:c.client.ConnectionGlobals()->EndPlayers(game.Server()?1U:0U);return;
        case E::ResumeGame:c.client.PopGamePause();return;
        case E::ResumeAudio:c.audio.PopMute();return;
        case E::Clear446e70:if (game.Borrowed()) game.RequireSession().Cursor().CancelForState();else NeedState(devices.cancelInitialCursor)();return;
        case E::Clear4c1f20:ResetClientClockSynchronization(local);return;
        case E::ResetClock:c.client.ResetGameClock();return;
        case E::Clear42b1b0:ClearClientMetaZoneRecords(c.client,*storage);return;
        case E::RefreshSp:NeedState(devices.refreshSp)();return;
        case E::ClearTemplates:c.templates.ClearAll();return;
        case E::LeaveMetamap:NeedState(devices.leaveMetamap)();return;
        case E::Clear445140:c.client.ChatChannels().ResetOccupied();return;
        case E::ClearCursor:ClearClientUserSelection(game,*storage,[&](auto cursor) { c.client.GetCursor().Set(std::bit_cast<std::int32_t>(cursor)); });return;
        case E::ClearPlayerLinks:runtime.ClearPlayerLinks();return;
        case E::ClearKernel:runtime.ClearKernel();return;
        case E::KillGumps:NeedState(devices.killGumps)();return;
        case E::CleanupGumps:NeedState(devices.cleanupGumps)();return;
        case E::ReplaceUber:NeedState(devices.replaceUber)();return;
        case E::Clear43b540:case E::Reset422490:case E::Clear4c2450:case E::Clear472830:case E::Clear4b2b50:case E::Clear4328a0:case E::Clear4ab940:case E::Reset568b60:case E::Reset54c8a0:case E::ResetGraph:ResetClientStateWorldStorage(c.client,*storage,effect);return;
        case E::MapMode:return; //보호 PE004437f0의 실제 C3 단일 RET다. 미복원 하위를 빈 성공으로 대체하는 분기가 아니다.
        case E::SceneMusic:c.audio.StartScene(c.surface.ownerMode.battle);return; //부모의 battle0쓰기 앞에 있는 현재 모드를 사용하고461의 같은 게임 난수를 소비한다.
        case E::Clear4a9e50:c.audio.Sounds().StopLoops();return;
        case E::InitializePlayers:InitializeClientPlayerRecords(c.client);return;
        case E::ResetZone:ResetClientStateZone(local);return;
        case E::InterpretOptions:c.client.InterpretStateOptions();return;
        }
        throw std::logic_error("전체 State 정리의 미등록 Effect");
    };
    o::CleanupStateWorld(reason,globals,hooks); //부모의 현재 값 재조회/직접 쓰기/효과 순서와 실패 prefix를 그대로 사용한다.
}
}
