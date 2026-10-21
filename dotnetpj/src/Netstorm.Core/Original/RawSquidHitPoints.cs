namespace Netstorm.Core.Original;

/// <summary>
/// 일반 자산의 HP 를 raw 슬롯에 쓰는 몸체(원본 SetRawSquidHitPoints, 004adca0 ↔ CD 004aed20).
/// 패치판은 26 번 자리에 signed DWORD, CD 는 signed WORD 를 쓴다.
/// 근거: cpppj/src/o/RawSquidHitPoints.cpp (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public static class RawSquidHitPoints
{
    /// <summary>슬롯 안 HP 필드의 위치.</summary>
    private const int HealthOffset = 26;

    /// <summary>
    /// 일반 자식의 HP 를 현재 raw 슬롯에 쓴다. 10.78 은 signed DWORD 를 쓴다.
    /// </summary>
    /// <param name="pool">대상 슬롯 풀.</param>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="value">쓸 HP 값.</param>
    public static void Set(RawSidPool pool, int sid, int value)
    {
        ArgumentNullException.ThrowIfNull(pool);
        Span<byte> raw = pool.AllocatedBytes(sid);
        // 낮은 바이트부터 네 바이트를 쓴다
        uint bits = unchecked((uint)value);
        for (int i = 0; i < 4; i++)
        {
            raw[HealthOffset + i] = (byte)(bits >> (i * 8));
        }
    }
}
