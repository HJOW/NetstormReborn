// 연소점 목표 개수의 원문DWORD감김과 signed IDIV를 픽셀/입자 접근과 구별한다.
namespace Netstorm.Core.Original;

/// <summary>004241a2/CD004a5d81의 현재HP/최대HP/이미지 크기 정수 부모다.</summary>
public static class RawBurningSourceCount
{
    /// <summary>getter 전 저장HP로 계산하고 모든 원문 감김/나눗셈 뒤에만 최소1로 제한한다.</summary>
    public static int Count(OriginalEdition edition, int hitPoints, int maximum, int width, int height)
    {
        int twice = unchecked((int)((uint)maximum * 2)); int half = twice / 4;
        bool patch = edition == OriginalEdition.Patch1078;
        int divisor = unchecked(patch ? maximum - half : half - maximum), numerator = unchecked(hitPoints * 5);
        if (divisor == 0) throw new DivideByZeroException("연소점 원문 IDIV의0 나눗셈");
        if (numerator == int.MinValue && divisor == -1) throw new OverflowException("연소점 원문 IDIV의signed 몫 범위 초과");
        int quotient = numerator / divisor, wanted = unchecked(patch ? 6 - quotient : quotient + 6), rows = unchecked(width + height) / 5;
        return Math.Max(1, Math.Min(wanted, rows));
    }
}
