// 실제process16과더미감시쌍을다른효과와같은Host/Kernel/목록에연결한다.
namespace Netstorm.Core.Original;

/// <summary>소유권이pending에서실제Kernel로한번만넘어가는파편프로세스다.</summary>
public sealed class ShrapnelVisualProcess : SquidProcess
{
    /// <summary>이객체보다오래유지되는같은시스템이다.</summary>
    private readonly ShrapnelVisualSystem _system;
    /// <summary>등록·bitmap·입자·시각의단일소유저장소다.</summary>
    public ShrapnelFrameFields Fields { get; } = new();
    /// <summary>부착전예외에도현재소유한더미를찾는다.</summary>
    internal int Dummy;
    /// <summary>실제상호의존쌍구성이완료됐는지표시한다.</summary>
    internal bool Paired;
    /// <summary>파생초기화완료뒤에만Frame/Draw를허용한다.</summary>
    internal bool Ready;
    /// <summary>같은시스템을빌리며실제생성은Add가수행한다.</summary>
    public ShrapnelVisualProcess(ShrapnelVisualSystem system) => _system = system ?? throw new ArgumentNullException(nameof(system));
    /// <summary>별도프레임목록없이같은Kernel에서실행한다.</summary>
    public override void RunFrame() => _system.Run(this);
    /// <summary>메모리반납전에현재공통삭제영역을통지한다.</summary>
    public override void OnFormDestroy(uint flags) => _system.Removing(this);
    /// <summary>공통kind3이현재캡처색을출력한다.</summary>
    public void Draw() => _system.Draw(this);
    /// <summary>실제소유버퍼·공통등록·미부착더미를정리한다.</summary>
    public override void Dispose() => _system.Destroy(this);
}

/// <summary>실제수명/메모리/픽셀경계다.생성/프레임계산은시스템본체가수행한다.</summary>
public sealed class ShrapnelSystemHooks
{
    /// <summary>attach/prepare는시스템이실제로연결하는월드장치다.</summary>
    public ShrapnelLifetimeHooks Lifetime { get; set; } = new();
    /// <summary>소진은같은Host로연결하는실제표시장치다.</summary>
    public ShrapnelFrameHooks Frame { get; set; } = new();
    /// <summary>반납순서를관찰하며실제저장소는수명본체가반납한다.</summary>
    public Action<ShrapnelRelease>? Release { get; set; }
    /// <summary>실제쌍구성전에실패한더미의전체가상삭제다.</summary>
    public Action<int>? DestroyUnpaired { get; set; }
}

/// <summary>모든효과와같은표시registry·Host·게임난수·CRT를공유한다.</summary>
public sealed class ShrapnelVisualSystem : IDisposable
{
    /// <summary>실패정리/종료에쓰는clientForm삭제권한이다.</summary>
    private const uint ClientKill = 16;
    /// <summary>같은목록·풀·Host다.</summary>
    private readonly RenderProcessSystem _rendering;
    /// <summary>게임전체의같은난수다.</summary>
    private readonly GameRandom _game;
    /// <summary>별도TLS상태를공유하는CRT난수다.</summary>
    private readonly CrtRandom _crt;
    /// <summary>현재시각·카메라·배율이다.</summary>
    private readonly FieryTailFrameState _frame;
    /// <summary>실제월드경계의함수사본이다.</summary>
    private readonly ShrapnelLifetimeHooks _lifetimeHooks;
    /// <summary>실제메모리/표시장치의함수사본이다.</summary>
    private readonly ShrapnelFrameHooks _frameHooks;
    /// <summary>반납관찰의같은소비자다.</summary>
    private readonly Action<ShrapnelRelease> _release;
    /// <summary>쌍구성전더미의실제가상삭제다.</summary>
    private readonly Action<int> _destroyUnpaired;
    /// <summary>소유권은pending/Kernel에있으며이표는조회만한다.</summary>
    private readonly Dictionary<uint, ShrapnelVisualProcess> _processes = [];
    /// <summary>실제부착전임시단독소유권이다.</summary>
    private ShrapnelVisualProcess? _pending;
    /// <summary>중첩Add가현재pending을덮어쓰지못하게한다.</summary>
    private bool _creating;

    /// <summary>실제세계/출력경계와같은배율을첫등록전에확인한다.</summary>
    public ShrapnelVisualSystem(RenderProcessSystem rendering, GameRandom game, CrtRandom crt, FieryTailFrameState frame, ShrapnelSystemHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(rendering); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(crt); ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(hooks);
        ShrapnelLifetimeHooks lifetime = hooks.Lifetime;
        if (lifetime.Reserve is null || lifetime.Create is null || lifetime.Pop is null || lifetime.Depend is null || lifetime.Capture is null || hooks.Release is null || hooks.DestroyUnpaired is null)
            throw new ArgumentException("파편 실제 세계/반납 경계 누락");
        _rendering = rendering; _game = game; _crt = crt; _frame = frame; _lifetimeHooks = lifetime.Copy(); _frameHooks = hooks.Frame.Copy(); _release = hooks.Release; _destroyUnpaired = hooks.DestroyUnpaired;
        CheckScale(); _ = new RawShrapnelFrame(_rendering.Pool, _game, _crt, _frame, FrameHooks(null));
    }
    /// <summary>다른종류를합산하지않은현재파편수다.</summary>
    public int Size => _processes.Count;
    /// <summary>같은전역배율로출력하는지확인한다.</summary>
    private void CheckScale()
    {
        if (_frame.PixelScaleX != _rendering.State.ScaleX || _frame.PixelScaleY != _rendering.State.ScaleY) throw new InvalidOperationException("파편 공통 표시 배율 불일치");
    }
    /// <summary>같은pending을실제Host로넘기고현재객체의실제준비를연결한다.</summary>
    private ShrapnelLifetimeHooks LifetimeHooks(ShrapnelVisualProcess process)
    {
        ShrapnelLifetimeHooks hooks = _lifetimeHooks.Copy();
        hooks.Attach = (fields, type, parent, flags) =>
        {
            if (!ReferenceEquals(_pending, process) || !ReferenceEquals(fields, process.Fields.Registration)) throw new InvalidOperationException("파편 실제 부착 객체 불일치");
            ShrapnelVisualProcess transferring = _pending!; _pending = null;
            SquidProcess attached = _rendering.Host.Attach(transferring, type, parent, flags) ?? throw new InvalidOperationException("파편 실제 더미 부착 실패");
            fields.Form = attached.Form(); fields.Parent = attached.Parent();
        };
        hooks.Depend = (form, dummy) => { _lifetimeHooks.Depend!(form, dummy); process.Paired = true; };
        hooks.Prepare = (fields, kind) => new RawShrapnelFrame(_rendering.Pool, _game, _crt, _frame, FrameHooks(process)).Prepare(fields, kind);
        return hooks;
    }
    /// <summary>소진을같은Form/Kernel의실제삭제로연결한다.</summary>
    private ShrapnelFrameHooks FrameHooks(ShrapnelVisualProcess? process)
    {
        ShrapnelFrameHooks hooks = _frameHooks.Copy();
        hooks.RemoveForm = () => { if (process is null) throw new InvalidOperationException("파편 소진 객체 미지정"); _rendering.Host.Kill(process, 0); };
        return hooks;
    }
    /// <summary>공통목록→더미/부착/쌍→캡처/준비다.104바이트실패는null이며Ready는마지막에세운다.</summary>
    public ShrapnelVisualProcess? Add(int source, ShrapnelKind kind)
    {
        if (source == 0 || _creating) throw new ArgumentException("파편 source/중첩 생성 오류");
        CheckScale(); if (!_lifetimeHooks.Reserve!(104)) return null;
        _creating = true; uint token = 0;
        try
        {
            _pending = new(this); ShrapnelVisualProcess process = _pending; ShrapnelLifetimeHooks lifetimeHooks = LifetimeHooks(process);
            token = _rendering.Track(process.Fields.Registration, lifetimeHooks.Attach!, process.Draw); _processes.Add(token, process);
            new RawShrapnelLifetime(_rendering.Pool, _rendering.Registry, lifetimeHooks).Initialize(process.Fields, token, source, kind, ref process.Dummy);
            if (!_processes.ContainsKey(token)) throw new InvalidOperationException("파편 생성 중 현재 객체 삭제");
            process.Ready = true; _rendering.Ready(token); _creating = false; return process;
        }
        catch
        {
            if (_pending is not null) { ShrapnelVisualProcess pending = _pending; _pending = null; pending.Dispose(); }
            else if (_processes.TryGetValue(token, out ShrapnelVisualProcess? current)) _rendering.Host.Kill(current, ClientKill);
            _creating = false; throw;
        }
    }
    /// <summary>자기삭제뒤현재객체를다시읽지않는다.</summary>
    internal void Run(ShrapnelVisualProcess process)
    {
        if (!process.Ready) return; CheckScale(); new RawShrapnelFrame(_rendering.Pool, _game, _crt, _frame, FrameHooks(process)).Step(process.Fields);
    }
    /// <summary>현재캡처bitmap과같은표면장치로출력한다.</summary>
    internal void Draw(ShrapnelVisualProcess process)
    {
        if (!process.Ready) return; CheckScale(); new RawShrapnelFrame(_rendering.Pool, _game, _crt, _frame, FrameHooks(process)).Draw(process.Fields);
    }
    /// <summary>프레임dirty와구별한공통삭제영역의정수화를유지한다.</summary>
    internal void Removing(ShrapnelVisualProcess process)
    {
        if (!process.Ready) return;
        int[] rect = RenderProcessSystem.RemovalBounds(process.Fields.Registration, _frame.CameraX, _frame.CameraY); _frameHooks.Invalidate!(new(rect[0], rect[1], rect[2], rect[3]));
    }
    /// <summary>버퍼/목록/조회뒤쌍구성전더미만정리한다.쌍이있으면실제Form삭제사슬에맡긴다.</summary>
    internal void Destroy(ShrapnelVisualProcess process)
    {
        uint token = process.Fields.Registration.Token;
        if (process.Fields.Registration.Slot >= 0) new RawShrapnelLifetime(_rendering.Pool, _rendering.Registry, LifetimeHooks(process)).Destroy(process.Fields, 1, _release);
        _processes.Remove(token); _rendering.Forget(token);
        if (process.Dummy != 0 && !process.Paired && (_rendering.Pool.Slot(process.Dummy)[11] & 3) == 0) _destroyUnpaired(process.Dummy);
    }
    /// <summary>같은Host가살아있을때잔여clientForm을정리한다.</summary>
    public void Dispose()
    {
        // 각삭제가조회표를바꾸므로현재첫항목을다시찾는다.
        while (_processes.Count != 0) _rendering.Host.Kill(_processes.First().Value, ClientKill);
    }
}
