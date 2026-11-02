namespace Netstorm.Core.Original;

/// <summary>
/// 팔레트 전환이 빌리는 고정 이름 버퍼와 현재 DWORD 전역이다(cpppj <c>o/RawFortPaletteTransition.h</c>의 <c>FortPaletteTransitionGlobals</c>).
/// 원문의 고정 주소 span과 같은 Client 명시 DWORD 참조를 대리자 쌍으로 옮겼다.
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 버퍼는 호출자가 소유하며 사본을 만들지 않는다.</remarks>
public sealed class FortPaletteTransitionGlobals
{
    /// <summary>005c8910의 실제 팔레트 이름 256바이트다. 같은 물리 버퍼를 빌리며 NUL 포함 복사 대상이다.</summary>
    public Memory<byte> PaletteName { get; set; }

    /// <summary>0056b7a0의 실제 구름 이름 256바이트다. 팔레트 이름과 별개 주소다.</summary>
    public Memory<byte> CloudName { get; set; }

    /// <summary>00569ffc의 현재 Kencloud 할당 주소·크기를 읽는다.</summary>
    public Func<uint>? GetCache { get; set; }

    /// <summary>00569ffc에 쓴다. 해제 뒤 부모가 무조건 0으로 쓴다.</summary>
    public Action<uint>? SetCache { get; set; }

    /// <summary>00540a80의 현재 개수를 읽는다. build가 바꾼 현재 개수까지 반영하며 원문 ESI 값을 캡처한다.</summary>
    public Func<uint>? Count { get; set; }

    /// <summary>0059a8bc의 현재 렌더러 존재 DWORD를 읽는다. maps 뒤에 있는 dirty 판정에만 쓴다.</summary>
    public Func<uint>? Renderer { get; set; }

    /// <summary>0059a8b0의 현재 dirty를 읽는다. 렌더러가 0이면 기존 값을 보존한다.</summary>
    public Func<uint>? GetDirty { get; set; }

    /// <summary>0059a8b0에 쓴다.</summary>
    public Action<uint>? SetDirty { get; set; }
}

/// <summary>
/// 두 NUL 종료 이름의 ASCII 대소문자를 무시하고 비교한다. 같으면 0을 돌려준다.
/// </summary>
/// <param name="first">첫째 이름의 NUL 포함 버퍼 구간이다.</param>
/// <param name="second">둘째 이름의 NUL 포함 버퍼 구간이다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 원문 CRT 비교의 0·비영 분기만 관측한다.</remarks>
public delegate int PaletteNameComparer(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second);

/// <summary>
/// 같은 입력 주소로 현재 NUL 경로를 빌린다. 경로는 다음 LoadPalette까지 유효해야 한다.
/// </summary>
/// <param name="key">원문 "GamePalSpec" 고정 키다.</param>
/// <param name="source">같은 입력 주소의 NUL 포함 이름이다.</param>
/// <param name="first">원문 push 1의 첫째 인자(항상 0)다.</param>
/// <param name="second">다음 LoadPalette의 둘째 인자(항상 0)다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 비어 있는 반환은 물리 거부 없이 빈 경로로 둔다.</remarks>
public delegate byte[] PalettePathResolver(string key, ReadOnlySpan<byte> source, uint first, uint second);

/// <summary>
/// 현재 경로의 팔레트를 arg2=1 위치 경계로 읽는다.
/// </summary>
/// <param name="path">현재 NUL 경로다.</param>
/// <param name="clear">원문 둘째 인자(항상 1)다. 기존 arg2=0 경로만으로는 대체하지 않는다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다.</remarks>
public delegate void PaletteLoader(ReadOnlySpan<byte> path, uint clear);

/// <summary>
/// 이름 계산·Screen 직접 호출 외 실제 팔레트·구름·경로·위치·표시·dirty를 같은 순서로 실행한다(cpppj <c>o/RawFortPaletteTransition.h</c>의 <c>FortPaletteTransitionHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortPaletteTransitionHooks
{
    /// <summary>004c2630(29,"init palette\n")이다. 비교 캡처보다 먼저 호출한다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>CRT 004e65ed의 현재 두 C 문자열 비교다. 0이면 조기 반환한다.</summary>
    public PaletteNameComparer? Compare { get; set; }

    /// <summary>004e4354의 현재 캐시 주소 해제다. 복구 뒤 부모는 반드시 같은 DWORD를 0으로 쓴다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>00471040의 실제 계산·할당 부모다. cache가 0일 때만 호출하며 성공을 가정하지 않는다.</summary>
    public Action? BuildCloud { get; set; }

    /// <summary>00470b20을 캡처한 count만큼 호출한다. callback 중 count·cache가 바뀌어도 같은 캡처 값을 쓴다.</summary>
    public Action? StepCloud { get; set; }

    /// <summary>004410f0("GamePalSpec",같은 입력 주소,0,0)의 현재 NUL 경로다.</summary>
    public PalettePathResolver? PathSpec { get; set; }

    /// <summary>004a4850(path,1)의 실제 위치 경계다.</summary>
    public PaletteLoader? LoadPalette { get; set; }

    /// <summary>0043c010의 실제 추측·계산·준비 갱신 경계다.</summary>
    public Action? BuildMaps { get; set; }
}

/// <summary>
/// 원본 00470c30·00471190·004973c0·004b6100의 팔레트 이름 전환·구름·dirty를 복원한다(cpppj <c>o/RawFortPaletteTransition</c>).
/// 전체 길이 계산의 중복 생성을 피하고 원문 이름·캐시·위치 출력을 같은 순서로 보존한다.
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다.</remarks>
public static class RawFortPaletteTransition
{
    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 로그·복사의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">팔레트 전환의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedPalette<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("팔레트 전환 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 원문 길이를 미리 계산하지 않고 한 BYTE씩 앞에서 복사한다. 겹치는 버퍼의 물리 중단 순서를 보존한다.
    /// </summary>
    /// <param name="target">복사 대상 256바이트 구간이다.</param>
    /// <param name="source">복사 원본 구간이다. NUL 직후에만 끝난다.</param>
    /// <exception cref="ArgumentOutOfRangeException">팔레트 이름 입력·출력 물리 범위다.</exception>
    private static void CopyNameForward(Span<byte> target, ReadOnlySpan<byte> source)
    {
        // 원문은 길이 판단이나 memcpy가 없고 NUL 바이트 직후에만 끝난다
        for (int index = 0; ; index++)
        {
            if (index >= source.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(source), "팔레트 이름 입력 물리 범위");
            }
            byte value = source[index];
            if (index >= target.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(target), "팔레트 이름 출력 물리 범위");
            }
            target[index] = value;
            if (value == 0)
            {
                return;
            }
        }
    }

    /// <summary>
    /// 00470c30: 비영 현재 캐시를 free하고 항상 복구 뒤 같은 DWORD를 0으로 쓴다.
    /// </summary>
    /// <param name="globals">고정 이름 버퍼와 현재 DWORD 전역이다.</param>
    /// <param name="hooks">실제 해제 장치다.</param>
    public static void ReleaseFortPaletteCloudCache(FortPaletteTransitionGlobals globals, FortPaletteTransitionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        uint current = NeedPalette(globals.GetCache)();
        if (current != 0)
        {
            NeedPalette(hooks.Release)(current);
            NeedPalette(globals.SetCache)(0u);
        }
    }

    /// <summary>
    /// 00471190: 같은 source 주소에서 NUL까지 전방위로 복사하고, 필요하면 build 뒤 현재 count만큼 step을 호출한다.
    /// </summary>
    /// <param name="source">같은 입력 주소의 NUL 포함 이름이다.</param>
    /// <param name="globals">고정 이름 버퍼와 현재 DWORD 전역이다.</param>
    /// <param name="hooks">실제 계산·진행 장치다.</param>
    public static void SetFortPaletteCloudName(ReadOnlySpan<byte> source, FortPaletteTransitionGlobals globals, FortPaletteTransitionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        CopyNameForward(globals.CloudName.Span, source);
        if (NeedPalette(globals.GetCache)() == 0)
        {
            NeedPalette(hooks.BuildCloud)();
        }
        uint remaining = NeedPalette(globals.Count)();
        // 매 step이 count·cache를 바꿔도 같은 캡처 값을 쓴다
        while (remaining != 0)
        {
            NeedPalette(hooks.StepCloud)();
            remaining--;
        }
    }

    /// <summary>
    /// 004973c0: 현재 렌더러 DWORD가 비영인 경우에만 dirty를 1로 만든다.
    /// </summary>
    /// <param name="globals">고정 이름 버퍼와 현재 DWORD 전역이다.</param>
    public static void MarkFortPaletteDirty(FortPaletteTransitionGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals);
        if (NeedPalette(globals.Renderer)() != 0)
        {
            NeedPalette(globals.SetDirty)(1u);
        }
    }

    /// <summary>
    /// 004b6100: log 뒤 입력 span 변환을 캡처한다. 이후 사용은 같은 물리로 계속 보이며 비교 중단은 전파하지 않는다.
    /// </summary>
    /// <param name="argument">진입 때 전달받은 실제 입력 버퍼 구간이다.</param>
    /// <param name="globals">고정 이름 버퍼와 현재 DWORD 전역이다.</param>
    /// <param name="hooks">실제 비교·구름·경로·위치·표시·dirty 장치다.</param>
    public static void TransitionFortPalette(ReadOnlySpan<byte> argument, FortPaletteTransitionGlobals globals, FortPaletteTransitionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        NeedPalette(hooks.Log)(29u, "init palette\n");
        ReadOnlySpan<byte> source = argument;
        if (NeedPalette(hooks.Compare)(globals.PaletteName.Span, source) == 0)
        {
            return;
        }
        CopyNameForward(globals.PaletteName.Span, source);
        ReleaseFortPaletteCloudCache(globals, hooks);
        SetFortPaletteCloudName(source, globals, hooks);
        byte[] path = NeedPalette(hooks.PathSpec)("GamePalSpec", source, 0u, 0u);
        NeedPalette(hooks.LoadPalette)(path, 1u);
        NeedPalette(hooks.BuildMaps)();
        MarkFortPaletteDirty(globals);
    }
}
