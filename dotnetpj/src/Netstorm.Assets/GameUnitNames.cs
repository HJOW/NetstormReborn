using System.Collections.ObjectModel;
using System.Text.Json;

namespace Netstorm.Assets;

/// <summary>
/// AGENTS.md의 건물·유닛 한국어 명칭을 공유 용어표에서 조회한다.
/// 사제·신전·다리·발사기·간헐천·제단·거주 구역은 번역해도 되고 다른 이름은 영어 발음의 한글 표기를 사용한다.
/// 발음이 모호한 Whirlibase 는 훨리베이스, Whirligig 는 훨리기그로 정했다 (2026-10-16 AGENTS.md).
/// 타입 식별자와 원본 description은 변경하지 않으며 표시할 때만 선택 언어를 적용한다.
/// </summary>
public static class GameUnitNames
{
    /// <summary>두 프로젝트가 참조할 공유 명칭 JSON의 고정 내장 리소스 이름.</summary>
    private const string ResourceName = "Netstorm.Assets.UnitNames.Korean.json";

    /// <summary>게임 시작 시 한 번 읽는 영어 이름→한국어 이름 표. 호출자가 수정할 수 없다.</summary>
    private static readonly IReadOnlyDictionary<string, string> KoreanNames = LoadKoreanNames();

    /// <summary>타입의 원본 description 또는 누락 시 타입 이름을 현재 언어의 표시 이름으로 바꾼다.</summary>
    /// <param name="type">식별자·규칙을 유지할 실제 타입.</param>
    /// <param name="language">GameLanguage에서 정규화한 표시 언어.</param>
    public static string ForType(TypeInfo type, string language)
    {
        ArgumentNullException.ThrowIfNull(type);
        return Format(type.Definition.GetString("description") ?? type.Name, language);
    }

    /// <summary>확정된 영어 명칭만 한국어로 표시한다. 다른 언어와 미등록 이름은 입력을 그대로 반환한다.</summary>
    /// <param name="name">설정 치환을 마친 원본 이름 또는 description.</param>
    /// <param name="language">현재 UI 언어. 한국어 여부만 비교하고 언어 설정은 바꾸지 않는다.</param>
    public static string Format(string name, string language)
    {
        ArgumentNullException.ThrowIfNull(name);
        return language.Equals(GameLanguage.Korean, StringComparison.OrdinalIgnoreCase)
            && KoreanNames.TryGetValue(name, out string? korean) ? korean : name;
    }

    /// <summary>같은 공유 JSON을 UTF-8 내장 자료로 읽어 대소문자 무시·읽기 전용 조회 표로 만든다.</summary>
    private static IReadOnlyDictionary<string, string> LoadKoreanNames()
    {
        using Stream stream = typeof(GameUnitNames).Assembly.GetManifestResourceStream(ResourceName)
            ?? throw new InvalidDataException("한국어 건물·유닛 명칭 리소스가 없습니다.");
        Dictionary<string, string> names = JsonSerializer.Deserialize<Dictionary<string, string>>(stream)
            ?? throw new InvalidDataException("한국어 건물·유닛 명칭 자료를 읽지 못했습니다.");
        return new ReadOnlyDictionary<string, string>(new Dictionary<string, string>(names, StringComparer.OrdinalIgnoreCase));
    }
}
