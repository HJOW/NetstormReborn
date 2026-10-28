namespace Netstorm.Client.Display;

/// <summary>
/// 파일 팔레트 읽기 뒤 원본 004a4923..004a49a2 의 clear 관문이 부르는 하위 장치 묶음. 실제 <see cref="Screen"/>이 제공하며,
/// 연결하지 않은 장치(null)에 도달하면 빈 성공으로 바꾸지 않고 <see cref="InvalidOperationException"/>을 던진다.
/// 근거: cpppj/src/client/ScreenPaletteApply.h. (2026-10-28 R1에서 이식)
/// </summary>
public sealed class ScreenPaletteApplyHooks
{
    /// <summary>004a14e0. 중첩 깊이와 같은 현재 그리기 표면을 잠근다.</summary>
    public Action? Lock { get; set; }

    /// <summary>004a2060(0,0,현재 폭,현재 높이,0). lock 뒤의 현재 크기를 읽어 검정으로 채운다.</summary>
    public Action? FillBlack { get; set; }

    /// <summary>깊이를 줄이고 0이면 표면을 푼다. 푼 뒤 버퍼·행 간격 0은 실제 Screen이 소유한다.</summary>
    public Action? Unlock { get; set; }

    /// <summary>004a1fb0. GDI 경로는 bit1 관문에서 자연 반환하며 DirectDraw는 필수 장치다.</summary>
    public Action? WaitVerticalBlank { get; set; }

    /// <summary>004a46d0. viewport Update 전후에 기존 clip을 보존한다.</summary>
    public Action? PresentPreservingClip { get; set; }

    /// <summary>004a16d0. flipping이 0이면 자연 반환하고 실제 flip은 필수 장치다.</summary>
    public Action? Flip { get; set; }

    /// <summary>하위 장치 하나를 호출한다. 연결돼 있지 않으면 예외로 알린다.</summary>
    internal static void Need(Action? device, string name)
    {
        if (device is null) throw new InvalidOperationException($"팔레트 전환 하위 장치 미연결: {name}");
        device();
    }
}

/// <summary>원본 팔레트 clear 관문. 근거: cpppj/src/client/ScreenPaletteApply.h, fixture screen-palette-clear-x86.tsv. (2026-10-28 R1에서 이식)</summary>
public static class ScreenPaletteApply
{
    /// <summary>
    /// loaded는 파일을 읽은 뒤 항상 1이 된다. clear가 0이 아니고 현재 flags도 0이 아닐 때만 필수 하위 장치를 원문 순서(lock → fillBlack → unlock →
    /// waitVerticalBlank → presentPreservingClip → flip)로 호출한다. 호출 도중 예외는 이전 부분 효과를 되돌리지 않고 그대로 전달한다.
    /// </summary>
    /// <param name="clear">지우기 요청 값.</param>
    /// <param name="flags">현재 화면 모드 플래그(관문 판정에 한 번만 읽는다).</param>
    /// <param name="loaded">팔레트 읽음 DWORD(00555e08).</param>
    /// <param name="hooks">하위 장치 묶음.</param>
    public static void ApplyScreenPaletteClear(uint clear, uint flags, ref uint loaded, ScreenPaletteApplyHooks hooks)
    {
        loaded = 1;
        if (clear != 0 && flags != 0)
        {
            ScreenPaletteApplyHooks.Need(hooks.Lock, nameof(hooks.Lock));
            ScreenPaletteApplyHooks.Need(hooks.FillBlack, nameof(hooks.FillBlack));
            ScreenPaletteApplyHooks.Need(hooks.Unlock, nameof(hooks.Unlock));
            ScreenPaletteApplyHooks.Need(hooks.WaitVerticalBlank, nameof(hooks.WaitVerticalBlank));
            ScreenPaletteApplyHooks.Need(hooks.PresentPreservingClip, nameof(hooks.PresentPreservingClip));
            ScreenPaletteApplyHooks.Need(hooks.Flip, nameof(hooks.Flip));
        }
    }
}
