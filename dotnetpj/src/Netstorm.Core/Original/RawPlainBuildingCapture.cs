using System.Buffers.Binary;

// 같은 기본 프레임과 빈 추가 그림을 공유하는 정확한 TYPE/VT만 허용한다.
namespace Netstorm.Core.Original;

/// <summary>RawCampaignSession.cpp의 IsPlainBuildingCapture와 다섯 워크샾 분배다.</summary>
public static class RawPlainBuildingCapture
{
    /// <summary>보호10.78에서 +78=4acfe0/+a4=41c100인 실제 건물17쌍이다.</summary>
    private static readonly (byte Type, uint Vtable)[] Buildings =
    [
        (129, 0x50b9b8), (136, 0x509470), (140, 0x50b6e8), (143, 0x5099e8), (148, 0x50b7c0),
        (75, 0x501ea0), (76, 0x501ea0), (77, 0x501ea0), (78, 0x50e940), (147, 0x50e940),
        (79, 0x50ea08), (139, 0x509aa0), (142, 0x50ead0), (149, 0x50e668),
        (164, 0x5076b8), (95, 0x506e50), (157, 0x506f08),
    ];

    /// <summary>TYPE만 같거나 VT만 공유하는 다른 파생을 일반 그림으로 통과시키지 않는다.</summary>
    public static bool Matches(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < 50) return false;
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        // 원본 순서의 실제 두 필드를 함께 검사한다.
        foreach (var building in Buildings) if (raw[10] == building.Type && table == building.Vtable) return true;
        return false;
    }
}
