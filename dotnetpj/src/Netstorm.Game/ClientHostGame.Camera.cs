using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제 지도 영역의 매 프레임 카메라 목표 적용을 제품 루프에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>같은 실제 화면·Renderer·커서 장치로 미션 카메라를 매 프레임 적용한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>제품 진입 경로의 세션 카메라 장치를 실제 화면·Renderer 장치와 연결한다.</summary>
    /// <param name="session">미션 세션이다.</param>
    /// <remarks>2026-11-26 1.062에서 R5-1용으로 추가했다. 근거: cpppj UberGumpCamera.cpp의 CommitCameraNow·ApplyCameraFrame 결선이다. 뒤집기는 매 프레임 출력에 맡기고 배경 시차는 비DIB에서만 요구한다. 1.064에서 Pan·저장 조립을 뺐다. 매 프레임 포인터 폴링이 미조립 Pan의 명시 실패에 닿아 활성 창 미션 실행이 깨졌으므로 Commit·Present만 조립한다. Pan 폴링·저장 키는 R6에서 실제 장치로 잇는다.</remarks>
    private void BindMissionCameraDevices(HostSession session)
    {
        ArgumentNullException.ThrowIfNull(session);
        session.BuildCameraFrameDevices(
            new RawCameraCommitHooks
            {
                Scroll = (x, y, width, height, dx, dy) => _driver.Screen.Scroll(x, y, width, height, dx, dy),
                ScrollBackground = (_, _, _, _, _, _) => throw new InvalidOperationException("비DIB 배경 시차 장치 미연결(R5)"),
                AddDirty = (rect, flags) => NativeRenderer.Invalidate(rect, flags),
                AddBackgroundDirty = rect => NativeRenderer.Invalidate(rect),
                RedrawRoots = () => _menuDirty = true,
            },
            new RawCameraPresentHooks
            {
                // DN은 Draw에서 매 프레임 출력하므로 뒤집기는 빈 동작이다.
                Flip = () => { },
            });
    }

    /// <summary>실제 지도 영역 전역으로 같은 세션 카메라의 목표·현재값을 적용한다.</summary>
    /// <param name="session">미션 세션이다.</param>
    /// <remarks>2026-11-26 1.062에서 R5-1용으로 추가했다. 근거: cpppj UberGumpCamera.cpp의 ApplyCameraFrame이다. 전체 viewport 대체 없이 실제 지도 영역을 쓴다.</remarks>
    private void ApplyMissionCameraFrame(HostSession session)
    {
        ArgumentNullException.ThrowIfNull(session);
        ScreenRect map = NativeMapView();
        var viewport = new RawCameraCommitViewport(map.Left, map.Top, map.Right - map.Left, map.Bottom - map.Top);
        ClientCameraGlobals camera = session.Gameplay.Camera;
        int[] before = [(int)camera.RendererView.Pixels[0], (int)camera.RendererView.Pixels[1]];
        session.CameraViewport = new RawCameraCommitGlobals(_driver.Screen.Flags, camera.Denominator.Current,
            _driver.Screen.Width, _driver.Screen.Height, viewport, map,
            new ScreenPoint(session.Gameplay.Cursor.ScreenX, session.Gameplay.Cursor.ScreenY));
        session.PresentCameraNow(session.Gameplay.PatchDiagnostic.Value);
        // 실제 view가 바뀌었을 때만 다시 합성한다. 월드 그리기는 R5 본편에서 잇는다.
        if (before[0] != camera.RendererView.Pixels[0] || before[1] != camera.RendererView.Pixels[1]) _menuDirty = true;
    }
}
