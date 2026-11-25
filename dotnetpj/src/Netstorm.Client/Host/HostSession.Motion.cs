using Netstorm.Assets;
using Netstorm.Core.Original;

// 기존 원문 본체를 같은 사제·운반·Graph·경로·공간에 한 번 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignMotionState의 실제 지면/정리/정지/도착 상태다. 검색은 기존 Host.PathSearchState를 빌린다.</summary>
public sealed class HostMotionState
{
    /// <summary>중립 도착의 현재 지면 getter 상태다.</summary>
    public CanonPlacementTerrainState Terrain { get; } = new();
    /// <summary>현재 예약/너겟/의식/환불 정리 상태다.</summary>
    public PathCleanupState Cleanup { get; } = new();
    /// <summary>현재 사제 정지/공통 Carrier 정지 상태다.</summary>
    public PriestHaltState Halt { get; } = new();
    /// <summary>현재 Carrier 도착/동맹/AI 존재 상태다.</summary>
    public CarrierArrivalState Carrier { get; } = new();
    /// <summary>현재 사제 중립/적재/귀환 억제 상태다.</summary>
    public PriestArrivalState Priest { get; } = new();
}

/// <summary>RawCampaignMotionEffects의 실제 큰 하위 장치다. 전체 가상 삭제를 일반 삭제로 대신하지 않는다.</summary>
public sealed record HostMotionEffects
{
    /// <summary>실제 예약/의식 장치이며 조회·환불·Host·삭제는 같은 세션에 결합한다.</summary>
    public PathCleanupHooks Cleanup { get; init; } = new();
    /// <summary>실제 포획/너겟 전달이며 공간은 같은 세션에 결합한다.</summary>
    public PriestHaltHooks Halt { get; init; } = new();
    /// <summary>실제 운반/포획/경제/회피이며 현재 경로/조회/삭제는 같은 세션에 결합한다.</summary>
    public CarrierArrivalHooks Carrier { get; init; } = new();
    /// <summary>실제 중립 귀환이며 자기삭제는 같은 전체 가상 삭제에 결합한다.</summary>
    public PriestArrivalHooks Priest { get; init; } = new();
    /// <summary>실제 TYPE/음향/겹침/재탐색/선택 부모 장치다.</summary>
    public HostPathEffects Path { get; init; } = new();
    /// <summary>같은 실제 월드의 전체 가상10 분배다. generic Pre/Post를 제공하면 안 된다.</summary>
    public Action<int, uint>? VirtualDestroy { get; init; }
}

/// <summary>RawCampaignMotionBindings.cpp의 순환된 Carrier→현재Path 조립이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 지면 RegionAt을 쓰는 도착 전용 getter다.</summary>
    private RawCanonPlacementTerrain? _motionTerrain;
    /// <summary>미설치/반납 뒤에는 실패하는 실제 도착용 지면 getter다.</summary>
    public RawCanonPlacementTerrain MotionTerrain => FixedPacketNeed(_motionTerrain);
    /// <summary>같은 실제 풀/시계의 정리→정지→Carrier→사제→경로를 순서대로 설치한다. 정리 예약·의식은 비어 있으면 실제 세션 장치를 쓴다.</summary>
    /// <remarks>2026-11-25 1.034에서 WP-1 Cleanup 상위 설치용으로 정리 예약·의식 기본값을 실제 세션 장치로 바꿨다. 근거: cpppj src/client/RawCampaignMotionBindings.cpp 39~44행의 cleanupHooks 조립이다. 가상 삭제는 호출자의 VirtualDestroy를 그대로 쓴다.</remarks>
    public RawPathStart BuildMotionWorldDevices(RawIslandRuntime islands, RawPlayerPlacementAnchor anchor, IReadOnlyList<PriestPlainCanonType> frames, HostMotionState state, HostMotionEffects effects, RawPathStartState? start = null, RawPathProcessState? process = null)
    {
        EnsureRuntimeCanRelease(); ValidatePathEffects(effects.Path);
        if (_pathStart is not null || _pathPriestHalt is not null || _pathCarrierArrival is not null || _pathPriestArrival is not null || _motionTerrain is not null) throw new InvalidOperationException("Motion 실제 부모 중복 설치");
        if (!ReferenceEquals(islands.Pool(), Pool) || !ReferenceEquals(anchor.Pool, Pool) || frames.Count != WorldFrames.Length || effects.VirtualDestroy is null) throw new ArgumentException("Motion 같은 실제 풀/프레임/전체 가상 삭제 연결 누락");
        _ = PathSearch; _ = PriestWorld; _ = ConstructionRefund;
        _ = FixedPacketNeed(_squidPacketStorage);
        var terrain = new RawCanonPlacementTerrain(Pool, FixedPacketNeed(_terrainPopTypes), frames, FixedPacketNeed(_terrainHash).BorrowEntries(0), state.Terrain, new()
        {
            CandidatePermission = (_, _) => throw new InvalidOperationException("도착 getter의 배치 권한 미연결"),
            ShapePermission = (_, _) => throw new InvalidOperationException("도착 getter의 배치 Shape 미연결"),
            FinishRelations = (_, _) => throw new InvalidOperationException("도착 getter의 배치 Finish 미연결"),
        });
        state.Cleanup.SharedBoss = () => Gameplay.Authority.Boss; state.Cleanup.SharedNuggetType = GameAux.NuggetTypeBox; state.Cleanup.Checking.Bind(() => AiChecking.Read());
        PathCleanupHooks rootCleanup = BuildPathCleanupRootHooks();
        PathCleanupHooks cleanupEffects = effects.Cleanup with
        {
            ReserveShared = effects.Cleanup.ReserveShared ?? rootCleanup.ReserveShared,
            Ritual = effects.Cleanup.Ritual ?? rootCleanup.Ritual,
            Destroy = effects.VirtualDestroy,
        };
        try
        {
            RawPathCleanup cleanup = BuildPathCleanupWorldDevices(PriestWorld.Carrier, anchor, state.Cleanup, cleanupEffects);
            _ = BuildPriestHaltWorldDevices(effects.Halt, state.Halt);
            _ = BuildCarrierArrivalWorldDevices(cleanup, anchor, state.Carrier, effects.Carrier with { Destroy = effects.VirtualDestroy });
            _ = BuildPriestArrivalWorldDevices(terrain, islands, state.Priest, effects.Priest with { Destroy = effects.VirtualDestroy });
            RawPathStart starter = BuildPathWorldDevices(cleanup, effects.Path, start, process); _motionTerrain = terrain; return starter;
        }
        catch
        {
            // 조립은 게임 효과를 실행하지 않으므로 실패 시 이번 참조/전송 대여만 되돌린다.
            ReleasePathDevices(); ReleasePathParentDevices(); throw;
        }
    }

    /// <summary>제품 Root 미션 흐름의 실제 Motion 효과 묶음을 만든다. 입력·음향·포획·회피·CD 갈고리는 명시 거부한다.</summary>
    /// <param name="definitions">같은 순서의 실제 자산 정의다. 경로 Metadata의 속도·회전·기본 프레임을 읽는다.</param>
    /// <returns>같은 세션 장치와 연결한 Motion 조립 효과다. 정리 예약·의식·가상 삭제·벌프 난수·의식 목표점은 실제 장치다.</returns>
    /// <remarks>2026-11-25 1.035에서 WP-2 제품 Root Path/Start/Motion 설치용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 3899~3928행의 effects 조립이다. Metadata·벌프 난수·의식 목표점은 실제 장치를 읽고 Cd*·PlayAt·Robust는 원문 FillMissing처럼 명시 거부한다. 포획·회피·귀환·귀환 선택·벌룬·크랩·Dude·건설 도착과 시작/도착 막힘·재탐색·발소리·위치 소리는 입력·음향·AI와 함께 R6에서 잇는다.</remarks>
    public HostMotionEffects BuildMotionRootEffects(IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(definitions);
        PathCleanupHooks rootCleanup = BuildPathCleanupRootHooks();
        Action<int, uint> destroy = rootCleanup.Destroy ?? throw new InvalidOperationException("Motion 실제 가상 삭제 미연결");
        static InvalidOperationException Missing(string name) => new("Motion 실제 효과 미연결(R6): " + name);
        static InvalidOperationException Unported(string name) => new("Motion 효과 미연결: " + name);
        RawTempleRitual ritual = TempleRitual;
        return new HostMotionEffects
        {
            Cleanup = new() { ReserveShared = rootCleanup.ReserveShared, Ritual = rootCleanup.Ritual },
            Halt = new()
            {
                CarrierPickup = _ => throw Missing("포획"),
                NuggetTransfer = _ => throw Missing("너겟 전달"),
            },
            Carrier = new()
            {
                Pickup = _ => throw Missing("포획 요청"),
                Escape = (_, _, _) => throw Missing("회피"),
                ChooseReturn = _ => throw Missing("귀환 선택"),
                ChooseGeyser = _ => throw Missing("간헐천 선택"),
                GoReturn = (_, _) => throw Missing("귀환 이동"),
                GoGeyser = (_, _) => throw Missing("간헐천 이동"),
                MinedPacket = _ => throw Missing("채굴 통지"),
                Destroy = destroy,
                CdPriestPickupPriest = () => throw Unported("CD 사제 포획 설정"),
                CdAddSp = (_, _) => throw Unported("CD SP 가산"),
                CdOwnerPacket = _ => throw Unported("CD 소유자 통지"),
                PlayAt = (_, _, _, _) => throw Unported("위치 소리"),
                Robust = _ => throw Unported("Robust 검사"),
            },
            Priest = new()
            {
                Destroy = destroy,
                NeutralReturn = _ => throw Missing("중립 귀환"),
            },
            Path = new()
            {
                Metadata = sid =>
                {
                    uint type = Pool.Slot(sid)[RawSidPool.TypeOffset];
                    RiftTypeDefinition definition = definitions[(int)type - 70];
                    return new RawPathStartType((float)definition.Number("speed")!.Value, (float)definition.Number("turningSpeed")!.Value, definition.SpecialFrames.DefaultFrame);
                },
                SoundId = _ => 0u,
                StepSound = (_, _, _, _) => throw Missing("발소리 재생"),
                StartBlocked = (_, _, _) => throw Missing("시작 막힘 정리"),
                ArrivalBlocked = (_, _, _, _) => throw Missing("도착 막힘 정리"),
                Retarget = (_, _) => throw Missing("재탐색"),
                BalloonFlame = (_, _) => throw Missing("벌룬 화염"),
                BulfRandom = (low, high) =>
                {
                    if (high <= low) throw new ArgumentOutOfRangeException(nameof(high), "벌프 공용 난수 범위 오류");
                    return unchecked((int)Game.Next(unchecked((uint)(high - low))) + low);
                },
                BulfSound = (_, _, _) => throw Missing("벌프 발소리"),
                CrabHalt = _ => throw Missing("크랩 정지"),
                DudeHalt = _ => throw Missing("Dude 정지"),
                DudeArrival = _ => throw Missing("Dude 도착"),
                ConstructionArrival = (_, _, _) => throw Missing("건설 도착"),
                Assertion = (expression, file, line) => throw new InvalidOperationException($"Motion 원본 진단: {expression} {file}:{line}"),
                StagePoint = (temple, stage) =>
                {
                    TempleRitualPoint point = ritual.StagePoint(temple, stage);
                    return new RawPathPoint(point.X, point.Y);
                },
            },
            VirtualDestroy = destroy,
        };
    }

    /// <summary>제품 Root 미션 흐름에 실제 Motion 전체 부모를 한 번 설치한다. 입력·음향 갈고리는 명시 거부다.</summary>
    /// <param name="definitions">같은 순서의 실제 자산 정의다. 경로 Metadata와 프레임 표를 만든다.</param>
    /// <returns>같은 실제 Pool/시계/공간의 시작 본체다. 입력이 없으면 게임 효과를 내지 않는다.</returns>
    /// <remarks>2026-11-25 1.035에서 WP-2 제품 Root Path/Start/Motion 설치용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 3806~3807행의 anchorState와 3929행의 motion 조립이다. 그래프 준비·펜스·바람 사냥꾼 TYPE은 원문 상수와 같고 추가 목록·내용물 조회는 세션 공유 상태를 쓴다.</remarks>
    public RawPathStart BuildMotionRootDevices(IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(definitions);
        RawIslandRuntime islands = TerrainBinding?.Islands ?? throw new InvalidOperationException("Motion 설치 전 지형 바인딩 미연결");
        var anchor = new RawPlayerPlacementAnchor(Pool, FixedPacketNeed(_terrainPopTypes), FixedPacketNeed(_terrainHash).BorrowEntries(0), TerrainBookkeeping, SidLists.Outposts, ProductionFinder, new PlayerPlacementAnchorState { GraphReady = _worldGraph is not null ? 1u : 0u, FenceType = 90, WindArcherType = 140 });
        PriestPlainCanonType[] frames = WorldFrames.Select((frames, index) => new PriestPlainCanonType { Frames = frames, DefaultFrame = index < 70 || index - 70 >= definitions.Count ? 0 : definitions[index - 70].SpecialFrames.DefaultFrame }).ToArray();
        return BuildMotionWorldDevices(islands, anchor, frames, new HostMotionState(), BuildMotionRootEffects(definitions));
    }
}
