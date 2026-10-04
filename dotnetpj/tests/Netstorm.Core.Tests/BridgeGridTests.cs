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

    /// <summary>
    /// 섬에 붙은 판자 두 칸: 양쪽이 이어진 안쪽 칸은 열린 쪽이 없어 구동자가 아니고, 바깥 끝 칸만 10초마다 줄어
    /// 80초에 먼저 사라진다 (원본 FUN_004217f0, 2026-09-29 관찰 "두 칸짜리는 끝 칸이 먼저 사라진다"). 그다음 안쪽 칸이 끝 칸이 되어 처음부터 다시 센다.
    /// </summary>
    [Fact]
    public void Decay_PlankChainCollapsesFromTheTip()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState inner = grid.Place(HorizontalSingle(), 10, 5, 1)[0];
        BridgeCellState tip = grid.Place(HorizontalSingle(), 11, 5, 1)[0];
        grid.Update(10);
        Assert.Equal((0, 7), (inner.TimeLeft, tip.TimeLeft));
        grid.Update(40);
        Assert.Equal((0, BridgeCondition.Normal, 4, BridgeCondition.Cracked), (inner.TimeLeft, inner.Condition, tip.TimeLeft, tip.Condition));
        // 80초 스캔: 안쪽 칸을 먼저 살피는 동안 바깥 끝 칸이 아직 있어 수명이 줄지 않고, 곧이어 끝 칸이 사라진다
        BridgeDecayResult firstGone = grid.Update(80);
        Assert.Equal([tip], firstGone.Removed);
        Assert.Equal(0, inner.TimeLeft);
        // 이제 안쪽 칸이 끝 칸이다: 90초에 7 부터 시작해 160초에 사라진다
        grid.Update(90);
        Assert.Equal(7, inner.TimeLeft);
        Assert.Empty(grid.Update(150).Removed);
        Assert.Equal(1, inner.TimeLeft);
        Assert.Equal([inner], grid.Update(160).Removed);
    }

    /// <summary>
    /// 접합 칸(A~I)에 이어진 판자 무리: 섬 쪽 판자는 경계라 수명이 줄지 않고, 접합 칸과 그 끝 판자 두 개가 함께 줄어든다.
    /// 끝 판자 두 개가 각각 구동자라 한 스캔에 두 번씩 줄어 40초에 모두 사라진다. 그다음 섬에 붙은 판자가 끝 칸이 되어 50초에 7 부터 시작한다.
    /// </summary>
    [Fact]
    public void Decay_JunctionClusterDecaysTogetherButAttachedPlankWaits()
    {
        var grid = new BridgeGrid(LeftIsland);
        // 조각 4(북 판자 + 갈래 + 동 판자 + 남 판자)를 180° 돌려 동쪽 판자가 서쪽(섬 쪽)을 보게 놓는다
        var piece = new BridgePiece(4, 2);
        Assert.True(grid.Check(piece, 10, 5, 1).Allowed);
        grid.Place(piece, 10, 5, 1);
        BridgeCellState top = grid.At(11, 5)!;
        BridgeCellState plank = grid.At(10, 6)!;
        BridgeCellState junction = grid.At(11, 6)!;
        BridgeCellState bottom = grid.At(11, 7)!;
        Assert.Equal(("J", "K", "D", "J"), ($"{top.Cell.Letter}", $"{plank.Cell.Letter}", $"{junction.Cell.Letter}", $"{bottom.Cell.Letter}"));
        // 첫 스캔: 위 판자가 구동자로 7, 이어서 아래 판자가 구동자로 6
        grid.Update(10);
        Assert.Equal((6, 6, 6, 0), (top.TimeLeft, junction.TimeLeft, bottom.TimeLeft, plank.TimeLeft));
        // 두 번째 스캔: 5 를 거쳐 4 가 되며 세 칸 모두 금이 간다. 섬에 붙은 판자는 그대로다
        grid.Update(20);
        Assert.All(new[] { top, junction, bottom }, c => Assert.Equal((4, BridgeCondition.Cracked), (c.TimeLeft, c.Condition)));
        Assert.Equal((0, BridgeCondition.Normal), (plank.TimeLeft, plank.Condition));
        BridgeDecayResult gone = grid.Update(40);
        Assert.Equal(3, gone.Removed.Count);
        Assert.Null(grid.At(11, 5));
        Assert.Null(grid.At(11, 6));
        Assert.Null(grid.At(11, 7));
        Assert.Equal(0, plank.TimeLeft);
        grid.Update(50);
        Assert.Equal(7, plank.TimeLeft);
    }

    /// <summary>
    /// 섬에서 떨어져 나온 표면 무리가 5칸 미만이면 다음 스캔에서 한 번에 무너진다 (원본 Graph.cpp numSurface &lt; 5).
    /// 5칸짜리는 양 끝부터 줄어 80초에 무너지고, 그때 끝 칸이 사라지면서 남은 4칸이 같은 스캔에서 함께 무너진다.
    /// </summary>
    [Fact]
    public void Decay_DetachedFragmentUnderFiveCellsCrumblesAtOnce()
    {
        var small = new BridgeGrid(LeftIsland);
        small.Place(new BridgePiece(3, 1), 20, 5, 1);
        Assert.Equal(4, small.Cells.Count);
        BridgeDecayResult result = small.Update(10);
        Assert.Equal(4, result.Removed.Count);
        Assert.Empty(small.Cells);

        var chain = new BridgeGrid(LeftIsland);
        chain.Place(new BridgePiece(3, 1), 20, 5, 1);
        chain.Place(HorizontalSingle(), 24, 5, 1);
        Assert.Equal(5, chain.Cells.Count);
        chain.Update(70);
        // 양 끝만 줄고 가운데 세 칸은 그대로다
        Assert.Equal((1, 0, 1), (chain.At(20, 5)!.TimeLeft, chain.At(22, 5)!.TimeLeft, chain.At(24, 5)!.TimeLeft));
        Assert.Equal(5, chain.Cells.Count);
        Assert.Equal(5, chain.Update(80).Removed.Count);
        Assert.Empty(chain.Cells);
    }

    /// <summary>단단한 끝 칸은 구동자가 되지 않아 오래 두어도 줄지 않는다</summary>
    [Fact]
    public void Decay_HardTipNeverCrumbles()
    {
        var grid = new BridgeGrid(LeftIsland);
        BridgeCellState hard = grid.Place(HorizontalSingle(), 10, 5, 1, BridgeCondition.Hard)[0];
        grid.Update(300);
        Assert.NotNull(grid.At(10, 5));
        Assert.Equal((0, BridgeCondition.Hard), (hard.TimeLeft, hard.Condition));
    }
}
