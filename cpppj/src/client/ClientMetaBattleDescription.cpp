// 설명 복사/정제 뒤 같은 현재 World의 실제88과 특정 전투의 기존 표시 본체를 실행한다.
#include "client/ClientMetaBattleDescription.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include <stdexcept>

namespace netstorm::client { //일반 World/메타 World를 새로 만들거나 성공 효과로 대체하지 않는다.
NativeMetaBattleDescriptionHooks MakeClientMetaBattleDescriptionHooks(Client& client) {
    NativeMetaBattleDescriptionHooks hooks;hooks.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };
    hooks.refreshObject=[&client](auto number) { auto& storage=*client.SurfaceStorage();if (number>65535U || number>=storage.pool.Capacity()) throw std::out_of_range("전투 설명의 실제 객체 SID 물리 범위");const auto raw=storage.pool.Slot(o::Sid{static_cast<std::uint16_t>(number)});client.GameplayGlobals()->RequireSession().Core().display.Update(raw,0); };
    hooks.updateBattle=[&client](auto battle) { client.UpdateNativeMetaBattle(battle); };return hooks; //전체 Poll/다른 참가자 갱신으로 대체하지 않는다.
}
}
