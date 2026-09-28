using System.Security.Cryptography;

namespace Netstorm.Assets.Tests;

/// <summary>저장 프레임과 지면 연결이 원본 데이터에 맞게 해석되는지 확인한다.</summary>
public sealed class MapRenderingTests
{
    /// <summary>원본 마스크 비교용 월드 한 변의 칸 수.</summary>
    private const int MaskSize = FortFile.WorldChunksX * FortMap.CellsPerChunk;

    /// <summary>직선·모서리 지면은 원본 방향 폴백으로 해당 절벽을 찾고 전투의 unlit 변형을 제외한다.</summary>
    [Theory]
    [InlineData("EI01", "EA")]
    [InlineData("HN01", "HA")]
    [InlineData("IO01", "IA")]
    [InlineData("AB01", "AB")]
    public void Fringe_UsesOrientationFallbackAndBattleLighting(string sourceName, string expectedOrientation)
    {
        var source = new Cluster(sourceName, ["rim", "fringe"], []);
        var definition = TypeDefinition.Parse($"""
            typename fringe
            {expectedOrientation}01 : lit : "fringe.gif" #0;
            {expectedOrientation}02 : unlit : "fringe.gif" #1;
            """);
        Assert.Equal(0, FortTerrainFringe.SelectCluster(source, definition, 1, 0));
        Assert.Equal(1, FortTerrainFringe.SelectCluster(source, definition, 1, 0, battleMode: false));
        Assert.Null(FortTerrainFringe.SelectCluster(source with { Flags = ["rim"] }, definition, 1, 0));
        Assert.Null(FortTerrainFringe.SelectCluster(source, TypeDefinition.Parse("typename fringe"), 1, 0));
    }

    /// <summary>공식 맵의 모든 fringe 지면에 하나의 절벽이 있고 원본 이동량·방향·조명 조건을 만족한다.</summary>
    [Theory]
    [InlineData("savetheisland")]
    [InlineData("thewarbegins")]
    public void Original_FringesFollowGeneratedRim(string name)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        TypeDefinition isle = catalog.Find("isle")!.Definition;
        TypeDefinition fringe = catalog.Find("fringe")!.Definition;
        var terrain = new FortTerrainPreview(new FortMap(new FortFile(archive.Read(entry), catalog)), isle);
        var sprites = FortTerrainFringe.Create(terrain, isle, fringe);
        Assert.NotEmpty(sprites);
        Assert.Equal(sprites, FortTerrainFringe.Create(terrain, isle, fringe));
        Assert.Equal(terrain.Tiles.Count(tile => isle.Clusters[tile.Cluster].Flags.Contains("fringe")), sprites.Count);
        var tiles = terrain.Tiles.ToDictionary(tile => (tile.X, tile.Y));
        // 실제 지면과 절벽의 관계를 검증하며 원본 전투 모드의 금지 조명 변형이 없는지 검사한다.
        foreach (FortTerrainFringeSprite sprite in sprites)
        {
            FortTerrainTile tile = tiles[(sprite.X, sprite.Y - 4)];
            Cluster source = isle.Clusters[tile.Cluster];
            Cluster selected = fringe.Clusters[sprite.Cluster];
            Assert.Contains("fringe", source.Flags);
            Assert.Equal(source.Name[0], selected.Name[0]);
            Assert.DoesNotContain("unlit", selected.Flags);
            Assert.InRange(MapSpriteFrames.BodyFrame(fringe, sprite.Cluster), 0, fringe.Clusters.Count - 1);
        }
    }

    /// <summary>공식 맵에서 edgeFarm은 원본 청크별 배치 수와 isle 프레임 대응을 지킨다.</summary>
    [Theory]
    [InlineData("savetheisland", 220)]
    [InlineData("thewarbegins", 180)]
    public void Original_EdgeFarmsMatchRimFramesAndChunkBudget(string name, int expectedCount)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var map = new FortMap(new FortFile(archive.Read(entry), catalog));
        TypeDefinition isle = catalog.Find("isle")!.Definition;
        TypeDefinition edgeFarm = catalog.Find("edgeFarm")!.Definition;
        var terrain = new FortTerrainPreview(map, isle);
        var farms = FortEdgeFarmPreview.Create(map, terrain, isle, edgeFarm);
        Assert.Equal(expectedCount, farms.Count);
        Assert.Equal(farms, FortEdgeFarmPreview.Create(map, terrain, isle, edgeFarm));
        Assert.Equal(farms.Count, farms.Select(tile => (tile.X, tile.Y)).Distinct().Count());
        var tiles = terrain.Tiles.ToDictionary(tile => (tile.X, tile.Y));
        // 선택된 칸마다 지면과 농장의 방향·원소·소유자가 일치하는지 확인한다.
        foreach (FortEdgeFarmTile farm in farms)
        {
            FortTerrainTile tile = tiles[(farm.X, farm.Y)];
            Assert.Equal(tile.Region, farm.Region);
            Assert.Equal(tile.Owner, farm.Owner);
            Assert.Equal(tile.Cluster, farm.Cluster);
            Assert.Contains("rim", isle.Clusters[farm.Cluster].Flags);
            Assert.Equal(isle.Clusters[farm.Cluster].Name, edgeFarm.Clusters[farm.Cluster].Name);
            Assert.InRange(MapSpriteFrames.BodyFrame(edgeFarm, farm.Cluster), 0, edgeFarm.Clusters.Count - 1);
        }
    }

    /// <summary>원본 edgeFarm의 모서리는 isle과 프레임 번호를 공유하되 돌출 높이가 더 크다.</summary>
    [Fact]
    public void Original_EdgeFarmFrameExtendsBeyondIsle()
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        var shapes = ShapeDatabase.Load(OriginalData.RequireFile("d/_shapes.shp"));
        TypeInfo isle = catalog.Find("isle")!;
        TypeInfo edgeFarm = catalog.Find("edgeFarm")!;
        int cluster = isle.Definition.Clusters.ToList().FindIndex(item => item.Name == "AB07");
        Assert.InRange(cluster, 0, edgeFarm.Definition.Clusters.Count - 1);
        Assert.Equal("AB07", edgeFarm.Definition.Clusters[cluster].Name);
        ShapeFrame ground = shapes.Blocks[isle.LoadIndex].Frames[cluster];
        ShapeFrame farm = shapes.Blocks[edgeFarm.LoadIndex].Frames[cluster];
        Assert.True(farm.Height > ground.Height + 20);
    }

    /// <summary>저장된 다리 값은 default보다 우선하며 본체는 그림자 레이어 앞에 연속으로 저장된다.</summary>
    [Fact]
    public void BridgeStoredFrame_UsesBodyLayerCluster()
    {
        var definition = TypeDefinition.Parse("""
            typename bridge
            typeflags bridge;
            A01 : default : "bridge.gif" #0 : "shadow.gif" #0;
            K01 : : "bridge.gif" #1 : "shadow.gif" #1;
            """);
        var type = new TypeInfo(0, "bridge", definition, 0, TypeFlagBits.Bridge);
        var obj = new FortObject(0, 0, type, null, null, null, 1, null, 1, []);
        Assert.Equal(1, MapSpriteFrames.BodyFrame(obj));
        Assert.Throws<InvalidDataException>(() => MapSpriteFrames.BodyFrame(obj with { BridgeShape = 2 }));
    }

    /// <summary>
    /// 원본 거주지(randframe)는 섬 원소별 그림을 쓴다 (Dissolved Alliance! 캡처: 바람 섬 = 흙집, 비 섬 = 돔).
    /// 원소마다 해당 그림의 lit 클러스터를 고르고, 저장 프레임이 있으면 그 값을 우선한다.
    /// </summary>
    [Theory]
    [InlineData("sun", "RESIDENCE")]
    [InlineData("rain", "RARESIDENCE")]
    [InlineData("wind", "WRESIDENCE")]
    [InlineData("thunder", "THRESIDENCE")]
    public void Original_ResidenceUsesTerritoryTheme(string theme, string image)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        TypeInfo type = new TypeCatalog(archive).Find("residence")!;
        var obj = new FortObject(0, 0, type, null, null, null, null, null, 0, []);
        // 좌표가 달라도 같은 원소 그림 안에서만 고른다
        foreach ((int x, int y) in new[] { (142, 140), (103, 134), (0, 0) })
        {
            Cluster cluster = type.Definition.Clusters[MapSpriteFrames.BodyFrame(new FortMapObject(x, y, 0, obj), theme)];
            Assert.Equal(image, Path.GetFileNameWithoutExtension(cluster.Layers[0].Image), ignoreCase: true);
            Assert.Contains("lit", cluster.Flags);
        }
        Assert.Equal(3, MapSpriteFrames.BodyFrame(new FortMapObject(0, 0, 0, obj with { Frame = 3 }), theme));
    }

    /// <summary>원본 그림 이름 접두어로 원소를 판정한다.</summary>
    [Theory]
    [InlineData("RAGRASS.GIF", "rain")]
    [InlineData("THRESIDENCE.GIF", "thunder")]
    [InlineData("WIGRASS.GIF", "wind")]
    [InlineData("WRESIDENCE.GIF", "wind")]
    [InlineData("residence.gif", "sun")]
    public void ImageTheme_FromPrefix(string image, string theme) =>
        Assert.Equal(theme, MapSpriteFrames.ImageTheme(image));

    /// <summary>원본 가이저 기본 B00은 본체 49번이며 그림자 영역으로 건너뛰지 않아야 한다.</summary>
    [Fact]
    public void Original_GeyserDefaultUsesBodyInsteadOfShadow()
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var type = new TypeCatalog(archive).Find("geyser")!;
        var obj = new FortObject(0, 0, type, null, null, null, null, null, 0, []);
        int frameIndex = MapSpriteFrames.BodyFrame(obj);
        Assert.Equal(49, frameIndex);
        Assert.Equal("B00", type.Definition.Clusters[frameIndex].Name);
        var shapes = ShapeDatabase.Load(OriginalData.RequireFile("d/_shapes.shp"));
        ShapeBlock block = shapes.Blocks[type.LoadIndex];
        Assert.False(block.Frames[frameIndex].IsSpecial);
        // 원본 본체는 높이가 큰 광맥이고 뒤쪽 그림자 프레임과 형태가 다르다.
        Assert.True(block.Frames[frameIndex].YMax - block.Frames[frameIndex].YMin >
            block.Frames[frameIndex + type.Definition.Clusters.Count].YMax
            - block.Frames[frameIndex + type.Definition.Clusters.Count].YMin);
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
