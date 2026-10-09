namespace Netstorm.Core.Display;

/// <summary>원본 두 판본의 선택 표시 확장 방식 (위쪽 여유가 다르다)</summary>
public enum DisplayEdition
{
    /// <summary>패치 10.78: 위쪽에 15픽셀을 더한다</summary>
    Patch1078,

    /// <summary>CD 10.72: 위쪽에 9픽셀을 더한다</summary>
    CD1072,
}

/// <summary>화면 픽셀 사각형 (원본 SquidDisplayRect. 오른쪽·아래 끝은 포함하지 않는다)</summary>
/// <param name="Left">왼쪽 끝</param>
/// <param name="Top">위쪽 끝</param>
/// <param name="Right">오른쪽 끝 (제외)</param>
/// <param name="Bottom">아래쪽 끝 (제외)</param>
public readonly record struct DisplayRect(int Left, int Top, int Right, int Bottom)
{
    /// <summary>폭 (오른쪽 − 왼쪽)</summary>
    public int Width => Right - Left;

    /// <summary>높이 (아래 − 위)</summary>
    public int Height => Bottom - Top;

    /// <summary>변경 표에 올리는 양의 면적이 있는지 (원본 Nonempty: 왼쪽 &lt; 오른쪽이고 위 &lt; 아래)</summary>
    public bool HasArea => Left < Right && Top < Bottom;
}

/// <summary>
/// 오브젝트의 화면 경계 계산. 원본 10.78 의 SHP 경계(00498ff0)·주 변경 등록(004990d0)을 옮겼다
/// (docs/exe/cpp-display-reconstruction.md. 기준 구현은 cpppj/src/o/SquidDisplay.cpp).
/// VFX 압축 사각형이 아니라 Squid 추가 헤더의 표시 폭·높이·hotspot(부호 있는 16비트)을 쓴다.
/// 화면 좌표는 trunc(x × 16 + 0.5) − 카메라X, trunc(y × 11 + 0.5) − 카메라Y 에서 hotspot 을 뺀다.
/// 폭·높이는 표시 폭·높이 + 1 이다. 선택 표시는 폭 19 미만이면 양쪽 9, 항상 양쪽 3, 위쪽 여유(패치 15·CD 9)를 더한다.
/// 그림자 프레임(타입 플래그1 0x40000)은 프레임 수 + 현재 프레임이며 확장을 적용하지 않는다.
/// 변경 표(dirty)·화면 합성·클릭 판정은 호출자가 처리한다. 뷰어의 선택/체력 상자는 BoundsInView를 쓴다.
/// 검증은 원본 기계어 기대값 display-x86.tsv 의 Bounds 행으로 한다.
/// </summary>
public static class DisplayBounds
{
    /// <summary>월드 x → 화면 픽셀 배율</summary>
    public const double ScreenScaleX = 16;

    /// <summary>월드 y → 화면 픽셀 배율</summary>
    public const double ScreenScaleY = 11;

    /// <summary>화면 좌표 투영에 더하는 반올림 오프셋 (0.5, 0 쪽으로 자른다)</summary>
    public const double ProjectionBias = 0.5;

    /// <summary>Q16 고정소수점의 1.0 (화면 확대율 단위)</summary>
    public const int ZoomOne = 65536;

    /// <summary>선택 표시의 좁은 폭 기준 (이보다 좁으면 양쪽에 9픽셀을 더한다)</summary>
    public const int NarrowWidth = 19;

    /// <summary>좁은 그림에 양쪽으로 더하는 선택 여유</summary>
    public const int NarrowMargin = 9;

    /// <summary>선택한 그림에 항상 양쪽으로 더하는 여유</summary>
    public const int SelectionMargin = 3;

    /// <summary>패치판이 선택한 그림의 위쪽에 더하는 여유</summary>
    public const int SelectionTopPatch = 15;

    /// <summary>CD판이 선택한 그림의 위쪽에 더하는 여유</summary>
    public const int SelectionTopCd = 9;

    /// <summary>그림자 프레임을 쓰는 타입의 플래그1 비트</summary>
    public const uint ShadowFlag1 = 0x40000;

    /// <summary>
    /// 월드 좌표를 화면 픽셀로 투영한다 (원본 Project: x87 처럼 double 에서 x × 배율 + 0.5 를 계산하고 0 쪽으로 자른 뒤 카메라를 뺀다).
    /// float 곱셈으로 먼저 반올림하지 않는다.
    /// </summary>
    /// <param name="value">월드 좌표</param>
    /// <param name="scale">축별 배율 (x 16, y 11)</param>
    /// <param name="camera">카메라 화면 좌표</param>
    public static int Project(float value, double scale, int camera)
    {
        // double 에서 곱셈·덧셈을 하고 0 쪽으로 자른다.
        double pixel = (double)value * scale + ProjectionBias;
        // 유한하지 않거나 32비트에 못 들어가는 좌표는 원본 배열 밖 접근이므로 거부한다.
        if (!double.IsFinite(pixel) || pixel < int.MinValue || pixel > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "화면 좌표가 범위를 벗어났습니다");
        }
        // 카메라를 뺀 값도 32비트에 들어가는지 확인한다 (원본 Coordinate).
        long screen = (long)pixel - camera;
        if (screen < int.MinValue || screen > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(camera), "화면 좌표가 범위를 벗어났습니다");
        }
        return (int)screen;
    }

    /// <summary>
    /// Q16 고정소수점으로 표시 치수를 화면 확대율에 맞춘다 (원본 Scale: imul 하위 DWORD 와 산술 오른쪽 이동).
    /// </summary>
    /// <param name="value">Squid 헤더의 부호 있는 치수 (표시 폭·높이·hotspot)</param>
    /// <param name="zoom">화면 확대율 (Q16, 1.0 = 65536)</param>
    public static int Scale(short value, int zoom)
    {
        // 부호 있는 64비트 곱을 32비트 패턴으로 자르고 산술 이동한다.
        uint product = unchecked((uint)((long)value * zoom));
        return unchecked((int)product) >> 16;
    }

    /// <summary>
    /// 오브젝트 한 프레임의 화면 경계 (원본 Bounds. 표시 영역 잘라내기·선택 확장은 호출자가 한다).
    /// </summary>
    /// <param name="width">Squid 표시 폭</param>
    /// <param name="height">Squid 표시 높이</param>
    /// <param name="hotspotX">Squid hotspot x</param>
    /// <param name="hotspotY">Squid hotspot y</param>
    /// <param name="x">오브젝트 월드 x 좌표</param>
    /// <param name="y">오브젝트 월드 y 좌표</param>
    /// <param name="cameraX">카메라 화면 x</param>
    /// <param name="cameraY">카메라 화면 y</param>
    /// <param name="zoom">화면 확대율 (Q16)</param>
    public static DisplayRect Bounds(short width, short height, short hotspotX, short hotspotY,
        float x, float y, int cameraX, int cameraY, int zoom)
    {
        // 화면 위치에서 Q16 hotspot 을 뺀 곳이 왼쪽·위쪽 끝이다.
        int left = ToCoordinate((long)Project(x, ScreenScaleX, cameraX) - Scale(hotspotX, zoom));
        int top = ToCoordinate((long)Project(y, ScreenScaleY, cameraY) - Scale(hotspotY, zoom));
        // 오른쪽·아래 끝은 Q16 표시 치수에 1을 더한 곳이다.
        int right = ToCoordinate((long)left + Scale(width, zoom) + 1);
        int bottom = ToCoordinate((long)top + Scale(height, zoom) + 1);
        return new DisplayRect(left, top, right, bottom);
    }

    /// <summary>
    /// 클론 뷰어의 카메라 중심·확대율을 반영한 표시 상자다.
    /// 원본 월드 투영을 먼저 절삭하고 뷰어 이동/확대를 적용한다. 이동 뒤 0.5를 더하면 화면 밖 음수 좌표가 1픽셀 어긋난다.
    /// 1배·정수 카메라/중심에서는 Bounds의 원본 계산에 중심 이동만 더한 결과와 같다.
    /// </summary>
    /// <param name="width">Squid 표시 폭</param>
    /// <param name="height">Squid 표시 높이</param>
    /// <param name="hotspotX">Squid hotspot x</param>
    /// <param name="hotspotY">Squid hotspot y</param>
    /// <param name="x">기준점 보정을 포함한 월드 x 좌표</param>
    /// <param name="y">기준점 보정을 포함한 월드 y 좌표</param>
    /// <param name="cameraX">뷰어 카메라의 원본 픽셀 x 좌표</param>
    /// <param name="cameraY">뷰어 카메라의 원본 픽셀 y 좌표</param>
    /// <param name="centerX">논리 화면 중심 x</param>
    /// <param name="centerY">논리 화면 중심 y</param>
    /// <param name="zoom">뷰어 확대율 Q16</param>
    public static DisplayRect BoundsInView(short width, short height, short hotspotX, short hotspotY,
        float x, float y, double cameraX, double cameraY, double centerX, double centerY, int zoom)
    {
        if (zoom <= 0) throw new ArgumentOutOfRangeException(nameof(zoom), "표시 확대율은 양수여야 합니다");
        double factor = zoom / (double)ZoomOne;
        int anchorX = ViewCoordinate((Project(x, ScreenScaleX, 0) - cameraX) * factor + centerX);
        int anchorY = ViewCoordinate((Project(y, ScreenScaleY, 0) - cameraY) * factor + centerY);
        int left = ToCoordinate((long)anchorX - Scale(hotspotX, zoom));
        int top = ToCoordinate((long)anchorY - Scale(hotspotY, zoom));
        return new DisplayRect(left, top, ToCoordinate((long)left + Scale(width, zoom) + 1),
            ToCoordinate((long)top + Scale(height, zoom) + 1));
    }

    /// <summary>뷰어에서 변환한 기준점을 범위 확인 후 절삭한다. 원본 투영의 0.5를 다시 더하지 않는다.</summary>
    /// <param name="value">카메라·중심·확대를 반영한 픽셀 좌표</param>
    private static int ViewCoordinate(double value)
    {
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(value), "뷰어 표시 좌표가 범위를 벗어났습니다");
        return (int)value;
    }

    /// <summary>
    /// 경계를 표시 영역으로 자른다 (원본 Clip: 각 모서리를 독립적으로 고정한다).
    /// </summary>
    /// <param name="rect">자를 사각형</param>
    /// <param name="viewport">표시 영역 (왼쪽·위·오른쪽·아래)</param>
    public static DisplayRect Clip(DisplayRect rect, DisplayRect viewport) => new(
        Math.Clamp(rect.Left, viewport.Left, viewport.Right),
        Math.Clamp(rect.Top, viewport.Top, viewport.Bottom),
        Math.Clamp(rect.Right, viewport.Left, viewport.Right),
        Math.Clamp(rect.Bottom, viewport.Top, viewport.Bottom));

    /// <summary>
    /// 선택한 오브젝트의 경계를 넓힌다 (원본 Update 의 marker 경로. 잘라낸 뒤에 넓히며 다시 자르지 않는다).
    /// </summary>
    /// <param name="rect">표시 영역으로 자른 경계</param>
    /// <param name="edition">위쪽 여유가 다른 판본</param>
    public static DisplayRect ExpandForSelection(DisplayRect rect, DisplayEdition edition)
    {
        int left = rect.Left;
        int top = rect.Top;
        int right = rect.Right;
        int bottom = rect.Bottom;
        // 폭이 19 미만이면 양쪽에 9픽셀을 더한다.
        if ((long)right - left < NarrowWidth)
        {
            left = ToCoordinate((long)left - NarrowMargin);
            right = ToCoordinate((long)right + NarrowMargin);
        }
        // 위쪽 여유는 판본별로 다르다.
        top = ToCoordinate((long)top - (edition == DisplayEdition.Patch1078 ? SelectionTopPatch : SelectionTopCd));
        // 양쪽에는 항상 3픽셀을 더한다.
        left = ToCoordinate((long)left - SelectionMargin);
        right = ToCoordinate((long)right + SelectionMargin);
        return new DisplayRect(left, top, right, bottom);
    }

    /// <summary>
    /// 실제로 그릴 물리 프레임 번호. 그림자 타입(플래그1 0x40000)은 프레임 수 + 현재 프레임이다.
    /// </summary>
    /// <param name="flags1">타입 플래그1</param>
    /// <param name="frameCount">타입의 프레임 수</param>
    /// <param name="frame">현재 프레임</param>
    public static int ShadowFrameIndex(uint flags1, int frameCount, int frame) =>
        (flags1 & ShadowFlag1) != 0 ? frame + frameCount : frame;

    /// <summary>64비트 화면 좌표를 32비트 범위로 확인한다 (원본 Coordinate).</summary>
    /// <param name="value">화면 좌표</param>
    private static int ToCoordinate(long value)
    {
        // 32비트에 못 들어가는 좌표는 거부한다.
        if (value < int.MinValue || value > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "화면 좌표가 범위를 벗어났습니다");
        }
        return (int)value;
    }
}
