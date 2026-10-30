// 실제 입력/Present 위치에 원문 가장자리·카메라 적용과 현재 Client 장치를 연결한다.
#include "client/UberGump.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/GameWorld.h"
#include "client/RawButtonControls.h"
#include "client/RawGameMenus.h"
#include "client/RawGameMenuBar.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCameraPresent.h"
#include "client/RawCameraCommit.h"
#include "client/RawEdgeScroll.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // Native 데이터와 현재 OS/장면 장치를 같은 Client 수명에서 사용한다.
// Guide 통지 뒤 현재 폴링을 공급하고 목표만 쓴다. 실제 현재 view는 Present 뒤에 적용한다.
void UberGump::PollEdgeScroll(const InputEvent& mouse) {
    client_.Configuration().ReadInt("edgeScroll",edgeScrollEnabled_);client_.Configuration().ReadInt("edgeScrollSpeed",edgeScrollSpeed_);
    auto& screen=client_.GetScreen();const auto border=screen.BorderOffset();RawEdgeScrollHooks hooks;
    hooks.poll=[this](auto code) { return client_.Poll(code); };hooks.menu=[this](auto serial) { return rawButtons_->SerialGump(serial); };
    hooks.blockingPopup=[this] { return rawButtons_->BlockingGumpOpen(); };hooks.closePopups=[this] { rawButtons_->RequestClosePopups(); };
    hooks.request=[this](auto dx,auto dy) { auto& camera=client_.GameplayGlobals()->Camera();RequestCameraDelta(camera.RendererView(),camera.Target(),client_.GetScreen().Height(),dx,dy,client_.LocalStateGlobals()->MetaRebuild()); };
    // Windows10/11 borderless 호환 모드는 다른 앱이 활성일 때 바깥 포인터로 배경 카메라를 이동하지 않는다. 원문 입력 계산/타이머는 같은 본체를 사용한다.
    const auto enabled=client_.Active() ? static_cast<std::uint32_t>(edgeScrollEnabled_) : 0U;
    ProcessEdgeScroll({pointerBoundary_.initializationFlags,edgeScrollSince_},{enabled,rawButtons_->CurrentDialog(),gameMenuState_ ? gameMenuState_->menuSerial : 0U,
        border.x,border.y,screen.Width(),screen.Height(),mouse.x,mouse.y,edgeScrollSpeed_,client_.Time().wall},hooks);
}
//497960의 실제 장치/dirty/두 setter를 같은 현재 Client에서 실행한다. 표시/음향 공개 시점은 호출자가 소유한다.
void UberGump::CommitCameraNow() {
    auto& gameplay=*client_.GameplayGlobals();auto& camera=gameplay.Camera();auto& renderer=client_.GetRenderer();auto& screen=client_.GetScreen();
    const auto clip=world_ ? world_->MapView() : ScreenRect{0,0,screen.Width(),screen.Height()};
    const RawCameraCommitViewport viewport{clip.left,clip.top,clip.right-clip.left,clip.bottom-clip.top};const auto width=screen.Width(),height=screen.Height();const auto flags=screen.Flags();
    RawCameraCommitHooks apply;
    apply.lockCursor=[this,&gameplay](auto skip) {
        if (world_ && world_->Session().HasCursorRuntimeForState()) { world_->Session().Cursor().Lock(skip);return; }
        if (!gameplay.cursor.lockDepth) throw std::logic_error("미설치 World의 활성 커서 Lock 경계 필요");
        ++gameplay.cursor.lockDepth; // 원문은 비영 깊이일 때 Activate/장치 없이 같은 DWORD만 증가한다.
    };
    apply.unlockCursor=[this,&gameplay] {
        if (world_ && world_->Session().HasCursorRuntimeForState()) { world_->Session().Cursor().Unlock();return; }
        --gameplay.cursor.lockDepth;if (!gameplay.cursor.lockDepth) throw std::logic_error("미설치 World의 활성 커서 preview 재생성 경계 필요"); // 원문 비영 결과의 자연 반환만 장치 없이 수행한다.
    };
    apply.redrawRoots=[this] { rawButtons_->RedrawRoots();rebuild_=true; };
    apply.scroll=[&screen](auto x,auto y,auto w,auto h,auto dx,auto dy) { screen.Scroll(x,y,w,h,dx,dy); };
    apply.scrollBackground=[](auto,auto,auto,auto,auto,auto) { throw std::logic_error("비DIB DirectDraw 시차 장치 미복원"); }; // Windows10/11 호환 Screen은 항상 DIB2이므로 원문 부모가 이 장치를 선택하지 않는다.
    apply.addDirty=[&renderer](auto rect,auto value) { renderer.Invalidate(rect,value); };apply.addBackgroundDirty=[&renderer](auto rect) { renderer.Invalidate(rect); };
    apply.updateMouse=[this,&renderer,&screen](ScreenPoint point) {
        if (!(screen.Flags()&ScreenMode::kSoftwareMouse)) return;const auto hotspot=client_.GetCursor().Hotspot(); //4a41c0도software64 비트가 없으면 자연 반환한다.
        renderer.SetSoftwareCursor(client_.Active() ? client_.GetCursor().SoftwareImage() : nullptr,{point.x-hotspot.x,point.y-hotspot.y},true);
    };
    CommitCameraTarget({camera.RendererView(),camera.Snapshot(),camera.Target(),renderer.CameraRedraw(),worldRedraw_,renderer.CameraDirtyEntries()},
        {flags,camera.Denominator().current,width,height,viewport,clip,gameplay.selection.mouse},apply);
}
// 실제 화면 출력 뒤에만 flip/목표 제한을 수행하고 현재 카메라를 적용한다. Home의 직접 적용과 구별한다.
void UberGump::ApplyCameraFrame() {
    auto& gameplay=*client_.GameplayGlobals();auto& camera=gameplay.Camera();auto& screen=client_.GetScreen();
    const auto beforePixels=camera.RendererView().pixels;const auto beforeZoom=camera.RendererView().zoom;const auto width=screen.Width(),height=screen.Height();
    RawCameraPresentHooks present;present.flip=[&screen] { screen.Flip(); };present.commit=[this] { CommitCameraNow(); };
    PresentCameraTarget(camera.Target(),{gameplay.PatchDiagnostic(),width,height,gameplay.WorldCells()},present);
    if (camera.RendererView().pixels!=beforePixels || camera.RendererView().zoom!=beforeZoom) {
        if (world_) world_->CameraApplied();rebuild_=true; // 프레임 말미의 같은 원점으로 표면/입자/위치 음향을 갱신하고 다음 Compose를 예약한다.
    }
}
}
