namespace Netstorm.Core.Original;

/// <summary>
/// 보상 계산이 비교하는 원본 타입 번호 전역. 두 PE 의 초기값이 같다.
/// 이름은 cpppj `--dump-types` 의 타입 이름이다.
/// </summary>
public sealed class SquidRewardTypes
{
    /// <summary>제단(altar) 타입. 보상 대상이 아니며 비용 곡선의 개수가 하나 줄어든다.</summary>
    public uint None { get; set; } = 164;

    /// <summary>받침(dais) 타입. 제단과 같은 개수 규칙을 쓴다.</summary>
    public uint Dais { get; set; } = 165;

    /// <summary>비를 막는 벽(rainBlocker) 타입. 삭제 보상에서는 제외하고 해체 보상에서는 포함한다.</summary>
    public uint Silent { get; set; } = 147;

    /// <summary>성장형 rainBlocker 타입. 비용은 rainBlocker 것을 쓰고 해체 환불은 0 이다.</summary>
    public uint Alias { get; set; } = 150;

    /// <summary>너겟(nugget) 타입. 해체 환불 비용에 1/4 배율을 곱하지 않는다.</summary>
    public uint FullValue { get; set; } = 154;

    /// <summary>사제(priest) 타입. 사제 1/4 HP 모드의 대상이다.</summary>
    public uint Priest { get; set; } = 158;
}

/// <summary>
/// 보상 계산이 읽거나 갱신하는 판본별 전역과 AI 지갑(원본 SquidRewardState, 10.78 패치판).
/// </summary>
public sealed class SquidRewardState
{
    /// <summary>보상 계산이 비교하는 타입 번호 전역.</summary>
    public SquidRewardTypes Types { get; } = new();

    /// <summary>삭제 보상 비율(원본 DAT_005424bc). 전투 초기화가 25 로 정한다.</summary>
    public uint Percent { get; set; } = 25;

    /// <summary>보상하지 않은 가치의 누적(원본 DAT_00595088). 샘 생성 비교에 쓴다.</summary>
    public int GeyserPool { get; set; }

    /// <summary>SP 를 고정값으로 만드는 첫 전역(원본 DAT_00594fb8).</summary>
    public bool FixedSpA { get; set; }

    /// <summary>SP 를 고정값으로 만드는 둘째 전역(원본 DAT_005c85a4).</summary>
    public bool FixedSpB { get; set; }

    /// <summary>해체 환불을 HP 절반 미만에서 0 으로 만드는 전역(원본 DAT_00594fbc).</summary>
    public bool HalfHealthMode { get; set; }

    /// <summary>AI 지갑 갱신을 끄는 전역(원본 DAT_00594fc8).</summary>
    public bool AiMirrorDisabled { get; set; }

    /// <summary>지갑을 0x4a6 이상으로 올리는 객체 조건(원본 DAT_005634a0→+0x68).</summary>
    public bool WalletClamp { get; set; }

    /// <summary>사제 최대 HP 1/4 전역(원본 DAT_005ca8f0).</summary>
    public bool PriestQuarterHp { get; set; }

    /// <summary>패치 0049fce0 의 디버그 assert 전역(원본 DAT_005e4794). 켜지면 위반 입력을 거부한다.</summary>
    public bool DebugAsserts { get; set; }

    /// <summary>소유자별 AI 지갑(1~8). 0 번은 쓰지 않는다.</summary>
    public int[] AiWallet { get; } = new int[RawPostPopState.OwnerCount];

    /// <summary>CD 판 로컬 SP 전역(원본 005120d4). 패치판은 이 필드 대신 SP 저장소를 쓴다.</summary>
    public float CdLocalSp { get; set; }
}

/// <summary>
/// 객체 보상 외부 동작을 연결하는 콜백 묶음(원본 SquidRewardHooks, 패치판 범위).
/// </summary>
public sealed class SquidRewardHooks
{
    /// <summary>파생 객체의 가상 +0x80 개수 조회. null 이면 상태 상위 3비트를 쓴다.</summary>
    public Func<int, int>? VirtualCount { get; set; }

    /// <summary>SP 표시 갱신(원본 0043dad0). 표시 효과이므로 null 이어도 된다.</summary>
    public Action? RefreshSp { get; set; }
}

/// <summary>
/// 한 번의 보상 계산 결과. <see cref="Applied"/> 가 거짓이면 아무 상태도 바뀌지 않았다.
/// </summary>
/// <param name="Applied">보상이 적용됐는지.</param>
/// <param name="Cost">정수 비용.</param>
/// <param name="Refund">지급액. 소유자가 0 이면 0 이다.</param>
public readonly record struct SquidRewardResult(bool Applied, int Cost, int Refund);

/// <summary>
/// 삭제·해체 보상(원본 패치 0044c2f0, 10.78): 비용 곡선·소유자 비율·SP 지급·AI 지갑·샘 풀을 복원한다.
/// 패치판 SP 저장소(<see cref="RawSpStore"/>)를 요구하며 CD 판은 옮기지 않았다.
/// 계산은 상태를 바꾸지 않고, 적용은 <see cref="Refund"/> 만 한다.
/// 근거: cpppj/src/o/SquidReward.cpp (W05 선행 조각, 2026-10-22).
/// </summary>
public sealed class RawSquidReward
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치. 상위 3비트가 가상 개수다.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 HP DWORD 의 위치(10.78).</summary>
    private const int HpOffset = 26;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>지갑을 끌어올리는 최소값(원본 float 0x500ff8 ≈ 0x4a6).</summary>
    private const int WalletFloor = 0x4a6;

    /// <summary>SP 고정 모드의 패치판 값(원본 194162.0).</summary>
    private const double PatchFixedSp = 194162.0;

    /// <summary>비용 표가 번호마다 더한 값(원본 23).</summary>
    private const long CostBias = 23;

    /// <summary>해체 환불 배율(원본 0.25).</summary>
    private const double SalvageScale = 0.25;

    /// <summary>타입 플래그2의 받침(dais) 비트.</summary>
    private const uint DaisKind = 0x400000;

    /// <summary>타입 플래그1의 HP 보유 비트.</summary>
    private const uint HasHitPointBit = 0x10;

    /// <summary>비용 누적이 정확한 정수 범위를 넘는지 보는 한계(2^53).</summary>
    private const double ExactLimit = 9007199254740992.0;

    /// <summary>비용 곡선 항 수의 상한. 넘으면 원본 계산 범위 밖이다.</summary>
    private const int MaxTerms = 0x01000000;

    /// <summary>객체 슬롯을 읽는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 비용·HP·플래그를 조회하는 타입 표.</summary>
    private readonly OriginalTypeRecord[] _types;

    /// <summary>AI 부착·로컬 소유자 장부.</summary>
    private readonly RawPostPopState _bookkeeping;

    /// <summary>보상 전역과 AI 지갑.</summary>
    private readonly SquidRewardState _state;

    /// <summary>패치판 SP 저장소. 10.78 만 옮겼으므로 반드시 있어야 한다.</summary>
    private readonly RawSpStore _store;

    /// <summary>외부 표시·개수 조회 훅.</summary>
    private readonly SquidRewardHooks _hooks;

    /// <summary>가상 +0x80 을 물은 횟수(진단용).</summary>
    private uint _virtualCalls;

    /// <summary>
    /// 보상 처리기를 만든다. 풀·타입 표·장부·상태·SP 저장소는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀.</param>
    /// <param name="types">10.78 타입 표(188개).</param>
    /// <param name="bookkeeping">AI 부착·로컬 소유자 장부.</param>
    /// <param name="state">보상 전역과 AI 지갑.</param>
    /// <param name="store">패치판 SP 저장소. null 이면 거부한다.</param>
    /// <param name="hooks">표시·개수 조회 훅.</param>
    public RawSquidReward(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState bookkeeping,
        SquidRewardState state, RawSpStore? store, SquidRewardHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(state);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (store is null)
        {
            throw new ArgumentException("패치판 보상은 SP 저장소가 필요합니다", nameof(store));
        }
        _pool = pool;
        _types = types.ToArray();
        _bookkeeping = bookkeeping;
        _state = state;
        _store = store;
        _hooks = hooks ?? new SquidRewardHooks();
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>삭제 장부가 같은 풀·장부를 쓰는지 확인할 때 쓴다.</summary>
    public RawPostPopState Bookkeeping => _bookkeeping;

    /// <summary>가상 +0x80 을 물은 횟수.</summary>
    public uint VirtualCountCalls => _virtualCalls;

    /// <summary>원본과 같이 번호·타입 범위를 확인한 뒤 슬롯을 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Raw(int sid)
    {
        if (sid == 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "보상 SID 범위 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), raw[TypeOffset], "보상 타입 번호 범위 오류");
        }
        return raw;
    }

    /// <summary>리틀 엔디언 32비트 값을 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Dword(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 네 바이트를 호스트 정렬과 무관하게 모은다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>원본 __ftol: 0 방향 절삭 뒤 하위 32비트만 남긴다. 비유한·범위 밖은 거부한다.</summary>
    /// <param name="value">변환할 값.</param>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || Math.Abs(value) >= 9.2e18)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "보상 정수 변환 범위 오류");
        }
        return unchecked((int)(uint)(long)value);
    }

    /// <summary>32비트로 감기는 정수 변환.</summary>
    /// <param name="value">변환할 64비트 값.</param>
    private static int Wrap(long value) => unchecked((int)(uint)value);

    /// <summary>
    /// 패치판 정수 비용 표(원본 DAT_0059ab50)의 디코딩 값이다. 로더가 ftol(cost + 번호*23)을 저장하고
    /// 읽을 때 번호*23 을 뺀다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    public int TypeCostUnits(uint type)
    {
        if (type >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "보상 비용 타입 번호 범위 오류");
        }
        double encoded = (double)_types[(int)type].Cost + (double)((long)type * CostBias);
        if (encoded < -2147483648.0 || encoded >= 2147483648.0)
        {
            throw new ArgumentOutOfRangeException(nameof(type), encoded, "보상 패치 비용 범위 오류");
        }
        return Wrap((long)encoded - (long)type * CostBias);
    }

    /// <summary>
    /// base 가상 +0x80 이 돌려주는 개수. 훅이 있으면 파생 구현이고, 없으면 상태 상위 3비트다.
    /// 호출마다 진단 카운터를 늘린다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private int Count(int sid)
    {
        _virtualCalls++;
        return _hooks.VirtualCount is not null ? _hooks.VirtualCount(sid) : Raw(sid)[StateOffset] >> 5;
    }

    /// <summary>
    /// 타입의 최대 HP. 사제 1/4 모드를 적용하고, 디버그 assert 가 켜지면 HP 없는 타입을 거부한다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    public int TypeHitPoints(uint type)
    {
        if (type >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "보상 HP 타입 번호 범위 오류");
        }
        OriginalTypeRecord record = _types[(int)type];
        // 원본은 UI 경고만 내지만 디버그 assert 가 켜지면 즉시 멈춘다
        if (_state.DebugAsserts && (record.Flags1 & HasHitPointBit) == 0 && type != _state.Types.Dais)
        {
            throw new InvalidOperationException("보상 HP 없는 타입의 assert 경로");
        }
        return _state.PriestQuarterHp && type == _state.Types.Priest ? record.MaxHitPoints / 4 : record.MaxHitPoints;
    }

    /// <summary>객체가 HP 를 갖는지 확인한다. 받침(dais)은 abstract 비트가 정한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public bool HasHitPoints(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        return (type.Flags2 & DaisKind) != 0 ? (raw[ExtraOffset] & 1) != 0 : (type.Flags1 & HasHitPointBit) != 0;
    }

    /// <summary>객체의 최대 HP. 받침은 abstract 면 0, 아니면 제단 타입 HP 다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public int MaxHitPoints(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        if ((type.Flags2 & DaisKind) == 0)
        {
            return TypeHitPoints(raw[TypeOffset]);
        }
        return (raw[ExtraOffset] & 1) != 0 ? TypeHitPoints(_state.Types.None) : 0;
    }

    /// <summary>
    /// 원본 0044b220: 판본별 부동소수 순서로 비용 곡선을 계산한 뒤 0 방향으로 절삭한다.
    /// 제단·받침은 개수를 하나 줄여 곡선을 쓴다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="count">현재 개수.</param>
    private int CostCurve(uint type, int count)
    {
        if (type >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "보상 비용 타입 번호 범위 오류");
        }
        int n = type == _state.Types.None || type == _state.Types.Dais ? Wrap((long)count - 1) : count;
        if (n < 0)
        {
            return 0;
        }
        if (n >= MaxTerms)
        {
            throw new ArgumentOutOfRangeException(nameof(count), count, "보상 비용 곡선 개수 범위 오류");
        }
        // 같은 float 비용을 n+1 번 누적한다. 항 수×단가가 정확한 정수 범위를 지키게 한다
        double unit = (double)(float)TypeCostUnits(type);
        double sum = unit * (double)(n + 1);
        if (Math.Abs(sum) >= ExactLimit)
        {
            throw new ArgumentOutOfRangeException(nameof(count), sum, "보상 비용 곡선 누적 범위 오류");
        }
        return Ftol(sum);
    }

    /// <summary>
    /// 원본 0043cb30: 해체 환불용 float 비용(nugget 외에는 1/4). 개수가 음수면 0 이다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="count">현재 개수.</param>
    private float SalvageCost(uint type, int count)
    {
        if (type >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "보상 환불 타입 번호 범위 오류");
        }
        int n = type == _state.Types.None || type == _state.Types.Dais ? Wrap((long)count - 1) : count;
        double sum = 0.0;
        if (n >= 0)
        {
            if (n >= MaxTerms)
            {
                throw new ArgumentOutOfRangeException(nameof(count), count, "보상 비용 곡선 개수 범위 오류");
            }
            double unit = (double)(float)TypeCostUnits(type);
            sum = unit * (double)(n + 1);
            if (Math.Abs(sum) >= ExactLimit)
            {
                throw new ArgumentOutOfRangeException(nameof(count), sum, "보상 비용 곡선 누적 범위 오류");
            }
        }
        // 누적값에 배율을 곱한 뒤 결과만 float 로 절삭한다
        return (float)(type != _state.Types.FullValue ? sum * SalvageScale : sum);
    }

    /// <summary>
    /// 원본 0044c240: 메뉴가 보존한 타입·개수로 환불만 계산한다. HP 비례와 절반 HP 규칙을 적용한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="type">계산에 쓸 타입 번호.</param>
    /// <param name="count">계산에 쓸 개수.</param>
    public int SalvageRefund(int sid, uint type, int count)
    {
        if (type == _state.Types.Alias)
        {
            return 0;
        }
        float cost = SalvageCost(type, count);
        if (!HasHitPoints(sid))
        {
            return Ftol((double)cost);
        }
        ReadOnlySpan<byte> raw = Raw(sid);
        int hp = unchecked((int)Dword(raw, HpOffset));
        int max = MaxHitPoints(sid);
        // 절반 미만이면 환불 없음. 나머지 나눗셈은 0 방향 절삭이다
        if (_state.HalfHealthMode && hp < max / 2)
        {
            return 0;
        }
        // 최대 HP 0 은 x87 에서 무한/NaN 이 되어 절삭 변수가 무효값이 된다. 원본 기계 확인대로 0 을 돌려준다
        if (max == 0)
        {
            return 0;
        }
        float product = (float)((double)hp * (double)cost);
        return Ftol((double)product / (double)max);
    }

    /// <summary>
    /// 상태를 바꾸지 않고 비용·지급액을 계산한다. 손상 입력은 여기서 거부한다.
    /// 제단은 대상이 아니고, 삭제 보상에서는 rainBlocker 를 제외하며, 성장형 rainBlocker 는 비용만 대체한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="recipient">받을 소유자(0~8). 0 이면 지급액이 0 이다.</param>
    /// <param name="salvage">참이면 남은 HP 비례 해체 환불, 거짓이면 삭제 보상 비율이다.</param>
    public SquidRewardResult Plan(int sid, uint recipient, bool salvage)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        if (recipient > RawPostPopState.OwnerCount - 1)
        {
            throw new ArgumentOutOfRangeException(nameof(recipient), recipient, "보상 수신자 번호는 0~8 이어야 합니다");
        }
        uint type = raw[TypeOffset];
        if (type == _state.Types.None || (!salvage && type == _state.Types.Silent))
        {
            return default;
        }
        // 가상 개수는 원본처럼 호출마다 다시 묻는다. 계산은 상태를 바꾸지 않는다
        int cost = CostCurve(type, Count(sid));
        if (raw[TypeOffset] == _state.Types.Alias)
        {
            cost = CostCurve(_state.Types.Silent, Count(sid));
        }
        if (cost == 0)
        {
            return default;
        }
        int refund = salvage
            ? SalvageRefund(sid, raw[TypeOffset], Count(sid))
            : unchecked((int)unchecked(_state.Percent * (uint)cost)) / 100;
        if (recipient == 0)
        {
            refund = 0;
        }
        return new SquidRewardResult(true, cost, refund);
    }

    /// <summary>
    /// 원본 00413bb0: SP 를 바꾸기 전에 AI 가 붙은 소유자의 지갑에 정수 금액을 더한다.
    /// 지갑 비활성·미부착·최솟값·32비트 감김 규칙을 쓴다.
    /// </summary>
    /// <param name="owner">소유자 번호.</param>
    /// <param name="amount">더할 정수 금액(음수면 차감).</param>
    private void AddWallet(uint owner, int amount)
    {
        if (_state.AiMirrorDisabled || owner < 1 || owner > RawPostPopState.OwnerCount - 1 || !_bookkeeping.AiAttached[owner])
        {
            return;
        }
        int wallet = Wrap((long)_state.AiWallet[owner] + amount);
        if (_state.WalletClamp && wallet <= WalletFloor)
        {
            wallet = WalletFloor;
        }
        _state.AiWallet[owner] = wallet;
    }

    /// <summary>너겟 내용물 삭제가 SP 앞에 수행하는 별도 AI 지갑 정수 가산.</summary>
    /// <param name="owner">소유자 번호.</param>
    /// <param name="amount">더할 금액.</param>
    public void CreditWallet(uint owner, int amount) => AddWallet(owner, amount);

    /// <summary>
    /// 원본 패치 0040eec0/0041df80: 소유자의 SP 에 delta 를 더하고 AI 지갑과 SP 표시를 갱신한다.
    /// SP 고정 모드에서는 delta 대신 고정값과의 차이가 쓰인다.
    /// </summary>
    /// <param name="owner">소유자 번호.</param>
    /// <param name="delta">더할 SP(음수면 차감).</param>
    public void AddSp(uint owner, float delta)
    {
        float sp = BitConverter.UInt32BitsToSingle(_store.Get(owner));
        if (!float.IsFinite(sp))
        {
            throw new InvalidOperationException("보상 SP 시작값이 유한하지 않습니다");
        }
        bool fixedMode = _state.FixedSpA || _state.FixedSpB;
        if (fixedMode)
        {
            delta = (float)(PatchFixedSp - (double)sp);
        }
        float sum = (float)((double)sp + (double)delta);
        int mirror = Ftol((double)delta);
        _store.Set(owner, BitConverter.SingleToUInt32Bits(sum));
        AddWallet(owner, mirror);
        _hooks.RefreshSp?.Invoke();
    }

    /// <summary>
    /// 원본 패치 0044c2f0: 계획대로 SP·지갑을 지급하고 보상하지 않은 가치를 샘 풀에 누적한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="recipient">받을 소유자(0~8).</param>
    /// <param name="salvage">참이면 해체 환불, 거짓이면 삭제 보상 비율이다.</param>
    public SquidRewardResult Refund(int sid, uint recipient, bool salvage)
    {
        SquidRewardResult plan = Plan(sid, recipient, salvage);
        if (!plan.Applied)
        {
            return plan;
        }
        if (recipient == _bookkeeping.LocalOwner)
        {
            // 패치판은 로컬이든 다른 소유자든 같은 SP 저장소 가산 경로를 쓴다
            AddSp(recipient, (float)plan.Refund);
        }
        else
        {
            AddSp(recipient, (float)plan.Refund);
        }
        _state.GeyserPool = Wrap((long)_state.GeyserPool + plan.Cost - plan.Refund);
        return plan;
    }
}
