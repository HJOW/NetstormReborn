using Netstorm.Core.Bridges;

namespace Netstorm.Core.Tests;

/// <summary>다리 배치 판정·연결망·붕괴 규칙 테스트 (docs/exe/bridge-pieces.md 8절)</summary>
public sealed class BridgeGridTests
{
    /// <summary>시험용 섬: x ≤ 9 인 칸 전체 (오른쪽 가장자리 x = 9)</summary>
    private static bool LeftIsland(int x, int y) => x <= 9;

    /// <summary>가로(K) 한 칸 조각 — 단일 조각(J)을 시계 방향으로 한 번 돌린 것</summary>
    private static BridgePiece HorizontalSingle() => new(BridgePatternCatalog.SinglePiece, 1);

    /// <summary>섬 오른쪽 가장자리 옆에 가로 칸은 붙고, 세로 칸은 붙을 방향이 없어 불가</summary>
    [Fact]
    public void Check_AttachesToIslandEdgeOnlyInLinkDirection()
    {
        var grid = new BridgeGrid(LeftIsland);
        Assert.True(grid.Check(HorizontalSingle(), 10, 5, 1).Allowed);
        BridgePlacementCheck vertical = grid.Check(new BridgePiece(BridgePatternCatalog.SinglePiece), 10, 5, 1);
        Assert.Equal(BridgePlacementProblem.NotAttached, vertical.Problem);
        // 섬에서 떨어진 하늘은 붙을 곳이 없다.
        Assert.Equal(BridgePlacementProblem.NotAttached, grid.Check(HorizontalSingle(), 20, 5, 1).Problem);
    }

    /// <summary>섬 칸·다른 다리·점유 칸과 겹치면 막힘, 월드 밖은 불가</summary>
    [Fact]
    public void Check_RejectsOverlapAndOutOfWorld()
    {
        var grid = new BridgeGrid(LeftIsland, (x, y) => x == 12 && y == 5);
        BridgePlacementCheck onIsland = grid.Check(HorizontalSingle(), 9, 5, 1);
        Assert.Equal(BridgePlacementProblem.Blocked, onIsland.Problem);
        Assert.Equal([(9, 5)], onIsland.BlockedCells);
        Assert.Equal(BridgePlacementProblem.Blocked, grid.Check(HorizontalSingle(), 12, 5, 1).Problem);
        grid.Place(HorizontalSingle(), 10, 5, 1);
        Assert.Equal(BridgePlacementProblem.Blocked, grid.Check(HorizontalSingle(), 10, 5, 1).Problem);
        Assert.Equal(BridgePlacementProblem.OutOfWorld, grid.Check(HorizontalSingle(), 256, 5, 1).Problem);
    }

    /// <summary>내 다리의 열린 끝에는 이어 붙일 수 있고, 남의 다리 끝에는 붙지 않는다</summary>
    [Fact]
    public void Check_AttachesToOwnOpenBridgeEnd()
    {
        var grid = new BridgeGrid(LeftIsland);
        grid.Place(HorizontalSingle(), 10, 5, 1);
        Assert.True(grid.Check(HorizontalSingle(), 11, 5, 1).Allowed);
        Assert.Equal(BridgePlacementProblem.NotAttached, grid.Check(HorizontalSingle(), 11, 5, 2).Problem);
    }

    /// <summary>초목 가장자리처럼 부착 불가 판정이 있으면 그 섬 칸에는 붙지 않는다</summary>
    [Fact]
    public void Check_RespectsIslandEdgeCallback()
    {
        var grid = new BridgeGrid(LeftIsland, canAttachToIsland: (x, y, side, player) => y != 5);
        Assert.Equal(BridgePlacementProblem.NotAttached, grid.Check(HorizontalSingle(), 10, 5, 1).Problem);
        Assert.True(grid.Check(HorizontalSingle(), 10, 6, 1).Allowed);
    }

    /// <summary>
    /// 한쪽만 섬에 붙은 칸: 10초마다 7 → 6 → 5 (보통), 4 에서 금 감, 3 → 2 → 1, 0 이 되는 80초째에 사라진다.
    /// </summary>
    [Fact]
    public void Decay_DanglingBridgeCracksAndCrumbles()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState cell = grid.Place(HorizontalSingle(), 10, 5, 1)[0];
        grid.Update(10);
        Assert.Equal(7, cell.TimeLeft);
        grid.Update(30);
        Assert.Equal((5, BridgeCondition.Normal), (cell.TimeLeft, cell.Condition));
        BridgeDecayResult crack = grid.Update(40);
        Assert.Equal([cell], crack.Cracked);
        Assert.Equal((4, BridgeCondition.Cracked), (cell.TimeLeft, cell.Condition));
        Assert.Empty(grid.Update(70).Removed);
        BridgeDecayResult gone = grid.Update(80);
        Assert.Equal([cell], gone.Removed);
        Assert.Null(grid.At(10, 5));
    }

    /// <summary>
    /// 금 간 품질로 놓인 칸은 처음부터 금 간 상태·수명 4 이고(원본 FUN_00442c80), 한쪽만 붙어 있으면
    /// 10초마다 3 → 2 → 1 → 0 으로 줄어 40초째에 사라진다.
    /// </summary>
    [Fact]
    public void Place_CrackedQualityStartsWeakAndCrumblesSooner()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState cell = grid.Place(HorizontalSingle(), 10, 5, 1, BridgeCondition.Cracked)[0];
        Assert.Equal((BridgeGrid.WeakenedTimeLeft, BridgeCondition.Cracked), (cell.TimeLeft, cell.Condition));
        // 이미 금이 가 있으므로 수명이 줄어도 금 감 알림은 다시 나오지 않는다
        Assert.Empty(grid.Update(10).Cracked);
        Assert.Equal(3, cell.TimeLeft);
        Assert.Empty(grid.Update(30).Removed);
        Assert.Equal([cell], grid.Update(40).Removed);
    }

    /// <summary>
    /// 건물형 유닛이 없어지면 중심 칸 ±2 사각형의 다리가 한 단계 약해진다 (원본 FUN_0044b9e0):
    /// 보통 → 금 감(수명 4), 금 감 → 무너짐, 단단함 → 그대로. 사각형 밖은 영향이 없다.
    /// </summary>
    [Fact]
    public void WeakenAround_DowngradesBridgesByOneStep()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState normal = grid.Place(HorizontalSingle(), 10, 5, 1)[0];
        BridgeCellState outside = grid.Place(HorizontalSingle(), 11, 5, 1)[0];
        BridgeCellState cracked = grid.Place(HorizontalSingle(), 10, 7, 1, BridgeCondition.Cracked)[0];
        BridgeCellState hard = grid.Place(HorizontalSingle(), 10, 8, 1, BridgeCondition.Hard)[0];
        BridgeCellState below = grid.Place(HorizontalSingle(), 10, 9, 1)[0];
        // 중심 (8, 6) → x 6~10, y 4~8
        BridgeDecayResult result = grid.WeakenAround(8, 6);
        Assert.Equal([normal], result.Cracked);
        Assert.Equal((BridgeGrid.WeakenedTimeLeft, BridgeCondition.Cracked), (normal.TimeLeft, normal.Condition));
        Assert.Equal([cracked], result.Removed);
        Assert.Null(grid.At(10, 7));
        Assert.Equal(BridgeCondition.Hard, hard.Condition);
        Assert.Equal((0, BridgeCondition.Normal), (outside.TimeLeft, outside.Condition));
        Assert.Equal(BridgeCondition.Normal, below.Condition);
        // 한 번 더 약해지면 금 간 칸도 무너진다
        Assert.Equal([normal], grid.WeakenAround(8, 6).Removed);
    }

    /// <summary>양 끝이 섬에 붙은 다리는 열린 끝이 없어 무너지지 않는다</summary>
    [Fact]
    public void Decay_BridgeBetweenIslandsIsStable()
    {
        var grid = new BridgeGrid((x, y) => x <= 9 || x >= 12);
        grid.Place(HorizontalSingle(), 10, 5, 1);
        grid.Place(HorizontalSingle(), 11, 5, 1);
        grid.Update(200);
        Assert.All(grid.Cells, c => Assert.Equal((0, BridgeCondition.Normal), (c.TimeLeft, c.Condition)));
        Assert.Single(grid.Networks());
    }

    /// <summary>저장된 다리 값(bridge.type 클러스터)을 글자·변형·상태로 되살린다: J01 보통, J11 금 감, J20 단단함</summary>
    [Fact]
    public void AddStored_DecodesConditionFromOriginalClusters()
    {
        Netstorm.Assets.TypeFrameTable frames = OriginalData.RequireTypes().Find("bridge")!.Definition.Frames;
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState normal = grid.AddStored(frames, frames.Find('J', 'P', 1), 10, 1, 1);
        BridgeCellState cracked = grid.AddStored(frames, frames.Find('J', 'P', 11), 10, 2, 1);
        BridgeCellState hard = grid.AddStored(frames, frames.Find('J', 'P', 20), 10, 3, 1);
        Assert.Equal((new BridgeCell('J', 1), BridgeCondition.Normal), (normal.Cell, normal.Condition));
        Assert.Equal((new BridgeCell('J', 1), BridgeCondition.Cracked), (cracked.Cell, cracked.Condition));
        Assert.Equal((new BridgeCell('J', 1), BridgeCondition.Hard), (hard.Cell, hard.Condition));
        // 세 칸은 남북으로 이어진 한 연결망이다.
        Assert.Single(grid.Networks());
    }

    /// <summary>연결망은 같은 수명을 공유하고, 단단한 칸은 줄지 않는다</summary>
    [Fact]
    public void Decay_NetworkSharesTimeLeftAndHardCellsSurvive()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState first = grid.Place(HorizontalSingle(), 10, 5, 1)[0];
        grid.Update(20);
        Assert.Equal(6, first.TimeLeft);
        BridgeCellState second = grid.Place(HorizontalSingle(), 11, 5, 1)[0];
        grid.Update(30);
        // 새 칸(0)도 연결망 최소값 − 1 로 맞춰진다.
        Assert.Equal((5, 5), (first.TimeLeft, second.TimeLeft));
        second.Condition = BridgeCondition.Hard;
        grid.Update(100);
        Assert.Null(grid.At(10, 5));
        Assert.NotNull(grid.At(11, 5));
    }
}
