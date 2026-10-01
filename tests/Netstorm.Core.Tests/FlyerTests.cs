using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>원본 타입을 사용하는 작은 지도에서 출격·공격·귀환·점유·결정론 경계를 확인한다.</summary>
public sealed class FlyerTests
{
    /// <summary>무료 비행체는 기지당 하나이며 바다를 건너 이동해 접근한 다음 피해를 준다.</summary>
    [Fact]
    public void Base_BuildsOneFreeFlyerThatCrossesEmptySpaceAndAttacks()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("sunBlocker", 2, 25, 10));
        int money = session.Player(1).StormPower;
        session.RunTicks(120);
        Assert.DoesNotContain(session.Entities, e => e.Kind == ObjectKind.Flyer);
        session.RunTicks(1);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Flyer);
        Assert.Equal(50, flyer.HitPoints);
        Assert.Equal(1, flyer.Flight!.BaseId);
        session.RunTicks(48);
        Assert.InRange(flyer.WorldX, 12, 14);
        Assert.False(session.Map.IsOccupied(flyer.Footprint));
        Assert.Equal(2000, session.Entity(2)!.HitPoints);
        session.RunTicks(240);
        Assert.True(session.Entity(2)!.HitPoints < 2000);
        Assert.Single(session.Entities, e => e.Kind == ObjectKind.Flyer);
        Assert.Equal(money, session.Player(1).StormPower);
    }

    /// <summary>비행체 4대가 같은 적을 보아도 목표 예약은 3개뿐이다.</summary>
    [Fact]
    public void Target_AllowsAtMostThreeFlyers()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("sunaviary", 1, 10, 15),
            Object("sunaviary", 1, 10, 20), Object("sunaviary", 1, 10, 25), Object("sunBlocker", 2, 25, 17));
        session.RunTicks(160);
        GameEntity[] flyers = session.Entities.Where(e => e.Flight != null).ToArray();
        Assert.Equal(4, flyers.Length);
        Assert.Equal(3, flyers.Count(e => e.AttackTargetId == 5));
        Assert.Single(flyers, e => e.Flight!.Phase == FlyerPhase.Docked);
    }

    /// <summary>수송·중립·아군·출발점 사거리 밖은 공격하지 않는다.</summary>
    [Theory]
    [InlineData("sunwalker", 2, 20)]
    [InlineData("sunBalloon", 2, 20)]
    [InlineData("sunBlocker", 0, 20)]
    [InlineData("sunBlocker", 1, 20)]
    [InlineData("sunBlocker", 2, 41)]
    public void Flyer_RejectsInvalidTargets(string type, int owner, int x)
    {
        // 제외 대상인 지상 수송이 낙하로 사라져 검사가 무의미해지지 않게 발판을 둔다.
        var map = new BattleMap([Object("sunaviary", 1, 10, 10), Object(type, owner, x, 10)], (_, _) => null);
        var session = new BattleSession(map, new BridgeGrid((gx, gy) => gx == x && gy == 10, (_, _) => false), OriginalData.RequireTypes());
        session.RunTicks(400);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Flight != null);
        Assert.Equal(0, flyer.AttackTargetId);
        Assert.Equal(FlyerPhase.Docked, flyer.Flight!.Phase);
        Assert.Empty(session.Shots);
        if (type == "sunwalker") Assert.NotNull(session.Entity(2));
    }

    /// <summary>커스텀 동맹 콜백도 포대와 같은 판정을 사용한다.</summary>
    [Fact]
    public void Flyer_RespectsAlliedPlayers()
    {
        var map = new BattleMap([Object("sunaviary", 1, 10, 10), Object("sunBlocker", 2, 20, 10)],
            (_, _) => null, allied: (_, _) => true);
        var session = new BattleSession(map, Grid(), OriginalData.RequireTypes());
        session.RunTicks(400);
        Assert.Equal(0, Assert.Single(session.Entities, e => e.Flight != null).AttackTargetId);
    }

    /// <summary>1분 뒤 공격을 멈춰 귀환하고 같은 비행체가 보급 후 재출격한다.</summary>
    [Fact]
    public void Flyer_ReturnsRefuelsAndLaunchesAgain()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("sunBlocker", 2, 18, 10));
        session.RunTicks(121);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Flight != null);
        session.RunTicks(1440);
        Assert.Equal(FlyerPhase.Returning, flyer.Flight!.Phase);
        Assert.Equal(0, flyer.AttackTargetId);
        session.RunTicks(200);
        Assert.Same(flyer, session.Entity(flyer.Id));
        Assert.Equal(FlyerPhase.Hunting, flyer.Flight.Phase);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.FlyerRefuelling);
    }

    /// <summary>기지가 사라져도 출격 중인 비행체는 남고, 연료 종료 후 보상 없이 사라진다.</summary>
    [Fact]
    public void BaseRemoval_LeavesSortieAliveUntilReturn()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("sunBlocker", 2, 18, 10));
        session.RunTicks(200);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Flight != null);
        int enemyMoney = session.Player(2).StormPower;
        session.Submit(new SalvageCommand(1, 1));
        session.RunTicks(1);
        Assert.Null(session.Entity(1));
        Assert.Same(flyer, session.Entity(flyer.Id));
        session.RunTicks(1440);
        Assert.DoesNotContain(session.Entities, e => e.Flight != null);
        Assert.Equal(enemyMoney, session.Player(2).StormPower);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.FlyerExpired);
    }

    /// <summary>전진 후 목표가 없어져도 출발 기지의 사거리 밖 적을 연속 추격하지 않는다.</summary>
    [Fact]
    public void Retarget_UsesOriginalBaseRange()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10),
            Object("sunBlocker", 2, 38, 10), Object("sunBlocker", 2, 45, 10));
        session.RunTicks(400);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Flight != null);
        Assert.True(flyer.WorldX > 30);
        session.Submit(new SalvageCommand(2, 2));
        session.RunTicks(1);
        Assert.Equal(0, flyer.AttackTargetId);
        Assert.Equal(FlyerPhase.Returning, flyer.Flight!.Phase);
    }

    /// <summary>비행체가 피격되어 사라져도 바로 아래 건물의 점유·체력과 파괴 보상은 그대로다.</summary>
    [Fact]
    public void FlyerDeath_DoesNotClearGroundOccupancyOrAwardMoney()
    {
        BattleSession session = Create(Object("sunFlyer", 1, 10, 10), Object("sunBlocker", 1, 10, 10),
            Object("sunCannon", 2, 18, 11));
        int money = session.Player(2).StormPower;
        session.RunTicks(30);
        Assert.Null(session.Entity(1));
        Assert.True(session.Map.IsOccupied(new Footprint(10, 10, 1, 1)));
        Assert.Equal(2000, session.Entity(2)!.HitPoints);
        Assert.Equal(money, session.Player(2).StormPower);
    }

    /// <summary>기지와 사선이 어긋난 적 포대가 접근한 비행체를 파괴하면 기지가 새 번호로 재생성한다.</summary>
    [Fact]
    public void FlyerDeath_BaseBuildsReplacement()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("sunCannon", 2, 25, 12));
        session.RunTicks(121);
        int first = Assert.Single(session.Entities, e => e.Flight != null).Id;
        session.RunTicks(550);
        Assert.Null(session.Entity(first));
        Assert.NotNull(session.Entity(1));
        Assert.True(session.DrainEvents().Count(e => e.Kind == SessionEventKind.FlyerLaunched) >= 2);
        Assert.InRange(session.Entities.Count(e => e.Flight != null), 0, 1);
    }

    /// <summary>동일 틱열에서 비행 좌표·연료 상태가 재현되며 전투를 끄면 위치·피해가 멈춘다.</summary>
    [Fact]
    public void Flight_IsDeterministicAndStopsWithCombat()
    {
        FortMapObject[] objects = [Object("sunaviary", 1, 10, 10), Object("sunBlocker", 2, 25, 10)];
        BattleSession first = Create(objects);
        BattleSession second = Create(objects);
        first.RunTicks(200);
        second.RunTicks(200);
        Assert.Equal(first.Checksum(), second.Checksum());
        GameEntity flyer = Assert.Single(first.Entities, e => e.Flight != null);
        double x = flyer.WorldX;
        first.CombatEnabled = false;
        first.RunTicks(500);
        Assert.Equal(x, flyer.WorldX);
        Assert.Equal(2000, first.Entity(2)!.HitPoints);
    }

    /// <summary>지형의 영향을 제외한 빈 다리 격자를 만든다.</summary>
    private static BridgeGrid Grid() => new((_, _) => false, (_, _) => false);

    /// <summary>지면 없는 작은 전투 세션을 만든다.</summary>
    private static BattleSession Create(params FortMapObject[] objects) =>
        new(new BattleMap(objects, (_, _) => null), Grid(), OriginalData.RequireTypes());

    /// <summary>실제 타입으로 저장 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name) ?? throw new InvalidDataException(name);
        return new FortMapObject(x, y, null, new FortObject(0, 0, type, null, null, null, null, null, owner, []));
    }
}
