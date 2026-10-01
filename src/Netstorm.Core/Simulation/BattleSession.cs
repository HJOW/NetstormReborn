using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 전투 한 판의 게임 세션: 규칙 코어(BattleMap·BridgeGrid·생산 창·다리 칸)를 고정 틱 루프로 묶는다.
/// <list type="bullet">
/// <item><description>상태는 <see cref="GameCommand"/> 로만 바뀐다. 명령은 다음 틱 처음에 넣은 순서대로 실행되므로
/// 같은 명령열이면 어느 컴퓨터에서나 같은 결과가 나온다 (<see cref="Checksum"/> 으로 확인).</description></item>
/// <item><description>시간은 정수 틱이다. 게임 시각(초) = 틱 ÷ 초당 틱 수. 다리 조각 채우기(1초)·붕괴(10초)·건설·재충전은 모두 틱으로 센다.</description></item>
/// <item><description>화면은 <see cref="Advance"/> 에 흐른 실제 시간을 주고, <see cref="DrainEvents"/> 로 일어난 일을 받아 알림을 띄운다.</description></item>
/// </list>
/// 수집·수송·희생 의식·포대·공중 공격체와 캠페인 1-1의 임시 방어 AI를 처리한다. 원본 전략 복원은 후속이다.
/// </summary>
public sealed partial class BattleSession
{
    /// <summary>플레이어 번호 → 상태 (번호 순 순회로 결과를 일정하게 한다)</summary>
    private readonly SortedDictionary<int, PlayerState> _players = [];

    /// <summary>오브젝트 번호 → 오브젝트 (번호 순)</summary>
    private readonly SortedDictionary<int, GameEntity> _entities = [];

    /// <summary>다음 틱 처음에 실행할 명령 (넣은 순서)</summary>
    private readonly Queue<GameCommand> _commands = new();

    /// <summary>아직 화면이 받아가지 않은 이벤트</summary>
    private readonly List<SessionEvent> _events = [];

    /// <summary>타입 이름 조회</summary>
    private readonly TypeCatalog _types;

    /// <summary>실제 시간 → 틱 변환</summary>
    private readonly FixedTimestep _timestep = new();

    /// <summary>게임 전역 난수 (다리 조각 추첨 등 원본과 같은 식, 원본은 하나를 여러 곳이 함께 쓴다)</summary>
    private readonly NetstormRandom _random;

    /// <summary>다리 연결망이 닿는 섬 계산 결과 캐시</summary>
    private IReadOnlyList<BridgeNetworkReach> _reaches = [];

    /// <summary>캐시를 계산한 때의 다리 격자 버전 (-1 이면 아직 계산 전)</summary>
    private int _reachVersion = -1;

    /// <summary>맵에 저장된 다리 오브젝트 → 격자 칸</summary>
    private readonly IReadOnlyDictionary<FortMapObject, BridgeCellState> _storedBridges;

    /// <summary>맵에 저장되어 있던 오브젝트 중 엔티티가 된 것 → 오브젝트 번호 (저장 다리는 엔티티가 아니라 제외)</summary>
    private readonly Dictionary<FortMapObject, int> _initialEntityIds = [];

    /// <summary>맵에 저장된 다리의 격자 칸 (칸 상태가 바뀌어도 같은 칸으로 찾도록 참조로 비교한다)</summary>
    public IReadOnlySet<BridgeCellState> StoredBridgeCells { get; }

    /// <summary>섬·영역 규칙 상태 (소유권·공급원·점유 칸·전투 옵션)</summary>
    public BattleMap Map { get; }

    /// <summary>놓인 다리 칸 (저장 다리 포함)</summary>
    public BridgeGrid Bridges { get; }

    /// <summary>미션 시작 조건 (미션 없이 맵만 연 세션은 null)</summary>
    public MissionStart? Mission { get; }

    /// <summary>미션 시작 조건(myStartMoney·myTech·techAllowed)이 적용되는 사람 플레이어 번호</summary>
    public int HumanPlayer { get; }

    /// <summary>
    /// 생산 규칙(기술 허용 표·덱 등록·배치 뒤 재충전·회수 금지)을 명령에 적용할지. 끄면 어떤 유닛이든 규칙 조건
    /// (섬·자리·비용·에너지)만 맞으면 놓을 수 있다 (맵 뷰어의 배치 시험용). 기본은 켜짐.
    /// 기술 허용 표(<see cref="PlayerState.Tech"/>)와 <see cref="DenySalvage"/> 는 미션 머리 값에서 시작하는 **변하는 상태**다.
    /// 원본은 튜토리얼 단계 처리가 실행 중에 이 값을 바꾼다 (docs/exe/mission-header-flags.md). 그 처리는 <see cref="Tutorial"/> 이
    /// 맡는다 (구현된 튜토리얼만). 단계 처리가 없는 미션에서는 호출하는 쪽이 필요한 때에 <c>Tech.Set</c>·<see cref="DenySalvage"/> 를 바꿔 준다.
    /// </summary>
    public bool EnforceProductionRules { get; set; } = true;

    /// <summary>
    /// 튜토리얼 단계 처리 (미션이 구현된 튜토리얼이면, 아니면 null). 세션을 만들 때 첫 단계 안내("A.")가 이벤트로 나온다.
    /// </summary>
    public TutorialStages? Tutorial { get; }

    /// <summary>튜토리얼 단계 처리를 틱마다 실행할지 (기본 켜짐). 끄면 단계는 그대로 멈춰 있다 (맵 뷰어의 규칙 시험용).</summary>
    public bool RunsTutorial { get; set; } = true;

    /// <summary>
    /// 회수 금지 (원본 전역 DAT_00595078). 미션 머리 denySalvage 로 시작하고, 켜져 있으면 회수 명령은 실행되지 않고
    /// 미션 스크립트의 "DenySalvage" 섹션을 알리는 것으로 끝난다 (Salvage 실행 함수 0044c420 · 메뉴 항목 0044ca00).
    /// 튜토리얼 2 는 1 로 시작해 단계 H 에서 0 으로 바꾼다.
    /// </summary>
    public bool DenySalvage { get; set; }

    /// <summary>
    /// 개발용: 빈 섬이 내 섬과 다리로 연결된 것으로 가정한다. 다리 연결 판정(BridgeReach)이 원본 규칙을 다 담지 못해서
    /// 연결 조건을 건너뛰고 싶을 때 쓴다.
    /// </summary>
    public bool AssumeConnected { get; set; }

    /// <summary>지금까지 진행한 틱 수</summary>
    public long Tick { get; private set; }

    /// <summary>초당 틱 수</summary>
    public int TicksPerSecond => _timestep.TicksPerSecond;

    /// <summary>게임 시각(초). 틱 ÷ 초당 틱 수로 정수에서 곧바로 계산해 오차가 쌓이지 않는다.</summary>
    public double Seconds => Tick / (double)TicksPerSecond;

    /// <summary>세션의 모든 플레이어 (번호 순)</summary>
    public IReadOnlyCollection<PlayerState> Players => _players.Values;

    /// <summary>세션의 모든 오브젝트 (번호 순, 맵에 저장된 것과 게임 중 만든 것)</summary>
    public IReadOnlyCollection<GameEntity> Entities => _entities.Values;

    /// <summary>
    /// 세션을 시작한다. 맵 오브젝트로 엔티티·플레이어 상태를 채우고, 미션이 있으면 사람 플레이어에게 시작 Storm Power·지식·기술 허용을 적용한다.
    /// 다리 격자와 규칙 상태는 <see cref="BattleSessionFactory"/> 가 맵에서 만들어 준다.
    /// </summary>
    /// <param name="map">섬·공급원·점유 규칙 상태 (전투 옵션 포함)</param>
    /// <param name="bridges">다리 칸 격자 (저장된 다리가 이미 들어 있어야 한다)</param>
    /// <param name="types">타입 목록 (명령의 타입 이름 조회)</param>
    /// <param name="mission">미션 시작 조건 (없으면 null)</param>
    /// <param name="humanPlayer">사람 플레이어 번호</param>
    /// <param name="startStormPower">미션 값이 없을 때 쓸 시작 Storm Power (없으면 전투 옵션의 시작 금액)</param>
    /// <param name="seed">전역 난수 시드</param>
    /// <param name="storedBridges">맵에 저장된 다리 오브젝트 → 격자 칸 (화면이 무너진 저장 다리를 숨기는 데 쓴다, 없으면 빈 표)</param>
    public BattleSession(BattleMap map, BridgeGrid bridges, TypeCatalog types, MissionStart? mission = null,
        int humanPlayer = 1, int? startStormPower = null, uint seed = 0,
        IReadOnlyDictionary<FortMapObject, BridgeCellState>? storedBridges = null)
    {
        Map = map;
        Bridges = bridges;
        _storedBridges = storedBridges ?? new Dictionary<FortMapObject, BridgeCellState>();
        StoredBridgeCells = new HashSet<BridgeCellState>(_storedBridges.Values, ReferenceEqualityComparer.Instance);
        _types = types;
        Mission = mission;
        HumanPlayer = humanPlayer;
        DenySalvage = mission?.DenySalvage ?? false;
        _random = new NetstormRandom(seed);
        if (mission?.TutorialNumber is int tutorialNumber && TutorialStages.IsSupported(tutorialNumber))
        {
            Tutorial = new TutorialStages(tutorialNumber);
        }

        // 플레이어는 맵 오브젝트에 나오는 소유자와 사람 플레이어다 (소유자 0 은 중립)
        var numbers = new SortedSet<int> { humanPlayer };
        // 저장 오브젝트의 소유자 번호를 모은다
        foreach ((int _, FortMapObject item) in map.InitialObjects)
        {
            if (item.Object.Owner is int owner and > 0)
            {
                numbers.Add(owner);
            }
        }
        // 플레이어마다 시작 상태를 만든다 (사람 플레이어에게만 미션 시작 조건을 적용)
        foreach (int number in numbers)
        {
            _players.Add(number, CreatePlayer(number, startStormPower));
        }

        // 맵에 저장된 오브젝트를 엔티티로 만들고, 템플·워크샵은 소유자의 생산 창에 등록한다 (저장 다리는 다리 격자가 다룬다)
        foreach ((int id, FortMapObject item) in map.InitialObjects)
        {
            ObjectKind kind = ObjectKinds.Of(item.Object.Type);
            if (kind == ObjectKind.Bridge)
            {
                continue;
            }
            Footprint footprint = Footprint.ForType(item.Object.Type.Definition, item.X, item.Y);
            int owner = item.Object.Owner ?? 0;
            var entity = new GameEntity(id, item.Object.Type, kind, owner, footprint, map.TerritoryAt(item.X, item.Y), item);
            _entities.Add(id, entity);
            _initialEntityIds[item] = id;
            if (owner > 0 && _players.TryGetValue(owner, out PlayerState? player))
            {
                RegisterCompletedBuilding(player, entity);
            }
        }
        // 튜토리얼은 첫 단계 안내로 시작한다 (원본은 시작하며 단계 'A' 의 섹션을 알린다)
        if (Tutorial != null)
        {
            EmitTutorialTell(humanPlayer, Tutorial.Section);
        }
        InitializeCampaignAi();
    }

    /// <summary>플레이어 시작 상태를 만든다: Storm Power, 다리 칸, 사람 플레이어는 미션의 시작 지식과 기술 허용</summary>
    private PlayerState CreatePlayer(int number, int? startStormPower)
    {
        BattleOptions options = Map.Options;
        bool human = number == HumanPlayer;
        int stormPower = human && Mission != null ? Mission.StormPower(options) : startStormPower ?? options.StartingStormPower;
        // 표는 실행 중에 바뀌므로 세션마다 복사본을 쓴다 (같은 미션으로 만든 다른 세션에 영향을 주지 않는다)
        TechPermissions tech = human && Mission != null ? Mission.Tech.Clone() : TechPermissions.Parse(null);
        var player = new PlayerState(number, stormPower, new BridgeTray(options.BridgeSlotCount, _random), tech);
        if (!human && Mission is { IsFirstCampaign: true })
        {
            player.StormPower = Mission.AiStartMoney ?? stormPower;
            // 초기 AI 지식은 원본 미션 머리 값만 사용한다.
            foreach (string name in Mission.AiKnowledge) player.Deck.LearnKnowledge(name);
        }
        if (human && Mission != null)
        {
            Mission.ApplyKnowledge(player.Deck);
        }
        return player;
    }

    /// <summary>플레이어 상태를 찾는다. 세션에 없는 번호면 예외.</summary>
    /// <param name="number">플레이어 번호</param>
    public PlayerState Player(int number) =>
        _players.TryGetValue(number, out PlayerState? player) ? player : throw new ArgumentException($"세션에 없는 플레이어입니다: {number}", nameof(number));

    /// <summary>오브젝트를 번호로 찾는다 (없으면 null)</summary>
    /// <param name="id">오브젝트 번호</param>
    public GameEntity? Entity(int id) => _entities.GetValueOrDefault(id);

    /// <summary>저장된 맵 오브젝트에 대응하는 현재 엔티티를 찾는다 (사제의 이동 위치 표시용).</summary>
    public GameEntity? EntityForInitial(FortMapObject item) =>
        _initialEntityIds.TryGetValue(item, out int id) ? Entity(id) : null;

    /// <summary>칸을 차지하는 오브젝트 (번호가 가장 큰 것, 없으면 null). 회수 대상을 커서로 고를 때 쓴다.</summary>
    /// <param name="x">칸 x</param>
    /// <param name="y">칸 y</param>
    public GameEntity? EntityAt(int x, int y) =>
        _entities.Values.Reverse().FirstOrDefault(e => e.Footprint.Contains(x, y));

    /// <summary>
    /// 맵에 저장되어 있던 (다리가 아닌) 오브젝트가 회수·파괴로 세션에서 사라졌는지 (화면이 그리지 않도록).
    /// 저장 다리는 엔티티가 아니라 항상 false 이며, 다리는 <see cref="IsStoredBridgeGone"/> 이 판정한다.
    /// </summary>
    /// <param name="item">맵 오브젝트</param>
    public bool IsInitialObjectRemoved(FortMapObject item) =>
        _initialEntityIds.TryGetValue(item, out int id) && !_entities.ContainsKey(id);

    /// <summary>맵에 저장되어 있던 다리 오브젝트가 붕괴로 격자에서 사라졌는지 (화면이 그리지 않도록). 저장 다리가 아니면 false.</summary>
    /// <param name="item">맵 오브젝트</param>
    public bool IsStoredBridgeGone(FortMapObject item) =>
        _storedBridges.TryGetValue(item, out BridgeCellState? state) && !ReferenceEquals(Bridges.At(state.X, state.Y), state);

    /// <summary>명령을 넣는다. 다음 틱 처음에 넣은 순서대로 실행된다.</summary>
    /// <param name="command">명령</param>
    public void Submit(GameCommand command) => _commands.Enqueue(command);

    /// <summary>
    /// 개발용: 다리 조각을 칸을 거치지 않고 곧바로 집은 상태로 만든다 (맵 뷰어의 <c>--bridge-hold</c> 검증 옵션).
    /// 명령 스트림을 거치지 않으므로 멀티플레이·리플레이에는 쓰지 않는다.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="pattern">조각 모양 번호</param>
    public void DebugHoldBridgePiece(int player, int pattern) => Player(player).HeldPiece = new BridgePiece(pattern);

    /// <summary>화면이 아직 받지 않은 이벤트를 모두 돌려주고 비운다.</summary>
    public IReadOnlyList<SessionEvent> DrainEvents()
    {
        SessionEvent[] events = [.. _events];
        _events.Clear();
        return events;
    }

    /// <summary>
    /// 흐른 실제 시간을 더하고 그만큼 틱을 진행한다. 한 번에 따라잡는 틱 수에는 한도가 있어(FixedTimestep) 창이 멈췄다 풀려도
    /// 게임이 한꺼번에 튀지 않는다.
    /// </summary>
    /// <param name="elapsedSeconds">지난 화면 갱신 이후 시간(초)</param>
    /// <returns>이번에 진행한 틱 수</returns>
    public int Advance(double elapsedSeconds)
    {
        int ticks = _timestep.Advance(elapsedSeconds);
        RunTicks(ticks);
        return ticks;
    }

    /// <summary>실제 시간과 상관없이 틱을 정확히 그 수만큼 진행한다 (테스트·되감기 없는 앞당기기용).</summary>
    /// <param name="count">진행할 틱 수</param>
    public void RunTicks(int count)
    {
        // 틱을 하나씩 진행한다
        for (int i = 0; i < count; i++)
        {
            Step();
        }
    }

    /// <summary>
    /// 틱 하나를 진행한다. 순서: 명령 실행 → 사제 수집·이동 → 수송·사제 이동(포획·운반) → 건설 완료 → 전투 → 희생 의식 → 튜토리얼 단계 처리
    /// → 플레이어별 다리 칸 채우기 → 다리 붕괴 → 지상 낙하·허공 사제 복귀 → 미션 승패 이벤트.
    /// 이 순서가 바뀌면 같은 명령열의 결과가 달라지므로 락스텝·리플레이를 위해 고정한다.
    /// </summary>
    private void Step()
    {
        Tick++;
        double now = Seconds;
        ExecuteQueuedCommands();
        UpdateCampaignAi();
        UpdateHarvests();
        UpdateUnitMoves();
        CompleteConstructions();
        UpdateCombat();
        UpdateSacrifices();
        if (Tutorial != null && RunsTutorial)
        {
            // 이번 틱에 일어난 이벤트(명령 결과·완공)를 보고 단계를 처리한다
            Tutorial.Update(this, [.. _events.Where(e => e.Tick == Tick)]);
        }
        // 플레이어 번호 순으로 다리 칸을 채운다 (같은 난수를 번호 순서대로 쓰도록)
        foreach (PlayerState player in _players.Values)
        {
            BridgePiece? added = player.Tray.Update(now, player.HasTemple);
            if (added != null)
            {
                Emit(SessionEventKind.BridgePieceAdded, player.Number, 0, $"새 다리 조각 {added.Pattern.Index}번");
            }
        }
        BridgeDecayResult decay = Bridges.Update(now);
        UpdateGroundSupport();
        // 다리 붕괴까지 반영한 뒤 승패·AI 이벤트를 판정한다 (원본은 미션 객체 프레임 함수에서 매 프레임 검사)
        UpdateMissionEvents();
        // 금 간 칸을 이벤트로 알린다
        foreach (BridgeCellState cell in decay.Cracked)
        {
            Emit(SessionEventKind.BridgeCracked, cell.Owner, 0, $"다리 ({cell.X}, {cell.Y}) 금 감");
        }
        // 무너진 칸을 이벤트로 알린다
        foreach (BridgeCellState cell in decay.Removed)
        {
            Emit(SessionEventKind.BridgeCollapsed, cell.Owner, 0, $"다리 ({cell.X}, {cell.Y}) 무너짐");
        }
    }

    /// <summary>이벤트 하나를 기록한다.</summary>
    private void Emit(SessionEventKind kind, int player, int entityId, string text, CommandFailure failure = CommandFailure.None) =>
        _events.Add(new SessionEvent(Tick, kind, player, entityId, text, failure));

    /// <summary>쌓인 명령을 넣은 순서대로 실행한다. 거부된 명령은 이벤트로 알린다.</summary>
    private void ExecuteQueuedCommands()
    {
        // 큐가 빌 때까지 명령을 하나씩 꺼내 실행한다
        while (_commands.Count > 0)
        {
            GameCommand command = _commands.Dequeue();
            CommandResult result = Execute(command);
            if (!result.Accepted)
            {
                Emit(SessionEventKind.CommandRejected, command.Player, 0,
                    result.Detail.Length == 0 ? SessionText.Describe(result.Failure) : result.Detail, result.Failure);
            }
        }
    }

    /// <summary>건설 시간이 지난 건물을 완성한다 (오브젝트 번호 순).</summary>
    private void CompleteConstructions()
    {
        // 건설 중이고 완공 틱이 된 오브젝트만 완성 처리한다
        foreach (GameEntity entity in _entities.Values.Where(e => !e.IsComplete && e.CompleteTick <= Tick).ToArray())
        {
            entity.IsComplete = true;
            if (entity.Type.Definition.HasFlag("createsisland")) Bridges.InvalidateTerrain();
            if (_players.TryGetValue(entity.Owner, out PlayerState? player))
            {
                RegisterCompletedBuilding(player, entity);
                // 건물은 완공할 때 지은 수로 센다 (튜토리얼 단계 처리가 읽는다)
                player.RecordMade(entity.Type.Name, entity.Type.Flags2);
            }
            Emit(SessionEventKind.BuildingCompleted, entity.Owner, entity.Id, $"{entity.DisplayName} 완공");
        }
    }

    /// <summary>
    /// 완성된 건물의 규칙 효과를 적용한다. 템플: 섬 소유·에너지 공급·다리/골렘 공급 시작.
    /// 워크샵: 지식을 등록할 수 있게 됨 (재건한 워크샵은 등록이 빈 상태). 그 밖의 오브젝트는 효과 없음.
    /// 맵에서 처음부터 있던 건물은 소유권·공급원을 BattleMap 이 이미 등록했으므로 덱 등록만 중복 없이 한다.
    /// </summary>
    private void RegisterCompletedBuilding(PlayerState player, GameEntity entity)
    {
        Element? element = Elements.FromTheme(entity.Type.Definition.GetString("theme"));
        if (entity.Kind == ObjectKind.Temple)
        {
            player.Deck.SetTemple(entity.Id);
            if (entity.Source == null)
            {
                // 게임 중 새로 지은 템플만 여기서 섬 소유와 공급원을 등록한다
                if (entity.Territory is int territory)
                {
                    Map.Ownership.SetTemple(territory, entity.Owner);
                }
                if (element != null)
                {
                    Map.AddSource(new EnergySource(entity.Id, element.Value, entity.Footprint.CenterX, entity.Footprint.CenterY, entity.Owner));
                }
            }
        }
        else if (entity.Kind == ObjectKind.Workshop)
        {
            player.Deck.AddWorkshop(entity.Id, element ?? Element.Sun);
        }
    }

    /// <summary>초 단위 간격을 틱으로 바꾼다 (올림, 최소 1틱).</summary>
    /// <param name="seconds">간격(초)</param>
    internal long TicksFor(double seconds) => _timestep.TicksFor(seconds);

    /// <summary>튜토리얼 안내(스크립트 섹션 이름)를 이벤트로 알린다.</summary>
    /// <param name="player">플레이어</param>
    /// <param name="section">섹션 이름 ("B." 또는 "NotVortex")</param>
    internal void EmitTutorialTell(int player, string section) => Emit(SessionEventKind.TutorialTell, player, 0, section);

    /// <summary>플레이어가 지금 템플을 선택하고 있는지 (원본은 선택한 오브젝트 타입의 플래그2 에 vortex 비트가 있는지 본다)</summary>
    /// <param name="player">플레이어</param>
    internal bool IsVortexSelected(PlayerState player) =>
        Entity(player.SelectedEntityId) is { } selected && (selected.Type.Flags2 & TypeFlagBits.Flag2Words["vortex"]) != 0;

    /// <summary>선택을 푼다 (원본 FUN_004d5c60: 선택 번호를 0 으로)</summary>
    /// <param name="player">플레이어</param>
    internal void ClearSelection(PlayerState player) => player.SelectedEntityId = 0;

    /// <summary>
    /// 지금까지의 게임 상태를 요약한 32비트 검사합 (FNV-1a). 같은 명령열을 같은 틱에 실행한 두 세션은 같은 값을 가져야 하고,
    /// 다르면 동기화가 깨진 것이다 (멀티플레이·리플레이 검증용). 화면 전용 상태(집은 조각의 회전, 알림)는 포함하지 않는다.
    /// </summary>
    public uint Checksum()
    {
        var hash = new Fnv1a();
        hash.Add(Tick);
        hash.Add(_random.State);
        // 회수 금지는 명령의 결과를 바꾸는 상태이므로 검사합에 넣는다
        hash.Add(DenySalvage ? 1 : 0);
        // 튜토리얼 단계와 타이머도 상태다
        hash.Add(Tutorial?.Stage ?? '\0');
        hash.Add(Tutorial?.TimerTick ?? 0);
        // 플레이어 상태: Storm Power, 다리 칸, 집은 조각, 재충전 예약(이름 순)
        foreach (PlayerState player in _players.Values)
        {
            hash.Add(player.Number);
            hash.Add(player.StormPower);
            hash.Add(player.Tray.DrawCount);
            // 칸에 있는 조각의 모양을 순서대로 섞는다
            foreach (BridgePiece piece in player.Tray.Pieces)
            {
                hash.Add(piece.Pattern.Index);
                // 품질 타이머는 놓인 다리의 시작 상태를 정하므로 상태다
                hash.Add(piece.CuredAtDeciseconds);
            }
            hash.Add(player.HeldPiece?.Pattern.Index ?? -1);
            hash.Add(player.HeldPiece?.CuredAtDeciseconds ?? -1);
            hash.Add(player.SelectedEntityId);
            hash.Add(player.Deck.TempleId ?? 0);
            // 지식·등록·워크샵 단계도 다음 생산 명령의 결과를 바꾼다.
            foreach (string knowledge in player.Deck.Knowledge.Order(StringComparer.OrdinalIgnoreCase)) hash.Add(knowledge.ToLowerInvariant());
            // 워크샵 번호와 등록 순서대로 생산 상태를 검사합에 넣는다.
            foreach (var workshop in player.Deck.WorkshopSnapshot())
            {
                hash.Add(workshop.Id); hash.Add((int)workshop.Element); hash.Add(workshop.Level);
                // 같은 지식이어도 등록한 공급 건물과 순서가 다르면 생산 상태가 다르다.
                foreach (string type in workshop.Registered) hash.Add(type.ToLowerInvariant());
            }
            // 지은 수(누적)는 튜토리얼 단계를 정하므로 이름 순서로 섞는다
            foreach ((string name, int count) in player.MadeSnapshot())
            {
                hash.Add(name);
                hash.Add(count);
            }
            // 기술 허용 표도 명령의 결과를 바꾸므로 기본값과 개별 값을 이름 순서로 섞는다
            hash.Add(player.Tech.DefaultAllowed ? 1 : 0);
            foreach ((string name, bool allowed) in player.Tech.Snapshot())
            {
                hash.Add(name);
                hash.Add(allowed ? 1 : 0);
            }
            // 재충전 예약을 이름 순서로 섞는다 (Dictionary 순회 순서에 의존하지 않는다)
            foreach ((string name, long ready) in player.UnitReadyTick.OrderBy(pair => pair.Key, StringComparer.OrdinalIgnoreCase))
            {
                hash.Add(name);
                hash.Add(ready);
            }
        }
        // 오브젝트: 번호·타입·소유자·자리·건설 상태
        foreach (GameEntity entity in _entities.Values)
        {
            hash.Add(entity.Id);
            hash.Add(entity.Type.LoadIndex);
            hash.Add(entity.Owner);
            hash.Add(entity.Footprint.AnchorX);
            hash.Add(entity.Footprint.AnchorY);
            hash.Add(entity.CarriedCrystals);
            hash.Add(entity.IsComplete ? 1 : 0);
            hash.Add(entity.CompleteTick);
            hash.Add(BitConverter.DoubleToInt64Bits(entity.HitPoints));
            hash.Add(entity.IsStunned ? 1 : 0);
            hash.Add(entity.IsSuspended ? 1 : 0);
            hash.Add((int)entity.Captivity);
            hash.Add(entity.CaptorId);
            hash.Add(entity.CarriedPriestId);
            hash.Add(entity.AttackTargetId);
            hash.Add(entity.NextAttackTick);
            AddFlightChecksum(hash, entity.Flight);
        }
        AddCombatChecksum(hash);
        AddSacrificeChecksum(hash);
        // 사제의 왕복 방향·예약 경로·남은 이동량도 다음 결과를 바꾸므로 검사합에 포함한다.
        foreach (PriestHarvestTask task in _harvestTasks.Values)
        {
            hash.Add(task.PriestId);
            hash.Add(task.GeyserId);
            hash.Add(task.TempleId);
            hash.Add((int)task.Phase);
            AddMovementChecksum(hash, task);
        }
        // 다리 칸: 좌표 순서로 모양·소유자·상태·수명
        foreach (BridgeCellState cell in Bridges.Cells.OrderBy(c => c.Y).ThenBy(c => c.X))
        {
            hash.Add(cell.X);
            hash.Add(cell.Y);
            hash.Add(cell.Cell.Letter);
            hash.Add(cell.Owner);
            hash.Add((int)cell.Condition);
            hash.Add(cell.TimeLeft);
        }
        return hash.Value;
    }

    /// <summary>FNV-1a 32비트 해시 누적기 (플랫폼과 상관없이 같은 값을 내기 위해 직접 구현)</summary>
    private sealed class Fnv1a
    {
        /// <summary>FNV 오프셋 기준값</summary>
        private const uint OffsetBasis = 2166136261;

        /// <summary>FNV 소수 곱셈 계수</summary>
        private const uint Prime = 16777619;

        /// <summary>현재 해시 값</summary>
        public uint Value { get; private set; } = OffsetBasis;

        /// <summary>64비트 정수를 낮은 바이트부터 섞는다</summary>
        /// <param name="value">섞을 값</param>
        public void Add(long value)
        {
            // 8바이트를 차례로 섞는다
            for (int i = 0; i < 8; i++)
            {
                Value = unchecked((Value ^ (byte)(value >> (i * 8))) * Prime);
            }
        }

        /// <summary>문자열을 UTF-16 코드 단위마다 섞는다</summary>
        /// <param name="text">섞을 문자열</param>
        public void Add(string text)
        {
            // 코드 단위를 하나씩 섞는다
            foreach (char c in text)
            {
                Add(c);
            }
            Add(text.Length);
        }
    }
}
