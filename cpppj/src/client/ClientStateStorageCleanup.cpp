// 첫 자연 생략도 원문 몸체로 판정하며 삭제가 선택되면 실제 현재 Runtime을 빌린다.
#include "client/ClientStateStorageCleanup.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawStateZoneReset.h"
#include "o/RawSquidPoolReset.h"
#include "o/RawTotalMade.h"
#include <stdexcept>

namespace netstorm::client { //장기 Client 물리의 주소와 실제 효과 실행기의 수명을 구별한다.
void ResetClientClockSynchronization(ClientLocalStateGlobals& globals) { o::ResetClockSynchronization(globals.ClockSynchronization()); }
void ResetClientStateZone(ClientLocalStateGlobals& globals) { o::ResetStateZone(globals.Zone()); }
//현재 실제 World 대여가 끝난 저장소만 받아 Native Pool 위치에서 부모 전체를 실행한다.
void ResetClientSquidPool(Client& client,RawSurfaceStorage& storage,int capacity) {
    const auto& current=client.SurfaceStorageIfInitialized();
    if (storage.pool.Edition()!=o::OriginalEdition::Patch1078 || storage.Authority()!=client.GameplayGlobals()->Authority() || storage.GameAux()!=client.GameplayGlobals()->GameAux() || (current && current.get()!=&storage)) throw std::invalid_argument("State Pool 정리의 실제 Client/표면 owner 불일치");
    if (storage.Borrowed()) throw std::logic_error("State Pool 정리 전 실제 World 대여 반납 필요");
    o::ResetSquidWorldPool(storage.pool,storage.boardTargetState.invalid,capacity);
}
// 원문 각 위치에서 같은 장기 물리의 기존 작은 몸체를 호출하며 Session/AI/Player를 재생성하지 않는다.
void ResetClientStateWorldStorage(Client& client,RawSurfaceStorage& storage,o::StateWorldCleanupEffect effect) {
    const auto& current=client.SurfaceStorageIfInitialized(); //검사 때문에 아직 없는 실제 Client 저장소를 새로 만들지 않는다.
    if (storage.pool.Edition()!=o::OriginalEdition::Patch1078 || storage.Authority()!=client.GameplayGlobals()->Authority() || storage.GameAux()!=client.GameplayGlobals()->GameAux() || (current && current.get()!=&storage))
        throw std::invalid_argument("State 물리 정리의 실제 Client/표면 owner 불일치");
    if (storage.Borrowed()) throw std::logic_error("State 물리 정리 전 실제 World 대여 반납 필요");
    switch (effect) {
    case o::StateWorldCleanupEffect::Clear472830:
        o::ResetRegisteredStateSidLists(storage.sidLists.registeredHeaders,[&](auto header) { storage.sidLists.ResetCount(header,client.GameplayGlobals()->selection.multipleCount); });return;
    case o::StateWorldCleanupEffect::Clear4c2450:o::ResetTotalMadeCounts(storage.totalMade);return;
    case o::StateWorldCleanupEffect::Clear4b2b50:storage.hash.Reset();return;
    case o::StateWorldCleanupEffect::Clear4328a0:o::ResetStatePlayerNoMakeWarnings(client.LocalStateGlobals()->PlayerNoMakeWarnings());return;
    case o::StateWorldCleanupEffect::Clear4ab940:o::ResetStateSpotMap(storage.spots);return;
    case o::StateWorldCleanupEffect::Reset568b60:o::ResetRawIslandRecords(storage.islands.records);return;
    case o::StateWorldCleanupEffect::Reset54c8a0:storage.chunks.Reset();return;
    case o::StateWorldCleanupEffect::ResetGraph:o::ResetRawGraphTable(storage.graph);return;
    default:throw std::invalid_argument("State 물리 정리의 미지원 실제 Effect");
    }
}
void ClearClientMetaZoneRecords(Client& client,RawSurfaceStorage& storage) {
    const auto& current=client.SurfaceStorageIfInitialized(); //미초기화 조회가 새 물리나 Session을 만들지 않는다.
    if (storage.pool.Edition()!=o::OriginalEdition::Patch1078 || storage.Authority()!=client.GameplayGlobals()->Authority() || storage.GameAux()!=client.GameplayGlobals()->GameAux() || (current && current.get()!=&storage))
        throw std::invalid_argument("메타구역 정리의 실제 Client/표면 owner 불일치");
    const auto globals=client.LocalStateGlobals()->MetaZones();
    o::ClearMetaZoneRecords(globals,{[&](auto index) {
        o::ClearMetaZoneMarker(globals.records,index,{[&](std::uint32_t sid,std::uint32_t flags) {
            auto& world=client.GameplayGlobals()->RequireSession().World(); //정상 빈 기록에는 Runtime을 요구하지 않으며 매 선택 위치에서 현재 것을 조회한다.
            if (world.Storage().get()!=&storage) throw std::logic_error("메타구역 삭제의 현재 Session/Pool 불일치");
            if (sid>=storage.pool.Capacity() || sid>0xffffU) throw std::out_of_range("메타구역 DWORD SID의 현재 Pool 물리 범위");
            static_cast<void>(world.Destroy(o::Sid{static_cast<std::uint16_t>(sid)},flags)); //지원된 실제 가상+10 분배를 재사용하며 bool 반환은 원문처럼 무시한다.
        }});
    }});
}
}
