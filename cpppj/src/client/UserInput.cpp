#include "client/UserInput.h"
#include "client/ProductionShortcuts.h"
#include "client/ClientMain.h"
#include "client/RawCampaignCursor.h"

namespace netstorm::client {
namespace {
// 이 입력 어댑터가 아직 처리하는 두 Home 키이며 화살표 카메라 속도를 따로 만들지 않는다.
constexpr unsigned kF4=0x73,kF5=0x74;
}
// 같은 Client/World를 빌리며 별도 키 누름/임시 카메라 시계를 만들지 않는다.
UserInput::UserInput(Client& client,GameWorld& world) : client_(client),world_(world) {}
// 원본의 좌클릭 이동과 우클릭 메뉴를 구분한다.
std::optional<o::SquidId> UserInput::Event(InputEvent event) {
    if ((event.code & InputCode::kOutside)!=0) return {};
    world_.SetMousePoint({event.x,event.y}); // 선택 시 모드5의 역투영도 같은 현재 클릭 좌표를 사용한다.
    ReadCursorOptions();std::int32_t rotateMode=0; // 원본 rotmode의 누락 기본값0이다.
    client_.Configuration().ReadInt("rotmode",rotateMode);
    if (world_.Session().Cursor().HandleInput(event,static_cast<std::uint32_t>(rotateMode))) return {};
    if (QueueProductionShortcut(event,client_.Paused(),{[] { return kProductionBridgeCommand; },[this](auto command,auto slot,auto zero) { client_.Input().PushCommand(command,slot,zero); }})) return {}; // 실제 Gump 경로가 같은 큐의 다음 사건을 소비한다.
    const auto key=(event.code>>InputCode::kVirtualKeyShift)&255u;
    const bool release=(event.code & InputCode::kRelease)!=0;
    if (key>=8) {
        if (release) return {};
        if (key==kF4) world_.Home(false); if (key==kF5) world_.Home(true); return {};
    }
    const auto character=event.code & 0xffffu;
    if (!release && (character=='h' || character=='H')) { world_.Home(false); return {}; }
    if (!release && (character=='p' || character=='P' || character=='r' || character=='R')) {
        if (const auto point=world_.Point("world:priest")) world_.Select(world_.Pick(*point)); return {};
    }
    if (release) return {};
    const ScreenPoint point{event.x,event.y};
    const auto mouse=event.code & 0x00ffffffu;
    if (mouse==InputCode::kRightButton) { const auto picked=world_.Pick(point);if (picked && world_.Session().Editor()) world_.PrepareEditorObjectMenu(picked);return picked; } // 원본 편집 우클릭의 선행 선택/Core 전환 뒤에만 객체 메뉴를 연다.
    if (mouse==InputCode::kLeftButton) {
        if (world_.InputMode()==2 || world_.InputMode()==5) world_.MoveSelected(point);
        else world_.Select(world_.Pick(point));
    }
    return {};
}
}

// 원본에 등록된 두 DWORD는 누락 시 BSS0이며 현재 설정을 해당 소비 시점에 읽는다.
void netstorm::client::UserInput::ReadCursorOptions() {
    std::int32_t snap=0,cannotPlace=0; // 두 설정의 별도 원본 전역이다.
    client_.Configuration().ReadInt("cursorSnapToGrid",snap);client_.Configuration().ReadInt("cursorShowCannotPlace",cannotPlace);
    auto& cursor=world_.Session().Cursor();cursor.SnapToGrid()=static_cast<std::uint32_t>(snap);cursor.Placement().placementFlags=static_cast<std::uint32_t>(cannotPlace);
}
// 원본445330의 깊이0 조건에서만 실제 정수 마우스 갱신446b50을 호출한다.
void netstorm::client::UserInput::UpdateCursor(InputEvent mouse) {
    world_.SetMousePoint({mouse.x,mouse.y}); // 강조/커서 모두 같은 현재 폴링 좌표를 사용한다.
    if (mouse.code&InputCode::kOutside) return;
    if (world_.Session().CursorDropSettings().lockDepth) { world_.UpdateMovementCursor();return; }
    ReadCursorOptions();world_.Session().Cursor().Update(mouse.x,mouse.y);
}
