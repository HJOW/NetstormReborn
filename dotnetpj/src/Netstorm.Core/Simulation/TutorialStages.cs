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

    /// <summary>튜토리얼 1 의 F4(섬으로 복귀) 명령 토큰. 단계 A 가 현재 사건의 하위 26비트와 비교한다(원본 005429ac 에 해당하는 클론의 표식).</summary>
    private const uint HomeCommandToken = 0x00730000;

    /// <summary>튜토리얼 1 의 회수 명령 토큰(원본 0054271c 에 해당하는 클론의 표식). 클론에는 이 토큰을 보내는 사건이 없다.</summary>
    private const uint ReclaimCommandToken = 0x00123456;

    /// <summary>튜토리얼 1 판정이 "최근 생성한 간헐천" 목록에 담는 최대 개수(원본 00595094/98).</summary>
    private const int RecentGeyserLimit = 5;

    /// <summary>단계 C: 신전을 선택한 채 이 시간(초)이 지나면 보정 안내를 띄운다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double NotVortexDelaySeconds = 2.0;

    /// <summary>단계 F: 신전을 선택하고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506588 의 double 값 4.0)</summary>
    private const double TempleViewDelaySeconds = 4.0;

    /// <summary>단계 H: 회수 명령을 내리고 이 시간(초)이 지나면 다음 단계로 간다 (exe 0x506590 의 double 값 2.0)</summary>
    private const double SalvageDelaySeconds = 2.0;

    /// <summary>신전 타입의 플래그2 비트 (typeflags "vortex" = 0x200)</summary>
    private static readonly uint VortexFlag = TypeFlagBits.Flag2Words["vortex"];

    /// <summary>단계 C 보정 안내 섹션 이름 (스크립트 [NotVortex])</summary>
    public const string NotVortexSection = "NotVortex";

    /// <summary>튜토리얼 번호 (미션 머리 tutorialNumber)</summary>
    public int Number { get; }

    /// <summary>헤드리스 합성 진행이 쓰는 단계 글자 저장소. 화면 게이트에 연결되면 게이트 상태의 값을 대신 읽는다.</summary>
    private char _stage = FirstStage;

    /// <summary>화면 게이트와 공유하는 필드 저장소(연결 전 null). 연결되면 단계 글자와 임시 타이머를 이 저장소 하나가 소유한다.</summary>
    private MissionFrameGateState? _shared;

    /// <summary>
    /// 지금 단계 글자 (원본 DAT_005ca8e8). 화면 게이트에 연결되면 공유 상태의 값(저위 8비트)이며,
    /// 단계 증가는 게이트의 004c33f0 구현이 한다.
    /// </summary>
    public char Stage => _shared != null ? (char)unchecked((byte)_shared.Stage) : _stage;

    /// <summary>지금 단계의 스크립트 섹션 이름 ("A." 형식, 원본 FUN_004c2a90)</summary>
    public string Section => SectionOf(Stage);

    /// <summary>마지막 단계에 도달했는지</summary>
    public bool Finished => Stage == (Number == BridgeTheGap ? BridgeTheGapLastStage : SecretWorkshopLastStage);

    /// <summary>튜토리얼 1 의 원본 단계 판정기(004c3a20 이식). 세션에 처음 닿을 때 만든다.</summary>
    private TutorialOne? _bridgeJudge;

    /// <summary>판정기가 읽는 최근 생성 간헐천 목록. 판정 직전에 세션의 간헐천으로 채운다.</summary>
    private readonly RecentGeyserList _recentGeysers = new();

    /// <summary>같은 판정 안에서 이미 단계를 올렸는지. 원본은 Advance 가 잠금 10프레임을 걸어 같은 판정의 추가 호출을 거부한다.</summary>
    private bool _advancedInJudgement;

    /// <summary>간헐천 도달 가능 여부 캐시의 다리 격자 버전. 버전이 바뀌면 다시 계산한다.</summary>
    private int _reachVersion = -1;

    /// <summary>간헐천 번호 → 현재 사제가 걸어서 닿을 수 있는지(다리 격자 버전별 캐시).</summary>
    private readonly Dictionary<int, bool> _reachable = [];

    /// <summary>헤드리스 합성 진행의 기존 타이머 틱과 진단값. 실제 화면 판정에서는 TimerSeconds를 사용한다.</summary>
    public long TimerTick { get; private set; }

    /// <summary>헤드리스 합성 진행이 쓰는 타이머 저장소. 화면 게이트에 연결되면 공유 상태의 +98을 대신 읽고 쓴다.</summary>
    private double _timerSeconds;

    /// <summary>
    /// 실제 화면에서 사용하는 원본 +98 double 게임 시각 타이머. 0이면 없음이며 단계가 바뀔 때 지운다.
    /// 화면 게이트에 연결되면 게이트 상태와 같은 값을 공유한다(단계 증가가 0으로 지운다).
    /// </summary>
    public double TimerSeconds
    {
        get => _shared?.TimerSeconds ?? _timerSeconds;
        private set
        {
            if (_shared != null) _shared.TimerSeconds = value;
            else _timerSeconds = value;
        }
    }

    /// <summary>
    /// 화면의 공통 프레임 게이트와 단계·타이머 저장소를 하나로 합친다. 지금까지의 단계 글자와 타이머를 공유 상태로 옮기고
    /// 이후에는 공유 상태만 읽고 쓴다. 튜토리얼 번호(1~6 분배)도 함께 기록한다. 세션이 화면에 연결될 때 한 번 호출한다.
    /// </summary>
    /// <param name="state">게이트가 소유하는 원본 필드 저장소.</param>
    /// <remarks>2026-10-15 감사 D02: 원본은 단계·타이머를 게이트 상태가 한 번만 소유한다(docs/exe/cpp-mission-frame-gate-20261015.md).</remarks>
    internal void AttachGateState(MissionFrameGateState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        state.Stage = Stage;
        state.TimerSeconds = TimerSeconds;
        state.TutorialNumber = (uint)Number;
        _shared = state;
    }

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
                if (session.BuiltForGenus(VortexFlag) > 0)
                {
                    Advance(session);
                }
                break;
            case 'B':
                // 표에서 Sun Workshop(sunFactory)을 허용으로 바꾼다 (원본 FUN_004c23e0(sunFactory, 허용))
                player.Tech.Set("sunFactory", true);
                // 워크샾(typeflags factory)을 지은 개수가 0 보다 크면 다음 단계 (선택을 푼다)
                if (session.BuiltForGenus(TypeFlagBits.Factory) > 0)
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
                if (session.BuiltCount("sunArcher") > 0)
                {
                    Advance(session);
                }
                break;
            case 'E':
                // 두 번째를 놓았으면 다음 단계 (선택을 푼다)
                if (session.BuiltCount("sunArcher") > 1)
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
                if (session.BuiltCount("sunArcher") > 3)
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

    /// <summary>
    /// 튜토리얼 1의 F4·다리 칸·간헐천 연결·결정 전달·600 SP 단계. 원본 004c3a20 의 판정 전체를 이식한 <see cref="TutorialOne"/>에 맡기고
    /// 이 클래스는 세션의 현재 상태(장부·간헐천·SP)를 경계로 연결한다.
    /// </summary>
    /// <remarks>
    /// 경계 중 근사: 표면 Graph 번호는 아직 세션에 없어 "현재 사제가 그 간헐천까지 걸어갈 수 있는가"로 대신한다(닿으면 신전과 같은 Graph).
    /// 간헐천 생성은 원본이 단계 C 에서 하지만 클론은 세션 생성 때 미리 만들어 두므로 생성 경계는 비어 있다(D05 후속).
    /// 2026-10-15 감사 D05/D14: 직접 비교하던 조건을 검증된 판정기와 Totalmade 장부로 교체.
    /// </remarks>
    /// <param name="session">세션</param>
    /// <param name="tickEvents">이번 화면 또는 헤드리스 호출의 명령 사건(F4 복귀를 알아본다)</param>
    private void UpdateBridgeTheGap(BattleSession session, IReadOnlyList<SessionEvent> tickEvents)
    {
        PlayerState player = session.Player(session.HumanPlayer);
        _bridgeJudge ??= CreateBridgeJudge(session);
        _advancedInJudgement = false;
        // 최근 생성 간헐천 목록(번호 순 마지막 5개)을 판정 직전의 현재 세션으로 채운다.
        _recentGeysers.Entries.Clear();
        _recentGeysers.Entries.AddRange(session.Entities.Where(e => e.Kind == ObjectKind.Geyser).OrderBy(e => e.Id)
            .TakeLast(RecentGeyserLimit).Select(e => (uint)e.Id));
        _recentGeysers.Count = (uint)_recentGeysers.Entries.Count;
        // 이번 호출에 이 플레이어의 F4 복귀 사건이 있으면 현재 사건을 F4 명령 토큰으로 넘긴다.
        uint currentEvent = tickEvents.Any(e => e.Kind == SessionEventKind.ReturnedHome && e.Player == player.Number) ? HomeCommandToken : 0;
        _bridgeJudge.Handle(currentEvent);
    }

    /// <summary>튜토리얼 1 판정기를 세션의 현재 상태에 연결해 만든다.</summary>
    private TutorialOne CreateBridgeJudge(BattleSession session) => new(
        new TutorialOneState
        {
            Stage = () => Stage,
            LocalPlayer = (uint)session.HumanPlayer,
            // 클론은 항상 Storm Power 를 알고 있다(원본 SP 조회 허용 전역 00595344 가 켜진 상태).
            BattleShown = 1,
            HomeCommand = HomeCommandToken,
            ReclaimCommand = ReclaimCommandToken,
        },
        _recentGeysers,
        new TutorialOneHooks
        {
            BuiltForGenus = session.BuiltForGenus,
            GraphFor = sid => GraphNumber(session, sid),
            Money = owner => session.Player((int)owner).StormPower,
            Advance = () =>
            {
                // 원본 Advance 는 10프레임 잠금을 걸어 같은 판정의 추가 호출을 거부한다. 헤드리스 경로에도 같은 효과를 준다.
                if (_advancedInJudgement) return;
                if (TryAdvance(session)) _advancedInJudgement = true;
            },
            // 간헐천은 세션 생성 때 이미 만들어 두었다(원본은 단계 C 에서 생성). D05 후속에서 원본 생성 위치로 옮긴다.
            SpawnGeysers = () => { },
            TempleOf = owner => session.Entities.FirstOrDefault(e => e.Kind == ObjectKind.Temple && e.Owner == (int)owner)?.Id is int id ? (uint)id : 0,
        });

    /// <summary>
    /// 표면 Graph 번호의 근사: 사제가 걸어서 닿을 수 있는 간헐천은 신전과 같은 번호(0), 닿을 수 없는 간헐천은 자기 번호로 구분한다.
    /// 다리 격자 버전이 같은 동안은 간헐천마다 결과를 재사용한다. 신전(또는 목록 밖 번호)은 기준 번호 0이다.
    /// </summary>
    /// <param name="session">세션</param>
    /// <param name="sid">신전 또는 간헐천의 객체 번호</param>
    private uint GraphNumber(BattleSession session, uint sid)
    {
        GameEntity? entity = session.Entity((int)sid);
        if (entity == null || entity.Kind != ObjectKind.Geyser) return 0;
        if (_reachVersion != session.Bridges.Version)
        {
            _reachVersion = session.Bridges.Version;
            _reachable.Clear();
        }
        if (!_reachable.TryGetValue(entity.Id, out bool reachable))
        {
            reachable = session.CanReachGeyser(session.HumanPlayer, entity);
            _reachable[entity.Id] = reachable;
        }
        return reachable ? 0u : 1u + sid;
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

    /// <summary>
    /// 원본 FUN_004c33f0의 정지/잠금 거부 뒤 글자를 높이고 임시 타이머를 지운다. 화면 안내는 예약 뒤 나중에 연다.
    /// 화면에 연결되면 게이트가 글자 증가·10프레임 잠금·안내 기한·타이머 0을 한 번에 처리하므로 여기서는 헤드리스 틱 표지만 지운다.
    /// </summary>
    /// <remarks>2026-10-15: 화면 연결 시 캐시된 TutorialFrameLocked 대신 게이트의 현재 정지/잠금 판정을 쓴다.</remarks>
    private void Advance(BattleSession session) => TryAdvance(session);

    /// <summary><see cref="Advance"/>와 같고 단계를 올렸는지(정지/잠금으로 거부되지 않았는지) 돌려준다.</summary>
    /// <param name="session">세션</param>
    private bool TryAdvance(BattleSession session)
    {
        if (!session.TryAdvanceTutorialStage()) return false;
        if (_shared == null) _stage++;
        TimerTick = 0;
        TimerSeconds = 0;
        if (!session.TutorialUsesScreenFrames) session.EmitTutorialTell(session.HumanPlayer, Section);
        return true;
    }
}
