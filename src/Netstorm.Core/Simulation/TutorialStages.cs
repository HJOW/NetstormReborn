using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 튜토리얼의 단계 처리 (원본 Totalmade.cpp 의 튜토리얼별 함수: 1 = 004c3a20, <b>2 = 004c3bb0</b>, 3 = 004c3f00 …, 프레임마다 호출).
/// 원본은 단계 글자('A'부터)를 하나씩 올리며 단계마다 미션 스크립트의 "글자." 섹션을 안내 창으로 띄운다.
/// 단계 처리는 미션 머리 값이 정한 <b>시작 상태</b>를 실행 중에 바꾼다: 튜토리얼 2 는 단계 B 에서 sunFactory 를 허용하고
/// 단계 H 에서 회수 금지를 푼다 (docs/exe/mission-header-flags.md).
/// 세션의 명령과 이벤트만 보고 상태를 바꾸는 별도 객체이며, 세션이 틱마다 <see cref="Update"/> 를 부른다.
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

    /// <summary>튜토리얼 1의 다리 실습 첫 통과 기준(원본은 7보다 큰 칸 수).</summary>
    private const int FirstBridgeTarget = 8;

    /// <summary>튜토리얼 1의 다리 회전 실습 통과 기준(원본은 18보다 큰 칸 수).</summary>
    private const int SecondBridgeTarget = 19;

    /// <summary>튜토리얼 1의 최종 Storm Power 목표(미션 스크립트 F.).</summary>
    private const int BridgeTheGapStormPowerTarget = 600;

    /// <summary>튜토리얼 1에서 결정 하나를 전달한 뒤 넘어가는 Storm Power 기준(원본 0x510298).</summary>
    private const int FirstCrystalStormPowerTarget = 200;

    /// <summary>단계 C: 템플을 선택한 채 이 시간(초)이 지나면 보정 안내를 띄운다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double NotVortexDelaySeconds = 2.0;

    /// <summary>단계 F: 템플을 선택하고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506588 의 double 값 4.0)</summary>
    private const double TempleViewDelaySeconds = 4.0;

    /// <summary>단계 H: 회수 명령을 내리고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double SalvageDelaySeconds = 2.0;

    /// <summary>템플 타입의 플래그2 비트 (typeflags "vortex" = 0x200)</summary>
    private static readonly uint VortexFlag = TypeFlagBits.Flag2Words["vortex"];

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

    /// <summary>단계 안의 타이머가 끝나는 틱 (0 이면 타이머 없음 — 원본은 double 0.0 으로 "없음"을 나타낸다)</summary>
    public long TimerTick { get; private set; }

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
    /// 틱 하나의 단계 처리 (원본은 프레임마다). 한 틱에 단계는 최대 하나만 넘어간다.
    /// 원본은 단계를 넘긴 뒤 열 번 세는 동안 다음 단계 처리를 멈추는데(Totalmade 프레임 함수 004c34c0 의 +0x84 카운터,
    /// 안내 창이 뜨고 닫힐 때까지의 잠금으로 보인다), 여기서는 창이 없으므로 다음 틱부터 검사하는 것으로 근사한다.
    /// </summary>
    /// <param name="session">세션</param>
    /// <param name="tickEvents">이번 틱에 일어난 이벤트 (단계 H 가 회수 명령을 알아본다)</param>
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
                // 템플(typeflags vortex)을 지은 개수가 0 보다 크면 다음 단계
                if (player.MadeWithFlags(VortexFlag) > 0)
                {
                    Advance(session);
                }
                break;
            case 'B':
                // 표에서 Sun Workshop(sunFactory)을 허용으로 바꾼다 (원본 FUN_004c23e0(sunFactory, 허용))
                player.Tech.Set("sunFactory", true);
                // 워크샵(typeflags factory)을 지은 개수가 0 보다 크면 다음 단계 (선택을 푼다)
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

    /// <summary>튜토리얼 1의 F4·다리 칸·가이저 연결·결정 전달·600 SP 단계.</summary>
    private void UpdateBridgeTheGap(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        PlayerState player = session.Player(session.HumanPlayer);
        int placedBridges = session.Bridges.Cells.Count(cell => cell.Owner == player.Number);
        switch (Stage)
        {
            case 'A':
                if (placedBridges > 0 || tickEvents.Any(e => e.Kind == SessionEventKind.ReturnedHome && e.Player == player.Number))
                {
                    Advance(session);
                }
                break;
            case 'B':
                if (placedBridges >= FirstBridgeTarget)
                {
                    Advance(session);
                }
                break;
            case 'C':
                if (placedBridges >= SecondBridgeTarget)
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
    /// 단계 C(생산): 템플을 선택한 채 2초가 지나면 "NotVortex" 안내를 띄우고 선택을 푼다 (워크샵을 우클릭하라는 보정).
    /// 워크샵에 sunArcher 를 등록했으면 다음 단계.
    /// </summary>
    private void UpdateProduction(BattleSession session, PlayerState player)
    {
        if (session.IsVortexSelected(player))
        {
            if (TimerTick == 0)
            {
                TimerTick = session.Tick + session.TicksFor(NotVortexDelaySeconds);
            }
            else if (TimerTick <= session.Tick)
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

    /// <summary>단계 F(에너지 표시): 템플을 선택하면 4초 타이머를 시작하고, 지나면 선택을 풀고 다음 단계로 간다.</summary>
    private void UpdateEnergyDisplay(BattleSession session, PlayerState player)
    {
        if (TimerTick == 0)
        {
            if (session.IsVortexSelected(player))
            {
                TimerTick = session.Tick + session.TicksFor(TempleViewDelaySeconds);
            }
        }
        else if (TimerTick <= session.Tick)
        {
            Advance(session);
            session.ClearSelection(player);
        }
    }

    /// <summary>
    /// 단계 H(회수): 회수 금지를 푼다(프레임마다). 회수 명령이 이번 틱에 받아들여지면 2초 타이머를 시작하고, 지나면 다음 단계로 간다.
    /// 원본은 현재 처리 중인 명령이 회수 명령인지 본다 (<c>DAT_0054286c</c>) — 여기서는 이번 틱의 회수 이벤트로 대신한다.
    /// </summary>
    private void UpdateSalvage(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        session.DenySalvage = false;
        if (TimerTick == 0)
        {
            if (tickEvents.Any(e => e.Kind == SessionEventKind.Salvaged && e.Player == session.HumanPlayer))
            {
                TimerTick = session.Tick + session.TicksFor(SalvageDelaySeconds);
            }
        }
        else if (TimerTick <= session.Tick)
        {
            Advance(session);
        }
    }

    /// <summary>다음 단계로 넘기고 그 단계의 스크립트 섹션을 알린다 (원본 FUN_004c33f0: 글자 +1, 타이머 지움)</summary>
    private void Advance(BattleSession session)
    {
        Stage++;
        TimerTick = 0;
        session.EmitTutorialTell(session.HumanPlayer, Section);
    }
}
