namespace Netstorm.Core.Original;

/// <summary>
/// 공통 PostPop 이 등록한 대상 번호를 모아 두는 목록(원본 SquidPostPopList).
/// 원본처럼 미리 확보한 배열과 현재 개수(count)를 따로 둔다. 개수 밖의 stale 값은 버퍼에 남기되 조회에서는 숨긴다.
/// </summary>
public sealed class RawPostPopList
{
    /// <summary>미리 확보한 번호 배열. 길이(capacity)가 그대로 유지되며 개수 밖의 stale 값도 검사 대조에 쓴다.</summary>
    public uint[] Entries { get; set; } = [];

    /// <summary>현재 항목 수. 배열 길이를 넘으면 손상된 목록이므로 조회 때 거부한다.</summary>
    public uint Count { get; set; }

    /// <summary>활성 항목만 읽기 전용으로 본다. 개수가 배열을 넘으면 예외다.</summary>
    public ReadOnlySpan<uint> Items()
    {
        // 손상된 개수는 목록 쓰기 전에 거부한다
        if (Count > (uint)Entries.Length)
        {
            throw new InvalidOperationException("postPop 목록 count 범위 오류");
        }
        return Entries.AsSpan(0, (int)Count);
    }

    /// <summary>번호가 활성 항목에 있는지 돌려준다.</summary>
    /// <param name="sid">찾을 슬롯 번호.</param>
    public bool Contains(int sid)
    {
        // 활성 범위만 앞에서부터 찾는다
        foreach (uint entry in Items())
        {
            if (entry == (uint)sid)
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>자리가 남았을 때만 번호를 덧붙인다. 가득 찼으면 기존 항목을 보존하고 생략한다(원본 Append).</summary>
    /// <param name="sid">덧붙일 슬롯 번호.</param>
    public void Append(int sid)
    {
        // 개수가 배열 안에 있을 때만 쓰고 개수를 늘린다
        if (Count < (uint)Entries.Length)
        {
            Entries[(int)Count++] = (uint)sid;
        }
    }
}

/// <summary>
/// 공통 PostPop 이 등록·조회하는 상태 묶음(원본 SquidPostPopState).
/// 전역 비용 집계·생산 재계산 알림·공통 깊이·타입별 세 통계표·공급원/작업장 목록을 담는다.
/// 이 인스턴스는 <see cref="RawPostPop"/> 과 같은 수명으로 공유하며 호출자가 State 행처럼 직접 채운다.
/// </summary>
public sealed class RawPostPopState
{
    /// <summary>소유자 수(0~8). 0 은 중립이며 배열은 9칸이다.</summary>
    public const int OwnerCount = 9;

    /// <summary>타입별 통계표의 칸 수(타입 번호로 접근한다).</summary>
    public const int TableSize = 256;

    /// <summary>표시·등록 후처리를 억제할지. 켜지면 통계·목록·비용을 건드리지 않고 깊이만 줄인다.</summary>
    public bool Suppressed { get; set; }

    /// <summary>표면 Graph 를 쓸지. 켜졌는데 영역·표면 등록이면 Graph 연결이 있어야 한다.</summary>
    public bool GraphsEnabled { get; set; }

    /// <summary>보류 배치가 있는지. 켜져 있으면 최초 등록의 비용 집계를 거부한다(미복원 선택 해제).</summary>
    public bool PendingPlacement { get; set; }

    /// <summary>로컬 소유자 번호. 소유자 통계를 로컬 표에 넣을 때 비교한다.</summary>
    public byte LocalOwner { get; set; }

    /// <summary>무효 Graph 번호. 표면 타입이 NoGraph 로 등록될 때 슬롯의 Graph 바이트에 쓴다(원본 254).</summary>
    public byte InvalidGraph { get; set; } = 254;

    /// <summary>소유자별 AI 부착 여부. 켜진 소유자의 최초 등록은 통지 미복원으로 거부한다.</summary>
    public bool[] AiAttached { get; } = new bool[OwnerCount];

    /// <summary>전역 비용 집계. 같은 타입을 다시 등록해도 최초 Pop 비트가 있으면 늘지 않는다.</summary>
    public int TotalCost { get; set; }

    /// <summary>공급·작업장 재계산 알림 수. 공급원·작업장 등록마다 1씩 늘어난다.</summary>
    public uint ProductionDirty { get; set; }

    /// <summary>Activate/postPop 깊이. Activate 가 1 늘리고 공통 몸체가 1 줄인다.</summary>
    public uint Depth { get; set; }

    /// <summary>로컬 소유자의 타입별 현재 수(앞 256칸만 쓴다).</summary>
    public uint[] LocalCounts { get; } = new uint[TableSize];

    /// <summary>로컬 소유자의 타입별 두 번째 현재 수(같은 조건으로 함께 늘어난다).</summary>
    public uint[] LocalSecondaryCounts { get; } = new uint[TableSize];

    /// <summary>모든 소유자의 타입별 현재 수.</summary>
    public uint[] GlobalCounts { get; } = new uint[TableSize];

    /// <summary>공급원 목록(공급 비트 타입의 최초 등록).</summary>
    public RawPostPopList Providers { get; } = new();

    /// <summary>작업장 목록(작업장 비트 타입의 최초 등록).</summary>
    public RawPostPopList Factories { get; } = new();

    /// <summary>소유자별 작업장 목록. 중립 0번도 메모리를 보존하며 검사 대조에 쓴다.</summary>
    public RawPostPopList[] OwnerFactories { get; } = Enumerable.Range(0, OwnerCount).Select(_ => new RawPostPopList()).ToArray();
}

/// <summary>
/// 공통 PostPop 이 표면 Graph 에 요구하는 연결 지점(원본 RawGraph 의 PostPop 쪽).
/// Graph 모듈이 생기면 구현한다. 연결 전(null)이면 영역·표면 등록은 거부된다.
/// </summary>
public interface IRawPostPopGraph
{
    /// <summary>이 Graph 가 쓰는 슬롯 풀. PostPop 과 같은 풀이어야 한다.</summary>
    RawSidPool Pool { get; }

    /// <summary>공간 쓰기 전에 영역·표면 등록을 Graph 가 받을 수 있는지 검사한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="region">영역 등록인지.</param>
    /// <param name="add">표면 생성 등록인지.</param>
    /// <param name="projection">등록 좌표·단계·타입.</param>
    void ValidatePostPop(int sid, bool region, bool add, in PopProjection projection);

    /// <summary>공통 몸체의 통계 쓰기와 함께 Graph 소속을 갱신한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="region">영역 등록인지.</param>
    /// <param name="add">표면 생성 등록인지.</param>
    void PostPop(int sid, bool region, bool add);

    /// <summary>Pop 과 같은 해시·점유 지도를 보는지 공간 쓰기 전에 확인한다.</summary>
    /// <param name="hash">Pop 이 쓰는 공간 해시.</param>
    /// <param name="spots">Pop 이 쓰는 칸별 점유 바이트 65,536개.</param>
    void ValidateSpace(RawSquidHash hash, byte[] spots);
}

/// <summary>
/// Pop 이후 타입별 등록 목록과 표면 연결 상태를 갱신하는 원본 공통 후처리(<c>004b0d30</c>, 10.78).
/// 순서는 원본 그대로 "상태 검사 → 파생 지원 검사 → Graph 검사 → 비용·목록·통계 검사(Validate) →
/// 영역 무효화·Graph 생성 → 비용 누적 → 공급원·작업장 목록 → 소유자 조건 통계 → 깊이 감소(PostPopBase)"다.
/// 파생 몸체(사제·운반체·자산·공장·룬·다이스·다리 등)는 아직 연결하지 않으므로 <see cref="HandlesDerived"/> 는 항상 거짓이며,
/// 그 효과가 필요한 가상 표의 등록은 풀·공간을 바꾸기 전에 거부한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 소유자·추가 비트 위치와 비용 누적 방식이 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/SquidPostPop.cpp (W02 PostPop 조각, 2026-10-20).
/// </summary>
public sealed class RawPostPop : IRawPopPostPop
{
    /// <summary>상태 바이트의 free 비트(빈 슬롯).</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 들어 있는 내용물(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>추가 비트의 abstract(1)·매립(8) 마스크. 켜져 있으면 최초 Pop 플래그가 2·4 로 갈린다.</summary>
    private const byte AbstractBuriedMask = 9;

    /// <summary>추가 비트의 최초 Pop(firstPop) 비트.</summary>
    private const byte FirstPopBit = 128;

    /// <summary>플래그2에서 영역 등록으로 보는 마스크(원본 0x50444208).</summary>
    private const uint RegionMask = 0x50444208;

    /// <summary>플래그2에서 작업장으로 보는 마스크(원본 0x4200).</summary>
    private const uint FactoryMask = 0x4200;

    /// <summary>플래그1에서 공급원으로 보는 비트(원본 0x10000000).</summary>
    private const uint ProviderBit = 0x10000000;

    /// <summary>Graph 를 건너뛰는 호출 플래그 비트(원본 0x2000000).</summary>
    private const uint NoGraphBit = 0x2000000;

    /// <summary>Graph 변경으로 보는 호출 플래그 마스크(원본 0x203).</summary>
    private const uint GraphChangeMask = 0x203;

    /// <summary>Graph 파괴 assert 경로의 호출 플래그 비트(원본 0x800). 들어오면 원본처럼 멈춘다.</summary>
    private const uint DestroyGraphBit = 0x800;

    /// <summary>패치판 비용 표가 타입 번호마다 더해 저장하는 정수(원본 23).</summary>
    private const long CostBias = 23;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 Graph 번호 바이트의 위치(10.78). NoGraph 표면 등록 때 무효 번호를 쓴다.</summary>
    private const int GraphOffset = 30;

    /// <summary>다리 postPop 접두가 flag 1 에서 켜는 추가 비트(원본 extra bit 2).</summary>
    private const byte BridgeTouchedBit = 2;

    /// <summary>다리 연결 효과를 요청하는 호출 플래그 비트(1 또는 abstract 아님 + 4).</summary>
    private const uint BridgeConnectFlag = 1;

    /// <summary>다리 연결 효과의 두 번째 조건 비트(abstract 비트가 꺼져 있을 때 4).</summary>
    private const uint BridgeAbstractFlag = 4;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 비용·플래그를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>처리 중 조회·갱신하는 상태. 호출자가 소유하며 이 클래스가 직접 바꾼다.</summary>
    private readonly RawPostPopState _state;

    /// <summary>표면 Graph. 연결 전이면 null 이며 영역·표면 등록은 거부된다.</summary>
    private readonly IRawPostPopGraph? _graph;

    /// <summary>공통 후처리 모듈을 만든다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="state">공유 상태. 이 모듈보다 오래 살아야 한다.</param>
    /// <param name="graph">표면 Graph. 같은 풀을 쓰는 몸체만 받는다. null 이면 영역·표면 등록을 거부한다.</param>
    public RawPostPop(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState state, IRawPostPopGraph? graph = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        // CD 10.72(171개)는 소유자·추가 비트 위치와 비용 누적 방식이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (graph is not null && !ReferenceEquals(graph.Pool, pool))
        {
            throw new ArgumentException("postPop/Graph SID 풀 불일치", nameof(graph));
        }
        _pool = pool;
        _types = types;
        _state = state;
        _graph = graph;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. Pop 이 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 다리 postPop(<c>00422150</c>)의 접두 부분. flag 1 은 추가 비트 2 를 켜고,
    /// flag 1 또는 (flag 4 이고 abstract 비트가 꺼져 있음)이면 연결 효과를 요청한다.
    /// 연결 효과는 명시적 외부 경계이며 필요할 때 없으면 추가 비트를 쓰기 전에 거부한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">다리 슬롯 번호.</param>
    /// <param name="flags">Activate 가 넘긴 호출 플래그.</param>
    /// <param name="connector">연결 효과(원본 004213b0). 필요할 때 null 이면 거부한다.</param>
    public static void BridgePrefix(RawSidPool pool, int sid, uint flags, Action<int>? connector)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ReadOnlySpan<byte> slot = pool.Slot(sid);
        byte extra = slot[ExtraOffset];
        bool connect = (flags & BridgeConnectFlag) != 0 || ((flags & BridgeAbstractFlag) != 0 && (extra & 1) == 0);
        // 연결이 필요할 때 효과가 없으면 쓰기 전에 거부한다
        if (connect && connector is null)
        {
            throw new ArgumentException("다리 postPop 연결 효과 누락", nameof(connector));
        }
        // flag 1 은 연결 함수를 부르기 전에 추가 비트를 먼저 기록한다
        if ((flags & BridgeConnectFlag) != 0)
        {
            pool.AllocatedBytes(sid)[ExtraOffset] |= BridgeTouchedBit;
        }
        if (connect)
        {
            connector!(sid);
        }
    }

    /// <summary>
    /// 이 모듈이 처리할 수 있는 파생 postPop 인지 돌려준다.
    /// 파생 몸체(사제·운반체·자산·공장·룬·다이스·다리 등)를 아직 연결하지 않으므로 항상 거짓이다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    public bool HandlesDerived(int sid) => false;

    /// <summary>Pop 과 같은 해시·점유 지도를 보는지 확인한다. Graph 가 없으면 검사할 것이 없어 그대로 둔다.</summary>
    /// <param name="hash">Pop 이 쓰는 공간 해시.</param>
    /// <param name="spots">Pop 이 쓰는 칸별 점유 바이트 65,536개.</param>
    public void ValidateSpace(RawSquidHash hash, byte[] spots)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        _graph?.ValidateSpace(hash, spots);
    }

    /// <summary>
    /// 공간 쓰기 전에 공통·파생 후처리를 처리할 수 있는지 검사한다. 처리할 수 없으면 예외를 던지며 그때까지 풀·공간은 바뀌지 않았다.
    /// 파생 몸체가 없으므로 지원 표 밖의 가상 표는 여기서 거부된다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">정규화한 호출 플래그.</param>
    /// <param name="projection">등록 좌표·단계·타입.</param>
    public void Validate(int sid, uint flags, in PopProjection projection)
    {
        ReadOnlySpan<byte> slot = _pool.Slot(sid);
        // 번호·빈·내용물·자산 타입 밖이면 공통 후처리의 대상이 아니다
        if (sid < RawSidPool.FirstOrdinary || (slot[RawSidPool.StateOffset] & (FreeBit | ContainedBit)) != 0
            || slot[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType || slot[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("postPop raw 자산 상태 오류");
        }
        uint vtable = Read(slot, 0, 4);
        // 공통 지원 표에 없고 파생 몸체도 없으면 미복원 효과다
        if (!OriginalPopDisplay.Supports(vtable, flags) && !HandlesDerived(sid))
        {
            throw new InvalidOperationException("파생 postPop 미복원");
        }
        ValidateBase(sid, flags, in projection);
    }

    /// <summary>
    /// 복원한 파생 몸체가 공통 함수를 직접 부르기 전에 검사한다. 가상 표의 전체 postPop 지원 여부만 제외한다.
    /// Pop/Activate 의 가상 분배는 계속 <see cref="Validate"/> 를 쓴다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    /// <param name="projection">등록 좌표·단계·타입(Graph 연결이 있을 때 넘긴다).</param>
    public void ValidateBase(int sid, uint flags, in PopProjection projection = default)
    {
        ReadOnlySpan<byte> slot = _pool.Slot(sid);
        // 번호·빈·내용물·자산 타입 밖이면 공통 장부의 대상이 아니다
        if (sid < RawSidPool.FirstOrdinary || (slot[RawSidPool.StateOffset] & (FreeBit | ContainedBit)) != 0
            || slot[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType || slot[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("postPop raw 자산 상태 오류");
        }
        // 표시·등록 후처리를 억제하면 타입 효과를 읽지 않는다
        if (_state.Suppressed)
        {
            return;
        }
        OriginalTypeRecord type = _types[slot[RawSidPool.TypeOffset]];
        if (_state.GraphsEnabled)
        {
            (bool region, bool add) = GraphKinds(type, flags);
            // 파괴 경로의 호출 플래그는 원본 assert 로 멈춘다
            if ((flags & GraphChangeMask) != 0 && (flags & NoGraphBit) == 0)
            {
                if ((flags & DestroyGraphBit) != 0)
                {
                    throw new InvalidOperationException("postPop destroyGraph 원본 assert 경로");
                }
            }
            // 영역·표면 등록인데 Graph 가 없으면 거부한다
            if ((region || add) && _graph is null)
            {
                throw new InvalidOperationException("postPop 표면/영역 그래프가 연결되지 않았습니다");
            }
            _graph?.ValidatePostPop(sid, region, add, in projection);
        }
        // 최초 등록(flags 1)이 아니거나 abstract·매립이면 비용·통계를 건드리지 않는다
        if ((flags & 1) == 0 || (slot[ExtraOffset] & AbstractBuriedMask) != 0)
        {
            return;
        }
        byte owner = slot[OwnerOffset];
        // 소유자는 0~8 이어야 한다
        if (owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), owner, "postPop 소유자 범위 오류");
        }
        // AI 가 붙은 소유자의 최초 등록 통지는 아직 복원하지 않았다
        if (owner != 0 && _state.AiAttached[owner])
        {
            throw new InvalidOperationException("postPop AI 생성 통지 미복원");
        }
        // 보류 배치가 있으면 선택 해제의 미복원 효과다
        if (_state.PendingPlacement)
        {
            throw new InvalidOperationException("postPop 배치 선택 해제 미복원");
        }
        // 비용 집계가 가능한지(유한·표 범위·누적 범위) 미리 확인한다
        _ = TotalCost(slot[RawSidPool.TypeOffset]);
        // 공급원·작업장 목록이 손상됐는지도 쓰기 전에 확인한다
        if ((type.Flags1 & ProviderBit) != 0)
        {
            _ = _state.Providers.Items().Length;
        }
        if ((type.Flags2 & FactoryMask) != 0)
        {
            _ = _state.Factories.Items().Length;
            if (owner != 0)
            {
                _ = _state.OwnerFactories[owner].Items().Length;
            }
        }
    }

    /// <summary>
    /// 비전투 Activate 에서 postPop 을 호출하는 깊이 증가·감소를 보존한다. 파생 몸체가 없으므로 바로 공통 몸체로 간다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">Pop 이 정규화한 호출 플래그.</param>
    public void Activate(int sid, uint flags)
    {
        Validate(sid, flags, default);
        uint before = _state.Depth;
        _state.Depth = unchecked(before + 1);
        PostPop(sid, flags);
        // 파생 몸체가 없으므로 공통 몸체가 깊이를 정확히 1 줄였는지 여기서 보장된다
        if (_state.Depth != before)
        {
            throw new InvalidOperationException("postPop Activate 공통 깊이 불일치");
        }
    }

    /// <summary>공통 몸체. 명시적인 직접 호출은 깊이를 한 단계 감소시킨다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void PostPop(int sid, uint flags)
    {
        Validate(sid, flags, default);
        PostPopBase(sid, flags, default);
    }

    /// <summary>
    /// 공통 후처리의 순서: Graph 생성 → 비용 → 공급·작업장 목록 → 소유자 조건 통계 → 깊이 감소다.
    /// 직접 base 호출도 공통 상태·효과의 검사와 깊이 감소를 그대로 수행한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    /// <param name="projection">등록 좌표·단계·타입(Graph 연결이 있을 때 넘긴다).</param>
    public void PostPopBase(int sid, uint flags, in PopProjection projection = default)
    {
        ValidateBase(sid, flags, in projection);
        // 억제 경로도 깊이는 1 줄인다
        if (_state.Suppressed)
        {
            _state.Depth = unchecked(_state.Depth - 1);
            return;
        }
        ReadOnlySpan<byte> slot = _pool.Slot(sid);
        int number = slot[RawSidPool.TypeOffset];
        OriginalTypeRecord type = _types[number];
        if (_state.GraphsEnabled && _graph is not null)
        {
            (bool region, bool add) = GraphKinds(type, flags);
            _graph.PostPop(sid, region, add);
        }
        // 표면 타입이 NoGraph 로 등록되면 Graph 바이트에 무효 번호를 쓴다
        if (_state.GraphsEnabled && (type.Flags1 & OriginalTypeRecord.Surface) != 0 && (flags & 1) != 0 && (flags & NoGraphBit) != 0)
        {
            _pool.RawSlot(sid)[GraphOffset] = _state.InvalidGraph;
        }
        // 최초 등록이고 abstract·매립이 아니면 비용·목록·통계를 갱신한다
        if ((flags & 1) != 0 && (slot[ExtraOffset] & AbstractBuriedMask) == 0)
        {
            _state.TotalCost = TotalCost(number);
            // 공급원이면 목록에 없을 때만 덧붙이고 재계산 알림은 항상 1 늘린다
            if ((type.Flags1 & ProviderBit) != 0)
            {
                if (!_state.Providers.Contains(sid))
                {
                    _state.Providers.Append(sid);
                }
                _state.ProductionDirty = unchecked(_state.ProductionDirty + 1);
            }
            byte owner = slot[OwnerOffset];
            // 작업장이면 소유자 목록과 전체 목록에 덧붙이고 알림을 1 늘린다
            if ((type.Flags2 & FactoryMask) != 0)
            {
                if (owner != 0)
                {
                    _state.OwnerFactories[owner].Append(sid);
                }
                _state.Factories.Append(sid);
                _state.ProductionDirty = unchecked(_state.ProductionDirty + 1);
            }
            // 로컬 소유자의 등록이면 두 로컬 표를 함께 늘린다(0도 로컬 번호 0과 비교한다)
            if (owner == _state.LocalOwner)
            {
                _state.LocalCounts[number] = unchecked(_state.LocalCounts[number] + 1);
                _state.LocalSecondaryCounts[number] = unchecked(_state.LocalSecondaryCounts[number] + 1);
            }
            _state.GlobalCounts[number] = unchecked(_state.GlobalCounts[number] + 1);
        }
        _state.Depth = unchecked(_state.Depth - 1);
    }

    /// <summary>호출 플래그와 타입 플래그에서 영역·표면 등록 여부를 계산한다.</summary>
    /// <param name="type">타입 레코드.</param>
    /// <param name="flags">호출 플래그.</param>
    private static (bool Region, bool Add) GraphKinds(OriginalTypeRecord type, uint flags)
    {
        bool region = (type.Flags2 & RegionMask) != 0 && (flags & 3) != 0 && (flags & NoGraphBit) == 0;
        bool add = (type.Flags1 & OriginalTypeRecord.Surface) != 0 && (flags & GraphChangeMask) != 0 && (flags & NoGraphBit) == 0;
        return (region, add);
    }

    /// <summary>
    /// 패치판 비용 집계(원본 trunc 누적). 비용 표의 인코딩 편향(타입×23)을 걷어낸 정수에 전역 집계를 더한다.
    /// 정상 누적의 signed DWORD 경계 통과는 low DWORD 로 보존한다.
    /// </summary>
    /// <param name="typeIndex">타입 번호.</param>
    private int TotalCost(int typeIndex)
    {
        double cost = _types[typeIndex].Cost;
        // 유한하지 않은 비용은 통계·깊이·슬롯에 쓰기 전에 거부한다
        if (!double.IsFinite(cost))
        {
            throw new InvalidOperationException("postPop 유한 비용이 필요합니다");
        }
        long bias = (long)typeIndex * CostBias;
        double encoded = cost + bias;
        // 패치판 비용 표의 int32 범위를 벗어나면 거부한다
        if (encoded < int.MinValue || encoded > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(typeIndex), "postPop 패치 비용 표 범위 오류");
        }
        // 0 쪽 절삭 뒤 편향을 빼고 32비트 low DWORD 로 읽는다
        int unit = unchecked((int)(uint)((long)encoded - bias));
        double total = (double)unit + _state.TotalCost;
        // 누적 범위를 벗어나면 원본 ftol 의 범위를 벗어난 것이므로 거부한다
        if (total < -4294967296.0 || total > 4294967295.0)
        {
            throw new ArgumentOutOfRangeException(nameof(typeIndex), "postPop 비용 누적 범위 오류");
        }
        return unchecked((int)(uint)(long)total);
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 필요한 폭만큼 읽는다(이웃 필드를 읽지 않는다).</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수(1~4).</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
