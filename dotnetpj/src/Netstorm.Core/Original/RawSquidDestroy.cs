namespace Netstorm.Core.Original;

/// <summary>
/// 삭제 과정에서 바깥으로 나가는 효과의 종류(원본 SquidDestroyEffect).
/// PreDestroy·PostDestroy 는 바깥(수명주기)이 완료 호출로 닫아야 하고, 그 사이는 깊이 카운터가 1 늘어난 채로 둔다.
/// </summary>
public enum SquidDestroyEffect
{
    /// <summary>삭제 시작 알림. 바깥은 <see cref="RawSquidDestroy.CompletePreDestroy"/> 를 정확히 한 번 불러야 한다.</summary>
    PreDestroy,

    /// <summary>종속(내용물) 연결 끊기 알림. 바깥은 그 종속의 다음 번호를 0 으로 지운다.</summary>
    ReleaseDependent,

    /// <summary>종속 파괴 알림. 바깥은 그 종속에 죽음 비트를 세운다(재귀 삭제는 수명주기가 한다).</summary>
    DestroyDependent,

    /// <summary>서버 권한 전송 알림. 서버 풀의 서버 쪽 삭제를 알릴 때만 나간다.</summary>
    Transmit,

    /// <summary>선택 해제 알림. 고른 번호가 지워지는 번호와 같을 때만 나간다.</summary>
    ClearSelection,

    /// <summary>삭제 끝 알림. 바깥은 <see cref="RawSquidDestroy.CompletePostDestroy"/> 를 정확히 한 번 불러야 한다.</summary>
    PostDestroy,
}

/// <summary>
/// 삭제 효과 한 건의 인자(원본 SquidDestroyEvent).
/// 번호는 풀 안의 슬롯 번호이며, 부모는 종속 효과에서만 쓰는 주인 번호다.
/// </summary>
/// <param name="Effect">바깥으로 나가는 효과의 종류.</param>
/// <param name="Sid">효과 대상의 슬롯 번호.</param>
/// <param name="Parent">종속의 주인 번호(종속이 아니면 0).</param>
/// <param name="Flags">삭제에 적용되는 플래그 비트.</param>
public readonly record struct SquidDestroyEvent(SquidDestroyEffect Effect, int Sid, int Parent, uint Flags);

/// <summary>
/// 삭제 본체가 바깥에 요구하는 콜백 묶음(원본 SquidDestroyHooks).
/// Emit·Selected 는 반드시 있어야 하며, 자산 밖 Form 뿌리(타입 70 미만)는 UnpopForm 도 있어야 한다.
/// </summary>
public sealed class SquidDestroyHooks
{
    /// <summary>삭제 효과를 바깥 처리기에 알리는 콜백. 호출 안에서 완료·다음 지우기·죽음 표시를 한다.</summary>
    public Action<SquidDestroyEvent>? Emit { get; set; }

    /// <summary>현재 고른 번호를 조회하는 콜백. 지워지는 번호와 같을 때만 선택 해제를 알린다.</summary>
    public Func<int>? Selected { get; set; }

    /// <summary>자산 밖 Form 뿌리(타입 70 미만)의 공간 해제(vtable +0x48). 자산 뿌리는 쓰지 않는다.</summary>
    public Action<int>? UnpopForm { get; set; }
}

/// <summary>
/// 공통 삭제 본체(원본 RawSquidDestroy 004af780, 10.78).
/// 죽음 표시→종속 끊기→Unpop→반납 순서이며, 이미 죽은 번호는 바깥 효과 없이 그대로 돌아온다.
/// Pre·Post 효과는 바깥이 완료 호출로 닫아야 하며, 닫지 않으면 예외로 멈추고 마무리를 건너뛴다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 주소·분기 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawSquidDestroy.cpp (W03 삭제·수명 첫 조각, 2026-10-21).
/// </summary>
public sealed class RawSquidDestroy
{
    /// <summary>슬롯 안 다음 번호(16비트)의 위치. 종속 체인의 머리·다음을 읽는다.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치. 뿌리의 첫 종속 번호가 있다.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 죽음 비트(삭제 준비).</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 공간 없음(void) 비트. Unpop 뒤에 켜진다.</summary>
    private const byte VoidBit = 4;

    /// <summary>상태 바이트의 들어 있는 내용물(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>뿌리가 될 수 있는 가장 작은 번호. 0~4 번은 일반 할당 대상이 아니다.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>플래그의 서버 권한 비트. 서버가 아닌 풀에서 서버 쪽 뿌리를 지울 때 필요하다.</summary>
    private const uint ServerFlag = 8;

    /// <summary>플래그의 클라이언트 비트. 서버 풀에서 클라이언트 쪽 뿌리를 지울 때 자동으로 붙는다.</summary>
    private const uint ClientFlag = 0x10;

    /// <summary>플래그의 종속 비트. 종속을 지우는 호출에 붙어 전송·선택 효과를 건너뛴다.</summary>
    private const uint DependentFlag = 0x40;

    /// <summary>삭제 대상 슬롯을 읽고 쓰고 반납하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>자산 뿌리의 공간 해제를 맡는 Unpop 처리기.</summary>
    private readonly RawUnpop _unpop;

    /// <summary>타입 번호 → 타입 레코드. 자산 여부·개수를 확인한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>열린 PreDestroy 효과의 깊이. 완료 호출이 하나씩 닫는다.</summary>
    private uint _preDepth;

    /// <summary>열린 PostDestroy 효과의 깊이. 완료 호출이 하나씩 닫는다.</summary>
    private uint _postDepth;

    /// <summary>삭제 본체를 만든다. 풀·Unpop·타입 표를 묶으며, 서로 다른 풀의 Unpop 은 거부한다.</summary>
    /// <param name="pool">삭제 대상 슬롯 풀.</param>
    /// <param name="unpop">자산 뿌리의 공간 해제기. 같은 풀을 써야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개다.</param>
    public RawSquidDestroy(RawSidPool pool, RawUnpop unpop, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(unpop);
        ArgumentNullException.ThrowIfNull(types);
        if (!ReferenceEquals(unpop.Pool, pool))
        {
            throw new ArgumentException("삭제 본체와 Unpop 의 슬롯 풀이 다릅니다", nameof(unpop));
        }
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _unpop = unpop;
        _types = types;
    }

    /// <summary>이 본체가 쓰는 슬롯 풀. 장면·수명주기가 같은 풀인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>열린 PreDestroy 효과의 깊이. 완료 호출이 닫기 전에는 다음 삭제가 같은 값을 보면 안 된다.</summary>
    public uint PreDepth => _preDepth;

    /// <summary>열린 PostDestroy 효과의 깊이. 완료 호출이 닫기 전에는 다음 삭제가 같은 값을 보면 안 된다.</summary>
    public uint PostDepth => _postDepth;

    /// <summary>PreDestroy 효과를 닫는다. 바깥이 Pre 안에서 정확히 한 번 불러야 한다.</summary>
    public void CompletePreDestroy()
    {
        // 감싼 카운터라 0 에서 부르면 최대값으로 감긴다(원본 DWORD 와 같다)
        _preDepth--;
    }

    /// <summary>PostDestroy 효과를 닫는다. 바깥이 Post 안에서 정확히 한 번 불러야 한다.</summary>
    public void CompletePostDestroy()
    {
        // 감싼 카운터라 0 에서 부르면 최대값으로 감긴다(원본 DWORD 와 같다)
        _postDepth--;
    }

    /// <summary>삭제 본체가 쓰는 해시·점유 지도가 그래프와 같은 풀을 보는지 확인한다.</summary>
    /// <param name="graph">같은 풀을 써야 하는 그래프.</param>
    public void ValidateGraph(RawGraph graph)
    {
        ArgumentNullException.ThrowIfNull(graph);
        if (!ReferenceEquals(graph.Pool, _pool))
        {
            throw new ArgumentException("삭제 본체와 그래프의 슬롯 풀이 다릅니다", nameof(graph));
        }
        // 원본 SquidUnpop::ValidateGraph 처럼 같은 공간 해시·같은 점유 지도인지도 확인한다
        // (W03 자산 삭제 분배 조각, 2026-10-23: 가상 삭제가 다른 공간의 Graph 를 읽지 않게 한다)
        graph.ValidateSpace(_unpop.Hash, _unpop.Spots);
    }

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안의 바이트 위치.</param>
    private ushort Word(int sid, int offset)
    {
        // 호스트 바이트 순서에 기대지 않고 낮은 바이트부터 합친다
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>
    /// 뿌리 삭제를 수행한다(원본 004af780).
    /// 죽음 표시→Pre 알림→종속 끊기·죽음 알림→전송·선택 알림→Unpop→Post 알림→반납 순서이며,
    /// 이미 죽은 번호는 바깥 효과 없이 그대로 돌아온다. Form 뿌리는 UnpopForm 으로, 자산 뿌리는 Unpop 으로 푼다.
    /// Pre·Post 안에서 완료 호출을 빼먹으면 예외로 멈추고 뒷정리를 건너뛴다.
    /// </summary>
    /// <param name="sid">지울 뿌리의 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    /// <param name="hooks">바깥 효과 콜백 묶음.</param>
    public void Destroy(int sid, uint flags, SquidDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (sid < FirstOrdinary || sid == RawSidPool.PredictableFirst || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "삭제 번호가 풀 범위를 벗어났습니다");
        }
        ReadOnlySpan<byte> head = _pool.Slot(sid);
        if ((head[StateOffset] & DeadBit) != 0)
        {
            return;
        }
        bool form = head[TypeOffset] < OriginalConstructors.FirstAssetType;
        if ((head[StateOffset] & FreeBit) != 0 || head[TypeOffset] >= _types.Count || (form ? hooks.UnpopForm is null : (head[StateOffset] & ContainedBit) != 0))
        {
            throw new InvalidOperationException("삭제 뿌리 상태 오류(빈·타입·Form·내용물)");
        }
        if (hooks.Emit is null || hooks.Selected is null)
        {
            throw new ArgumentException("삭제 효과 콜백이 연결되지 않았습니다", nameof(hooks));
        }
        bool client = sid < RawSidPool.ServerFirst;
        if (!_pool.IsServer && !client && (flags & ServerFlag) == 0)
        {
            throw new InvalidOperationException("서버 권한 없는 풀에서 서버 쪽 삭제를 거부합니다");
        }
        if (_pool.IsServer && client)
        {
            flags |= ClientFlag;
        }
        _pool.AllocatedBytes(sid)[StateOffset] |= DeadBit;
        uint pre = _preDepth++;
        hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 0, flags));
        if (_preDepth != pre)
        {
            throw new InvalidOperationException("PreDestroy 완료 호출이 빠졌습니다");
        }
        int dependent = Word(sid, HeadOffset);
        var visited = new HashSet<int>();
        // 종속 체인을 머리부터 따라가며 끊기·죽음 알림을 순서대로 낸다
        while (dependent != 0)
        {
            if (dependent >= _pool.Capacity || !visited.Add(dependent))
            {
                throw new InvalidOperationException("삭제 종속 체인 오류(범위·순환)");
            }
            ReadOnlySpan<byte> child = _pool.Slot(dependent);
            if ((child[StateOffset] & FreeBit) != 0 || ((child[StateOffset] & ContainedBit) == 0 && child[TypeOffset] >= OriginalConstructors.FirstAssetType))
            {
                throw new InvalidOperationException("삭제 종속 상태 오류(빈·내용물·타입)");
            }
            int next = Word(dependent, NextOffset);
            hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.ReleaseDependent, dependent, sid, 0));
            if ((_pool.Slot(dependent)[StateOffset] & FreeBit) == 0)
            {
                hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.DestroyDependent, dependent, sid, flags | DependentFlag));
            }
            dependent = next;
        }
        if ((_pool.Slot(sid)[StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("Pre 처리 중에 뿌리가 반납됐습니다");
        }
        if ((flags & (ClientFlag | DependentFlag)) == 0 && _pool.IsServer && !client)
        {
            hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.Transmit, sid, 0, flags | ServerFlag));
        }
        if (hooks.Selected() == sid)
        {
            hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.ClearSelection, sid, 0, 0));
        }
        ReadOnlySpan<byte> root = _pool.Slot(sid);
        if ((root[StateOffset] & VoidBit) == 0)
        {
            if (root[TypeOffset] >= _types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), root[TypeOffset], "삭제 대상 타입 번호 오류");
            }
            if (root[TypeOffset] < OriginalConstructors.FirstAssetType)
            {
                // Form 뿌리는 바깥의 Form 해제기로 void 까지 푼 뒤 주인 체인에서 뺀다
                if (hooks.UnpopForm is null)
                {
                    throw new InvalidOperationException("Form 뿌리의 해제 콜백이 없습니다");
                }
                hooks.UnpopForm(sid);
                if ((_pool.Slot(sid)[StateOffset] & VoidBit) == 0)
                {
                    throw new InvalidOperationException("Form 해제 뒤에도 void 가 아닙니다");
                }
            }
            else
            {
                _unpop.Unpop(sid, _types[root[TypeOffset]], 0);
            }
        }
        uint post = _postDepth++;
        hooks.Emit(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 0, flags));
        if (_postDepth != post)
        {
            throw new InvalidOperationException("PostDestroy 완료 호출이 빠졌습니다");
        }
        _pool.Release(sid);
    }
}
