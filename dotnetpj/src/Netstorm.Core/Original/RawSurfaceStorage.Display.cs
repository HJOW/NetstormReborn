using Netstorm.Core.Display;

// 현재 Client 장치와 짧은 World 표시 대여를 같은 장기 실행기에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 카메라/dirty/메타 장치와 실제 호출 수명만 보관한다.</summary>
public sealed class RawSurfaceDisplayBindings
{
    /// <summary>현재 World의 기존 논리 변경 관찰이다. World가 없으면 설치하지 않는다.</summary>
    public Action? Changed { get; set; }
    /// <summary>현재 카메라/viewport 값 조회다.</summary>
    public Func<SquidDisplayView>? View { get; set; }
    /// <summary>현재 전체 갱신 억제 관문이다.</summary>
    public Func<bool>? Suppressed { get; set; }
    /// <summary>실제 계산된 main/shadow 영역을 같은 dirty표에 전달한다.</summary>
    public Action<DisplayRect, uint>? Invalidate { get; set; }
    /// <summary>확인된 현재 메타/Sprite 가상88 전체 갱신이다.</summary>
    public Action<ReadOnlySpan<byte>>? MetaRefresh { get; set; }
    /// <summary>현재 전체 Runtime을 선택 호출 끝까지 유지하는 owner 조회다.</summary>
    public Func<object?>? Pin { get; set; }
    /// <summary>기본 Client 장치는 함수 객체만 값 복사하고 현재 owner를 조회하지 않는다.</summary>
    internal RawSurfaceDisplayBindings Copy() => new() { Changed = Changed, View = View, Suppressed = Suppressed, Invalidate = Invalidate, MetaRefresh = MetaRefresh, Pin = Pin };
}

/// <summary>World와 독립인 실제 표시 실행기·싱크를 같은 물리 수명에 한 번 소유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>장기 실제 Client 장치의 함수 사본이다.</summary>
    private RawSurfaceDisplayBindings _displayDevices = new();
    /// <summary>현재 World가 소유하는 짧은 표시 대여다. Storage는 World를 소유하지 않는다.</summary>
    private WeakReference<RawSurfaceDisplayBindings>? _displayRuntime;
    /// <summary>현재 뷰/TYPE/셰이프를 읽는 유일한 장기 표시 실행기다.</summary>
    private SquidDisplay? _displayExecutor;

    /// <summary>조회/그리기 없이 실제 기본 장치의 필수 함수만 확인해 값 복사한다.</summary>
    public void BindDisplayDevices(RawSurfaceDisplayBindings devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (devices.View is null || devices.Suppressed is null || devices.Invalidate is null) throw new ArgumentException("표면 표시의 현재 Client 장치 누락", nameof(devices));
        _displayDevices = devices.Copy();
    }
    /// <summary>살아 있는 다른 World를 덮지 않고 같은 현재 대여만 약하게 공개한다.</summary>
    public void BindDisplayRuntime(RawSurfaceDisplayBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime); RawSurfaceDisplayBindings? current = DisplayRuntime();
        if ((current is not null && !ReferenceEquals(current, runtime)) || runtime.View is null || runtime.Suppressed is null || runtime.Invalidate is null)
            throw new ArgumentException("표면 표시의 같은 World 대여 필요", nameof(runtime));
        _displayRuntime = new(runtime);
    }
    /// <summary>managed 일반 종료에서 자기 대여만 결정적으로 반납한다. 다른 현재 대여는 변경하지 않는다.</summary>
    internal void ReleaseDisplayRuntime(RawSurfaceDisplayBindings runtime)
    {
        if (!ReferenceEquals(DisplayRuntime(), runtime)) throw new ArgumentException("표시의 같은 현재 대여 반납 필요", nameof(runtime));
        _displayRuntime = null;
    }
    /// <summary>그때 살아 있는 같은 World 대여만 강하게 빌린다.</summary>
    private RawSurfaceDisplayBindings? DisplayRuntime() => _displayRuntime is not null && _displayRuntime.TryGetTarget(out RawSurfaceDisplayBindings? current) ? current : null;

    /// <summary>실제 현재 카메라 장치를 선택 호출 동안 pin하고 독립 기본 뷰를 합성하지 않는다.</summary>
    public SquidDisplayView CurrentDisplayView()
    {
        RawSurfaceDisplayBindings? current = DisplayRuntime(); object? owner = current?.Pin?.Invoke();
        try
        {
            Func<SquidDisplayView> read = (current is null ? _displayDevices.View : current.View) ?? throw new InvalidOperationException("표면 표시의 현재 카메라/지도 장치 미연결");
            return read() ?? throw new InvalidOperationException("표면 표시의 현재 뷰 값 누락");
        }
        finally { GC.KeepAlive(owner); }
    }
    /// <summary>같은 현재 전체 갱신 관문만 읽고 실행 중 대여/Runtime을 유지한다.</summary>
    public bool DisplaySuppressed()
    {
        RawSurfaceDisplayBindings? current = DisplayRuntime(); object? owner = current?.Pin?.Invoke();
        try { return ((current is null ? _displayDevices.Suppressed : current.Suppressed) ?? throw new InvalidOperationException("표면 표시의 실제 전체 갱신 관문 미연결"))(); }
        finally { GC.KeepAlive(owner); }
    }
    /// <summary>실제 dirty 장치에 정확한 영역/플래그를 전달한다.</summary>
    public void InvalidateDisplay(DisplayRect rectangle, uint flags)
    {
        RawSurfaceDisplayBindings? current = DisplayRuntime(); object? owner = current?.Pin?.Invoke();
        try { ((current is null ? _displayDevices.Invalidate : current.Invalidate) ?? throw new InvalidOperationException("표면 표시의 실제 dirty 장치 미연결"))(rectangle, flags); }
        finally { GC.KeepAlive(owner); }
    }
    /// <summary>현재 World가 실제 설치한 메타 장치만 우선하며 없으면 실제 Client 장치를 선택한다.</summary>
    public void RefreshMetaDisplay(ReadOnlySpan<byte> raw)
    {
        RawSurfaceDisplayBindings? current = DisplayRuntime(); bool selected = current?.MetaRefresh is not null; object? owner = selected ? current!.Pin?.Invoke() : null;
        try { ((selected ? current!.MetaRefresh : _displayDevices.MetaRefresh) ?? throw new InvalidOperationException("현재 메타 표시의 실제 파생 갱신 장치 미연결"))(raw); }
        finally { GC.KeepAlive(owner); }
    }
    /// <summary>원문에서 선택한 현재 World의 기존 changed 관찰만 알리고 게임 효과/성공을 대신하지 않는다.</summary>
    public void NoteChanged()
    {
        RawSurfaceDisplayBindings? current = DisplayRuntime(); if (current?.Changed is null) return;
        object? owner = current.Pin?.Invoke();
        try { (current.Changed ?? throw new InvalidOperationException("현재 World의 선택된 changed 관찰 미연결"))(); }
        finally { GC.KeepAlive(owner); }
    }
    /// <summary>현재 장치가 있을 때만 한 표시 실행기를 구성하고 새 World/대여를 만들지 않는다.</summary>
    public SquidDisplay DisplayExecutor()
    {
        if (_displayExecutor is null)
        {
            var display = new SquidDisplay(_edition, Types, Shapes, new StorageDisplaySink(this), CurrentDisplayView());
            display.BindView(CurrentDisplayView); _displayExecutor = display;
        }
        return _displayExecutor;
    }

    /// <summary>원문 private sink 상속을 Storage의 같은 현재 장치에 전달하는 관리 어댑터다.</summary>
    /// <param name="storage">싱크보다 오래 사는 같은 장기 물리/장치 owner다.</param>
    private sealed class StorageDisplaySink(RawSurfaceStorage storage) : SquidDisplaySink
    {
        /// <summary>현재 전체 갱신 관문이다.</summary>
        public override bool Suppressed() => storage.DisplaySuppressed();
        /// <summary>현재 dirty표에 원문 영역을 전달한다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => storage.InvalidateDisplay(rect, flags);
        /// <summary>Storage의 같은 현재 checking을 읽는다.</summary>
        public override RawCheckingFlag Checking() => storage.Checking();
        /// <summary>장기 실제 상세 owner에 오류를 기록한다.</summary>
        public override void FrameError(uint type, int frame, uint abstractValue) => storage.RecordDisplayFrameError(type, frame, abstractValue);
        /// <summary>선택한 실제 원문 진단의 복귀를 전달한다.</summary>
        public override void Assertion(string expression, string file, uint line) => storage.AssertDisplay(expression, file, line);
        /// <summary>현재 실제 메타/일반 Sprite 가상88을 선택한다.</summary>
        public override void MetaRefresh(ReadOnlySpan<byte> raw) => storage.RefreshMetaDisplay(raw);
    }
}
