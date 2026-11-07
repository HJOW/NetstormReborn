// 객체 Form과 커널 프로세스의 부착 기반(생성·조회·번호)을 공유한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

// 프로세스 호스트 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.

// 객체 Form과 커널 프로세스의 부착·통지·수명을 연결하는 호스트 껍데기다. 부착·검색·삭제·전송 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.
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
}
