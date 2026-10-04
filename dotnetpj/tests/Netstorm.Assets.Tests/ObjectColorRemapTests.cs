namespace Netstorm.Assets.Tests;

/// <summary>원본 exe의 색 후보·타입 초기값·명도 대응과 오브젝트 소유자색의 경계를 검사한다.</summary>
public sealed class ObjectColorRemapTests
{
    /// <summary>배포 데이터의 기본 팔레트로 실제 변환표를 만든다.</summary>
    private static ObjectColorRemap OriginalRemap() =>
        new(Palette.Load(OriginalData.RequireFile("d/" + Palette.GameCol)));

    /// <summary>사제의 명암띠는 원본 0x531a08 후보를 거꾸로 쓰고 밝은 열 칸은 후보 0을 쓴다.</summary>
    [Theory]
    [InlineData(1, new byte[] { 10, 4, 147, 146, 145, 76, 80, 81 })]
    [InlineData(2, new byte[] { 227, 217, 249, 204, 169, 170, 100, 94 })]
    [InlineData(3, new byte[] { 15, 70, 149, 31, 35, 185, 93, 195 })]
    [InlineData(4, new byte[] { 143, 142, 2, 136, 128, 133, 131, 207 })]
    [InlineData(5, new byte[] { 10, 14, 70, 148, 150, 253, 190, 189 })]
    [InlineData(6, new byte[] { 213, 67, 59, 52, 3, 219, 218, 113 })]
    [InlineData(7, new byte[] { 143, 144, 72, 6, 156, 155, 85, 9 })]
    [InlineData(8, new byte[] { 213, 227, 63, 224, 223, 222, 221, 220 })]
    public void PriestBand_MatchesOriginalCandidateOrder(int color, byte[] expected)
    {
        byte[] row = OriginalRemap().Table(TypeLoadOrder.IndexOf("priest"), color).ToArray();
        Assert.Equal(expected, row[0xee..0xf6]);
        Assert.All(row[0xe4..0xee], value => Assert.Equal(expected[^1], value));
        Assert.Equal(0xe3, row[0xe3]);
        Assert.Equal(0xf6, row[0xf6]);
    }

    /// <summary>원본 기본 색 규칙 열두 타입이 같은 띠를 사용하고 미지정 타입에는 적용하지 않는다.</summary>
    [Fact]
    public void DefaultTypes_UseBandWithoutRecoloringOtherTypes()
    {
        ObjectColorRemap remap = OriginalRemap();
        // 0043be30에서 호출하는 기본 타입마다 파랑 띠를 검사한다.
        foreach (string name in new[] { "bridge", "bridgeConnector", "priest", "altar", "bulf", "sunWalker",
                     "rainWalker", "sunFlyer", "fenceShield", "rainBalloon", "rainFlyer", "outpost" })
        {
            Assert.True(remap.AppliesTo(TypeLoadOrder.IndexOf(name)));
            Assert.Equal(81, remap.Table(TypeLoadOrder.IndexOf(name), 1).Span[0xf5]);
        }
        Assert.False(remap.AppliesTo(TypeLoadOrder.IndexOf("rainCannon")));
        Assert.Equal(Enumerable.Range(0, 256).Select(i => (byte)i),
            remap.Table(TypeLoadOrder.IndexOf("rainCannon"), 1).ToArray());
    }

    /// <summary>다리만 빨강·연파랑의 어두운 띠를 한 색으로 고정하며 bridgeConnector는 일반 규칙을 쓴다.</summary>
    [Theory]
    [InlineData(2, 204)]
    [InlineData(7, 156)]
    public void Bridge_UsesOriginalSpecialColors(int color, byte expected)
    {
        ObjectColorRemap remap = OriginalRemap();
        Assert.All(remap.Table(TypeLoadOrder.IndexOf("bridge"), color).ToArray()[0xee..0xf6],
            value => Assert.Equal(expected, value));
        Assert.NotEqual(expected, remap.Table(TypeLoadOrder.IndexOf("bridgeConnector"), color).Span[0xee]);
    }

    /// <summary>풍선·Sail Skater의 지정 픽셀은 원본 후보와 기본 팔레트의 명도에 맞고 다른 픽셀은 보존한다.</summary>
    [Theory]
    [InlineData(1, 81, 80, 81)]
    [InlineData(2, 100, 204, 169)]
    [InlineData(3, 185, 31, 35)]
    [InlineData(4, 207, 207, 207)]
    [InlineData(5, 189, 253, 190)]
    [InlineData(6, 113, 3, 218)]
    [InlineData(7, 85, 155, 85)]
    [InlineData(8, 220, 223, 220)]
    public void ExplicitSources_MatchOriginalLightness(int color, byte sun, byte wind, byte sail)
    {
        ObjectColorRemap remap = OriginalRemap();
        Assert.Equal(sun, remap.Table(TypeLoadOrder.IndexOf("sunBalloon"), color).Span[86]);
        Assert.Equal(wind, remap.Table(TypeLoadOrder.IndexOf("windBalloon"), color).Span[86]);
        Assert.Equal(sail, remap.Table(TypeLoadOrder.IndexOf("windWalker"), color).Span[218]);
        Assert.Equal(228, remap.Table(TypeLoadOrder.IndexOf("sunBalloon"), color).Span[228]);
        Assert.Equal(19, remap.Table(TypeLoadOrder.IndexOf("windBalloon"), color).Span[19]);
    }

    /// <summary>중립·유효 범위 밖 색은 변환하지 않으며 지면·받침은 기존 테두리 규칙을 유지한다.</summary>
    [Fact]
    public void NeutralAndTerrain_KeepTheirExistingRules()
    {
        Palette palette = Palette.Load(OriginalData.RequireFile("d/" + Palette.GameCol));
        var remap = new ObjectColorRemap(palette);
        var isle = new IsleColorRemap(palette);
        byte[] identity = Enumerable.Range(0, 256).Select(i => (byte)i).ToArray();
        // 범위 밖 값도 중립 표를 반환해야 이전 소유자 색이 남지 않는다.
        foreach (int color in new[] { -1, 0, 9 })
            Assert.Equal(identity, remap.Table(TypeLoadOrder.IndexOf("priest"), color).ToArray());
        // 기존 지면·장식·받침 경로의 여덟 소유자색을 그대로 보존한다.
        foreach (string name in new[] { "isle", "isleBig", "edgeFarm", "island", "islandStalag" })
        {
            // 색 번호별 모든 팔레트 인덱스를 비교한다.
            for (int color = 1; color <= 8; color++)
                Assert.Equal(isle.Table(color).ToArray(), remap.Table(TypeLoadOrder.IndexOf(name), color).ToArray());
        }
    }
}
