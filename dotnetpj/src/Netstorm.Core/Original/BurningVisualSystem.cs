using Netstorm.Core.Display;

// 연소14의 단일 소유권과 공통 등록·Host/Form/Kernel·그리기 수명을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>실제 커널이 소유하는 원문 연소 파생 프로세스다.</summary>
public sealed class BurningVisualProcess : SquidProcess
{
    /// <summary>이 객체보다 오래 사는 같은 실제 시스템이다.</summary>
    private readonly BurningVisualSystem _system;
    /// <summary>현재 단일 캡처/입자/등록 필드다. 삭제 뒤 사용하지 않는다.</summary>
    public BurningVisualFields Fields { get; } = new();
    /// <summary>첫 timer가 쓸 두float다.</summary>
    internal BurningFrameFields Frame { get; } = new();
    /// <summary>미완료 파생 필드를 Kernel/Draw/삭제통지가 읽지 않게 한다.</summary>
    internal bool Ready { get; set; }
    /// <summary>실제 초기화/부착은 시스템의Add가 이어서 수행한다.</summary>
    public BurningVisualProcess(BurningVisualSystem system) => _system = system ?? throw new ArgumentNullException(nameof(system));
    /// <summary>원문+18 gate 뒤 선택된+24를 실행한다.</summary>
    public override void RunFrame() => _system.Run(this);
    /// <summary>소멸자 앞 원문487960의 현재 영역을 통지한다.</summary>
    public override void OnFormDestroy(uint flags) => _system.Removing(this);
    /// <summary>실제VT503730+1c의 빈ret12 통지다.</summary>
    public override void Notify(uint ev, uint info, uint value) { }
    /// <summary>Kernel이 소유권을 반납할 때 버퍼/소리/등록을 원문 순서로 정리한다.</summary>
    public override void Dispose() => _system.Destroy(this);
}
/// <summary>같은 현재 월드의 실제 시각·캡처·팔레트·픽셀/dirty 장치다.</summary>
public sealed class BurningSystemHooks
{
    /// <summary>실제 메모리/첫팔레트 장치다.</summary>
    public BurningVisualHooks Visual { get; set; } = new();
    /// <summary>같은 실제SHP 캡처/소리정지다.</summary>
    public BurningFrameHooks Frame { get; set; } = new();
    /// <summary>현재 Renderer 또는 보존 dirty큐의 소비자다.</summary>
    public Action<DisplayRect>? Invalidate { get; set; }
    /// <summary>그리기 진입에서만 읽는 실제 VFX dirty clip이다.</summary>
    public Func<DisplayRect>? Clip { get; set; }
    /// <summary>같은8비트 표면의 한 픽셀을 쓴다.</summary>
    public Action<int, int, byte>? Pixel { get; set; }
    /// <summary>버퍼/객체의 원문 반납을 관찰한다. 예외를 던지지 않아야 한다.</summary>
    public Action<BurningRelease>? Release { get; set; }
    /// <summary>중첩 함수묶음은 값사본으로 보관하고 실제 장치 owner는 빌린다.</summary>
    internal BurningSystemHooks Copy() => new() { Visual = Visual.Copy(), Frame = Frame.Copy(), Invalidate = Invalidate, Clip = Clip, Pixel = Pixel, Release = Release };
}
/// <summary>공통 등록에 연소14를 생성하고 실제Host/Kernel과 현재 출력장치로 실행/반납한다.</summary>
public sealed class BurningVisualSystem : IDisposable
{
    /// <summary>다른효과와 공통 등록/Host/배율 owner를 공유한다.</summary>
    private readonly RenderProcessSystem _rendering;
    /// <summary>실제 공통 게임난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>현재TYPE표를 빌린다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 시각/카메라/배율/바람 owner다.</summary>
    private readonly BurningFrameState _frameState;
    /// <summary>같은 공유 색 표 snapshot이다.</summary>
    private readonly BurningDrawPalette _palette;
    /// <summary>원문 constDWORD주소 대신 쓰는 같은 현재frame 조회다.</summary>
    private readonly Func<uint> _frameNumber;
    /// <summary>실제 장치의 함수 사본이다.</summary>
    private readonly BurningSystemHooks _hooks;
    /// <summary>프레임의 두dirty를 같은 현재장치에 전달한다.</summary>
    private readonly Sink _sink;
    /// <summary>원문 생성/Append 몸체다.</summary>
    private readonly RawBurningVisual _visual;
    /// <summary>현재HP/캡처의 원문 연소점 선택기다.</summary>
    private readonly RawBurningSources _sources;
    /// <summary>실제 소유권은pending 또는 Kernel에 있고 이 표는 조회만 보관한다.</summary>
    private readonly Dictionary<uint, BurningVisualProcess> _processes = [];
    /// <summary>실제 Host부착 전 단독 소유자다.</summary>
    private BurningVisualProcess? _pending;
    /// <summary>같은 시스템의 생성 재진입을 명시 거부한다.</summary>
    private bool _creating;
    /// <summary>현재 살아 있는 실제14 조회 수다.</summary>
    public int Size => _processes.Count;
    /// <summary>실제 프레임이 요청한flags0의 영역만 전달하는 sink다.</summary>
    private sealed class Sink : SquidDisplaySink
    {
        /// <summary>시스템이 보관한 같은 함수 사본이다.</summary>
        private readonly BurningSystemHooks _hooks;
        /// <summary>현재 장치 수명을 시스템과 공유한다.</summary>
        public Sink(BurningSystemHooks hooks) => _hooks = hooks;
        /// <summary>연소 프레임의 두SetRect는 억제 전역을 조회하지 않는다.</summary>
        public override bool Suppressed() => false;
        /// <summary>원문 프레임이 보낸 영역을 실제 dirty소비자로 넘긴다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => _hooks.Invalidate!(rect);
    }
    /// <summary>같은 실제월드/장치/판본을 확인한다. 표시배율 일치는 선택된Run에서 확인한다.</summary>
    public BurningVisualSystem(RenderProcessSystem rendering, GameRandom random, IReadOnlyList<OriginalTypeRecord> types,
        BurningVisualState visualState, BurningSourcesState sourcesState, BurningFrameState frameState, BurningDrawPalette palette, Func<uint> frameNumber, BurningSystemHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(rendering); ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frameState);
        ArgumentNullException.ThrowIfNull(palette); ArgumentNullException.ThrowIfNull(frameNumber); ArgumentNullException.ThrowIfNull(hooks);
        _rendering = rendering; _random = random; _types = types; _frameState = frameState; _palette = palette; _frameNumber = frameNumber; _hooks = hooks.Copy(); _sink = new(_hooks);
        _visual = new(rendering.Pool, rendering.Registry, visualState, _hooks.Visual); _sources = new(rendering.Pool, types, _visual, random, sourcesState);
        if (_hooks.Invalidate is null || _hooks.Clip is null || _hooks.Pixel is null || _hooks.Release is null || _hooks.Frame.StopSound is null || rendering.Pool.Edition != OriginalEdition.Patch1078)
            throw new ArgumentException("Burning 실제 시스템/장치/판본 누락");
        _ = new RawBurningFrame(rendering.Pool, types, _visual, _sources, random, frameState, _sink, _hooks.Frame);
    }
    /// <summary>등록→실제Host부착→150입자/첫팔레트 순서로 생성하고 실패한 단독소유권만 반납한다.</summary>
    public BurningVisualProcess Add(int victim, uint mode)
    {
        if (victim == 0 || _creating) throw new ArgumentException("Burning 실제 피해부모/중첩생성 오류");
        uint token = 0; _creating = true;
        try
        {
            _pending = new(this); BurningVisualProcess process = _pending;
            token = _rendering.Track(process.Fields.Registration, (fields, type, target, flags) =>
            {
                if (_pending is null || !ReferenceEquals(_pending.Fields.Registration, fields)) throw new InvalidOperationException("Burning 실제 부착필드 불일치");
                BurningVisualProcess transferring = _pending; _pending = null;
                SquidProcess attached = _rendering.Host.Attach(transferring, type, target, flags) ?? throw new InvalidOperationException("Burning 실제 부모부착 거부");
                fields.Form = attached.Form(); fields.Parent = attached.Parent();
            }, () => Draw(process));
            _processes.Add(token, process); _visual.Initialize(process.Fields, token, victim, mode); process.Ready = true; _rendering.Ready(token); _creating = false; return process;
        }
        catch
        {
            if (_pending is not null) { BurningVisualProcess pending = _pending; _pending = null; pending.Dispose(); }
            else if (_processes.TryGetValue(token, out BurningVisualProcess? process)) _rendering.Host.Kill(process, 0x10);
            _creating = false; throw;
        }
    }
    /// <summary>원문+18 뒤 선택된+24만 실행하며 현재 공통배율을 같은 프레임에 확인한다.</summary>
    internal void Run(BurningVisualProcess process)
    {
        if (!process.Ready || !RawBurningLifetimeDraw.FrameDue(process.Fields.Registration.Slot, _frameNumber())) return;
        if (_frameState.PixelScaleX != _rendering.State.ScaleX || _frameState.PixelScaleY != _rendering.State.ScaleY) throw new InvalidOperationException("Burning 현재 표시/프레임 배율 불일치");
        new RawBurningFrame(_rendering.Pool, _types, _visual, _sources, _random, _frameState, _sink, _hooks.Frame).Step(process.Fields, process.Frame);
    }
    /// <summary>현재상태 값사본을 먼저 고정한 뒤 실제clip을 조회해 원문5픽셀을 출력한다.</summary>
    private void Draw(BurningVisualProcess process)
    {
        if (!process.Ready) return;
        BurningFrameState state = _frameState.Copy(); state.Viewport = _hooks.Clip!(); RawBurningLifetimeDraw.Draw(process.Fields, state, _palette, _hooks.Pixel!);
    }
    /// <summary>초기화 완료 파생의 현재 삭제영역만 실제 dirty소비자로 통지한다.</summary>
    internal void Removing(BurningVisualProcess process)
    {
        if (!process.Ready) return;
        int[] rect = RenderProcessSystem.RemovalBounds(process.Fields.Registration, _frameState.CameraX, _frameState.CameraY); _hooks.Invalidate!(new(rect[0], rect[1], rect[2], rect[3]));
    }
    /// <summary>실제 소멸자에서 공유팔레트는 두고 버퍼/등록·단일현재조회만 제거한다.</summary>
    internal void Destroy(BurningVisualProcess process)
    {
        uint token = process.Fields.Registration.Token;
        if (process.Fields.Registration.Slot >= 0) RawBurningLifetimeDraw.Destroy(process.Fields, _rendering.Registry, 1, _hooks.Release!, _hooks.Frame.StopSound!);
        _processes.Remove(token); _rendering.Forget(token);
    }
    /// <summary>Host가 살아 있을 때 같은 실제client Form을 즉시반납하며 매번 현재 첫항목을 다시 읽는다.</summary>
    public void Dispose()
    {
        // 각 삭제가 현재조회표를 바꾸므로 미리 만든 배열을 반복하지 않는다.
        while (_processes.Count != 0) _rendering.Host.Kill(_processes.First().Value, 0x10);
    }
}
