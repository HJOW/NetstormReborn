// 장기 공장이 실제 현재 World의 짧은 공간/Form 장치만 약하게 빌리게 한다.
namespace Netstorm.Core.Original;

/// <summary>실제 같은 풀의 현재 World 장치다. 호출자가 수명을 소유하고 공장은 약한 참조만 보유한다.</summary>
public sealed class SquidFactoryRuntimeBindings
{
    /// <summary>현재 실제 자산 공간 해제 몸체다.</summary>
    public IRawUnpop? Unpop { get; set; }
    /// <summary>현재 Session 또는 같은 World의 실제 공통 종속 분리다.</summary>
    public Action<int>? FormDetach { get; set; }
    /// <summary>현재 내용물 TYPE DWORD를 선택 위치에서 읽는다.</summary>
    public Func<uint>? ContentType { get; set; }
    /// <summary>실제 Form 전송 장치다. 빈 성공 대체를 넣지 않는다.</summary>
    public Action<int, uint>? FormTransmit { get; set; }
    /// <summary>실제 외곽 Runtime owner를 선택 호출 동안 보유한다.</summary>
    public Func<object?>? Pin { get; set; }
    /// <summary>managed 명시 반납이 진행 중 원문 shared_ptr 호출을 앞서지 않게 하는 호출 수다.</summary>
    internal uint ActiveCalls { get; set; }
    /// <summary>진행 중 명시 반납은 마지막 실제 호출이 돌아온 뒤만 적용한다.</summary>
    internal bool ReleaseRequested { get; set; }
}

/// <summary>현재 World 장치와 독립 공장 장치를 실제 소비 위치에서 고른다.</summary>
public sealed partial class RawSquidFactory
{
    /// <summary>같은 저장소의 현재 실제 Unpop을 소비 위치에서 얻는 장기 getter다.</summary>
    private Func<IRawUnpop>? _currentUnpop;
    /// <summary>이전 World를 장기 공장이 소유하지 않게 하는 현재 대여다.</summary>
    private WeakReference<SquidFactoryRuntimeBindings>? _runtimeBinding;

    /// <summary>실제 장기 Unpop getter만 연결하며 아직 공간/UI를 조회하지 않는다.</summary>
    public void BindUnpop(Func<IRawUnpop> unpop) => _currentUnpop = unpop ?? throw new ArgumentNullException(nameof(unpop));

    /// <summary>현재 같은 Pool의 필수 장치를 약하게 연결한다. 다른 살아 있는 World와 섞지 않는다.</summary>
    public void BindRuntime(SquidFactoryRuntimeBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        SquidFactoryRuntimeBindings? current = CurrentRuntime();
        if ((current is not null && !ReferenceEquals(current, runtime)) || runtime.Unpop is null || !ReferenceEquals(runtime.Unpop.Pool, _pool) ||
            runtime.FormDetach is null || runtime.ContentType is null || runtime.FormTransmit is null || runtime.ReleaseRequested)
            throw new ArgumentException("Factory의 같은 실제 World 대여/가상 경계 필요", nameof(runtime));
        _runtimeBinding = new(runtime);
    }

    /// <summary>같은 managed World가 명시 종료될 때 반납한다. 실제 호출 중에는 shared_ptr처럼 마지막 복귀까지 대여를 유지한다.</summary>
    internal void ReleaseRuntime(SquidFactoryRuntimeBindings runtime)
    {
        if (!ReferenceEquals(CurrentRuntime(), runtime)) throw new ArgumentException("Factory의 같은 현재 대여 반납 필요", nameof(runtime));
        runtime.ReleaseRequested = true;
        if (runtime.ActiveCalls == 0) _runtimeBinding = null;
    }

    /// <summary>소유자가 만료되었으면 독립 장치로 돌아가며 가짜 World를 만들지 않는다.</summary>
    private SquidFactoryRuntimeBindings? CurrentRuntime() => _runtimeBinding is not null && _runtimeBinding.TryGetTarget(out SquidFactoryRuntimeBindings? runtime) ? runtime : null;

    /// <summary>현재 장치/실제 외곽 owner를 정상·예외 복귀까지 보유한다.</summary>
    private FactoryRuntimeCall BeginRuntimeCall() => new(this, CurrentRuntime());

    /// <summary>C++ 현재 shared_ptr와 외곽 owner의 호출 범위를 managed 수명으로 옮긴다.</summary>
    private sealed class FactoryRuntimeCall : IDisposable
    {
        /// <summary>마지막 명시 반납의 적용 대상 공장이다.</summary>
        private readonly RawSquidFactory _factory;
        /// <summary>실제 외곽 owner를 호출 끝까지 쥐는 강한 참조다.</summary>
        private object? _owner;
        /// <summary>현재 실제 장치 대여다. 호출 끝에 null로 소비한다.</summary>
        public SquidFactoryRuntimeBindings? Runtime { get; private set; }
        /// <summary>pin을 먼저 실행하고 같은 대여의 실제 호출 수를 늘린다.</summary>
        public FactoryRuntimeCall(RawSquidFactory factory, SquidFactoryRuntimeBindings? runtime)
        {
            _factory = factory; Runtime = runtime; _owner = runtime?.Pin?.Invoke();
            if (runtime is not null) runtime.ActiveCalls = unchecked(runtime.ActiveCalls + 1);
        }
        /// <summary>마지막 복귀 뒤 일반 소유자는 보유만 끝내고 표시된 호출 대여를 한 번 반납한다.</summary>
        public void Dispose()
        {
            if (Runtime is { } runtime)
            {
                runtime.ActiveCalls = unchecked(runtime.ActiveCalls - 1);
                if (runtime.ActiveCalls == 0 && runtime.ReleaseRequested && ReferenceEquals(_factory.CurrentRuntime(), runtime)) _factory._runtimeBinding = null;
                Runtime = null;
            }
            object? owner = _owner; _owner = null; RawRuntimePin.Release(owner);
        }
    }
}
