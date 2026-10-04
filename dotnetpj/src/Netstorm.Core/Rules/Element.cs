namespace Netstorm.Core.Rules;

/// <summary>
/// 원소(Fury). Rain·Wind·Thunder 세 원소와 공용 원소 Sun 이 있다 (docs/gameplay/elements-energy.md 1절).
/// 에너지 요구 문자열의 글자는 원본 원소 문자 표(exe 0x540ad8 → "wrtsWRTS.?>")를 따른다.
/// </summary>
public enum Element
{
    /// <summary>공용 원소. Sun 에너지는 아무 원소 공급원으로 채울 수 있다 (요구 문자 's')</summary>
    Sun,

    /// <summary>비 (요구 문자 'r')</summary>
    Rain,

    /// <summary>바람 (요구 문자 'w')</summary>
    Wind,

    /// <summary>번개 (요구 문자 't')</summary>
    Thunder,
}

/// <summary>원소 이름·요구 문자 변환.</summary>
public static class Elements
{
    /// <summary>
    /// .type 의 theme 값을 원소로 바꾼다. 대소문자는 무시한다.
    /// 알 수 없는 값(예: 미사용 fireVortex 의 "Fire")이나 빈 값이면 null.
    /// </summary>
    /// <param name="theme">.type 의 theme 속성</param>
    public static Element? FromTheme(string? theme) => theme?.Trim().ToLowerInvariant() switch
    {
        "sun" => Element.Sun,
        "rain" => Element.Rain,
        "wind" => Element.Wind,
        "thunder" => Element.Thunder,
        _ => null,
    };

    /// <summary>원본 요구 문자열의 글자 (w·r·t·s). 대문자도 같은 원소로 본다.</summary>
    /// <param name="letter">요구 문자</param>
    public static Element? FromLetter(char letter) => char.ToLowerInvariant(letter) switch
    {
        's' => Element.Sun,
        'r' => Element.Rain,
        'w' => Element.Wind,
        't' => Element.Thunder,
        _ => null,
    };

    /// <summary>원소의 원본 요구 문자 (소문자).</summary>
    /// <param name="element">원소</param>
    public static char ToLetter(Element element) => element switch
    {
        Element.Sun => 's',
        Element.Rain => 'r',
        Element.Wind => 'w',
        _ => 't',
    };

    /// <summary>.type theme 에 쓰이는 원소 이름 (소문자).</summary>
    /// <param name="element">원소</param>
    public static string ToTheme(Element element) => element.ToString().ToLowerInvariant();
}
