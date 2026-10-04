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
