using System.Globalization;

namespace Netstorm.Assets;

/// <summary>
/// 게임 언어 이름. 언어 이름이 곧 파일 확장자다 (tutorial1.english, xlat.german, config.korean …).
/// 원본 언어 표(VA 0x5435b8): english, english(영국), french, german, spanish, japanese, portuguese.
/// 클론은 여기에 korean 을 더한다 (1차 목표 언어: 영어·한국어).
/// </summary>
public static class GameLanguage
{
    /// <summary>기본 언어이자 번역 원문 언어. 파일이 없을 때의 대체 언어이기도 하다</summary>
    public const string English = "english";

    /// <summary>한국어 (클론에서 추가)</summary>
    public const string Korean = "korean";

    /// <summary>원본 게임이 아는 언어 (중복 제거, 원본 표 순서)</summary>
    public static IReadOnlyList<string> OriginalLanguages { get; } =
        [English, "french", "german", "spanish", "japanese", "portuguese"];

    /// <summary>클론이 지원하려는 언어 전체 (원본 언어 + 한국어)</summary>
    public static IReadOnlyList<string> AllLanguages { get; } = [.. OriginalLanguages, Korean];

    /// <summary>
    /// 운영체제 언어에서 게임 언어를 고른다. 원본(FUN_004de390)의 GetUserDefaultLangID 대응에
    /// 한국어를 더했다. 모르는 언어는 영어.
    /// </summary>
    /// <param name="culture">운영체제 UI 문화권</param>
    public static string FromCulture(CultureInfo culture) => culture.TwoLetterISOLanguageName switch
    {
        "ko" => Korean,
        "de" => "german",
        "fr" => "french",
        "es" => "spanish",
        "ja" => "japanese",
        "pt" => "portuguese",
        _ => English,
    };

    /// <summary>
    /// 언어 이름을 정규화한다 (소문자). 알 수 없는 이름이면 영어로 되돌린다.
    /// 원본(FUN_004de420)도 모르는 이름이면 0번(english)을 쓴다.
    /// </summary>
    /// <param name="name">설정의 currentLanguage 값 등</param>
    public static string Normalize(string? name)
    {
        string lower = (name ?? "").Trim().ToLowerInvariant();
        return AllLanguages.Contains(lower) ? lower : English;
    }

    /// <summary>
    /// 언어별 파일을 고른다: "&lt;이름&gt;.&lt;언어&gt;" 가 있으면 그것, 없으면 "&lt;이름&gt;.english".
    /// 원본은 대체하지 않지만, 클론은 번역되지 않은 미션도 열 수 있도록 영어로 대체한다 (LEFT_JOBS 12단계).
    /// </summary>
    /// <param name="files">가상 파일 시스템</param>
    /// <param name="pathWithoutExtension">확장자를 뺀 경로 (예: "d/tutorial1")</param>
    /// <param name="language">원하는 언어</param>
    /// <returns>찾은 경로, 영어 파일도 없으면 null</returns>
    public static string? ResolveFile(GameFileSystem files, string pathWithoutExtension, string language)
    {
        string wanted = $"{pathWithoutExtension}.{Normalize(language)}";
        if (files.Exists(wanted))
        {
            return wanted;
        }
        string fallback = $"{pathWithoutExtension}.{English}";
        return files.Exists(fallback) ? fallback : null;
    }
}
