namespace Netstorm.Assets.Tests;

/// <summary>저장 프레임과 지면 연결이 원본 데이터에 맞게 해석되는지 확인한다.</summary>
public sealed class MapRenderingTests
{
    /// <summary>저장된 다리 값은 default보다 우선하며 레이어 수를 적용한다.</summary>
    [Fact]
    public void BridgeStoredFrame_UsesClusterAndLayerStride()
    {
        var definition = TypeDefinition.Parse("""
            typename bridge
            typeflags bridge;
            A01 : default : "bridge.gif" #0 : "shadow.gif" #0;
            K01 : : "bridge.gif" #1 : "shadow.gif" #1;
            """);
        var type = new TypeInfo(0, "bridge", definition, 0, TypeFlagBits.Bridge);
        var obj = new FortObject(0, 0, type, null, null, null, 1, null, 1, []);
        Assert.Equal(2, MapSpriteFrames.BodyFrame(obj));
        Assert.Throws<InvalidDataException>(() => MapSpriteFrames.BodyFrame(obj with { BridgeShape = 2 }));
    }

    /// <summary>원본 연결 표의 A(네 방향), J(세로), K(가로), P(독립)를 확인한다.</summary>
    [Theory]
    [InlineData('A', 15)]
    [InlineData('J', 5)]
    [InlineData('K', 10)]
    [InlineData('P', 0)]
    public void TerrainConnections_MatchOriginal(char orientation, int mask)
    {
        Assert.Equal(mask, FortTerrainPreview.ConnectionMask(orientation));
        Assert.Equal(orientation, FortTerrainPreview.MaskOrientation(mask));
    }

    /// <summary>캡처에 대응하는 미션의 지면 생성이 반복 가능하고 테마·타일 번호가 유효한지 확인한다.</summary>
    [Theory]
    [InlineData("savetheisland")]
    [InlineData("thewarbegins")]
    public void Original_TerrainPreviewIsRepeatable(string name)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var map = new FortMap(new FortFile(archive.Read(entry), catalog));
        TypeDefinition isle = catalog.Find("isle")!.Definition;
        var first = new FortTerrainPreview(map, isle);
        var second = new FortTerrainPreview(map, isle);
        Assert.NotEmpty(first.Tiles);
        Assert.Equal(first.Tiles, second.Tiles);
        Assert.Contains(first.Tiles, t => t.Theme == "rain");
        // 모든 생성 타일은 월드 안에 있고 실제 클러스터에 대응해야 한다.
        foreach (FortTerrainTile tile in first.Tiles)
        {
            Assert.InRange(tile.X, 0, 255);
            Assert.InRange(tile.Y, 0, 255);
            Assert.InRange(tile.Cluster, 0, isle.Clusters.Count - 1);
            string image = isle.Clusters[tile.Cluster].Layers[0].Image;
            if (tile.Theme == "rain")
            {
                Assert.StartsWith("RA", image, StringComparison.OrdinalIgnoreCase);
            }
        }
    }
}
