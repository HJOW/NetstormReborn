using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>수송 사제 포획·제단 의식·구출 이벤트의 시간과 결정론을 검사한다.</summary>
public sealed class SacrificeTests
{
    /// <summary>기절하지 않은 적 사제는 일반 캠페인에서 집을 수 없다.</summary>
    [Fact]
    public void Capture_RejectsPriestBeforeStun()
    {
        BattleSession session = Create();
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.CommandRejected
            && item.Failure == CommandFailure.NotCapturable);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
    }

    /// <summary>포획 후 제단까지 운반하고 내 사제가 옆에 있으면 의식이 시작된다.</summary>
    [Fact]
    public void Capture_CarriesCaptiveToAltarAndStartsRitual()
    {
        BattleSession session = Create(includeCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;

        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        session.RunTicks(session.TicksPerSecond * 60);

        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        Assert.Equal(altar.Id, captive.CaptorId);
        Assert.Equal(0, carrier.CarriedPriestId);
        Assert.Single(session.Rituals, ritual => ritual.AltarId == altar.Id);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeStarted);
    }

    /// <summary>다섯 룬·희생 음성·제단 소멸·포로 제거 시각에 맞춰 승패 안내가 한 번 발생한다.</summary>
    [Fact]
    public void Ritual_UsesMeasuredTimelineAndDelaysVictoryUntilPriestRemoval()
    {
        MissionStart mission = CampaignMission();
        BattleSession session = Create(mission, includeCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        // 포획·운반 경로가 끝나 의식이 시작될 때까지 한 틱씩 진행한다.
        for (int tick = 0; tick < session.TicksPerSecond * 90 && session.Rituals.Count == 0; tick++)
        {
            session.RunTicks(1);
        }
        long started = Assert.Single(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeStarted).Tick;

        session.RunTicks(Ticks(session, BattleSession.CompletionSeconds));
        SessionEvent[] completed = [.. session.DrainEvents()];
        Assert.Equal(BattleSession.SacrificeRuneCount, completed.Count(item => item.Kind == SessionEventKind.SacrificeRune));
        Assert.Contains(completed, item => item.Kind == SessionEventKind.SacrificeCompleted);
        Assert.True(session.Tick >= started + Ticks(session, BattleSession.CompletionSeconds));

        session.RunTicks(Ticks(session, BattleSession.SacrificeKillDelaySeconds));
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.PriestSacrificed);
        session.RunTicks(Ticks(session, BattleSession.AltarConsumeDelaySeconds - BattleSession.SacrificeKillDelaySeconds));
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.AltarConsumed);
        Assert.Null(session.Entity(altar.Id));
        Assert.Same(captive, session.Entity(captive.Id));
        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        Assert.Equal(0, captive.CaptorId);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "BadTeamDead");

        session.RunTicks(Ticks(session, BattleSession.SacrificedPriestRemovalDelaySeconds));
        Assert.Null(session.Entity(captive.Id));
        Assert.Single(session.DrainEvents(), item => item.Text == "BadTeamDead");
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "BadTeamDead");
    }

    /// <summary>allowAnyCapture 미션은 기절·적대 여부와 관계없이 사제를 집고, 미션 섹션은 한 번만 알린다.</summary>
    [Fact]
    public void RescueMission_AllowsCaptureAndTellsCapturedOnce()
    {
        MissionStart mission = MissionStart.FromHeader(key => key switch
        {
            "allowAnyCapture" => "1",
            "ai2AllyList" => "1",
            _ => null,
        });
        BattleSession session = Create(mission);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);

        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        Assert.Equal(captive.Id, carrier.CarriedPriestId);
        Assert.Single(session.DrainEvents(), item => item.Text == "ai2PriestCaptured");
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "ai2PriestCaptured");
    }

    /// <summary>동맹 사제를 사람 섬에 내려놓으면 완전히 회복하고 PriestSaved 안내를 한 번 낸다.</summary>
    [Fact]
    public void RescueMission_DropOnHumanTerritoryTellsPriestSaved()
    {
        MissionStart mission = MissionStart.FromHeader(key => key switch
        {
            "allowAnyCapture" => "1",
            "ai2AllyList" => "1",
            _ => null,
        });
        BattleSession session = Create(mission, includeTemples: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);

        session.Submit(new DropPriestCommand(1, carrier.Id, 90, 90));
        session.RunTicks(session.TicksPerSecond * 90);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Equal(captive.MaxHitPoints, captive.HitPoints);
        Assert.InRange(Math.Abs(captive.Footprint.AnchorX - 90) + Math.Abs(captive.Footprint.AnchorY - 90), 0, 2);
        Assert.Single(session.DrainEvents(), item => item.Text == "ai2PriestSaved");
    }

    /// <summary>운반 유닛이 파괴되면 포획된 사제가 그 자리에 완전히 회복해 풀려난다.</summary>
    [Fact]
    public void DestroyedCarrier_ReleasesAndHealsCaptive()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(mission, includeEnemyCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.CombatEnabled = false;
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        session.CombatEnabled = true;
        session.RunTicks(session.TicksPerSecond * 3);

        Assert.Null(session.Entity(carrier.Id));
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Equal(captive.MaxHitPoints, captive.HitPoints);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.PriestReleased);
    }

    /// <summary>제단 체력이 절반 아래로 떨어지면 의식이 깨지고 묶인 사제가 회복해 달아난다.</summary>
    [Fact]
    public void DamagedAltar_BreaksRitualAndReleasesCaptive()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(mission, lowHpAltar: true, includeAltarCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.CombatEnabled = false;
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        // 포획·운반을 마친 뒤 제단 옆에 사제가 있어 의식이 시작되기를 기다린다.
        for (int tick = 0; tick < session.TicksPerSecond * 90 && session.Rituals.Count == 0; tick++)
        {
            session.RunTicks(1);
        }
        Assert.Single(session.Rituals);
        session.DrainEvents();
        session.CombatEnabled = true;
        session.RunTicks(session.TicksPerSecond * 2);

        Assert.Empty(session.Rituals);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeBroken);
        Assert.True(altar.HitPoints < altar.MaxHitPoints * BattleSession.AltarBreakHealthRatio);
    }

    /// <summary>같은 시드와 명령·틱열은 수송 경로·의식 상태·알림 집합까지 같은 검사합을 만든다.</summary>
    [Fact]
    public void RitualState_IsDeterministic()
    {
        BattleSession first = Create(includeCannon: true);
        BattleSession second = Create(includeCannon: true);
        first.RunTicks(first.TicksPerSecond * 2);
        second.RunTicks(second.TicksPerSecond * 2);
        first.CombatEnabled = false;
        second.CombatEnabled = false;
        GameEntity firstVictim = Entity(first, ObjectKind.Priest, 2);
        GameEntity secondVictim = Entity(second, ObjectKind.Priest, 2);
        GameEntity firstCarrier = Entity(first, ObjectKind.Transport, 1);
        GameEntity secondCarrier = Entity(second, ObjectKind.Transport, 1);
        int firstAltar = Entity(first, ObjectKind.Altar, 1).Id;
        int secondAltar = Entity(second, ObjectKind.Altar, 1).Id;
        first.Submit(new CapturePriestCommand(1, firstCarrier.Id, firstVictim.Id, firstAltar));
        second.Submit(new CapturePriestCommand(1, secondCarrier.Id, secondVictim.Id, secondAltar));
        first.RunTicks(first.TicksPerSecond * 90);
        second.RunTicks(second.TicksPerSecond * 90);
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>가짜 미션 헤더로 포획·승패 테스트 조건을 만든다.</summary>
    private static MissionStart CampaignMission() => MissionStart.FromHeader(key => key switch
    {
        "title" => "Sacrifice test",
        "loadFort" => "SacrificeTest",
        _ => null,
    });

    /// <summary>주어진 플레이어·분류의 고유 엔티티를 조회한다.</summary>
    private static GameEntity Entity(BattleSession session, ObjectKind kind, int owner) =>
        Assert.Single(session.Entities, entity => entity.Kind == kind && entity.Owner == owner);

    /// <summary>측정 초를 틱으로 바꿀 때 분수 틱이 생기면 다음 틱 경계로 올림한다.</summary>
    private static int Ticks(BattleSession session, double seconds) =>
        (int)Math.Ceiling(seconds * session.TicksPerSecond);

    /// <summary>원본 타입과 간단한 양측 지도로 포획·제단 규칙을 실행한다.</summary>
    private static BattleSession Create(MissionStart? mission = null, bool includeCannon = false, bool includeTemples = false,
        bool includeEnemyCannon = false, bool includeAltarCannon = false, bool lowHpAltar = false)
    {
        TypeCatalog types = OriginalData.RequireTypes();
        var objects = new List<FortMapObject>
        {
            Object(types, "sunwalker", 1, 30, 15),
            Object(types, "priest", 2, 20, 9),
            Object(types, "altar", 1, 65, 30, lowHpAltar ? 100 : null),
            Object(types, "priest", 1, 66, 30),
        };
        if (includeCannon)
        {
            objects.Add(Object(types, "suncannon", 1, 10, 10));
        }
        if (includeTemples)
        {
            objects.Add(Object(types, "windvortex", 1, 100, 100));
            objects.Add(Object(types, "rainvortex", 2, 5, 80));
        }
        if (includeEnemyCannon)
        {
            objects.Add(Object(types, "suncannon", 2, 40, 10));
        }
        if (includeAltarCannon)
        {
            objects.Add(Object(types, "suncannon", 2, 55, 31));
        }
        int? TerritoryAt(int x, int _) => x < 50 ? 2 : 1;
        bool Allied(int first, int second) => mission?.AreAllied(first, second) ?? first == second;
        var map = new BattleMap(objects, TerritoryAt, allied: Allied);
        var bridges = new BridgeGrid((_, _) => true);
        return new BattleSession(map, bridges, types, mission);
    }

    /// <summary>원본 타입을 사용해 맵 오브젝트를 만든다.</summary>
    private static FortMapObject Object(TypeCatalog types, string typeName, int owner, int x, int y, int? hitPoints = null)
    {
        TypeInfo type = types.Find(typeName) ?? throw new InvalidDataException(typeName);
        if (hitPoints is int value)
        {
            string properties = string.Join('\n', type.Definition.Properties.Select(property => $"{property.Key} = \"{property.Value}\";"));
            string flags = string.Join(' ', type.Definition.Flags);
            TypeDefinition definition = TypeDefinition.Parse($"typename {type.Name}\ntypeflags {flags};\n{{\n{properties}\nmaxHitPoints = {value};\n}}");
            type = type with { Definition = definition };
        }
        int territory = x < 50 ? 2 : 1;
        var item = new FortObject(0, 0, type, null, null, null, null, null, owner, []);
        return new FortMapObject(x, y, territory, item);
    }
}
