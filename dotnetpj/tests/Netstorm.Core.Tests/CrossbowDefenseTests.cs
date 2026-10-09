using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>원본 각도표·장전 간격·방위 면역을 실제 타입으로 검사한다.</summary>
public sealed class CrossbowDefenseTests
{
    /// <summary>배치·저장 방위가 북·동·남·서 실제 각도표와 일치한다.</summary>
    [Theory]
    [InlineData(0, 0)]
    [InlineData(1, 25)]
    [InlineData(2, 50)]
    [InlineData(3, 70)]
    public void Placement_UsesOriginalCrossbowAngles(int direction, int frame)
    {
        Assert.Equal(frame, CrossbowAnimation.PlacementFrame(direction));
        Assert.Equal(direction, CrossbowAnimation.SavedDirection(frame));
        Assert.Equal(frame, EmplacementDirection.PlacementFrame(OriginalData.RequireTypes().Find("windArcher")!, direction));
    }

    /// <summary>조준 그림이 바뀔 때 탄을 놓지 않고 0.1초 간격 뒤 준비된 자세에서 발사한다.</summary>
    [Fact]
    public void Crossbow_TurnsBeforeReleasingAndKeepsFixedSector()
    {
        BattleSession session = Create(Object("windArcher", 1, 20, 30, 0), Object("sunBlocker", 2, 25, 18));
        GameEntity bow = session.Entity(OriginalData.Sid(1))!;
        session.RunTicks(1);
        Assert.Equal(5, bow.CrossbowFrame);
        Assert.Empty(session.Shots);
        session.RunTicks(2);
        Assert.Empty(session.Shots);
        session.RunTicks(1);
        Assert.Equal(9, bow.CrossbowFrame);
        Assert.Equal(0, bow.CannonDirection);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(4, shot.FiredTick);
        Assert.Equal(shot.FiredTick + Math.Ceiling(Math.Sqrt(169) / 35 * 24), shot.ImpactTick);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CombatSound && e.Text == "sunDiscThrowerRatchet02.wav");
    }

    /// <summary>발사 후 네 장전 그림이 각각 0.2초 진행되어야 두 번째 탄을 놓는다.</summary>
    [Fact]
    public void Crossbow_ReloadsBeforeSecondRelease()
    {
        BattleSession session = Create(Object("windArcher", 1, 20, 30, 0), Object("sunBlocker", 2, 20, 18));
        GameEntity bow = session.Entity(OriginalData.Sid(1))!;
        session.RunTicks(1);
        Assert.Equal(4, bow.CrossbowFrame);
        Assert.Equal(1, bow.LastShotTick);
        session.RunTicks(1);
        Assert.Equal(3, bow.CrossbowFrame);
        session.RunTicks(5);
        Assert.Equal(2, bow.CrossbowFrame);
        session.RunTicks(5);
        Assert.Equal(1, bow.CrossbowFrame);
        session.RunTicks(5);
        Assert.Equal(0, bow.CrossbowFrame);
        Assert.Equal(1, bow.LastShotTick);
        session.RunTicks(5);
        Assert.Equal(4, bow.CrossbowFrame);
        Assert.Equal(22, bow.LastShotTick);
        Assert.Equal(2, session.DrainEvents().Count(e => e.Kind == SessionEventKind.ShotFired));
    }

    /// <summary>장전 중 목표가 사라지면 발사 사건을 더 만들지 않는다.</summary>
    [Fact]
    public void Crossbow_CancelsLoadingWhenTargetDisappears()
    {
        BattleSession session = Create(Object("windArcher", 1, 20, 30, 0), Object("sunBlocker", 2, 20, 18));
        session.RunTicks(2);
        Assert.True(session.Entity(OriginalData.Sid(1))!.CrossbowFiring);
        session.Submit(new SalvageCommand(2, OriginalData.Sid(2)));
        session.RunTicks(60);
        Assert.False(session.Entity(OriginalData.Sid(1))!.CrossbowFiring);
        Assert.Equal(0, session.Entity(OriginalData.Sid(1))!.AttackTargetId);
        Assert.Equal(1, session.DrainEvents().Count(e => e.Kind == SessionEventKind.ShotFired));
    }

    /// <summary>원반과 석궁의 원본 탄속을 논리 거리·틱 수로 확인한다.</summary>
    [Theory]
    [InlineData("sunArcher", 20, 22)]
    [InlineData("windArcher", 20, 18)]
    public void Archers_TravelAtThirtyFiveCellsPerSecond(string name, int x, int y)
    {
        BattleSession session = Create(Object(name, 1, 20, 30, 0), Object("sunBlocker", 2, x, y));
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        double distance = Math.Sqrt(Math.Pow(shot.EndX - shot.StartX, 2) + Math.Pow(shot.EndY - shot.StartY, 2));
        Assert.Equal(Math.Ceiling(distance / 35 * 24), shot.ImpactTick - shot.FiredTick);
    }

    /// <summary>Wind Tower의 네 면 중 선택한 면에서 온 탄은 피해를 주지 않고 반대편은 정상 피해를 준다.</summary>
    [Theory]
    [InlineData(0, 20, 12, 0)]
    [InlineData(1, 28, 20, 2)]
    [InlineData(2, 20, 28, 1)]
    [InlineData(3, 12, 20, 3)]
    public void WindTower_ProtectsOnlyChosenGroundSide(int direction, int x, int y, byte sunFrame)
    {
        TypeInfo towerType = OriginalData.RequireTypes().Find("windBlocker")!;
        byte frame = (byte)EmplacementDirection.PlacementFrame(towerType, direction);
        BattleSession immune = Create(Object("sunCannon", 1, x, y, sunFrame), Object("windBlocker", 2, 20, 20, frame));
        immune.RunTicks(30);
        Assert.Equal(600, immune.Entity(OriginalData.Sid(2))!.HitPoints);
        Assert.Equal(direction, immune.Entity(OriginalData.Sid(2))!.CannonDirection);
        Assert.Empty(immune.Shots);
        Assert.Equal(0, immune.Entity(OriginalData.Sid(1))!.AttackTargetId);
        byte opposite = (byte)EmplacementDirection.PlacementFrame(towerType, (direction + 2) % 4);
        BattleSession vulnerable = Create(Object("sunCannon", 1, x, y, sunFrame), Object("windBlocker", 2, 20, 20, opposite));
        vulnerable.RunTicks(30);
        Assert.Equal(520, vulnerable.Entity(OriginalData.Sid(2))!.HitPoints);
    }

    /// <summary>Wind Tower는 공중 공격·주문 폭탄·번개 울타리·폭발에 방위 면역을 적용하지 않는다.</summary>
    [Theory]
    [InlineData("sunFlyer")]
    [InlineData("bombExplodeSmall")]
    [InlineData("thunderFence")]
    [InlineData(null)]
    public void WindTower_DirectionalImmunityHasOriginalExceptions(string? name)
    {
        GameEntity tower = Create(Object("windBlocker", 2, 20, 20, 0)).Entity(OriginalData.Sid(1))!;
        TypeInfo? attacker = name == null ? null : OriginalData.RequireTypes().Find(name);
        if (name != null) Assert.NotNull(attacker);
        Assert.False(CombatImmunity.Blocks(tower, attacker, tower.WorldX, tower.WorldY - 8));
    }

    /// <summary>Bulwark를 공격할 수 없는 비행체는 다른 지상 목표를 골라 실제 피해를 준다.</summary>
    [Fact]
    public void Bulwark_FlyerChoosesVulnerableTarget()
    {
        BattleSession session = Create(Object("sunaviary", 1, 10, 10), Object("thunderBlocker", 2, 17, 10), Object("windBlocker", 2, 20, 14, 0));
        session.RunTicks(400);
        GameEntity flyer = Assert.Single(session.Entities, e => e.Flight != null);
        Assert.Equal(OriginalData.Sid(3), flyer.AttackTargetId);
        Assert.Equal(3900, session.Entity(OriginalData.Sid(2))!.HitPoints);
        Assert.True(session.Entity(OriginalData.Sid(3))!.HitPoints < 600);
        Assert.False(CombatImmunity.Blocks(session.Entity(OriginalData.Sid(2))!, OriginalData.RequireTypes().Find("sunCannon"), 10, 10));
    }

    /// <summary>석궁 상태를 포함한 검사합·피해·효과음 순서가 화면 갱신 간격에 좌우되지 않는다.</summary>
    [Fact]
    public void Crossbow_ReloadIsDeterministicAcrossFrameRates()
    {
        FortMapObject[] objects = [Object("windArcher", 1, 20, 30, 0), Object("sunBlocker", 2, 25, 18)];
        BattleSession first = Create(objects), second = Create(objects);
        // 서로 다른 화면 갱신 간격으로 같은 3초를 진행한다.
        for (int frame = 0; frame < 90; frame++) first.Advance(1.0 / 30);
        // 120Hz에서도 장전·발사·피해 예약은 논리 틱을 사용한다.
        for (int frame = 0; frame < 360; frame++) second.Advance(1.0 / 120);
        Assert.Equal(first.Checksum(), second.Checksum());
        Assert.Equal(first.DrainEvents(), second.DrainEvents());
    }

    /// <summary>실제 타입의 완성된 시설만 놓는 평지 전투 세션을 만든다.</summary>
    private static BattleSession Create(params FortMapObject[] objects) => new(
        new BattleMap(objects, (_, _) => 0), new BridgeGrid((_, _) => true, (_, _) => false), OriginalData.RequireTypes());

    /// <summary>저장 그림을 포함하는 원본 시설 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y, byte frame = 0) =>
        new(x, y, 0, new FortObject(0, 0, OriginalData.RequireTypes().Find(name)!, frame, null, null, null, null, owner, []));
}
