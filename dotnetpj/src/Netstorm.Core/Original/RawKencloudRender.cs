// 원문 구름 갱신/dirty/행 조회와 두 표시 부모를 같은 packed 전역에 실행한다.
// 화면 장치는 호출자가 제공하며 계산·반복·쓰기 순서는 원문 몸체다. cpppj o/RawKencloudRender.cpp/.h 의 직역이다. (0.381, 2026-11-11)
namespace Netstorm.Core.Original;

/// <summary>
/// 표시·갱신 부모가 빌리는 같은 packed 전역과 현재 속도다.
/// <see cref="State"/>는 005697f0부터 2054 DWORD의 이전 행/캐시/현재 행/flags가 같은 물리이며,
/// <see cref="Speed"/>는 00540a80의 현재 값을 읽는다(갱신 부모는 생성 복귀 뒤에만 다시 읽는다).
/// </summary>
/// <remarks>0.381에서 추가했다.</remarks>
public sealed class KencloudRenderGlobals
{
    /// <summary>같은 연속 전역 2054 DWORD이며 복제하지 않는다.</summary>
    public uint[] State { get; }

    /// <summary>현재 cloudSpeed를 읽는 장치다(읽을 때마다 현재 값이다).</summary>
    public Func<uint> Speed { get; }

    /// <summary>빌릴 전역과 속도 조회자로 만든다.</summary>
    /// <param name="state">같은 packed 연속 전역이다.</param>
    /// <param name="speed">현재 cloudSpeed 조회자다.</param>
    public KencloudRenderGlobals(uint[] state, Func<uint> speed)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(speed);
        State = state;
        Speed = speed;
    }
}

/// <summary>
/// 갱신·표시 부모가 선택된 시점에만 요구하는 장치 묶음이다. 자연 생략 경로에는 장치를 요구하지 않는다.
/// </summary>
/// <remarks>0.381에서 추가했다.</remarks>
public sealed class KencloudRenderHooks
{
    /// <summary>기존 실제 생성(00471040)을 선택된 시점에 합성한다.</summary>
    public Action? Build { get; set; }

    /// <summary>기존 실제 행 이동(00470b20)을 선택된 시점에 합성한다.</summary>
    public Action? Step { get; set; }

    /// <summary>현재 005c7a44..50의 clip 네 DWORD(left, top, right, bottom)를 읽는다.</summary>
    public Func<uint[]>? Clip { get; set; }

    /// <summary>004a1580의 현재 clip 저장 효과다.</summary>
    public Action<uint[]>? SetClip { get; set; }

    /// <summary>004a14e0(false)/004a15d0(true)의 별개 잠금 장치다.</summary>
    public Action<bool>? Lock { get; set; }

    /// <summary>004a1550(false)/004a1650(true)의 별개 해제 장치다.</summary>
    public Action<bool>? Unlock { get; set; }

    /// <summary>004a1fe0의 destX, destY, cache, stride, sourceL, sourceT, sourceR, sourceB 원시 인자 8개다.</summary>
    public Action<uint[]>? Blit { get; set; }
}

/// <summary>
/// 원본 구름의 갱신(00470c50)·dirty 표시(00470c80)·정리(00470ca0)·행 조회(00470ce0)·두 표시 부모(004711d0/00471410)다.
/// cpppj <c>o/RawKencloudRender.cpp</c>의 직역이며 DWORD 32비트 감김·signed 나머지·현재 값 재조회 순서를 보존한다.
/// </summary>
/// <remarks>0.381에서 추가했다(고정 Native 944관측 <c>kencloud-render-x86.tsv</c> 재생).</remarks>
public static class RawKencloudRender
{
    /// <summary>cache 주소 DWORD 위치(515)다.</summary>
    private const int CacheIndex = 515;

    /// <summary>현재 행 dirty 표 시작 위치(516)다.</summary>
    private const int RowsIndex = 516;

    /// <summary>전체 표시 요청 위치(2051)다.</summary>
    private const int AllIndex = 2051;

    /// <summary>부분 표시 요청 위치(2052)다.</summary>
    private const int PartialIndex = 2052;

    /// <summary>
    /// 연속 전역의 한 DWORD에 접근한다. 물리 범위를 벗어나면 이전 쓰기를 되돌리지 않고 그 시점에 예외를 던진다.
    /// </summary>
    /// <param name="state">전체 물리 배열이다.</param>
    /// <param name="index">DWORD 위치다.</param>
    private static ref uint At(uint[] state, int index)
    {
        if ((uint)index >= (uint)state.Length) throw new ArgumentOutOfRangeException(nameof(index), "구름 표시 현재 DWORD 물리 범위");
        return ref state[index];
    }

    /// <summary>원문 signed 비교를 위해 DWORD를 부호 있는 값으로 읽는다.</summary>
    /// <param name="value">DWORD 값이다.</param>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원문 signed 나머지(음수는 음의 나머지)를 DWORD 비트로 돌려준다.</summary>
    /// <param name="value">DWORD 값이다.</param>
    private static uint Remainder(uint value) => unchecked((uint)(Signed(value) % 512));

    /// <summary>선택된 장치가 없으면 원문 미연결 오류로 거부한다(자연 생략 경로에는 호출하지 않는다).</summary>
    /// <typeparam name="T">장치 형식이다.</typeparam>
    /// <param name="device">필요한 장치다.</param>
    private static T Need<T>(T? device) where T : class =>
        device ?? throw new InvalidOperationException("구름 표시 선택된 실제 장치 누락");

    /// <summary>
    /// 00470c50: cache가 없으면 생성하고 그때만 속도를 다시 읽은 뒤, 캡처한 횟수만큼 행 이동을 실행한다.
    /// 하위가 현재 speed를 바꿔도 이미 캡처한 반복 수는 유지한다.
    /// </summary>
    /// <param name="globals">같은 packed 전역과 속도 조회자다.</param>
    /// <param name="hooks">생성·이동 장치다.</param>
    public static void UpdateKencloud(KencloudRenderGlobals globals, KencloudRenderHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        uint count = globals.Speed();
        if (At(globals.State, CacheIndex) == 0)
        {
            Need(hooks.Build)();
            count = globals.Speed(); // 기존 캐시는 최초 속도만 캡처한다.
        }
        // 캡처한 횟수만큼 행을 이동한다.
        for (; count != 0; count--) Need(hooks.Step)();
    }

    /// <summary>00470c80: 전체 표시 요청을 1로, 부분 표시 요청을 0으로 쓴다(이 순서다). 첫 store 뒤 중단은 그대로 남는다.</summary>
    /// <param name="state">같은 packed 전역이다.</param>
    public static void MarkKencloudDirty(uint[] state)
    {
        ArgumentNullException.ThrowIfNull(state);
        At(state, AllIndex) = 1;
        At(state, PartialIndex) = 0;
    }

    /// <summary>00470ca0: 실제 표시된 이전 행만 순서대로 지운다.</summary>
    /// <param name="state">같은 packed 전역이다.</param>
    public static void CleanKencloudDirty(uint[] state)
    {
        ArgumentNullException.ThrowIfNull(state);
        // 이전 행 표 512칸을 순서대로 훑어 표시된 행의 현재·이전 요청을 지운다.
        for (int index = 0; index < 512; index++)
        {
            if (At(state, index) != 0)
            {
                At(state, RowsIndex + index) = 0;
                At(state, index) = 0;
                At(state, AllIndex) = 0;
            }
        }
        if (At(state, PartialIndex) != 0) { At(state, AllIndex) = 0; At(state, PartialIndex) = 0; } // 전체 표시가 남긴 부분 플래그도 마지막에 정리한다.
    }

    /// <summary>
    /// 00470ce0: (first+second)의 signed 나머지 행이 dirty이거나 전체 표시 요청이 있으면 1이다.
    /// 음수 행은 앞선 같은 물리 DWORD(cache 등 인접 alias)를 읽는다.
    /// </summary>
    /// <param name="state">같은 packed 전역이다.</param>
    /// <param name="first">첫째 인자(행)다.</param>
    /// <param name="second">둘째 인자(카메라 Y)다.</param>
    /// <returns>dirty면 1, 아니면 0이다.</returns>
    public static uint KencloudRowDirty(uint[] state, uint first, uint second)
    {
        ArgumentNullException.ThrowIfNull(state);
        int row = Signed(Remainder(unchecked(first + second)));
        return At(state, RowsIndex + row) != 0 || At(state, AllIndex) != 0 ? 1U : 0U;
    }

    /// <summary>
    /// 004711d0(기본)/00471410(alternate)의 표시: clip을 영역으로 줄이고 잠근 뒤 512 타일을 전체 또는 dirty 행 단위로 복사하고 되돌린다.
    /// </summary>
    /// <param name="globals">같은 packed 전역과 속도 조회자다.</param>
    /// <param name="area">left, top, right, bottom, x, y 여섯 DWORD다(x·y는 카메라 원점).</param>
    /// <param name="hooks">생성·clip·잠금·복사 장치다.</param>
    /// <param name="alternate">true면 두 번째 표시 부모(004a15d0/1650 잠금)다.</param>
    public static void DrawKencloud(KencloudRenderGlobals globals, uint[] area, KencloudRenderHooks hooks, bool alternate = false)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(area);
        ArgumentNullException.ThrowIfNull(hooks);
        if (area.Length != 6) throw new ArgumentException("구름 표시 영역은 여섯 DWORD다", nameof(area));
        uint left = area[0], top = area[1], right = area[2], bottom = area[3], x = area[4], y = area[5];
        if (unchecked((right - left) * (bottom - top)) == 0) return; // 원문 곱의 하위 32비트가 0이면 장치도 조회하지 않는다.
        if (At(globals.State, CacheIndex) == 0) Need(hooks.Build)();
        uint[] previous = Need(hooks.Clip)();
        Need(hooks.SetClip)([left, top, right, bottom]);
        uint firstX = unchecked((left + x) - (Remainder(unchecked(left + x)) + x));
        uint firstY = unchecked((top + y) - (Remainder(unchecked(top + y)) + y));
        uint lastX = unchecked((x + 511U + right) - (Remainder(unchecked(x + 511U + right)) + x));
        uint lastY = unchecked((bottom + 511U + y) - (Remainder(unchecked(bottom + 511U + y)) + y)); // x86 덧셈/뺄셈의 실제 감김이다.
        Need(hooks.Lock)(alternate);
        // 각 세로 타일에서 현재 allDirty를 다시 읽는다.
        for (uint tileY = firstY, rowBottom = unchecked(1U - firstY); Signed(tileY) < Signed(lastY); tileY = unchecked(tileY + 512U), rowBottom = unchecked(rowBottom - 512U))
        {
            if (At(globals.State, AllIndex) == 0)
            {
                // dirty 행만 골라 각 세로 타일의 해당 행을 복사한다. dirty 값은 각 실제 행 접근 때 조회한다.
                for (uint row = 0; row < 512; row++)
                {
                    if (At(globals.State, RowsIndex + (int)row) == 0) continue;
                    // 각 복사 전에 현재 cache를 조회한다.
                    for (uint tileX = firstX; Signed(tileX) < Signed(lastX); tileX = unchecked(tileX + 512U))
                        Need(hooks.Blit)([tileX, unchecked(tileY + row), At(globals.State, CacheIndex), 512, 0, row, 512, unchecked(rowBottom + tileY + row)]);
                    At(globals.State, (int)row) = 1; // 가로 복사가 없어도 선택된 행의 이전 표시 요청을 남긴다.
                }
            }
            else
            {
                // 전체 경로도 callback 뒤 현재 cache를 읽는다.
                for (uint tileX = firstX; Signed(tileX) < Signed(lastX); tileX = unchecked(tileX + 512U))
                    Need(hooks.Blit)([tileX, tileY, At(globals.State, CacheIndex), 512, 0, 0, 512, 512]);
                At(globals.State, PartialIndex) = 1; // 현재 세로 타일의 가로 복사가 끝난 뒤에만 저장한다.
            }
        }
        Need(hooks.Unlock)(alternate);
        Need(hooks.SetClip)(previous); // 예외 때 이 후속 효과를 가짜로 실행하지 않는다.
    }
}
