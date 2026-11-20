// Factory/아웃포스트의 예약·상태 프레임·중복 삭제·목록 등록을 같은 raw 월드에서 실행한다.
namespace Netstorm.Core.Original;

// 공장 후처리 몸체를 담는 네임스페이스다. 2026-11-19 0.624에서 R4-2 공장 정기 트리거용으로 추가했다.

/// <summary>
/// 원본이 직접 읽는 타입 번호와 권한이다. Client는 현재 DWORD를 빌리고 독립 입력은 기존 값 사본을 쓴다.
/// 근거: cpppj <c>o/RawFactoryPostPop.h</c>의 <c>FactoryPostPopTypes</c>.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 정기 트리거용으로 추가했다.</remarks>
public sealed class FactoryPostPopTypes
{
    /// <summary>썬 워크샾 타입이다. 최초 event2를 8초 뒤에 예약한다. 2026-11-19 0.624에서 추가했다.</summary>
    public uint FirstTimerFactory { get; set; } = 85u;

    /// <summary>최초 event1을 0.5초 뒤에 예약하는 타입이다. 2026-11-19 0.624에서 추가했다.</summary>
    public uint SecondTimer { get; set; } = 87u;

    /// <summary>상태별 프레임 보정을 건너뛰는 타입이다. 2026-11-19 0.624에서 추가했다.</summary>
    public uint FixedFrame { get; set; } = 126u;

    /// <summary>실제 Client의 005411ac을 선택적으로 빌린다. 없으면 기존 소유 값을 쓴다. 2026-11-19 0.624에서 추가했다.</summary>
    public Func<uint>? CurrentFirstTimer { get; set; }

    /// <summary>실제 Client의 005411b4를 선택적으로 빌린다. 없으면 기존 소유 값을 쓴다. 2026-11-19 0.624에서 추가했다.</summary>
    public Func<uint>? CurrentSecondTimer { get; set; }

    /// <summary>실제 Client의 005412c8을 선택적으로 빌린다. 없으면 기존 소유 값을 쓴다. 2026-11-19 0.624에서 추가했다.</summary>
    public Func<uint>? CurrentFixedFrame { get; set; }

    /// <summary>현재 첫 타이머 값을 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <returns>빌린 값 또는 기존 소유 값이다.</returns>
    public uint ReadFirstTimer() => CurrentFirstTimer?.Invoke() ?? FirstTimerFactory;

    /// <summary>현재 두 번째 타이머 값을 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <returns>빌린 값 또는 기존 소유 값이다.</returns>
    public uint ReadSecondTimer() => CurrentSecondTimer?.Invoke() ?? SecondTimer;

    /// <summary>현재 고정 프레임 값을 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <returns>빌린 값 또는 기존 소유 값이다.</returns>
    public uint ReadFixedFrame() => CurrentFixedFrame?.Invoke() ?? FixedFrame;
}

/// <summary>
/// 몸체가 요구하는 실제 프로세스·프레임 가상 효과와 아웃포스트 소유권 재계산 경계다.
/// 근거: cpppj <c>o/RawFactoryPostPop.h</c>의 <c>FactoryPostPopHooks</c>.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 정기 트리거용으로 추가했다.</remarks>
public sealed class FactoryPostPopHooks
{
    /// <summary>원본 각 new(0x28)의 독립 성공 여부다. 2026-11-19 0.624에서 추가했다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>같은 SID의 Regular 생성·부착 전체를 실행한다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>SquidFrame의 dirty/Unpop/Repop 몸체를 연결한다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }

    /// <summary>아웃포스트와 신전으로 섬 소유권을 다시 계산한다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<float, float>? RegionOwnership { get; set; }

    /// <summary>중복 워크샾 삭제 직전의 원본 경고 로그다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<int, int, int>? WarnDuplicate { get; set; }

    /// <summary>실제 같은 풀의 가상 +0x10 삭제이며 flags 0으로 전달한다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>가상84의 Level 저장 뒤 실제 가상88 기본 표시 갱신이다. 2026-11-19 0.624에서 추가했다.</summary>
    public Action<int>? UpdateState { get; set; }

    /// <summary>프레임 보정의 각 가상80 호출을 현재 VT로 다시 분배한다. 2026-11-19 0.624에서 추가했다.</summary>
    public Func<int, uint>? VirtualCount { get; set; }
}

/// <summary>
/// 실제 vtable +0x20과 +0x80이 검증된 Factory 부류만 받아 공통 깊이를 한 번 줄인다.
/// 근거: cpppj <c>o/RawFactoryPostPop.h/.cpp</c>의 <c>RawFactoryPostPop</c>, 원본 00455380/CD 0047b3c0.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 정기 트리거용으로 추가했다.</remarks>
public sealed class RawFactoryPostPop
{
    /// <summary>같은 풀의 슬롯 저장소다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>중복 워크샾 검사가 읽는 같은 월드의 실제 공간 해시다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>일반 finder의 발자국·플래그 표다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>상태별 첫 프레임/개수 구간을 읽는 현재 프레임 표다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>공통 몸체. 공통 깊이를 한 번 줄인다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly RawPostPop _base;

    /// <summary>직접 Damageable 후처리다. 한 번 실행한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly RawCarrierPostPop _damageable;

    /// <summary>boss·아웃포스트/거주 구역 목록 상태다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly AssetPostPopState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 콜백이다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly FactoryPostPopHooks _hooks;

    /// <summary>원본이 직접 읽는 타입 번호와 권한이다. 2026-11-19 0.624에서 추가했다.</summary>
    private readonly FactoryPostPopTypes _numbers;

    /// <summary>
    /// 풀·해시·타입·프레임·공통 몸체·상태를 받는다. 표면/객체 사본은 만들지 않는다.
    /// </summary>
    /// <param name="pool">슬롯 풀. 기본·Damageable 몸체와 같아야 한다.</param>
    /// <param name="hash">중복 검사용 공간 해시다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 판본 크기와 같아야 한다.</param>
    /// <param name="frames">타입별 프레임 표다. 타입 표와 길이가 같아야 한다.</param>
    /// <param name="base">공통 후처리 몸체다.</param>
    /// <param name="damageable">직접 Damageable 후처리다.</param>
    /// <param name="state">boss·목록 상태다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <param name="numbers">직접 읽는 타입 번호와 권한이다. 없으면 기본값을 쓴다.</param>
    /// <exception cref="ArgumentException">풀/판본/프레임 표 불일치다.</exception>
    public RawFactoryPostPop(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, RawPostPop @base, RawCarrierPostPop damageable, AssetPostPopState state, FactoryPostPopHooks hooks, FactoryPostPopTypes? numbers = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(@base);
        ArgumentNullException.ThrowIfNull(damageable);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, @base.Pool) || !ReferenceEquals(pool, damageable.Pool) || types.Count != frames.Count
            || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u))
        {
            throw new ArgumentException("Factory 후처리 풀/판본/프레임 표 불일치", nameof(types));
        }

        _pool = pool;
        _hash = hash;
        _types = types;
        _frames = frames;
        _base = @base;
        _damageable = damageable;
        _state = state;
        _hooks = hooks;
        // 원본 생성자는 타입 번호 구조체를 값으로 복사한다. 호출 뒤 원본 구조체 변경은 반영하지 않되 현재 DWORD 대리자는 공유한다. 2026-11-19 0.624에서 독립 사본 계약으로 수정했다.
        _numbers = numbers is null
            ? new FactoryPostPopTypes()
            : new FactoryPostPopTypes
            {
                FirstTimerFactory = numbers.FirstTimerFactory,
                SecondTimer = numbers.SecondTimer,
                FixedFrame = numbers.FixedFrame,
                CurrentFirstTimer = numbers.CurrentFirstTimer,
                CurrentSecondTimer = numbers.CurrentSecondTimer,
                CurrentFixedFrame = numbers.CurrentFixedFrame,
            };
    }

    /// <summary>같은 월드의 실제 지역 조회인지 확인한다.</summary>
    /// <returns>이 몸체가 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>실제 생성자가 기록하는 vtable 시작 주소와 Ghidra의 표 조각 시작 주소를 구별한다.</summary>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>1은 Factory, 2는 아웃포스트 wrapper, 0은 미복원이다.</returns>
    private uint Kind(int sid)
    {
        uint table = Read(_pool.Slot(sid), 0);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (table == (patch ? 0x00509210u : 0x00503358u))
        {
            return 2;
        }

        if (table == (patch ? 0x00509148u : 0x00503298u))
        {
            return 1;
        }

        return 0;
    }

    /// <summary>타입 이름이 일치해도 다른 가상 표의 wrapper는 허용하지 않는다.</summary>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>Factory 부류이면 참이다.</returns>
    public bool Handles(int sid) => Kind(sid) != 0;

    /// <summary>실제 흐름에서 쓰는 효과만 요구하되 콜백 뒤 분기 변화도 필수 경계로 유지한다.</summary>
    /// <param name="sid">검사할 슬롯 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    /// <exception cref="ArgumentException">가상 표·타입 미복원·필수 효과 누락이다.</exception>
    public void Validate(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint kind = Kind(sid);
        if (kind == 0 || raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentException("Factory 후처리 가상 표·타입 미복원", nameof(sid));
        }

        _base.ValidateBase(sid, flags);
        if ((flags & 1) != 0 && (_hooks.ReserveRegular is null || _hooks.AddRegular is null))
        {
            throw new ArgumentException("Factory Regular 경계 누락", nameof(sid));
        }

        if (_hooks.SetFrame is null)
        {
            throw new ArgumentException("Factory 실제 프레임 설정 경계 누락", nameof(sid));
        }

        if (_state.Boss && (_hooks.Destroy is null || _hooks.WarnDuplicate is null))
        {
            throw new ArgumentException("Factory 중복 삭제/경고 경계 누락", nameof(sid));
        }

        if (kind == 2)
        {
            _ = _state.CurrentOutposts.Items().Length;
            _ = _state.CurrentResidences.Items().Length;
            if (_hooks.RegionOwnership is null)
            {
                throw new ArgumentException("아웃포스트 실제 섬 소유권 경계 누락", nameof(sid));
            }
        }
    }

    /// <summary>가상 호출을 매번 현재 VT로 분배한다. 독립 네 표의 base getter만 허용한다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>현재 가상80 개수다.</returns>
    private int Count(int sid)
    {
        if (_hooks.VirtualCount is not null)
        {
            return unchecked((int)FactoryNeed(_hooks.VirtualCount)(sid));
        }

        if (Kind(sid) == 0)
        {
            throw new InvalidOperationException("Factory 프레임의 현재 가상80 미연결");
        }

        if (_pool.Slot(sid)[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Factory Count의 현재 타입 물리 범위 오류");
        }

        return _pool.Slot(sid)[RawSidPool.StateOffset] >> 5;
    }

    /// <summary>각 가상 호출이 바꾼 TYPE로 프레임 표를 읽고 물리 범위를 확인한다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>현재 타입의 프레임 표다.</returns>
    private RiftTypeFrames CurrentFrames(int sid)
    {
        byte number = _pool.Slot(sid)[RawSidPool.TypeOffset];
        if (number >= _frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Factory 현재 프레임 타입 물리 범위 오류");
        }

        return _frames[number];
    }

    /// <summary>실제 Factory 가상84 뒤 같은 실제 가상88 기본 표시 갱신을 호출한다.</summary>
    /// <param name="sid">보정할 슬롯 번호다.</param>
    public void AdjustFrame(int sid)
    {
        if (_pool.Slot(sid)[RawSidPool.TypeOffset] == _numbers.ReadFixedFrame())
        {
            return;
        }

        int frame = _pool.Edition == OriginalEdition.Patch1078
            ? unchecked((int)Read(_pool.Slot(sid), 36))
            : _pool.Slot(sid)[34];
        int count = Count(sid);
        if (count < 0 || count >= 16)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Factory 현재 프레임 글자 물리 범위 오류");
        }

        RiftTypeFrames.LetterRun run = CurrentFrames(sid).Run(unchecked((byte)('A' + count)));
        if (run.First <= frame)
        {
            int next = Count(sid);
            if (next < 0 || next >= 16)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "Factory 현재 프레임 개수 글자 물리 범위 오류");
            }

            RiftTypeFrames.LetterRun current = CurrentFrames(sid).Run(unchecked((byte)('A' + next)));
            // 원본 signed DWORD 덧셈 감김을 유지한다. 부호 있는 비교다.
            int end = unchecked((int)(unchecked((uint)run.First) + unchecked((uint)current.Count)));
            if (frame < end)
            {
                return;
            }
        }

        FactoryNeed(_hooks.SetFrame)(sid, run.First, 0);
    }

    /// <summary>각각의 실패는 해당 예약만 건너뛰고 다음 호출 순서를 유지한다.</summary>
    /// <param name="sid">예약할 슬롯 번호다.</param>
    /// <param name="event">예약 사건 번호다.</param>
    /// <param name="delay">예약 지연(초)이다.</param>
    private void Regular(int sid, uint @event, float delay)
    {
        if (FactoryNeed(_hooks.ReserveRegular)())
        {
            FactoryNeed(_hooks.AddRegular)(sid, @event, delay);
        }
    }

    /// <summary>wrapper 등록→예약→공통 몸체→프레임→현재 공간 순서가 원본과 같다.</summary>
    /// <param name="sid">후처리할 슬롯 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    public void PostPop(int sid, uint flags)
    {
        if (_pool.Edition != OriginalEdition.Patch1078)
        {
            Validate(sid, flags);
        }

        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int extraOffset = patch ? 40 : 35;
        if (Kind(sid) == 2 && (flags & 1) != 0 && (_pool.Slot(sid)[extraOffset] & 9) == 0)
        {
            AddUnique(_state.CurrentOutposts, sid);
            AddUnique(_state.CurrentResidences, sid);
            FactoryNeed(_hooks.RegionOwnership)(Position(_pool.Slot(sid), 14), Position(_pool.Slot(sid), 18));
        }

        if ((flags & 1) != 0)
        {
            if (_pool.Slot(sid)[RawSidPool.TypeOffset] == _numbers.ReadFirstTimer())
            {
                Regular(sid, 2, 8.0f);
            }

            if (_pool.Slot(sid)[RawSidPool.TypeOffset] == _numbers.ReadSecondTimer())
            {
                Regular(sid, 1, 0.5f);
            }

            if (patch)
            {
                Regular(sid, 5, 0.5f);
            }
        }

        _damageable.DamageablePostPop(sid, flags);
        AdjustFrame(sid);
        if (!_state.Boss)
        {
            return;
        }

        int x = Truncate(Position(_pool.Slot(sid), 14));
        int y = Truncate(Position(_pool.Slot(sid), 18));
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 실제 finder는 다음 raw 연결을 먼저 저장하므로 가상 삭제 뒤에도 원본 순서로 진행한다.
        for (int candidate = finder.Begin(new SquidSearchArea(x, y, x, y)); candidate != 0; candidate = finder.Next())
        {
            if (candidate != sid && _pool.Slot(candidate)[RawSidPool.TypeOffset] == _pool.Slot(sid)[RawSidPool.TypeOffset]
                && ((_pool.Slot(candidate)[extraOffset] ^ _pool.Slot(sid)[extraOffset]) & 8) == 0)
            {
                FactoryNeed(_hooks.WarnDuplicate)(candidate, Truncate(Position(_pool.Slot(candidate), 14)), Truncate(Position(_pool.Slot(candidate), 18)));
                FactoryNeed(_hooks.Destroy)(candidate, 0);
            }
        }
    }

    /// <summary>실제 Factory 가상84의 Level 저장 뒤 같은 가상88 기본 표시 갱신을 호출한다. 표시 장치는 저장 후 호출 지점에서 요구한다.</summary>
    /// <param name="level">같은 풀의 레벨 몸체다.</param>
    /// <param name="sid">저장할 슬롯 번호다.</param>
    /// <param name="value">쓸 레벨 값이다.</param>
    public void SetState(RawSquidLevel level, int sid, uint value)
    {
        ArgumentNullException.ThrowIfNull(level);
        if (!ReferenceEquals(level.Pool, _pool) || !Handles(sid))
        {
            throw new ArgumentException("Factory State의 같은 Pool·가상 표 경계 불일치", nameof(sid));
        }

        level.Set(sid, value);
        FactoryNeed(_hooks.UpdateState)(sid);
    }

    /// <summary>정렬되지 않은 little endian DWORD를 현재 슬롯에서 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>결합한 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 현재 필드의 네 바이트만 조합한다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return value;
    }

    /// <summary>원본 float 좌표를 전달할 때 비트와 소수 부분을 그대로 유지한다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>비트 그대로의 float 값이다.</returns>
    private static float Position(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));

    /// <summary>원본 CRT ftol의 signed64 범위와 low DWORD를 유지하며 손상 입력은 거부한다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>0 방향 절삭 정수 좌표다.</returns>
    private static int Truncate(float value)
    {
        double wide = value;
        if (!double.IsFinite(wide) || wide < -9223372036854775808.0 || wide >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "Factory 좌표 CRT 범위 오류");
        }

        return unchecked((int)(long)wide);
    }

    /// <summary>두 판본 모두 목록 용량 안에서 중복을 건너뛰고 SID를 추가한다.</summary>
    /// <param name="list">추가할 목록이다.</param>
    /// <param name="sid">추가할 슬롯 번호다.</param>
    private static void AddUnique(RawPostPopList list, int sid)
    {
        ArgumentNullException.ThrowIfNull(list);
        uint used = list.Count;
        uint capacity = list.Capacity;
        // 활성 범위 밖 stale 꼬리는 건드리지 않는다.
        if (used >= capacity)
        {
            return;
        }

        // 현재 활성 항목에 중복이 있으면 건너뛴다.
        foreach (uint entry in list.Items())
        {
            if (entry == (uint)sid)
            {
                return;
            }
        }

        list.Storage[checked((int)used)] = unchecked((uint)sid);
        list.Count = unchecked(used + 1);
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식)이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T FactoryNeed<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Factory 후처리의 선택된 실제 장치 미연결");
        }

        return function;
    }

    /// <summary>다른 풀의 가상 프레임 효과를 거부하며 실제 dirty/공간 순서를 그대로 호출한다.</summary>
    /// <param name="pool">같은 풀의 슬롯 저장소다.</param>
    /// <param name="frame">같은 풀의 프레임 몸체다.</param>
    /// <param name="hooks">나머지 효과 묶음이다. setFrame만 채운다.</param>
    /// <returns>setFrame이 연결된 갈고리 묶음이다.</returns>
    public static FactoryPostPopHooks MakeFactoryPostPopHooks(RawSidPool pool, SquidFrame frame, FactoryPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frame);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, frame.Pool))
        {
            throw new ArgumentException("Factory 프레임 지정 풀 불일치", nameof(pool));
        }

        hooks.SetFrame = (sid, value, flags) => frame.Set(sid, value, flags);
        return hooks;
    }
}
