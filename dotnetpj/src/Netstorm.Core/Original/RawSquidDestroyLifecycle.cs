using System.Buffers.Binary;

// 공통 삭제의 현재 장부와 외부 효과를 연결하는 네임스페이스다.
namespace Netstorm.Core.Original;

/// <summary>
/// 공통 삭제 수명주기에서 바깥으로 나가는 효과의 종류(원본 SquidDeletionEffect).
/// ClearSelection 은 선택 해제, Refund 는 삭제 보상, UnitLostSound 는 잃어버린 오브젝트의 소리다.
/// </summary>
public enum SquidDeletionEffect
{
    /// <summary>선택 해제 알림. 고른 번호가 지워지는 번호와 같을 때만 나간다.</summary>
    ClearSelection,

    /// <summary>삭제 보상 알림. 수신자는 호출 플래그의 16~19비트(0~15)다.</summary>
    Refund,

    /// <summary>잃어버린 오브젝트 소리 알림. 좌표는 상태에 따로 남는다.</summary>
    UnitLostSound,
}

/// <summary>
/// 삭제 수명주기 효과 한 건의 인자(원본 SquidDeletionEvent).
/// </summary>
/// <param name="Effect">바깥으로 나가는 효과의 종류.</param>
/// <param name="Sid">효과 대상의 슬롯 번호.</param>
/// <param name="Recipient">보상 수신자 번호(Refund 만 쓰고 나머지는 0).</param>
public readonly record struct SquidDeletionEvent(SquidDeletionEffect Effect, int Sid, uint Recipient);

/// <summary>
/// 삭제 수명주기가 바깥에 요구하는 콜백 묶음(원본 SquidDeletionHooks).
/// Emit 은 선택 해제·보상·소리 효과를 알리고, Forward 는 공통 몸체가 다루지 않는 종속·전파 효과를 넘긴다.
/// </summary>
public sealed class SquidDeletionHooks
{
    /// <summary>선택 해제·보상·소리 효과를 바깥 처리기에 알리는 콜백. 필요한 효과에 없으면 쓰기 전에 거부한다.</summary>
    public Action<SquidDeletionEvent>? Emit { get; set; }

    /// <summary>종속 끊기·종속 파괴·전송 같은 파생 효과를 바깥 처리기로 넘기는 콜백.</summary>
    public Action<SquidDestroyEvent>? Forward { get; set; }

    /// <summary>Native 선택 비교 위치에서 현재 Client의 전체 DWORD를 읽는다.</summary>
    public Func<uint>? SelectedRaw { get; set; }

    /// <summary>선택 해제 전에 실제 UI에 X/Y 원시 DWORD와 현재 TYPE를 기억시킨다.</summary>
    public Action<uint, uint, uint>? RememberPlacement { get; set; }

    /// <summary>Native의 각 소유자 비교 위치에서 현재 로컬 소유자 DWORD를 읽는다.</summary>
    public Func<uint>? LocalOwner { get; set; }
}

/// <summary>
/// 삭제 처리 중 조회·갱신하는 상태 묶음(원본 SquidDeletionState).
/// 선택 번호·배치/잃어버린 좌표·소리 억제 조건·Graph 분할 정책 전역을 담는다.
/// </summary>
public sealed class SquidDeletionState
{
    /// <summary>현재 고른 번호. 지워지는 번호와 같으면 선택 해제를 알린다(0 이면 없음).</summary>
    public int Selected { get; set; }

    /// <summary>abstract·매립 선택 객체가 남기는 배치 타입 번호. 0 이면 없음.</summary>
    public uint PlacementType { get; set; }

    /// <summary>배치 좌표 x. 원본 바이트 순서의 비트를 그대로 보존한다.</summary>
    public float PlacementX { get; set; }

    /// <summary>배치 좌표 y.</summary>
    public float PlacementY { get; set; }

    /// <summary>잃어버린 오브젝트 좌표 x.</summary>
    public float LostX { get; set; }

    /// <summary>잃어버린 오브젝트 좌표 y.</summary>
    public float LostY { get; set; }

    /// <summary>원본 전역 1 의 소리·좌표 기록 억제 조건.</summary>
    public bool UnitLostSuppressed1 { get; set; }

    /// <summary>원본 전역 2 의 소리·좌표 기록 억제 조건.</summary>
    public bool UnitLostSuppressed2 { get; set; }

    /// <summary>패치판에서 소리를 내지 않는 타입 번호. 좌표 기록은 이 조건과 따로다.</summary>
    public uint SilentType { get; set; }

    /// <summary>사후 Graph 추가의 대상 타입 마스크(원본 0x50444200). 실제 분기는 이 값에 8 을 OR 한다.</summary>
    public uint RegionMask { get; set; } = 0x50444200;

    /// <summary>원본 삭제 분할 정책 전역. GraphRecovery 의 소진 복구와 구별한다.</summary>
    public bool RebuildGraph { get; set; }

    /// <summary>특수 표면 타입 번호(원본 157). 이 타입은 방향 코드가 H 일 때만 치환된다.</summary>
    public byte SpecialSurfaceType { get; set; } = 157;

    /// <summary>특수 표면의 치환 타입 번호(원본 162).</summary>
    public byte SpecialSurfaceReplacementType { get; set; } = 162;

    /// <summary>대형 표면 타입 번호(원본 162). 이 타입은 표면 9 개를 함께 뗀다.</summary>
    public byte LargeSurfaceType { get; set; } = 162;

    /// <summary>같은 실제 손실 X 저장소다. 독립 LostX 필드는 보존한다.</summary>
    public FloatBox? SharedLostX { get; set; }
    /// <summary>같은 실제 손실 Y 저장소다. X와 별도로 빌린다.</summary>
    public FloatBox? SharedLostY { get; set; }
    /// <summary>현재 로딩 깊이 DWORD다. 비영이면 소리와 좌표 기록을 억제한다.</summary>
    public UIntBox? SharedLoadingDepth { get; set; }
    /// <summary>현재 fort 모드 bool이다.</summary>
    public BoolBox? SharedFortMode { get; set; }
    /// <summary>현재 소리 제외 TYPE DWORD다.</summary>
    public UIntBox? SharedSilentType { get; set; }
    /// <summary>현재 H 표면 치환 대상 TYPE DWORD다.</summary>
    public UIntBox? SharedSpecialSurfaceType { get; set; }
    /// <summary>현재 H 표면 대체 TYPE DWORD다. BYTE 변환은 실제 치환 때만 검사한다.</summary>
    public UIntBox? SharedReplacementSurfaceType { get; set; }
    /// <summary>현재 9칸 감소 대상 TYPE DWORD다.</summary>
    public UIntBox? SharedLargeSurfaceType { get; set; }
    /// <summary>실제 저장소를 빌렸으면 같은 X만 읽고 쓴다.</summary>
    public float CurrentLostX { get => SharedLostX?.Value ?? LostX; set { if (SharedLostX is { } box) box.Value = value; else LostX = value; } }
    /// <summary>실제 저장소를 빌렸으면 같은 Y만 읽고 쓴다.</summary>
    public float CurrentLostY { get => SharedLostY?.Value ?? LostY; set { if (SharedLostY is { } box) box.Value = value; else LostY = value; } }
    /// <summary>현재 로딩 깊이 또는 독립 억제 조건이다.</summary>
    public bool LoadingSuppressed => SharedLoadingDepth is { } depth ? depth.Value != 0 : UnitLostSuppressed1;
    /// <summary>현재 fort 모드 또는 독립 억제 조건이다.</summary>
    public bool FortSuppressed => SharedFortMode?.Value ?? UnitLostSuppressed2;
    /// <summary>현재 소리 제외 TYPE 또는 독립 입력이다.</summary>
    public uint CurrentSilentType => SharedSilentType?.Value ?? SilentType;
    /// <summary>현재 H 치환 대상 또는 독립 입력이다.</summary>
    public uint CurrentSpecialSurfaceType => SharedSpecialSurfaceType?.Value ?? SpecialSurfaceType;
    /// <summary>현재 대체 TYPE 또는 독립 입력이다.</summary>
    public uint CurrentReplacementSurfaceType => SharedReplacementSurfaceType?.Value ?? SpecialSurfaceReplacementType;
    /// <summary>현재 9칸 감소 TYPE 또는 독립 입력이다.</summary>
    public uint CurrentLargeSurfaceType => SharedLargeSurfaceType?.Value ?? LargeSurfaceType;
}

/// <summary>
/// 공통 pre/postDestroy 의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부를 복원한다(원본 SquidDestroyLifecycle, 10.78).
/// 순서는 원본 그대로 "선택 해제 → 그래프 분할 → 소유자 작업장 → 보상 → 현재/누적 통계 → 소리·좌표 →
/// 공급원 → 작업장 → 깊이 감소(사전)"와 "주변 표면 추가 → ordinary 비용 차감 → 깊이 감소(사후)"다.
/// 독립 경로는 선검사를 유지한다. Native PATCH는 도달한 현재 필드와 효과만 원문 순서로 소비한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 소유자·추가 비트 위치와 비용 인코딩이 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/SquidDestroyLifecycle.cpp (W03 삭제·수명 셋째 조각, 2026-10-21).
/// </summary>
public sealed class RawSquidDestroyLifecycle
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 그래프 번호 바이트의 위치(10.78).</summary>
    private const int GraphOffset = 30;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78). abstract(1)·매립(8) 비트가 있다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>추가 비트의 abstract·매립 마스크.</summary>
    private const byte AbstractBuriedMask = 9;

    /// <summary>추가 비트의 매립(buried) 비트.</summary>
    private const byte BuriedBit = 8;

    /// <summary>상태 바이트의 들어 있는 내용물(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>플래그2의 작업장(공급원·작업장 목록) 마스크(원본 0x4200).</summary>
    private const uint FactoryMask = 0x4200;

    /// <summary>플래그1의 공급원 비트(원본 0x10000000).</summary>
    private const uint ProviderBit = 0x10000000;

    /// <summary>플래그2의 배치 타입 마스크(원본 0x30000).</summary>
    private const uint PlacementMask = 0x30000;

    /// <summary>플래그의 보상 금지 비트(원본 0x200000).</summary>
    private const uint NoRefundBit = 0x200000;

    /// <summary>플래그의 그래프 분할 금지 비트(원본 0x2000000).</summary>
    private const uint NoGraphBit = 0x2000000;

    /// <summary>플래그의 그래프 파괴 비트(원본 0x800).</summary>
    private const uint DestroyGraphBit = 0x800;

    /// <summary>플래그2의 종류 소리 제외 비트(원본 0x200000).</summary>
    private const uint SilentGenusBit = 0x200000;

    /// <summary>타입 번호마다 더하는 비용 표 인코딩 계수(원본 23).</summary>
    private const long CostBias = 23;

    /// <summary>소유자 번호의 최댓값(원본 kPlayerCount). 0~8 까지 쓴다.</summary>
    private const int MaximumOwner = 8;

    /// <summary>타입의 "그룹 없음" 번호(원본 kTypeNoGroup). 이 그룹은 잃어버린 소리를 내지 않는다.</summary>
    private const int TypeGroupNone = 10;

    /// <summary>삭제 대상 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 생성 시 복사해 장부 처리 중의 입력을 고정한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>공유 장부. Pop 과 같은 수명으로 쓴다.</summary>
    private readonly RawPostPopState _bookkeeping;

    /// <summary>처리 중 조회·갱신하는 상태.</summary>
    private readonly SquidDeletionState _state;

    /// <summary>삭제 본체. 완료 호출(깊이 감소)을 이 수명주기가 맡는다.</summary>
    private readonly RawSquidDestroy _destroy;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly SquidDeletionHooks _hooks;

    /// <summary>표면 Graph. 연결 전이면 null 이며 그래프 효과가 필요한 삭제에는 거부한다.</summary>
    private readonly RawGraph? _graph;

    /// <summary>같은 풀과 같은 장부의 실제 삭제 보상 몸체다. 없으면 외부 효과를 사용한다.</summary>
    private readonly RawSquidReward? _reward;

    /// <summary>공급원 제거 뒤 현재 소유자의 실제 AI 삭제 통지를 전달한다.</summary>
    private Action<uint, int, bool>? _aiNotice;

    /// <summary>
    /// 삭제 수명주기를 만든다. 타입 표를 복사해 장부 처리 중의 입력을 고정하고, 풀·그래프 연결이 어긋나면 거부한다.
    /// </summary>
    /// <param name="pool">삭제 대상 슬롯 풀. Pop 과 같은 풀이어야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="bookkeeping">공유 장부. 이 수명주기보다 오래 살아야 한다.</param>
    /// <param name="state">삭제 상태.</param>
    /// <param name="destroy">삭제 본체. 같은 풀을 써야 한다.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    /// <param name="graph">표면 Graph. 같은 풀을 쓰는 몸체만 받는다.</param>
    /// <param name="reward">같은 풀·장부를 쓰는 실제 보상 몸체다.</param>
    public RawSquidDestroyLifecycle(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState bookkeeping,
        SquidDeletionState state, RawSquidDestroy destroy, SquidDeletionHooks hooks, RawGraph? graph = null, RawSquidReward? reward = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(hooks);
        // CD 10.72(171개)는 소유자·추가 비트 위치와 비용 인코딩이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (!ReferenceEquals(destroy.Pool, pool))
        {
            throw new ArgumentException("삭제 수명주기와 삭제 본체의 슬롯 풀이 다릅니다", nameof(destroy));
        }
        if (graph is not null)
        {
            if (!ReferenceEquals(graph.Pool, pool))
            {
                throw new ArgumentException("삭제 수명주기와 그래프의 슬롯 풀이 다릅니다", nameof(graph));
            }
            graph.ValidateTypes(types);
            destroy.ValidateGraph(graph);
        }
        if (reward is not null && (!ReferenceEquals(reward.Pool, pool) || !ReferenceEquals(reward.Bookkeeping, bookkeeping)))
            throw new ArgumentException("삭제 수명주기와 보상의 풀 또는 장부가 다릅니다", nameof(reward));
        _reward = reward;
        _pool = pool;
        // 타입 표를 값으로 복사한다. 장부 처리 중에 호출자가 값을 바꿔도 판정 기준이 흔들리지 않게 한다.
        OriginalTypeRecord[] copy = new OriginalTypeRecord[types.Count];
        // 타입 번호 순서대로 한 항목씩 복사한다
        for (int i = 0; i < types.Count; i++)
        {
            copy[i] = types[i].Clone();
        }
        _types = copy;
        _bookkeeping = bookkeeping;
        _state = state;
        _destroy = destroy;
        _hooks = hooks;
        _graph = graph;
    }

    /// <summary>이 수명주기가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>이 수명주기가 참조하는 그래프. 연결 전이면 null 이다.</summary>
    public RawGraph? Graph => _graph;

    /// <summary>
    /// 루트 자산의 슬롯을 읽는다. 자산이 아니거나 빈·내용물 상태면 예외다.
    /// </summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private ReadOnlySpan<byte> Object(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (_destroy.HasPatchDiagnostics) return raw;
        // 일반 번호·예측 머리 밖이고, 빈·내용물 상태가 아니며, 자산 타입이어야 한다
        if (sid < RawSidPool.FirstOrdinary || sid == RawSidPool.PredictableFirst
            || (raw[StateOffset] & (RawSidPool.FreeBit | ContainedBit)) != 0
            || raw[TypeOffset] < OriginalConstructors.FirstAssetType || raw[TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("삭제 수명주기 raw 자산 상태 오류");
        }
        return raw;
    }

    /// <summary>앞선 외부 콜백이 타입을 바꿨을 때도 캐시하지 않고 다시 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private OriginalTypeRecord Type(int sid)
    {
        int number = Object(sid)[TypeOffset];
        IReadOnlyList<OriginalTypeRecord> types = _destroy.HasPatchDiagnostics ? _destroy.Types : _types;
        if (number >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "삭제의 현재 TYPE 물리 범위");
        return types[number];
    }

    /// <summary>Native에서만 실제 현재 로컬 소유자를 읽고 독립 BYTE 사본은 유지한다.</summary>
    private uint LocalOwner() => _destroy.HasPatchDiagnostics && _hooks.LocalOwner is { } owner ? owner() : _bookkeeping.LocalOwner;

    /// <summary>슬롯의 소유자 번호(10.78 의 34 번 바이트).</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private byte Owner(int sid) => Object(sid)[OwnerOffset];

    /// <summary>abstract 또는 매립이면 글로벌 장부·비용 경로를 생략한다. 소유자 목록 제거와 구별한다.</summary>
    /// <param name="sid">확인할 슬롯 번호.</param>
    private bool Ordinary(int sid) => (Object(sid)[ExtraOffset] & AbstractBuriedMask) == 0;

    /// <summary>원본 unitLost 조건. 패치판의 silentType 소리 제외는 좌표 기록과 따로 적용한다.</summary>
    /// <param name="sid">확인할 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    private bool LostNotice(int sid, uint flags)
    {
        return Type(sid).Group != TypeGroupNone && (flags & (NoRefundBit | DestroyGraphBit)) == 0
            && Owner(sid) == LocalOwner() && (Type(sid).Flags2 & SilentGenusBit) == 0
            && !_state.LoadingSuppressed && !_state.FortSuppressed;
    }

    /// <summary>사후 비용 차감값. x87 뺄셈·0 방향 절삭과 DWORD 감김을 원본처럼 보존한다.</summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    private int RemainingCost(int sid)
    {
        int number = Object(sid)[TypeOffset];
        IReadOnlyList<OriginalTypeRecord> costs = _destroy.HasPatchDiagnostics && _destroy.LoadedCostTypes is { Count: > 0 } loaded ? loaded : _types;
        if (number >= costs.Count) throw new ArgumentOutOfRangeException(nameof(sid), "삭제의 최초 로딩 비용표 물리 범위");
        double cost = costs[number].Cost;
        if (!double.IsFinite(cost))
        {
            throw new ArgumentException("삭제 수명주기 유한 비용이 필요합니다");
        }
        // 차감 값은 원본처럼 비용을 그대로 뺀 뒤 마지막에 0 방향으로 절삭한다
        double decoded = cost;
        // 패치판 비용 표는 타입 번호마다 23 을 더해 정수로 저장한다
        long bias = (long)number * CostBias;
        double encoded = cost + bias;
        if (encoded < int.MinValue || encoded > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "삭제 수명주기 패치 비용 표 범위 오류");
        }
        decoded = unchecked((int)unchecked((uint)((long)encoded - bias)));
        double result = _bookkeeping.CurrentTotalCost - decoded;
        if (result < -4294967296.0 || result > 4294967295.0)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "삭제 수명주기 비용 누적 범위 오류");
        }
        return unchecked((int)unchecked((uint)(long)result));
    }

    /// <summary>
    /// 사전 처리의 모든 효과를 쓰기 전에 검사한다. 손상 목록·미연결 그래프·AI 효과·비유한 비용·누락 외부 효과를 거부한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void ValidatePre(int sid, uint flags)
    {
        if (_destroy.HasPatchDiagnostics) return;
        ReadOnlySpan<byte> raw = Object(sid);
        OriginalTypeRecord type = Type(sid);
        if (_state.Selected == sid && _hooks.Emit is null)
        {
            throw new InvalidOperationException("삭제 수명주기 선택 해제 콜백 누락");
        }
        PreGraph(sid, flags, true);
        if (_bookkeeping.Suppressed)
        {
            return;
        }
        byte owner = Owner(sid);
        if (owner > MaximumOwner)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), owner, "삭제 수명주기 소유자 범위 오류");
        }
        if ((type.Flags2 & FactoryMask) != 0 && owner != 0)
        {
            _ = _bookkeeping.OwnerFactories[owner].Items().Length;
        }
        if (!Ordinary(sid))
        {
            return;
        }
        if (_aiNotice is null && owner != 0 && _bookkeeping.AiAttached[owner])
        {
            throw new InvalidOperationException("삭제 수명주기 AI 통지 미복원");
        }
        // 실제 보상이 연결되면 쓰기 전 계산 입력을 검사하고, 없으면 외부 효과가 필요하다.
        if ((flags & NoRefundBit) == 0)
        {
            if (_reward is not null) _ = _reward.Plan(sid, (flags >> 16) & 15, false);
            else if (_hooks.Emit is null) throw new InvalidOperationException("삭제 수명주기 보상 콜백 누락");
        }
        if (LostNotice(sid, flags) && raw[TypeOffset] != _state.CurrentSilentType && _hooks.Emit is null)
        {
            throw new InvalidOperationException("삭제 수명주기 소리 콜백 누락");
        }
        if ((type.Flags1 & ProviderBit) != 0)
        {
            _ = _bookkeeping.Providers.Items().Length;
        }
        if ((type.Flags2 & FactoryMask) != 0)
        {
            _ = _bookkeeping.Factories.Items().Length;
        }
    }

    /// <summary>
    /// 사후 처리의 모든 효과를 쓰기 전에 검사한다. 비용 효과는 억제와 무관하며 그래프 분기를 먼저 검사한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void ValidatePost(int sid, uint flags)
    {
        if (_destroy.HasPatchDiagnostics) return;
        _ = Object(sid);
        if (NeedsPostGraph(sid, flags))
        {
            if (_graph is null)
            {
                throw new InvalidOperationException("공통 사후 파괴 그래프 효과 미연결");
            }
            _graph.ValidatePostDestroy(sid);
        }
        if (Ordinary(sid))
        {
            _ = RemainingCost(sid);
        }
    }

    /// <summary>선택 해제의 외부 UI 경계와 논리 선택 번호를 함께 처리한다.</summary>
    /// <param name="sid">지워지는 슬롯 번호.</param>
    private void ClearSelection(int sid)
    {
        if (_hooks.Emit is null)
        {
            throw new InvalidOperationException("삭제 수명주기 선택 해제 콜백 누락");
        }
        _hooks.Emit(new SquidDeletionEvent(SquidDeletionEffect.ClearSelection, sid, 0));
        if (!_destroy.HasPatchDiagnostics || _hooks.SelectedRaw is null) _state.Selected = 0;
    }

    /// <summary>
    /// 선택 처리 뒤·장부 억제 앞의 실제 분할·반납을 처리한다. 검사 모드에서는 아무것도 바꾸지 않는다.
    /// 원본은 0x800 파괴를 그래프 금지보다 먼저 판단하며 매립만 그래프 효과를 생략한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    /// <param name="validate">검사만 하면 참.</param>
    private void PreGraph(int sid, uint flags, bool validate)
    {
        ReadOnlySpan<byte> raw = Object(sid);
        // 그래프가 꺼졌거나 매립·비표면 타입이거나 분할 금지이면 그래프 효과가 없다
        if (!_bookkeeping.GraphsEnabled || (raw[ExtraOffset] & BuriedBit) != 0
            || (Type(sid).Flags1 & OriginalTypeRecord.Surface) == 0
            || ((flags & DestroyGraphBit) == 0 && (flags & NoGraphBit) != 0))
        {
            return;
        }
        if (_graph is null)
        {
            throw new InvalidOperationException("공통 사전 파괴 그래프 효과 미연결");
        }
        // 그래프 파괴 플래그가 있으면 그 번호의 표를 반납한다
        if ((flags & DestroyGraphBit) != 0)
        {
            byte number = raw[GraphOffset];
            if (validate)
            {
                _graph.ValidateFree(number);
            }
            else
            {
                _graph.Free(number);
            }
            return;
        }
        byte? sourceType = null;
        // 특수 표면은 방향 코드가 H 일 때만 치환 타입으로 뗀다
        if (raw[TypeOffset] == _state.CurrentSpecialSurfaceType)
        {
            if (_graph.Frame(sid).Side != 'H')
            {
                return;
            }
            uint replacement = _state.CurrentReplacementSurfaceType;
            if (replacement > byte.MaxValue) throw new ArgumentOutOfRangeException(nameof(sid), "삭제 Graph의 현재 대체 TYPE BYTE 범위");
            sourceType = (byte)replacement;
        }
        byte kind = sourceType ?? raw[TypeOffset];
        byte removed = kind == _state.CurrentLargeSurfaceType ? (byte)9 : (byte)1;
        if (validate)
        {
            _graph.ValidateDetach(sid, _state.RebuildGraph, removed, sourceType);
        }
        else
        {
            _graph.Detach(sid, _state.RebuildGraph, removed, sourceType);
        }
    }

    /// <summary>그래프 금지는 사전 분할만 억제한다. 사후 주변 추가는 임의로 억제하지 않는다.</summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    private bool NeedsPostGraph(int sid, uint flags)
    {
        return _bookkeeping.GraphsEnabled && (Object(sid)[ExtraOffset] & BuriedBit) == 0
            && (flags & DestroyGraphBit) == 0 && (Type(sid).Flags2 & (_state.RegionMask | 8)) != 0;
    }

    /// <summary>
    /// 공통 사전 파괴를 실행한다. 순서는 원본 그대로 "선택 해제 → 그래프 분할 → 소유자 작업장 → 보상 →
    /// 현재/누적 통계 → 소리·좌표 → 공급원 → 작업장 → 깊이 감소"다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void PreDestroy(int sid, uint flags)
    {
        if (_destroy.HasPatchDiagnostics)
        {
            if (Object(sid)[TypeOffset] < OriginalConstructors.FirstAssetType) _destroy.AssertPatch("!isForm()", 0x771);
        }
        else ValidatePre(sid, flags);
        if (IsSelected(sid))
        {
            // 실제 UI 기억점은 flags2 판정을 자체 몸체에 맡기고 원시 좌표를 먼저 전달한다.
            if (!Ordinary(sid))
            {
                if (_destroy.HasPatchDiagnostics && _hooks.RememberPlacement is { } remember)
                {
                    ReadOnlySpan<byte> raw = Object(sid);
                    remember(CoordinateBits(raw, XOffset), CoordinateBits(raw, YOffset), raw[TypeOffset]);
                }
                else if ((Type(sid).Flags2 & PlacementMask) != 0)
                {
                    ReadOnlySpan<byte> raw = Object(sid);
                    _state.PlacementX = Coordinate(raw, XOffset);
                    _state.PlacementY = Coordinate(raw, YOffset);
                    _state.PlacementType = raw[TypeOffset];
                    _bookkeeping.PendingPlacement = true;
                }
            }
            ClearSelection(sid);
        }
        PreGraph(sid, flags, false);
        if (!_bookkeeping.CurrentSuppressed)
        {
            // 소유자별 작업장 목록에서 지운다
            if ((Type(sid).Flags2 & FactoryMask) != 0)
            {
                byte owner = Owner(sid);
                if (owner != 0)
                {
                    Remove(_bookkeeping.OwnerFactories[owner], sid, _destroy.HasPatchDiagnostics);
                }
            }
            if (Ordinary(sid))
            {
                // 원본 인자: 슬롯 번호, 플래그 16~19비트의 수신자
                if ((flags & NoRefundBit) == 0)
                {
                    if (_reward is not null) _reward.Refund(sid, (flags >> 16) & 15, false);
                    else (_hooks.Emit ?? throw new InvalidOperationException("삭제 수명주기 보상 콜백 누락"))
                        (new SquidDeletionEvent(SquidDeletionEffect.Refund, sid, (flags >> 16) & 15));
                }
                int number = Object(sid)[TypeOffset];
                if (Owner(sid) == LocalOwner())
                {
                    _bookkeeping.LocalCounts[number] = unchecked(_bookkeeping.LocalCounts[number] - 1);
                }
                // 누적 생산 수는 남기므로 두 번째 표는 줄이지 않는다
                _bookkeeping.GlobalCounts[number] = unchecked(_bookkeeping.GlobalCounts[number] - 1);
                if (LostNotice(sid, flags))
                {
                    if (Object(sid)[TypeOffset] != _state.CurrentSilentType)
                    {
                        if (_hooks.Emit is null)
                        {
                            throw new InvalidOperationException("삭제 수명주기 소리 콜백 누락");
                        }
                        _hooks.Emit(new SquidDeletionEvent(SquidDeletionEffect.UnitLostSound, sid, 0));
                    }
                    ReadOnlySpan<byte> raw = Object(sid);
                    _state.CurrentLostX = Coordinate(raw, XOffset);
                    _state.CurrentLostY = Coordinate(raw, YOffset);
                }
                // 공급원·작업장 목록에서 빼고 재계산 알림을 늘린다
                if ((Type(sid).Flags1 & ProviderBit) != 0)
                {
                    Remove(_bookkeeping.Providers, sid, _destroy.HasPatchDiagnostics);
                    _bookkeeping.ProductionDirty = unchecked(_bookkeeping.ProductionDirty + 1);
                }
                _aiNotice?.Invoke(Owner(sid), sid, false);
                if ((Type(sid).Flags2 & FactoryMask) != 0)
                {
                    Remove(_bookkeeping.Factories, sid, _destroy.HasPatchDiagnostics);
                    _bookkeeping.ProductionDirty = unchecked(_bookkeeping.ProductionDirty + 1);
                }
            }
        }
        _destroy.CompletePreDestroy();
    }

    /// <summary>
    /// 공통 사후 파괴를 실행한다. 주변 표면을 원본 순서로 다시 추가하고 ordinary 비용을 마지막에 차감한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void PostDestroy(int sid, uint flags)
    {
        if (!_destroy.HasPatchDiagnostics) ValidatePost(sid, flags);
        if (NeedsPostGraph(sid, flags))
        {
            (_graph ?? throw new InvalidOperationException("공통 사후 파괴 Graph 효과 미연결")).PostDestroy(sid);
        }
        if (Ordinary(sid))
        {
            _bookkeeping.CurrentTotalCost = RemainingCost(sid);
        }
        _destroy.CompletePostDestroy();
    }

    /// <summary>
    /// 공통 삭제 본체의 실제 가상 훅을 만든다. 파생 가상 메서드는 바깥으로 넘기고 공통 사전·사후는 이 몸체가 처리한다.
    /// </summary>
    public SquidDestroyHooks Hooks()
    {
        var result = new SquidDestroyHooks
        {
            // 미연결 외부 효과는 해당 경계에서 중단한다. 임의의 대체 효과를 만들지 않는다.
            Emit = (SquidDestroyEvent effect) =>
            {
                switch (effect.Effect)
                {
                    case SquidDestroyEffect.PreDestroy:
                        PreDestroy(effect.Sid, effect.Flags);
                        break;
                    case SquidDestroyEffect.PostDestroy:
                        PostDestroy(effect.Sid, effect.Flags);
                        break;
                    case SquidDestroyEffect.ClearSelection:
                        ClearSelection(effect.Sid);
                        break;
                    default:
                        if (_hooks.Forward is null)
                        {
                            throw new InvalidOperationException("삭제 수명주기 종속·전파 콜백 미연결");
                        }
                        _hooks.Forward(effect);
                        break;
                }
            },
            Selected = () => _state.Selected,
        };
        if (_destroy.HasPatchDiagnostics && _hooks.SelectedRaw is not null)
            result.SelectedRaw = () => (_hooks.SelectedRaw ?? throw new InvalidOperationException("현재 Native 전체 DWORD 선택 조회 미연결"))();
        return result;
    }

    /// <summary>공급원 제거 뒤의 현재 AI 통지를 PATCH에서만 연결한다. null은 연결을 분리한다.</summary>
    public void BindPatchAiNotice(Action<uint, int, bool>? notice)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("PATCH AI 삭제 통지 판본 불일치");
        _aiNotice = notice;
    }

    /// <summary>Native 전체 선택 DWORD는 좁히지 않고 현재 getter의 함수 사본으로 비교한다.</summary>
    private bool IsSelected(int sid) => _destroy.HasPatchDiagnostics && _hooks.SelectedRaw is { } selected
        ? selected() == (uint)sid : _state.Selected == sid;

    /// <summary>
    /// 원본 Array::remove 와 같이 모든 중복을 순서대로 압축한다. inactive 꼬리 메모리는 지우지 않는다.
    /// </summary>
    /// <param name="list">압축할 목록.</param>
    /// <param name="sid">뺄 슬롯 번호.</param>
    private static void Remove(RawPostPopList list, int sid, bool native)
    {
        if (!native) { list.RemoveAll((uint)sid); return; }
        uint removed = 0, index = 0;
        // 원문 signed count가 양수일 때만 현재 포인터를 읽으며 같은 반복의 포인터에 압축한다.
        while (unchecked((int)index) < unchecked((int)list.Count))
        {
            uint[] source = list.Entries;
            if (index >= source.Length) throw new ArgumentOutOfRangeException(nameof(list), "PATCH 삭제 목록의 현재 DWORD 물리 부족");
            uint value = source[index];
            if (value == (uint)sid) removed = unchecked(removed + 1);
            else
            {
                uint destination = unchecked(index - removed);
                if (destination >= source.Length) throw new ArgumentOutOfRangeException(nameof(list), "PATCH 삭제 압축의 현재 DWORD 물리 부족");
                source[destination] = value;
            }
            index = unchecked(index + 1);
        }
        list.Count = unchecked(list.Count - removed);
    }

    /// <summary>정렬되지 않은 좌표의 원문 DWORD를 UI 기억점에 그대로 전달한다.</summary>
    private static uint CoordinateBits(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]);

    /// <summary>정렬되지 않은 float 의 비트를 원본 바이트 순서대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">좌표 바이트 위치.</param>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint bits = CoordinateBits(raw, offset);
        return BitConverter.UInt32BitsToSingle(bits);
    }
}
