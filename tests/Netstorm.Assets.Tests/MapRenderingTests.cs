using System.Security.Cryptography;

namespace Netstorm.Assets.Tests;

/// <summary>저장 프레임과 지면 연결이 원본 데이터에 맞게 해석되는지 확인한다.</summary>
public sealed class MapRenderingTests
{
    /// <summary>원본 마스크 비교용 월드 한 변의 칸 수.</summary>
    private const int MaskSize = FortFile.WorldChunksX * FortMap.CellsPerChunk;

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

    /// <summary>원본 어셈블리에서 독립 재현한 Python 마스크와 전체 65,536칸·영역별 크기를 대조한다.</summary>
    [Theory]
    [InlineData("savetheisland", "68e699f55ad34041f316f4466f6c33d2edb6807833a648462a64ebbc2c505cc8", 0, 1384, 7, 698)]
    [InlineData("thewarbegins", "b24af563875ef935aa1053d521a6abf8490c4ddea840ac1845b43223fd0436d1", 0, 954, 1, 551)]
    public void Original_IslandMaskMatchesIndependentReconstruction(string name, string expectedHash,
        int firstRegion, int firstCount, int secondRegion, int secondCount)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var map = new FortMap(new FortFile(archive.Read(entry), catalog));
        var terrain = new FortTerrainPreview(map, catalog.Find("isle")!.Definition);
        var mask = Enumerable.Repeat(byte.MaxValue, MaskSize * MaskSize).ToArray();
        // 본섬만 비교하여 미확정인 건물·가이저 받침 생성이 기준값에 섞이지 않게 한다.
        foreach (FortTerrainCell cell in terrain.IslandCells)
        {
            Assert.Equal(byte.MaxValue, mask[cell.Y * MaskSize + cell.X]);
            mask[cell.Y * MaskSize + cell.X] = checked((byte)cell.Region);
        }
        Assert.Equal(expectedHash, Convert.ToHexStringLower(SHA256.HashData(mask)));
        Assert.Equal(firstCount, terrain.IslandCells.Count(cell => cell.Region == firstRegion));
        Assert.Equal(secondCount, terrain.IslandCells.Count(cell => cell.Region == secondRegion));
        Assert.Equal(firstCount + secondCount, terrain.IslandCells.Count);
    }
}
