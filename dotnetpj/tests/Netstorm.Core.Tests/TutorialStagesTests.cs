using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 튜토리얼 2(Secret Workshop) 단계 처리 테스트 (원본 Totalmade.cpp 004c3bb0, docs/exe/mission-header-flags.md 3.5절).
/// 명령만으로 단계 A~I 를 끝까지 걷고, 각 단계가 표·회수 금지·안내 이벤트를 원본대로 바꾸는지 확인한다.
/// </summary>
public sealed class TutorialStagesTests
{
    /// <summary>이벤트를 비우면서 그 안의 튜토리얼 안내 섹션 이름을 log 에 순서대로 쌓는다. 비운 이벤트 전체를 돌려준다.</summary>
    private static IReadOnlyList<SessionEvent> Drain(BattleSession session, List<string> log)
    {
        // 화면이 받아가는 것처럼 이벤트를 비우며 안내만 골라 쌓는다
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        log.AddRange(events.Where(e => e.Kind == SessionEventKind.TutorialTell).Select(e => e.Text));
        return events;
    }

    /// <summary>이벤트를 비우며 안내를 log 에 쌓고, 지금까지 쌓인 안내 전체를 돌려준다</summary>
    private static List<string> Tells(BattleSession session, List<string> log)
    {
        Drain(session, log);
        return log;
    }

    /// <summary>조건에 맞는 첫 칸에 유닛을 놓고 자원 도착·실체화 완료까지 진행한다 (재충전도 기다린다).</summary>
    private static void PlaceArcher(BattleSession session)
    {
        // 직전 배치의 재충전(Unit Rate Fast = 1초)이 끝날 때까지 기다린다
        session.RunTicks((int)Math.Ceiling(session.SecondsUntilReady(1, "sunArcher") * session.TicksPerSecond));
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckUnit(1, "sunArcher", cx, cy).Allowed);
        session.Submit(new PlaceUnitCommand(1, "sunArcher", x, y));
        session.RunTicks(1);
        SessionData.RunUntilComplete(session, session.Entities.Single(e => e.Type.Name.Equals("sunArcher", StringComparison.OrdinalIgnoreCase)
            && e.Footprint.AnchorX == x && e.Footprint.AnchorY == y));
    }

    /// <summary>템플을 짓고 완공 틱까지 진행한다 (사제가 걸어가 도착한 뒤 건설하며, 완공 틱에 단계가 넘어가므로 그 틱에서 멈춘다)</summary>
    private static GameEntity BuildTemple(BattleSession session)
    {
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "windVortex", x, y));
        session.RunTicks(1);
        GameEntity temple = session.Entities.Single(e => e.Kind == ObjectKind.Temple);
        SessionData.RunUntilComplete(session, temple);
        return temple;
    }

    /// <summary>워크샵을 짓고 완공 틱까지 진행한다</summary>
    private static GameEntity BuildWorkshop(BattleSession session)
    {
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "sunFactory", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", x, y));
        session.RunTicks(1);
        GameEntity workshop = session.Entities.Single(e => e.Kind == ObjectKind.Workshop);
        SessionData.RunUntilComplete(session, workshop);
        return workshop;
    }

    /// <summary>단계 A~I 를 끝까지 걷는다. 걷는 도중 단계마다의 상태를 확인하고 안내 목록을 돌려준다.</summary>
    private static List<string> PlayThrough(BattleSession session)
    {
        var log = new List<string>();
        int second = session.TicksPerSecond;
        PlayerState player = session.Player(1);

        // 단계 A: 처음 안내. sunFactory 는 아직 허용되지 않고 회수는 금지다
        Assert.Equal('A', session.Tutorial!.Stage);
        session.RunTicks(second);
        Assert.Equal('A', session.Tutorial.Stage);
        Assert.False(player.Tech.IsAllowed("sunFactory"));
        Assert.True(session.DenySalvage);

        // 템플 완공 틱에 단계 B 로 넘어간다. 다음 틱부터 단계 B 처리가 sunFactory 를 허용한다
        GameEntity temple = BuildTemple(session);
        Assert.Equal('B', session.Tutorial.Stage);
        Assert.False(player.Tech.IsAllowed("sunFactory"));
        session.RunTicks(1);
        Assert.True(player.Tech.IsAllowed("sunFactory"));

        // 워크샵 완공 → 단계 C
        GameEntity workshop = BuildWorkshop(session);
        Assert.Equal('C', session.Tutorial.Stage);

        // 단계 C: 워크샵 대신 템플을 선택하고 있으면 2초 뒤에 "NotVortex" 안내가 나오고 선택이 풀린다. 단계는 그대로다
        session.Submit(new SelectEntityCommand(1, temple.Id));
        session.RunTicks(1);
        Assert.Equal(temple.Id, player.SelectedEntityId);
        session.RunTicks(2 * second - 2);
        Assert.DoesNotContain("NotVortex", Tells(session, log));
        session.RunTicks(3);
        Assert.Contains("NotVortex", Tells(session, log));
        Assert.Equal(0, player.SelectedEntityId);
        Assert.Equal('C', session.Tutorial.Stage);

        // 지식을 워크샵에 등록하면 단계 D
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunArcher"));
        session.RunTicks(1);
        Assert.Equal('D', session.Tutorial.Stage);

        // 첫 유닛을 놓으면 E, 둘째를 놓으면 F
        PlaceArcher(session);
        Assert.Equal('E', session.Tutorial.Stage);
        PlaceArcher(session);
        Assert.Equal('F', session.Tutorial.Stage);

        // 단계 F: 템플을 고르지 않으면 넘어가지 않는다. 고르면 4초 뒤 단계 G 로 넘어가고 선택이 풀린다
        session.RunTicks(6 * second);
        Assert.Equal('F', session.Tutorial.Stage);
        session.Submit(new SelectEntityCommand(1, temple.Id));
        session.RunTicks(1);
        long timer = session.Tutorial.TimerTick;
        Assert.Equal(session.Tick + 4 * second, timer);
        session.RunTicks((int)(timer - session.Tick) - 1);
        Assert.Equal('F', session.Tutorial.Stage);
        session.RunTicks(1);
        Assert.Equal('G', session.Tutorial.Stage);
        Assert.Equal(0, player.SelectedEntityId);

        // 단계 G: 셋째까지는 그대로, 넷째("두 개 더")를 놓으면 H. 그 전에는 회수가 금지다
        PlaceArcher(session);
        Assert.Equal('G', session.Tutorial.Stage);
        Drain(session, log);
        GameEntity archer = session.Entities.Last(e => e.Kind == ObjectKind.Emplacement);
        session.Submit(new SalvageCommand(1, archer.Id));
        session.RunTicks(1);
        Assert.Equal(CommandFailure.SalvageDenied, Drain(session, log).Single(e => e.Kind == SessionEventKind.CommandRejected).Failure);
        PlaceArcher(session);
        Assert.Equal('H', session.Tutorial.Stage);

        // 단계 H: 다음 틱부터 회수 금지가 풀린다. 회수하면 2초 뒤 단계 I (Mission Accomplished!)
        session.RunTicks(1);
        Assert.False(session.DenySalvage);
        Assert.Equal(0, session.Tutorial.TimerTick);
        int before = player.StormPower;
        session.Submit(new SalvageCommand(1, archer.Id));
        session.RunTicks(1);
        Assert.Equal(before + 75, player.StormPower);
        timer = session.Tutorial.TimerTick;
        Assert.Equal(session.Tick + 2 * second, timer);
        session.RunTicks((int)(timer - session.Tick) - 1);
        Assert.Equal('H', session.Tutorial.Stage);
        session.RunTicks(1);
        Assert.Equal('I', session.Tutorial.Stage);
        Assert.True(session.Tutorial.Finished);
        return Tells(session, log);
    }

    /// <summary>튜토리얼 2 는 단계 A 안내로 시작하고, 명령만으로 A~I 를 걸으면 단계마다의 안내가 차례로 나온다</summary>
    [Fact]
    public void Tutorial2_WalksThroughAllStagesInOrder()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        Assert.Equal(["A."], Tells(session, []));
        Assert.False(session.Tutorial!.Finished);
        // 시작 안내를 이미 받았으므로 걷는 동안 나오는 안내만 확인한다
        List<string> tells = PlayThrough(SessionData.FromMission("tutorial2"));
        Assert.Equal(["A.", "B.", "C.", "NotVortex", "D.", "E.", "F.", "G.", "H.", "I."], tells);
    }

    /// <summary>같은 명령을 같은 틱에 넣은 두 세션은 단계 처리를 거쳐도 검사합이 같다 (락스텝·리플레이 조건)</summary>
    [Fact]
    public void Tutorial2_IsDeterministic()
    {
        BattleSession a = SessionData.FromMission("tutorial2");
        BattleSession b = SessionData.FromMission("tutorial2");
        PlayThrough(a);
        PlayThrough(b);
        Assert.Equal(a.Checksum(), b.Checksum());
        Assert.Equal('I', b.Tutorial!.Stage);
        // 단계가 다르면 검사합도 다르다
        Assert.NotEqual(a.Checksum(), SessionData.FromMission("tutorial2").Checksum());
    }

    /// <summary>지은 수는 누적이다: 유닛을 회수해도 줄지 않는다 (원본 DAT_005c98d0 — 파괴·회수 경로가 줄이지 않는다)</summary>
    [Fact]
    public void MadeCount_IsCumulativeAndSurvivesSalvage()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        PlayThrough(session);
        PlayerState player = session.Player(1);
        // 넷을 놓고 하나를 회수했지만 누적은 그대로다
        Assert.Equal(4, player.Made("sunArcher"));
        Assert.Equal(3, session.Entities.Count(e => e.Kind == ObjectKind.Emplacement && e.Owner == 1));
        Assert.Equal(1, player.MadeWithFlags(Netstorm.Assets.TypeFlagBits.Flag2Words["vortex"]));
        Assert.Equal(1, player.MadeWithFlags(Netstorm.Assets.TypeFlagBits.Factory));
        // 대소문자를 구분하지 않고, 짓지 않은 타입은 0 이다
        Assert.Equal(4, player.Made("SUNARCHER"));
        Assert.Equal(0, player.Made("bulf"));
    }

    /// <summary>선택 명령은 없는 오브젝트를 거부하고, 선택한 오브젝트가 회수로 사라지면 선택도 풀린다</summary>
    [Fact]
    public void SelectEntity_RejectsUnknownAndClearsWhenSalvaged()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        session.Submit(new SelectEntityCommand(1, 99999));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NoSuchEntity);
        Assert.Equal(0, session.Player(1).SelectedEntityId);

        // 사제를 선택했다가 0 으로 풀 수 있다
        GameEntity priest = session.Entities.First(e => e.Kind == ObjectKind.Priest && e.Owner == 1);
        session.Submit(new SelectEntityCommand(1, priest.Id));
        session.RunTicks(1);
        Assert.Equal(priest.Id, session.Player(1).SelectedEntityId);
        session.Submit(new SelectEntityCommand(1, 0));
        session.RunTicks(1);
        Assert.Equal(0, session.Player(1).SelectedEntityId);

        // 회수 금지를 풀고 워크샵을 회수하면 선택이 사라진다 (단계 처리는 끄고 규칙만 확인)
        session.RunsTutorial = false;
        session.Player(1).Tech.Set("sunFactory", true);
        BuildTemple(session);
        GameEntity workshop = BuildWorkshop(session);
        session.DenySalvage = false;
        session.Submit(new SelectEntityCommand(1, workshop.Id));
        session.RunTicks(1);
        session.Submit(new SalvageCommand(1, workshop.Id));
        session.RunTicks(1);
        Assert.Equal(0, session.Player(1).SelectedEntityId);
    }

    /// <summary>단계 처리를 끄면 조건을 채워도 그대로다. 튜토리얼 1·2는 지원하고 3은 아직 지원하지 않는다.</summary>
    [Fact]
    public void RunsTutorialFlag_FreezesStageAndOtherMissionsHaveNone()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        session.RunsTutorial = false;
        BuildTemple(session);
        Assert.Equal('A', session.Tutorial!.Stage);
        Assert.False(session.Player(1).Tech.IsAllowed("sunFactory"));

        // 튜토리얼 1 은 가이저 수집 경로와 함께 단계 처리가 생겼다.
        Assert.Equal('A', SessionData.FromMission("tutorial1").Tutorial!.Stage);
        Assert.True(TutorialStages.IsSupported(1));
        Assert.True(TutorialStages.IsSupported(2));
        Assert.False(TutorialStages.IsSupported(3));
        Assert.Equal("C.", TutorialStages.SectionOf('C'));
    }
}
