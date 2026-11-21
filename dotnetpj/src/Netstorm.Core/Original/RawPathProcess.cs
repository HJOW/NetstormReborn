// 실제 같은 Pool/hash/spot/Kernel의 경로와 자기 삭제 수명을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPathProcess.cpp의 실제8종 경로 실행 객체다.</summary>
public sealed partial class RawPathProcess : SquidProcess
{
    /// <summary>생성 때 지정한 같은 실제 Pool이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 현재 월드의 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재TYPE 속성 표의 실제 뷰다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재TYPE별 실제 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>현재TYPE별 실제 SHP 프레임이다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>같은 현재65536칸의 점유 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>같은 시계/권한/역할/진단 전역이다.</summary>
    private readonly RawPathProcessState _state;
    /// <summary>원문 값형으로 복사한 실제 외부 함수다.</summary>
    private readonly RawPathProcessHooks _hooks;
    /// <summary>실제 사제 가상 클래스를 선택한다.</summary>
    private readonly bool _priest;
    /// <summary>실제 비행39 가상 클래스를 선택한다.</summary>
    private readonly bool _flying;
    /// <summary>실제 웜40 가상 클래스를 선택한다.</summary>
    private readonly bool _worm;
    /// <summary>실제 벌룬42 가상 클래스를 선택한다.</summary>
    private readonly bool _balloon;
    /// <summary>실제 Dude24 가상 클래스를 선택한다.</summary>
    private readonly bool _dude;
    /// <summary>실제 벌프41 가상 클래스를 선택한다.</summary>
    private readonly bool _bulf;
    /// <summary>실제 워커38 가상 클래스를 선택한다.</summary>
    private readonly bool _walker;
    /// <summary>원문 객체 밖에서 자기 삭제 뒤 읽는 같은 수명 관찰이다.</summary>
    private readonly PathLifeWatch _watch = new();
    /// <summary>서버 전송/상위가 같은 실제 Pool을 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>탐색/수신/시각/프레임이 함께 쓰는 유일한 raw 경로다.</summary>
    public RawPathRuntime Runtime { get; }
    /// <summary>표/지도/필수 가상 장치를 확인하고 생성만 하며 Host에는 아직 붙이지 않는다.</summary>
    public RawPathProcess(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<SquidDisplayShape> shapes, byte[] spots, RawPathProcessState state, RawPathProcessHooks hooks,
        bool priest, bool flying = false, bool worm = false, bool balloon = false, bool dude = false, bool bulf = false, bool walker = false)
    {
        _pool = pool; _hash = hash; _types = types; _frames = frames; _shapes = shapes; _spots = spots; _state = state; _hooks = hooks with { };
        _priest = priest; _flying = flying; _worm = worm; _balloon = balloon; _dude = dude; _bulf = bulf; _walker = walker;
        Runtime = new(pool.Edition, priest, 0, flying, worm, balloon, dude, bulf, walker);
        if (types.Count != frames.Count || types.Count != shapes.Count || spots.Length != 65536 || hooks.Unpop is null || hooks.Repop is null || hooks.Pop is null || hooks.Halt is null || hooks.Arrival is null || hooks.SoundId is null || hooks.StepSound is null || hooks.ClearBlocked is null || hooks.Cleanup is null) throw new ArgumentException("경로 월드 연결 누락");
    }
    /// <summary>원문 destructor처럼 외부 watch를 끄며 GC 시점에 의존하지 않는다.</summary>
    public override void Dispose() { _watch.Alive = false; base.Dispose(); }
    /// <summary>현재 실제 가상 클래스가 사용하는 역할 번호다. raw TYPE로 클래스를 추측하지 않는다.</summary>
    public uint MovementType() => _walker ? _state.WalkerProcess : _bulf ? _state.BulfProcess : _dude ? _state.DudeProcess : _balloon ? _state.BalloonProcess : _worm ? _state.WormProcess : _flying ? _state.FlyProcess : _priest ? _state.PriestProcess : _state.TeleportProcess;
    /// <summary>같은 Pool/부모 종류를 확인하고 해당 파생의 기존 프로세스 제거 뒤 실제 Host에 소유권을 넘긴다.</summary>
    public static RawPathProcess? Attach(SquidProcessHost host, RawPathProcess process, int parent, uint flags = 0, int form = 0)
    {
        if (process is null) throw new ArgumentException("경로 부착 Pool 불일치"); bool transferred = false;
        try
        {
            if (!ReferenceEquals(host.Pool(), process._pool)) throw new ArgumentException("경로 부착 Pool 불일치"); byte kind = process._pool.Slot(parent)[10];
            if (process._priest && kind != process._state.PriestType) throw new ArgumentException("사제 경로 부모TYPE");
            if (process._dude && kind != process._state.DudeType) throw new ArgumentException("Dude24 경로 부모TYPE");
            if (process._worm && kind != process._state.SunWalkerType && kind != process._state.EmptyType) throw new ArgumentException("웜40 부모TYPE");
            uint type = process.MovementType(); if (process._flying || process._balloon || process._walker) RawProcessRemoval.RemoveAttached(host, parent, type);
            transferred = true; return (RawPathProcess?)host.Attach(process, type, parent, flags, form);
        }
        finally
        {
            // 원문 unique_ptr 인자의 소유권을 Host에 넘기기 전 실패하면 즉시 파생 수명을 닫는다.
            if (!transferred) process.Dispose();
        }
    }
    /// <summary>실제 Host 부착 뒤 같은 base Form/부모 DWORD를 동기화한다.</summary>
    public override void OnAttached() => Runtime.SetAttachment((ushort)Form(), (ushort)Parent());
    /// <summary>선택한 실제 함수만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("경로의 선택된 실제 경계 미연결");
    /// <summary>같은 부착 Host를 빌리고 미부착 선택을 명시적으로 드러낸다.</summary>
    private SquidProcessHost CurrentHost() => Host() ?? throw new InvalidOperationException("미부착 경로 Host 호출");
    /// <summary>실제 비행 Init 또는 지면이 있을 때의 Pop→BFS→current0을 실행한다.</summary>
    public void InitializePath(int parent, RawPathPoint destination, RawPathPoint start, uint flags = 0)
    {
        if (_flying || _balloon) { Runtime.InitializeFlight(RawPathSlot.Position(_pool, parent), destination, _balloon ? 44 : 0, _balloon ? _pool.Slot(parent)[30] : (byte)0); return; }
        if (RawPathSurface.SurfaceAt(_hash, start) == 0) return;
        Action<RawPathRuntime, int, RawPathPoint, RawPathPoint, uint> search = Need(_hooks.Search);
        if ((_pool.Slot(parent)[11] & 4) != 0) _hooks.Pop!(parent, start, 0); search(Runtime, parent, start, destination, _bulf || _walker ? 1u : flags); Runtime.ResetCurrent();
    }
    /// <summary>PATCH만 nonzero override를 사용하고 같은 실제 시계를 두 원문 위치에서 읽는다.</summary>
    public void Prepare(float speedOverride, float speed, float turningSpeed)
    {
        if (speed == 0 || turningSpeed == 0) throw new ArgumentException("경로TYPE 속도0"); float effective = _pool.Edition == OriginalEdition.Patch1078 && speedOverride != 0 ? speedOverride : speed;
        Runtime.Prepare(_state.Clock.Now, _state.Clock.Now, effective, turningSpeed, Runtime.Direction(0));
    }
    /// <summary>현재 boss 또는 원문 client Form 범위를 판정한다.</summary>
    private bool MayMutate() => _state.Clock.Boss || (Form() > 4 && Form() < (_pool.Edition == OriginalEdition.Patch1078 ? RawSidPool.ServerFirst : RawSidPool.CdServerFirst));
    /// <summary>PATCH 현재 두 SID 진단의 항상참과 CD 기존 Host/생존 판정을 구별한다.</summary>
    private bool Robust()
    {
        if (_pool.Edition == OriginalEdition.Patch1078) return RawPathArrivalParent.Robust(new() { Form = () => (uint)Runtime.ReadBits(4), Parent = () => (uint)Runtime.ReadBits(8), Capacity = () => (uint)_pool.Capacity, Assertion = _hooks.Assertion });
        return Host() is { } host && host.IsRegistered(Pid(), this) && Form() != 0 && (_pool.Slot(Form())[11] & 3) == 0 && Parent() != 0 && (_pool.Slot(Parent())[11] & 3) == 0;
    }
    /// <summary>PATCH의 현재 진단/토큰/자기 삭제와 CD의 원문 반환 계약을 유지한다.</summary>
    private bool Halt()
    {
        PathLifeWatch watch = _watch;
        if (_pool.Edition == OriginalEdition.Patch1078) return RawPathHaltParent.Run(_state.Checking, new()
        {
            Parent = Parent, Halt = _hooks.Halt, Alive = () => watch.Alive, Assertion = _hooks.Assertion,
            ForcedStop = parent => (_pool.Slot(parent)[40] & 0x20) != 0, MayKill = MayMutate, Kill = () => CurrentHost().Kill(this, 0),
            Boss = () => _state.Clock.Boss, Carrier = parent => (_types[_pool.Slot(parent)[10]].Flags2 & 0x30000) != 0, HasTargets = () => Runtime.TargetB != 0 || Runtime.TargetA != 0,
            Retarget = parent => Need(_hooks.Retarget)(Runtime, parent), ArmWatch = () => Runtime.DeathWatchToken = 1, ClearWatch = () => Runtime.DeathWatchToken = 0,
        });
        int savedParent = Parent(); uint result = _hooks.Halt!(savedParent);
        if (!watch.Alive) { if (result != 12) throw new InvalidOperationException("경로 Halt 자기 삭제 반환값"); return true; }
        if (result == 10 && (_pool.Slot(savedParent)[35] & 0x20) != 0) result = 11;
        if (result == 11) { if (MayMutate()) CurrentHost().Kill(this, 0); return true; }
        int type = _pool.Slot(savedParent)[10];
        if (_state.Clock.Boss && (_types[type].Flags2 & 0x30000) != 0 && (Runtime.TargetA != 0 || Runtime.TargetB != 0)) { Need(_hooks.Retarget)(Runtime, savedParent); if (!watch.Alive) return true; }
        return result != 10;
    }
    /// <summary>허용TYPE→편향/절삭 spot6→권한→같은 일반 finder의 Dais 순서로 길 밖 정지를 처리한다.</summary>
    private bool GroundStop(RawPathPoint point)
    {
        byte type = _pool.Slot(Parent())[10]; int count = _pool.Edition == OriginalEdition.Patch1078 ? 5 : 4;
        if (Array.IndexOf(_state.GroundTypes, (uint)type, 0, count) < 0) return false;
        int cx = RawPathSlot.Cell(point.X, true), cy = RawPathSlot.Cell(point.Y, true), fx = RawPathSlot.Cell(point.X, false), fy = RawPathSlot.Cell(point.Y, false);
        if ((_spots[cy * 256 + cx] & 6) != 0 && (_spots[fy * 256 + fx] & 6) != 0) return false;
        if (MayMutate())
        {
            var finder = new RawSquidFinder(_pool, _hash, _types); int candidate = finder.Begin(new(cx - 1, cy - 1, cx + 1, cy + 1), 1);
            // 원문 finder 순서에서 현재 Dais 부류만 확인한다.
            while (candidate != 0) { if ((_types[_pool.Slot(candidate)[10]].Flags2 & 0x400000) != 0) return false; candidate = finder.Next(); }
            CurrentHost().Kill(this, 0); return true;
        }
        return false;
    }
    /// <summary>실제 같은 일반 finder/spot8의 현재 마스크로 도착 겹침을 조회한다.</summary>
    private bool Blocked(RawPathPoint point) => RawPathArrivalParent.Blocked(_pool, _hash, _types, _spots, _state.ReadBlockingGenus(), point);
    /// <summary>실제 공간 접두 뒤 같은 Runtime으로 예약/의식/대상 정리를 호출한다.</summary>
    public override void OnFormDestroy(uint flags)
    {
        int parent = Parent(); if (MayMutate() && (_pool.Slot(parent)[11] & 2) == 0) { _hooks.Unpop!(parent, 0); _hooks.Repop!(parent, 0); } _hooks.Cleanup!(Runtime, parent, flags);
    }
}
