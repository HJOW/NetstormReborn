using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>공유 SID의 생성·반납·소진과 고정 틱별 다리 스캔을 세션 경계에서 검증한다.</summary>
public sealed class SidSessionTests
{
    /// <summary>원본 서버 영역 첫 번호.</summary>
    private const int First = SidPool.ServerFirst1078;

    /// <summary>24Hz에서 한 틱에 처리하는 번호 수: trunc((1/24)/10 × 8001).</summary>
    private const int SlotsPerTick = 33;

    /// <summary>원본 타입으로 합성 저장 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int x, int y) =>
        new(x, y, 0, new FortObject(0, 0, OriginalData.RequireTypes().Find(name)!, null, null, null, null, null, 1, []));

    /// <summary>왼쪽 섬에 붙은 가로 한 칸 조각.</summary>
    private static BridgePiece Plank() => new(BridgePatternCatalog.SinglePiece, 1);

    /// <summary>빈 맵과 공유 풀을 사용하는 세션. 그래프 수는 번호·시간 검증에서 고정한다.</summary>
    private static BattleSession EmptySession(SidPool? pool = null)
    {
        var map = new BattleMap([], (_, _) => 0, sids: pool);
        var grid = new BridgeGrid((x, _) => x == 9, graphSurfaces: _ => BridgeGrid.MinSurfaceGraphSize, sids: map.Sids);
        return new BattleSession(map, grid, OriginalData.RequireTypes());
    }

    /// <summary>원본 120000슬롯 초기화와 16비트 빈 목록 연결을 보존하며 서버 영역은 정상 할당한다.</summary>
    [Fact]
    public void NativeCapacity_KeepsServerRangeAndWrappedLinks()
    {
        var pool = new SidPool(SidPool.NativeCapacity1078);
        Assert.Equal(119995u, pool.FreeCount);
        Assert.Equal(0, pool.Next(65535));
        Assert.Equal(0, pool.Next(pool.Capacity - 1));
        Assert.Equal(First, pool.AllocateWorld(90));
        Assert.Equal((90, 0), (pool.Type(First), pool.State(First)));
    }

    /// <summary>엔티티와 논리 지면·다리가 같은 서버 풀을 공유하고 독립 격자는 생성 시 연결된다.</summary>
    [Fact]
    public void MapAndGrid_ShareServerIdsIncludingLogicalGround()
    {
        var map = new BattleMap([Object("noIsland", 9, 5), Object("sunCannon", 5, 5)], (_, _) => 0);
        var grid = new BridgeGrid((x, _) => x == 9);
        var session = new BattleSession(map, grid, OriginalData.RequireTypes());
        BridgeCellState cell = grid.Place(Plank(), 10, 5, 1)[0];
        Assert.Same(map.Sids, grid.Sids);
        Assert.Equal(First + 1, Assert.Single(session.Entities).Id);
        Assert.Equal(First + 2, cell.Sid);
        Assert.Equal(0, map.Sids.State(First));
        Assert.Equal(First + 3, map.NextId());
    }

    /// <summary>실제 맵에서 저장 다리의 번호를 두 번 할당하지 않고 엔티티와 격자가 로더 번호를 그대로 쓴다.</summary>
    [Fact]
    public void StoredBridges_ReuseMapIds()
    {
        BattleSession session = SessionData.FromMap("b10");
        Assert.Same(session.Map.Sids, session.Bridges.Sids);
        Assert.NotEmpty(session.Bridges.Cells);
        Assert.Equal(session.Entities.Count + session.Bridges.Cells.Count,
            session.Entities.Select(e => e.Id).Concat(session.Bridges.Cells.Select(c => c.Sid)).Distinct().Count());
        // 로더가 등록한 다리마다 프레임 해석으로 만든 현재 격자 칸의 번호를 대조한다.
        foreach ((int id, FortMapObject item) in session.Map.InitialObjects.Where(o => ObjectKinds.Of(o.Item.Object.Type) == ObjectKind.Bridge))
        {
            if (session.Bridges.At(item.X, item.Y) is { } cell) Assert.Equal(id, cell.Sid);
        }
    }

    /// <summary>첫 구간의 끝은 배타적이다. 앞선 다른 오브젝트들이 다리의 첫 스캔을 다음 틱으로 미룬다.</summary>
    [Fact]
    public void Scan_ProcessesBridgeOnlyWhenItsSidIsInTickInterval()
    {
        BattleSession session = EmptySession();
        // 다른 오브젝트가 첫 틱의 전체 번호 구간을 차지한다.
        for (int i = 0; i < SlotsPerTick; i++) session.Map.NextId();
        BridgeCellState cell = session.Bridges.Place(Plank(), 10, 5, 1)[0];
        session.RunTicks(1);
        Assert.Equal(First + SlotsPerTick, session.Bridges.ScanCursor);
        Assert.Equal(0, cell.TimeLeft);
        session.RunTicks(1);
        Assert.Equal(7, cell.TimeLeft);
        session.RunTicks(239);
        Assert.Equal(7, cell.TimeLeft);
        session.RunTicks(1);
        Assert.Equal(6, cell.TimeLeft);
    }

    /// <summary>10초 경계는 나머지 슬롯까지 처리하고 커서를 시작 번호로 돌린다.</summary>
    [Fact]
    public void Scan_PeriodBoundaryVisitsRemainingSlots()
    {
        BattleSession session = EmptySession();
        // 서버 마지막 할당 가능 번호 바로 앞까지 사용한다. 23000번은 예약 꼬리다.
        for (int i = First; i < SidPool.PredictableFirst1078 - 2; i++) session.Map.NextId();
        BridgeCellState cell = session.Bridges.Place(Plank(), 10, 5, 1)[0];
        Assert.Equal(22999, cell.Sid);
        session.RunTicks(239);
        Assert.Equal(0, cell.TimeLeft);
        session.RunTicks(1);
        Assert.Equal(7, cell.TimeLeft);
        Assert.Equal(First, session.Bridges.ScanCursor);
        Assert.Equal(20, session.Bridges.NextScanTime);
    }

    /// <summary>정지 스캔은 수명·커서를 바꾸지 않고 새 세션은 시간 0의 커서로 시작한다.</summary>
    [Fact]
    public void Scan_PauseAndNewSessionPreserveTimeOrigin()
    {
        BattleSession session = EmptySession();
        BridgeCellState cell = session.Bridges.Place(Plank(), 10, 5, 1)[0];
        Assert.Empty(session.Bridges.Update(0, 1.0 / 24, paused: true).Removed);
        Assert.Equal((First, 0), (session.Bridges.ScanCursor, cell.TimeLeft));
        session.RunTicks(1);
        Assert.Equal(7, cell.TimeLeft);
        BattleSession fresh = EmptySession();
        Assert.Equal((0L, First, 10.0), (fresh.Tick, fresh.Bridges.ScanCursor, fresh.Bridges.NextScanTime));
    }

    /// <summary>약화와 붕괴 삭제는 번호를 꼬리에 반납하고 다음 생성은 아직 쓰지 않은 머리 번호를 받는다.</summary>
    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void RemovedBridge_ReturnsSidToFifoTail(bool weakened)
    {
        BattleSession session = EmptySession();
        BridgeCellState cell = session.Bridges.Place(Plank(), 10, 5, 1, BridgeCondition.Cracked)[0];
        if (weakened) session.Bridges.WeakenAround(10, 5);
        else
        {
            cell.TimeLeft = 1;
            session.RunTicks(1);
        }
        Assert.Empty(session.Bridges.Cells);
        Assert.Equal(SidPool.FreeBit | SidPool.DeadBit, session.Map.Sids.State(cell.Sid));
        Assert.Equal(cell.Sid, session.Map.Sids.Tail(false));
        Assert.Equal(cell.Sid, (int)session.Map.Sids.Deletions(false)[0].Sid);
        Assert.Equal(cell.Sid + 1, session.Bridges.Place(Plank(), 10, 5, 1)[0].Sid);
    }

    /// <summary>회수로 제거되는 엔티티도 선택 정리 후 공유 번호를 반납한다.</summary>
    [Fact]
    public void SalvagedEntity_ReturnsSid()
    {
        var map = new BattleMap([Object("sunCannon", 5, 5)], (_, _) => 0);
        var session = new BattleSession(map, new BridgeGrid((_, _) => true), OriginalData.RequireTypes());
        session.Submit(new SalvageCommand(1, First));
        session.RunTicks(1);
        Assert.Empty(session.Entities);
        Assert.Equal(First, map.Sids.Tail(false));
        Assert.Equal(SidPool.FreeBit | SidPool.DeadBit, map.Sids.State(First));
    }

    /// <summary>소진한 풀은 비용·점유·집은 조각·커서를 바꾸지 않고 생성을 거부한다.</summary>
    [Fact]
    public void Exhaustion_RejectsCreationWithoutSpendingOrPartialPiece()
    {
        var pool = new SidPool(15005, predictableFirst: 15003);
        BattleSession session = EmptySession(pool);
        session.EnforceProductionRules = false;
        session.Map.NextId();
        int head = pool.ServerHead;
        uint free = pool.FreeCount;
        Assert.Throws<InvalidOperationException>(() => session.Bridges.Place(new BridgePiece(4), 10, 5, 1));
        Assert.Equal((head, free, 0), (pool.ServerHead, pool.FreeCount, session.Bridges.Cells.Count));
        session.Map.NextId();
        int power = session.Player(1).StormPower;
        session.Submit(new PlaceUnitCommand(1, "sunwalker", 5, 5));
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", 7, 7));
        session.Map.Ownership.SetTemple(0, 1);
        // 설치 조건은 충족시키고 번호 소진만으로 거부되는지 확인한다.
        foreach (Element element in Enum.GetValues<Element>())
            session.Map.AddSource(new EnergySource(First, element, 5, 5, 1));
        Assert.True(session.CheckUnit(1, "sunwalker", 5, 5).Allowed);
        session.Player(1).Deck.SetTemple(First);
        session.Player(1).Tray.Update(0, hasTemple: true);
        session.Submit(new PickBridgePieceCommand(1, 0));
        Assert.True(session.CheckBuilding(1, "sunFactory", 7, 7).Allowed);
        session.Submit(new PlaceBridgeCommand(1, 1, 10, 5));
        session.RunTicks(1);
        Assert.Equal(3, session.DrainEvents().Count(e => e.Failure == CommandFailure.ObjectLimit));
        Assert.Equal(power, session.Player(1).StormPower);
        Assert.NotNull(session.Player(1).HeldPiece);
        Assert.Empty(session.Entities);
        Assert.Empty(session.Bridges.Cells);
        Assert.False(session.Map.IsOccupied(new Footprint(5, 5, 1, 1)));
    }

    /// <summary>보이는 엔티티가 같아도 다음 생성 번호를 바꾸는 FIFO 상태는 검사합에 포함된다.</summary>
    [Fact]
    public void Checksum_IncludesAllocatorHistory()
    {
        BattleSession first = EmptySession();
        BattleSession second = EmptySession();
        int sid = first.Map.NextId();
        first.Map.Sids.ReleaseWorld(sid);
        Assert.Empty(first.Entities);
        Assert.Empty(second.Entities);
        Assert.NotEqual(first.Checksum(), second.Checksum());
    }
}
