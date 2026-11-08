// 기존메타표시와같은실제팩터리/QA WORD·Pool/가상Pop을생성부모에공급한다.
#include "client/ClientMetaBattleObjects.h"
#include "client/NativeMetaBattleCreate.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include <stdexcept>

namespace netstorm::client { //Client는바인딩한동기효과보다오래살아야한다.
// 생성시없을수있는Session/CS는요구하지않고각원문효과를선택한위치에서읽는다.
void BindClientMetaBattleObjects(NativeMetaBattleCreateHooks& hooks,Client& client) {
    if (client.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("메타전투 실제 물리 장치의10.78 Client 필요");
    const auto sid=[&client](std::uint32_t number) { if (number>65535U || number>=client.SurfaceStorage()->pool.Capacity()) throw std::out_of_range("메타전투 실제 DWORD SID 물리 범위");return o::Sid{static_cast<std::uint16_t>(number)}; }; //전체DWORD범위를확인한뒤현재WORD주소로만변환한다.
    hooks.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };hooks.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };hooks.battleType=[&client] { return client.ChallengeGlobals()->BattleType(); };
    hooks.create=[&client](auto type,auto flags) { return std::uint32_t(client.SurfaceStorage()->CreateAsset(type,flags).value); };
    hooks.raw=[&client,sid](auto number)->std::span<std::uint8_t> { const auto bytes=client.SurfaceStorage()->pool.Slot(sid(number));return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }; //같은비const Pool의실제50BYTE뷰이며free/active필터를추가하지않는다.
    hooks.setKind=[&client,sid,raw=hooks.raw](auto number,auto kind) {
        const auto bytes=raw(number);bytes[12]=static_cast<std::uint8_t>(kind);bytes[13]=static_cast<std::uint8_t>(kind>>8);const auto storage=client.SurfaceStorage();if (bytes[10]==storage->links.bridgeType) storage->Building().NotifySurface(sid(number)); //4ac220의실제QA WORD쓰기뒤현재다리만NotifySurface한다.
    };
    hooks.pop=[&client,sid](auto number,float x,float y,auto flags) { static_cast<void>(client.SurfaceStorage()->PopAsset(sid(number),x,y,flags)); };
}
}
