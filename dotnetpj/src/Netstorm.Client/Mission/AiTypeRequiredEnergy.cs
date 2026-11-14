using Netstorm.Core.Original;

// 실제자산AST와현재188TYPE의에너지조회연결을제공한다.
namespace Netstorm.Client.Mission;

/// <summary>cpppj/src/client/AiTypeRequiredEnergy.h/.cpp의실제자산70..185·내부/빈타입연결이다.</summary>
public static class AiTypeRequiredEnergy
{
    /// <summary>같은현재188TYPE와원본116개AST를빌린다. getter생성뒤TYPE속성변경도즉시읽는다.</summary>
    public static Func<uint, string> BindAiTypeRequiredEnergy(IReadOnlyList<OriginalTypeRecord> currentTypes,
        IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(currentTypes);
        ArgumentNullException.ThrowIfNull(definitions);
        if (currentTypes.Count != 188 || definitions.Count != 116)
            throw new ArgumentException("AI에너지 실제PATCH188TYPE/116자산 계약 오류");
        return number =>
        {
            if (number >= currentTypes.Count) throw new ArgumentOutOfRangeException(nameof(number), "AI에너지 현재TYPE범위 오류");
            RiftTypeDefinition? definition = number >= 70 && number < 70 + definitions.Count ? definitions[(int)number - 70] : null;
            return RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, currentTypes[(int)number], definition);
        };
    }
}
