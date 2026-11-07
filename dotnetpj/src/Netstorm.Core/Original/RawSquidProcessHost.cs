// 객체 Form과 커널 프로세스의 부착·삭제·실행 기반(생성·조회·번호·해제·삭제·정기)을 공유한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

// 프로세스 호스트 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.

// 객체 Form과 커널 프로세스의 부착·통지·수명·삭제·정기 실행을 연결하는 호스트 본체다. 검색·전송 본체와 패킷 직렬화는 후속 단위에서 잇는다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐고 0.204에서 삭제 본체를, 0.205에서 정기 실행을, 0.214에서 부착 본체를 이었다.
public sealed class SquidProcessHost
{
    /// <summary>CD판의 절차 상한 비트다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    public const uint CdProcessEnd = 62u;

    /// <summary>패치판 form의 실제 가상 표 기록값이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    public const uint PatchFormVtable = 0x512098u;

    /// <summary>CD판 form의 실제 가상 표 기록값이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    public const uint CdFormVtable = 0x505990u;

    /// <summary>풀·커널·공통 삭제·상태는 호출자가 소유하며 host보다 오래 살아야 한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>직접 입력의 TYPE 사본이다. 장기 실제 TYPE와 구별한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly List<OriginalTypeRecord> _types = [];

    /// <summary>장기 실제 TYPE다. 비어 있으면 직접 사본을 쓴다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord>? _sharedTypes;

    /// <summary>같은 커널이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly Kernel _kernel;

    /// <summary>직접 입력의 공통 삭제 몸체다. 장기 대여가 있으면 현재 월드의 몸체를 쓴다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly RawSquidDestroy? _destroy;

    /// <summary>같은 프로세스 상태다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly SquidProcessState _state;

    /// <summary>직접 입력의 가상 Regular 사건이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private RegularHandler? _handler;

    /// <summary>직접 입력의 자산 효과다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private SquidDestroyHooks? _asset;

    /// <summary>직접 입력의 직렬화·송신 경계다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private ProcessFormTransmission? _transmit;

    /// <summary>장기 Pool·커널·상태를 빌리고 현재 월드의 가상 처리기만 선택 위치에서 조회한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private readonly Func<ProcessHostRuntimeBindings?>? _currentRuntime;

    /// <summary>같은 수명의 직접 Host에만 연결하는 공장이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private RawSquidFactory? _directFormFactory;

    /// <summary>풀·커널·공통 삭제·상태는 호출자가 소유하며 host보다 오래 살아야 한다. asset은 form이 아닌 객체의 삭제 훅이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="types">같은 전역 TYPE이다. 판본 개수(패치 188·CD 171)와 같아야 한다.</param>
    /// <param name="kernel">같은 커널이다.</param>
    /// <param name="destroy">공통 삭제 몸체다. 같은 풀을 써야 한다.</param>
    /// <param name="state">같은 프로세스 상태다.</param>
    /// <param name="handler">가상 Regular 사건이다.</param>
    /// <param name="asset">자산 효과다.</param>
    /// <param name="transmit">직렬화·송신 경계다.</param>
    public SquidProcessHost(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, Kernel kernel, RawSquidDestroy destroy, SquidProcessState state, RegularHandler? handler = null, SquidDestroyHooks? asset = null, ProcessFormTransmission? transmit = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(state);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("프로세스 타입 판본·크기 오류", nameof(types));
        }

        if (!ReferenceEquals(destroy.Pool, pool))
        {
            throw new ArgumentException("프로세스·삭제 SID 풀이 다릅니다.", nameof(destroy));
        }

        _pool = pool;
        _types.AddRange(types);
        _kernel = kernel;
        _destroy = destroy;
        _state = state;
        _handler = handler;
        _asset = asset;
        _transmit = transmit;
    }

    /// <summary>같은 Native Host 주소를 다음 월드에서도 유지하고 실제 현재 처리기만 대여한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="types">같은 전역 TYPE이다. 판본 개수(패치 188·CD 171)와 같아야 한다.</param>
    /// <param name="kernel">같은 커널이다.</param>
    /// <param name="state">같은 프로세스 상태다.</param>
    /// <param name="current">현재 월드 대여를 호출 위치에서 얻는 getter다.</param>
    public SquidProcessHost(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, Kernel kernel, SquidProcessState state, Func<ProcessHostRuntimeBindings?> current)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(current);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("장기 프로세스 호스트의 실제 판본·크기 오류", nameof(types));
        }

        _pool = pool;
        _sharedTypes = types;
        _kernel = kernel;
        _state = state;
        _currentRuntime = current;
    }

    /// <summary>생성하지 않은 빈 월드를 합성하지 않고 선택된 실제 처리기 부재를 그 위치에서 보고한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>현재 런타임 묶음이다.</returns>
    public ProcessHostRuntimeBindings Runtime()
    {
        ProcessHostRuntimeBindings? value = _currentRuntime?.Invoke();
        if (value is null || value.Destroy is null || !ReferenceEquals(value.Destroy.Pool, _pool))
        {
            throw new InvalidOperationException("장기 프로세스 호스트의 현재 월드 처리기 미연결");
        }

        return value;
    }

    /// <summary>직접 입력과 현재 월드 대여는 각각 자기 실제 삭제 몸체를 사용한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>현재 공통 삭제 몸체다.</returns>
    public RawSquidDestroy Destroy()
    {
        if (_currentRuntime is not null)
        {
            return Runtime().Destroy!;
        }

        if (_destroy is null)
        {
            throw new InvalidOperationException("프로세스 호스트의 공통 삭제 몸체 미연결");
        }

        return _destroy;
    }

    /// <summary>직접 입력의 사본과 장기 실제 TYPE를 구분한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>TYPE 개수다.</returns>
    public int TypeCount() => _sharedTypes is not null ? _sharedTypes.Count : _types.Count;

    /// <summary>옵션·다음 월드가 바꾼 실제 플래그도 즉시 읽는다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    public OriginalTypeRecord Type(int type)
    {
        if (type < 0 || type >= TypeCount())
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "프로세스 호스트 현재 TYPE 범위 오류");
        }

        return _sharedTypes is not null ? _sharedTypes[type] : _types[type];
    }

    /// <summary>현재 사건 함수만 짧게 빌린다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>현재 Regular 처리기다.</returns>
    public RegularHandler? Regular() => _currentRuntime is not null ? Runtime().Regular : _handler;

    /// <summary>현재 자산 효과를 복사해 호출 중 수명을 지킨다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>현재 자산 효과다.</returns>
    public SquidDestroyHooks? Asset() => _currentRuntime is not null ? Runtime().Asset : _asset;

    /// <summary>원문 재조회 위치를 유지한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>현재 전송 경계다.</returns>
    public ProcessFormTransmission? Transmission() => _currentRuntime is not null ? Runtime().Transmit : _transmit;

    /// <summary>같은 수명의 직접 Host에만 공장을 연결한다. 장기 Host는 현재 대여의 factory를 소비 위치에서 조회한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="factory">연결할 공장이다. 같은 풀을 써야 한다.</param>
    public void BindFormFactory(RawSquidFactory factory)
    {
        ArgumentNullException.ThrowIfNull(factory);
        if (_currentRuntime is not null || !ReferenceEquals(factory.Pool, _pool) || (_directFormFactory is not null && !ReferenceEquals(_directFormFactory, factory)))
        {
            throw new ArgumentException("ProcessForm 직접 공장 연결의 같은 수명·Pool 계약 위반", nameof(factory));
        }

        _directFormFactory = factory;
    }

    /// <summary>모듈을 연결할 때 다른 월드의 풀을 섞지 않도록 참조를 제공한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>같은 풀이다.</returns>
    public RawSidPool Pool() => _pool;

    /// <summary>월드 파생 사건을 등록할 때 기존 fallback을 합성한 실제 처리기를 교체한다. 다음 Regular 호출은 현재 처리기를 읽는다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="handler">교체할 처리기다.</param>
    public void SetRegularHandler(RegularHandler? handler)
    {
        if (_currentRuntime is not null)
        {
            Runtime().Regular = handler;
        }
        else
        {
            _handler = handler;
        }
    }

    /// <summary>미션 runtime의 실제 직렬화기를 연결한다. 기존 프로세스도 다음 Pop에서 이 경계를 읽는다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="transmit">연결할 전송 경계다.</param>
    public void SetFormTransmit(ProcessFormTransmission? transmit)
    {
        if (_currentRuntime is not null)
        {
            Runtime().Transmit = transmit;
        }
        else
        {
            _transmit = transmit;
        }
    }

    /// <summary>판본의 프로세스 타입 번호 범위 끝(패치 63, CD 62)이다. PATCH는 같은 현재 상한 비트를 반환한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>절차 상한 비트다.</returns>
    public uint ProcessTypeEnd() => _pool.Edition == OriginalEdition.Patch1078 ? _state.ProcessTypeEnd() : CdProcessEnd;

    /// <summary>form의 실제 가상 표 기록값이다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <returns>가상 표 기록값이다.</returns>
    public uint FormVtable() => _pool.Edition == OriginalEdition.Patch1078 ? PatchFormVtable : CdFormVtable;

    /// <summary>그 번호의 커널 슬롯에 지금도 이 프로세스가 등록돼 있는지 본다. 주소만 비교하고 객체를 읽지 않으므로 Kill이 프로세스를 없앴는지 확인할 때 쓸 수 있다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="pid">확인할 슬롯 번호다.</param>
    /// <param name="process">비교할 프로세스다.</param>
    /// <returns>같은 주소면 참이다.</returns>
    public bool IsRegistered(uint pid, SquidProcess? process) => process is not null && ReferenceEquals(_kernel.Get(pid), process);

    /// <summary>잔류 객체가 실제로 이 Host를 빌리는지 조회하며 PID나 SID를 정규화하지 않는다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    /// <param name="process">확인할 프로세스다.</param>
    /// <returns>같은 Host면 참이다.</returns>
    public bool Owns(BaseProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        return process is SquidProcess squid && ReferenceEquals(squid.HostLink, this);
    }

    /// <summary>이것은 월드 최종 반납이며 Native 커널 Clear의 단일 순회를 대신하지 않는다. 독립 월드의 최종 fallback만 자기 현재 대여를 정리하며 이전 Native 잔류는 보존한다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    /// <param name="runtime">정리할 현재 대여다.</param>
    public void DrainRuntimeBindings(ProcessHostRuntimeBindings? runtime)
    {
        _kernel.DrainOwnedForShutdown(candidate =>
            candidate is SquidProcess process && ReferenceEquals(process.HostLink, this) && ReferenceEquals(process.RuntimeLink, runtime));
    }

    /// <summary>현재 Host의 잔류 중 C++ Regular 이외 수명은 별도 복원을 요구한다. 표시 타입와 C++ 실행 클래스는 구별한다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    /// <returns>Regular만 남았으면 참이다.</returns>
    public bool HasOnlyRegularProcesses()
    {
        // 단일 현재 표를 읽으며 다른 Host·Client 프로세스를 자기 소유로 판정하지 않는다
        for (uint id = 1u; id <= _kernel.Capacity(); ++id)
        {
            BaseProcess? process = _kernel.Get(id);
            if (process is not null && Owns(process) && process is not RegularProcess)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>실제 가상 34는 server가 아니면 PID조차 읽지 않는다. 정상 현재 PID의 가상 통지만 호출한다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="watcher">보는 쪽 SID다.</param>
    /// <param name="watched">보이는 쪽 SID다.</param>
    /// <param name="dependForm">종속 Form SID다.</param>
    public void NotifyDependency(int watcher, int watched, int dependForm)
    {
        if (!_state.Server)
        {
            return;
        }

        SquidProcess? process = FromForm(watcher);
        if (process is null)
        {
            throw new InvalidOperationException("현재 Depend 통지의 실제 ProcessForm·PID 미연결");
        }

        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            process.Notify(2u, unchecked((uint)dependForm), unchecked((uint)watched));
        }
    }

    /// <summary>부모의 종속 체인에서 이벤트 번호가 같은 첫 Regular 프로세스를 찾는다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="ev">찾을 이벤트 번호다.</param>
    /// <returns>찾은 프로세스다. 없으면 null이다.</returns>
    public RegularProcess? FindEvent(int parent, uint ev) =>
        Find(parent, process => process.Event() == ev);

    /// <summary>(이벤트 &amp; 마스크) == 값인 첫 Regular 프로세스를 찾는다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="mask">마스크다.</param>
    /// <param name="value">기대값이다.</param>
    /// <returns>찾은 프로세스다. 없으면 null이다.</returns>
    public RegularProcess? FindEventMasked(int parent, uint mask, uint value) =>
        Find(parent, process => (process.Event() & mask) == value);

    /// <summary>가상 삭제 효과를 처리할 현재 Process 가상 표인지 판단한다. finder의 원문 TYPE·상한 관문과 구별한다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="sid">확인할 번호다.</param>
    /// <returns>프로세스 Form이면 참이다.</returns>
    public bool IsProcessForm(int sid)
    {
        if (sid == 0 || sid >= _pool.Capacity)
        {
            return false;
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint table = (uint)raw[0] | ((uint)raw[1] << 8) | ((uint)raw[2] << 16) | ((uint)raw[3] << 24);
        return table == FormVtable();
    }

    /// <summary>원본 004ae240의 프로세스에 통지하고 커널에서 제거한 뒤 삭제 깊이를 줄인다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    /// <param name="form">지울 Form 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public void FormPreDestroy(int form, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(form);
        uint pid = (uint)raw[FormPidOffset] | ((uint)raw[FormPidOffset + 1] << 8) | ((uint)raw[FormPidOffset + 2] << 16) | ((uint)raw[FormPidOffset + 3] << 24);
        var process = _kernel.Get(pid) as SquidProcess;
        if (process is null)
        {
            throw new InvalidOperationException("form의 프로세스가 커널에 없습니다.");
        }

        process.OnFormDestroy(flags);
        // 원본은 통지가 끝난 뒤 Form의 현재 PID를 다시 읽는다. 콜백 중 다른 번호로 바뀐 경우 그 슬롯을 제거한다
        ReadOnlySpan<byte> current = _pool.Slot(form);
        uint reread = (uint)current[FormPidOffset] | ((uint)current[FormPidOffset + 1] << 8) | ((uint)current[FormPidOffset + 2] << 16) | ((uint)current[FormPidOffset + 3] << 24);
        _kernel.Remove(reread);
        Destroy().CompletePreDestroy();
    }

    /// <summary>원본 004ae2b0의 부모가 지워질 때 프로세스에 통지한다. 원문 +1c의 NULL 조회만 빈 통지로 생략한다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    /// <param name="form">통지할 Form 번호다.</param>
    /// <param name="parent">지워지는 부모 번호다.</param>
    public void FormParentDying(int form, int parent)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(form);
        uint pid = (uint)raw[FormPidOffset] | ((uint)raw[FormPidOffset + 1] << 8) | ((uint)raw[FormPidOffset + 2] << 16) | ((uint)raw[FormPidOffset + 3] << 24);
        BaseProcess? current = _kernel.Get(pid);
        if (current is null)
        {
            return;
        }

        var process = current as SquidProcess;
        if (process is null)
        {
            throw new InvalidOperationException("현재 프로세스 부모 통지의 실제 가상+10 미연결");
        }

        process.OnParentDying(parent);
    }

    /// <summary>원본 004ae170의 Form 분리를 현재 월드·직접 공장·공통 순서로 실행한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    /// <param name="form">해제할 Form 번호다.</param>
    public void UnpopForm(int form)
    {
        ProcessHostRuntimeBindings? runtime = _currentRuntime is not null ? Runtime() : null;
        using (var call = new RuntimeCallGuard(runtime))
        {
            if (runtime is not null)
            {
                if (runtime.Factory is null)
                {
                    throw new InvalidOperationException("Process Unpop의 현재 공장 미연결");
                }

                runtime.Factory.UnpopForm(form);
                return;
            }

            if (_directFormFactory is not null)
            {
                _directFormFactory.UnpopForm(form);
                return;
            }

            // 독립 Host도 같은 공통 해제 순서를 사용한다
            new RawSquidForms(_pool).Unpop(form);
        }
    }

    /// <summary>원본 004ae140의 권한·번호 관문을 지나 공통 삭제로 프로세스를 지운다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    /// <param name="process">지울 프로세스다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public void Kill(SquidProcess process, uint flags)
    {
        ArgumentNullException.ThrowIfNull(process);
        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            int form = process.Form();
            // 권한이 없고 form이 서버 번호이며 client flag도 없으면 아무것도 하지 않는다
            if (!_state.Boss && !(form > 4 && form < RawSidPool.ServerFirst) && (flags & ClientForm) == 0u)
            {
                return;
            }

            if (form == 0 || form >= _pool.Capacity || (_pool.Slot(form)[StateOffset] & DeadBit) != 0)
            {
                throw new InvalidOperationException("프로세스 form 상태 오류");
            }

            Destroy().Destroy(form, flags, Hooks());
        }
    }

    /// <summary>원본 004ae300의 부모·시각 관문을 지나 가상 사건을 부르고 반환값으로 재예약·삭제한다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    /// <param name="process">실행할 정기 프로세스다.</param>
    public void RunRegular(RegularProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<byte> raw = _pool.Slot(process.Parent());
        if ((raw[StateOffset] & (FreeBit | DeadBit)) != 0)
        {
            return;
        }

        // 간헐천은 0.5초보다 먼 예약을 현재 시각으로 당긴다
        if (raw[TypeOffset] == _state.PollType && process.Time() - _state.Now > PollLimit)
        {
            process.SetNextTime(_state.Now);
        }

        // 패치판만 추상 부모(factory·vortex·dais 제외)를 건너뛴다. CD판은 genus를 읽기만 한다
        if (patch && (raw[ParentAbstractOffset] & 1) != 0 && (Type(raw[TypeOffset]).Flags2 & AbstractRunGenus) == 0u)
        {
            return;
        }

        // 아직 시각이 안 됐거나 비교할 수 없으면(NaN) 실행하지 않는다
        if (!(process.Time() <= _state.Now))
        {
            return;
        }

        if ((raw[StateOffset] & VoidBit) != 0)
        {
            return;
        }

        uint pid = process.KernelId;
        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            RegularHandler? handler = Regular();
            float result = handler is not null ? handler(process.Parent(), process.Event(), process.Count(), process.Payload()) : process.Payload();
            // 처리기가 이 프로세스를 지웠으면 더 건드리지 않는다
            if (!ReferenceEquals(_kernel.Get(pid), process))
            {
                return;
            }

            if (result > 0.0f || (patch && float.IsNaN(result)))
            {
                // 반환값이 다음 payload와 지연이 된다. 패치판은 NaN도 재예약으로 취급한다
                process.ApplyHandlerResult(result, _state.Now);
            }
            else if (result == 0.0f)
            {
                Kill(process, 0u);
            }
        }
    }

    /// <summary>원본의 선택·분리·효과 분배를 같은 현재 대여·자산에 연결한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    /// <returns>현재 삭제 효과 묶음이다.</returns>
    public SquidDestroyHooks Hooks()
    {
        var hooks = new SquidDestroyHooks();
        if (_currentRuntime is not null)
        {
            // form은 선택 대상이 아니므로 현재 자산 효과를 읽어 답한다
            hooks.Selected = () =>
            {
                using (var call = new RuntimeCallGuard(_currentRuntime()))
                {
                    SquidDestroyHooks? asset = Asset();
                    return asset?.Selected is not null ? asset.Selected() : 0;
                }
            };
        }
        else if (_asset?.Selected is not null)
        {
            hooks.Selected = _asset.Selected;
        }
        else
        {
            hooks.Selected = () => 0;
        }

        hooks.UnpopForm = UnpopForm;
        hooks.Emit = e =>
        {
            using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
            {
                // pre/post는 루트가, release/destroy는 종속이 ProcessForm일 때만 여기서 처리한다
                if (IsProcessForm(e.Sid))
                {
                    switch (e.Effect)
                    {
                        case SquidDestroyEffect.PreDestroy:
                            FormPreDestroy(e.Sid, e.Flags);
                            return;
                        case SquidDestroyEffect.PostDestroy:
                            Destroy().CompletePostDestroy();
                            return;
                        case SquidDestroyEffect.ReleaseDependent:
                            FormParentDying(e.Sid, e.Parent);
                            return;
                        case SquidDestroyEffect.DestroyDependent:
                            Destroy().Destroy(e.Sid, e.Flags, Hooks());
                            return;
                    }
                }

                SquidDestroyHooks? asset = Asset();
                if (asset?.Emit is null)
                {
                    throw new InvalidOperationException("프로세스 host의 자산 삭제 훅 미연결");
                }

                asset.Emit(e);
            }
        };
        return hooks;
    }

    /// <summary>가장 최근에 붙은 종속부터 다음 링크를 따라간다. 자산 타입은 건너뛴다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="match">고른 프로세스 판정이다.</param>
    private RegularProcess? Find(int parent, Func<RegularProcess, bool> match)
    {
        ArgumentNullException.ThrowIfNull(match);
        if (parent == 0 || parent >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), parent, "이벤트 조회 SID 범위 오류");
        }

        int current = Word(parent, HeadOffset);
        uint guard = 0u;
        // 다음 WORD·현재 content DWORD를 원문 위치에서 읽는 반복문이다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.
        while (current != 0)
        {
            if (current >= _pool.Capacity || ++guard > (uint)_pool.Capacity)
            {
                throw new InvalidOperationException("종속 체인 손상");
            }

            ReadOnlySpan<byte> raw = _pool.Slot(current);
            int next = Word(current, NextOffset);
            byte type = raw[TypeOffset];
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            bool process = !patch || (type >= FirstProcessType && type <= LastFormType && (int)type < unchecked((int)ProcessTypeEnd()));
            if (process && (type == (patch ? _state.RegularType() : RegularTypeNumber) || type == (patch ? _state.SharedRegularType() : SharedRegularTypeNumber)))
            {
                uint pid = (uint)raw[FormPidOffset] | ((uint)raw[FormPidOffset + 1] << 8) | ((uint)raw[FormPidOffset + 2] << 16) | ((uint)raw[FormPidOffset + 3] << 24);
                var regular = _kernel.Get(pid) as RegularProcess;
                if (regular is null)
                {
                    throw new InvalidOperationException("현재 Regular 종속의 커널 PID·실제 클래스 미연결");
                }

                if (match(regular))
                {
                    return regular;
                }
            }

            current = next;
        }

        return null;
    }

    /// <summary>부착 플래그의 client 번호 비트다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    private const uint ClientForm = 0x10u;

    /// <summary>다음 종속 번호(16비트)의 위치다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const int NextOffset = 4;

    /// <summary>종속 머리(16비트)의 위치다. 부모가 첫 종속을 가리킨다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const int HeadOffset = 6;

    /// <summary>타입 번호(1바이트)의 위치다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>상태 바이트의 위치다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    private const int StateOffset = 11;

    /// <summary>상태 바이트의 빈 슬롯 비트다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 죽음 비트다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 공간 없음(void) 비트다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    private const byte VoidBit = 4;

    /// <summary>부모 추상 비트의 위치다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    private const int ParentAbstractOffset = 40;

    /// <summary>추상 상태여도 실제 Regular를 실행하는 부모 genus 비트다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    private const uint AbstractRunGenus = 0x404200u;

    /// <summary>간헐천에 붙은 Regular가 허용하는 가장 먼 예약(초)이다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    private const double PollLimit = 0.5;

    /// <summary>Form의 비정렬 커널 번호 DWORD 위치다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const int FormPidOffset = 18;

    /// <summary>프로세스 타입의 첫 번호다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const uint FirstProcessType = 10u;

    /// <summary>일반 Regular 타입 번호다. CD판은 현재값 대신 이 상수를 쓴다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const uint RegularTypeNumber = 46u;

    /// <summary>공유 Regular 타입 번호다. CD판은 현재값 대신 이 상수를 쓴다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const uint SharedRegularTypeNumber = 61u;

    /// <summary>form으로 취급하는 마지막 번호다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private const uint LastFormType = 0x45u;

    /// <summary>raw WORD를 호스트 정렬과 무관하게 읽는다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    /// <param name="sid">읽을 번호다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private ushort Word(int sid, int offset)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>정렬되지 않은 form의 현재 커널 번호를 읽는다. 다른 종류의 기본 프로세스는 null로 거부한다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    /// <param name="form">조회할 Form 번호다.</param>
    /// <returns>찾은 프로세스다. 없으면 null이다.</returns>
    public SquidProcess? FromForm(int form)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(form);
        uint pid = (uint)raw[18] | ((uint)raw[19] << 8) | ((uint)raw[20] << 16) | ((uint)raw[21] << 24);
        return _kernel.Get(pid) as SquidProcess;
    }

    /// <summary>인자 계산 뒤 현재 번호를 읽고 실제 선택 장치만 요구한다. 지정 슬롯의 Take 반환은 원문처럼 무시하고 원래 SID를 후단에 사용한다. 부착 본체 단위에서 소비한다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="form">고를 Form SID다. 0이면 신규 생성이다.</param>
    /// <returns>쓸 Form SID다.</returns>
    internal int CreateOrTakeForm(uint flags, int form)
    {
        RawSquidFactory factory;
        if (_currentRuntime is not null)
        {
            ProcessHostRuntimeBindings runtime = Runtime();
            if (runtime.Factory is null)
            {
                throw new InvalidOperationException("현재 월드의 ProcessForm 공장 미연결");
            }

            factory = runtime.Factory;
        }
        else if (_directFormFactory is not null)
        {
            factory = _directFormFactory;
        }
        else
        {
            factory = new RawSquidFactory(_pool, _types);
        }

        if (!ReferenceEquals(factory.Pool, _pool))
        {
            throw new ArgumentException("ProcessForm 공장의 실제 Pool 불일치");
        }

        uint type = _state.ProcessFormType();
        if (form != 0)
        {
            factory.Take(type, form);
            return form;
        }

        return factory.Create(type, (flags & ClientForm) != 0u ? SidAllocation.Client : SidAllocation.Server);
    }

    /// <summary>C++ 소유권을 가진 신규 입력도 같은 원문 부모에 전달한다. 동일 포인터는 해제 후 재수신으로 넘긴다. 2026-11-07 F2d 프로세스 호스트 부착 본체 이식에서 추가됐다.</summary>
    /// <param name="process">부를 프로세스다. 실패하면 소멸 효과를 실행한다.</param>
    /// <param name="processType">부착할 프로세스 타입 번호다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="form">고를 Form SID다. 0이면 신규 생성이다.</param>
    /// <returns>붙은 프로세스다. 부분 실패면 null이다.</returns>
    public SquidProcess? Attach(SquidProcess process, uint processType, int parent, uint flags, int form = 0)
    {
        ArgumentNullException.ThrowIfNull(process);
        if (_kernel.Find(process) != 0u)
        {
            return Reattach(process, processType, parent, flags, form);
        }

        bool attached;
        try
        {
            attached = AttachBase(process, processType, parent, flags, form, true);
        }
        catch
        {
            process.Dispose();
            throw;
        }

        if (!attached)
        {
            process.Dispose();
            return null;
        }

        return process;
    }

    /// <summary>실제 Kernel이 소유한 기존 포인터는 새 소유권 없이 같은 상위 부모에 등록한다. 2026-11-07 F2d 프로세스 호스트 부착 본체 이식에서 추가됐다.</summary>
    /// <param name="process">다시 받을 프로세스다.</param>
    /// <param name="processType">부착할 프로세스 타입 번호다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="form">고를 Form SID다.</param>
    /// <returns>붙은 프로세스다. 부분 실패면 null이다.</returns>
    public SquidProcess? Reattach(SquidProcess process, uint processType, int parent, uint flags, int form)
    {
        ArgumentNullException.ThrowIfNull(process);
        if (_kernel.Find(process) == 0u || (process.HostLink is not null && !ReferenceEquals(process.HostLink, this)))
        {
            throw new ArgumentException("재수신 프로세스의 C++ Kernel/host 소유권 불일치", nameof(process));
        }

        return AttachBase(process, processType, parent, flags, form, false) ? process : null;
    }

    /// <summary>장기 Host의 현재 장치와 실제 두 필드를 원문 부모에 제공하며 부분 실패를 숨기지 않는다. 2026-11-07 F2d 프로세스 호스트 부착 본체 이식에서 추가됐다.</summary>
    /// <param name="process">부를 프로세스다.</param>
    /// <param name="processType">부착할 프로세스 타입 번호다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="form">고를 Form SID다. 0이면 신규 생성이다.</param>
    /// <param name="owned">커널 등록을 새 소유로 넘기면 참이다.</param>
    /// <returns>부착했으면 참이다.</returns>
    private bool AttachBase(SquidProcess process, uint processType, int parent, uint flags, int form, bool owned)
    {
        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            process.HostLink = this;
            process.RuntimeLink = _currentRuntime?.Invoke();
            uint pid = 0u;
            var hooks = new BaseProcessAttachHooks
            {
                TypeEnd = () => ProcessTypeEnd(),
                ProcessFormType = () => _state.ProcessFormType(),
                Assertion = (expression, file, line) =>
                {
                    Action<string, string, uint>? device = _currentRuntime is not null ? Runtime().Assertion : _state.Assertion;
                    if (device is null)
                    {
                        throw new InvalidOperationException("BaseProcess 부착의 선택된 원문 진단 장치 미연결");
                    }

                    device(expression, file, line);
                },
                RegisterProcess = () =>
                {
                    if (_pool.Edition == OriginalEdition.Patch1078)
                    {
                        _state.TypeRotation = unchecked((_state.TypeRotation + 1u) % (uint)TypeCount());
                    }

                    uint assigned = owned ? _kernel.Add(process) : _kernel.AddRef(process);
                    pid = assigned;
                    return assigned;
                },
                Create = allocationFlags => CreateOrTakeForm(allocationFlags == 2u ? ClientForm : 0u, 0),
                Take = selected => { CreateOrTakeForm(0u, selected); },
                Parent = selected => RawContainedLink.CommonContainedParent(_pool, selected),
                Attach = (selected, into, popFlags) => AttachForm(selected, into, popFlags),
                OnAttached = () => process.OnAttached(),
            };
            var fields = new BaseProcessAttachFields();
            bool attached = RawBaseProcessAttach.Attach(_pool, fields, processType, parent, form, flags, hooks);
            if (attached)
            {
                process.SetIdentity(fields.Form, fields.Parent, pid);
            }

            return attached;
        }
    }

    /// <summary>서버 조회가 저장한 실제 포인터만 writer에서 사용하며 파생 직렬화 접두 뒤에 공통 부착을 실행한다. 2026-11-07 F2d 프로세스 호스트 부착 본체 이식에서 추가됐다.</summary>
    /// <param name="form">부를 Form SID다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    public void AttachForm(int form, int parent, uint flags)
    {
        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            SquidProcess? selected = null;
            Action<int, int, uint> attach = (attachForm, attachParent, attachFlags) =>
            {
                if (_currentRuntime is not null)
                {
                    ProcessHostRuntimeBindings bindings = Runtime();
                    if (bindings.Factory is null)
                    {
                        throw new InvalidOperationException("현재 World의 공통 Form 부착 공장 미연결");
                    }

                    bindings.Factory.AttachForm(attachForm, attachParent, attachFlags);
                }
                else if (_directFormFactory is not null)
                {
                    _directFormFactory.AttachForm(attachForm, attachParent, attachFlags);
                }
                else
                {
                    // 독립 입력도 같은 현재 표·진단 부모를 쓴다
                    new RawSquidFactory(_pool, _types).AttachForm(attachForm, attachParent, attachFlags);
                }
            };
            var hooks = new ProcessAttachHooks
            {
                Server = () => _pool.IsServer,
                Lookup = sid => { selected = FromForm(sid); },
                WriteData = changed =>
                {
                    if (selected is null)
                    {
                        throw new InvalidOperationException("Process 부착의 선택된 실제 PID 미연결");
                    }

                    ProcessFormTransmission? device = Transmission();
                    if (device?.WriteData is null)
                    {
                        throw new InvalidOperationException("Process 부착의 선택된 실제 writer 미연결");
                    }

                    return device.WriteData(selected, changed);
                },
                Transmit = size =>
                {
                    ProcessFormTransmission? device = Transmission();
                    if (device?.Transmit is null)
                    {
                        throw new InvalidOperationException("Process 부착의 선택된 실제 송신 장치 미연결");
                    }

                    device.Transmit(size);
                },
                Assertion = (expression, file, line) =>
                {
                    Action<string, string, uint>? device = _currentRuntime is not null ? Runtime().Assertion : _state.Assertion;
                    if (device is null)
                    {
                        throw new InvalidOperationException("Process 부착의 선택된 원문 진단 장치 미연결");
                    }

                    device(expression, file, line);
                },
                Attach = attach,
            };
            // TYPE를 바꾸어도 실제 생성 VT의 +74를 선택한다
            uint table = (uint)Word(form, 0) | ((uint)Word(form, 2) << 16);
            if (table == FormVtable())
            {
                RawProcessAttach.Attach(_pool, form, parent, flags, hooks);
                return;
            }

            bool common = false;
            foreach (uint kind in new uint[] { 1u, 3u, 6u })
            {
                if (table == RawSquidForms.VtableFor(kind))
                {
                    common = true;
                }
            }

            if (!common)
            {
                throw new InvalidOperationException("BaseProcess 부착의 미복원 현재 VT+74");
            }

            attach(form, parent, flags);
        }
    }

    /// <summary>현재 raw 부모를 다시 읽어 프로세스의 오래된 parent 복사값으로 재부착하지 않는다. 2026-11-07 F2d 프로세스 호스트 부착 본체 이식에서 추가됐다.</summary>
    /// <param name="process">갱신할 프로세스다.</param>
    public void RefreshForm(SquidProcess process)
    {
        ArgumentNullException.ThrowIfNull(process);
        using (var call = new RuntimeCallGuard(_currentRuntime?.Invoke()))
        {
            if (!ReferenceEquals(process.HostLink, this) || !IsRegistered(process.KernelId, process))
            {
                throw new InvalidOperationException("Form 갱신 프로세스의 C++ 소유권 불일치");
            }

            // 부모는 첫 getter에서 저장하고 Form 필드는 각 가상 진입 전에 다시 읽는다
            int parent = RawContainedLink.CommonContainedParent(_pool, process.Form());
            ReadOnlySpan<byte> raw = _pool.Slot(parent);
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            Action<string, uint, uint> diagnose = (expression, patchLine, cdLine) =>
            {
                Action<string, string, uint>? device = _currentRuntime is not null ? Runtime().Assertion : _state.Assertion;
                if (device is null)
                {
                    throw new InvalidOperationException("Form 갱신의 선택된 원문 진단 장치 미연결");
                }

                device(expression, patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp", patch ? patchLine : cdLine);
            };
            if ((raw[11] & DeadBit) != 0)
            {
                diagnose("!parent.isDead()", 0xb0u, 0xabu);
            }

            if ((raw[11] & FreeBit) != 0)
            {
                diagnose("!parent.isFree()", 0xb1u, 0xacu);
            }

            UnpopForm(process.Form());
            AttachForm(process.Form(), parent, 0u);
        }
    }
}
