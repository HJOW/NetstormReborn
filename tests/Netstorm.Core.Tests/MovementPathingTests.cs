using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>수확 도중 경로 단절·우회·대기·복구를 실제 타입 속도와 다리 연결로 검사한다.</summary>
public sealed class MovementPathingTests
{
    /// <summary>새 명령 없이 기존 수확 작업이 복구된 다리를 건너 결정을 전달한다.</summary>
    [Fact]
    public void Harvest_WaitsForRepairThenDeliversWithOriginalOrder()
    {
        BattleSession session = Create();
        StartThenCut(session);
        GameEntity priest = session.Entity(1)!;
        Footprint waiting = priest.Footprint;
        session.RunTicks(48);
        Assert.True(session.IsMoveBlocked(priest.Id));
        Assert.Equal(waiting, priest.Footprint);
        Assert.False(priest.IsStunned);
        Assert.Equal(0, priest.CarriedCrystals);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.MoveBlocked);

        Repair(session);
        int before = session.Player(1).StormPower;
        // 첫 결정 전달까지 진행하고 복구 이후의 이벤트를 모아 검사한다.
        var events = new List<SessionEvent>();
        for (int tick = 0; tick < session.TicksPerSecond * 25 && session.Player(1).StormPower == before; tick++)
        {
            session.RunTicks(1);
            events.AddRange(session.DrainEvents());
        }
        Assert.Equal(before + StormPower.CrystalValue, session.Player(1).StormPower);
        Assert.False(session.IsMoveBlocked(priest.Id));
        Assert.False(priest.IsSuspended);
        Assert.Single(events, e => e.Kind == SessionEventKind.MoveResumed);
        Assert.Single(events, e => e.Kind == SessionEventKind.CrystalDelivered);
    }

    /// <summary>길이 막힌 뒤 다른 곳만 바뀌어도 실패 이벤트를 반복하지 않는다.</summary>
    [Fact]
    public void BlockedHarvest_EmitsOnceAndCancelsWhenTempleDisappears()
    {
        BattleSession session = Create();
        StartThenCut(session);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.MoveBlocked);
        AddBridge(session, 10, 70);
        session.RunTicks(2);
        Assert.True(session.IsMoveBlocked(1));
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.MoveBlocked);
        session.Submit(new SalvageCommand(1, 2));
        session.RunTicks(1);
        Assert.Null(session.Entity(2));
        Assert.False(session.IsMoveBlocked(1));
        Repair(session);
        session.RunTicks(240);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind is SessionEventKind.MoveResumed or SessionEventKind.CrystalCollected);
    }

    /// <summary>우회 지면이 있으면 끊긴 길을 밟지 않고 같은 가이저까지 계속 이동한다.</summary>
    [Fact]
    public void Harvest_RepathsOverDetourWithoutBlockingOrFalling()
    {
        BattleSession session = Create(detour: true);
        StartThenCut(session);
        GameEntity priest = session.Entity(1)!;
        bool usedDetour = false;
        var events = new List<SessionEvent>();
        // 가이저로 가는 첫 여정에서 우회 행을 통과했는지 확인한다.
        for (int tick = 0; tick < session.TicksPerSecond * 15 && priest.CarriedCrystals == 0; tick++)
        {
            session.RunTicks(1);
            usedDetour |= priest.Footprint.AnchorY == 48;
            events.AddRange(session.DrainEvents());
        }
        Assert.True(usedDetour);
        Assert.Equal(1, priest.CarriedCrystals);
        Assert.False(priest.IsSuspended);
        Assert.DoesNotContain(events, e => e.Kind is SessionEventKind.MoveBlocked or SessionEventKind.UnitFell or SessionEventKind.PriestSuspended);
    }

    /// <summary>대기 상태와 복구 후 이동도 같은 틱열에서 동일한 검사합을 만든다.</summary>
    [Fact]
    public void BlockedAndResumedHarvest_IsDeterministic()
    {
        BattleSession first = Create();
        BattleSession second = Create();
        StartThenCut(first);
        StartThenCut(second);
        Assert.True(first.IsMoveBlocked(1));
        Assert.Equal(first.Checksum(), second.Checksum());
        Repair(first);
        Repair(second);
        first.RunTicks(360);
        second.RunTicks(360);
        Assert.False(first.IsMoveBlocked(1));
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>두 섬 사이 직선 다리와 반복 수확 대상·완공 신전을 배치한다.</summary>
    private static BattleSession Create(bool detour = false)
    {
        BattleSession? session = null;
        var grid = new BridgeGrid((x, y) => x <= 9 || x >= 20 || detour && y == 48,
            (x, y) => session?.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)) ?? false);
        session = new BattleSession(new BattleMap([Object("priest", 9, 50), Object("windVortex", 6, 51), Object("geyser", 23, 51)],
            (x, _) => x <= 9 ? 0 : x >= 20 ? 1 : null), grid, OriginalData.RequireTypes());
        session.CombatEnabled = false;
        // 가이저 섬까지 열 칸의 가로 다리를 잇는다.
        for (int x = 10; x <= 19; x++) AddBridge(session, x, 50);
        return session;
    }

    /// <summary>사제가 다리 앞부분에 도착한 뒤 발밑을 남기고 뒤쪽 네 칸을 끊는다.</summary>
    private static void StartThenCut(BattleSession session)
    {
        session.Submit(new HarvestGeyserCommand(1, 3));
        session.RunTicks(24);
        Assert.Equal(10, session.Entity(1)!.Footprint.AnchorX);
        session.Bridges.WeakenAround(18, 50);
        session.Bridges.WeakenAround(18, 50);
        session.RunTicks(1);
    }

    /// <summary>수확 목표를 바꾸지 않고 사라진 다리 네 칸만 복구한다.</summary>
    private static void Repair(BattleSession session)
    {
        // 원래 지상 경로의 끊긴 구간에 한 칸씩 다리를 잇는다.
        for (int x = 16; x <= 19; x++)
            session.Bridges.Place(new BridgePiece(BridgePatternCatalog.SinglePiece, 1), x, 50, 1);
    }

    /// <summary>실제 프레임의 저장 다리 칸을 추가한다.</summary>
    private static void AddBridge(BattleSession session, int x, int y)
    {
        TypeFrameTable frames = OriginalData.RequireTypes().Find("bridge")!.Definition.Frames;
        session.Bridges.AddStored(frames, frames.Find('K', TypeFrameTable.DefaultVariant, 1), x, y, 1);
    }

    /// <summary>실제 설치본의 타입으로 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        return new FortMapObject(x, y, x <= 9 ? 0 : 1, new FortObject(0, 0, type, null, null, null, null, null, 1, []));
    }
}
