using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>TEST01 녹화와 원본 정적 분석에서 확인한 캐논·아이스 타워·썬 바리케이트 규칙을 검사한다.</summary>
public sealed class RecordedCombatTests
{
    /// <summary>타워는 두 번 부서져도 같은 받침·선택·소유자를 유지하며 처치 보상을 주지 않는다.</summary>
    [Fact]
    public void IceTower_RegrowsRepeatedlyWithoutRewardOrSupportLoss()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3, overrides: "hpPerSec = 400;"), Object("rainBlocker", 2, 18, 10));
        GameEntity tower = session.Entity(2)!;
        int money = session.Player(1).StormPower;
        session.Submit(new SelectEntityCommand(2, tower.Id));
        // 두 번의 파괴와 재성장을 모두 검증해 후속 탄의 중복 보상·재파괴를 막는다.
        for (int cycle = 0; cycle < 2; cycle++)
        {
            // 재성장 동안 접기 시작한 캐논은 재조준을 마쳐야 하므로 정확한 피해 발생 틱까지 기다린다.
            for (int waited = 0; waited < 160 && !tower.IsRegenerating; waited++) session.RunTicks(1);
            Assert.True(tower.IsRegenerating);
            Assert.Equal(0, tower.HitPoints);
            Assert.Same(tower, session.Entity(tower.Id));
            Assert.True(session.Map.IsOccupied(tower.Footprint));
            Assert.Equal(tower.Id, session.Player(2).SelectedEntityId);
            Assert.Equal(money, session.Player(1).StormPower);
            Assert.Equal(0, session.IceGrowthFrame(tower));
            Assert.Equal(832, session.IceRegrowthTick(tower) - tower.RegenerationStartTick);
            session.CombatEnabled = false;
            session.RunTicks((int)(session.IceRegrowthTick(tower) - session.Tick - 1));
            Assert.True(tower.IsRegenerating);
            Assert.Equal(13, session.IceGrowthFrame(tower));
            session.RunTicks(1);
            Assert.False(tower.IsRegenerating);
            Assert.Equal(1060, tower.HitPoints);
            session.CombatEnabled = true;
        }
        SessionEvent[] events = [.. session.DrainEvents()];
        Assert.Equal(2, events.Count(e => e.Kind == SessionEventKind.IceTowerShattered));
        Assert.Equal(2, events.Count(e => e.Kind == SessionEventKind.IceTowerRegrown));
        Assert.DoesNotContain(events, e => e.Kind == SessionEventKind.EntityDestroyed && e.EntityId == tower.Id);
    }

    /// <summary>성장 그림은 이름이 중복된 열한 번째 P00도 순서대로 지나간다.</summary>
    [Fact]
    public void IceTower_GrowthUsesClusterOrderAndSalvageCancelsWithoutRefund()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3, overrides: "hpPerSec = 400;"), Object("rainBlocker", 2, 18, 10));
        session.RunTicks(12);
        GameEntity tower = session.Entity(2)!;
        session.CombatEnabled = false;
        session.RunTicks(576 - (int)(session.Tick - tower.RegenerationStartTick));
        Assert.Equal(10, session.IceGrowthFrame(tower));
        Assert.Equal("P00", OriginalData.RequireTypes().Find("growingRainBlocker")!.Definition.Clusters[10].Name);
        int money = session.Player(2).StormPower;
        Assert.Equal(0, tower.SalvageRefund);
        session.Submit(new SalvageCommand(2, tower.Id));
        session.RunTicks(1000);
        Assert.Null(session.Entity(tower.Id));
        Assert.False(session.Map.IsOccupied(tower.Footprint));
        Assert.Equal(money, session.Player(2).StormPower);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.IceTowerRegrown);
    }

    /// <summary>체력 없는 성장 상태는 새 표적이 되지 않고 이미 발사된 후속 탄에도 다시 깨지지 않는다.</summary>
    [Fact]
    public void IceTower_AbsorbsOnlyUntilShattered()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3, overrides: "hpPerSec = 400;"),
            Object("sunCannon", 1, 26, 10, overrides: "hpPerSec = 400;"), Object("rainBlocker", 2, 18, 10));
        session.RunTicks(160);
        Assert.True(session.Entity(3)!.IsRegenerating);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.IceTowerShattered);
        Assert.Empty(session.Shots);
        Assert.All(session.Entities.Take(2), e => Assert.Equal(0, e.AttackTargetId));
    }

    /// <summary>적 아이스 타워가 가로막으면 뒤의 유닛 대신 타워를 공격한다.</summary>
    [Fact]
    public void IceTower_InterceptsFireAimedBehindIt()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3), Object("rainBlocker", 2, 18, 10), Object("sunwalker", 2, 26, 9));
        session.RunTicks(15);
        Assert.Equal(2, session.Entity(1)!.AttackTargetId);
        Assert.Equal(1060 - 80, session.Entity(2)!.HitPoints);
        Assert.Equal(session.Entity(3)!.MaxHitPoints, session.Entity(3)!.HitPoints);
    }

    /// <summary>가장 가까운 동맹 기둥만 연결하며 세 기둥을 하나의 긴 선으로 중복 연결하지 않는다.</summary>
    [Fact]
    public void Fence_ConnectsNearestPostsOnEachAxis()
    {
        BattleSession session = Create(Object("sunFence", 1, 10, 10), Object("sunFence", 1, 20, 10),
            Object("sunFence", 1, 30, 10), Object("sunFence", 1, 20, 20), Object("sunFence", 1, 22, 23));
        Assert.Equal(3, session.SunForceFields.Count);
        Assert.DoesNotContain(session.SunForceFields, f => f.FirstId == 1 && f.SecondId == 3);
        Assert.Contains(session.SunForceFields, f => f.FirstId == 2 && f.SecondId == 4);
    }

    /// <summary>동맹 소유자가 달라도 연결하지만 중간의 적 기둥은 긴 선을 끊는다.</summary>
    [Fact]
    public void Fence_UsesAllianceAndHostileIntermediatePosts()
    {
        FortMapObject[] posts = [Object("sunFence", 1, 10, 10), Object("sunFence", 3, 20, 10), Object("sunFence", 1, 30, 10)];
        var map = new BattleMap(posts, (_, _) => 0, allied: (a, b) => a == b || a is 1 or 3 && b is 1 or 3);
        var allied = new BattleSession(map, Grid(), OriginalData.RequireTypes());
        Assert.Equal(2, allied.SunForceFields.Count);
        BattleSession hostile = Create(posts);
        Assert.Empty(hostile.SunForceFields);
        hostile.Submit(new SalvageCommand(3, 2));
        hostile.RunTicks(1);
        Assert.Single(hostile.SunForceFields);
    }

    /// <summary>50칸 경계와 그 밖에서는 연결되지 않고, 안쪽에서는 연결된다.</summary>
    [Theory]
    [InlineData(49, 1)]
    [InlineData(50, 0)]
    [InlineData(51, 0)]
    public void Fence_RespectsOriginalRange(int distance, int expected)
    {
        BattleSession session = Create(Object("sunFence", 1, 10, 10), Object("sunFence", 1, 10 + distance, 10));
        Assert.Equal(expected, session.SunForceFields.Count);
    }

    /// <summary>적 방어선은 탄을 흡수하고 같은 소유자·동맹의 탄은 통과한다. 가로·세로 연결 모두 검사한다.</summary>
    [Theory]
    [InlineData(false, 2, true)]
    [InlineData(false, 1, false)]
    [InlineData(true, 2, true)]
    [InlineData(true, 1, false)]
    public void Fence_AbsorbsEnemyFireButPassesFriendlyFire(bool horizontal, int owner, bool blocked)
    {
        FortMapObject[] objects = horizontal
            ? [Object("sunCannon", 1, 20, 10, 0), Object("sunBlocker", 2, 20, 28), Object("sunFence", owner, 10, 20), Object("sunFence", owner, 30, 20)]
            : [Object("sunCannon", 1, 10, 20, 3), Object("sunBlocker", 2, 28, 20), Object("sunFence", owner, 20, 10), Object("sunFence", owner, 20, 30)];
        BattleSession session = Create(objects);
        double before = session.Entity(2)!.HitPoints;
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(blocked, shot.BlockedByFenceId != 0);
        session.RunTicks(30);
        Assert.Equal(before - (blocked ? 0 : 80), session.Entity(2)!.HitPoints);
        Assert.Equal(blocked, session.DrainEvents().Any(e => e.Kind == SessionEventKind.ShotBlocked));
    }

    /// <summary>발사 뒤 탄이 도달하기 전에 새 방어선이 만들어져도 적 탄을 흡수한다.</summary>
    [Fact]
    public void Fence_CanInterceptAShotAlreadyInFlight()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 20, 3), Object("sunBlocker", 2, 28, 20),
            Object("sunFence", 2, 20, 10), Object("sunFence", 2, 20, 30, overrides: "maxHitPoints = 0; cost = 0;"), Object("windVortex", 2, 30, 30));
        session.EnforceProductionRules = false;
        session.RunTicks(1);
        Assert.Equal(0, Assert.Single(session.Shots).BlockedByFenceId);
        session.Submit(new SalvageCommand(2, 4));
        session.RunTicks(1);
        session.Submit(new PlaceUnitCommand(2, "sunFence", 20, 30));
        double before = session.Entity(2)!.HitPoints;
        session.RunTicks(30);
        Assert.Single(session.SunForceFields);
        Assert.Equal(before, session.Entity(2)!.HitPoints);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotBlocked);
    }

    /// <summary>기둥을 회수하면 방어선이 사라지고 다음 적 탄은 보호받던 목표에 도달한다.</summary>
    [Fact]
    public void Fence_RemovingAPostAllowsLaterShotsThrough()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 20, 3), Object("sunBlocker", 2, 28, 20),
            Object("sunFence", 2, 20, 10), Object("sunFence", 2, 20, 30));
        session.RunTicks(30);
        double health = session.Entity(2)!.HitPoints;
        session.Submit(new SalvageCommand(2, 3));
        session.RunTicks(121);
        Assert.Empty(session.SunForceFields);
        Assert.Equal(health - 80, session.Entity(2)!.HitPoints);
    }

    /// <summary>원본 공통 발사 경로처럼 Vander Tower의 번개도 적 방어선 교점에서 끝난다.</summary>
    [Fact]
    public void Fence_AbsorbsLightningAtTheIntersection()
    {
        BattleSession session = Create(Object("thunderArcher", 1, 10, 20), Object("sunBlocker", 2, 19, 20),
            Object("sunFence", 2, 15, 5), Object("sunFence", 2, 15, 35));
        session.RunTicks(1);
        CombatShot beam = Assert.Single(session.Lightning);
        Assert.True(beam.IsBeam);
        Assert.NotEqual(0, beam.BlockedByFenceId);
        Assert.Equal(14, beam.EndX);
        session.RunTicks(1);
        Assert.Equal(session.Entity(2)!.MaxHitPoints, session.Entity(2)!.HitPoints);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotBlocked);
    }

    /// <summary>새 배치 명령의 방위도 저장 프레임과 같은 네 방향 규칙으로 정규화한다.</summary>
    [Theory]
    [InlineData("rainCannon", -1, 3)]
    [InlineData("rainCannon", 1, 1)]
    [InlineData("rainCannon", 5, 1)]
    [InlineData("thunderCannon", -1, 3)]
    [InlineData("thunderCannon", 1, 1)]
    [InlineData("thunderCannon", 5, 1)]
    [InlineData("windArcher", -1, 3)]
    [InlineData("windArcher", 1, 1)]
    [InlineData("windArcher", 5, 1)]
    [InlineData("windBlocker", -1, 3)]
    [InlineData("windBlocker", 1, 1)]
    [InlineData("windBlocker", 5, 1)]
    public void FixedCannon_PlacementUsesTheRequestedRotation(string name, int rotation, int expected)
    {
        BattleSession session = Create(Object("windVortex", 1, 30, 30));
        session.EnforceProductionRules = false;
        // 모든 원소 공급을 충분히 둬 방위 전달만 검사한다. 소유 섬·점유·비용 검사는 실제 배치 경로를 거친다.
        for (int element = 0; element < 4; element++)
        {
            for (int number = 0; number < 4; number++)
                session.Map.AddSource(new EnergySource(100 + element * 4 + number, (Element)element, 43, 29, 1));
        }
        session.Submit(new PlaceUnitCommand(1, name, 44, 30, rotation));
        session.RunTicks(1);
        GameEntity cannon = Assert.Single(session.Entities, e => e.Type.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
        Assert.Equal(expected, cannon.CannonDirection);
        if (name.Equals("windArcher", StringComparison.OrdinalIgnoreCase))
            Assert.Equal(CrossbowAnimation.PlacementFrame(expected), cannon.CrossbowFrame);
        if (CannonAnimation.IsCannon(cannon.Type))
            Assert.Equal(CannonAnimation.Side(expected), cannon.Type.Definition.Frames.Codes[CannonAnimation.Frame(cannon, session.Tick, session.TicksPerSecond)].Side);
    }

    /// <summary>저장된 네 방위의 Crossbow는 뒤쪽의 더 가까운 적 대신 앞쪽 적만 쏜다.</summary>
    [Theory]
    [InlineData(0, 30, 18, 30, 37)]
    [InlineData(25, 42, 30, 23, 30)]
    [InlineData(50, 30, 42, 30, 23)]
    [InlineData(75, 18, 30, 37, 30)]
    [InlineData(43, 42, 30, 23, 30)]
    public void Crossbow_UsesStoredSectorInsteadOfNearestTarget(byte frame, int frontX, int frontY, int backX, int backY)
    {
        BattleSession session = Create(Object("windArcher", 1, 30, 30, frame),
            Object("sunBlocker", 2, frontX, frontY), Object("sunBlocker", 2, backX, backY));
        session.RunTicks(1);
        Assert.Equal(2, session.Entity(1)!.AttackTargetId);
        // 저장된 중간 조준 자세는 실제 회전이 끝나야 탄을 놓는다. 앞쪽 목표 유지·발사 단언은 그대로 검사한다.
        session.RunTicks(19);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired && e.EntityId == 1);
        session.Submit(new SalvageCommand(2, 2));
        session.RunTicks(1);
        Assert.Equal(0, session.Entity(1)!.AttackTargetId);
    }

    /// <summary>썬 캐논은 고정 캐논용 배치 방위 값이 있어도 설치 방향에 구속되지 않는다(사용자 확인 2026-10-03).</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void SunCannon_PlacementDoesNotApplyFixedRotation(int rotation)
    {
        BattleSession session = Create(Object("windVortex", 1, 30, 30));
        session.EnforceProductionRules = false;
        // 실제 배치 조건을 만족시키되 목표가 없는 상태에서 전달 방위의 영향을 비교한다.
        for (int number = 0; number < 4; number++)
            session.Map.AddSource(new EnergySource(100 + number, Element.Sun, 43, 29, 1));
        session.Submit(new PlaceUnitCommand(1, "sunCannon", 44, 30, rotation));
        session.RunTicks(1);
        GameEntity cannon = Assert.Single(session.Entities, e => e.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase));
        Assert.False(CannonAnimation.IsFixed(cannon.Type));
        Assert.Equal(0, cannon.CannonDirection);
    }

    /// <summary>썬 캐논은 첫 목표가 기절해 다른 방향의 목표로 바뀌면 재발사 대기 중에도 스스로 방향을 바꾼다.</summary>
    [Fact]
    public void SunCannon_TurnsWhenItsTargetChangesDirection()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 1), Object("priest", 2, 9, 2), Object("priest", 2, 18, 9));
        session.RunTicks(1);
        GameEntity cannon = session.Entity(1)!;
        Assert.Equal(2, cannon.AttackTargetId);
        Assert.Equal(0, cannon.CannonDirection);
        CombatShot north = Assert.Single(session.Shots);
        Assert.Equal(north.StartX, north.EndX);
        session.RunTicks(11);
        Assert.True(session.Entity(2)!.IsStunned);
        Assert.Equal(3, cannon.AttackTargetId);
        Assert.Equal(1, cannon.CannonDirection);
        Assert.Empty(session.Shots);
        session.RunTicks(109);
        CombatShot east = Assert.Single(session.Shots);
        Assert.Equal(3, east.TargetId);
        Assert.Equal(east.StartY, east.EndY);
    }

    /// <summary>고정 캐논은 앞의 목표를 잃어도 옆의 목표로 회전하지 않고 설치 방향을 유지한다.</summary>
    [Theory]
    [InlineData("rainCannon")]
    [InlineData("thunderCannon")]
    public void FixedCannon_KeepsDirectionAfterLosingItsTarget(string name)
    {
        BattleSession session = Create(Object(name, 1, 10, 10, 0), Object("priest", 2, 9, 2), Object("priest", 2, 18, 9));
        session.RunTicks(1);
        GameEntity cannon = session.Entity(1)!;
        Assert.Equal(2, cannon.AttackTargetId);
        session.RunTicks(240);
        Assert.True(session.Entity(2)!.IsStunned);
        Assert.Equal(0, cannon.AttackTargetId);
        Assert.Equal(0, cannon.CannonDirection);
        Assert.Equal(100, session.Entity(3)!.HitPoints);
        Assert.DoesNotContain(session.Shots, shot => shot.TargetId == 3);
    }

    /// <summary>받침의 각도 저장 프레임은 실제 북·동·남·서 사격 방향으로 복원한다.</summary>
    [Theory]
    [InlineData("rainCannon", 0, 10, 2)]
    [InlineData("rainCannon", 1, 18, 10)]
    [InlineData("rainCannon", 2, 10, 18)]
    [InlineData("rainCannon", 3, 2, 10)]
    [InlineData("thunderCannon", 0, 10, 2)]
    [InlineData("thunderCannon", 1, 18, 10)]
    [InlineData("thunderCannon", 2, 10, 18)]
    [InlineData("thunderCannon", 3, 2, 10)]
    public void FixedCannon_RestoresSavedDirection(string name, int direction, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        byte frame = (byte)type.Definition.Frames.Find(CannonAnimation.Side(direction), TypeFrameTable.DefaultVariant, 0);
        BattleSession session = Create(Object(name, 1, 10, 10, frame), Object("sunBlocker", 2, x, y));
        Assert.Equal(direction, session.Entity(1)!.CannonDirection);
        session.RunTicks(1);
        Assert.Equal(2, session.Entity(1)!.AttackTargetId);
        session.RunTicks(100);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired);
    }

    /// <summary>고정 캐논의 뒤·옆·대각선 목표는 자동 회전으로 공격하지 않는다.</summary>
    [Theory]
    [InlineData("rainCannon", 18, 10)]
    [InlineData("rainCannon", 10, 18)]
    [InlineData("rainCannon", 18, 18)]
    [InlineData("thunderCannon", 18, 10)]
    [InlineData("thunderCannon", 10, 18)]
    public void FixedCannon_DoesNotTurnAwayFromItsPlacementDirection(string name, int x, int y)
    {
        BattleSession session = Create(Object(name, 1, 10, 10, 0), Object("sunBlocker", 2, x, y));
        session.RunTicks(240);
        Assert.Empty(session.Shots);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired);
    }

    /// <summary>썬더 캐논은 3.5초 충전 뒤 원본 마지막 발사 프레임으로 한 발을 쏜다.</summary>
    [Fact]
    public void ThunderCannon_ChargesBeforeFiringAndUsesOriginalSpeed()
    {
        BattleSession session = Create(Object("thunderCannon", 1, 10, 20, 0), Object("sunBlocker", 2, 10, 10));
        GameEntity cannon = session.Entity(1)!;
        session.RunTicks(1);
        Assert.Equal(1, CannonAnimation.Frame(cannon, session.Tick, session.TicksPerSecond));
        session.RunTicks(83);
        Assert.Empty(session.Shots);
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(8, CannonAnimation.Frame(cannon, session.Tick, session.TicksPerSecond));
        Assert.Equal(12, shot.ImpactTick - shot.FiredTick);
        session.RunTicks(3);
        Assert.Equal(1, CannonAnimation.Frame(cannon, session.Tick, session.TicksPerSecond));
    }

    /// <summary>아이스 캐논은 4초 동안 쏜 뒤 6초 동안 쉬고 다시 연속 사격한다.</summary>
    [Fact]
    public void IceCannon_UsesBurstAndRestWindows()
    {
        BattleSession session = Create(Object("rainCannon", 1, 10, 30, 0), Object("sunBlocker", 2, 10, 10));
        // 연속 사격 내내 피해 예약과 실제 발사 그림이 일치하는지 검사한다.
        for (int tick = 0; tick < 96; tick++)
        {
            session.RunTicks(1);
            GameEntity cannon = session.Entity(1)!;
            if (cannon.LastShotTick == session.Tick)
                Assert.Equal(2, CannonAnimation.Frame(cannon, session.Tick, session.TicksPerSecond));
        }
        Assert.True(session.DrainEvents().Count(e => e.Kind == SessionEventKind.ShotFired) > 10);
        session.RunTicks(144);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired);
        session.RunTicks(12);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.ShotFired);
    }

    /// <summary>태양 캐논은 넓은 목표에도 행·열을 유지하며 이동으로 벗어난 목표에 강제로 명중하지 않는다.</summary>
    [Fact]
    public void Cannon_UsesStraightAimAndAllowsMovementToAvoidImpact()
    {
        BattleSession session = Create(Object("sunCannon", 1, 10, 10, 3), Object("priest", 2, 28, 9));
        session.RunTicks(1);
        CombatShot shot = Assert.Single(session.Shots);
        Assert.Equal(shot.StartY, shot.EndY);
        session.Submit(new MoveEntityCommand(2, 2, 28, 16));
        session.RunTicks(30);
        Assert.False(session.Entity(2)!.IsStunned);
        Assert.Equal(100, session.Entity(2)!.HitPoints);
    }

    /// <summary>착탄 경계에 걸친 이동체도 화면 보간 시간 때문에 명중 결과가 달라지지 않는다.</summary>
    [Fact]
    public void Cannon_ImpactDoesNotDependOnRenderInterpolation()
    {
        FortMapObject[] objects = [Object("sunCannon", 1, 10, 10, 3), Object("priest", 2, 16, 9)];
        BattleSession first = Create(objects), second = Create(objects);
        first.Advance(1.0 / first.TicksPerSecond);
        second.Advance(1.99 / second.TicksPerSecond);
        Assert.Equal(first.Tick, second.Tick);
        first.RunTicks(3); second.RunTicks(3);
        first.Submit(new MoveEntityCommand(2, 2, 16, 16));
        second.Submit(new MoveEntityCommand(2, 2, 16, 16));
        first.RunTicks(6); second.RunTicks(6);
        Assert.True(first.Entity(2)!.IsStunned);
        Assert.True(second.Entity(2)!.IsStunned);
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>재성장·사격 준비·방어선 흡수를 포함하는 같은 틱열은 같은 검사합과 사건을 낸다.</summary>
    [Fact]
    public void RecordedCombat_IsDeterministic()
    {
        FortMapObject[] objects = [Object("sunCannon", 1, 10, 10, 3, overrides: "hpPerSec = 400;"), Object("rainBlocker", 2, 18, 10),
            Object("thunderCannon", 1, 40, 40, 0), Object("sunBlocker", 2, 40, 20), Object("sunFence", 2, 30, 30), Object("sunFence", 2, 50, 30)];
        BattleSession first = Create(objects), second = Create(objects);
        first.RunTicks(1100); second.RunTicks(1100);
        Assert.Equal(first.Checksum(), second.Checksum());
        Assert.Equal(first.DrainEvents(), second.DrainEvents());
    }

    /// <summary>전투 규칙을 따로 검사하는 전체 평지 격자.</summary>
    private static BridgeGrid Grid() => new((_, _) => true, (_, _) => false);

    /// <summary>같은 섬 영역에 실제 타입과 여러 소유자를 두는 작은 세션을 만든다.</summary>
    private static BattleSession Create(params FortMapObject[] objects) =>
        new(new BattleMap(objects, (_, _) => 0), Grid(), OriginalData.RequireTypes());

    /// <summary>저장 방위와 경계 검사용 속성 덮어쓰기를 가진 오브젝트. 원본의 모든 프레임을 유지한다.</summary>
    private static FortMapObject Object(string name, int owner, int x, int y, byte? frame = null, string? overrides = null)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        if (overrides != null)
        {
            string properties = string.Join('\n', type.Definition.Properties.Select(p => $"{p.Key} = \"{p.Value}\";"));
            string clusters = string.Join('\n', type.Definition.Clusters.Select(c =>
                $"{c.Name} : {string.Join(' ', c.Flags)} : {string.Join(" : ", c.Layers.Select(l => $"\"{l.Image}\" #{l.Frame}"))};"));
            type = type with { Definition = TypeDefinition.Parse($"typename {type.Name}\ntypeflags {string.Join(' ', type.Definition.Flags)};\n{{\n{properties}\n{overrides}\n}}\n{clusters}") };
        }
        return new FortMapObject(x, y, 0, new FortObject(0, 0, type, frame, null, null, null, null, owner, []));
    }
}
