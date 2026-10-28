//World 없는 메뉴에서도 원문의 커서/기한 진행을 보존하며 현재 대여의 처리기만 빌린다.
#include "client/ClientBridgeDecay.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawCampaignSession.h"
#include "o/RawBridgeDecayScan.h"
#include <stdexcept>
#include <sstream>

namespace netstorm::client { //호스트용 고정 틱이나 독립 스캔 사본을 만들지 않는다.
void RunSurfaceBridgeDecay(RawSurfaceStorage& storage,const ClientGameplayGlobals& globals,const o::FrameTime& time,bool paused,const std::function<void(o::Sid)>& cell) {
    if (storage.Authority()!=globals.Authority()) throw std::logic_error("공통 다리 스캔의 현재 권한 소유자 불일치");
    if (storage.pool.Edition()!=o::OriginalEdition::Patch1078) return; //CD의 별도 인라인 스캔은 후속 복원하며 현재 실제 Client 연결은10.78이다.
    o::BridgeDecayMode mode;mode.server=globals.Authority()->server;mode.authority=globals.Authority()->boss;mode.editor=globals.world.editor!=0;
    o::RawBridgeDecayScan scan(storage.pool,storage.types,storage.bridgeDecayScan,storage.bridgeDecayPeriod,mode,cell);scan.Frame(time,paused);
}
//현재 실제 Time을 참조로 유지해 칸 효과 뒤 기한 재설정도 같은 전역을 다시 읽는다.
void RunClientBridgeDecay(Client& client) {
    const auto& storage=client.SurfaceStorageIfInitialized();if (!storage) return; //실제 WinMain은 자산/풀 초기화 뒤 이 프레임에 들어온다.
    const auto& globals=client.GameplayGlobals();RunSurfaceBridgeDecay(*storage,*globals,client.Time(),client.Paused(),[&client,&globals](o::Sid sid) { globals->RequireSession().World().DecayBridgeCell(client.Time(),sid); });
}
//World 없는 메뉴도 관측할 수 있게 하되 슬롯/커서/기한을 쓰지 않는다.
std::string ReportClientBridgeDecay(const Client& client) {
    const auto& storage=client.SurfaceStorageIfInitialized();if (!storage) return {};const auto& globals=client.GameplayGlobals();const auto& scan=storage->bridgeDecayScan;std::size_t eligible{};
    if (storage->pool.Edition()==o::OriginalEdition::Patch1078) for (std::uint32_t number=0;number<storage->pool.Capacity();++number) { const auto raw=storage->pool.Slot(o::Sid{static_cast<std::uint16_t>(number)});if (!(raw[11]&7U) && raw[10]<storage->types.size() && (storage->types[raw[10]].flags2&4U) && !(raw[40]&1U)) ++eligible; } //명시 CLI 보고에만 실제 후보 조건을 관찰한다.
    std::ostringstream out;out<<"bridge_scan_global\t"<<scan.first<<'\t'<<scan.last<<'\t'<<scan.cursor<<'\t'<<scan.next<<'\t'<<storage->bridgeDecayPeriod<<'\t'<<globals->world.editor<<'\t'<<globals->Authority()->boss<<'\t'<<globals->Authority()->server<<'\t'<<globals->Borrowed()<<'\t'<<eligible<<'\n';return out.str();
}
}
