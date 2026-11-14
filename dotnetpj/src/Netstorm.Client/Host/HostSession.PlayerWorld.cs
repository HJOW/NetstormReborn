using System.Buffers.Binary;
using Netstorm.Core.Original;

// 미션의 실제 Player 화신과 공유 장부·공간·프로세스 결선을 담는다.
namespace Netstorm.Client.Host;

/// <summary>실제 세션의 화신 생성과 기술부모 등록을 한 수명으로 유지한다.</summary>
public sealed partial class HostSession
{
    /// <summary>월드 적재와 AI가 함께 사용하는 화신 몸체다.</summary>
    private RawPlayerWorld? _livePlayerWorld;

    /// <summary>화신 몸체가 재사용하는 동일 갈고리다.</summary>
    private PlayerWorldHooks? _livePlayerHooks;

    /// <summary>Player·사제가 같은 가상 후처리를 등록하는 실제 공통 부모다.</summary>
    private RawPostPop? _livePostPop;
    /// <summary>Player·사제·보호막이 같은 해시/점유 지도에 등록하는 몸체다.</summary>
    private RawPop? _livePop;
    /// <summary>같은 공유 공간의 등록 해제 몸체다.</summary>
    private RawUnpop? _liveUnpop;
    /// <summary>현재 프레임의 표시·공간 효과를 같은 실제 장치로 전달한다.</summary>
    private SquidFrame? _liveFrame;

    /// <summary>원문 할당 관문에서 확보해 다음 부착에 넘기는 실제 객체다.</summary>
    private RegularProcess? _reservedPlayerRegular;

    /// <summary>R4 초기 시각을 담는 실제 공유 상자다. 프레임 시계 갱신은 R6에서 결선한다.</summary>
    public DoubleBox ProcessTime { get; } = new();

    /// <summary>화신의 실제 부모 체인·커널 등록을 처리하는 호스트다.</summary>
    public SquidProcessHost PlayerProcesses { get; private set; } = null!;

    /// <summary>월드 적재와 AI가 동일한 Player 기록을 읽는 몸체다. 결선 전 조회는 거부한다.</summary>
    public RawPlayerWorld PlayerWorld => _livePlayerWorld
        ?? throw new InvalidOperationException("실제 Player 월드가 연결되지 않았다");

    /// <summary>실제 공장·공간·표시 헤더·장부·프로세스를 한 번 조립한다.</summary>
    private PlayerWorldHooks BindLivePlayerWorld()
    {
        if (_livePlayerHooks is not null) return _livePlayerHooks;
        if (_fortFactory is null || _playerOwner is null || _terrainHash is null
            || _terrainSpots is null || _terrainDisplay is null || _terrainPopTypes is null)
            throw new InvalidOperationException("Player 월드 장치는 요새 객체 장치 뒤에 연결해야 한다");

        var types = _terrainPopTypes;
        var factory = _fortFactory;
        var owner = _playerOwner;
        var hash = _terrainHash;
        var display = _terrainDisplay;
        var inner = new RawPostPop(Pool, types, TerrainBookkeeping);
        var pop = new RawPop(Pool, hash, _terrainSpots, display, inner);
        var unpop = new RawUnpop(Pool, hash, _terrainSpots, display);
        _livePostPop = inner; _livePop = pop; _liveUnpop = unpop;
        _liveFrame = new SquidFrame(Pool, types, SquidFrameBinding.MakeSquidFrameHooks(Pool, types, display, unpop, pop));
        _liveFrame.BindCurrentTypes(types);
        var destroy = new RawSquidDestroy(Pool, unpop, types);
        PlayerProcesses = new SquidProcessHost(Pool, types, Kernel, destroy,
            new SquidProcessState { SharedNow = ProcessTime });
        PlayerProcesses.BindFormFactory(factory);
        BindWorldPopParent();
        var hooks = new PlayerWorldHooks
        {
            Assertion = assertion => throw new InvalidOperationException($"Player 원본 진단: {assertion}"),
            Find = (x, y, type) =>
            {
                var finder = new RawSquidFinder(Pool, hash, types);
                int left = unchecked((int)x), top = unchecked((int)y);
                // 원문 공간 순서에서 같은 타입의 첫 슬롯을 찾는다.
                for (int sid = finder.Begin(new SquidSearchArea(left, top, left, top)); sid != 0; sid = finder.Next())
                    if (Pool.Slot(sid)[RawSidPool.TypeOffset] == type) return sid;
                return 0;
            },
            Create = (type, flags) => factory.Create(type,
                (flags & 1) != 0 ? Netstorm.Core.Simulation.SidAllocation.Predictable
                : (flags & 2) != 0 ? Netstorm.Core.Simulation.SidAllocation.Client
                : Netstorm.Core.Simulation.SidAllocation.Server),
            SetOwner = (sid, player) => owner.Set(sid, player),
            Pop = (sid, x, y, flags) =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(sid);
                int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
                float[] size = display.FrameSize(Pool.Edition, raw, frame);
                TerrainBookkeeping.LocalOwner = checked((byte)MissionPlayer.BattleOwner);
                pop.Pop(sid, types[raw[RawSidPool.TypeOffset]], size[0], size[1], x, y, flags);
            },
            ReserveRegular = ReservePlayerRegular,
            AddRegular = AttachPlayerRegular,
        };
        _livePlayerWorld = new RawPlayerWorld(Pool, inner, PlayerRecords, MissionPlayer, hooks);
        inner.SetPlayerPostPop(_livePlayerWorld);
        _livePlayerHooks = hooks;
        return hooks;
    }

    /// <summary>원문 Regular 할당 성공 여부를 실제 관리 객체 할당으로 결정한다.</summary>
    private bool ReservePlayerRegular()
    {
        if (_reservedPlayerRegular is not null)
            throw new InvalidOperationException("Player Regular 예약이 아직 소비되지 않았다");
        try { _reservedPlayerRegular = new RegularProcess(1u, 5f); return true; }
        catch (OutOfMemoryException) { return false; }
    }

    /// <summary>할당 관문에서 확보한 같은 객체를 실제 화신 부모 체인에 부착한다.</summary>
    private void AttachPlayerRegular(int sid, uint ev, float payload)
    {
        RegularProcess process = _reservedPlayerRegular
            ?? throw new InvalidOperationException("Player Regular의 실제 할당 예약 누락");
        _reservedPlayerRegular = null;
        if (process.Event() != ev || process.Payload() != payload)
            throw new InvalidOperationException("Player Regular의 원본 event·payload 계약 위반");
        _ = PlayerProcesses.AttachRegular(process, sid, false)
            ?? throw new InvalidOperationException("Player Regular 부착 실패");
    }

    /// <summary>빌린 커널에서 이 세션의 실제 Host가 부착한 프로세스만 비우고 사제 대여를 반납한다.</summary>
    public void ReleasePlayerProcesses()
    {
        if (PlayerProcesses is not null) Kernel.DrainOwnedForShutdown(PlayerProcesses.Owns);
        ReleasePriestWorld();
        _reservedPlayerRegular = null;
    }
}
