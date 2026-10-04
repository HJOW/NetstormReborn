namespace Netstorm.Core.Rules;

/// <summary>
/// 플레이어 색 번호 규칙. 원본은 소유자 번호 → 색 번호 표(<c>DAT_00531b08</c>)를 "색 번호 = 소유자 번호"로 초기화하고
/// (<c>FUN_0043b540</c>), AI 플레이어를 만들 때 미션 머리 값 <c>aiNColor</c> 가 색 이름 목록에 있으면 그 번호로 덮어쓴다
/// (<c>FUN_00417e50</c> → <c>FUN_0043ca70</c>). 색 번호는 섬 테두리 색 변환표의 행, 선택 괄호 색, 미니맵 색을 고른다.
/// 2026-10-03 TEST01 시험 전투 녹화: 사람(1) 파란 테두리, 소유자 2 빨간 테두리, 소유자 3 흰 테두리 — 기본 표와 같다.
/// </summary>
public static class PlayerColors
{
    /// <summary>색이 없는 중립 번호 (<c>none</c>).</summary>
    public const int None = 0;

    /// <summary>색 번호의 최댓값 (<c>orange</c>).</summary>
    public const int Maximum = 8;

    /// <summary>
    /// 원본 exe 의 색 이름 목록 <c>/none/blue/red/white/green/purple/yellow/lightblue/orange/</c> (VA 0x542660 이 가리키는 문자열).
    /// 목록 안 순번이 곧 색 번호다.
    /// </summary>
    private static readonly string[] Names = ["none", "blue", "red", "white", "green", "purple", "yellow", "lightblue", "orange"];

    /// <summary>색 이름을 색 번호로 바꾼다. 원본처럼 대소문자를 가리지 않으며 목록에 없으면(예: cyan, magenta) 0이다.</summary>
    /// <param name="name">미션 머리 값의 색 이름 (따옴표를 벗긴 값)</param>
    public static int Parse(string? name)
    {
        if (string.IsNullOrWhiteSpace(name)) return None;
        int index = Array.FindIndex(Names, candidate => candidate.Equals(name.Trim(), StringComparison.OrdinalIgnoreCase));
        return Math.Max(index, None);
    }

    /// <summary>덮어쓰지 않은 기본 색 번호: 소유자 1~8은 자기 번호, 그 밖(중립 0 등)은 0.</summary>
    /// <param name="owner">소유 플레이어 번호</param>
    public static int Default(int owner) => owner is >= 1 and <= Maximum ? owner : None;

    /// <summary>
    /// 소유자 번호 → 색 번호 표를 만든다. 기본 표에 <paramref name="overrides"/> (AI 번호 → 색 번호)를 덮어쓴다.
    /// 값이 1~8 범위 밖인 덮어쓰기는 원본처럼 무시한다.
    /// </summary>
    /// <param name="overrides">미션 머리 값 aiNColor 에서 읽은 덮어쓰기 (없으면 기본 표)</param>
    public static IReadOnlyDictionary<int, int> Table(IReadOnlyDictionary<int, int>? overrides = null)
    {
        var table = new Dictionary<int, int>();
        // 소유자 1~8의 기본 색을 채우고 유효한 덮어쓰기만 반영한다
        for (int owner = 1; owner <= Maximum; owner++)
        {
            table[owner] = overrides != null && overrides.TryGetValue(owner, out int color) && color is >= 1 and <= Maximum
                ? color : Default(owner);
        }
        return table;
    }
}
