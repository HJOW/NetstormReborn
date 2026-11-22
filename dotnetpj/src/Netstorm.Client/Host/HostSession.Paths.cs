using Netstorm.Core.Original;

// 실제 경로의 생성·시작·프레임·현재 조회를 같은 Host 소유 자원에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignPathEffects의 실제 metadata/음향/겹침/큰 부모 효과다.</summary>
public sealed record HostPathEffects
{
    /// <summary>현재 부모TYPE의 실제 속도·회전·기본 프레임이다.</summary>
    public Func<int, RawPathStartType>? Metadata { get; init; }
    /// <summary>현재 부모TYPE의 실제 발소리 자산 번호다.</summary>
    public Func<int, uint>? SoundId { get; init; }
    /// <summary>현재 부모/위치의 실제 발소리 재생이다.</summary>
    public Action<int, float, float, uint>? StepSound { get; init; }
    /// <summary>시작 실패의 실제 겹침 정리다.</summary>
    public Action<RawPathPoint, uint, uint>? StartBlocked { get; init; }
    /// <summary>도착의 실제 부모/위치 겹침 정리다.</summary>
    public Action<int, RawPathPoint, uint, uint>? ArrivalBlocked { get; init; }
    /// <summary>실제 현재 운반 대상의 재탐색이다.</summary>
    public Action<RawPathRuntime, int>? Retarget { get; init; }
    /// <summary>CD의 실제 관찰 시계이며 게임 시각과 구별한다.</summary>
    public Func<ulong>? ObservationStamp { get; init; }
    /// <summary>134/105 실제 건설/마나 볼트 부모 도착이다.</summary>
    public Func<int, uint, uint, uint>? ConstructionArrival { get; init; }
    /// <summary>현재 벌룬 화염의 위치 음향이다.</summary>
    public Action<float, float>? BalloonFlame { get; init; }
    /// <summary>같은 게임의 벌프 반열린 난수다.</summary>
    public Func<int, int, int>? BulfRandom { get; init; }
    /// <summary>벌프의 실제 파일/위치 음향이다.</summary>
    public Action<float, float, string>? BulfSound { get; init; }
    /// <summary>156 크리스탈 크랩의 실제 전용 정지다.</summary>
    public Func<int, uint>? CrabHalt { get; init; }
    /// <summary>70 Dude의 실제 정지 부모다.</summary>
    public Func<int, uint>? DudeHalt { get; init; }
    /// <summary>70 Dude의 실제 도착 부모다.</summary>
    public Func<int, uint>? DudeArrival { get; init; }
    /// <summary>정상 복귀 가능한 현재 Client의 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; init; }
    /// <summary>같은 Rune SHP/현재 지면의 의식 단계점이다.</summary>
    public Func<int, uint, RawPathPoint>? StagePoint { get; init; }
}

/// <summary>RawCampaignPathBindings.cpp의 본체를 실제 Host에 설치하는 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 시계와 원문 시작 전역을 빌리는 시작 본체다.</summary>
    private RawPathStart? _pathStart;
    /// <summary>같은 Host/Prepare 시계와 현재 역할을 빌리는 상태다.</summary>
    private RawPathStartState? _pathStartState;
    /// <summary>8종 클래스가 공유하는 같은 시계/진단/역할 상태다.</summary>
    private RawPathProcessState? _pathProcessState;
    /// <summary>모든 클래스가 공간 접두 뒤 사용하는 같은 정리 본체다.</summary>
    private RawPathCleanup? _pathCleanup;
    /// <summary>현재 세션의 실제 함수 값이다.</summary>
    private HostPathEffects? _pathEffects;
    /// <summary>기존 파생 writer 뒤에 설치한 현재 Path writer다.</summary>
    private ProcessFormTransmission? _pathTransmission;
    /// <summary>현재 세션이 반납할 때 복원할 이전 파생 체인이다.</summary>
    private ProcessFormTransmission? _beforePathTransmission;
    /// <summary>미설치/반납 뒤에는 실패하는 실제 시작 본체다.</summary>
    public RawPathStart PathStarter => FixedPacketNeed(_pathStart);
    /// <summary>같은 실제 원문 시작 전역을 빌린다.</summary>
    public RawPathStartState PathStartState => FixedPacketNeed(_pathStartState);
    /// <summary>같은 실제 프레임/Prepare 전역을 빌린다.</summary>
    public RawPathProcessState PathProcessState => FixedPacketNeed(_pathProcessState);
    /// <summary>원문 필수 metadata/음향/겹침/재탐색을 효과 전에 확인한다.</summary>
    private static void ValidatePathEffects(HostPathEffects effects)
    {
        if (effects.Metadata is null || effects.SoundId is null || effects.StepSound is null || effects.StartBlocked is null || effects.ArrivalBlocked is null || effects.Retarget is null) throw new ArgumentException("캠페인 경로의 실제 필수 효과 누락");
    }
    /// <summary>기존 부모 조립 뒤 같은 Pool/시계/공간에8종 생성·Start·서버 전송을 설치한다. 설치 자체는 게임 효과를 내지 않는다.</summary>
    public RawPathStart BuildPathWorldDevices(RawPathCleanup cleanup, HostPathEffects effects, RawPathStartState? start = null, RawPathProcessState? process = null)
    {
        EnsureRuntimeCanRelease(); ValidatePathEffects(effects);
        if (_pathStart is not null) throw new InvalidOperationException("캠페인 경로 중복 설치");
        _ = PathSearch; _ = PathPriestHalt; _ = PathPriestArrival; _ = PathCarrierArrival;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        SquidProcessState clock = PlayerProcessClock;
        start ??= new(clock, priestType: GameAux.PriestTypeBox, geyserType: GameAux.GeyserTypeBox);
        process ??= new(clock);
        if (!ReferenceEquals(cleanup.Pool, Pool) || !ReferenceEquals(start.Clock, clock) || !ReferenceEquals(process.Clock, clock) || start.PriestType != 158 || process.PriestType != 158 || process.PriestProcess != 54) throw new ArgumentException("경로의 같은 실제 Pool/시계/54 역할 불일치");
        var copy = effects with { }; var lookup = new RawContainedLookup(Pool, (_, _) => throw new InvalidOperationException("현재 경로 조회의 삭제는 실제 가상 분배를 사용해야 합니다"));
        var starter = new RawPathStart(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), PlayerProcesses, lookup, FixedPacketNeed(_liveFrame), start, new()
        {
            ProcessType = PriestWorld.MovementProcessType, Create = priest => CreatePathType(priest ? PathProcessState.PriestProcess : PathProcessState.TeleportProcess),
            CreateType = CreatePathType, Metadata = copy.Metadata, ClearBlocked = copy.StartBlocked,
        });
        _pathStartState = start; _pathProcessState = process; _pathCleanup = cleanup; _pathEffects = copy; _pathParentLookup = lookup;
        process.Checking.Bind(() => AiChecking.Read());
        _beforePathTransmission = PlayerProcesses.Transmission();
        Func<SquidProcess, Memory<byte>, bool, uint, int>? native = storage.UsesNativeTransmissionDevices()
            ? (current, buffer, _, _) => storage.SerializePathProcess(current as RawPathProcess ?? throw new ArgumentException("Native Path writer 클래스 오류"), buffer) : null;
        _pathTransmission = RawPathTransmission.Make(Pool, FixedPacketNeed(_terrainPopTypes), GameAux.PathPacketHeader, GameAux.SquidTransmission.Buffer, BroadcastPathPacket, _beforePathTransmission, native);
        PlayerProcesses.SetFormTransmit(_pathTransmission); _pathStart = starter; return starter;
    }
    /// <summary>현재 작성된 같은 공통 버퍼를 실제 Native 또는 명시 typed 전송 장치로 전달한다.</summary>
    private void BroadcastPathPacket(int length)
    {
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
        byte[] buffer = GameAux.SquidTransmission.Buffer; if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length));
        FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
    }
    /// <summary>현재 역할 번호로 클래스를 고르며 고정 constructor 수신과 같은 실제 생성기를 쓴다.</summary>
    public RawPathProcess CreatePathType(uint type)
    {
        RawPathProcessState s = PathProcessState;
        bool priest = type == s.PriestProcess, flying = type == s.FlyProcess, worm = type == s.WormProcess, balloon = type == s.BalloonProcess, dude = type == s.DudeProcess, bulf = type == s.BulfProcess, walker = type == s.WalkerProcess;
        if (!priest && !flying && !worm && !balloon && !dude && !bulf && !walker && type != s.TeleportProcess) throw new InvalidOperationException("다른 캠페인 경로 생성자 미연결");
        return CreatePathVariant(priest, flying, worm, balloon, dude, bulf, walker);
    }
    /// <summary>cdecl 주소가 고정한8클래스만 생성한다. mutable 역할 번호로 클래스를 다시 추측하지 않는다.</summary>
    public RawPathProcess? CreatePathByConstructor(uint address)
    {
        if (Pool.Edition != OriginalEdition.Patch1078) return null;
        return address switch
        {
            0x48be40 => CreatePathVariant(false, false, false, false, false, false, false),
            0x48be70 => CreatePathVariant(false, false, false, true, false, false, false),
            0x48bea0 => CreatePathVariant(false, true, false, false, false, false, false),
            0x48bed0 => CreatePathVariant(false, false, false, false, false, false, true),
            0x48bf00 => CreatePathVariant(false, false, false, false, false, true, false),
            0x48bf30 => CreatePathVariant(false, false, false, false, true, false, false),
            0x48bf60 => CreatePathVariant(true, false, false, false, false, false, false),
            0x48bf90 => CreatePathVariant(false, false, true, false, false, false, false), _ => null,
        };
    }
    /// <summary>기존 실제 부모/공간/정리와 동일 BFS를 각 원문 클래스에 연결한다.</summary>
    private RawPathProcess CreatePathVariant(bool priest, bool flying, bool worm, bool balloon, bool dude, bool bulf, bool walker)
    {
        HostPathEffects effects = FixedPacketNeed(_pathEffects); RawPathProcessState state = PathProcessState;
        RawPathProcessHooks hooks = new()
        {
            Assertion = effects.Assertion, Unpop = UnpopWorldObject, Repop = RepopWorldObject, Pop = (sid, point, flags) => PopWorldObject(sid, point.X, point.Y, flags),
            Halt = sid =>
            {
                if (dude) return FixedPacketNeed(effects.DudeHalt)(sid); if (priest) return PathPriestHalt.Halt(sid);
                if (worm || balloon || bulf || walker) { ValidatePathCarrier(sid); if (walker && Pool.Slot(sid)[10] == state.RainWalkerType) return FixedPacketNeed(effects.CrabHalt)(sid); return PathPriestHalt.CommonHalt(sid); }
                ValidatePathConstruction(sid); return 10;
            },
            Arrival = (sid, first, second) =>
            {
                if (dude) return FixedPacketNeed(effects.DudeArrival)(sid); if (priest) return PathPriestArrival.Arrival(sid, first, second);
                if (worm || balloon || bulf || walker) { ValidatePathCarrier(sid); return PathCarrierArrival.Arrival(sid, first, second); }
                ValidatePathConstruction(sid); return FixedPacketNeed(effects.ConstructionArrival)(sid, first, second);
            },
            SoundId = effects.SoundId, StepSound = effects.StepSound, ClearBlocked = effects.ArrivalBlocked, Retarget = effects.Retarget,
            ObservationStamp = effects.ObservationStamp, BalloonFlame = effects.BalloonFlame, BulfRandom = effects.BulfRandom, BulfSound = effects.BulfSound,
            Cleanup = (runtime, parent, _) => FixedPacketNeed(_pathCleanup).AfterSpatial(runtime, parent),
        };
        hooks = RawPathSearch.MakeHooks(Pool, PathSearch, hooks);
        return new(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), WorldFrames, FixedPacketNeed(_terrainPopShapes), FixedPacketNeed(_terrainSpots), state, hooks, priest, flying, worm, balloon, dude, bulf, walker);
    }
    /// <summary>Kernel/경로 정리 뒤 자기 전송 체인만 반납하고 다른 Runtime의 장치를 덮어쓰지 않는다.</summary>
    private void ReleasePathDevices()
    {
        if (_pathTransmission is not null && !ProcessBindingsReturned() && ReferenceEquals(PlayerProcesses.Transmission(), _pathTransmission)) PlayerProcesses.SetFormTransmit(_beforePathTransmission);
        _pathStart = null; _pathStartState = null; _pathProcessState = null; _pathCleanup = null; _pathEffects = null; _pathTransmission = null; _beforePathTransmission = null;
    }
}
