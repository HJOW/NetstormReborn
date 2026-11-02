namespace Netstorm.Core.Original;

/// <summary>
/// 원본 높이 조회가 사용하는 실제 청크 지역·영역 판정·쓰기 경계다(cpppj <c>o/RawFortRegionSave.h</c>의 <c>FortRegionSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortRegionSaveHooks
{
    /// <summary>0054c8a4·0054c8a8의 현재 signed DWORD를 원본 읽기 위치에서 조회한다.</summary>
    public Func<int>? ChunksX { get; set; }

    /// <summary>현재 높이 DWORD를 원본 읽기 위치에서 조회한다.</summary>
    public Func<int>? ChunksY { get; set; }

    /// <summary>00432180의 실제 청크 조회며 Chaff는 반환값만 버리고 쓰기 효과는 실행한다.</summary>
    public Func<int, int, byte>? ChunkRegion { get; set; }

    /// <summary>0046f200: 청크의 signed BYTE와 고정 owner·region을 전달한다.</summary>
    public Func<sbyte, uint, int, uint>? Allowed { get; set; }

    /// <summary>004be020: 고정 stream에 해당 청크 x·y를 전달한다.</summary>
    public Action<uint, int, int>? Chunk { get; set; }
}

/// <summary>
/// 전체 Territory·Chaff 조회는 원본 높이·scope·선택·정렬·쓰기 효과만 남긴다(cpppj <c>o/RawFortRegionSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public static class RawFortRegionSave
{
    /// <summary>
    /// 실제 선택된 기본 경계를 요구하며 매 지점에서 전달하지 않는 장치를 먼저 거부한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">전체 Fort 청크 저장의 실제 경계 누락이다.</exception>
    private static T NeedRegion<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("전체 Fort 청크 저장 실제 경계 누락");
        }
        return function;
    }

    /// <summary>
    /// 원본 INC와 같은 signed DWORD 감산 셈이다. C++ signed overflow 동작을 보존한다.
    /// </summary>
    /// <param name="coordinate">늘릴 좌표다.</param>
    private static int NextRegionCoordinate(int coordinate)
    {
        return unchecked((int)(unchecked((uint)coordinate) + 1u));
    }

    /// <summary>
    /// 원본 높이 조회가 끝난 뒤에 scope를 쓰고 Territory는 signed BYTE 영역 판정을 실행한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="owner">적을 소유자다.</param>
    /// <param name="region">적을 영역 값이다.</param>
    /// <param name="scope">공유 scope다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 조회·판정·쓰기 경계다.</param>
    /// <param name="territory">Territory면 참, Chaff면 거짓이다.</param>
    private static void SaveRegionChunks(uint stream, uint owner, int region, ref uint scope, FortRegionSaveHooks hooks, bool territory)
    {
        int height = NeedRegion(hooks.ChunksY)();
        scope = territory ? 1u : 2u;
        if (height <= 0)
        {
            return;
        }
        int width = NeedRegion(hooks.ChunksX)();
        int y = 0;
        // 끝 청크의 동작과 다음 너비는 매번 getter로 다시 읽는다
        do
        {
            int x = 0;
            if (width > 0)
            {
                do
                {
                    byte cell = NeedRegion(hooks.ChunkRegion)(x, y);
                    if (!territory || NeedRegion(hooks.Allowed)(unchecked((sbyte)cell), owner, region) != 0)
                    {
                        NeedRegion(hooks.Chunk)(stream, x, y);
                    }
                    width = NeedRegion(hooks.ChunksX)();
                    x = NextRegionCoordinate(x);
                }
                while (x < width);
            }
            height = NeedRegion(hooks.ChunksY)();
            y = NextRegionCoordinate(y);
        }
        while (y < height);
    }

    /// <summary>
    /// 004be230: 현재 height 읽기로 scope 1을 먼저 세우고 모든 청크를 적용한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="owner">적을 소유자다.</param>
    /// <param name="region">선택 영역 값이다.</param>
    /// <param name="scope">공유 scope다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 조회·판정·쓰기 경계다.</param>
    public static void SaveFortTerritoryChunks(uint stream, uint owner, int region, ref uint scope, FortRegionSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        SaveRegionChunks(stream, owner, region, ref scope, hooks, true);
    }

    /// <summary>
    /// Chaff는 실제 청크 getter 반환값을 쓰고 영역 판정·변경 효과는 Territory와 같은 위치에서 비교한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="scope">공유 scope다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 조회·쓰기 경계다.</param>
    public static void SaveFortChaffChunks(uint stream, ref uint scope, FortRegionSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        SaveRegionChunks(stream, 0, 0, ref scope, hooks, false);
    }
}
