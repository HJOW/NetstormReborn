using Netstorm.Assets;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Tests;

/// <summary>초목이 있는 섬 가장자리에서는 다리를 시작할 수 없다는 규칙 테스트 (사용자 확인 2026-09-30)</summary>
public sealed class BridgeAnchorTests
{
    /// <summary>원본 타입 플래그: edgeFarm·신전·워크샵·가이저는 dropBlocking, 섬 지면·다리는 아니다</summary>
    [Fact]
    public void DropBlocking_MatchesOriginalTypeFlags()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        Assert.True(BridgeAnchors.IsDropBlocking(types.Find("edgeFarm")!));
        Assert.True(BridgeAnchors.IsDropBlocking(types.Find("windVortex")!));
        Assert.True(BridgeAnchors.IsDropBlocking(types.Find("sunFactory")!));
        Assert.True(BridgeAnchors.IsDropBlocking(types.Find("geyser")!));
        Assert.False(BridgeAnchors.IsDropBlocking(types.Find("isle")!));
        Assert.False(BridgeAnchors.IsDropBlocking(types.Find("bridge")!));
    }

    /// <summary>초목 칸 옆에는 붙지 않고, 초목 없는 가장자리 칸 옆에는 붙는다</summary>
    [Fact]
    public void BridgeGrid_RejectsVegetatedEdge()
    {
        var blocked = BridgeAnchors.DropBlockingCells([], [(9, 5)]);
        var grid = new BridgeGrid((x, y) => x <= 9, canAttachToIsland: BridgeAnchors.CanAttach(blocked));
        var horizontal = new BridgePiece(BridgePatternCatalog.SinglePiece, 1);
        Assert.Equal(BridgePlacementProblem.NotAttached, grid.Check(horizontal, 10, 5, 1).Problem);
        Assert.True(grid.Check(horizontal, 10, 6, 1).Allowed);
    }

    /// <summary>
    /// 원본 Bridge the Gap! 맵: 가장자리 초목(edgeFarm 미리보기)이 있는 칸 옆에서는 어떤 방향 조각으로도 시작할 수 없고,
    /// 초목도 오브젝트도 없는 가장자리 칸 옆에서는 시작할 수 있다.
    /// </summary>
    [Fact]
    public void BridgeTheGap_VegetationBlocksBridgeStart()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        GameResources resources = OriginalData.RequireResources();
        var map = new FortMap(resources.LoadFort("bridgethegap", types));
        TypeDefinition isle = types.Find("isle")!.Definition;
        var terrain = new FortTerrainPreview(map, isle);
        var edgeFarms = FortEdgeFarmPreview.Create(map, terrain, isle, types.Find("edgeFarm")!.Definition);
        Assert.NotEmpty(edgeFarms);
        var island = terrain.IslandCells.Select(c => (c.X, c.Y)).ToHashSet();
        var blocked = BridgeAnchors.DropBlockingCells(map.Objects, edgeFarms.Select(t => (t.X, t.Y)));
        var grid = new BridgeGrid((x, y) => island.Contains((x, y)), canAttachToIsland: BridgeAnchors.CanAttach(blocked));
        // 섬 가장자리 칸마다 바깥 네 방향에 한 칸 조각을 대 보고, 초목 칸이면 모두 불가, 아니면 하나 이상 가능해야 한다
        (int Dx, int Dy, int Rotation)[] outward = [(0, -1, 0), (1, 0, 1), (0, 1, 0), (-1, 0, 1)];
        int vegetated = 0, clear = 0;
        foreach ((int x, int y) in island)
        {
            var sides = outward.Where(o => !island.Contains((x + o.Dx, y + o.Dy))).ToArray();
            if (sides.Length == 0)
            {
                continue;
            }
            bool anyAllowed = sides.Any(o => grid.Check(new BridgePiece(BridgePatternCatalog.SinglePiece, o.Rotation), x + o.Dx, y + o.Dy, 1).Allowed
                && !Neighbours(x + o.Dx, y + o.Dy).Any(n => n != (x, y) && island.Contains(n)));
            if (blocked.Contains((x, y)))
            {
                vegetated++;
                Assert.All(sides, o => Assert.False(AttachesOnlyTo(grid, island, x, y, o)));
            }
            else if (anyAllowed)
            {
                clear++;
            }
        }
        Assert.True(vegetated > 0 && clear > 0, $"초목 가장자리 {vegetated}칸, 시작 가능한 가장자리 {clear}칸");
        // 뷰어 확인 명령(docs/map-viewer.md)에 쓰는 오른쪽 가장자리: (61,42) 는 초목 없음, (61,47) 은 초목 칸
        Assert.DoesNotContain((61, 42), blocked);
        Assert.Contains((61, 47), blocked);
    }

    /// <summary>(x, y) 섬 칸 하나에만 닿는 조각이 붙는지 (다른 섬 칸과 동시에 닿는 경우는 판정에서 제외)</summary>
    private static bool AttachesOnlyTo(BridgeGrid grid, HashSet<(int, int)> island, int x, int y, (int Dx, int Dy, int Rotation) side)
    {
        (int px, int py) = (x + side.Dx, y + side.Dy);
        var piece = new BridgePiece(BridgePatternCatalog.SinglePiece, side.Rotation);
        // 조각의 연결 방향으로 닿는 다른 섬 칸이 있으면 이 칸만의 판정이 아니므로 붙지 않은 것으로 본다
        (int, int) other = (px + side.Dx, py + side.Dy);
        return !island.Contains(other) && grid.Check(piece, px, py, 1).Allowed;
    }

    /// <summary>네 방향 이웃 칸</summary>
    private static IEnumerable<(int, int)> Neighbours(int x, int y) => [(x, y - 1), (x + 1, y), (x, y + 1), (x - 1, y)];
}
