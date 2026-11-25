using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 실제 지도 영역의 저장·Commit·Present·Pan을 같은 세션 카메라에서 실행한다.
public sealed partial class HostSession
{
    /// <summary>같은 세션 카메라에서 조립한 Commit 본체 갈고리다. 비어 있으면 미조립이다.</summary>
    private RawCameraCommitHooks? _cameraCommit;

    /// <summary>같은 세션 카메라에서 조립한 Present 본체 갈고리다. 비어 있으면 미조립이다.</summary>
    private RawCameraPresentHooks? _cameraPresent;

    /// <summary>같은 세션 카메라에서 조립한 Pan 본체 갈고리다. 비어 있으면 미조립이다.</summary>
    private CameraPanHooks? _cameraPan;

    /// <summary>같은 세션 카메라에서 조립한 저장 카메라 갈고리다. 비어 있으면 미조립이다.</summary>
    private SavedCameraKeyHooks? _savedCamera;

    /// <summary>Commit·Present가 읽는 실제 지도 영역 전역이다. 비어 있으면 미설정이다.</summary>
    public RawCameraCommitGlobals? CameraViewport { get; set; }

    /// <summary>Commit 본체가 쓰는 세션 소유 전체 다시 그리기다. Renderer 결선(R5) 전까지 여기서 관찰한다.</summary>
    public UIntBox CameraRedraw { get; } = new();

    /// <summary>Commit 본체가 쓰는 세션 소유 세계 다시 그리기다. Renderer 결선(R5) 전까지 여기서 관찰한다.</summary>
    public UIntBox CameraWorldRedraw { get; } = new();

    /// <summary>도달한 누락 장치를 R5·R6 작업으로 미룬다는 명시 실패다.</summary>
    private static T CameraMissing<T>(string name) => throw new InvalidOperationException(name);

    /// <summary>같은 세션 카메라에서 저장·Commit·Present·Pan 본체 갈고리를 연결한다.</summary>
    /// <param name="commit">호출자가 미리 묶은 Commit 장치다. 비어 있으면 세션 실제 장치와 R5·R6 명시 거부로 채운다.</param>
    /// <param name="present">호출자가 미리 묶은 Present 장치다. 비어 있으면 세션 실제 장치와 R5 명시 거부로 채운다.</param>
    /// <param name="pan">호출자가 미리 묶은 Pan 장치다. 비어 있으면 세션 실제 요청과 R5·R6 명시 거부로 채운다.</param>
    /// <param name="saved">호출자가 미리 묶은 저장 카메라 장치다. 비어 있으면 R6 명시 거부로 채운다.</param>
    /// <remarks>2026-11-26 1.050에서 DI-4d 카메라 결선용으로 추가했다. 근거: cpppj UberGumpCamera.cpp의 CommitCameraNow·CenterViewPoint·ApplyCameraFrame 결선이다. 화면 이동·뒤집기·지도 영역·소리·중심은 R5·R6에서 잇는다. 제품 미션 경로는 Pan·저장 키 폴링이 R6이므로 <see cref="BuildCameraFrameDevices"/>를 쓴다.</remarks>
    public void BuildCameraDevices(RawCameraCommitHooks? commit = null, RawCameraPresentHooks? present = null, CameraPanHooks? pan = null, SavedCameraKeyHooks? saved = null)
    {
        if (_cameraCommit is not null || _cameraPresent is not null || _cameraPan is not null || _savedCamera is not null) throw new InvalidOperationException("세션 카메라 장치 중복 연결");
        RawCameraCommitHooks live = commit ?? new();
        live.LockCursor ??= _ => { Gameplay.Cursor.LockDepth++; };
        live.UnlockCursor ??= () =>
        {
            if (Gameplay.Cursor.LockDepth == 0) throw new InvalidOperationException("세션 커서 잠금 underflow(R6 미리보기 재생성 경계)");
            Gameplay.Cursor.LockDepth--;
        };
        live.RedrawRoots ??= () => CameraMissing<object>("카메라 Gump 다시 그리기 실제 장치 미연결(R5)");
        live.Scroll ??= (_, _, _, _, _, _) => CameraMissing<object>("카메라 화면 이동 실제 장치 미연결(R5)");
        live.ScrollBackground ??= (_, _, _, _, _, _) => CameraMissing<object>("카메라 배경 이동 실제 장치 미연결(R5)");
        live.AddDirty ??= (_, _) => CameraMissing<object>("카메라 변경 영역 실제 장치 미연결(R5)");
        live.AddBackgroundDirty ??= _ => CameraMissing<object>("카메라 배경 변경 영역 실제 장치 미연결(R5)");
        live.UpdateMouse ??= point => { Gameplay.Cursor.ScreenX = point.X; Gameplay.Cursor.ScreenY = point.Y; };
        _cameraCommit = live;

        RawCameraPresentHooks shown = present ?? new();
        shown.Flip ??= () => CameraMissing<object>("카메라 뒤집기 실제 장치 미연결(R5)");
        shown.Commit ??= () => CommitCameraNow();
        _cameraPresent = shown;

        CameraPanHooks moving = pan ?? new();
        moving.Poll ??= _ => CameraMissing<InputEvent>("카메라 pan 폴링 실제 장치 미연결(R6)");
        moving.Globals ??= () => CameraMissing<CameraPanGlobals>("카메라 pan 전역 실제 장치 미연결(R6)");
        moving.ClosePopups ??= () => CameraMissing<object>("카메라 pan 팝업 정리 실제 장치 미연결(R6)");
        moving.Request ??= (dx, dy) =>
        {
            if (CameraViewport is null) throw new InvalidOperationException("카메라 pan 실제 지도 영역 미설정(R5)");
            RequestCameraDelta(dx, dy, CameraViewport.Height);
        };
        _cameraPan = moving;

        SavedCameraKeyHooks kept = saved ?? new();
        kept.Paused ??= () => CameraMissing<bool>("저장 카메라 실제 정지 판정 미연결(R6)");
        kept.Language ??= () => CameraMissing<int>("저장 카메라 현재 공개 언어 미연결(R6)");
        kept.Sound ??= () => CameraMissing<object>("저장 카메라 실제 선택음 미연결(R6)");
        kept.Capture ??= () => CameraMissing<ConstructionNoticePoint>("저장 카메라 현재 중심 미연결(R6)");
        kept.Center ??= (_, _) => CameraMissing<object>("저장 카메라 실제 즉시 중심 미연결(R6)");
        _savedCamera = kept;
        RawSavedCameraKey.InitializeSavedCameraPositions(Gameplay.Camera.SavedPositions);
    }

    /// <summary>실제 지도 영역 전역으로 같은 Renderer의 Commit 본체를 실행한다.</summary>
    public void CommitCameraNow()
    {
        RawCameraCommitHooks hooks = _cameraCommit ?? throw new InvalidOperationException("세션 카메라 장치 미연결");
        RawCameraCommitGlobals viewport = CameraViewport ?? throw new InvalidOperationException("카메라 실제 지도 영역 미설정(R5: 전체 viewport 대체 없음)");
        RawCameraCommit.CommitCameraTarget(
            new(Gameplay.Camera.RendererView, Gameplay.Camera.Snapshot, Gameplay.Camera.Target, CameraRedraw, CameraWorldRedraw, []),
            viewport, hooks);
    }

    /// <summary>두 제한을 적용한 뒤 같은 Renderer의 Commit 본체를 실행한다.</summary>
    /// <param name="diagnostic">현재 진단 DWORD이며 정확 1에서 이후 호출을 건너뛴다.</param>
    public void PresentCameraNow(uint diagnostic)
    {
        RawCameraPresentHooks hooks = _cameraPresent ?? throw new InvalidOperationException("세션 카메라 장치 미연결");
        RawCameraCommitGlobals viewport = CameraViewport ?? throw new InvalidOperationException("카메라 실제 지도 영역 미설정(R5: 전체 viewport 대체 없음)");
        RawCameraPresent.PresentCameraTarget(Gameplay.Camera.Target,
            new(diagnostic, viewport.Width, viewport.Height, Gameplay.WorldCells.Value), hooks);
    }

    /// <summary>제품 미션 경로의 Commit·Present만 조립한다. Pan·저장은 R6까지 미조립으로 둔다.</summary>
    /// <param name="commit">호출자가 미리 묶은 Commit 장치다. 비어 있으면 세션 실제 장치와 R5 명시 거부로 채운다.</param>
    /// <param name="present">호출자가 미리 묶은 Present 장치다. 비어 있으면 세션 실제 장치와 R5 명시 거부로 채운다.</param>
    /// <remarks>2026-11-26 1.064에서 R5-1 회귀 교정용으로 추가했다. 매 프레임 포인터 폴링이 미조립 Pan의 명시 실패에 닿아 활성 창 미션 실행이 깨졌으므로, 제품 경로는 Commit·Present만 조립하고 Pan·저장은 미조립(false 반환)으로 둔다. 근거: cpppj UberGump.cpp 1895행의 PollCameraPan은 R6 입력 결선에서 실제 장치로 잇는다.</remarks>
    public void BuildCameraFrameDevices(RawCameraCommitHooks? commit = null, RawCameraPresentHooks? present = null)
    {
        if (_cameraCommit is not null || _cameraPresent is not null) throw new InvalidOperationException("세션 카메라 프레임 장치 중복 연결");
        RawCameraCommitHooks live = commit ?? new();
        live.LockCursor ??= _ => { Gameplay.Cursor.LockDepth++; };
        live.UnlockCursor ??= () =>
        {
            if (Gameplay.Cursor.LockDepth == 0) throw new InvalidOperationException("세션 커서 잠금 underflow(R6 미리보기 재생성 경계)");
            Gameplay.Cursor.LockDepth--;
        };
        live.RedrawRoots ??= () => CameraMissing<object>("카메라 Gump 다시 그리기 실제 장치 미연결(R5)");
        live.Scroll ??= (_, _, _, _, _, _) => CameraMissing<object>("카메라 화면 이동 실제 장치 미연결(R5)");
        live.ScrollBackground ??= (_, _, _, _, _, _) => CameraMissing<object>("카메라 배경 이동 실제 장치 미연결(R5)");
        live.AddDirty ??= (_, _) => CameraMissing<object>("카메라 변경 영역 실제 장치 미연결(R5)");
        live.AddBackgroundDirty ??= _ => CameraMissing<object>("카메라 배경 변경 영역 실제 장치 미연결(R5)");
        live.UpdateMouse ??= point => { Gameplay.Cursor.ScreenX = point.X; Gameplay.Cursor.ScreenY = point.Y; };
        _cameraCommit = live;

        RawCameraPresentHooks shown = present ?? new();
        shown.Flip ??= () => CameraMissing<object>("카메라 뒤집기 실제 장치 미연결(R5)");
        shown.Commit ??= () => CommitCameraNow();
        _cameraPresent = shown;
    }

    /// <summary>같은 세션 카메라에서 Pan 본체를 실행한다. 미조립이면 거짓이다.</summary>
    /// <param name="pointer">소비할 포인터 사건이다.</param>
    /// <returns>원문 이동 전용 큐 경로이면 참이다.</returns>
    public bool ProcessCameraPan(InputEvent pointer)
    {
        if (_cameraPan is null) return false;
        return RawCameraPan.ProcessCameraPan(pointer, _cameraPan);
    }

    /// <summary>같은 세션 카메라에서 저장 카메라 분배를 실행한다. 미조립이면 거짓이다.</summary>
    /// <param name="input">소비할 입력 사건이다.</param>
    /// <returns>숫자 호출·저장·불러오기·무시이면 참이다.</returns>
    public bool DispatchSavedCameraKey(InputEvent input)
    {
        if (_savedCamera is null) return false;
        return RawSavedCameraKey.DispatchSavedCameraKey(input, Gameplay.Camera.SavedPositions, _savedCamera);
    }

    /// <summary>현재 view와 이동량으로 같은 세션 카메라 목표를 쓴다.</summary>
    /// <param name="dx">X 이동량이다.</param>
    /// <param name="dy">Y 이동량이다.</param>
    /// <param name="height">현재 논리 화면 높이다.</param>
    public void RequestCameraDelta(int dx, int dy, int height) =>
        RawCameraPan.RequestCameraDelta(Gameplay.Camera.RendererView, Gameplay.Camera.Target, height, dx, dy, LocalState.MetaRebuild);
}
