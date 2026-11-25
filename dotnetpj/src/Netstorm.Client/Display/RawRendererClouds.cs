// 원문4977f0/497860과499bfd..499cfe의 구름 사각형 저장·시차 갱신·배경 선택이다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>실제 4칸과 같은 count를 빌리며 함수가 임의로 초기화하지 않는다.</summary>
public sealed class RendererCloudQueue
{
    /// <summary>네 사각형이다. 유효 범위 밖 쓰기는 호스트 오류로 구별한다.</summary>
    public ScreenRect[] Rectangles { get; } = new ScreenRect[4];
    /// <summary>현재 signed 비교 대상 DWORD다.</summary>
    public UIntBox Count { get; } = new();
}

/// <summary>실제 진단/색 지우기/구름 픽셀은 선택한 경로에서만 요구한다.</summary>
public sealed class RendererCloudHooks
{
    /// <summary>cloudDirtyRectCount &lt; NUM_CLOUD_DIRTY_RECT 진단 행을 받는다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>같은 현재 네 좌표/전체 색 DWORD로 지운다.</summary>
    public Action<ScreenRect, uint>? Clear { get; set; }
    /// <summary>구름 픽셀을 그린다. 참은 별도 시차 표면이며 같은 장치로 속이지 않는다.</summary>
    public Action<ScreenRect, int, int, bool>? Draw { get; set; }
}

/// <summary>배경 선택 효과 뒤에도 같은 현재 화면·색·카메라를 참조한다.</summary>
public sealed class RendererBackdropGlobals
{
    /// <summary>현재 화면 flags다.</summary>
    public uint Mode { get; set; }
    /// <summary>묶음 숨김 값이다.</summary>
    public uint HideGroups { get; set; }
    /// <summary>현재 날씨색이다.</summary>
    public uint Tint { get; set; }
    /// <summary>기본 검정색이다.</summary>
    public uint Black { get; set; }
    /// <summary>현재 snapshot 가로 원점이다.</summary>
    public int CameraX { get; set; }
    /// <summary>현재 snapshot 세로 원점이다.</summary>
    public int CameraY { get; set; }
    /// <summary>실제 signed 분모이며 Q16 배율과 구별한다.</summary>
    public int Denominator { get; set; }
}

/// <summary>화면 배경 선택의 현재 전역·원문 signed 카운터/IDIV와 선택된 외부 효과만 처리한다.</summary>
/// <remarks>cpppj client/RawRendererClouds.cpp 대응이다. 픽셀 장치는 같은 원문 순서에서 호출자가 제공한다. 2026-11-26 1.066에서 R5-2d1용으로 추가했다.</remarks>
public static class RawRendererClouds
{
    /// <summary>원문 DWORD 비트를 signed 비교로만 해석한다.</summary>
    /// <param name="value">DWORD 비트다.</param>
    /// <returns>signed 값이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>선택된 위치의 실제 나눗셈 오류를 미정의 동작이나 가짜 0 성공으로 바꾸지 않는다.</summary>
    /// <param name="value">분자다.</param>
    /// <param name="denominator">분모다.</param>
    /// <returns>절삭 몫이다.</returns>
    private static int Divide(int value, int denominator)
    {
        if (denominator == 0 || (value == int.MinValue && denominator == -1))
        {
            throw new DivideByZeroException("구름시차원문IDIV오류");
        }
        return value / denominator;
    }

    /// <summary>원문 EBP/EDI의 right/bottom은 먼저 보존하고 left/top만 호출 직전에 읽는다.</summary>
    /// <param name="clipped">지도 clip 사각형이다.</param>
    /// <returns>현재 사각형이다.</returns>
    private static ScreenRect CurrentRect(ScreenRect clipped) => new(clipped.Left, clipped.Top, clipped.Right, clipped.Bottom);

    /// <summary>사각형 4칸의 개수와 원문 진단 뒤 재조회를 보존하며 음수 index의 앞선 메모리 쓰기는 물리 계약 오류로 구별한다.</summary>
    /// <param name="queue">구름 dirty 4칸이다.</param>
    /// <param name="rect">저장할 사각형이다.</param>
    /// <param name="checking">현재 진단 DWORD다.</param>
    /// <param name="hooks">구름 장치다.</param>
    public static void AppendRendererCloudDirty(RendererCloudQueue queue, ScreenRect rect, uint checking, RendererCloudHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(queue);
        ArgumentNullException.ThrowIfNull(hooks);
        if (checking != 0 && Signed(queue.Count.Value) >= 4)
        {
            (hooks.Assertion ?? throw new InvalidOperationException("구름진단장치미연결"))(0x40cU);
        }
        int index = Signed(queue.Count.Value);
        if (index >= 4)
        {
            return;
        }
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(queue), "구름4칸 앞의 미제공 물리 영역 쓰기입니다");
        }
        queue.Rectangles[index] = rect;
        queue.Count.Value++;
    }

    /// <summary>각 표시 효과 뒤 현재 count를 다시 읽으며 다음 사각형/카메라·분모는 다음 반복에서 조회한다.</summary>
    /// <param name="queue">구름 dirty 4칸이다.</param>
    /// <param name="cameraX">현재 카메라 가로다.</param>
    /// <param name="cameraY">현재 카메라 세로다.</param>
    /// <param name="denominator">실제 signed 분모다.</param>
    /// <param name="hooks">구름 장치다.</param>
    public static void RefreshRendererCloudBackground(RendererCloudQueue queue, int cameraX, int cameraY, int denominator, RendererCloudHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(queue);
        ArgumentNullException.ThrowIfNull(hooks);
        // signed 0/음수 개수는 표시 없이 마지막 count 0만 수행한다
        for (int index = 0; index < Signed(queue.Count.Value); index++)
        {
            if (index >= 4)
            {
                throw new ArgumentOutOfRangeException(nameof(queue), "구름4칸 뒤의 미제공 물리 영역 읽기");
            }
            int divisor = denominator;
            int dy = Divide(cameraY, divisor);
            int dx = Divide(cameraX, divisor);
            (hooks.Draw ?? throw new InvalidOperationException("구름그리기장치미연결"))(queue.Rectangles[index], dx, dy, true);
        }
        queue.Count.Value = 0;
    }

    /// <summary>debug 묶음 숨김의 검정 지우기는 UI 전용/배경 덮임 검사보다 먼저 실행하며 그 효과 뒤 현재 관문을 다시 읽는다.</summary>
    /// <param name="clipped">지도 clip 사각형이다.</param>
    /// <param name="dirtyFlags">갱신 계층 플래그다.</param>
    /// <param name="covered">수집한 항목이 배경 영역을 덮는지.</param>
    /// <param name="globals">현재 화면·색·카메라다.</param>
    /// <param name="hooks">구름 장치다.</param>
    public static void DrawRendererBackdrop(ScreenRect clipped, uint dirtyFlags, uint covered, RendererBackdropGlobals globals, RendererCloudHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.HideGroups != 0)
        {
            (hooks.Clear ?? throw new InvalidOperationException("구름지우기장치미연결"))(CurrentRect(clipped), globals.Black);
        }
        if ((dirtyFlags & 4U) != 0 || covered != 0)
        {
            return;
        }
        uint flags = globals.Mode & 255U;
        if ((flags & 8U) == 0)
        {
            (hooks.Clear ?? throw new InvalidOperationException("구름지우기장치미연결"))(CurrentRect(clipped), globals.Tint);
            return;
        }
        if ((flags & 16U) != 0 && (flags & 1U) != 0 && (flags & 2U) == 0)
        {
            (hooks.Clear ?? throw new InvalidOperationException("구름지우기장치미연결"))(CurrentRect(clipped), 255);
            return;
        }
        // 두 경로 모두 원문의 Y→X 읽기/나눗셈 순서로 채운다
        int y, x;
        if ((flags & 16U) != 0)
        {
            int currentY = globals.CameraY;
            int divisor = globals.Denominator;
            y = Divide(currentY, divisor);
            x = Divide(globals.CameraX, divisor);
        }
        else
        {
            y = globals.CameraY;
            x = globals.CameraX;
        }
        (hooks.Draw ?? throw new InvalidOperationException("구름그리기장치미연결"))(CurrentRect(clipped), x, y, false);
    }
}
