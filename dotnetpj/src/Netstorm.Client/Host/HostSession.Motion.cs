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
    /// <summary>같은 실제 풀/시계의 정리→정지→Carrier→사제→경로를 순서대로 설치한다.</summary>
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
        try
        {
            RawPathCleanup cleanup = BuildPathCleanupWorldDevices(PriestWorld.Carrier, anchor, state.Cleanup, effects.Cleanup with { Destroy = effects.VirtualDestroy });
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
}
