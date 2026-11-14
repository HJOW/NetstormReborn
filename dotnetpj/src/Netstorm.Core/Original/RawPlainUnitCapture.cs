using System.Buffers.Binary;

// 보호10.78에서 공통+a0와 빈+a4를 함께 확인한 실제 TYPE/VT 쌍이다.
namespace Netstorm.Core.Original;

/// <summary>원문 plain 유닛만 기본프레임·빈 추가그림 캡처에 분배한다.</summary>
public static class RawPlainUnitCapture
{
    /// <summary>+78=4acfe0/+a0=41c0f0/+a4=41c100인13쌍이다.</summary>
    private static readonly (byte Type, uint Vtable)[] Units =
    [
        (70, 0x508cc8), (73, 0x50a1e8), (111, 0x5040c0), (133, 0x50e568), (135, 0x503e50),
        (137, 0x503d80), (138, 0x50a050), (141, 0x5040c0), (144, 0x50a388), (145, 0x50e738),
        (146, 0x504198), (156, 0x503f20), (158, 0x50f210),
    ];
    /// <summary>TYPE만 같은 다른 가상파생을 빈 추가그림으로 통과시키지 않는다.</summary>
    public static bool Matches(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < 50) return false;
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        // 원본 표 순서로 정확한 두 필드만 비교한다.
        foreach (var unit in Units) if (raw[10] == unit.Type && table == unit.Vtable) return true;
        return false;
    }
}
