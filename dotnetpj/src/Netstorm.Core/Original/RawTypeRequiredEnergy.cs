using System.Text;
using Netstorm.Assets;

// 실제 TYPE와속성AST에서원본mana후처리문자열을계산한다.
namespace Netstorm.Core.Original;

/// <summary>PATCH0049c3b0의mana문자열과0049b0d0의기본보충·소문자화다.
/// 근거: cpppj/src/o/RawTypeRequiredEnergy.h/.cpp. requiredEnergy라는합성속성을조회하지않는다.</summary>
public static class RawTypeRequiredEnergy
{
    /// <summary>00540ad8→0050c340의원소문자표다. 끝NUL도인덱스11의실제입력이다.</summary>
    private const string EnergyLetters = "wrtsWRTS.?>\0";

    /// <summary>표안의현재원소문자를읽으며원문메모리밖접근만명시거부한다.</summary>
    private static char Letter(int theme) => theme >= 0 && theme < EnergyLetters.Length ? EnergyLetters[theme]
        : throw new ArgumentOutOfRangeException(nameof(theme), "TYPE요구에너지 실제원소문자표 범위 오류");

    /// <summary>현재C문자열의첫NUL까지보존한다.</summary>
    private static string CutNul(string value)
    {
        int end = value.IndexOf('\0');
        return end < 0 ? value : value[..end];
    }

    /// <summary>현재TYPE와실제AST의mana를읽고그룹·레벨·원소에따른원문기본값을보충한다.
    /// definition이null이면내부/빈TYPE의실제zero초기mana입력이다.</summary>
    public static string TypeRequiredEnergy(OriginalEdition edition, OriginalTypeRecord type, RiftTypeDefinition? definition)
    {
        ArgumentNullException.ThrowIfNull(type);
        if (edition != OriginalEdition.Patch1078) throw new ArgumentException("TYPE요구에너지는PATCH실제후처리전용입니다", nameof(edition));
        string value = "";
        if (definition is not null)
        {
            // 모든지정값을원문순서로검사한다. 잘못된앞선AST를뒤의정상값으로숨기지않는다.
            foreach (TypeProperty property in definition.Properties)
            {
                if (!string.Equals(property.Name, "mana", StringComparison.OrdinalIgnoreCase)) continue;
                if (property.Text is null) throw new ArgumentException("원본TYPEmana는문자열AST여야합니다", nameof(definition));
                value = CutNul(Encoding.Latin1.GetString(OriginalText.EncodeWindows1252(property.Text)));
                if (value.Length > 9) throw new ArgumentOutOfRangeException(nameof(definition), "원본TYPEmana는9바이트이하입니다");
            }
        }
        if (type.Group != 10 && (type.Flags2 & 0x200000u) == 0 && value.Length == 0)
        {
            if (type.Level >= 0) value += Letter(type.Theme);
            if (type.Level == 1) value += EnergyLetters[3];
            if (type.Level == 2) { value += Letter(type.Theme); value += EnergyLetters[3]; }
        }
        value = CutNul(value);
        char[] bytes = value.ToCharArray();
        // 현재원본mana는ASCII이며비ASCII CRT locale동일성을가정하지않는다.
        for (int index = 0; index < bytes.Length; index++)
        {
            if (bytes[index] >= 128) throw new ArgumentException("TYPE비ASCII mana의원본CRTtolower경계미복원", nameof(definition));
            if (bytes[index] is >= 'A' and <= 'Z') bytes[index] = (char)(bytes[index] + 32);
        }
        return new string(bytes);
    }
}
