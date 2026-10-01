using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>실제 타입 수치와 작은 전투 지도로 사격 시간·목표·파괴 효과·사제 보호·결정론을 검증한다.</summary>
public sealed class CombatTests
{
    /// <summary>영상의 Vander Tower 번개는 거리에 따른 탄 이동 없이 다음 틱에 35 피해를 주고 짧게 표시된다.</summary>
    [Fact]
    public void VanderTower_UsesLightningInsteadOfTravellingProjectile()
    {
        BattleSession session = Create(Object("thunderArcher", 1, 10, 10), Object("sunBlocker", 2, 20, 14));
        double health = session.Entity(2)!.HitPoints;
        session.RunTicks(1);
        CombatShot beam = Assert.Single(session.Shots);
        Assert.True(beam.IsBeam);
        Assert.Equal(beam.FiredTick + 1, beam.ImpactTick);
        session.RunTicks(1);
        Assert.Equal(health - 35, session.Entity(2)!.HitPoints);
        Assert.Single(session.Lightning);
        session.RunTicks(5);
        Assert.Empty(session.Lightning);
        session.RunTicks(18);
        Assert.Single(session.Shots);
    }

    /// <summary>원본 Sun Cannon은 5초마다 80 피해를 예약하며 도착 전에는 피해를 주지 않는다.</summary>
    [Fact]
    public void Cannon_UsesOriginalDamageAndCooldown()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10));
        GameEntity target = session.Entities.Last();
        double initial = target.HitPoints;
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(80, shot.Damage);
        Assert.Equal(initial, target.HitPoints);
        session.RunTicks((int)(shot.ImpactTick - session.Tick));
        Assert.Equal(initial - 80, target.HitPoints);
        session.RunTicks(120 - (int)session.Tick);
        Assert.Empty(session.Shots);
        session.RunTicks(1);
        Assert.Single(session.Shots);
    }

    /// <summary>같은 거리는 오브젝트 번호순이며 살아 있는 목표를 유지한다.</summary>
    [Fact]
    public void Target_UsesStableTieBreakAndPersists()
    {
        BattleSession session = Create(Object("sunCannon", 1, 20, 20),
            Object("sunBlocker", 2, 28, 20), Object("sunBlocker", 2, 12, 20));
        session.RunTicks(1);
        Assert.Equal(2, session.Entity(1)!.AttackTargetId);
        session.RunTicks(120);
        Assert.Equal(2, session.Entity(1)!.AttackTargetId);
        Assert.All(session.Shots, shot => Assert.Equal(2, shot.TargetId));
    }

    /// <summary>중립·같은 소유자·사거리 밖·대각선 목표는 Sun Cannon이 쏘지 않는다.</summary>
    [Theory]
    [InlineData(0, 18, 10)]
    [InlineData(1, 18, 10)]
    [InlineData(2, 31, 10)]
    [InlineData(2, 18, 18)]
    public void Cannon_RejectsInvalidTargets(int owner, int x, int y)
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunBlocker", owner, x, y));
        session.RunTicks(1);
        Assert.Empty(session.Shots);
    }

    /// <summary>에너지 공급에 쓰는 사용자 지정 동맹 판정이 자동 전투에도 적용된다.</summary>
    [Fact]
    public void AlliedPlayers_AreNotTargets()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        var map = new BattleMap([Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10)],
            (_, _) => 0, allied: (_, _) => true);
        var session = new BattleSession(map, Grid(), types);
        session.RunTicks(1);
        Assert.Empty(session.Shots);
    }

    /// <summary>아군 신전이 사선 사이에 있으면 그 뒤의 적을 쏘지 않는다.</summary>
    [Fact]
    public void ShotBlockingBuilding_BlocksFire()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10),
            Object("windVortex", 1, 21, 12), Object("sunBlocker", 2, 27, 10));
        session.RunTicks(1);
        Assert.Empty(session.Shots);
    }

    /// <summary>두 탄이 같은 목표를 파괴해도 보상·공급원·점유 정리는 한 번뿐이다.</summary>
    [Fact]
    public void Destruction_RemovesGeneratorAndRewardsOnce()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunCannon", 1, 26, 10),
            Object("rainBattery", 2, 18, 10, "maxHitPoints = 50;"));
        GameEntity target = session.Entity(3)!;
        int money = session.Player(1).StormPower;
        session.Submit(new SelectEntityCommand(1, target.Id));
        session.RunTicks(12);
        Assert.Null(session.Entity(target.Id));
        Assert.DoesNotContain(session.Map.Sources, source => source.Id == target.Id);
        Assert.False(session.Map.IsOccupied(target.Footprint));
        Assert.Equal(0, session.Player(1).SelectedEntityId);
        Assert.Equal(money + StormPower.KillReward(target.Cost, session.Map.Options.KillRewardPercent), session.Player(1).StormPower);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.EntityDestroyed);
    }

    /// <summary>신전 파괴는 섬과 생산을 해제하며 적 사제나 승리를 제거·생성하지 않는다.</summary>
    [Fact]
    public void TempleDestruction_ClearsOwnershipWithoutEndingMission()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10),
            Object("windVortex", 2, 24, 12, "maxHitPoints = 50;"), Object("priest", 2, 30, 20));
        session.RunTicks(30);
        Assert.Null(session.Entity(2));
        Assert.Null(session.Map.Ownership.OwnerOf(1));
        Assert.False(session.Player(2).HasTemple);
        Assert.NotNull(session.Entity(3));
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.TutorialTell);
    }

    /// <summary>
    /// 템플이 파괴되면 주위 1칸에 400 피해가 들어가 옆의 골렘은 한 번에 파괴되고 사제는 기절한다. 휘말린 포대(Sun Disc Thrower)도
    /// 파괴되며, 연쇄 파괴의 보상은 템플을 부순 플레이어가 받는다 (사용자 설명 2026-10-01, exe FUN_0044b9e0).
    /// </summary>
    [Fact]
    public void TempleDestruction_ExplodesOntoAdjacentUnitsAndChainsRewards()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10),
            Object("windVortex", 2, 24, 12, "maxHitPoints = 50;"), Object("sunwalker", 2, 25, 12), Object("priest", 2, 25, 11),
            Object("sunArcher", 2, 26, 14), Object("priest", 2, 30, 20));
        GameEntity temple = session.Entity(2)!;
        GameEntity golem = session.Entity(3)!;
        GameEntity nearPriest = session.Entity(4)!;
        GameEntity archer = session.Entity(5)!;
        GameEntity farPriest = session.Entity(6)!;
        int money = session.Player(1).StormPower;
        int percent = session.Map.Options.KillRewardPercent;
        // 템플이 파괴될 때까지 진행한다
        for (int tick = 0; tick < 600 && session.Entity(temple.Id) != null; tick++)
        {
            session.RunTicks(1);
        }
        Assert.Null(session.Entity(temple.Id));
        Assert.Null(session.Entity(golem.Id));
        Assert.Null(session.Entity(archer.Id));
        Assert.True(nearPriest.IsStunned);
        Assert.Equal(nearPriest.MaxHitPoints / 2, nearPriest.HitPoints);
        Assert.False(farPriest.IsStunned);
        Assert.Equal(farPriest.MaxHitPoints, farPriest.HitPoints);
        int expected = StormPower.KillReward(temple.Cost, percent) + StormPower.KillReward(golem.Cost, percent)
            + StormPower.KillReward(archer.Cost, percent);
        Assert.Equal(money + expected, session.Player(1).StormPower);
        SessionEvent[] events = [.. session.DrainEvents()];
        // 템플과 휘말린 포대(archer 그룹) 두 번 폭발한다. 골렘은 폭발하지 않는다.
        Assert.Equal(2, events.Count(e => e.Kind == SessionEventKind.EntityExploded));
        Assert.Equal(3, events.Count(e => e.Kind == SessionEventKind.EntityDestroyed && e.Player == 1));
    }

    /// <summary>포대의 폭발 피해는 최대 체력/2+1 이고, 판매(회수)는 폭발하지 않는다.</summary>
    [Fact]
    public void CannonExplosion_UsesHalfHitPointsAndSalvageDoesNotExplode()
    {
        BattleSession destroyed = Create(Object("sunCannon", 1, 10, 10),
            Object("sunArcher", 2, 24, 10, "maxHitPoints = 40;"), Object("sunBlocker", 2, 25, 10));
        GameEntity blocker = destroyed.Entity(3)!;
        double before = blocker.HitPoints;
        // 궁수가 파괴될 때까지 진행한다 (Sun Cannon 한 발 80 피해)
        for (int tick = 0; tick < 600 && destroyed.Entity(2) != null; tick++)
        {
            destroyed.RunTicks(1);
        }
        Assert.Null(destroyed.Entity(2));
        Assert.Equal(before - (40 / 2 + 1), blocker.HitPoints);

        BattleSession salvaged = Create(Object("sunArcher", 1, 24, 10), Object("sunwalker", 1, 25, 10));
        salvaged.CombatEnabled = false;
        salvaged.Submit(new SalvageCommand(1, 1));
        salvaged.RunTicks(1);
        Assert.Null(salvaged.Entity(1));
        Assert.NotNull(salvaged.Entity(2));
        Assert.DoesNotContain(salvaged.DrainEvents(), e => e.Kind == SessionEventKind.EntityExploded);
    }

    /// <summary>사제는 큰 피해에도 절반 체력에서 기절하고 후속 탄으로 죽지 않는다.</summary>
    [Fact]
    public void Priest_StunsAndSurvivesFurtherFireWithoutTemple()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("priest", 2, 18, 9));
        session.RunTicks(300);
        GameEntity priest = session.Entity(2)!;
        Assert.True(priest.IsStunned);
        Assert.Equal(50, priest.HitPoints);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.PriestStunned);
    }

    /// <summary>자기 신전이 있으면 기절한 사제가 회복한다. 공격자는 회수해 재피격을 막는다.</summary>
    [Fact]
    public void Priest_RecoversWithTemple()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("priest", 2, 18, 9),
            Object("windVortex", 2, 50, 50));
        session.RunTicks(12);
        Assert.True(session.Entity(2)!.IsStunned);
        session.Submit(new SalvageCommand(1, 1));
        session.RunTicks(250);
        Assert.False(session.Entity(2)!.IsStunned);
        Assert.Equal(100, session.Entity(2)!.HitPoints);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.PriestRecovered);
    }

    /// <summary>전투를 끄면 피해 예약도 정지하며 같은 틱·명령열은 같은 검사합이다.</summary>
    [Fact]
    public void Combat_IsDeterministicAndCanBeDisabled()
    {
        FortMapObject[] objects = [Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10)];
        BattleSession first = Create(objects);
        BattleSession second = Create(objects);
        first.RunTicks(200);
        second.RunTicks(200);
        Assert.Equal(first.Checksum(), second.Checksum());
        first.CombatEnabled = false;
        double hp = first.Entity(2)!.HitPoints;
        first.RunTicks(500);
        Assert.Equal(hp, first.Entity(2)!.HitPoints);
        Assert.NotEqual(first.Checksum(), second.Checksum());
    }

    /// <summary>포대 규칙 테스트는 평지에서 실행해 별도 낙하 규칙의 영향을 제외한다.</summary>
    private static BridgeGrid Grid() => new((_, _) => true, (_, _) => false);

    /// <summary>실제 타입 목록과 소규모 두 영역 지도로 세션을 만든다.</summary>
    private static BattleSession Create(params FortMapObject[] objects) =>
        new(new BattleMap(objects, (x, _) => x < 16 ? 0 : 1), Grid(), OriginalData.RequireTypes());

    /// <summary>원본 타입을 사용하는 저장 오브젝트를 만든다. 짧은 피해 경계 테스트만 체력을 덮어쓴다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y, string? overrides = null)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        if (overrides != null)
        {
            string properties = string.Join('\n', type.Definition.Properties.Select(p => $"{p.Key} = \"{p.Value}\";"));
            TypeDefinition definition = TypeDefinition.Parse($"typename {type.Name}\ntypeflags {string.Join(' ', type.Definition.Flags)};\n{{\n{properties}\n{overrides}\n}}");
            type = type with { Definition = definition };
        }
        var item = new FortObject(0, 0, type, null, null, null, null, null, owner, []);
        return new FortMapObject(x, y, x < 16 ? 0 : 1, item);
    }
}
