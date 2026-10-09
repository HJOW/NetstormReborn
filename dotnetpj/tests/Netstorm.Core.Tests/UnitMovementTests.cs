using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 일반 이동 명령의 걸음 규칙: 대각선을 먼저 걷는 8방향 경로, 대각선 한 걸음의 √2칸 길이, 바라보는 방향과 화면용 보간 좌표.
/// 근거는 2026-10-03 원본 자동 분석 녹화(docs/videos/auto-war-begins-20261003.md)다.
/// </summary>
public sealed class UnitMovementTests
{
    /// <summary>섬 한 변의 칸 수 (x·y 0~39가 모두 지면).</summary>
    private const int IslandSize = 40;

    /// <summary>걷는 도중 무관한 지면 버전이 바뀌어도 진행량을 잃거나 같은 걸음을 다시 시작하지 않는다.</summary>
    [Theory]
    [InlineData(9, 2)]
    [InlineData(9, 9)]
    public void TerrainChange_PreservesASafeStepInProgress(int x, int y)
    {
        BattleSession changed = Create(2, 2), reference = Create(2, 2);
        changed.Submit(new MoveEntityCommand(1, OriginalData.Sid(1), x, y));
        reference.Submit(new MoveEntityCommand(1, OriginalData.Sid(1), x, y));
        changed.RunTicks(7); reference.RunTicks(7);
        changed.Bridges.InvalidateTerrain();
        changed.RunTicks(1); reference.RunTicks(1);
        Assert.Equal(reference.VisualCell(reference.Entity(OriginalData.Sid(1))!), changed.VisualCell(changed.Entity(OriginalData.Sid(1))!));
        changed.RunTicks(180); reference.RunTicks(180);
        Assert.Equal(reference.Entity(OriginalData.Sid(1))!.Footprint, changed.Entity(OriginalData.Sid(1))!.Footprint);
        Assert.False(changed.IsMoveBlocked(1));
    }

    /// <summary>가로 7칸·세로 3칸 목표로 보내면 대각선 3걸음 뒤 직선 4걸음이 이어진다.</summary>
    [Fact]
    public void MoveToCell_WalksDiagonalsFirstThenStraight()
    {
        BattleSession session = Create(2, 2);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 9, 5));
        List<(int Dx, int Dy)> steps = Walk(session, priest);
        Assert.Equal(new Footprint(9, 5, 1, 1), priest.Footprint);
        // 걸음 수는 큰 쪽 변(7)이고 앞의 셋이 남동 대각선, 나머지 넷이 동쪽이다.
        Assert.Equal([(1, 1), (1, 1), (1, 1), (1, 0), (1, 0), (1, 0), (1, 0)], steps);
    }

    /// <summary>순수한 세로 이동은 대각선 없이 곧게 간다.</summary>
    [Fact]
    public void MoveToCell_VerticalGoalStaysStraight()
    {
        BattleSession session = Create(5, 5);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 5, 12));
        List<(int Dx, int Dy)> steps = Walk(session, priest);
        Assert.Equal(7, steps.Count);
        Assert.All(steps, step => Assert.Equal((0, 1), step));
    }

    /// <summary>대각선 걸음은 직선보다 √2배 오래 걸려 속도가 길이 기준(칸/초)으로 같다.</summary>
    [Fact]
    public void DiagonalStep_TakesSquareRootOfTwoLonger()
    {
        int straight = TicksToArrive(2, 2, 9, 2);
        int diagonal = TicksToArrive(2, 2, 9, 9);
        Assert.InRange(diagonal / (double)straight, 1.38, 1.45);
        // 사제 속도 1.8칸/초: 직선 7칸 = 3.9초 = 약 93틱
        Assert.InRange(straight, 90, 96);
    }

    /// <summary>한 걸음이 끝날 때까지 칸 사이의 좌표를 보여 주고, 도착하면 기준 칸과 같다.</summary>
    [Fact]
    public void VisualCell_InterpolatesBetweenCellsAndSettlesOnArrival()
    {
        BattleSession session = Create(2, 2);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 8, 2));
        // 명령이 실행되고 약 0.45칸을 걸은 시점(6틱 = 0.25초 × 1.8칸/초)
        session.RunTicks(7);
        (double x, double y) = session.VisualCell(priest);
        Assert.Equal(2, priest.Footprint.AnchorX);
        Assert.InRange(x, 2.2, 2.9);
        Assert.Equal(2.0, y);
        Assert.True(session.IsMoving(priest.Id));
        session.RunTicks(24 * 6);
        Assert.False(session.IsMoving(priest.Id));
        Assert.Equal((8.0, 2.0), session.VisualCell(priest));
    }

    /// <summary>걷는 방향이 바라보는 방향으로 기록되고 움직이지 않은 유닛은 방향이 없다.</summary>
    [Theory]
    [InlineData(10, 2, 2)]   // 동 = C
    [InlineData(5, 10, 4)]   // 남 = E
    [InlineData(10, 10, 3)]  // 남동 = D
    [InlineData(1, 10, 5)]   // 남서(대각선 먼저) = F
    public void Heading_FollowsTheWalkingDirection(int x, int y, int expected)
    {
        BattleSession session = Create(5, 2);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        Assert.Equal(UnitHeading.None, priest.Heading);
        session.Submit(new MoveEntityCommand(1, priest.Id, x, y));
        session.RunTicks(3);
        Assert.Equal(expected, priest.Heading);
    }

    /// <summary>방향 번호와 클러스터 측면 글자 대응(A=북 … H=북서).</summary>
    [Theory]
    [InlineData(0, -1, 'A')]
    [InlineData(1, -1, 'B')]
    [InlineData(1, 0, 'C')]
    [InlineData(1, 1, 'D')]
    [InlineData(0, 1, 'E')]
    [InlineData(-1, 1, 'F')]
    [InlineData(-1, 0, 'G')]
    [InlineData(-1, -1, 'H')]
    public void UnitHeading_MapsStepsToClusterSides(int dx, int dy, char side)
    {
        Assert.Equal(side, UnitHeading.Side(UnitHeading.FromStep(dx, dy)));
        // 걸음 크기와 상관없이 부호만 본다.
        Assert.Equal(side, UnitHeading.Side(UnitHeading.FromStep(dx * 7, dy * 3)));
        Assert.Null(UnitHeading.Side(UnitHeading.None));
    }

    /// <summary>모서리가 허공인 대각선은 가로지르지 않고 두 걸음으로 돌아간다.</summary>
    [Fact]
    public void Diagonal_DoesNotCutAcrossAVoidCorner()
    {
        // (5,3) 한 칸만 허공이라 (4,3)→(5,4) 대각선은 허공 모서리를 가로지른다.
        BattleSession session = Create(4, 3, island: (x, y) => x is >= 0 and < IslandSize && y is >= 0 and < IslandSize && (x, y) != (5, 3));
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 5, 4));
        List<(int Dx, int Dy)> steps = Walk(session, priest);
        Assert.Equal([(0, 1), (1, 0)], steps);
    }

    /// <summary>허공 칸을 목적지로 주면 이동 없이 거부된다 (원본: 선택만 풀림).</summary>
    [Fact]
    public void MoveToVoid_IsRejectedWithoutMoving()
    {
        BattleSession session = Create(5, 5);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 60, 60));
        session.RunTicks(2);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.NoRoute);
        Assert.False(session.IsMoving(priest.Id));
        Assert.Equal(new Footprint(5, 5, 1, 1), priest.Footprint);
    }

    /// <summary>걷는 도중 새 명령을 받으면 진행 중이던 걸음을 마저 걷고 새 경로를 이어 화면 위치가 뒤로 튀지 않는다.</summary>
    [Fact]
    public void Redirect_MidStep_KeepsTheVisualPositionContinuous()
    {
        BattleSession session = Create(2, 2);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, 25, 2));
        // 한 걸음(약 13틱)의 3/4쯤 걸은 시점
        session.RunTicks(10);
        (double beforeX, double beforeY) = session.VisualCell(priest);
        Assert.InRange(beforeX, 2.5, 3.0);
        // 반대 방향(서쪽 아래)으로 다시 명령한다. 새 경로는 현재 걸음 끝에서 이어진다.
        session.Submit(new MoveEntityCommand(1, priest.Id, 2, 12));
        session.RunTicks(1);
        (double afterX, double afterY) = session.VisualCell(priest);
        // 뒤로 튀었다면 x가 0.7칸 가까이 줄었을 것이다. 한 틱(0.075칸) 앞으로 가는 정도의 차이만 허용한다.
        Assert.InRange(afterX - beforeX, -0.01, 0.2);
        Assert.InRange(afterY - beforeY, -0.01, 0.2);
        session.RunTicks(24 * 30);
        Assert.Equal(new Footprint(2, 12, 1, 1), priest.Footprint);
        Assert.False(session.IsMoving(priest.Id));
    }

    /// <summary>같은 명령은 방향·보간과 무관하게 같은 검사합을 만든다.</summary>
    [Fact]
    public void DiagonalMovement_IsDeterministic()
    {
        BattleSession first = Create(2, 2);
        BattleSession second = Create(2, 2);
        first.Submit(new MoveEntityCommand(1, OriginalData.Sid(1), 30, 20));
        second.Submit(new MoveEntityCommand(1, OriginalData.Sid(1), 30, 20));
        first.RunTicks(300);
        second.RunTicks(300);
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>한 틱씩 진행하며 칸이 바뀔 때마다 걸음(dx, dy)을 모은다. 명령은 호출 전에 제출한다.</summary>
    private static List<(int Dx, int Dy)> Walk(BattleSession session, GameEntity entity)
    {
        var steps = new List<(int Dx, int Dy)>();
        (int x, int y) = (entity.Footprint.AnchorX, entity.Footprint.AnchorY);
        // 도착해서 이동이 끝나기까지 충분히 오래 진행한다.
        for (int tick = 0; tick < 24 * 60; tick++)
        {
            session.RunTicks(1);
            (int nx, int ny) = (entity.Footprint.AnchorX, entity.Footprint.AnchorY);
            if ((nx, ny) != (x, y))
            {
                steps.Add((nx - x, ny - y));
                (x, y) = (nx, ny);
            }
            else if (tick > 5 && !session.IsMoving(entity.Id)) break;
        }
        return steps;
    }

    /// <summary>시작 칸에서 목적 칸까지 도착하는 데 걸린 틱 수.</summary>
    private static int TicksToArrive(int fromX, int fromY, int toX, int toY)
    {
        BattleSession session = Create(fromX, fromY);
        GameEntity priest = session.Entity(OriginalData.Sid(1))!;
        session.Submit(new MoveEntityCommand(1, priest.Id, toX, toY));
        int ticks = 0;
        // 목적 칸에 닿을 때까지 한 틱씩 진행한다.
        while ((priest.Footprint.AnchorX, priest.Footprint.AnchorY) != (toX, toY) && ticks < 24 * 120)
        {
            session.RunTicks(1);
            ticks++;
        }
        return ticks;
    }

    /// <summary>열린 정사각 섬 한가운데에 내 사제를 둔 세션을 만든다 (가이저·신전 없음).</summary>
    private static BattleSession Create(int priestX, int priestY, Func<int, int, bool>? island = null)
    {
        island ??= (x, y) => x is >= 0 and < IslandSize && y is >= 0 and < IslandSize;
        BattleSession? session = null;
        var grid = new BridgeGrid(island, (x, y) => session?.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)) ?? false);
        TypeInfo type = OriginalData.RequireTypes().Find("priest")!;
        var priest = new FortMapObject(priestX, priestY, 0, new FortObject(0, 0, type, null, null, null, null, null, 1, []));
        session = new BattleSession(new BattleMap([priest], (_, _) => 0), grid, OriginalData.RequireTypes());
        session.CombatEnabled = false;
        return session;
    }
}
