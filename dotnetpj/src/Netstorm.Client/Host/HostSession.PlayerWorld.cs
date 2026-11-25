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
    /// <summary>같은실제삭제/공장/Regular/전송을소유하는현재프로세스대여다. Host는약하게조회한다.</summary>
    private ProcessHostRuntimeBindings? _playerProcessRuntime;
    /// <summary>현재 Host가 실제로 빌리는 같은 시계/권한 객체다. 경로가 별도 상태를 만들지 않게 보관한다.</summary>
    private SquidProcessState? _playerProcessClock;
    /// <summary>설치된 실제 Host의 상태를 빌리며 미설치/반납 뒤에는 거부한다.</summary>
    public SquidProcessState PlayerProcessClock => _playerProcessClock ?? throw new InvalidOperationException("현재 Host 시계 미설치 또는 반납");
    /// <summary>같은Client Kernel/Frame과현재TYPE를빌리는장기프로세스저장소다. 독립조합만null이다.</summary>
    private readonly RawProcessHostStorage? _sharedPlayerProcesses;
    /// <summary>현재실제Runtime이단독으로빌린장기Host의반납권한이다.</summary>
    private RawProcessHostStorage.Use? _playerProcessUse;
    /// <summary>같은Host의현재대여선택칸이다. 외부에남은묶음참조와관계없이명시반납한다.</summary>
    private WeakReference<ProcessHostRuntimeBindings>? _playerProcessCurrent;

    /// <summary>원문 할당 관문에서 확보해 다음 부착에 넘기는 실제 객체다.</summary>
    private RegularProcess? _reservedPlayerRegular;

    /// <summary>R4 초기 시각을 담는 실제 공유 상자다. 프레임 시계 갱신은 0.629의 미션 틱에서 전진한다.</summary>
    public DoubleBox ProcessTime { get; }

    /// <summary>R4-2 공장 프레임 집계용으로 실제 공유 시각을 전진시키고 커널을 한 번 실행한다.</summary>
    /// <param name="seconds">전진할 초다. 고정 프레임 모드에서는 프레임 밀리초를 초로 나눈 값이다.</param>
    /// <remarks>2026-11-19 0.629에서 R4-4 공장 프레임·정기 집계용으로 추가했다. 메뉴에는 미션 세션이 없어 닿지 않으며 전체 R6 틱 결선은 R6에서 잇는다.</remarks>
    public void AdvanceMissionFrame(double seconds)
    {
        if (!double.IsFinite(seconds) || seconds < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(seconds), "미션 프레임 전진 초 오류");
        }

        ProcessTime.Value += seconds;
        RunMissionFrame();
    }

    /// <summary>일시정지가 아닐 때만 실제 AI 전체 프레임을 실행한다. 원본 PrepareFrame의 aiRuntime->Frame()과 같다.</summary>
    /// <param name="paused">정지 중이면 참이다. 참이면 AI를 실행하지 않고 설치 여부도 검사하지 않는다.</param>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 세션 결선용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 4569행의 if (!paused) aiRuntime->Frame()이다. Drops·Bridges 미연결은 R6에서 잇는다. 실제 틱 결선(매 프레임 호출)은 live 루프가 R6에서 잇는다.</remarks>
    public void RunAiFrame(bool paused)
    {
        if (paused)
        {
            return;
        }

        (_aiRuntime ?? throw new InvalidOperationException("AI 전체 프레임 미연결")).Frame();
    }

    /// <summary>같은 현재 시각/Player 모드를 Gun에 준비하고 Kernel을 한 번 실행한다. 시계를 전진하지 않는다.</summary>
    /// <remarks>2026-11-19 0.629에서 추가했다. AI 전체 프레임의 실제 틱 결선은 Drops·Bridges와 함께 R6에서 잇는다.</remarks>
    public void RunMissionFrame() { if (_gunProcess is not null) PrepareGunFrame(); Kernel.RunFrame(); }

    /// <summary>화신의 실제 부모 체인·커널 등록을 처리하는 호스트다.</summary>
    public SquidProcessHost PlayerProcesses { get; private set; } = null!;

    /// <summary>월드 적재와 AI가 동일한 Player 기록을 읽는 몸체다. 결선 전 조회는 거부한다.</summary>
    public RawPlayerWorld PlayerWorld => _livePlayerWorld
        ?? throw new InvalidOperationException("실제 Player 월드가 연결되지 않았다");

    /// <summary>실제 공장·공간·표시 헤더·장부·프로세스를 한 번 조립한다.</summary>
    private PlayerWorldHooks BindLivePlayerWorld()
    {
        if (_livePlayerHooks is not null) return _livePlayerHooks;
        if (_fortFactory is null || _fortBuilding is null || _playerOwner is null || _terrainHash is null
            || _terrainSpots is null || _terrainDisplay is null || _terrainPopTypes is null)
            throw new InvalidOperationException("Player 월드 장치는 요새 객체 장치 뒤에 연결해야 한다");

        var types = _terrainPopTypes;
        var factory = _fortFactory;
        var owner = _playerOwner;
        var hash = _terrainHash;
        var display = _terrainDisplay;
        var inner = TrackPostPopRuntime(new RawPostPop(Pool, types, TerrainBookkeeping));
        // 실제 호스트의 같은 현재 TYPE를 빌리되 최초 비용 snapshot은 별도로 보존한다.
        inner.BindCurrentTypes(types);
        // 전용 장치가 먼저 준비된 경우도 같은 실제 후처리 본체를 빌린다.
        if (_vortexAssetPostPop is not null) inner.SetAssetPostPop(_vortexAssetPostPop);
        if (_factoryPostPop is not null) inner.SetFactoryPostPop(_factoryPostPop);
        if (_fencePostPop is not null) inner.SetFencePostPop(_fencePostPop);
        var pop = new RawPop(Pool, hash, _terrainSpots, display, inner);
        var unpop = new RawUnpop(Pool, hash, _terrainSpots, display);
        // 요새/지형과 같은 실제 건물 옆면을 공유해 신규 건물의 공간 등록과 완료 재등록을 처리한다.
        pop.SetBuildingSide(_fortBuilding); unpop.SetBuildingSide(_fortBuilding);
        _livePostPop = inner; _livePop = pop; _liveUnpop = unpop;
        _liveFrame = new SquidFrame(Pool, types, SquidFrameBinding.MakeSquidFrameHooks(Pool, types, display, unpop, pop));
        _liveFrame.BindCurrentTypes(types);
        // Native만 같은 장기 pre/post 깊이를 빌리고 독립 typed 경로는 기존 지역 삭제 본체를 유지한다.
        var destroy = _surfaceStorage is { } storage && storage.UsesNativeTransmissionDevices()
            ? storage.BaseDestroy() : new RawSquidDestroy(Pool, unpop, types);
        Func<IDisposable>? pin = _runtimePin;
        var runtime = new ProcessHostRuntimeBindings
        {
            Destroy = destroy, Factory = factory, PostPopDepth = () => TerrainBookkeeping.Depth,
            Assertion = (expression, file, line) => throw new InvalidOperationException($"프로세스 원본 진단: {expression} {file}:{line}"),
            Pin = pin is null ? null : () => pin(),
        };
        if (_sharedPlayerProcesses is null)
        {
            var current = new WeakReference<ProcessHostRuntimeBindings>(runtime);
            var clock = new SquidProcessState
            {
                SharedNow = ProcessTime, SharedAuthority = Gameplay.Authority, SharedPollType = GameAux.GeyserTypeBox,
                CurrentProcessFormType = GameAux.ProcessFormTypeBox, CurrentRegularType = GameAux.RegularProcessTypeBox,
                CurrentSharedRegularType = GameAux.SharedRegularProcessTypeBox, CurrentProcessTypeEnd = GameAux.ProcessTypeEndBox,
                CurrentFireAnimationType = GameAux.FireAnimationProcessTypeBox, CurrentRotateAnimationType = GameAux.RotateAnimationProcessTypeBox,
            };
            PlayerProcesses = new SquidProcessHost(Pool, types, Kernel, clock,
                () => current.TryGetTarget(out ProcessHostRuntimeBindings? selected) ? selected : null);
            _playerProcessClock = clock;
            _playerProcessCurrent = current;
        }
        else
        {
            _playerProcessUse = _sharedPlayerProcesses.Bind(runtime);
            PlayerProcesses = _sharedPlayerProcesses.Host();
            _playerProcessClock = _sharedPlayerProcesses.State();
        }
        _playerProcessRuntime = runtime;
        if (_surfaceStorage is { } nativeStorage && nativeStorage.UsesNativeTransmissionDevices())
            BindNativeProcessFormDeletion(nativeStorage, runtime);
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

    /// <summary>현재 Native Runtime의 알려진 ProcessForm 가상 삭제를 실제 Host 본체에 연결한다. 전체 자산 분배는 BindWorldDeletion이 뒤에 설치한다.</summary>
    private void BindNativeProcessFormDeletion(RawSurfaceStorage storage, ProcessHostRuntimeBindings runtime)
    {
        SquidProcessHost processes = PlayerProcesses;
        SquidDestroyHooks forms = processes.Hooks();
        SquidDestroyHooks assets = storage.DefaultProcessAssetHooks();
        runtime.Asset = new()
        {
            SelectedRaw = assets.SelectedRaw, Selected = assets.Selected, UnpopForm = assets.UnpopForm,
            Emit = effect =>
            {
                // 실제 전송/선택 정리는 이미 설치한 같은 Client 장치를 사용한다.
                if (effect.Effect is SquidDestroyEffect.Transmit or SquidDestroyEffect.ClearSelection)
                {
                    assets.Emit!(effect); return;
                }
                // 알려진 ProcessForm의 실제 가상 Pre/Post·부모 통지·삭제만 기존 Host가 수행한다.
                if (!processes.IsProcessForm(effect.Sid))
                    throw new InvalidOperationException("현재 Native Runtime의 비 ProcessForm 전체 가상 삭제 분배 미연결");
                forms.Emit!(effect);
            },
        };
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
        EnsureRuntimeCanRelease();
        // 장기Host의다른Runtime/Native잔류는보존하고현재대여가붙인프로세스만반납한다.
        if (!ProcessBindingsReturned())
        {
            if (_playerProcessUse is not null) _playerProcessUse.DrainOwnedForShutdown();
            else if (PlayerProcesses is not null) Kernel.DrainOwnedForShutdown(PlayerProcesses.Owns);
        }
        ReleaseDependencyWorld();
        ReleaseFlareWorld();
        ReleaseBurningWorld();
        ReleasePriestWorld();
        ReleaseProductionWorld();
        ReleaseSquidPacketDevices();
        if (_nativePlayerRegular is { } playerRegular) { _surfaceStorage!.ReleasePlayerRegularRuntime(playerRegular); _nativePlayerRegular = null; }
        _reservedPlayerRegular = null;
        _playerProcessUse?.Dispose(); _playerProcessUse = null;
        _playerProcessCurrent?.SetTarget(null!); _playerProcessCurrent = null;
        _playerProcessRuntime = null;
        _playerProcessClock = null;
    }
}
