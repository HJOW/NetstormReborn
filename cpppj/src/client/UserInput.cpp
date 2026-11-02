#include "client/UserInput.h"
#include "client/ProductionShortcuts.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h" //Ctrl+special 강제 후보 조회의 현재0055a480을 읽는다.
#include "client/RawUserSelection.h" //원문 WorldDestSelected와 실제 Shift 다중 선택 부모를 사용한다.
#include "client/RawCampaignCursor.h"
#include "client/SelectionKeyInput.h" //H/F4/F5의 일반 View 큐와 N/P/R의 원문 부모 정지 관문이다.
#include "client/GameMenuCommands.h" //현재 메뉴 명령 ID를 기존 조회 표에서 읽는다.
#include "client/ClientAudio.h" //편집 대상 취득 뒤 같은 실제 selectTab.wav 이름/장치를 사용한다.
#include "client/RawCursorPosition.h" //ChalSailPos 원문의 현재 카메라 역투영을 사용한다.
#include <bit> //View 명령의 owner DWORD를 같은 signed 큐 저장 비트로 전달한다.
#include <cmath> //원문ftol의 비정상/64비트 범위 밖 입력을 같은 하위DWORD0으로 처리한다.

namespace netstorm::client {
namespace { //멀티플레이 모드6의 정적 사전 연결이며 새 실행/통신 검사는 하지 않는다.
std::uint32_t SailCoordinate(float value) { //4e49c0의int64절삭 결과 하위DWORD이며 indefinite의 하위도0이다.
    if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
}
// 같은 Client/World를 빌리며 별도 키 누름/임시 카메라 시계를 만들지 않는다.
UserInput::UserInput(Client& client,GameWorld& world) : client_(client),world_(world) {}
// 원본의 좌클릭 이동과 우클릭 메뉴를 구분한다.
std::optional<o::SquidId> UserInput::Event(InputEvent event) {
    if ((event.code & InputCode::kOutside)!=0) return {};
    //005cae94/98은 앞선 폴링의 현재 좌표다. 큐 사건 좌표로 덮어쓰면 모드5/6의 원문 입력이 달라진다.
    ReadCursorOptions();auto& rotateMode=client_.GameplayGlobals()->CursorGlobals().RotationMode(); // 실제 옵션 해석/메뉴와 같은 원문 DWORD다.
    if (world_.Session().Cursor().HandleInput(event,rotateMode)) return {};
    if (QueueProductionShortcut(event,client_.Paused(),{[] { return kProductionBridgeCommand; },[this](auto command,auto slot,auto zero) { client_.Input().PushCommand(command,slot,zero); }})) return {}; // 실제 Gump 경로가 같은 큐의 다음 사건을 소비한다.
    SelectionKeyInputHooks selectionKeys;selectionKeys.paused=[this] { return client_.Paused(); };selectionKeys.command=[](auto address) { return GameMenuCommandValue(address).value(); };selectionKeys.localOwner=[this] { return world_.Session().LocalOwner(); }; //같은 실제 Client/Session의 현재 입력 경계를 사용한다.
    selectionKeys.queue=[this](auto code,auto first,auto second) { client_.Input().PushCommand(code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };selectionKeys.next=[this](auto add) { world_.SelectNextUnit(add); };selectionKeys.priest=[this,queue=selectionKeys.queue] { world_.SelectPriest(queue); };
    if (DispatchSelectionKeyInput(event,selectionKeys)) return {};
    const auto key=(event.code>>InputCode::kVirtualKeyShift)&255u;
    const bool release=(event.code & InputCode::kRelease)!=0;
    if (key>=8) {
        if (release) return {};
        return {}; //F4/F5는 위의 원문 View/다음 유닛 큐 경로가 처리했다.
    }
    if (release) return {};
    const ScreenPoint point{event.x,event.y};
    const auto mouse=event.code & 0x03ffffffu; //명령 사건을 같은 하위BYTE의 마우스로 잘못 해석하지 않는다.
    if (mouse==InputCode::kRightButton) {
        if (world_.InputMode()!=0) { world_.Highlight(0);world_.ResetSelection();return {}; } //선행강조0은enabled/demo 관문과 독립이며0번Select는정지로거부하지않는다.
        const auto force=(event.code&InputCode::kControl) && (client_.GameplayGlobals()->player.special || world_.Session().Editor()) ? 1U : 0U;const auto picked=world_.Pick(point,force);
        if (picked) { if (world_.Session().Editor()) world_.PrepareEditorObjectMenu(picked);return picked; }
        const auto core=world_.Session().Core();if (!core.ownerMode.challenge || !world_.Session().PlayerKnown()) world_.ResetSelection(); //challenge/known의별도항해메뉴는멀티플레이후속이며기존무동작을유지한다.
        return {};
    }
    if (mouse==InputCode::kLeftButton) {
        if (world_.InputMode()==2 || world_.InputMode()==5) {
            const auto force=(event.code&InputCode::kControl) && client_.GameplayGlobals()->player.special ? 1U : 0U;const auto picked=world_.Pick(point,force); //원문은 Shift 분기와 무관하게 현재 후보를 먼저 조회한다.
            if (event.code&InputCode::kShift) world_.ExtendSelection(picked,force);
            else world_.MoveSelected(point,[this](auto x,auto y) { client_.Input().PushCommand(kWorldDestinationSelectedCommand,x,y); });
        }
        else if (world_.InputMode()==1) {
            const auto picked=world_.Pick(point,0);if (picked) {
                world_.Session().Cursor().TakeObject(picked,0);
                if (auto* audio=client_.Audio()) audio->Sounds().Play(audio->Names().Lookup("selectTab.wav"),0,0,0,0,0); //검사용noAudio는기존장치정책으로음향만생략한다.
            }
            world_.Highlight(0); //대상이없어도강조를해제하며기존selected는일반Select로바꾸지않는다.
        }
        else if (world_.InputMode()==6) { //ChalSailPos 전용 분기는 디컴파일만으로 사전 연결하며 새 MP 검사는 하지 않는다.
            const auto view=world_.MapView();const auto current=client_.GameplayGlobals()->selection.mouse;
            if (view.left<=current.x && current.x<view.right && view.top<=current.y && current.y<view.bottom) {
                const auto target=RawCursorPosition::ScreenToWorld(event.x,event.y,client_.GameplayGlobals()->Camera().Snapshot().worldBits);const auto packed=(SailCoordinate(target[0])<<16)|(SailCoordinate(target[1])&0xffffU);
                client_.Input().PushCommand(0x0200006cU,0,std::bit_cast<std::int32_t>(packed));world_.Highlight(0); //005428a8의현재ChalSailPos 초기명령이며X가상위WORD다.
            }
        }
        else {
            const bool control=(event.code&InputCode::kControl)!=0;const auto force=control && client_.GameplayGlobals()->player.special ? 1U : 0U; //같은 Ctrl+special 값을 Pick과 Select 양쪽에 전달한다.
            world_.SelectFromPointer(world_.Pick(point,force),force,control);
        }
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
