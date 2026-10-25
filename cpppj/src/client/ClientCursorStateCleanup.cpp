// 외곽게임준비를호출하지않고실제Cancel/Prepare두본문만같은데이터에조립한다.
#include "client/ClientCursorStateCleanup.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/RawSurfaceWorld.h"
#include <utility>

namespace netstorm::client { // 실제게임모듈의선택효과는호출자가살아있는동안에만제공한다.
void CancelCursorForState(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,ClientCursorGlobals& state,CursorLifecycleHooks effects,CursorPlacementHooks placement,std::span<const std::uint16_t> surfaceMap) {
    RawCursorPlacement prepare(pool,types,surfaceMap,drop,state.Lifecycle(),state.Placement(),std::move(placement));effects.prepareCursor=[&prepare] { prepare.Prepare(); }; //NativeActivate가요청하는위치에서만현재Project/dirty를실행한다.
    RawCursorLifecycle lifecycle(pool,types,drop,state.Lifecycle(),std::move(effects));lifecycle.Cancel();
}
void CancelClientCursorForState(Client& client,RawSurfaceStorage& surface,CursorLifecycleHooks selectedEffects,std::function<Renderer&()> renderer) {
    auto& gameplay=*client.GameplayGlobals();auto& state=gameplay.CursorGlobals();state.Placement().sharedScreenOrigin=&gameplay.Camera().Snapshot().pixels; //Camera의같은현재픽셀만빌리고UI/Session을장기보관하지않는다.
    CancelCursorForState(surface.pool,surface.types,gameplay.cursor,state,std::move(selectedEffects),MakeClientCursorPrepareHooks(client,std::move(renderer)),surface.hash.Entries(0));
}
}
