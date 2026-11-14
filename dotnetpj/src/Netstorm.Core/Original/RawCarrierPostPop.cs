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

    /// <summary>현재 실제 권한을 읽는 장기 전역이다. 미연결이면 독립 Boss를 읽는다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>현재 실제 로딩 깊이를 읽는 장기 전역이다.</summary>
    public Func<uint>? SharedLoadingDepth { get; set; }
    /// <summary>현재 실제 간헐천TYPE DWORD를 읽는 장기 전역이다.</summary>
    public Func<uint>? SharedGeyserType { get; set; }
    /// <summary>비최초 flag 관문 뒤 현재 실제 권한을 조회한다.</summary>
    public bool ReadBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>Carrier 확인 뒤 현재 실제 로딩 깊이를 조회한다.</summary>
    public uint ReadLoadingDepth() => SharedLoadingDepth?.Invoke() ?? LoadingDepth;
    /// <summary>로딩 관문 뒤 현재 간헐천TYPE를 조회한다.</summary>
    public uint ReadGeyserType() => SharedGeyserType?.Invoke() ?? GeyserType;
}

/// <summary>
/// 운반체 후처리가 외부 효과에 연결하는 지점 묶음(원본 CarrierPostPopHooks).
/// PATCH는 도달한 효과만 요구하며 CD는 구성 때 네 효과를 요구한다.
/// </summary>
public sealed class CarrierPostPopHooks
{
    /// <summary>가상 +0xCC 확인. 원본은 반환값을 쓰지 않는다. 비보스의 비최초 호출 때만 부른다.</summary>
    public Action<int>? CarrierCheck { get; set; }

    /// <summary>지면 받침 소유자 갱신(원본 0044bd20). 로딩 중 조건을 만족할 때 현재 좌표·소유자로 부른다.</summary>
    public Action<float, float, byte>? ClaimGround { get; set; }

    /// <summary>CD의 기존 사전 검사다. PATCH 실제 호출은 이 경계를 호출하지 않는다.</summary>
    public Action<int, uint>? ValidateBase { get; set; }

    /// <summary>같은 플래그로 공통 후처리 몸체를 부른다.</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>
/// 운반체의 직접 호출 몸체와 Damageable 지원 처리(원본 00427720·0044bf80, 10.78).
/// 직접 호출은 비보스의 비최초 때 +0xCC 확인을 한 뒤 Damageable 몸체로 이어지고,
/// Damageable 직접 호출은 그 확인을 거치지 않는다. 두 경로 모두 공통 몸체를 정확히 한 번 부른다.
/// PATCH는 실제 도달 순서와 현재 전역을 읽고 CD는 판본별 필드와 사전 검사 계약을 유지한다.
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

    /// <summary>실제 PATCH 월드의 현재 TYPE 표다. 없으면 생성 시 소유 사본을 읽는다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;

    /// <summary>처리 중 읽는 상태. 호출자가 소유하며 이 클래스가 직접 바꾸지 않는다.</summary>
    private readonly CarrierPostPopState _state;

    /// <summary>선택된 실제 외부 효과를 원문 함수 객체처럼 복사해 소유한다.</summary>
    private readonly CarrierPostPopHooks _hooks;

    /// <summary>운반체 후처리 모듈을 만든다. 같은 풀·판본별 TYPE 표·효과를 받는다. PATCH는 도달한 효과만 요구한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. PATCH188개·CD171개여야 한다.</param>
    /// <param name="state">공유 상태. 이 모듈보다 오래 살아야 한다.</param>
    /// <param name="hooks">실제 효과. CD는 네 항목 모두 구성 때 요구한다.</param>
    public RawCarrierPostPop(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CarrierPostPopState state, CarrierPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // 판본별 실제 TYPE 표 크기는 효과 전에 확인한다
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("Carrier 타입 판본·물리 크기 오류", nameof(types));
        }
        if (pool.Edition != OriginalEdition.Patch1078
            && (hooks.CarrierCheck is null || hooks.ClaimGround is null || hooks.ValidateBase is null || hooks.Base is null))
        {
            throw new ArgumentException("운반체 후처리 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _types = types.Select(type => type.Clone()).ToArray();
        _state = state;
        _hooks = new CarrierPostPopHooks
        {
            CarrierCheck = hooks.CarrierCheck, ClaimGround = hooks.ClaimGround,
            ValidateBase = hooks.ValidateBase, Base = hooks.Base,
        };
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 공통 후처리와 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 직접 호출 몸체(원본 00427720). PATCH는 사전 검사를 삽입하지 않고,
    /// 비최초 호출의 현재 boss가 거짓이면 +0xCC 확인 뒤 Damageable 몸체로 간다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void PostPop(int sid, uint flags)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) Validate(sid, flags);
        // 비최초 호출인데 보스가 아니면 가상 +0xCC 확인을 먼저 부른다
        if ((flags & FirstPopFlag) == 0 && !_state.ReadBoss())
        {
            Need(_hooks.CarrierCheck)(sid);
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
        if (_pool.Edition != OriginalEdition.Patch1078) Validate(sid, flags);
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
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 원문 최초 flag의 genus 조회는 반환값을 버려도 현재 TYPE 물리 접근을 먼저 수행한다.
        if (patch && (flags & FirstPopFlag) != 0) _ = CurrentType(slot[RawSidPool.TypeOffset]).Flags2 & 0x130000u;
        int extraOffset = patch ? ExtraOffset : 35;
        int ownerOffset = patch ? OwnerOffset : 32;
        if (_state.ReadLoadingDepth() != 0
            && slot[RawSidPool.TypeOffset] != _state.ReadGeyserType()
            && (slot[extraOffset] & AbstractBuriedMask) == 0
            && (CurrentType(slot[RawSidPool.TypeOffset]).Flags1 & ClaimGroundBit) != 0)
        {
            float x = BitConverter.UInt32BitsToSingle(Read(slot, XOffset));
            float y = BitConverter.UInt32BitsToSingle(Read(slot, YOffset));
            Need(_hooks.ClaimGround)(x, y, slot[ownerOffset]);
        }
        Need(_hooks.Base)(sid, flags);
    }

    /// <summary>도달한 실제 장치만 그 시점에 선택한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function
        ?? throw new InvalidOperationException("Carrier 후처리의 선택된 실제 장치 미연결");

    /// <summary>낮은 프로세스TYPE도 실제188칸 범위 안이면 원문처럼 읽는다.</summary>
    private OriginalTypeRecord CurrentType(uint number)
    {
        IReadOnlyList<OriginalTypeRecord> current = _currentTypes ?? _types;
        if (number >= current.Count) throw new ArgumentOutOfRangeException(nameof(number), "Carrier 현재TYPE 실제 물리 범위 오류");
        return current[(int)number];
    }

    /// <summary>현재 PATCH TYPE 표를 빌리고 빈 입력이면 생성 시 사본으로 돌아간다. 실패 시 기존 대여를 보존한다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord>? types)
    {
        if (types is not null && types.Count != 0 && (_pool.Edition != OriginalEdition.Patch1078 || types.Count != 188))
            throw new ArgumentException("Carrier 현재TYPE 대여 판본·길이 오류", nameof(types));
        _currentTypes = types is null || types.Count == 0 ? null : types;
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
