using System.Globalization;
using System.Text;

// 객체·Player 메뉴가 번역 뒤에 사용하는 원본 고정 인자 서식이다.
namespace Netstorm.Client.Gump;

/// <summary>원본 메뉴의 %s·%d·%%와 256바이트 출력 계약을 처리한다.</summary>
public static class PlayerMenuText
{
    /// <summary>서식과 실제 문자열·정수 인자를 순서대로 소비한다.</summary>
    /// <param name="pattern">호출자가 이미 선택한 원문 서식이다.</param>
    /// <param name="values">원본 순서의 문자열 또는 signed 정수 인자다.</param>
    /// <returns>번역을 추가하지 않은 서식 결과다.</returns>
    public static string Format(string pattern, IReadOnlyList<PriestMenuArgument> values)
    {
        ArgumentNullException.ThrowIfNull(pattern);
        ArgumentNullException.ThrowIfNull(values);
        var output = new StringBuilder(); int argument = 0;
        // 서식 문자만 해석하고 인자에 포함된 퍼센트는 다시 해석하지 않는다.
        for (int index = 0; index < pattern.Length; index++)
        {
            if (pattern[index] != '%') { output.Append(pattern[index]); continue; }
            if (++index >= pattern.Length) throw new InvalidOperationException("객체 메뉴 서식 끝 오류");
            if (pattern[index] == '%') { output.Append('%'); continue; }
            if (argument >= values.Count) throw new InvalidOperationException("객체 메뉴 서식 인자 부족");
            PriestMenuArgument value = values[argument++];
            if (pattern[index] == 's' && value.IsText) output.Append(value.Text);
            else if (pattern[index] == 'd' && !value.IsText) output.Append(value.Number.ToString(CultureInfo.InvariantCulture));
            else throw new InvalidOperationException("객체 메뉴 미지원 서식 또는 인자 오류");
        }
        if (argument != values.Count || output.Length >= 256) throw new ArgumentOutOfRangeException(nameof(values), "객체 메뉴 원본 256바이트 서식 계약 오류");
        return output.ToString();
    }
}
