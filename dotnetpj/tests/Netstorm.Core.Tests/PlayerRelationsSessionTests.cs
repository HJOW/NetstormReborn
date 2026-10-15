using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 방향별 동맹과 플레이어 참여가 실제 세션의 소비자(사격 표적, 승패 대상)에서 원본처럼 읽히는지 검사한다.
/// 원본 표적 검색(RawGunTarget)은 공격자 소유자의 동맹 마스크만 읽고, 승패(004c36c0)는 참여 중인 플레이어만 센다.
/// </summary>
/// <remarks>원본 기계어 대조는 <see cref="X86PlayerParticipationTests"/>가 맡는다. 2026-10-15 감사 D03 에서 추가.</remarks>
public sealed class PlayerRelationsSessionTests
{
    /// <summary>모든 칸이 섬이고 점유가 없는 작은 다리 격자.</summary>
    private static BridgeGrid Grid() => new((_, _) => true, (_, _) => false);

    /// <summary>같은 섬 영역에 실제 타입과 여러 소유자를 두는 작은 세션을 만든다(전투 켬).</summary>
    private static BattleSession Create(Func<int, int, bool>? allied, params FortMapObject[] objects) =>
        new(new BattleMap(objects, (_, _) => 0, allied: allied), Grid(), OriginalData.RequireTypes()) { CombatEnabled = true };

    /// <summary>저장 방위와 속성 덮어쓰기를 가진 실제 타입 오브젝트.</summary>
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

    /// <summary>
    /// 캐논은 자기 소유자가 대상 소유자를 동맹으로 선언했을 때만 쏘지 않는다. 대상 쪽의 선언은 읽지 않는다
    /// (예전의 대칭 판정은 한쪽만 선언해도 양쪽이 서로 쏘지 않았다).
    /// </summary>
    [Theory]
    [InlineData(false, false, true)]
    [InlineData(true, false, false)]
    [InlineData(false, true, true)]
    [InlineData(true, true, false)]
    public void Cannon_SkipsOnlyTargetsOfOwnersItsOwnerDeclared(bool attackerDeclares, bool targetDeclares, bool fires)
    {
        bool Allied(int from, int to) => from == to || attackerDeclares && from == 2 && to == 3 || targetDeclares && from == 3 && to == 2;
        BattleSession session = Create(Allied, Object("sunCannon", 2, 10, 10, 3, overrides: "hpPerSec = 400;"), Object("rainBlocker", 3, 18, 10));
        session.RunTicks(session.TicksPerSecond * 10);
        bool shot = session.DrainEvents().Any(e => e.Kind == SessionEventKind.ShotFired && e.Player == 2);
        Assert.Equal(fires, shot);
    }

    /// <summary>
    /// 맵의 동맹 질의는 방향별이고(같은 플레이어는 항상 동맹), 양방향 질의는 두 방향이 모두 참일 때만 참이다.
    /// 에너지 공급은 소비자 → 공급원 소유자 방향을 읽는다.
    /// </summary>
    [Fact]
    public void BattleMap_QueriesAreDirectionalAndMutualRequiresBothWays()
    {
        var map = new BattleMap([], (_, _) => 0, allied: (from, to) => from == 5 && to == 6);
        Assert.True(map.IsAllyOf(5, 6));
        Assert.False(map.IsAllyOf(6, 5));
        Assert.True(map.IsAllyOf(4, 4));
        Assert.False(map.AreMutualAllies(5, 6));
        Assert.True(map.AreMutualAllies(7, 7));
        Assert.True(map.IsFriendly(5, 6));
        Assert.False(map.IsFriendly(6, 5));
        var both = new BattleMap([], (_, _) => 0, allied: (from, to) => from != to && from is 5 or 6 && to is 5 or 6);
        Assert.True(both.AreMutualAllies(5, 6));
    }

    /// <summary>
    /// 세션의 기본 참여 상태는 로컬 사람과 사제가 있는 AI 소유자만 참여자로 만들고, 승패 사건은 참여자에게만 낸다.
    /// 사제가 없는 소유자(신전만 있음)는 AI 가 만들어지지 않아 참여하지 않으므로 그 신전 파괴로 사건이 나지 않는다.
    /// </summary>
    [Fact]
    public void Outcome_CountsOnlyParticipantsWithAPriestAtStart()
    {
        var header = new Dictionary<string, string> { ["ai2AllyList"] = "\"\"", ["ai3AllyList"] = "\"\"" };
        MissionStart mission = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        FortMapObject[] objects =
        [
            Object("priest", 1, 10, 10), Object("windVortex", 1, 14, 10),
            Object("priest", 2, 60, 10), Object("windVortex", 2, 64, 10),
            Object("windVortex", 3, 100, 10),
        ];
        var session = new BattleSession(new BattleMap(objects, (_, _) => 0), Grid(), OriginalData.RequireTypes(), mission);
        Assert.True(session.Participation.Participates(1));
        Assert.True(session.Participation.Participates(2));
        Assert.False(session.Participation.Participates(3));
        Assert.NotEqual(0u, session.Participation.Brain(2));
        Assert.Equal(0u, session.Participation.Brain(3));
        session.RunTicks(2);
        // 두 신전을 모두 지운다. 참여자(2)만 신전 사망 사건을 낸다.
        foreach (GameEntity temple in session.Entities.Where(e => e.Kind == ObjectKind.Temple && e.Owner is 2 or 3).ToArray())
            session.Submit(new SalvageCommand(temple.Owner, temple.Id));
        session.DenySalvage = false;
        session.RunTicks(2);
        string[] told = [.. session.DrainEvents().Where(e => e.Kind == SessionEventKind.MissionTell).Select(e => e.Text)];
        Assert.Contains("ai2TempleDead", told);
        Assert.DoesNotContain("ai3TempleDead", told);
    }
}
