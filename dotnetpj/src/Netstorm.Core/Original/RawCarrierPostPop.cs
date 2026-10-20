namespace Netstorm.Core.Original;

/// <summary>
/// 운반체 후처리(Carrier/Damageable)가 읽고 갱신하는 상태 묶음(원본 CarrierPostPopState).
/// 보스 여부·로딩 깊이·간헐천 타입 번호를 담으며, 이 인스턴스는 모듈보다 오래 살아야 한다.
/// </summary>
public sealed class CarrierPostPopState
{
    /// <summary>보스 여부(원본 00540bc4). 거짓이면 비최초 호출 때 가상 +0xCC 확인을 먼저 부른다.</summary>
    public bool Boss { get; set; } = true;

    /// <summary>현재 로딩의 중첩 깊이(원본 005c89b8). 0이 아니면 지면 소유자 갱신 조건을 검사한다.</summary>
    public uint LoadingDepth { get; set; }

    /// <summary>간헐천 타입 번호(원본 00541240). 로딩 중 지면 소유자 갱신에서 제외한다.</summary>
    public uint GeyserType { get; set; } = 122;
}

/// <summary>
/// 운반체 후처리가 외부 효과에 연결하는 지점 묶음(원본 CarrierPostPopHooks).
/// 호출자가 모두 연결해야 하며, 하나라도 없으면 생성자에서 거부한다.
/// </summary>
public sealed class CarrierPostPopHooks
{
    /// <summary>가상 +0xCC 확인. 원본은 반환값을 쓰지 않는다. 비보스의 비최초 호출 때만 부른다.</summary>
    public Action<int>? CarrierCheck { get; set; }

    /// <summary>지면 받침 소유자 갱신(원본 0044bd20). 로딩 중 조건을 만족할 때 현재 좌표·소유자로 부른다.</summary>
    public Action<float, float, byte>? ClaimGround { get; set; }

    /// <summary>효과 전에 공통 후처리의 상태·Graph·비용 검사를 먼저 한다.</summary>
    public Action<int, uint>? ValidateBase { get; set; }

    /// <summary>같은 플래그로 공통 후처리 몸체를 부른다.</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>
/// 운반체의 직접 호출 몸체와 Damageable 지원 처리(원본 00427720·0044bf80, 10.78).
/// 직접 호출은 비보스의 비최초 때 +0xCC 확인을 한 뒤 Damageable 몸체로 이어지고,
/// Damageable 직접 호출은 그 확인을 거치지 않는다. 두 경로 모두 공통 몸체를 정확히 한 번 부른다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 추가·소유자 바이트 위치와 Damageable 조건이 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawCarrierPostPop.cpp (W02 Carrier 조각, 2026-10-20).
/// </summary>
public sealed class RawCarrierPostPop
{
    /// <summary>상태 바이트의 free(1)·들어 있는 내용물(8) 마스크. 켜져 있으면 후처리의 대상이 아니다.</summary>
    private const byte StateMask = 9;

    /// <summary>추가 비트의 abstract(1)·매립(8) 마스크. 켜져 있으면 지면 소유자 갱신을 건너뛴다.</summary>
    private const byte AbstractBuriedMask = 9;

    /// <summary>타입 플래그1의 지면 소유자 갱신 비트(원본 0x400).</summary>
    private const uint ClaimGroundBit = 0x400;

    /// <summary>최초 등록 호출 플래그 비트(원본 flags &amp; 1).</summary>
    private const uint FirstPopFlag = 1;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 현재 X 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 Y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 플래그1의 지면 갱신 비트를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>처리 중 읽는 상태. 호출자가 소유하며 이 클래스가 직접 바꾸지 않는다.</summary>
    private readonly CarrierPostPopState _state;

    /// <summary>외부 효과 연결 지점. 생성 때 모두 있어야 한다.</summary>
    private readonly CarrierPostPopHooks _hooks;

    /// <summary>운반체 후처리 모듈을 만든다. 같은 풀·10.78 타입 표·연결된 효과를 받아야 한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="state">공유 상태. 이 모듈보다 오래 살아야 한다.</param>
    /// <param name="hooks">외부 효과 연결 지점. 네 항목이 모두 있어야 한다.</param>
    public RawCarrierPostPop(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CarrierPostPopState state, CarrierPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // CD 10.72(171개)는 추가·소유자 바이트 위치와 Damageable 조건이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (hooks.CarrierCheck is null || hooks.ClaimGround is null || hooks.ValidateBase is null || hooks.Base is null)
        {
            throw new ArgumentException("운반체 후처리 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _types = types;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 공통 후처리와 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 직접 호출 몸체(원본 00427720). 공간 쓰기 전에 상태를 검사하고,
    /// 비최초 호출인데 보스가 아니면 +0xCC 확인을 먼저 부른 뒤 Damageable 몸체로 간다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void PostPop(int sid, uint flags)
    {
        Validate(sid, flags);
        // 비최초 호출인데 보스가 아니면 가상 +0xCC 확인을 먼저 부른다
        if ((flags & FirstPopFlag) == 0 && !_state.Boss)
        {
            _hooks.CarrierCheck!(sid);
        }
        RunDamageable(sid, flags);
    }

    /// <summary>
    /// Damageable 직접 호출 몸체(원본 0044bf80). +0xCC 확인을 거치지 않고 바로 지원 처리로 간다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void DamageablePostPop(int sid, uint flags)
    {
        Validate(sid, flags);
        RunDamageable(sid, flags);
    }

    /// <summary>
    /// 공통 후처리와 같은 풀의 <see cref="RawPostPop"/> 에 운반체 효과를 연결하는 묶음을 만든다.
    /// +0xCC 확인과 지면 갱신은 호출자가 주고, 공통 검사·몸체는 같은 풀의 기본 몸체에 위임한다.
    /// </summary>
    /// <param name="pool">슬롯 풀. 기본 몸체의 풀과 같아야 한다.</param>
    /// <param name="base">같은 풀의 공통 후처리 몸체.</param>
    /// <param name="carrierCheck">가상 +0xCC 확인.</param>
    /// <param name="claimGround">지면 받침 소유자 갱신.</param>
    public static CarrierPostPopHooks MakeCarrierPostPopHooks(RawSidPool pool, RawPostPop @base, Action<int> carrierCheck, Action<float, float, byte> claimGround)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(@base);
        // 기본 몸체와 다른 풀의 효과를 섞으면 원본 iterator 와 달라지므로 거부한다
        if (!ReferenceEquals(pool, @base.Pool))
        {
            throw new ArgumentException("운반체/공통 후처리의 SID 풀이 다릅니다", nameof(pool));
        }
        return new CarrierPostPopHooks
        {
            CarrierCheck = carrierCheck,
            ClaimGround = claimGround,
            ValidateBase = (sid, flags) => @base.ValidateBase(sid, flags),
            Base = (sid, flags) => @base.PostPopBase(sid, flags),
        };
    }

    /// <summary>
    /// 공간 쓰기 전에 현재 입력 분기가 요구하는 외부 효과와 공통 검사를 확인한다.
    /// 빈·내용물·자산 밖 타입은 풀·외부 효과를 바꾸기 전에 거부한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    private void Validate(int sid, uint flags)
    {
        // 할당되지 않은 번호는 슬롯을 읽기 전에 거부한다
        Span<byte> raw = _pool.AllocatedBytes(sid);
        // 빈·내용물·자산 밖 타입은 운반체 후처리의 대상이 아니다
        if ((raw[RawSidPool.StateOffset] & StateMask) != 0
            || raw[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType
            || raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("운반체 후처리 raw 자산 상태 오류");
        }
        _hooks.ValidateBase!(sid, flags);
    }

    /// <summary>
    /// Damageable 지원 처리. 로딩 중 조건을 만족하면 현재 좌표·소유자로 지면을 갱신하고, 같은 플래그로 공통 몸체를 부른다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    private void RunDamageable(int sid, uint flags)
    {
        ReadOnlySpan<byte> slot = _pool.Slot(sid);
        // 로딩 중이고 간헐천이 아니며 abstract·매립이 아니고 타입에 지면 갱신 비트가 있으면 현재 소유자를 갱신한다
        if (_state.LoadingDepth != 0
            && slot[RawSidPool.TypeOffset] != _state.GeyserType
            && (slot[ExtraOffset] & AbstractBuriedMask) == 0
            && (_types[slot[RawSidPool.TypeOffset]].Flags1 & ClaimGroundBit) != 0)
        {
            float x = BitConverter.UInt32BitsToSingle(Read(slot, XOffset));
            float y = BitConverter.UInt32BitsToSingle(Read(slot, YOffset));
            _hooks.ClaimGround!(x, y, slot[OwnerOffset]);
        }
        _hooks.Base!(sid, flags);
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        uint value = 0;
        // 필요한 4바이트만 낮은 바이트부터 조합한다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
