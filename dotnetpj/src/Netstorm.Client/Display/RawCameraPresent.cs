using Netstorm.Client;

namespace Netstorm.Client.Display;

// 실제 Present 말미의 진단 관문·두 목표 제한과 뒤 적용 호출의 순서를 보존한다.
public sealed class RawCameraPresentGlobals
{
    /// <summary>현재 DWORD이며 정확 1에서 이후 호출을 건너뛴다.</summary>
    public uint Diagnostic { get; }
    /// <summary>현재 논리 화면 폭이다.</summary>
    public int Width { get; }
    /// <summary>현재 논리 화면 높이다.</summary>
    public int Height { get; }
    /// <summary>현재 signed 세계 크기이며 Renderer의 두 limit과 별개다.</summary>
    public int WorldCells { get; }

    /// <summary>Present 본체가 읽는 현재 전역 값을 묶는다.</summary>
    /// <param name="diagnostic">현재 DWORD이며 정확 1에서 이후 호출을 건너뛴다.</param>
    /// <param name="width">현재 논리 화면 폭이다.</param>
    /// <param name="height">현재 논리 화면 높이다.</param>
    /// <param name="worldCells">현재 signed 세계 크기다.</param>
    public RawCameraPresentGlobals(uint diagnostic, int width, int height, int worldCells)
    {
        Diagnostic = diagnostic; Width = width; Height = height; WorldCells = worldCells;
    }
}

/// <summary>장치 출력과 현재 논리 카메라 데이터를 구별한다.</summary>
public sealed class RawCameraPresentHooks
{
    /// <summary>실제 장치 경계다. 진단 1이면 요구하지 않는다.</summary>
    public Action? Flip { get; set; }
    /// <summary>두 제한 뒤 같은 Renderer의 전체를 호출한다. 현재 view를 본체에서 대신 변경하지 않는다.</summary>
    public Action? Commit { get; set; }
}

/// <summary>49a081의 실제 후처리만 실행하며 전체 자체와 구별한다.</summary>
public static class RawCameraPresent
{
    /// <summary>실제 호출 지점에 도달한 장치만 요구하며 진단의 조기 반환을 앞서 바꾸지 않는다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="callback">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("카메라Present현재장치미연결");

    /// <summary>원문은 signed 최소값에서 0과 비교한 뒤 FTOL 순서다. int 입력은 double에서 정확히 표현된다.</summary>
    /// <param name="value">제한할 목표 픽셀이다.</param>
    /// <param name="maximum">현재 상한이다.</param>
    /// <returns>0 이상 상한 이하의 절삭값이다.</returns>
    private static int Limit(int value, int maximum) => RawCameraView.FtolLow(Math.Max(0, Math.Min(value, maximum)));

    /// <summary>IMUL/SUB의 하위32비트 결과를 signed overflow 없이 같은 signed 상한으로 해석한다.</summary>
    /// <param name="cells">현재 signed 세계 크기다.</param>
    /// <param name="scale">축별 기본 칸 크기다.</param>
    /// <param name="screen">현재 논리 화면 크기다.</param>
    /// <returns>세계 한계 상한이다.</returns>
    private static int WorldLimit(int cells, uint scale, int screen) => unchecked((int)(unchecked((uint)cells) * scale - unchecked((uint)screen)));

    /// <summary>원문은 진단을 한 번만 읽고 뒤의 현재 한계·목표·세계/화면 크기를 순서대로 사용한다.</summary>
    /// <param name="target">같은 Renderer의 한계/목표다.</param>
    /// <param name="globals">현재 전역 값이다.</param>
    /// <param name="hooks">실제 장치 경계와 전체 호출이다.</param>
    /// <remarks>2026-11-26 1.048에서 DI-4b 카메라 Present용으로 추가했다. 근거: cpppj RawCameraPresent.cpp의 PresentCameraTarget(49a081)이다. 전체 자체는 호출자가 소유한다.</remarks>
    public static void PresentCameraTarget(RawCameraTargetState target, RawCameraPresentGlobals globals, RawCameraPresentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Diagnostic == 1u) return;
        Need(hooks.Flip)();
        target.Pixels[0] = Limit(target.Pixels[0], target.Limits[0]); target.Pixels[1] = Limit(target.Pixels[1], target.Limits[1]);
        int maximumX = WorldLimit(globals.WorldCells, 16u, globals.Width), maximumY = WorldLimit(globals.WorldCells, 11u, globals.Height);
        target.Pixels[0] = Limit(target.Pixels[0], maximumX); target.Pixels[1] = Limit(target.Pixels[1], maximumY);
        Need(hooks.Commit)();
    }
}
