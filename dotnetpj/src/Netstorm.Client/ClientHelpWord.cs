using System.Text;
using Netstorm.Core.Original;

// 현재 Client의 원본 도움말 항목 표시 바이트를 장기 보관한다.
namespace Netstorm.Client;

/// <summary>옵션 해석과 메뉴 소비자가 공유하는 NULL 가능 도움말 문구다.</summary>
public sealed class ClientHelpWord
{
    /// <summary>현재 표시 바이트 문자열이며 null과 빈 문자열을 구별한다.</summary>
    public string? Value { get; private set; }
    /// <summary>실제 문구 교체 횟수다.</summary>
    public uint Generation { get; private set; }

    /// <summary>ASCII 영문자만 접고 나머지 표시 바이트는 그대로 비교한다.</summary>
    public static bool EqualsIgnoreCase(string left, string right)
    {
        ArgumentNullException.ThrowIfNull(left); ArgumentNullException.ThrowIfNull(right);
        if (left.Length != right.Length) return false;
        // 같은 위치의 원본 표시 바이트를 순서대로 비교한다.
        for (int index = 0; index < left.Length; index++)
        {
            char a = left[index], b = right[index];
            if (a is >= 'A' and <= 'Z') a = (char)(a + 32);
            if (b is >= 'A' and <= 'Z') b = (char)(b + 32);
            if (a != b) return false;
        }
        return true;
    }

    /// <summary>표시 바이트 설정이 유효하면 기존 값과 비교해 필요한 경우만 교체한다.</summary>
    /// <param name="configured">한 번 부호화된 현재 표시 바이트이며 null은 설정 부재다.</param>
    /// <returns>유효한 비빈 설정을 읽었으면 참이다.</returns>
    public bool Interpret(string? configured)
    {
        if (string.IsNullOrEmpty(configured)) return false;
        if (Value is not null && EqualsIgnoreCase(Value, configured)) return true;
        Value = configured; Generation = unchecked(Generation + 1); return true;
    }

    /// <summary>현재 helpWord 설정을 표시 바이트로 한 번 부호화해 해석한다.</summary>
    /// <param name="configuration">현재 실제 Client 설정이다.</param>
    /// <returns>유효한 비빈 설정을 읽었으면 참이다.</returns>
    public bool ReadOptions(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration);
        string? value = configuration.Configuration.Get("helpWord");
        return Interpret(value is null ? null : Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(value)));
    }
}
