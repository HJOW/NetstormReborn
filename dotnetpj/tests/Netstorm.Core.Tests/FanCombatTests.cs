using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>팬게임을 참고하고 원본 전이표·녹화로 교차 확인한 조준 동작과 착탄 결과를 검사한다.</summary>
public sealed class FanCombatTests
{
    /// <summary>기본 접힌 자세에서 동쪽으로 펼치는 동안 발사하지 않고, 준비가 끝난 틱에 발사 그림과 탄이 함께 생긴다.</summary>
    [Fact]
    public void SunCannon_UnfoldsBeforeFirstShot()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10));
        GameEntity cannon = session.Entity(OriginalData.Sid(1))!;
        Assert.Equal(16, cannon.SunCannonFrame);
        session.RunTicks(1);
        Assert.Equal(17, cannon.SunCannonFrame);
        Assert.Empty(session.Shots);
        session.RunTicks(14);
        Assert.Equal(19, cannon.SunCannonFrame);
        session.RunTicks(5);
        Assert.Equal(3, cannon.SunCannonFrame);
        Assert.Empty(session.Shots);
        session.RunTicks(1);
        Assert.Equal(27, cannon.SunCannonFrame);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(21, shot.FiredTick);
        Assert.Equal(80, shot.Damage);
        session.RunTicks(2);
        Assert.Equal(3, cannon.SunCannonFrame);
        Assert.Equal(4, session.DrainEvents().Count(e => e.Kind == SessionEventKind.CombatSound));
    }

    /// <summary>북·동·남·서의 준비 자세에서 다른 방향으로 돌아가는 동안에는 발사를 기다린다.</summary>
    [Theory]
    [InlineData(1, 18, 10, 1, 27)]
    [InlineData(3, 10, 2, 0, 25)]
    [InlineData(0, 2, 10, 3, 26)]
    [InlineData(2, 10, 18, 2, 24)]
    public void SunCannon_TurnsFromEveryReadyDirection(byte initialFrame, int x, int y, int direction, int firingFrame)
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, initialFrame), Object("sunBlocker", 2, x, y));
        session.RunTicks(1);
        Assert.Empty(session.Shots);
        // 원본 전이표의 회전 경로가 끝날 때까지 기다리고 중간 자세에서의 발사를 거부한다.
        for (int tick = 0; tick < 100 && session.Shots.Count == 0; tick++) session.RunTicks(1);
        Assert.Single(session.Shots);
        Assert.Equal(direction, session.Entity(OriginalData.Sid(1))!.CannonDirection);
        Assert.Equal(firingFrame, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
    }

    /// <summary>조준 중 목표를 잃으면 원본 접힌 자세로 돌아가고 사라진 목표에 발사하지 않는다.</summary>
    [Fact]
    public void SunCannon_LostTargetRetractsWithoutFiring()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10));
        session.RunTicks(1);
        session.Submit(new SalvageCommand(2, OriginalData.Sid(2)));
        session.RunTicks(60);
        Assert.Equal(0, session.Entity(OriginalData.Sid(1))!.AttackTargetId);
        Assert.Equal(16, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired);
    }

    /// <summary>조준 도중 새 방향의 목표가 생겨도 현재 중간 자세에서 이어서 회전한다.</summary>
    [Fact]
    public void SunCannon_RetargetsFromCurrentPose()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10), Object("sunBlocker", 2, 10, 2));
        session.RunTicks(1);
        Assert.Equal(17, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
        session.Submit(new SalvageCommand(2, OriginalData.Sid(2)));
        session.RunTicks(4);
        Assert.Equal(OriginalData.Sid(3), session.Entity(OriginalData.Sid(1))!.AttackTargetId);
        Assert.Equal(17, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
        session.RunTicks(1);
        Assert.Equal(16, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
        session.RunTicks(40);
        Assert.Equal(OriginalData.Sid(3), Assert.Single(session.Shots).TargetId);
        Assert.Equal(25, session.Entity(OriginalData.Sid(1))!.SunCannonFrame);
    }

    /// <summary>원반·석궁 탄도 발사 때 정한 착탄점을 벗어난 이동 목표에 예약 피해를 주지 않는다.</summary>
    [Theory]
    [InlineData("sunArcher", 18, 29)]
    [InlineData("windArcher", 9, 16)]
    public void Archer_MovingTargetCanAvoidImpact(string name, int x, int y)
    {
        BattleSession session = Create(Object(name, 1, 10, 30, 0), Object("priest", 2, x, y));
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        // 원반의 짧은 사거리에서도 폭이 1칸인 사제가 직각으로 벗어날 충분한 시간을 확보한다.
        session.Submit(new MoveEntityCommand(2, OriginalData.Sid(2), x, y + 5));
        session.RunTicks((int)(shot.ImpactTick - session.Tick));
        Assert.Equal(100, session.Entity(OriginalData.Sid(2))!.HitPoints);
        Assert.Empty(session.Impacts);
    }

    /// <summary>파괴된 시설의 폭발 위치가 남고 논리 시간이 멈추면 그림 단계도 바뀌지 않는다.</summary>
    [Fact]
    public void Impact_PreservesDestroyedTargetAndUsesSessionClock()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3), Object("windVortex", 2, 24, 12, health: 50));
        session.RunTicks(30);
        Assert.Null(session.Entity(OriginalData.Sid(2)));
        CombatImpact impact = Assert.Single(session.Impacts, i => i.Group == 'A');
        TypeDefinition animation = OriginalData.RequireTypes().Find("anim")!.Definition;
        int frame = ProjectileAnimation.ImpactFrame(animation, impact, session.Tick, session.TicksPerSecond);
        session.Advance(0);
        Assert.Equal(frame, ProjectileAnimation.ImpactFrame(animation, impact, session.Tick, session.TicksPerSecond));
        session.RunTicks(40);
        Assert.Empty(session.Impacts);
    }

    /// <summary>폭발 묶음의 빠진 D09 이름 때문에 그림이 사라지거나 한 단계 건너뛰지 않는다.</summary>
    [Fact]
    public void Impact_UsesActualClusterOrder()
    {
        TypeDefinition animation = OriginalData.RequireTypes().Find("anim")!.Definition;
        var impact = new CombatImpact(10, 10, 0, 'D');
        int frame = ProjectileAnimation.ImpactFrame(animation, impact, 0.181 * 24, 24);
        Assert.Equal("D10", animation.Clusters[frame].Name);
    }

    /// <summary>석궁 탄은 원본 20방향 그림의 북·동·남·서를 골라 그린다.</summary>
    [Theory]
    [InlineData(0, -10, 0)]
    [InlineData(10, 0, 5)]
    [InlineData(0, 10, 10)]
    [InlineData(-10, 0, 15)]
    public void Bolt_UsesOriginalOrientation(int dx, int dy, int expected)
    {
        var shot = new CombatShot(1, 1, 2, 1, 10, 10, 10 + dx, 10 + dy, 0, 10, AttackerType: "windArcher");
        Assert.Equal(expected, ProjectileAnimation.Frame(OriginalData.RequireTypes().Find("bolt")!.Definition, shot, 0, 24));
    }

    /// <summary>동일한 조준·피해 시나리오는 화면 갱신 간격과 무관하게 동일한 사건·검사합을 낸다.</summary>
    [Fact]
    public void TurningAndImpacts_AreDeterministic()
    {
        FortMapObject[] objects = [Object("sunCannon", 1, 10, 10), Object("sunBlocker", 2, 18, 10)];
        BattleSession first = Create(objects), second = Create(objects);
        // 30Hz 화면 갱신과 120Hz 화면 갱신으로 같은 2초의 논리 시간을 진행한다.
        for (int frame = 0; frame < 60; frame++) first.Advance(1.0 / 30);
        // 높은 화면 갱신율에서도 조준·발사·피해 순서는 논리 틱을 따른다.
        for (int frame = 0; frame < 240; frame++) second.Advance(1.0 / 120);
        Assert.Equal(first.Checksum(), second.Checksum());
        Assert.Equal(first.DrainEvents(), second.DrainEvents());
        Assert.Equal(first.Impacts, second.Impacts);
    }

    /// <summary>지형·건설과 독립적으로 조준·피해 규칙을 검사하는 평지 세션.</summary>
    private static BattleSession Create(params FortMapObject[] objects) =>
        new(new BattleMap(objects, (_, _) => 0), new BridgeGrid((_, _) => true, (_, _) => false), OriginalData.RequireTypes());

    /// <summary>실제 저장 자세와 선택적인 체력 경계를 가진 원본 타입 오브젝트를 만든다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y, byte? frame = null, int? health = null)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        if (health.HasValue)
        {
            string properties = string.Join('\n', type.Definition.Properties.Select(p => $"{p.Key} = \"{p.Value}\";"));
            string clusters = string.Join('\n', type.Definition.Clusters.Select(c =>
                $"{c.Name} : {string.Join(' ', c.Flags)} : {string.Join(" : ", c.Layers.Select(l => $"\"{l.Image}\" #{l.Frame}"))};"));
            type = type with { Definition = TypeDefinition.Parse($"typename {type.Name}\ntypeflags {string.Join(' ', type.Definition.Flags)};\n{{\n{properties}\nmaxHitPoints={health.Value};\n}}\n{clusters}") };
        }
        return new FortMapObject(x, y, 0, new FortObject(0, 0, type, frame, null, null, null, null, owner, []));
    }
}
