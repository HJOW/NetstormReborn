using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>발밑 다리 붕괴·허공 사제 복귀·공중 구조와 지상 점유의 경계를 실제 타입으로 검증한다.</summary>
public sealed class GroundSupportTests
{
    /// <summary>자연 붕괴가 일어난 바로 그 틱에 지상 유닛을 제거하고 선택·점유를 정리한다.</summary>
    [Fact]
    public void NaturalCollapse_RemovesWalkerAtEndOfSameTick()
    {
        BattleSession session = Create((_, _) => false, Object("sunwalker", 1, 10, 10));
        AddBridge(session, 10, 10);
        session.Submit(new SelectEntityCommand(1, 1));
        session.RunTicks(239);
        Assert.NotNull(session.Entity(1));
        session.RunTicks(1);
        Assert.Null(session.Bridges.At(10, 10));
        Assert.Null(session.Entity(1));
        Assert.Equal(0, session.Player(1).SelectedEntityId);
        Assert.False(session.Map.IsOccupied(new Footprint(10, 10, 1, 1)));
        Assert.Equal(240, Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.UnitFell).Tick);
    }

    /// <summary>사제는 체력에 관계없이 그 자리에서 기절하고, 점유를 비워 다리 복구를 허용한다.</summary>
    [Fact]
    public void PriestSuspension_AllowsBridgeRepairAndRecoversWithoutNewOrders()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 1, 10, 10));
        AddBridge(session, 10, 10);
        session.RunTicks(1);
        Cut(session, 10, 10);
        session.RunTicks(1);
        GameEntity priest = session.Entity(1)!;
        Assert.True(priest.IsStunned);
        Assert.True(priest.IsSuspended);
        Assert.Equal(100, priest.HitPoints);
        Assert.Equal((10, 10), (priest.Footprint.AnchorX, priest.Footprint.AnchorY));
        Assert.False(priest.OccupiesGround);
        Assert.False(session.Map.IsOccupied(priest.Footprint));
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.PriestSuspended);
        BridgePlacementCheck repair = session.Bridges.Check(Horizontal(), 10, 10, 1);
        Assert.True(repair.Allowed);
        session.Bridges.Place(Horizontal(), 10, 10, 1);
        session.RunTicks(1);
        Assert.False(priest.IsStunned);
        Assert.False(priest.IsSuspended);
        Assert.True(session.Map.IsOccupied(priest.Footprint));
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.PriestRecovered);
    }

    /// <summary>평소 피해 기절과 달리 허공 기절은 정확히 절반 체력에서도 발판 복구로 풀린다.</summary>
    [Fact]
    public void SuspendedPriest_RecoversAtExactlyHalfHealth()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 1, 10, 10), Object("sunCannon", 2, 18, 11));
        AddBridge(session, 10, 10);
        session.RunTicks(30);
        GameEntity priest = session.Entity(1)!;
        Assert.True(priest.IsStunned);
        Assert.Equal(50, priest.HitPoints);
        session.CombatEnabled = false;
        Cut(session, 10, 10);
        session.RunTicks(1);
        session.Bridges.Place(Horizontal(), 10, 10, 1);
        session.RunTicks(1);
        Assert.False(priest.IsStunned);
        Assert.Equal(50, priest.HitPoints);
    }

    /// <summary>허공에 있는 동안 신전 회복은 적용하지 않고 체력·위치를 유지한다.</summary>
    [Fact]
    public void SuspendedPriest_DoesNotHealInTempleRange()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 1, 10, 10),
            Object("sunCannon", 2, 18, 11), Object("windVortex", 1, 6, 9));
        AddBridge(session, 10, 10);
        session.RunTicks(30);
        Cut(session, 10, 10);
        session.RunTicks(1);
        GameEntity priest = session.Entity(1)!;
        double health = priest.HitPoints;
        session.RunTicks(200);
        Assert.Equal(health, priest.HitPoints);
        Assert.True(priest.IsSuspended);
        Assert.True(priest.IsStunned);
    }

    /// <summary>허공의 사제는 옆의 지상 수송으로 집을 수 없지만 비행 수송은 포획한다.</summary>
    [Fact]
    public void SuspendedPriest_RequiresAirborneTransport()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 2, 10, 10),
            Object("sunwalker", 1, 9, 10), Object("sunBalloon", 1, 12, 10));
        session.RunTicks(1);
        session.Submit(new CapturePriestCommand(1, 2, 1));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NoRoute);
        session.Submit(new CapturePriestCommand(1, 3, 1));
        session.RunTicks(60);
        GameEntity priest = session.Entity(1)!;
        Assert.Equal(PriestCaptivity.Carried, priest.Captivity);
        Assert.Equal(3, priest.CaptorId);
        Assert.False(priest.IsSuspended);
        // 원래 칸에 다리가 복구되어도 운반 중인 사제를 자유 상태로 풀지 않는다.
        session.Bridges.Place(Horizontal(), 10, 10, 1);
        session.RunTicks(1);
        Assert.Equal(PriestCaptivity.Carried, priest.Captivity);
    }

    /// <summary>비행 운반체가 허공에서 사라져도 사제를 땅으로 순간이동시키지 않는다.</summary>
    [Fact]
    public void AirCarrierRemoval_ReleasesPriestAtCurrentUnsupportedPosition()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 2, 10, 10), Object("sunBalloon", 1, 12, 10));
        session.RunTicks(1);
        session.Submit(new CapturePriestCommand(1, 2, 1));
        session.RunTicks(60);
        GameEntity priest = session.Entity(1)!;
        Footprint position = priest.Footprint;
        session.Submit(new SalvageCommand(1, 2));
        session.RunTicks(1);
        Assert.Null(session.Entity(2));
        Assert.Equal(PriestCaptivity.Free, priest.Captivity);
        Assert.Equal(position, priest.Footprint);
        Assert.True(priest.IsSuspended);
        Assert.Equal(100, priest.HitPoints);
        Assert.False(session.Map.IsOccupied(priest.Footprint));
    }

    /// <summary>운반 중 지상 수송이 낙하하면 포로를 풀어 그 자리에서 기절시키며 적 보상을 주지 않는다.</summary>
    [Fact]
    public void GroundCarrierFall_ReleasesCaptiveWithoutKillReward()
    {
        MissionStart mission = MissionStart.FromHeader(k => k == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(LeftIsland, mission, Object("priest", 2, 9, 10), Object("sunwalker", 1, 10, 10));
        AddBridge(session, 10, 10);
        session.Submit(new CapturePriestCommand(1, 2, 1));
        session.RunTicks(2);
        Assert.Equal(PriestCaptivity.Carried, session.Entity(1)!.Captivity);
        int money = session.Player(2).StormPower;
        Cut(session, 10, 10);
        session.RunTicks(1);
        Assert.Null(session.Entity(2));
        GameEntity priest = session.Entity(1)!;
        Assert.Equal(PriestCaptivity.Free, priest.Captivity);
        Assert.True(priest.IsSuspended);
        Assert.Equal((10, 10), (priest.Footprint.AnchorX, priest.Footprint.AnchorY));
        Assert.Equal(money, session.Player(2).StormPower);
    }

    /// <summary>비행 수송·공격체는 바닥이 없어도 낙하하지 않는다.</summary>
    [Theory]
    [InlineData("sunBalloon")]
    [InlineData("sunFlyer")]
    public void AirborneEntity_DoesNotNeedSupport(string type)
    {
        BattleSession session = Create((_, _) => false, Object(type, 1, 10, 10));
        session.RunTicks(20);
        Assert.NotNull(session.Entity(1));
        Assert.False(session.Entity(1)!.OccupiesGround);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.UnitFell);
    }

    /// <summary>어느 소유자의 다리라도 발판은 되며 지나갈 경로 허용과 지지를 구분한다.</summary>
    [Fact]
    public void EnemyBridge_StillSupportsStandingWalker()
    {
        BattleSession session = Create((_, _) => false, Object("sunwalker", 1, 10, 10));
        AddBridge(session, 10, 10, owner: 2);
        session.RunTicks(20);
        Assert.NotNull(session.Entity(1));
    }

    /// <summary>허공 기절도 사제의 생존 상태이므로 사망·실패 이벤트를 잘못 내지 않는다.</summary>
    [Fact]
    public void SuspendedPriest_RemainsAliveForMissionEvents()
    {
        MissionStart mission = MissionStart.FromHeader(k => k == "loadFort" ? "test" : null);
        BattleSession session = Create(LeftIsland, mission, Object("priest", 1, 10, 10));
        AddBridge(session, 10, 10);
        session.RunTicks(1);
        session.DrainEvents();
        Cut(session, 10, 10);
        session.RunTicks(100);
        Assert.True(session.Entity(1)!.IsSuspended);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.MissionTell &&
            (e.Text == "Failed" || e.Text.EndsWith("PriestDead", StringComparison.OrdinalIgnoreCase)));
    }

    /// <summary>비행 수송의 생성·회수는 같은 칸의 건물 점유를 증감하지 않는다.</summary>
    [Fact]
    public void AirCarrierRemoval_DoesNotLeaveOrClearGroundOccupancy()
    {
        BattleSession session = Create((_, _) => true, Object("sunBlocker", 1, 10, 10), Object("sunBalloon", 1, 10, 10));
        session.Submit(new SalvageCommand(1, 2));
        session.RunTicks(1);
        Assert.True(session.Map.IsOccupied(new Footprint(10, 10, 1, 1)));
        session.Submit(new SalvageCommand(1, 1));
        session.RunTicks(1);
        Assert.False(session.Map.IsOccupied(new Footprint(10, 10, 1, 1)));
    }

    /// <summary>처음부터 허공에 있는 사제는 옆 땅으로 걸어 나가지 못하고 첫 틱에 기절한다.</summary>
    [Fact]
    public void UnsupportedPriest_CannotWalkOutBeforeSupportCheck()
    {
        BattleSession session = Create(LeftIsland, Object("priest", 1, 10, 10), Object("altar", 1, 8, 10));
        session.Submit(new MovePriestToAltarCommand(1, 2, 1));
        session.RunTicks(1);
        Assert.True(session.Entity(1)!.IsSuspended);
        Assert.Equal(10, session.Entity(1)!.Footprint.AnchorX);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NoRoute);
    }

    /// <summary>실제 미션의 건물 받침은 회수 시 없어지고, 경로용 지형 버전도 갱신된다.</summary>
    [Fact]
    public void Factory_RemovingCreatesIslandRemovesItsStoredSupport()
    {
        BattleSession session = SessionData.FromMap("savetheisland");
        GameEntity support = session.Entities.First(e => e.Kind == ObjectKind.Emplacement && e.Type.Definition.HasFlag("createsisland") &&
            session.Map.TerritoryAt(e.Footprint.AnchorX, e.Footprint.AnchorY) == null);
        Assert.True(session.Bridges.IsIsland(support.Footprint.AnchorX, support.Footprint.AnchorY));
        int version = session.Bridges.Version;
        session.CombatEnabled = false;
        session.Submit(new SalvageCommand(support.Owner, support.Id));
        session.RunTicks(1);
        Assert.Null(session.Entity(support.Id));
        Assert.False(session.Bridges.IsIsland(support.Footprint.AnchorX, support.Footprint.AnchorY));
        Assert.True(session.Bridges.Version > version);
    }

    /// <summary>동일한 붕괴·복구 틱열은 기절 상태까지 동일한 검사합을 만든다.</summary>
    [Fact]
    public void CollapseAndRepair_AreDeterministic()
    {
        BattleSession first = Create(LeftIsland, Object("priest", 1, 10, 10));
        BattleSession second = Create(LeftIsland, Object("priest", 1, 10, 10));
        // 두 세션에 동일한 발판 소멸과 복구를 적용한다.
        foreach (BattleSession session in new[] { first, second })
        {
            AddBridge(session, 10, 10);
            session.RunTicks(1);
            Cut(session, 10, 10);
            session.RunTicks(2);
            session.Bridges.Place(Horizontal(), 10, 10, 1);
            session.RunTicks(1);
        }
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>왼쪽 본섬의 동쪽 해안.</summary>
    private static bool LeftIsland(int x, int _) => x <= 9;
    /// <summary>가로 한 칸짜리 다리.</summary>
    private static BridgePiece Horizontal() => new(BridgePatternCatalog.SinglePiece, 1);
    /// <summary>발밑 칸만 약화·제거한다.</summary>
    private static void Cut(BattleSession session, int x, int y)
    {
        session.Bridges.WeakenAround(x, y);
        session.Bridges.WeakenAround(x, y);
        Assert.Null(session.Bridges.At(x, y));
    }
    /// <summary>원본 프레임 표에서 저장된 가로 다리 칸을 추가한다.</summary>
    private static void AddBridge(BattleSession session, int x, int y, int owner = 1)
    {
        TypeFrameTable frames = OriginalData.RequireTypes().Find("bridge")!.Definition.Frames;
        session.Bridges.AddStored(frames, frames.Find('K', TypeFrameTable.DefaultVariant, 1), x, y, owner);
    }
    /// <summary>원본 타입과 점유 콜백을 사용하는 작은 세션을 만든다.</summary>
    private static BattleSession Create(Func<int, int, bool> ground, params FortMapObject[] objects) => Create(ground, null, objects);
    /// <summary>포획 규칙을 위한 선택적 미션을 적용한다.</summary>
    private static BattleSession Create(Func<int, int, bool> ground, MissionStart? mission, params FortMapObject[] objects)
    {
        BattleSession? session = null;
        var grid = new BridgeGrid(ground, (x, y) => session?.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)) ?? false);
        session = new BattleSession(new BattleMap(objects, (_, _) => 0), grid, OriginalData.RequireTypes(), mission);
        return session;
    }
    /// <summary>실제 설치본 타입으로 저장 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        return new FortMapObject(x, y, 0, new FortObject(0, 0, type, null, null, null, null, null, owner, []));
    }
}
