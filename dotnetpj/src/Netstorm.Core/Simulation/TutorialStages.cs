using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 튜토리얼의 단계 처리 (원본 Totalmade.cpp 의 튜토리얼별 함수: 1 = 004c3a20, <b>2 = 004c3bb0</b>, 3 = 004c3f00 …, 프레임마다 호출).
/// 원본은 단계 글자('A'부터)를 하나씩 올리며 단계마다 미션 스크립트의 "글자." 섹션을 안내 창으로 띄운다.
/// 단계 처리는 미션 머리 값이 정한 <b>시작 상태</b>를 실행 중에 바꾼다: 튜토리얼 2 는 단계 B 에서 sunFactory 를 허용하고
/// 단계 H 에서 회수 금지를 푼다 (docs/exe/mission-header-flags.md).
/// 세션 상태/명령을 보고 판정한다. 실제 게임은 화면마다 한 번, 연결 전 헤드리스는 기존 명시적 틱에서 Update를 부른다.
/// 튜토리얼 1·2 를 구현했다. 튜토리얼 3~6 은 전투가 필요하다.
/// </summary>
public sealed class TutorialStages
{
    /// <summary>구현된 튜토리얼 번호 (Secret Workshop)</summary>
    private const int SecretWorkshop = 2;

    /// <summary>Bridge the Gap! 튜토리얼 번호.</summary>
    private const int BridgeTheGap = 1;

    /// <summary>첫 단계 글자 (원본 FUN_004c2990 이 0x41 로 시작한다)</summary>
    private const char FirstStage = 'A';

    /// <summary>마지막 단계 글자 — "Mission Accomplished!" (튜토리얼 2 의 [I.])</summary>
    private const char SecretWorkshopLastStage = 'I';

    /// <summary>튜토리얼 1의 완료 단계.</summary>
    private const char BridgeTheGapLastStage = 'G';

    /// <summary>튜토리얼 1의 다리 실습 첫 통과 기준(원본 004c3a20 단계 B: 누적 수가 7보다 큼).</summary>
    private const int FirstBridgeTarget = 8;

    /// <summary>튜토리얼 1의 다리 회전 실습 통과 기준(원본 004c3a20 단계 C: 누적 수가 0x12(18)보다 큼).</summary>
    private const int SecondBridgeTarget = 19;

    /// <summary>튜토리얼 1의 최종 Storm Power 목표(미션 스크립트 F.).</summary>
    private const int BridgeTheGapStormPowerTarget = 600;

    /// <summary>튜토리얼 1에서 결정 하나를 전달한 뒤 넘어가는 Storm Power 기준(원본 0x510298).</summary>
    private const int FirstCrystalStormPowerTarget = 200;

    /// <summary>단계 C: 신전을 선택한 채 이 시간(초)이 지나면 보정 안내를 띄운다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double NotVortexDelaySeconds = 2.0;

    /// <summary>단계 F: 신전을 선택하고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506588 의 double 값 4.0)</summary>
    private const double TempleViewDelaySeconds = 4.0;

    /// <summary>단계 H: 회수 명령을 내리고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double SalvageDelaySeconds = 2.0;

    /// <summary>신전 타입의 플래그2 비트 (typeflags "vortex" = 0x200)</summary>
    private static readonly uint VortexFlag = TypeFlagBits.Flag2Words["vortex"];

    /// <summary>다리 타입의 플래그2 비트 (typeflags "bridge" = 0x4). 튜토리얼 1 단계 A·B·C 가 누적 제작 수의 합으로 읽는다</summary>
    private static readonly uint BridgeFlag = TypeFlagBits.Bridge;

    /// <summary>단계 C 보정 안내 섹션 이름 (스크립트 [NotVortex])</summary>
    public const string NotVortexSection = "NotVortex";

    /// <summary>튜토리얼 번호 (미션 머리 tutorialNumber)</summary>
    public int Number { get; }

    /// <summary>지금 단계 글자 (원본 DAT_005ca8e8)</summary>
    public char Stage { get; private set; } = FirstStage;

    /// <summary>지금 단계의 스크립트 섹션 이름 ("A." 형식, 원본 FUN_004c2a90)</summary>
    public string Section => SectionOf(Stage);

    /// <summary>마지막 단계에 도달했는지</summary>
    public bool Finished => Stage == (Number == BridgeTheGap ? BridgeTheGapLastStage : SecretWorkshopLastStage);

    /// <summary>튜토리얼 1에서 마지막으로 연결을 검사한 다리 격자 버전.</summary>
    private int _lastReachVersion = -1;

    /// <summary>마지막 연결 검사에서 가이저에 닿을 수 있었는지.</summary>
    private bool _lastReachable;

    /// <summary>헤드리스 합성 진행의 기존 타이머 틱과 진단값. 실제 화면 판정에서는 TimerSeconds를 사용한다.</summary>
    public long TimerTick { get; private set; }

    /// <summary>실제 화면에서 사용하는 원본 +98 double 게임 시각 타이머. 0이면 없음이며 단계가 바뀔 때 지운다.</summary>
    public double TimerSeconds { get; private set; }

    /// <summary>튜토리얼 번호에 대한 단계 처리를 구현했는지</summary>
    /// <param name="number">튜토리얼 번호</param>
    public static bool IsSupported(int number) => number is BridgeTheGap or SecretWorkshop;

    /// <summary>단계 글자로 스크립트 섹션 이름을 만든다</summary>
    /// <param name="stage">단계 글자</param>
    public static string SectionOf(char stage) => $"{stage}.";

    /// <summary>단계 처리를 만든다</summary>
    /// <param name="number">튜토리얼 번호</param>
    internal TutorialStages(int number) => Number = number;

    /// <summary>
    /// 단계 판정 한 번. 실제 화면은 현재 원본 게임 시각과 +78/+80/10프레임 잠금을 공유한다.
    /// 화면 연결 전의 헤드리스 명시적 진행은 기존 틱/즉시 안내 경로를 유지한다.
    /// </summary>
    /// <param name="session">세션</param>
    /// <param name="tickEvents">이번 화면 또는 헤드리스 호출의 명령 사건. 단계 H가 회수 명령을 알아본다.</param>
    internal void Update(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        if (Number == BridgeTheGap)
        {
            UpdateBridgeTheGap(session, tickEvents);
            return;
        }
        if (Number != SecretWorkshop)
        {
            return;
        }
        PlayerState player = session.Player(session.HumanPlayer);
        switch (Stage)
        {
            case 'A':
                // 전투 옵션을 덮어쓴다: Generator Range = Short(14칸), Unit Rate = Fast (프레임마다 다시 정하지만 값은 같다)
                session.Map.Options.ApplyTutorialTwoOverrides();
                // 신전(typeflags vortex)을 지은 개수가 0 보다 크면 다음 단계
                if (player.MadeWithFlags(VortexFlag) > 0)
                {
                    Advance(session);
                }
                break;
            case 'B':
                // 표에서 Sun Workshop(sunFactory)을 허용으로 바꾼다 (원본 FUN_004c23e0(sunFactory, 허용))
                player.Tech.Set("sunFactory", true);
                // 워크샾(typeflags factory)을 지은 개수가 0 보다 크면 다음 단계 (선택을 푼다)
                if (player.MadeWithFlags(TypeFlagBits.Factory) > 0)
                {
                    Advance(session);
                    session.ClearSelection(player);
                }
                break;
            case 'C':
                UpdateProduction(session, player);
                break;
            case 'D':
                // 첫 Sun Disc Thrower 를 놓았으면 다음 단계
                if (player.Made("sunArcher") > 0)
                {
                    Advance(session);
                }
                break;
            case 'E':
                // 두 번째를 놓았으면 다음 단계 (선택을 푼다)
                if (player.Made("sunArcher") > 1)
                {
                    Advance(session);
                    session.ClearSelection(player);
                }
                break;
            case 'F':
                UpdateEnergyDisplay(session, player);
                break;
            case 'G':
                // 네 번째를 놓았으면 ("두 개 더") 다음 단계 (선택을 푼다, 타이머는 단계를 넘길 때 지워진다)
                if (player.Made("sunArcher") > 3)
                {
                    Advance(session);
                    session.ClearSelection(player);
                }
                break;
            case 'H':
                UpdateSalvage(session, tickEvents);
                break;
        }
    }

    /// <summary>튜토리얼 1의 F4·다리 칸·가이저 연결·결정 전달·600 SP 단계 (원본 004c3a20).</summary>
    private void UpdateBridgeTheGap(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        PlayerState player = session.Player(session.HumanPlayer);
        // 원본은 살아 있는 칸 수가 아니라 누적 제작 표의 합(FUN_004c2500(4) = DAT_005c98d0 에서 플래그2 4번 비트 타입들의 합)을 본다.
        // 무너진 다리는 표에서 줄지 않으므로 (줄이는 곳은 전체 삭제 004c27c0 뿐) 놓은 칸의 누적 수로 판정한다.
        // 단계 A·B·C 의 기준(0 보다 큼·7 보다 큼·0x12 보다 큼)은 정밀 디컴파일(004c3a20)로 확인했다.
        int builtBridges = player.MadeWithFlags(BridgeFlag);
        switch (Stage)
        {
            case 'A':
                if (builtBridges > 0 || tickEvents.Any(e => e.Kind == SessionEventKind.ReturnedHome && e.Player == player.Number))
                {
                    Advance(session);
                }
                break;
            case 'B':
                if (builtBridges >= FirstBridgeTarget)
                {
                    Advance(session);
                }
                break;
            case 'C':
                if (builtBridges >= SecondBridgeTarget)
                {
                    Advance(session);
                }
                break;
            case 'D':
                if (_lastReachVersion != session.Bridges.Version)
                {
                    _lastReachVersion = session.Bridges.Version;
                    _lastReachable = session.CanReachAnyGeyser(player.Number);
                }
                if (_lastReachable)
                {
                    Advance(session);
                }
                break;
            case 'E':
                if (player.StormPower >= FirstCrystalStormPowerTarget)
                {
                    Advance(session);
                }
                break;
            case 'F':
                if (player.StormPower >= BridgeTheGapStormPowerTarget)
                {
                    Advance(session);
                }
                break;
        }
    }

    /// <summary>
    /// 단계 C(생산): 신전을 선택한 채 2초가 지나면 "NotVortex" 안내를 띄우고 선택을 푼다 (워크샾을 우클릭하라는 보정).
    /// 워크샾에 sunArcher 를 등록했으면 다음 단계.
    /// </summary>
    private void UpdateProduction(BattleSession session, PlayerState player)
    {
        if (session.IsVortexSelected(player))
        {
            if (!HasTimer(session))
            {
                StartTimer(session, NotVortexDelaySeconds);
            }
            else if (TimerExpired(session))
            {
                session.EmitTutorialTell(player.Number, NotVortexSection);
                session.ClearSelection(player);
            }
        }
        // 생산 창(덱)에 sunArcher 가 올라가 있으면 다음 단계
        if (player.Deck.Entries().Any(e => e.Kind != DeckEntryKind.Bridge && e.TypeName.Equals("sunArcher", StringComparison.OrdinalIgnoreCase)))
        {
            Advance(session);
        }
    }

    /// <summary>단계 F(에너지 표시): 신전을 선택하면 4초 타이머를 시작하고, 지나면 선택을 풀고 다음 단계로 간다.</summary>
    private void UpdateEnergyDisplay(BattleSession session, PlayerState player)
    {
        if (!HasTimer(session))
        {
            if (session.IsVortexSelected(player))
            {
                StartTimer(session, TempleViewDelaySeconds);
            }
        }
        else if (TimerExpired(session))
        {
            Advance(session);
            session.ClearSelection(player);
        }
    }

    /// <summary>
    /// 단계 H(회수): 회수를 허용하고 이번 화면/헤드리스 호출의 회수 성공 사건에서 2초 타이머를 시작한다.
    /// 원본의 현재 처리 중인 회수 명령 ID(<c>DAT_0054286c</c>) 전체 분배는 후속이며, 지금은 이 호출의 회수 사건으로 대신한다.
    /// </summary>
    private void UpdateSalvage(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        session.DenySalvage = false;
        if (!HasTimer(session))
        {
            if (tickEvents.Any(e => e.Kind == SessionEventKind.Salvaged && e.Player == session.HumanPlayer))
            {
                StartTimer(session, SalvageDelaySeconds);
            }
        }
        else if (TimerExpired(session))
        {
            Advance(session);
        }
    }

    /// <summary>현재 경로의 타이머가 있는지. 화면은 원본의 double 0 표지를 읽고 헤드리스는 기존 틱 표지를 읽는다.</summary>
    private bool HasTimer(BattleSession session) => session.TutorialUsesScreenFrames ? TimerSeconds != 0 : TimerTick != 0;

    /// <summary>원본의 현재 게임 시각 + double 간격을 저장한다. 헤드리스 틱 값은 호환/진단용으로 함께 기록한다.</summary>
    private void StartTimer(BattleSession session, double delaySeconds)
    {
        TimerSeconds = (session.TutorialUsesScreenFrames ? session.TutorialFrameSeconds : session.FrameSeconds) + delaySeconds;
        TimerTick = session.Tick + session.TicksFor(delaySeconds);
    }

    /// <summary>화면 타이머는 현재 절대 게임 시각의 포함 경계로 판정하며 24Hz 틱으로 올림하지 않는다.</summary>
    private bool TimerExpired(BattleSession session) => session.TutorialUsesScreenFrames
        ? TimerSeconds <= session.TutorialFrameSeconds : TimerTick <= session.Tick;

    /// <summary>원본 FUN_004c33f0의 정지/잠금 거부 뒤 글자를 높이고 임시 타이머를 지운다. 화면 안내는 예약 뒤 나중에 연다.</summary>
    private void Advance(BattleSession session)
    {
        if (session.TutorialFrameLocked || !session.ScheduleTutorialStage()) return;
        Stage++;
        TimerTick = 0;
        TimerSeconds = 0;
        if (!session.TutorialUsesScreenFrames) session.EmitTutorialTell(session.HumanPlayer, Section);
    }
}
