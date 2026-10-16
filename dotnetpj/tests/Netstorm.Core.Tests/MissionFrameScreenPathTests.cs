using Netstorm.Core.Display;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 화면 경로(원본 숫자 시계 + 세션 프레임 시계 + 공통 미션 프레임 게이트 + 튜토리얼 단계의 공유 상태)를 헤드리스로 재생한다.
/// 뷰어와 같은 정지 사유 동기화·훅 연결 방식을 쓰므로, 안내 10프레임 잠금·7프레임 재개·정지 중 단계 증가 거부·
/// 0.5초 후속 지연·2/4초 타이머 경계를 같은 코드로 검사한다.
/// </summary>
/// <remarks>
/// 원본 기계어 대조는 <see cref="X86MissionFrameGateTests"/>가 맡고, 이 검사는 그 게이트가 세션과 조립된 뒤의 동작을 확인한다.
/// 창이 필요한 부분(그리기·입력)은 포함하지 않는다. 2026-10-15 감사 D01/D02 에서 추가.
/// </remarks>
public sealed class MissionFrameScreenPathTests
{
    /// <summary>한 화면 프레임이 흘려보내는 밀리초(약 62.5Hz). 간격이 일정해야 경계 시각 계산이 단순하다.</summary>
    private const int FrameMilliseconds = 16;

    /// <summary>원본 004c2990/004c34c0 의 안내 잠금 프레임 수. 게이트의 상수와 독립적으로 원본 값을 직접 쓴다.</summary>
    private const int OriginalLockFrames = 10;

    /// <summary>원본 004c34c0 의 안내 종료 뒤 재개까지의 연속 빈 프레임 수. 게이트의 상수와 독립적으로 원본 값을 직접 쓴다.</summary>
    private const int OriginalResumeFrames = 7;

    /// <summary>
    /// 뷰어의 정지 사유 동기화와 훅 연결을 그대로 옮긴 화면 모형. 안내 창은 열림/닫힘 플래그로 대신한다.
    /// 안내 창이 열려 있으면 뷰어는 세션 진행(UpdateSession)을 부르지 않으므로 같은 방식으로 생략한다.
    /// </summary>
    private sealed class Screen
    {
        /// <summary>검사 대상 세션.</summary>
        public BattleSession Session { get; }

        /// <summary>앱이 공유하는 원본 숫자 게임 시계(정지 없이 시작).</summary>
        public OriginalGameClock GameClock { get; } = new(0);

        /// <summary>세션 시각과 정지 소유권을 관리하는 실제 연결기.</summary>
        public BattleSessionFrameClock Clock { get; }

        /// <summary>공통 미션 프레임 게이트.</summary>
        public MissionFrameGate Gate { get; }

        /// <summary>화면이 고정한 현재 밀리초.</summary>
        public long Milliseconds { get; private set; }

        /// <summary>수동 정지(Shift-F9)를 모형화한 다른 소유자의 정지.</summary>
        public bool ManualPause { get; set; }

        /// <summary>스크립트 안내 창이 열려 있는지.</summary>
        public bool DialogOpen { get; set; }

        /// <summary>외부 도움말/옵션 창이 열려 있는지(안내 잠금과 재개를 막는다).</summary>
        public bool ExternalDialogOpen { get; set; }

        /// <summary>안내가 열린 순서대로 쌓은 섹션 이름.</summary>
        public List<string> Tells { get; } = [];

        /// <summary>이 미션이 정지를 보유하는지(게이트의 Pause/Resume 경계가 바꾼다).</summary>
        private bool _scriptPauseHeld;

        /// <summary>초기 브리핑 요청을 보냈는지.</summary>
        private bool _initialBriefingRequested;

        /// <summary>세션과 게이트를 뷰어와 같은 순서로 연결한다.</summary>
        /// <param name="session">미션 세션.</param>
        public Screen(BattleSession session)
        {
            Session = session;
            Clock = new BattleSessionFrameClock(session, GameClock, 0);
            var state = new MissionFrameGateState { Started = 1, StartSeconds = Clock.RawGameSeconds };
            Gate = new MissionFrameGate(state, new MissionFrameGateHooks
            {
                GameSeconds = () => Clock.RawGameSeconds,
                Paused = () => Clock.IsPaused,
                CheckOutcome = () => Session.CheckMissionOutcome(),
                DialogsClear = () => !(ExternalDialogOpen || DialogOpen),
                KnowledgeOpen = () => false,
                GuideOpen = () => DialogOpen,
                BlockingGumpOpen = () => false,
                TellStage = () =>
                {
                    bool initial = !_initialBriefingRequested;
                    _initialBriefingRequested = true;
                    Tells.Add(initial ? "A." : Session.Tutorial?.Section ?? "A.");
                    DialogOpen = true;
                    Gate!.PauseForDialog();
                },
                TellAt = () => throw new NotSupportedException(),
                EndDemo = () => throw new NotSupportedException(),
                Tutorial = (number, current) =>
                {
                    Synchronize();
                    Session.AdvanceTutorialScreenFrame(Clock.RawGameSeconds);
                    return 0;
                },
                Pause = () => _scriptPauseHeld = true,
                Resume = () => _scriptPauseHeld = false,
            });
            Session.ConnectTutorialScreenFrames(Gate, section =>
            {
                Tells.Add(section);
                DialogOpen = true;
                Gate.PauseForDialog();
                Synchronize();
            });
            Session.TutorialFrameLocked = true;
            Synchronize();
        }

        /// <summary>뷰어의 SynchronizePauseReasons 와 같다: 열린 안내는 정지를 잡고 보유 상태·수동 정지를 시계에 반영한다.</summary>
        public void Synchronize()
        {
            SessionPauseReason reasons = SessionPauseReason.None;
            if (ManualPause) reasons |= SessionPauseReason.Manual;
            if (DialogOpen) Gate.PauseForDialog();
            if (_scriptPauseHeld) reasons |= SessionPauseReason.ScriptDialog;
            Clock.SetPauseReasons(reasons, Milliseconds);
            Session.TutorialFrameLocked = Gate.State.LockFrames > 0 || Clock.IsPaused;
        }

        /// <summary>화면 프레임 하나: 정지 동기화 → (안내가 닫혀 있으면) 세션 진행 → 공통 미션 프레임 → 정지 동기화.</summary>
        public void Frame()
        {
            Milliseconds += FrameMilliseconds;
            Synchronize();
            if (!DialogOpen) Clock.AdvanceFrame(Milliseconds);
            try
            {
                Gate.RunFrame();
            }
            finally
            {
                Session.EndTutorialScreenFrame();
            }
            Synchronize();
        }

        /// <summary>프레임을 지정한 수만큼 진행한다.</summary>
        /// <param name="count">프레임 수.</param>
        public void Frames(int count)
        {
            // 요청한 수만큼 화면 프레임을 반복한다.
            for (int i = 0; i < count; i++) Frame();
        }

        /// <summary>조건이 참이 될 때까지 프레임을 진행하고 진행한 프레임 수를 돌려준다. 제한을 넘으면 실패한다.</summary>
        /// <param name="condition">종료 조건.</param>
        /// <param name="maxFrames">최대 프레임 수.</param>
        public int RunUntil(Func<bool> condition, int maxFrames = 20000)
        {
            // 조건이 참이 되거나 제한에 닿을 때까지 한 프레임씩 진행한다.
            for (int frame = 0; frame < maxFrames; frame++)
            {
                if (condition()) return frame;
                Frame();
            }
            throw new InvalidOperationException($"{maxFrames}프레임 안에 조건이 참이 되지 않았습니다. 안내: {string.Join(',', Tells)}");
        }

        /// <summary>열린 안내를 닫고 자기 정지가 풀릴 때까지 정확히 7프레임이 걸리는지 확인한다.</summary>
        public void Acknowledge()
        {
            Assert.True(DialogOpen);
            DialogOpen = false;
            // 닫은 뒤 6프레임까지는 정지가 유지된다.
            for (int i = 1; i < OriginalResumeFrames; i++)
            {
                Frame();
                Assert.True(Clock.IsPaused, $"{i}번째 빈 프레임에서 재개되면 안 됩니다.");
            }
            Frame();
            Assert.False(Clock.IsPaused);
            Assert.Equal(0u, Gate.State.ScriptPaused);
            Assert.Equal(0, Gate.State.ClearFrames);
        }

        /// <summary>10프레임 잠금이 끝나 초기 브리핑이 열릴 때까지 진행하고 닫는다.</summary>
        public void PassInitialBriefing()
        {
            RunUntil(() => Tells.Count == 1);
            Assert.Equal(["A."], Tells);
            Acknowledge();
        }
    }

    /// <summary>초기 브리핑은 안내가 없는 연속 10프레임째에 열리고, 열리는 순간 게임 시계가 정지한다.</summary>
    [Fact]
    public void InitialBriefing_OpensOnTenthQuietFrameAndPausesClock()
    {
        var screen = new Screen(SessionData.FromMission("tutorial2"));
        // 9프레임까지는 잠금 카운터만 줄어든다.
        for (int i = 1; i < OriginalLockFrames; i++)
        {
            screen.Frame();
            Assert.Empty(screen.Tells);
            Assert.Equal(OriginalLockFrames - i, screen.Gate.State.LockFrames);
            Assert.False(screen.Clock.IsPaused);
        }
        screen.Frame();
        Assert.Equal(["A."], screen.Tells);
        Assert.Equal(0, screen.Gate.State.LockFrames);
        Assert.True(screen.Clock.IsPaused);
        Assert.Equal(1u, screen.Gate.State.ScriptPaused);
    }

    /// <summary>외부 창(도움말/옵션)이 열린 프레임은 잠금 카운터를 10으로 되돌리고, 닫힌 뒤 다시 10프레임이 필요하다.</summary>
    [Fact]
    public void BriefingLock_ResetsWhileExternalDialogIsOpen()
    {
        var screen = new Screen(SessionData.FromMission("tutorial2"));
        screen.Frames(5);
        Assert.Equal(5, screen.Gate.State.LockFrames);
        screen.ExternalDialogOpen = true;
        screen.Frames(3);
        Assert.Equal(OriginalLockFrames, screen.Gate.State.LockFrames);
        Assert.Empty(screen.Tells);
        screen.ExternalDialogOpen = false;
        screen.Frames(9);
        Assert.Empty(screen.Tells);
        screen.Frame();
        Assert.Equal(["A."], screen.Tells);
    }

    /// <summary>안내를 닫은 뒤 7프레임 연속으로 막는 창이 없어야 정지가 풀리고, 중간에 창이 열리면 카운터가 0으로 돌아간다.</summary>
    [Fact]
    public void Resume_RequiresSevenConsecutiveQuietFrames()
    {
        var screen = new Screen(SessionData.FromMission("tutorial2"));
        screen.RunUntil(() => screen.Tells.Count == 1);
        screen.DialogOpen = false;
        screen.Frames(3);
        Assert.Equal(3, screen.Gate.State.ClearFrames);
        screen.ExternalDialogOpen = true;
        screen.Frame();
        Assert.Equal(0, screen.Gate.State.ClearFrames);
        Assert.True(screen.Clock.IsPaused);
        screen.ExternalDialogOpen = false;
        screen.Frames(6);
        Assert.True(screen.Clock.IsPaused);
        screen.Frame();
        Assert.False(screen.Clock.IsPaused);
        Assert.Equal(0u, screen.Gate.State.ScriptPaused);
    }

    /// <summary>
    /// 다른 소유자가 정지 중이어도 단계 판정은 호출되지만 단계 증가는 거부된다. 거부된 프레임의 사건은 다음 프레임에 재생하지 않으며
    /// 정지가 풀린 뒤 새 사건이 오면 증가하고 게이트 상태(글자·잠금·안내 기한)를 함께 갱신한다.
    /// </summary>
    [Fact]
    public void StageAdvance_IsRefusedWhilePausedAndDoesNotReplayDroppedEvent()
    {
        var screen = new Screen(SessionData.FromMission("tutorial1"));
        screen.PassInitialBriefing();
        Assert.Equal('A', screen.Session.Tutorial!.Stage);
        screen.ManualPause = true;
        screen.Frame();
        Assert.True(screen.Clock.IsPaused);

        // 정지 중에도 명령은 실행되고 사건이 단계 판정에 전달되지만 증가는 거부된다.
        screen.Session.Submit(new ReturnHomeCommand(1));
        screen.Frame();
        Assert.Equal('A', screen.Session.Tutorial.Stage);
        screen.ManualPause = false;
        screen.Frames(5);
        Assert.False(screen.Clock.IsPaused);
        Assert.Equal('A', screen.Session.Tutorial.Stage);

        // 새 사건은 증가시키고 원본 004c33f0 처럼 글자·잠금 10·안내 기한(현재 + float 0.5)·타이머 0을 한 번에 쓴다.
        screen.Session.Submit(new ReturnHomeCommand(1));
        screen.Frame();
        Assert.Equal('B', screen.Session.Tutorial.Stage);
        Assert.Equal((int)'B', screen.Gate.State.Stage);
        Assert.Equal(OriginalLockFrames, screen.Gate.State.LockFrames);
        Assert.Equal((double)0.5f + screen.Clock.RawGameSeconds, screen.Gate.State.NextStageSeconds);
        Assert.Equal(0, screen.Gate.State.TimerSeconds);
    }

    /// <summary>튜토리얼 단계 글자와 임시 타이머는 게이트 상태 하나를 공유한다(연결 시 현재 값을 옮기고 이후 같은 값을 읽는다).</summary>
    [Fact]
    public void TutorialStage_SharesStateWithGate()
    {
        var screen = new Screen(SessionData.FromMission("tutorial2"));
        Assert.Equal((int)'A', screen.Gate.State.Stage);
        Assert.Equal(2u, screen.Gate.State.TutorialNumber);
        screen.Gate.State.Stage = 'D';
        screen.Gate.State.TimerSeconds = 12.5;
        Assert.Equal('D', screen.Session.Tutorial!.Stage);
        Assert.Equal(12.5, screen.Session.Tutorial.TimerSeconds);
        // 튜토리얼이 없는 미션은 1~6 분배를 하지 않는다.
        MissionStart campaign = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission(CampaignAccess.FirstMission)!.Script);
        Assert.Equal(0u, new Screen(SessionData.FromMap(CampaignAccess.FirstMission, campaign)).Gate.State.TutorialNumber);
    }

    /// <summary>
    /// 튜토리얼 2의 A~I를 화면 경로로 끝까지 걷는다. 단계마다 0.5초 + 10프레임 뒤 안내가 열리고 정지하며,
    /// 7프레임 뒤 재개된다. 단계 C·F·H의 double 타이머는 현재 게임 시각 + 2/4/2초의 포함 경계에서 만료한다.
    /// </summary>
    [Fact]
    public void Tutorial2_WalksAllStagesThroughScreenPath()
    {
        var screen = new Screen(SessionData.FromMission("tutorial2"));
        BattleSession session = screen.Session;
        PlayerState player = session.Player(1);
        screen.PassInitialBriefing();

        // 신전 완공 → 단계 B. 안내는 0.5초 뒤 10프레임 연속을 채운 프레임에 열린다.
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "windVortex", x, y));
        screen.Frame();
        GameEntity temple = session.Entities.Single(e => e.Kind == ObjectKind.Temple);
        // 완공된 바로 그 프레임의 공통 프레임이 단계를 올린다(RunUntil은 조건이 참인 상태에서 더 진행하지 않는다).
        screen.RunUntil(() => temple.IsComplete);
        Assert.Equal('B', session.Tutorial!.Stage);
        double stageAt = screen.Gate.State.NextStageSeconds;
        Assert.Equal((double)0.5f + screen.Clock.RawGameSeconds, stageAt);
        Assert.Equal(OriginalLockFrames, screen.Gate.State.LockFrames);
        int countdown = 0;
        // 안내 기한 이후의 프레임만 잠금 카운터를 센다. 기한 전에는 카운터와 안내가 움직이지 않는다.
        while (screen.Tells.Count == 1)
        {
            screen.Frame();
            if (screen.Clock.RawGameSeconds >= stageAt) countdown++;
        }
        Assert.Equal(OriginalLockFrames, countdown);
        Assert.Equal(["A.", "B."], screen.Tells);
        screen.Acknowledge();

        // 워크샾 완공 → 단계 C(sunFactory는 단계 B의 판정이 허용한다).
        Assert.True(player.Tech.IsAllowed("sunFactory"));
        (x, y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "sunFactory", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", x, y));
        screen.Frame();
        GameEntity workshop = session.Entities.Single(e => e.Kind == ObjectKind.Workshop);
        screen.RunUntil(() => workshop.IsComplete);
        screen.RunUntil(() => screen.Tells.Count == 3);
        Assert.Equal('C', session.Tutorial.Stage);
        screen.Acknowledge();

        // 단계 C: 신전을 선택한 채 2초(포함 경계) 뒤 NotVortex 안내가 열리고 선택이 풀린다. 단계는 그대로다.
        session.Submit(new SelectEntityCommand(1, temple.Id));
        screen.Frame();
        double timer = session.Tutorial.TimerSeconds;
        Assert.Equal(screen.Clock.RawGameSeconds + 2.0, timer);
        Assert.Equal(timer, screen.Gate.State.TimerSeconds);
        double before;
        // 안내가 열리는 프레임까지 직전 시각을 기억하며 진행한다.
        do
        {
            before = screen.Clock.RawGameSeconds;
            screen.Frame();
        }
        while (!screen.Tells.Contains("NotVortex"));
        Assert.True(before < timer && timer <= screen.Clock.RawGameSeconds);
        Assert.Equal(0, player.SelectedEntityId);
        Assert.Equal('C', session.Tutorial.Stage);
        Assert.True(screen.Clock.IsPaused);
        screen.Acknowledge();

        // 지식을 워크샾에 등록하면 단계 D.
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunArcher"));
        screen.Frame();
        Assert.Equal('D', session.Tutorial.Stage);
        screen.RunUntil(() => screen.Tells.Count == 5);
        screen.Acknowledge();

        // 첫 유닛 → E, 둘째 → F. 각 단계 안내는 정지/재개를 거친다.
        PlaceArcher(screen);
        Assert.Equal('E', session.Tutorial.Stage);
        screen.RunUntil(() => screen.Tells.Count == 6);
        screen.Acknowledge();
        PlaceArcher(screen);
        Assert.Equal('F', session.Tutorial.Stage);
        screen.RunUntil(() => screen.Tells.Count == 7);
        screen.Acknowledge();

        // 단계 F: 신전을 고르지 않으면 넘어가지 않는다. 고르면 4초(포함 경계) 뒤 단계 G가 되고 선택이 풀린다.
        screen.Frames(6 * 60);
        Assert.Equal('F', session.Tutorial.Stage);
        session.Submit(new SelectEntityCommand(1, temple.Id));
        screen.Frame();
        timer = session.Tutorial.TimerSeconds;
        Assert.Equal(screen.Clock.RawGameSeconds + 4.0, timer);
        // 만료 프레임의 직전 시각을 기억하며 단계가 바뀔 때까지 진행한다.
        do
        {
            before = screen.Clock.RawGameSeconds;
            screen.Frame();
        }
        while (session.Tutorial.Stage == 'F');
        Assert.True(before < timer && timer <= screen.Clock.RawGameSeconds);
        Assert.Equal('G', session.Tutorial.Stage);
        Assert.Equal(0, player.SelectedEntityId);
        Assert.Equal(0, session.Tutorial.TimerSeconds);
        screen.RunUntil(() => screen.Tells.Count == 8);
        screen.Acknowledge();

        // 단계 G: 셋째까지는 그대로, 넷째를 놓으면 H. 그 전에는 회수가 금지다.
        PlaceArcher(screen);
        Assert.Equal('G', session.Tutorial.Stage);
        GameEntity victim = PlaceArcher(screen);
        Assert.Equal('H', session.Tutorial.Stage);
        screen.RunUntil(() => screen.Tells.Count == 9);
        screen.Acknowledge();

        // 단계 H: 회수 금지가 풀리고, 회수하면 2초(포함 경계) 뒤 마지막 단계 I(Mission Accomplished!)가 된다.
        Assert.False(session.DenySalvage);
        session.Submit(new SalvageCommand(1, victim.Id));
        screen.Frame();
        timer = session.Tutorial.TimerSeconds;
        Assert.Equal(screen.Clock.RawGameSeconds + 2.0, timer);
        // 만료 프레임의 직전 시각을 기억하며 단계가 바뀔 때까지 진행한다.
        do
        {
            before = screen.Clock.RawGameSeconds;
            screen.Frame();
        }
        while (session.Tutorial.Stage == 'H');
        Assert.True(before < timer && timer <= screen.Clock.RawGameSeconds);
        screen.RunUntil(() => screen.Tells.Count == 10);
        Assert.True(session.Tutorial.Finished);
        Assert.Equal(["A.", "B.", "C.", "NotVortex", "D.", "E.", "F.", "G.", "H.", "I."], screen.Tells);
    }

    /// <summary>유닛 재충전을 기다린 뒤 조건에 맞는 첫 칸에 sunArcher 를 놓고 완공된 바로 그 프레임까지 진행한다.</summary>
    /// <param name="screen">화면 모형.</param>
    /// <returns>완공된 유닛.</returns>
    private static GameEntity PlaceArcher(Screen screen)
    {
        BattleSession session = screen.Session;
        screen.RunUntil(() => session.SecondsUntilReady(1, "sunArcher") <= 0);
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckUnit(1, "sunArcher", cx, cy).Allowed);
        session.Submit(new PlaceUnitCommand(1, "sunArcher", x, y));
        screen.Frame();
        GameEntity archer = session.Entities.Single(e => e.Type.Name.Equals("sunArcher", StringComparison.OrdinalIgnoreCase)
            && e.Footprint.AnchorX == x && e.Footprint.AnchorY == y);
        screen.RunUntil(() => archer.IsComplete);
        return archer;
    }
}
