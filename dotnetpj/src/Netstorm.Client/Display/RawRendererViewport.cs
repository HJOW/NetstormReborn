// 같은 dirty·지도 사각형의 월드 환산과 지형 행 제한을 현재 분수 카메라로 계산한다.
namespace Netstorm.Client.Display;

/// <summary>원문 앞단·배경 판정·구름 표시/정리의 같은 장기 owner에 연결한다.</summary>
/// <remarks>cpppj ClientRendererFrame.cpp 39~56행의 WorldRect·ClipWorld·RendererCollectionViewport·RendererCollectionRowLimit 대응이다. 화면 투영과 실제 SHP 그리기는 후속이며 이 계층은 raw 월드 키 환산만 제출한다. 2026-11-26 1.063에서 R5-2a용으로 추가했다.</remarks>
public static class RawRendererViewport
{
    /// <summary>원문 1/16의 단정도 가로 배율이다.</summary>
    private const float XScale = 0.0625f;
    /// <summary>원문 503b54/504194의 단정도 1/11이다. double 1/11로 대체하지 않는다.</summary>
    private static readonly float InverseY = System.BitConverter.UInt32BitsToSingle(0x3dba2e8c);
    /// <summary>화면+16/+11의 실제 32비트 감김이다.</summary>
    /// <param name="value">원본 화면 좌표다.</param>
    /// <param name="delta">더할 오프셋이다.</param>
    /// <returns>감김을 유지한 signed 합이다.</returns>
    private static int Add(int value, uint delta) => unchecked((int)((uint)value + delta));
    /// <summary>499758 이후의 가시 월드 계산이며 픽셀 원점에서 분수값을 재구성하지 않는다.</summary>
    /// <param name="rect">화면 사각형이다.</param>
    /// <param name="view">현재 분수 카메라 snapshot이다.</param>
    /// <returns>월드 사각형이다.</returns>
    public static Netstorm.Core.Original.RenderWorldRect WorldRect(ScreenRect rect, RawCameraViewState view)
    {
        System.ArgumentNullException.ThrowIfNull(view);
        float baseX = System.BitConverter.UInt32BitsToSingle(view.WorldBits[0]);
        float baseY = System.BitConverter.UInt32BitsToSingle(view.WorldBits[1]);
        // 원문은 (pixel+16)/+11 오프셋 뒤 FILD/FMUL/FADD/FSTP32다. 각 축 오프셋을 보존한다.
        float left = (float)((double)Add(rect.Left, 16) * XScale + baseX);
        float top = (float)((double)Add(rect.Top, 11) * InverseY + baseY);
        float right = (float)((double)Add(rect.Right, 16) * XScale + baseX);
        float bottom = (float)((double)Add(rect.Bottom, 11) * InverseY + baseY);
        return new(left, top, right, bottom);
    }
    /// <summary>4970c0의 각 좌표 제한 순서다.</summary>
    /// <param name="rect">제한할 월드 사각형이다.</param>
    /// <param name="bounds">현재 전체 지도 경계다.</param>
    /// <returns>경계 안에 제한한 월드 사각형이다.</returns>
    public static Netstorm.Core.Original.RenderWorldRect ClipWorld(Netstorm.Core.Original.RenderWorldRect rect, Netstorm.Core.Original.RenderWorldRect bounds)
    {
        static float Clamp(float value, float low, float high) => System.Math.Max(low, System.Math.Min(value, high));
        return new(Clamp(rect.Left, bounds.Left, bounds.Right), Clamp(rect.Top, bounds.Top, bounds.Bottom), Clamp(rect.Right, bounds.Left, bounds.Right), Clamp(rect.Bottom, bounds.Top, bounds.Bottom));
    }
    /// <summary>현재 분수 카메라로 두 사각형을 환산하고 원문 순서로 지도 안에 제한한다.</summary>
    /// <param name="dirty">실제 dirty 사각형이다.</param>
    /// <param name="map">실제 지도 영역이다.</param>
    /// <param name="view">현재 분수 카메라 snapshot이다.</param>
    /// <returns>수집에 쓰는 월드 사각형이다.</returns>
    public static Netstorm.Core.Original.RenderWorldRect CollectionViewport(ScreenRect dirty, ScreenRect map, RawCameraViewState view)
    {
        System.ArgumentNullException.ThrowIfNull(view);
        return ClipWorld(WorldRect(dirty, view), WorldRect(map, view));
    }
    /// <summary>양수 화면의 곱셈도 원문 32비트 저장 뒤 signed 나눗셈 순서를 유지한다.</summary>
    /// <param name="width">게임 화면의 픽셀 가로다.</param>
    /// <param name="height">게임 화면의 픽셀 세로다.</param>
    /// <returns>지형 행 제한이다.</returns>
    public static int CollectionRowLimit(int width, int height)
    {
        return unchecked((int)((uint)(width / 16) * (uint)Add(height, 10))) / 11;
    }
}
