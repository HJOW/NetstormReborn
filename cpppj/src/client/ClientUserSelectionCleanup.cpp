// 첫 메뉴의 자연 빈 경로도 raw 본문을 실행하고 선택된 객체 효과만 실제 Runtime에 요구한다.
#include "client/ClientUserSelectionCleanup.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidSelection.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // transient 처리기는 같은 owner/장치보다 먼저 반납한다.
namespace {
// 실제 객체 통지에 도달했을 때만 같은 현재 Pool의 Session을 요구한다.
RawWorldCore SelectionCore(ClientGameplayGlobals& globals,const RawSurfaceStorage& storage) {
    auto core=globals.RequireSession().Core();if (&core.pool!=&storage.pool) throw std::logic_error("선택 정리의 현재 Session/Pool 불일치");return core;
}
}
// count0/음수는 객체 조회 없이 자연 반환하며 기존 단일 선택의 가상 통지를 추가하지 않는다.
void ClearClientUserSelection(ClientGameplayGlobals& globals,RawSurfaceStorage& storage,std::function<void(std::uint32_t)> selectCursor) {
    RawUserHighlight highlight(globals.selection.HighlightState(),UserHighlightHooks{std::move(selectCursor),{}, {}});
    UserSelectionHooks hooks;hooks.enabled=[&globals] { return globals.selection.enabled; };hooks.highlight=[&highlight](auto mode) { highlight.Highlight(mode); };
    hooks.selectNotice=[&globals,&storage](o::Sid sid,std::uint32_t on) {
        auto core=SelectionCore(globals,storage);o::RawSquidSelection marks(core.pool,[core](o::Sid current) { core.display.Update(core.pool.Slot(current)); });marks.Mark(sid,on);
    };
    hooks.refresh=[&globals,&storage](o::Sid sid) { auto core=SelectionCore(globals,storage);core.display.Update(core.pool.Slot(sid)); };
    RawUserSelection selection(storage.pool,storage.types,globals.selection.SelectionState(),std::move(hooks));selection.Clear();
}
// Renderer/월드/메뉴를 만들지 않고 실제 현재 장치를 호출 위치에서 조회한다.
void ClearClientUserSelection(Client& client) {
    ClearClientUserSelection(*client.GameplayGlobals(),*client.SurfaceStorage(),[&client](std::uint32_t cursor) { client.GetCursor().Set(std::bit_cast<std::int32_t>(cursor)); });
}
}
