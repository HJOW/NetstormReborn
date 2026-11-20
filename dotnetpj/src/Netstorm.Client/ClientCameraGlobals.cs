using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 장기 snapshot과 논리 Renderer 현재 view/목표를 별개 주소에서 보관한다.
namespace Netstorm.Client;

/// <summary>Renderer 앞64BYTE 밖의 목표와 한계다.</summary>
public sealed class RawCameraTargetState
{
    /// <summary>+48/4c의 칸 크기×월드 크기−화면 크기다.</summary>
    public int[] Limits { get; } = new int[2];
    /// <summary>+50/54의 다음 적용할 signed 픽셀 원점이다.</summary>
    public int[] Pixels { get; } = new int[2];
    /// <summary>+58의 다음 Q16 배율이다.</summary>
    public uint Zoom { get; set; }
}

/// <summary>00540ce0/ce4의 별도 배경 분모이며 view.zoom과 합치지 않는다.</summary>
public sealed class RawRendererDenominatorState
{
    /// <summary>인자0일 때 고르는 원문 최초15다.</summary>
    public int DefaultValue { get; set; } = 15;
    /// <summary>원문 store 위치에서 쓰는 현재 분모이며 최초15다.</summary>
    public int Current { get; set; } = 15;
}

/// <summary>Client 수명의 두64BYTE와 목표·배경 분모·최근 배치·저장 위치 소유자다.</summary>
public sealed class ClientCameraGlobals
{
    /// <summary>최초 BSS0인 실제 전역 snapshot이다.</summary>
    public RawCameraViewState Snapshot { get; } = new();
    /// <summary>별개 주소인 논리 Native Renderer의 앞64BYTE다.</summary>
    public RawCameraViewState RendererView { get; } = new();
    /// <summary>현재 view와 구분되는 목표/한계다.</summary>
    public RawCameraTargetState Target { get; } = new();
    /// <summary>최초15/15의 별도 분모다.</summary>
    public RawRendererDenominatorState Denominator { get; } = new();
    /// <summary>Tab의 최근40BYTE와 별도 순번의 단일 장기 소유자다.</summary>
    public ConstructionHistoryState ConstructionHistory { get; } = new();
    /// <summary>숫자 키의 별도80BYTE 장기 소유자다.</summary>
    public SavedCameraPositions SavedPositions { get; } = new();
    /// <summary>4993c0의 view→목표/한계→snapshot→redraw1→revision++ 순서다.</summary>
    public void InitializeNativeRenderer(int width, int height, int worldCells, UIntBox worldRedraw, UIntBox mapRevision)
    {
        if (width <= 0 || height <= 0) throw new ArgumentOutOfRangeException(nameof(width), "NativeRenderer초기화면크기오류");
        RawCameraView.Initialize(RendererView); float cells = worldCells;
        Target.Limits[0] = RawCameraView.FtolLow((double)RendererView.CellPixels[0] * cells - width); Target.Zoom = RendererView.Zoom;
        Target.Limits[1] = RawCameraView.FtolLow((double)RendererView.CellPixels[1] * cells - height); Target.Pixels[0] = 0; Target.Pixels[1] = 0;
        RawCameraView.CopySnapshot(Snapshot, RendererView); worldRedraw.Value = 1; mapRevision.Value = unchecked(mapRevision.Value + 1);
    }
    /// <summary>현재 view 정수 칸 크기로 두 한계만 다시 계산한다.</summary>
    public void UpdateTargetLimits(int width, int height, float worldWidth, float worldHeight)
    {
        Target.Limits[0] = RawCameraView.FtolLow((double)RendererView.CellPixels[0] * worldWidth - width); Target.Limits[1] = RawCameraView.FtolLow((double)RendererView.CellPixels[1] * worldHeight - height);
    }
    /// <summary>목표만 쓰며 현재 Renderer/snapshot을 변경하지 않는다.</summary>
    public void RequestPixels(int x, int y) { Target.Pixels[0] = x; Target.Pixels[1] = y; }
    /// <summary>PATCH497f20의 음수16/11 곱→ftol→NEG/SUB 뒤 revision을 증가시킨다.</summary>
    public void RequestCenter(float x, float y, int width, int height, UIntBox revision)
    {
        Target.Pixels[0] = unchecked(-RawCameraView.FtolLow(-16.0 * x) - width / 2); Target.Pixels[1] = unchecked(-RawCameraView.FtolLow(-11.0 * y) - height / 2); revision.Value = unchecked(revision.Value + 1);
    }
    /// <summary>433aa0의 redraw 증가→목표 이동→실제 commit 순서이며 누락도 선행 쓰기를 보존한다.</summary>
    public void CenterImmediately(float x, float y, int width, int height, UIntBox worldRedraw, UIntBox revision, Action? commit)
    {
        worldRedraw.Value = unchecked(worldRedraw.Value + 1); RequestCenter(x, y, width, height, revision); (commit ?? throw new InvalidOperationException("즉시카메라실제497960장치미연결"))();
    }
    /// <summary>497960의 두 setter 조각이며 scroll/dirty/커서 통지 전체 부모와 구분한다.</summary>
    public void ApplyPixelsToRendererAndSnapshot(int x, int y) { RawCameraView.SetPixels(RendererView, x, y); RawCameraView.SetPixels(Snapshot, x, y); }
    /// <summary>436450 프레임 말미의 명시16DWORD 복사이며 목표는 복사하지 않는다.</summary>
    public void CopyRendererSnapshot() => RawCameraView.CopySnapshot(Snapshot, RendererView);
    /// <summary>0이면 default를 고르고 변경된 현재 분모를 먼저 쓴 뒤 redraw1을 쓴다.</summary>
    public void SelectDenominator(int value, UIntBox worldRedraw)
    {
        if (value == 0) value = Denominator.DefaultValue; if (value != Denominator.Current) { Denominator.Current = value; worldRedraw.Value = 1; }
    }
}
