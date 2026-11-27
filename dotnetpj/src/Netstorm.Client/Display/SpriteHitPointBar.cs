using Netstorm.Core.Original;

// 선택 체력 막대의 판본별 현재 값 조회 순서를 복원한다.
namespace Netstorm.Client.Display;

/// <summary>체력 막대의 최종 현재값·최대값·색이다.</summary>
/// <param name="Current">표시할 현재 체력이다.</param>
/// <param name="Maximum">표시할 최대 체력이다.</param>
/// <param name="Color">현재 비교에서 선택한 팔레트 색이다.</param>
public readonly record struct SpriteHitPointResult(int Current, int Maximum, uint Color);

/// <summary>현재 체력과 최대 체력을 원본 호출 순서대로 조회한다.</summary>
public static class SpriteHitPointBar
{
    /// <summary>패치와 CD의 서로 다른 조회 횟수·비교·절삭을 적용한다.</summary>
    /// <param name="edition">현재 게임 판본이다.</param>
    /// <param name="colors">정상·절반 이하·4분의 1 이하 색의 현재 배열이다.</param>
    /// <param name="current">현재 객체 체력 조회다.</param>
    /// <param name="maximum">현재 선택 TYPE 최대 체력 조회다.</param>
    /// <returns>원본 순서로 조회한 체력과 선택된 색이다.</returns>
    /// <remarks>cpppj SpriteHitPointBar.cpp의 진단 중 상태 변경과 색 배열 재조회를 따른다.</remarks>
    public static SpriteHitPointResult Read(OriginalEdition edition, uint[] colors, Func<int> current, Func<int> maximum)
    {
        ArgumentNullException.ThrowIfNull(colors);
        ArgumentNullException.ThrowIfNull(current);
        ArgumentNullException.ThrowIfNull(maximum);
        if (colors.Length < 3) throw new ArgumentException("체력 막대 색 배열 부족", nameof(colors));
        if (edition == OriginalEdition.Patch1078)
        {
            uint color = colors[0];
            if (current() <= maximum() / 2) color = colors[1];
            if (current() <= maximum() / 4) color = colors[2];
            int value = current();
            int max = maximum();
            return new(value, max, color);
        }
        int limit = 1;
        if (maximum() >= 1) limit = maximum();
        int actual = current();
        int lesser = actual > limit ? limit : actual;
        if (lesser < 0) actual = 0;
        else if (actual > limit) actual = limit;
        uint selected = colors[0];
        if (actual <= limit / 2) selected = colors[1];
        if (actual <= limit / 4) selected = colors[2];
        return new(actual, limit, selected);
    }
}
