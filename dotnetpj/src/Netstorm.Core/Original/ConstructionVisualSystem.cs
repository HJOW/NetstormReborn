// 원본 건설 표시 본체를 같은 실제 Kernel·Form·표시 목록의 소유권에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>Kernel이 소유하고 생성 완료 뒤에만 실행/출력하는 건설 표시 프로세스다.</summary>
public sealed class ConstructionVisualProcess : SquidProcess
{
    /// <summary>이 프로세스보다 오래 유지할 같은 수명 관리자다.</summary>
    private readonly ConstructionVisualSystem _system;
    /// <summary>이 객체가 소유하는 픽셀·입자·시간 상태다.</summary>
    public ConstructionFrameFields Fields { get; } = new();
    /// <summary>같은 객체의 원본 표시 등록 필드다.</summary>
    public RenderProcessFields Registration { get; } = new();
    /// <summary>캡처·초기화가 끝난 뒤에만 프레임과Draw를 허용한다.</summary>
    internal bool Ready { get; set; }
    /// <summary>실제 생성과 부착은System.Add가 수행한다.</summary>
    public ConstructionVisualProcess(ConstructionVisualSystem system) => _system = system ?? throw new ArgumentNullException(nameof(system));
    /// <summary>자기 삭제 뒤 객체를 다시 읽지 않고 실제 프레임에 전달한다.</summary>
    public override void RunFrame() => _system.Run(this);
    /// <summary>출력 경계가 자기 수명을 바꿔도 이후 멤버를 다시 읽지 않는다.</summary>
    public void Draw() => _system.DrawProcess(this);
    /// <summary>Kernel이 반납할 때 소유 버퍼와 원본 등록·토큰을 정리한다.</summary>
    public override void Dispose() => _system.Destroy(this);
}

/// <summary>생성·실행·출력·소멸 본체를 실제 Host와 공통 표시 저장소에 묶는다.</summary>
/// <remarks>근거: cpppj/src/o/ConstructionVisualSystem.cpp·h. 2026-11-20 0.675 이식.</remarks>
public sealed class ConstructionVisualSystem : IDisposable
{
    /// <summary>실패 정리·종료 때 client Form 삭제 권한을 명시한다.</summary>
    private const uint ClientKill = 0x10;
    /// <summary>다른 효과와 공유하거나 단독으로 소유하는 같은 표시 저장소다.</summary>
    private readonly RenderProcessSystem _rendering;
    /// <summary>매 프레임 현재 시각·카메라다.</summary>
    private readonly ConstructionFrameState _frameState;
    /// <summary>생성과 실행이 같은 토큰으로 읽는 현재 팔레트다.</summary>
    private readonly ConstructionFramePalette _palette;
    /// <summary>다른 게임 계산과 공유하는 실제 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>생성의 장치 대리자 사본이다.</summary>
    private readonly ConstructionVisualHooks _visualHooks;
    /// <summary>실행·출력의 장치 대리자 사본이다.</summary>
    private readonly ConstructionFrameHooks _frameHooks;
    /// <summary>원본 반납 순서를 관찰한다. 예외나 수명 변경을 일으키지 않아야 한다.</summary>
    private readonly Action<ConstructionRelease> _release;
    /// <summary>소유권은pending 또는Kernel에 있고 이 표는 현재 조회만 보관한다.</summary>
    private readonly Dictionary<uint, ConstructionVisualProcess> _processes = [];
    /// <summary>실제 Host부착 전 생성 중인 객체의 단독 소유권이다.</summary>
    private ConstructionVisualProcess? _pending;
    /// <summary>경계 중 같은 시스템에 대한 중첩Add를 막는다.</summary>
    private bool _creating;

    /// <summary>단독 표시 저장소를 만들며 실제Host·풀·배율은 그대로 공유한다.</summary>
    public ConstructionVisualSystem(RawSidPool pool, SquidProcessHost host, ConstructionVisualState visualState,
        ConstructionFrameState frameState, ConstructionFramePalette palette, GameRandom random,
        RenderRegistryHooks registryHooks, ConstructionVisualHooks visualHooks, ConstructionFrameHooks frameHooks, Action<ConstructionRelease> release)
        : this(new RenderProcessSystem(pool, host, visualState, registryHooks), frameState, palette, random, visualHooks, frameHooks, release) { }

    /// <summary>다른 효과와 같은 목록·토큰·Host를 공유하며 선택 장치만 본체의 실제 연결로 바꾼다.</summary>
    public ConstructionVisualSystem(RenderProcessSystem rendering, ConstructionFrameState frameState,
        ConstructionFramePalette palette, GameRandom random, ConstructionVisualHooks visualHooks, ConstructionFrameHooks frameHooks, Action<ConstructionRelease> release)
    {
        ArgumentNullException.ThrowIfNull(rendering); ArgumentNullException.ThrowIfNull(frameState); ArgumentNullException.ThrowIfNull(palette);
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(visualHooks); ArgumentNullException.ThrowIfNull(frameHooks); ArgumentNullException.ThrowIfNull(release);
        _rendering = rendering; _frameState = frameState; _palette = palette; _random = random;
        _visualHooks = visualHooks.Copy(); _frameHooks = frameHooks.Copy(); _release = release;
        ConstructionVisualHooks visualCheck = _visualHooks.Copy(); visualCheck.Attach = (_, _) => { };
        _ = new RawConstructionVisual(Pool, _rendering.State, _random, visualCheck);
        ConstructionFrameHooks frameCheck = _frameHooks.Copy(); frameCheck.Kill = _ => { };
        _ = new RawConstructionFrame(Pool, _rendering.State, _frameState, _palette, _random, frameCheck);
    }

    /// <summary>현재 같은 풀이다.</summary>
    public RawSidPool Pool => _rendering.Pool;
    /// <summary>공통 수집·정렬·출력이 사용하는 같은 저장소다.</summary>
    public RenderProcessSystem Rendering => _rendering;
    /// <summary>생성 중 객체를 포함한 현재 조회 수다.</summary>
    public int Size => _processes.Count;
    /// <summary>원본 빈 칸과40칸 확장을 유지한 현재 목록이다.</summary>
    public ReadOnlySpan<uint> Slots => _rendering.Slots;

    /// <summary>목록 등록→client Form 부착→캡처/입자 생성 순서이며 실패한 새 수명만 정리한다.</summary>
    public ConstructionVisualProcess Add(int parent, bool instant)
    {
        if (_creating) throw new InvalidOperationException("건설 표시 중첩 생성은 지원하지 않습니다");
        uint token = 0; _creating = true;
        try
        {
            _pending = new(this); ConstructionVisualProcess process = _pending;
            token = _rendering.Track(process.Registration, (fields, type, target, flags) =>
            {
                if (_pending is null || !ReferenceEquals(_pending.Registration, fields)) throw new InvalidOperationException("건설 표시 부착 객체 불일치");
                ConstructionVisualProcess transferring = _pending; _pending = null;
                SquidProcess attached = _rendering.Host.Attach(transferring, type, target, flags) ?? throw new InvalidOperationException("건설 표시 부모 부착 실패");
                fields.Form = attached.Form(); fields.Parent = attached.Parent();
            }, process.Draw);
            _processes.Add(token, process);
            var visual = new RawConstructionVisual(Pool, _rendering.State, _random,
                RawConstructionLifetime.MakeRegistryHooks(_rendering.Registry, process.Registration, token, _visualHooks));
            ConstructionVisualFields fields = visual.Create(parent, instant);
            if (!_processes.ContainsKey(token)) throw new InvalidOperationException("건설 표시 생성 중 객체 삭제");
            process.Fields.Visual = fields; Synchronize(process.Registration, fields);
            process.Ready = true; _rendering.Ready(token); _creating = false; return process;
        }
        catch
        {
            if (_pending is not null) { ConstructionVisualProcess pending = _pending; _pending = null; pending.Dispose(); }
            else if (_processes.TryGetValue(token, out ConstructionVisualProcess? process)) _rendering.Host.Kill(process, ClientKill);
            _creating = false; throw;
        }
    }

    /// <summary>한 프레임의 실제 종료 콜백만 만들며 자기 삭제 콜백을 프로세스 필드에 저장하지 않는다.</summary>
    private ConstructionFrameHooks FrameHooks(ConstructionVisualProcess process)
    {
        ConstructionFrameHooks hooks = _frameHooks.Copy(); hooks.Kill = flags => _rendering.Host.Kill(process, flags); return hooks;
    }
    /// <summary>초기화 뒤에만 실행하며 삭제되지 않은 현재 토큰의 등록 영역만 갱신한다.</summary>
    internal void Run(ConstructionVisualProcess process)
    {
        if (!process.Ready) return; uint token = process.Registration.Token;
        new RawConstructionFrame(Pool, _rendering.State, _frameState, _palette, _random, FrameHooks(process)).Run(process.Fields);
        if (_processes.TryGetValue(token, out ConstructionVisualProcess? current) && ReferenceEquals(current, process)) Synchronize(current.Registration, current.Fields.Visual);
    }
    /// <summary>초기화 완료 버퍼만 제출하고 장치 호출 뒤 객체를 다시 읽지 않는다.</summary>
    internal void DrawProcess(ConstructionVisualProcess process)
    {
        if (!process.Ready) return; ConstructionFrameHooks hooks = _frameHooks.Copy(); hooks.Kill = _ => { };
        new RawConstructionFrame(Pool, _rendering.State, _frameState, _palette, _random, hooks).Draw(process.Fields);
    }
    /// <summary>출력 중 확장·삭제에도 매 차례 현재 슬롯을 다시 읽는다.</summary>
    public void Draw() => _rendering.Draw();
    /// <summary>원본 수집기에 같은 목록의 완료된 현재 항목만 제공한다.</summary>
    public int Collect(RenderWorldRect view, Span<RenderCollectionEntry> output, int limit) => _rendering.Collect(view, output, limit);
    /// <summary>수집 시점의kind3 토큰만 제출하며 새 객체를 이전 목록에 섞지 않는다.</summary>
    public void DrawCollected(ReadOnlySpan<RenderCollectionEntry> entries)
    {
        // 항목을복사하지않고같은수집스냅샷의순서를유지한다.
        foreach (RenderCollectionEntry entry in entries)
        {
            if (entry.Kind != 3) throw new ArgumentException("건설 표시 수집 항목 종류 오류", nameof(entries));
            DrawToken(entry.Token);
        }
    }
    /// <summary>공통kind3 분배는 현재 살아 있는 토큰만 제출한다.</summary>
    public void DrawToken(uint token) => _rendering.DrawToken(token);
    /// <summary>실제 소멸에서 소유 버퍼·등록·현재 토큰 조회를 같은 순서로 반납한다.</summary>
    internal void Destroy(ConstructionVisualProcess process)
    {
        uint token = process.Registration.Token;
        if (process.Registration.Slot >= 0) RawConstructionLifetime.Destroy(process.Fields.Visual, process.Registration, _rendering.Registry, 1, _release);
        _processes.Remove(token); _rendering.Forget(token);
    }
    /// <summary>Host가 살아 있을 때 남은Form을 모두 삭제해Kernel에시스템참조가남지않게한다.</summary>
    public void Dispose()
    {
        // 각Kill이조회표를바꾸므로매번현재첫항목을다시읽는다.
        while (_processes.Count != 0) _rendering.Host.Kill(_processes.First().Value, ClientKill);
    }
    /// <summary>같은월드의Start가표시를붙인뒤현재시각을읽도록연결한다.</summary>
    public ConstructionProcessHooks MakeProcessHooks(RawSidPool pool, ConstructionProcessHooks hooks)
    {
        if (!ReferenceEquals(pool, Pool)) throw new ArgumentException("건설 표시 시작의 풀 연결 오류");
        ConstructionProcessHooks result = hooks.Copy(); result.Effect = (parent, instant) => { _ = Add(parent, instant); }; return result;
    }
    /// <summary>프레임이바꾼영역과깊이를같은RenderProcess등록에반영한다.</summary>
    private static void Synchronize(RenderProcessFields registration, ConstructionVisualFields visual)
    {
        registration.Left = visual.Left; registration.Top = visual.Top; registration.Right = visual.Right; registration.Bottom = visual.Bottom; registration.ZOrder = visual.ZOrder;
    }
}
