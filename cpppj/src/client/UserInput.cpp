#include "client/UserInput.h"
#include "client/ClientMain.h"
#include "client/RawCampaignCursor.h"
#include <algorithm>
#include <cmath>

namespace netstorm::client {
namespace {
// 카메라 속도는 검사용 InspectView의 기존 12픽셀/14ms를 초 단위로 옮긴 임시값이다.
constexpr double kCameraPixelsPerSecond=12.0/0.014;
// 원본 키 번호(왼쪽/위/오른쪽/아래, Alt, F4/F5).
constexpr unsigned kLeft=0x25,kUp=0x26,kRight=0x27,kDown=0x28,kAlt=0x12,kF4=0x73,kF5=0x74;
}
// 누름 상태를 월드마다 따로 관리한다.
UserInput::UserInput(Client& client,GameWorld& world) : client_(client),world_(world),lastWall_(client.Time().wall) {}
// 원본의 좌클릭 이동과 우클릭 메뉴를 구분한다.
std::optional<o::SquidId> UserInput::Event(InputEvent event) {
    if ((event.code & InputCode::kOutside)!=0) return {};
    ReadCursorOptions();std::int32_t rotateMode=0; // 원본 rotmode의 누락 기본값0이다.
    client_.Configuration().ReadInt("rotmode",rotateMode);
    if (world_.Session().Cursor().HandleInput(event,static_cast<std::uint32_t>(rotateMode))) return {};
    const auto key=(event.code>>InputCode::kVirtualKeyShift)&255u;
    const bool release=(event.code & InputCode::kRelease)!=0;
    if (key>=8) {
        held_[key]=!release; if (release) return {};
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
    if (mouse==InputCode::kRightButton) return world_.Pick(point);
    if (mouse==InputCode::kLeftButton) {
        const auto object=world_.Pick(point);
        if (object) world_.Select(object); else world_.MoveSelected(point);
    }
    return {};
}
// 게임 시간 정지와 무관하게 화면 이동은 현재 벽시계를 사용한다.
void UserInput::Tick() {
    const double wall=client_.Time().wall,delta=std::clamp(wall-lastWall_,0.0,0.05); lastWall_=wall;
    if (!client_.Active()) { Cancel(); return; }
    const auto held=[&](unsigned key) { return held_[key] || (client_.Poll(key<<InputCode::kVirtualKeyShift).code & InputCode::kRelease)==0; };
    const bool cursorActive=world_.Session().CursorDropSettings().lockDepth==0; // 활성 커서의 좌우키는 원본 회전으로 소비한다.
    int x=cursorActive ? 0 : (held(kRight)?1:0)-(held(kLeft)?1:0),y=(held(kDown)?1:0)-(held(kUp)?1:0);
    const auto mouse=client_.Poll(InputCode::kMiddleButton);
    if ((mouse.code & InputCode::kOutside)==0 && (held(kAlt) || (mouse.code & InputCode::kRelease)==0)) {
        x=(mouse.x>client_.GetScreen().Width()/2+16)-(mouse.x<client_.GetScreen().Width()/2-16);
        y=(mouse.y>client_.GetScreen().Height()/2+16)-(mouse.y<client_.GetScreen().Height()/2-16);
    }
    const int amount=static_cast<int>(std::lround(delta*kCameraPixelsPerSecond)); world_.Scroll(x*amount,y*amount);UpdateCursor(mouse);
}
// 메뉴/비활성으로 건너뛴 키 올림 사건이 다음 월드 조작에 남지 않게 한다.
void UserInput::Cancel() { held_.fill(false); lastWall_=client_.Time().wall; }
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
    if ((mouse.code&InputCode::kOutside) || world_.Session().CursorDropSettings().lockDepth) return;
    ReadCursorOptions();world_.Session().Cursor().Update(mouse.x,mouse.y);
}
