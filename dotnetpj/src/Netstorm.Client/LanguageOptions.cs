using System.Text;
using Netstorm.Core.Original;

// 원문 언어 선택의 현재/공개 번호와 두 적재 효과를 구별한다.
namespace Netstorm.Client;

/// <summary>언어 선택이 실제로 도달한 설정 읽기/용어표 적재 장치다.</summary>
public sealed class LanguageOptionHooks
{
    /// <summary>현재 currentLanguage 원문 문자열이다. 누락 값은 빈 문자열로 공급한다.</summary>
    public Func<string>? Read { get; init; }
    /// <summary>현재 같은 LanguageSpec/Registry에 용어표를 적재한다.</summary>
    public Action<string>? Load { get; init; }
}

/// <summary>004de390/004de420와435248..43529f의 선택·부분 효과를 제공한다.</summary>
public static class LanguageOptions
{
    /// <summary>원문 지역 문자열의128BYTE 용량이다.</summary>
    private const int NameCapacity = 128;

    /// <summary>OS LANGID의 낮은 WORD를 원문 primary ID/나머지 sublanguage2 분기로 해석한다.</summary>
    public static uint Detect(ushort langid) => (langid & 0x3ff) switch
    {
        7 => 3, 10 => 4, 12 => 2, 17 => 5, 22 => 6,
        _ => (langid >> 10) == 2 ? 1u : 0u,
    };
    /// <summary>현재 언어를 먼저 쓰고 정규 이름→현재 raw 이름 적재가 모두 돌아온 뒤 공개 번호를 쓴다.</summary>
    public static bool Apply(uint detected, IntBox current, IntBox published, LanguageOptionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(current); ArgumentNullException.ThrowIfNull(published); ArgumentNullException.ThrowIfNull(hooks);
        string name = (hooks.Read ?? throw new InvalidOperationException("언어 옵션의 현재 설정 읽기 미연결"))();
        int end = name.IndexOf('\0'); if (end >= 0) name = name[..end];
        if (Encoding.UTF8.GetByteCount(name) >= NameCapacity) throw new ArgumentOutOfRangeException(nameof(hooks), "언어 옵션 원문128BYTE 지역 범위");
        uint selected = RootLanguageNumbers.Find(name);
        if (detected != selected && selected == 0) selected = detected == 1 ? 1u : 0u;
        if (current.Value == unchecked((int)selected)) return false;
        current.Value = unchecked((int)selected);
        Action<string> load = hooks.Load ?? throw new InvalidOperationException("언어 옵션의 실제 용어표 적재 미연결");
        load(RootLanguageNumbers.Name(selected)); load(name); published.Value = unchecked((int)selected);
        return true;
    }
}
