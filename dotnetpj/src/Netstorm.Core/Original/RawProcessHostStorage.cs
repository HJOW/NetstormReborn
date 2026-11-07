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
