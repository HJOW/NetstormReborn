namespace Netstorm.Core.Rules;

/// <summary>
/// 생산 창(덱)의 유닛을 배치한 뒤 다시 쓸 수 있게 되는 예약 간격 (docs/exe/production-refresh.md).
/// </summary>
public static class ProductionTimers
{
    /// <summary>기본 경로(useProductTimers = 0, Cursor.cpp 004468b0)의 Unit Rate 인덱스별 간격(초): Slow·Medium·Fast</summary>
    private static readonly double[] DefaultIntervals = [10, 5, 1];

    /// <summary>useProductTimers 가 켜진 경로(Combatgump.cpp 0043f3xx)의 간격(초)</summary>
    private static readonly double[] ProductTimerIntervals = [30, 15, 8];

    /// <summary>요새 모드(inFortMode)에서 기본 경로가 대신 쓰는 간격 (상수 0x38d1b717 ≈ 0.0001초)</summary>
    public const double FortModeInterval = 0.0001;

    /// <summary>useProductTimers 경로의 생산 수량 상한 (DAT_0054db74 초기값)</summary>
    public const int ProductTimerStockLimit = 1;

    /// <summary>
    /// 배치 후 재충전 간격(초)을 구한다.
    /// </summary>
    /// <param name="unitRateIndex">BattleOptions Unit Rate 인덱스 (0 Slow, 1 Medium, 2 Fast)</param>
    /// <param name="inFortMode">요새 편집 모드인지</param>
    /// <param name="useProductTimers">원본 설정 useProductTimers (보유 setup.cfg 는 0)</param>
    public static double RefreshInterval(int unitRateIndex, bool inFortMode, bool useProductTimers = false)
    {
        int index = Math.Clamp(unitRateIndex, 0, 2);
        if (useProductTimers)
        {
            // 이 경로에는 요새 모드 예외가 없다.
            return ProductTimerIntervals[index];
        }
        return inFortMode ? FortModeInterval : DefaultIntervals[index];
    }
}
