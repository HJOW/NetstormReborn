using System.Globalization;

namespace Netstorm.Assets;

/// <summary>
/// 글 사이 그림 표시 <c>~[I타입.프레임]</c> 의 "타입.프레임" 을 타입과 클러스터 번호로 푼다.
/// 원본 미션 브리핑·도움말·채팅이 쓰는 표기다 (예: <c>~[Iicon.12]</c>, <c>~[Ipriest.E3]</c>, <c>~[I93.113]</c>).
/// </summary>
public static class InlinePicture
{
    /// <summary>
    /// 그림 표시를 타입과 본체 클러스터 번호로 바꾼다. 타입을 모르면 null 이다.
    /// 타입은 이름 또는 실행 중 타입 번호(숫자)이고, 프레임은 클러스터 번호(숫자) 또는 클러스터 이름(글자 + 번호)이다.
    /// 프레임을 찾지 못하면 기본 프레임을 쓴다 — TEST01 의 <c>~[IsunBalloon.a1]</c> 은 A01 이 없는데도 원본이
    /// 풍선 그림(A00, 기본 프레임)을 보여 주었다. '*' 도 기본 프레임으로 본다(추정).
    /// </summary>
    /// <param name="catalog">타입 목록</param>
    /// <param name="spec">"타입.프레임" (예: "sunBalloon.a1")</param>
    public static (TypeInfo Type, int Frame)? Resolve(TypeCatalog catalog, string spec)
    {
        int dot = spec.IndexOf('.');
        string typeName = dot < 0 ? spec : spec[..dot];
        string frameName = dot < 0 ? "" : spec[(dot + 1)..];
        TypeInfo? type = int.TryParse(typeName, NumberStyles.None, CultureInfo.InvariantCulture, out int number)
            ? catalog.FindByRuntimeIndex(number) : catalog.Find(typeName);
        if (type == null) return null;
        return (type, Frame(type.Definition, frameName));
    }

    /// <summary>프레임 표기를 클러스터 번호로 바꾼다 (찾지 못하면 기본 프레임).</summary>
    /// <param name="definition">타입 정의</param>
    /// <param name="frameName">숫자, 글자 + 숫자, 또는 '*'</param>
    public static int Frame(TypeDefinition definition, string frameName)
    {
        TypeFrameTable frames = definition.Frames;
        if (int.TryParse(frameName, NumberStyles.None, CultureInfo.InvariantCulture, out int index))
            return index < frames.Codes.Count ? index : frames.DefaultFrame;
        // 글자 1~2개 뒤에 번호가 오는 클러스터 이름 표기 (D1 → 방향 D, 번호 1)
        int letters = 0;
        while (letters < frameName.Length && char.IsAsciiLetter(frameName[letters])) letters++;
        if (letters is 1 or 2 && int.TryParse(frameName[letters..], NumberStyles.None, CultureInfo.InvariantCulture, out int code))
        {
            char side = char.ToUpperInvariant(frameName[0]);
            char variant = letters == 2 ? char.ToUpperInvariant(frameName[1]) : TypeFrameTable.DefaultVariant;
            int found = frames.Find(side, variant, code);
            if (found >= 0) return found;
        }
        return frames.DefaultFrame;
    }
}
