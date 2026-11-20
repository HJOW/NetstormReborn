// State의 실제 소유자 색과 다리 붕괴 스캔을 작은 원문 몸체로 초기화한다.
namespace Netstorm.Core.Original;

/// <summary>전체 State/프레임을 실행하지 않고 선택된 실제 물리만 쓴다.</summary>
public static class RawStateBridgeReset
{
    /// <summary>0043b540의 앞9DWORD를0..8로 쓰고 이웃과 모드/TYPE는 유지한다.</summary>
    public static void ResetOwnerColors(Span<int> colors)
    {
        if (colors.Length < 9) throw new ArgumentException("State 소유자 색 표의 실제9DWORD 부족", nameof(colors));
        // 원문이 지정한 아홉 owner만 쓰며 열째 칸은 건드리지 않는다.
        for (int owner = 0; owner < 9; owner++) colors[owner] = owner;
    }
    /// <summary>00422490의 float 지연+double 게임시각을 먼저 계산한 뒤 세DWORD와 기한을 쓴다.</summary>
    public static void ResetDecayScan(BridgeDecayScanState scan, float period, double now)
    {
        ArgumentNullException.ThrowIfNull(scan); double next = (double)period + now;
        scan.First = 15000; scan.Last = 23001; scan.Cursor = 15000; scan.Next = next;
    }
}
