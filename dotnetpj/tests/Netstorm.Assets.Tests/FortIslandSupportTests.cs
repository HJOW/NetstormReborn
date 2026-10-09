namespace Netstorm.Assets.Tests;

/// <summary>저장된 받침 논리 칸과 원본 소유자색 프레임 선택을 검증한다.</summary>
public sealed class FortIslandSupportTests
{
    /// <summary>붙어 있는 받침도 건물 기준점에서 각각 복원하며 중복 생성하지 않는다.</summary>
    [Fact]
    public void AdjacentSupports_KeepCreatorAnchors()
    {
        var objects = Square(10, 10, 1).Concat(Square(13, 10, 1)).ToList();
        var creator = new TypeInfo(1, "building", TypeDefinition.Parse("typename building\ntypeflags createsisland;"), 0, 0);
        objects.Add(new FortMapObject(13, 10, null, new FortObject(0, 0, creator, null, null, null, null, null, 1, [])));
        Assert.Equal(new[] { new FortIslandSupport(10, 10, 1), new FortIslandSupport(13, 10, 1) }, FortIslandSupports.Find(objects));
    }

    /// <summary>원본 조건을 만족하지 않는 불완전하거나 소유자가 섞인 칸은 임의의 받침으로 바꾸지 않는다.</summary>
    [Fact]
    public void IncompleteOrMixedOwner_LeavesCellsUnrecognized()
    {
        var cells = Square(10, 10, 1).ToArray();
        Assert.Empty(FortIslandSupports.Find(cells[..8]));
        cells[0] = cells[0] with { Object = cells[0].Object with { Owner = 2 } };
        Assert.Empty(FortIslandSupports.Find(cells));
    }

    /// <summary>플레이어 번호와 색상 번호는 별개다. 중립·알 수 없는 색상은 테두리 없는 P09로 표시한다.</summary>
    [Theory]
    [InlineData(0, 8)]
    [InlineData(1, 6)]
    [InlineData(2, 1)]
    [InlineData(3, 8)]
    public void PlayerColor_SelectsExplicitPalette(int owner, int expected)
    {
        Assert.Equal(expected, FortIslandSupports.ColorCluster(owner, new Dictionary<int, int> { [1] = 7, [2] = 2 }));
    }

    /// <summary>두 공식 맵의 저장 noIsland 칸을 빠짐없이 9칸씩 복원하고 일반 지면과 중복하지 않는지 검사한다.</summary>
    [Theory]
    [InlineData("savetheisland", 26)]
    [InlineData("thewarbegins", 13)]
    public void Original_SavedSupportsCoverExactlyNineCells(string name, int expectedCount)
    {
        var archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var catalog = new TypeCatalog(archive);
        Assert.True(archive.TryFind($"d/{name}.fort", out TaffEntry entry));
        var map = new FortMap(new FortFile(archive.Read(entry), catalog));
        var terrain = new FortTerrainPreview(map, catalog.Find("isle")!.Definition);
        Assert.Equal(expectedCount, terrain.Supports.Count);
        var saved = map.Objects.Where(item => item.Object.Type.Name == "noIsland")
            .ToDictionary(item => (item.X, item.Y), item => item.Object.Owner ?? 0);
        var covered = new HashSet<(int X, int Y)>();
        var tiles = terrain.Tiles.Select(tile => (tile.X, tile.Y)).ToHashSet();
        // 복원된 받침마다 저장된 소유자와 각 칸의 중복 여부를 확인한다.
        foreach (FortIslandSupport support in terrain.Supports)
        {
            // 실제 원본 발자국의 세 행을 확인한다.
            for (int y = support.Y - 2; y <= support.Y; y++)
            {
                // 같은 행의 세 칸이 저장 데이터에 존재해야 한다.
                for (int x = support.X - 2; x <= support.X; x++)
                {
                    Assert.Equal(support.Owner, saved[(x, y)]);
                    Assert.True(covered.Add((x, y)));
                    Assert.DoesNotContain((x, y), tiles);
                }
            }
        }
        Assert.Equal(saved.Count, covered.Count);
        Assert.Equal(expectedCount * 9, covered.Count);
        if (name == "savetheisland")
        {
            Assert.Contains(new FortIslandSupport(175, 94, 1), terrain.Supports);
            Assert.Contains(new FortIslandSupport(165, 137, 0), terrain.Supports);
        }
    }

    /// <summary>
    /// 네 미션에서 받침의 기준점 집합이 cpppj 월드 조립(GameWorld::BuildTerrain)의 묶는 방식과 같다 (LEFT_JOBS.dotnetpj.md 6-4).
    /// cpppj 는 createsisland·geyser 타입 오브젝트의 칸을 저장 순서로 먼저, 이어 noIsland 칸을 (x, y) 순서로 기준점 후보로 보고,
    /// 후보의 왼쪽 위 3×3 이 모두 noIsland 이며 아직 다른 받침에 쓰이지 않았으면 받침으로 삼는다 (소유자가 섞였는지는 보지 않는다).
    /// 받침은 기준점(오른쪽 아래 칸)에 그리며 일반 절벽과 달리 y 를 옮기지 않는다 — 뷰어도 <see cref="FortIslandSupport"/> 의 칸에 그대로 그린다.
    /// </summary>
    /// <param name="name">맵 이름</param>
    /// <param name="expectedCount">받침 수</param>
    [Theory]
    [InlineData("thewarbegins", 13)]
    [InlineData("savetheisland", 26)]
    [InlineData("BridgeTheGap", 0)]
    [InlineData("TEST01", 53)]
    public void Original_SupportAnchorsMatchCppGrouping(string name, int expectedCount)
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), GameLanguage.English);
        TypeCatalog catalog = resources.LoadTypes();
        IReadOnlyList<FortMapObject> objects = new FortMap(resources.LoadFort(name, catalog)).Objects;
        var pad = objects.Where(item => item.Object.Type.Name.Equals("noIsland", StringComparison.OrdinalIgnoreCase))
            .Select(item => (item.X, item.Y)).ToHashSet();
        var candidates = objects.Where(item => item.Object.Type.Definition.HasFlag("createsisland") || item.Object.Type.Definition.HasFlag("geyser"))
            .Select(item => (item.X, item.Y)).ToList();
        candidates.AddRange(pad.OrderBy(cell => cell.X).ThenBy(cell => cell.Y));
        var claimed = new HashSet<(int X, int Y)>();
        var anchors = new List<(int X, int Y)>();
        // cpppj 와 같은 순서로 후보를 보며 완전한 9칸만 받침으로 삼는다.
        foreach ((int x, int y) in candidates)
        {
            var cells = new List<(int X, int Y)>();
            // 기준점의 왼쪽 위 3×3 칸을 모은다.
            for (int i = 0; i < FortIslandSupports.Size * FortIslandSupports.Size; i++)
            {
                cells.Add((x - i % FortIslandSupports.Size, y - i / FortIslandSupports.Size));
            }
            if (cells.Any(cell => !pad.Contains(cell) || claimed.Contains(cell))) continue;
            claimed.UnionWith(cells);
            anchors.Add((x, y));
        }
        IReadOnlyList<FortIslandSupport> supports = FortIslandSupports.Find(objects);
        Assert.Equal(expectedCount, supports.Count);
        Assert.Equal(anchors.OrderBy(a => a.Y).ThenBy(a => a.X), supports.Select(s => (s.X, s.Y)));
    }

    /// <summary>원본처럼 오른쪽 아래 기준점에서 왼쪽 위로 3×3 noIsland 논리 칸을 만든다.</summary>
    private static IEnumerable<FortMapObject> Square(int anchorX, int anchorY, int owner)
    {
        var type = new TypeInfo(0, "noIsland", TypeDefinition.Parse("typename noIsland"), 0, 0);
        // 기준점에 포함되는 세 행의 논리 지면을 만든다.
        for (int y = anchorY - 2; y <= anchorY; y++)
        {
            // 각 행에 세 칸을 추가한다.
            for (int x = anchorX - 2; x <= anchorX; x++)
            {
                yield return new FortMapObject(x, y, null, new FortObject(0, 0, type, null, null, null, null, null, owner, []));
            }
        }
    }
}
