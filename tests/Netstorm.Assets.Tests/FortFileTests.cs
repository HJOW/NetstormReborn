namespace Netstorm.Assets.Tests;

/// <summary>.fort 해석 테스트 (docs/formats/fort.md 의 확인 결과와 일치해야 한다)</summary>
public sealed class FortFileTests
{
    /// <summary>클론 데이터의 전체 .fort 수 (원본 느슨한 파일 431 + 커스텀 맵 TEST01·TEST02 2 + 아카이브 32)</summary>
    private const int ExpectedFortCount = 465;

    /// <summary>원본 월드 청크 수 (16 × 16)</summary>
    private const int ExpectedWorldChunks = 256;

    /// <summary>타입 이름 해시: "dude" 는 4글자라 바이트가 그대로 이어 붙은 값이 된다</summary>
    [Fact]
    public void NameHash_MatchesOriginalRule()
    {
        Assert.Equal(0x65647564u, TypeCatalog.NameHash("dude"));
        // 20바이트를 넘는 이름은 잘라서 계산한다
        Assert.Equal(TypeCatalog.NameHash("fakeThreeByThreeSurf"), TypeCatalog.NameHash("fakeThreeByThreeSurface"));
    }

    /// <summary>파생 플래그 규칙: walker 는 container, buried 는 saveQA</summary>
    [Fact]
    public void ComputeFlags_AppliesDerivedRules()
    {
        (uint f1, _) = TypeCatalog.ComputeFlags(["walker", "priest", "shadow", "saveQa"]);
        Assert.NotEqual(0u, f1 & TypeFlagBits.Container);
        Assert.NotEqual(0u, f1 & TypeFlagBits.SaveQA);
        (uint b1, _) = TypeCatalog.ComputeFlags(["buried"]);
        Assert.NotEqual(0u, b1 & TypeFlagBits.SaveQA);
    }

    /// <summary>원본 b0.fort: 이름·스톰 파워·청크 수·오브젝트 구성</summary>
    [Fact]
    public void Original_B0()
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        var fort = new FortFile(File.ReadAllBytes(OriginalData.RequireFile("d/b0.fort")), catalog);
        Assert.Equal("Unnamed", fort.Name);
        Assert.Equal(963f, fort.Money);
        Assert.Equal(ExpectedWorldChunks, fort.Chaff.Count);
        var counts = fort.Chaff.Concat(fort.Territories.SelectMany(t => t))
            .SelectMany(c => c.Objects).GroupBy(o => o.Type.Name).ToDictionary(g => g.Key, g => g.Count());
        Assert.Equal(58, counts["geyser"]);
        Assert.Equal(5, counts["priest"]);
    }

    /// <summary>공식 미션의 Deck 항목이 원본 Deck.cpp 저장 순서와 TypeNames 변환 결과에 맞는지 확인한다.</summary>
    [Theory]
    [InlineData("savetheisland", 4)]
    [InlineData("thewarbegins", 3)]
    public void Original_DeckEntriesMatchSourceFormat(string name, byte firstChance)
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var fort = new FortFile(archive.Read(entry), new TypeCatalog(archive));
        Assert.Equal(38, fort.Deck.Count);
        FortDeckEntry first = fort.Deck[0];
        Assert.Equal(0x47, first.TypeNumber);
        Assert.Equal("sunArcher", first.Type?.Name);
        Assert.Equal(firstChance, first.Chance);
        Assert.Equal(1, first.Power);
        Assert.Equal(255, first.NumRemaining);
        Assert.Equal(0xA8, fort.Deck[^1].TypeNumber);
        Assert.Equal("bombSpecialOne", fort.Deck[^1].Type?.Name);
    }

    /// <summary>공식 미션의 Technology가 타입 상태와 container 개수 바이트를 올바르게 읽는지 확인한다.</summary>
    [Theory]
    [InlineData("savetheisland")]
    [InlineData("thewarbegins")]
    public void Original_TechnologyEntriesMatchSourceFormat(string name)
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var fort = new FortFile(archive.Read(entry), new TypeCatalog(archive));
        Assert.Equal(26, fort.Technology.Count);
        FortTechnologyEntry first = fort.Technology[0];
        Assert.Equal(0x47, first.TypeNumber);
        Assert.Equal("sunArcher", first.Type.Name);
        Assert.Equal(4, first.ListFlags);
        Assert.Null(first.QA);
        Assert.Null(first.QB);
        FortTechnologyEntry blocker = Assert.Single(fort.Technology, item => item.Type.Name == "thunderBlocker");
        Assert.Empty(blocker.Contents);
        Assert.Equal("rainWalker", fort.Technology[^1].Type.Name);
    }

    /// <summary>원본 전체 .fort (느슨한 파일 + 아카이브) 가 예외 없이 해석되고 월드가 256 청크다</summary>
    [Fact]
    public void Original_AllFortsParse()
    {
        string dataDir = OriginalData.RequireDirectory();
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        var files = new List<(string Name, byte[] Data)>();
        string dDir = GameDataLocator.FindFile(dataDir, "d") ?? Path.Combine(dataDir, "d");
        // 느슨한 d/*.fort
        foreach (string path in Directory.EnumerateFiles(dDir).Where(p => p.EndsWith(".fort", StringComparison.OrdinalIgnoreCase)))
        {
            files.Add((Path.GetFileName(path), File.ReadAllBytes(path)));
        }
        // 아카이브 안의 .fort
        foreach (TaffEntry entry in archive.Entries.Where(e => e.Name.EndsWith(".fort", StringComparison.OrdinalIgnoreCase)))
        {
            files.Add((entry.Name, archive.Read(entry)));
        }
        Assert.Equal(ExpectedFortCount, files.Count);
        // 파일마다 해석하고 월드 크기를 확인
        foreach ((string name, byte[] data) in files)
        {
            var fort = new FortFile(data, catalog);
            Assert.True(fort.Chaff.Count == ExpectedWorldChunks, $"{name}: Chaff 청크 {fort.Chaff.Count}개");
            ReadOnlySpan<byte> deck = fort.Section("Deck");
            Assert.Equal(deck.IsEmpty ? 0 : deck[0], fort.Deck.Count);
            ReadOnlySpan<byte> technology = fort.Section("Technology");
            Assert.Equal(technology.IsEmpty ? 0 : technology[0], fort.Technology.Count);
            var map = new FortMap(fort);
            Assert.Equal(fort.Chaff.Concat(fort.Territories.SelectMany(t => t)).Sum(c => c.Objects.Count), map.Objects.Count);
            // 저장된 다리 프레임이 실제 bridge 본체 클러스터 범위에 들어가는지 전수 검사한다.
            foreach (FortMapObject item in map.Objects.Where(o => o.Object.BridgeShape.HasValue))
            {
                Assert.InRange(MapSpriteFrames.BodyFrame(item.Object), 0, item.Object.Type.Definition.Clusters.Count - 1);
            }
        }
    }

    /// <summary>원본 화면의 템플릿 매칭으로 확인한 건물 좌표와 영역 배치를 대조한다.</summary>
    [Fact]
    public void Original_SaveTheIsland_MatchesScreenshotCoordinates()
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        Assert.True(archive.TryFind("d/savetheisland.fort", out TaffEntry entry));
        var map = new FortMap(new FortFile(archive.Read(entry), new TypeCatalog(archive)));
        Assert.Contains(map.Objects, o => o.Object.Type.Name == "rainVortex" && o.Territory == 7 && o.X == 187 && o.Y == 108);
        Assert.Contains(map.Objects, o => o.Object.Type.Name == "priest" && o.Territory == 7 && o.X == 185 && o.Y == 118);
        Assert.Contains(map.Objects, o => o.Object.Type.Name == "rainBattery" && o.Territory == null && o.X == 193 && o.Y == 106);
        Assert.Equal(new[] { (10, 6), (11, 6), (10, 7), (11, 7) }, FortMap.TerritoryChunks(map.Territories[7]));
    }

    /// <summary>저장된 방향을 생성 함수의 회전으로 변환하고 빈 셀을 제외한다 (원본 없이 실행).</summary>
    [Theory]
    [InlineData(0, 1, 1, 2, 2)]
    [InlineData(1, 1, 1, 2, 2)]
    [InlineData(2, 2, 1, 2, 2)]
    [InlineData(3, 2, 1, 2, 2)]
    public void TerritoryPattern_RotatesSparseShape(int rotation, int firstX, int firstY, int lastX, int lastY)
    {
        var territory = new FortTerritory(0, (byte)(3 | rotation << 6), 9, 0, 0, 0, 0);
        var positions = FortMap.TerritoryChunks(territory);
        Assert.Equal(3, positions.Count);
        Assert.Equal((firstX, firstY), positions[0]);
        Assert.Equal((lastX, lastY), positions[^1]);
        Assert.Empty(FortMap.TerritoryChunks(territory with { Flags = 3 }));
    }
}
