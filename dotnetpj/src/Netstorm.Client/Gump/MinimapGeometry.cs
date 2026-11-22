using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 실제 Sky Overview의 crop과 카메라 목표를 계산한다.

/// <summary>같은 미니맵 노드의 원본 크기와+a0/+a4/+a8 상태다.</summary>
public sealed class MinimapGeometryState
{
    /// <summary>배율 적용 전 실제 가로 크기다.</summary>
    public float Width { get; set; } = 128;
    /// <summary>배율 적용 전 실제 세로 크기다.</summary>
    public float Height { get; set; } = 128;
    /// <summary>공유128폭 bitmap의 signed 가로 원점이다.</summary>
    public int CropX { get; set; }
    /// <summary>공유128폭 bitmap의 signed 세로 원점이다.</summary>
    public int CropY { get; set; }
    /// <summary>실제 캡처 중 자동 crop 추적을 중단하는 DWORD다.</summary>
    public uint Scrolling { get; set; }
}

/// <summary>동일 시점의 실제 카메라 비트·지도 영역·칸 배율이다.</summary>
/// <param name="CameraX">0059a900의 float 비트다.</param>
/// <param name="CameraY">0059a904의 float 비트다.</param>
/// <param name="Viewport">실제 지도 표시 영역이다.</param>
/// <param name="PixelsX">현재 signed 가로 칸 배율이다.</param>
/// <param name="PixelsY">현재 signed 세로 칸 배율이다.</param>
public readonly record struct MinimapGeometryView(uint CameraX, uint CameraY, ScreenRect Viewport, int PixelsX = 16, int PixelsY = 11)
{
    /// <summary>원본 기본 생성의 현재 칸 배율16/11을 유지한다. 실제 Root는 현재 지도 영역과 카메라를 공급한다.</summary>
    public MinimapGeometryView() : this(0, 0, default, 16, 11) { }
}

/// <summary>원본 crop 블릿과 오른쪽/아래를 제외한 카메라 외곽선이다.</summary>
/// <param name="Source">같은 bitmap의 원점이다.</param>
/// <param name="Destination">실제 노드의 화면 원점이다.</param>
/// <param name="Size">원본 ftol한 블릿 크기다.</param>
/// <param name="Camera">004a24b0에 전달할 사각형이다.</param>
public readonly record struct MinimapDrawPlan(ScreenPoint Source, ScreenPoint Destination, ScreenPoint Size, ScreenRect Camera);

/// <summary>cpppj/src/client/MinimapGeometry.cpp의 다섯 기하 본체다. 외부 카메라/입력 효과는 호출자가 수행한다.</summary>
public static class MinimapGeometry
{
    /// <summary>실제 bitmap 한 변이며 현재 노드 크기와 구별한다.</summary>
    private const double BitmapSide = 128;
    /// <summary>원본 signed64 ftol의 하위DWORD이며 integer-indefinite는0이다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>원문의 비교 순서를 유지해 NaN에서도 첫 인자를 보존한다.</summary>
    private static double Min(double a, double b) => b < a ? b : a;
    /// <summary>원문의 비교 순서를 유지해 NaN에서도 첫 인자를 보존한다.</summary>
    private static double Max(double a, double b) => a < b ? b : a;
    /// <summary>카메라 ftol과 지도 원점의 정수 나눗셈을 더한 뒤2로 나눈다.</summary>
    private static ScreenPoint Center(MinimapGeometryView view)
    {
        if (view.PixelsX <= 0 || view.PixelsY <= 0) throw new ArgumentException("미니맵의 유효한 현재 칸 배율이 필요합니다", nameof(view));
        return new(unchecked(Truncate(BitConverter.UInt32BitsToSingle(view.CameraX)) + view.Viewport.Left / view.PixelsX) / 2,
            unchecked(Truncate(BitConverter.UInt32BitsToSingle(view.CameraY)) + view.Viewport.Top / view.PixelsY) / 2);
    }
    /// <summary>실제 지도 폭/높이를 현재 배율의 두 배로 signed 나눈다.</summary>
    private static ScreenPoint Visible(MinimapGeometryView view) => new(unchecked(view.Viewport.Right - view.Viewport.Left) / unchecked(view.PixelsX * 2), unchecked(view.Viewport.Bottom - view.Viewport.Top) / unchecked(view.PixelsY * 2));
    /// <summary>00480270. 드래그 중에는 상태를 보존하고 현재 지도 주변의 crop만 갱신한다.</summary>
    public static bool Rebuild(MinimapGeometryState state, MinimapGeometryView view)
    {
        if (state.Scrolling != 0) return false;
        var center = Center(view); var visible = Visible(view);
        int candidateX = unchecked(center.X - Truncate(((double)state.Width - visible.X) * 0.5));
        int candidateY = unchecked(center.Y - Truncate(((double)state.Height - visible.Y) * 0.5));
        state.CropX = Truncate(Max(0, Min((float)candidateX, BitmapSide - state.Width)));
        state.CropY = Truncate(Max(0, Min((float)candidateY, BitmapSide - state.Height)));
        return true;
    }
    /// <summary>004803f0. signed crop+local을 확장한 뒤 두 배하고 마지막에만 float로 저장한다.</summary>
    public static (float X, float Y) Target(MinimapGeometryState state, float localX, float localY) => ((float)(((double)state.CropX + localX) * 2), (float)(((double)state.CropY + localY) * 2));
    /// <summary>00480440. 바깥 드래그의 축별 crop4 이동 뒤 같은 좌표의 목표를 반환한다.</summary>
    public static (float X, float Y) Drag(MinimapGeometryState state, float localX, float localY)
    {
        if (localX <= 0) state.CropX = Math.Max(0, unchecked(state.CropX - 4));
        if (state.Width <= localX) state.CropX = Truncate(Min((float)unchecked(state.CropX + 4), BitmapSide - state.Width));
        if (localY <= 0) state.CropY = Math.Max(0, unchecked(state.CropY - 4));
        if ((double)state.Height - 1 <= localY) state.CropY = Truncate(Min((float)unchecked(state.CropY + 4), BitmapSide - state.Height));
        return Target(state, localX, localY);
    }
    /// <summary>004805e0. crop+크기가128을 넘은 축만 보정하며 음수 crop은 별도로 지우지 않는다.</summary>
    public static void ClampAttachment(MinimapGeometryState state)
    {
        if (BitmapSide < (double)(float)state.CropX + state.Width) state.CropX = Truncate(BitmapSide - state.Width);
        if (BitmapSide < (double)(float)state.CropY + state.Height) state.CropY = Truncate(BitmapSide - state.Height);
    }
    /// <summary>00480660. 같은 crop/지도 영역으로 블릿과 외곽선을 계산하며 clip과 실제 색은 호출자가 공급한다.</summary>
    public static MinimapDrawPlan PlanDraw(MinimapGeometryState state, MinimapGeometryView view, ScreenPoint destination)
    {
        var center = Center(view); var visible = Visible(view);
        int left = unchecked(destination.X + center.X - state.CropX), top = unchecked(destination.Y + center.Y - state.CropY);
        return new(new(state.CropX, state.CropY), destination, new(Truncate(Min(BitmapSide, state.Width)), Truncate(Min(BitmapSide, state.Height))), new(left, top, unchecked(left + visible.X), unchecked(top + visible.Y)));
    }
}
