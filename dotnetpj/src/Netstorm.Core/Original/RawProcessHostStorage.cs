// 같은 물리의 프로세스 상태·호스트·커널과 짧은 현재 월드 처리기 대여를 분리한다.
namespace Netstorm.Core.Original;

// 프로세스 호스트 저장소 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.

// 실행 모듈을 복제하지 않고 실제 공통 Host의 수명만 소유하는 저장소 껍데기다. Host 본체·현재 대여·legacy 회수·소멸 반납은 Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.
public sealed class RawProcessHostStorage
{
    /// <summary>실제 물리 배열은 외부 저장소가 소유한다. 같은 풀이다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은 전역 TYPE을 빌린다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 두 권한 bool과 같은 긴 수명이다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly RawAuthorityState? _authority;

    /// <summary>now fallback·타입 순환·외부 참조를 한 번 소유하는 실제 스칼라 저장소다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly SquidProcessState _state = new();

    /// <summary>스칼라 포인터의 실제 소유자를 pin한다. 같은 장기 시계다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private FrameTimeBox? _clock;

    /// <summary>스칼라 포인터의 실제 소유자를 pin한다. 같은 장기 간헐천 타입이다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private UIntBox? _pollType;

    /// <summary>일반·공유·상한 참조가 Host의 최종 반납까지 유효하도록 소유자를 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly RawGameAuxState? _gameAux;

    /// <summary>Host보다 먼저 반납하며 독립 커널도 잔류보다 먼저 사라지지 않는다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private Kernel? _kernel;

    /// <summary>같은 명시 외부 장치와 독립 최종 종료 정책만 구별한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private bool _external;

    /// <summary>현재 World 처리기만 빌리며 실제 반납 증빙의 발급 위치를 소유한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private SquidProcessHost? _host;

    /// <summary>현재 짧은 대여만 조회하며 월드를 숨겨서 붙잡지 않는다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private ProcessHostRuntimeBindings? _current;

    /// <summary>외부 raw 커널의 월드를 장기 소유하지 않고 현재 가상 회수 경계만 조회한다. 호출자가 반환 대리자를 쥐고 있어야 산다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private WeakReference<Action<int>>? _legacyReclaim;

    /// <summary>Pool·TYPE는 이 owner보다 오래 살아야 하며 실제 TYPE owner도 pin한다. 장기 권한·타입의 원래 주소를 빌리며 CD의 독립 계약은 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 실제 풀이다.</param>
    /// <param name="types">같은 전역 TYPE이다.</param>
    /// <param name="authority">현재 두 권한 bool과 같은 긴 수명이다. null이면 독립 값을 쓴다.</param>
    /// <param name="pollType">같은 장기 간헐천 타입 owner다. null이면 독립 값을 쓴다.</param>
    /// <param name="gameAux">같은 장기 절차 번호 owner다. null이면 독립 값을 쓴다.</param>
    public RawProcessHostStorage(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawAuthorityState? authority, UIntBox? pollType = null, RawGameAuxState? gameAux = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        _pool = pool;
        _types = types;
        _authority = authority;
        _pollType = pollType;
        _gameAux = gameAux;
        _state.SharedAuthority = authority;
        _state.SharedPollType = pollType;
        if (gameAux is not null)
        {
            _state.CurrentProcessFormType = gameAux.ProcessFormTypeBox;
            if (pool.Edition == OriginalEdition.Patch1078)
            {
                _state.CurrentRegularType = gameAux.RegularProcessTypeBox;
                _state.CurrentSharedRegularType = gameAux.SharedRegularProcessTypeBox;
                _state.CurrentProcessTypeEnd = gameAux.ProcessTypeEndBox;
            }

            if (pool.Edition == OriginalEdition.Patch1078)
            {
                _state.CurrentFireAnimationType = gameAux.FireAnimationProcessTypeBox;
                _state.CurrentRotateAnimationType = gameAux.RotateAnimationProcessTypeBox;
            }
        }
    }

    /// <summary>같은 실제 Client 장치를 pin하며 Session 지역 주소는 받지 않는다. 명시 미지정 pollType은 기존 같은 장기 TYPE owner를 보존한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="kernel">같은 실제 Client 커널이다.</param>
    /// <param name="clock">같은 실제 Client 시계다.</param>
    /// <param name="pollType">같은 장기 간헐천 타입 owner다. null이면 기존 owner를 둔다.</param>
    public void BindDevices(Kernel kernel, FrameTimeBox clock, UIntBox? pollType = null)
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(clock);
        pollType ??= _pollType;
        if (_external && ReferenceEquals(_kernel, kernel) && ReferenceEquals(_clock, clock) && ReferenceEquals(_pollType, pollType))
        {
            return;
        }

        if (_kernel is not null && (!ReferenceEquals(_kernel, kernel) || !ReferenceEquals(_clock, clock) || !ReferenceEquals(_pollType, pollType)))
        {
            throw new ArgumentException("프로세스 호스트의 같은 장기 커널·시각 장치 계약 위반");
        }

        _kernel = kernel;
        _clock = clock;
        _pollType = pollType;
        _external = true;
        _state.SharedNow = new FrameGameBox(clock);
        _state.SharedPollType = pollType;
    }

    /// <summary>raw legacy 입력이 이미 pin한 같은 실제 장치인지 조회한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="kernel">조회할 커널이다.</param>
    /// <returns>같은 장치면 참이다.</returns>
    public bool UsesKernel(Kernel? kernel) => _kernel is not null && ReferenceEquals(_kernel, kernel);

    /// <summary>명시 장기 외부 장치가 연결됐는지 읽는다. 대여를 만들거나 상태를 복사하지 않는다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>외부 장치가 있으면 참이다.</returns>
    public bool HasDevices() => _external;

    /// <summary>외부 장치가 없으면 같은 저장소의 독립 커널을 한 번만 만든다. 독립 판본의 실제 capacity를 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>외부 또는 독립 커널이다.</returns>
    public Kernel Kernel()
    {
        _kernel ??= new Kernel(_pool.Edition == OriginalEdition.Patch1078 ? KernelShape.ProcessCapacity : KernelShape.CdProcessCapacity);
        return _kernel;
    }

    /// <summary>전체 월드·Regular가 같은 실제 스칼라 저장소를 빌린다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>같은 스칼라 저장소다.</returns>
    public SquidProcessState State() => _state;

    /// <summary>첫 사용에 안정된 Host를 만들고 같은 주소를 유지한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    /// <returns>같은 장기 호스트다.</returns>
    public SquidProcessHost Host()
    {
        _host ??= new SquidProcessHost(_pool, _types, Kernel(), State(), () => _current);
        return _host;
    }

    /// <summary>현재 가상 삭제 대여를 호출 끝까지 pin하고 SID 소진의 flags 0 삭제를 실행한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    /// <param name="sid">회수할 번호다.</param>
    public void ReclaimForAllocation(int sid)
    {
        if (_legacyReclaim is not null && _legacyReclaim.TryGetTarget(out Action<int>? legacy))
        {
            legacy(sid);
            return;
        }

        ProcessHostRuntimeBindings? runtime = _current;
        Action<SquidDestroyEvent>? emit = runtime?.Asset?.Emit;
        if (runtime is null || emit is null)
        {
            throw new InvalidOperationException("SID 소진 복구의 현재 월드 가상 삭제 미연결");
        }

        using (new RuntimeCallGuard(runtime))
        {
            emit(new SquidDestroyEvent(SquidDestroyEffect.DestroyDependent, sid, 0, 0u));
        }
    }

    /// <summary>raw 외부 커널의 현재 월드만 회수 callable을 소유하며 마지막 대여 반납 뒤에는 조회를 거부한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    /// <param name="reclaim">현재 월드 회수 콜백이다.</param>
    /// <returns>호출자가 쥐는 회수 callable이다.</returns>
    public Action<int> BindLegacyAllocationReclaim(Action<int> reclaim)
    {
        ArgumentNullException.ThrowIfNull(reclaim);
        if (_current is not null || (_legacyReclaim is not null && _legacyReclaim.TryGetTarget(out _)))
        {
            throw new ArgumentException("SID legacy 회수의 단일 현재 월드 대여 계약 위반", nameof(reclaim));
        }

        _legacyReclaim = new WeakReference<Action<int>>(reclaim);
        return reclaim;
    }

    /// <summary>중복·다른 풀은 현재 바인딩을 바꾸기 전에 거부한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    /// <param name="runtime">빌릴 현재 런타임 묶음이다.</param>
    /// <returns>현재 대여다.</returns>
    public Use Bind(ProcessHostRuntimeBindings? runtime)
    {
        if (runtime is null || runtime.Destroy is null || !ReferenceEquals(runtime.Destroy.Pool, _pool) || _current is not null ||
            (_legacyReclaim is not null && _legacyReclaim.TryGetTarget(out _)))
        {
            throw new ArgumentException("프로세스 호스트의 실제 현재 월드 단일 대여 계약 위반", nameof(runtime));
        }

        if (runtime.Factory is not null && !ReferenceEquals(runtime.Factory.Pool, _pool))
        {
            throw new ArgumentException("프로세스 호스트 현재 공장의 다른 Pool 대여 위반", nameof(runtime));
        }

        _ = Host();
        return new Use(this, runtime);
    }

    /// <summary>현재 월드 처리기만 빌리며 실제 반납 증빙의 발급 위치를 소유한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    public sealed class Use : IDisposable
    {
        /// <summary>반납 시 null로 소비하는 실제 대여 소유권이다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        private RawProcessHostStorage? _owner;

        /// <summary>이전 월드 캡처는 이 짧은 대여에만 존재한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        private ProcessHostRuntimeBindings? _runtime;

        /// <summary>검증한 현재 월드만 등록한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        /// <param name="owner">빌린 저장소다.</param>
        /// <param name="runtime">빌릴 현재 런타임 묶음이다.</param>
        internal Use(RawProcessHostStorage owner, ProcessHostRuntimeBindings runtime)
        {
            _owner = owner;
            _runtime = runtime;
            owner._current = runtime;
        }

        /// <summary>현재 자기 처리기만 반납하며 커널을 실행하거나 비우지 않는다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        public void Dispose()
        {
            if (_owner is not null && ReferenceEquals(_owner._current, _runtime))
            {
                _owner._current = null;
            }

            _owner = null;
            _runtime = null;
        }

        /// <summary>현재 월드의 생성 실패·일반 종료에서 자기 대여가 만든 객체만 반납한다. 이전 Native 잔류는 건드리지 않는다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        public void DrainOwnedForShutdown()
        {
            if (_owner is not null && ReferenceEquals(_owner._current, _runtime))
            {
                _owner.Host().DrainRuntimeBindings(_runtime!);
            }
        }

        /// <summary>실제 같은 커널 Clear 한 번이 정상 복귀했을 때만 증빙을 만든다. 예외이면 증빙을 발급하지 않으며 이미 수행된 부분 효과는 되돌리지 않는다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        /// <returns>정상 복귀 증빙이다.</returns>
        public ClearPass ClearForState()
        {
            if (_owner is null || !ReferenceEquals(_owner._current, _runtime))
            {
                throw new InvalidOperationException("현재 프로세스 호스트 대여 없는 Native Clear");
            }

            using (new RuntimeCallGuard(_runtime))
            {
                _owner.Kernel().Clear();
            }

            return new ClearPass(_owner, _runtime!);
        }

        /// <summary>같은 대여의 증빙과 현재 잔류 종류를 확인하고 실제 바인딩을 반납한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        /// <param name="pass">소비할 정상 복귀 증빙이다.</param>
        /// <returns>현재 처리기 반납 뒤 커널을 pin하는 결과다.</returns>
        public Returned ReturnForState(ClearPass pass)
        {
            ArgumentNullException.ThrowIfNull(pass);
            if (_owner is null || !ReferenceEquals(pass.Owner, _owner) || !ReferenceEquals(pass.Runtime, _runtime) || !ReferenceEquals(_owner._current, _runtime))
            {
                throw new ArgumentException("Native Clear의 같은 프로세스 호스트 대여 증빙 필요", nameof(pass));
            }

            if (pass.Consumed)
            {
                throw new ArgumentException("이미 소비한 정상 복귀 증빙", nameof(pass));
            }

            if (_runtime!.ActiveCalls != 0ul)
            {
                throw new InvalidOperationException("진행 중 프로세스 효과의 현재 대여 반납 금지");
            }

            if (!_owner.Host().HasOnlyRegularProcesses())
            {
                throw new InvalidOperationException("Regular 이외 잔류의 현재 모듈 수명 이행 미완료");
            }

            Kernel kernel = _owner.Kernel();
            pass.Consume();
            _owner._current = null;
            _owner = null;
            _runtime = null;
            return new Returned(kernel);
        }
    }

    /// <summary>같은 실제 커널 Clear 정상 복귀의 이동 전용 증빙이다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    public sealed class ClearPass
    {
        /// <summary>발급한 실제 저장소다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        internal RawProcessHostStorage Owner { get; }

        /// <summary>발급 위치의 런타임 묶음이다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        internal ProcessHostRuntimeBindings Runtime { get; }

        /// <summary>소비됐는지다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        internal bool Consumed { get; private set; }

        /// <summary>대여의 실제 Clear 정상 복귀에서만 발급한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        /// <param name="owner">발급 저장소다.</param>
        /// <param name="runtime">발급 위치 묶음이다.</param>
        internal ClearPass(RawProcessHostStorage owner, ProcessHostRuntimeBindings runtime)
        {
            Owner = owner;
            Runtime = runtime;
        }

        /// <summary>증빙을 소비한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        internal void Consume() => Consumed = true;
    }

    /// <summary>현재 처리기 반납 뒤 커널을 pin하는 이동 전용 결과다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    public sealed class Returned
    {
        /// <summary>잔류가 이전 월드보다 오래 살아도 먼저 사라지지 않는 같은 커널이다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        public Kernel Kernel { get; }

        /// <summary>실제 대여가 끝난 뒤 같은 커널 소유권을 유지한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
        /// <param name="kernel">유지할 커널이다.</param>
        internal Returned(Kernel kernel)
        {
            Kernel = kernel;
        }
    }

    /// <summary>같은 시계 상자의 게임 시각을 직접 읽고 쓰는 빌림 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private sealed class FrameGameBox : DoubleBox
    {
        /// <summary>같은 장기 시계 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
        private readonly FrameTimeBox _clock;

        /// <summary>같은 시계 상자를 빌린다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
        /// <param name="clock">빌릴 시계 상자다.</param>
        internal FrameGameBox(FrameTimeBox clock)
        {
            _clock = clock;
        }

        /// <summary>같은 시계 상자의 게임 시각을 직접 읽고 쓴다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
        public override double Value
        {
            get => _clock.Value.Game;
            set
            {
                FrameTime frame = _clock.Value;
                frame.Game = value;
                _clock.Value = frame;
            }
        }
    }
}
