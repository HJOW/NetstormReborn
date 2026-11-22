using System.Buffers.Binary;

// 현재 원문TYPE/SHP와 float 저장 경계를 쓰는 Wind 원주 좌표다.
namespace Netstorm.Core.Original;

/// <summary>RawWindCircle.h의 SHP 추가 헤더 세 실제 필드다.</summary>
public readonly struct WindCircleHeader
{
    /// <summary>원문+14의 float 높이 보정이다.</summary>
    public float WalkStepY { get; }
    /// <summary>원문+1a의 signedWORD 높이다.</summary>
    public short Height { get; }
    /// <summary>원문+1e의 signedWORD 기준점이다.</summary>
    public short HotspotY { get; }
    /// <summary>VFX 크기와 구별한 실제 SHP 필드만 받는다.</summary>
    public WindCircleHeader(float walkStepY, short height, short hotspotY) { WalkStepY = walkStepY; Height = height; HotspotY = hotspotY; }
}

/// <summary>RawWindCircle.cpp의 원주 점과 현재 헤더/부모 중심 보정이다.</summary>
public static class RawWindCircle
{
    /// <summary>원문502328의 실제 double 원주 상수다.</summary>
    private const double Pi = 3.141593;
    /// <summary>원문502320의 실제 binary32 각도 배율이다.</summary>
    private const float DegreeScale = 0.0055555556900799274f;
    /// <summary>원문50a380의 실제 binary32 플라이어 가로 보정이다.</summary>
    private const float FlyerOffset = 1.399999976158142f;
    /// <summary>원문504194의 실제 binary32 높이 배율이다.</summary>
    private const float HeightScale = 0.09090909361839294f;
    /// <summary>큰 효과 뒤 지정한 실제 객체의 현재 TYPE를 읽는다.</summary>
    private static OriginalTypeRecord Type(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ushort sid)
    {
        byte kind = pool.Slot(sid)[10]; if (kind >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Wind 원주 현재 TYPE 범위"); return types[kind];
    }
    /// <summary>정수360/count·index DWORD 감김·원문각도상수와 두float 저장을 보존한다.</summary>
    public static (float X, float Y) Point(float x, float y, int count, int index)
    {
        if (count == 0) throw new ArgumentException("Wind 원주 개수0 나눗셈", nameof(count));
        int next = unchecked(index + 1); double angle = (double)(360 / count) * next * Pi * (double)DegreeScale;
        return ((float)(Math.Cos(angle) * count + x), (float)(Math.Sin(angle) * count + y));
    }
    /// <summary>헤더 뒤 현재부모TYPE/좌표와 현재count/index를 읽고 프레임13 실제높이를 보정한다.</summary>
    public static (float X, float Y) Position(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ushort parent, ushort flyer, WindAviaryProcessFields fields, Func<ushort, uint, int, WindCircleHeader> readHeader)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || readHeader is null) throw new ArgumentException("Wind 원주 PATCH/188TYPE/현재 헤더 필요");
        WindCircleHeader header = readHeader(flyer, (uint)(pool.Slot(flyer)[40] & 1), 13); OriginalTypeRecord type = Type(pool, types, parent); ReadOnlySpan<byte> raw = pool.Slot(parent);
        float x = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[14..]) - type.FootX * 0.5), y = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[18..]) - type.FootY * 0.5);
        float centerX = (float)((double)x + 1.0), centerY = (float)((double)y + (float)((int)header.Height - header.HotspotY + 1));
        (float px, float py) = Point(centerX, centerY, BinaryPrimitives.ReadInt32LittleEndian(fields.Bytes.AsSpan(68)), BinaryPrimitives.ReadInt32LittleEndian(fields.Bytes.AsSpan(72)));
        return ((float)((double)px - FlyerOffset), (float)((double)header.WalkStepY * HeightScale - Type(pool, types, parent).FootY + py));
    }
}
