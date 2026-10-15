namespace Netstorm.Assets;

/// <summary>
/// 원본이 설정 값에서 숫자와 목록 항목을 읽는 규칙: CRT atol 과 ';' 구분 목록 함수.
/// 문서화되지 않은 편의 해석(쉼표 구분·따옴표 제거 등)을 더하지 않는다.
/// </summary>
/// <remarks>
/// 근거: cpppj ConfigParseLong/ConfigListItem(ConfigInterfaceTests.cpp 의 경계 검사와 같은 벡터).
/// 2026-10-15 감사 D03 에서 미션 AllyList 해석을 원본과 맞추려고 추가했다.
/// </remarks>
public static class ConfigNumbers
{
    /// <summary>
    /// CRT atol 처럼 앞의 공백류를 건너뛰고 부호와 십진수를 읽어 숫자가 아닌 글자에서 멈춘다.
    /// 읽을 숫자가 없으면 0이고, 넘침은 검사하지 않고 32비트로 감긴다.
    /// </summary>
    /// <param name="text">설정 값 조각.</param>
    /// <returns>부호 있는 32비트 값.</returns>
    public static int ParseLong(string? text)
    {
        if (string.IsNullOrEmpty(text)) return 0;
        int position = 0;
        // 앞의 공백류(공백, \t, \n, \v, \f, \r)를 건너뛴다.
        while (position < text.Length && (text[position] == ' ' || text[position] is >= '\t' and <= '\r')) position++;
        bool negative = false;
        if (position < text.Length && text[position] is '-' or '+') negative = text[position++] == '-';
        uint total = 0;
        // 숫자가 아닌 글자에서 멈추며 32비트를 넘으면 감긴다.
        for (; position < text.Length && text[position] is >= '0' and <= '9'; position++)
            total = unchecked(total * 10u + (uint)(text[position] - '0'));
        return unchecked((int)(negative ? 0u - total : total));
    }

    /// <summary>
    /// ';' 로 나눈 목록의 index번째 조각을 그대로 돌려준다(공백·따옴표를 다듬지 않는다). 조각이 없거나 index가 음수이면 빈 문자열이다.
    /// 쉼표는 구분자가 아니므로 "3,4" 는 한 조각이다.
    /// </summary>
    /// <param name="list">';' 로 이어진 목록 문자열.</param>
    /// <param name="index">0부터 시작하는 조각 번호.</param>
    public static string ListItem(string? list, int index)
    {
        if (list == null || index < 0) return "";
        int start = 0;
        // 앞 조각을 index개 건너뛴다.
        for (; index > 0; index--)
        {
            int separator = list.IndexOf(';', start);
            if (separator < 0) return "";
            start = separator + 1;
        }
        int end = list.IndexOf(';', start);
        return end < 0 ? list[start..] : list[start..end];
    }
}
