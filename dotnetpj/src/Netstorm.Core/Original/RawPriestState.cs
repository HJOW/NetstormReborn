namespace Netstorm.Core.Original;

/// <summary>
/// 사제·운반체 HP 변경의 권한과 회복 허용 상태(원본 PriestHitPointMode).
/// 권한과 허용 표는 같은 Client의 장기 값을 빌릴 수 있으며, 미연결 독립 검사는 값형 입력을 쓴다.
/// </summary>
public sealed class PriestHitPointMode
{
    /// <summary>HP 변경을 마무리할 권한(원본 00540bc4). 공유 권한이 연결되면 읽지 않는다.</summary>
    public bool Authority { get; set; } = true;

    /// <summary>실제 Client boss의 장기 권한. 연결되면 <see cref="GetAuthority"/>가 이 값을 읽는다.</summary>
    public Func<bool>? SharedAuthority { get; set; }

    /// <summary>Player +0x7c 의 회복 허용 표. 최초에는 모두0이며, 0이면 회복을 막는다. 장기 기록이 연결되면 읽지 않는다.</summary>
    public bool[] RecoveryAllowed { get; set; } = new bool[9];

    /// <summary>같은 Client의 실제 Player 9 기록. 사제 신전 DWORD의 0 아님을 허용 조건으로 읽으며, 허용 bool이나 SID 사본을 만들지 않는다.</summary>
    public byte[]? PlayerRecords { get; set; }

    /// <summary>HP·중립 회복의 원문 조회 위치에서 현재 권한을 읽는다.</summary>
    public bool GetAuthority() => SharedAuthority?.Invoke() ?? Authority;

    /// <summary>
    /// 원문 회복 분기에서 현재 신전 DWORD의 0 아님을 읽는다. 장기 기록이 없으면 기존 배열을 쓴다.
    /// PATCH는 stride0xb4/offset0x7c, CD는 stride0xac/offset0x74의 실제9기록을 읽는다.
    /// 근거: cpppj/src/o/RawPriestState.cpp RecoveryAllowed (R0-4 재대조, 2026-10-28).
    /// </summary>
    /// <param name="owner">소유자 번호(0~8).</param>
    public bool GetRecoveryAllowed(uint owner, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        if (owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "사제 HP 회복 Player 번호 오류");
        }
        if (PlayerRecords is null)
        {
            return RecoveryAllowed[owner];
        }
        int stride = edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac;
        int field = edition == OriginalEdition.Patch1078 ? 0x7c : 0x74;
        if (PlayerRecords.Length != 9 * stride)
        {
            throw new ArgumentException("사제 HP 회복 Player9 기록 폭 오류", nameof(PlayerRecords));
        }
        int offset = (int)owner * stride + field;
        uint temple = 0;
        // 리틀 엔디언 네 바이트만 읽으며 인접 Player 필드를 쓰지 않는다
        for (int i = 0; i < 4; i++)
        {
            temple |= (uint)PlayerRecords[offset + i] << (8 * i);
        }
        return temple != 0;
    }
}

/// <summary>
/// 사제·운반체 HP 변경의 공간 효과 경계(원본 PriestHitPointHooks).
/// </summary>
public sealed class PriestHitPointHooks
{
    /// <summary>가상 +0x48 공간 해제. HP가 절반 경계를 넘나드는 실제 공간 전환에서 부른다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>가상 +0x4c 공간 등록. HP 전환 뒤 현재 좌표를 보정하고 부른다.</summary>
    public Action<int, uint>? Repop { get; set; }
}

/// <summary>
/// 사제·운반체 HP 변경과 이동 불가 판정을 복원한다(원본 RawPriestState, 00492090).
/// HP 가 절반 경계를 넘나들 때만 공간 해제·등록을 수행하고, 좌표는 HP 전환 전용 배율로 거의 올림한다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPriestState.cpp (W03 삭제·수명 아홉째 조각, 2026-10-21).
/// </summary>
public sealed class RawPriestState
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 HP 필드의 위치.</summary>
    private const int HealthOffset = 26;

    /// <summary>슬롯 안 추가 비트 바이트의 판본별 위치(PATCH40/CD35).</summary>
    private int ExtraOffset => _pool.Edition == OriginalEdition.Patch1078 ? 40 : 35;

    /// <summary>사제 타입 번호(원본 158).</summary>
    private const byte PriestType = 158;

    /// <summary>사제 가상 표 값(10.78).</summary>
    private const uint PatchPriestVtable = 0x0050F210;

    /// <summary>CD 사제 가상 표이며 PATCH 표로 대신하지 않는다.</summary>
    private const uint CdPriestVtable = 0x005003e0;

    /// <summary>사제 부류 비트(원본 0x200000).</summary>
    private const uint PriestGenus = 0x200000;

    /// <summary>강제 이동 불가 비트(원본 extra 0x20).</summary>
    private const byte ForcedImmobile = 0x20;

    /// <summary>이동 가능한 지면의 spot 마스크(원본 6).</summary>
    private const byte SupportedGround = 6;

    /// <summary>free·contained 상태 마스크(원본 9).</summary>
    private const byte InvalidState = 9;

    /// <summary>거의 올림에 쓰는 실제 float 비트(원본 00500edc).</summary>
    private const float CoordinateBias = 0.9999f;

    /// <summary>HP 전환의 좌표 전용 배율(원본 00502310).</summary>
    private const float SnapBias = 0.99999f;

    /// <summary>지도 한 변의 칸 수.</summary>
    private const int Side = 256;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 최대 HP를 조회하는 실제 장치의 빌린 메서드다.</summary>
    private readonly Func<int, int> _maxHitPoints;

    /// <summary>타입 번호로 속성을 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>실제 PATCH 월드가 빌리는 주소 고정 현재 TYPE 표다. null은 생성 시 사본을 선택한다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;

    /// <summary>지면 genus/타입 범위 판정 시점의 현재 표를 읽는다.</summary>
    private IReadOnlyList<OriginalTypeRecord> CurrentTypes() => _currentTypes ?? _types;

    /// <summary>PATCH188칸의 현재 표를 빌리며 빈 표/null은 생성 시 사본으로 복귀한다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord>? types)
    {
        if (types is { Count: > 0 } && (_pool.Edition != OriginalEdition.Patch1078 || types.Count != 188))
            throw new ArgumentException("사제 상태 현재 TYPE 대여의 판본/길이 오류", nameof(types));
        _currentTypes = types is { Count: > 0 } ? types : null;
    }

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도.</summary>
    private readonly byte[] _spots;

    /// <summary>
    /// 사제 상태 몸체를 만든다. HP 조회 몸체의 풀과 타입 표·지도 크기를 확인한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀.</param>
    /// <param name="hp">현재 최대 HP를 조회하는 실제 장치의 빌린 메서드다. 같은 풀을 써야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    /// <param name="spots">칸별 점유 지도 65,536칸.</param>
    public RawPriestState(RawSidPool pool, RawHitPoints hp, IReadOnlyList<OriginalTypeRecord> types, byte[] spots)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(spots);
        if (!ReferenceEquals(hp.Pool, pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != Side * Side)
        {
            throw new ArgumentException("사제 상태 조회 풀·타입·지도 연결 오류", nameof(types));
        }
        _pool = pool;
        _maxHitPoints = hp.MaxHitPoints;
        _types = types.Select(type => type.Clone()).ToArray();
        _spots = spots;
    }

    /// <summary>사제 등록·상태가 같은 실제 보상 HP 장치를 공유하도록 연결한다.</summary>
    public RawPriestState(RawSidPool pool, RawSquidReward hp, IReadOnlyList<OriginalTypeRecord> types, byte[] spots)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(spots);
        if (!ReferenceEquals(hp.Pool, pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != Side * Side)
            throw new ArgumentException("사제 상태 조회 실제 보상HP·타입·지도 연결 오류", nameof(types));
        _pool = pool; _maxHitPoints = hp.MaxHitPoints; _types = types.Select(type => type.Clone()).ToArray(); _spots = spots;
    }

    /// <summary>회복 처리기의 연결 검사용 원본 풀을 반환한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>예약·풀 밖 번호와 자산이 아닌 슬롯을 보호한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Raw(int sid)
    {
        if (sid < 5 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "사제 상태 조회 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if ((raw[StateOffset] & InvalidState) != 0 || raw[TypeOffset] < OriginalConstructors.FirstAssetType
            || raw[TypeOffset] >= CurrentTypes().Count)
        {
            throw new InvalidOperationException("사제 상태 조회 raw 자산 오류");
        }
        return raw;
    }

    /// <summary>현재 HP를 PATCH signed DWORD/CD signed WORD로 읽고 인접 stale BYTE는 보존한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public int CurrentHitPoints(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        return _pool.Edition == OriginalEdition.Patch1078 ? unchecked((int)Read(raw, HealthOffset)) : unchecked((short)Read(raw, HealthOffset, 2));
    }

    /// <summary>사제 부류의 지면 이동 불가 판정(원본 00492090).</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public bool GroundImmobile(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        if ((CurrentTypes()[raw[TypeOffset]].Flags2 & PriestGenus) == 0)
        {
            return false;
        }
        // 저 HP 일 때에는 지면 좌표를 읽지 않는다.
        if (CurrentHitPoints(sid) < _maxHitPoints(sid) / 2)
        {
            return true;
        }
        int y = Cell(BitConverter.UInt32BitsToSingle(Read(raw, YOffset)));
        int x = Cell(BitConverter.UInt32BitsToSingle(Read(raw, XOffset)));
        return (_spots[y * Side + x] & SupportedGround) == 0 || (raw[ExtraOffset] & ForcedImmobile) != 0;
    }

    /// <summary>강제 이동 비트를 먼저 검사하는 이동 불가 판정.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public bool Immobile(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        return (raw[ExtraOffset] & ForcedImmobile) != 0 || GroundImmobile(sid);
    }

    /// <summary>
    /// HP 를 쓰고 절반 경계 전환이 있으면 공간 해제·등록을 수행한다.
    /// 권한이 없으면 경계값으로 되돌리고, 회복이 막혔으면 원래 HP 를 되살린다.
    /// 회복 허용은 장기 Player 기록이 있으면 그 신전 DWORD를, 없으면 기존 배열을 읽는다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="value">쓸 HP 값.</param>
    /// <param name="mode">권한·회복 허용 상태.</param>
    /// <param name="hooks">공간 효과 경계.</param>
    public void SetHitPoints(int sid, int value, PriestHitPointMode mode, PriestHitPointHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(hooks);
        ReadOnlySpan<byte> raw = Raw(sid);
        if (raw[TypeOffset] != PriestType || Read(raw, 0) != (_pool.Edition == OriginalEdition.Patch1078 ? PatchPriestVtable : CdPriestVtable))
        {
            throw new InvalidOperationException("사제 HP 변경 타입·가상 표 오류");
        }
        int oldHp = CurrentHitPoints(sid);
        int half = _maxHitPoints(sid) / 2;
        bool wasLow = oldHp < half;
        int stored = _pool.Edition == OriginalEdition.Patch1078 ? value : unchecked((short)value);
        bool transition = (stored < half) != wasLow;
        byte owner = raw[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
        // 실제로 참조하는 소유자 칸과 필요한 가상 효과만 쓰기 전에 검사한다.
        if (transition && mode.GetAuthority() && wasLow && owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), owner, "사제 HP 회복 소유자 오류");
        }
        bool space = transition && mode.GetAuthority() && (!wasLow || mode.GetRecoveryAllowed(owner, _pool.Edition));
        if (space && (hooks.Unpop is null || hooks.Repop is null))
        {
            throw new ArgumentException("사제 HP 변경 공간 효과 누락", nameof(hooks));
        }
        // 원본 setter 폭 밖의 stale 바이트는 그대로 보존한다.
        void Write(int hp)
        {
            Span<byte> target = _pool.AllocatedBytes(sid);
            // 실제 setter 폭만 쓰고 CD의 이웃 HP stale 두 바이트를 보존한다.
            for (int i = 0; i < (_pool.Edition == OriginalEdition.Patch1078 ? 4 : 2); i++)
            {
                target[HealthOffset + i] = (byte)((uint)hp >> (8 * i));
            }
        }
        Write(value);
        if (!transition)
        {
            return;
        }
        if (!mode.GetAuthority())
        {
            Write(half - (wasLow ? 1 : 0));
            return;
        }
        if (wasLow && !mode.GetRecoveryAllowed(owner, _pool.Edition))
        {
            Write(oldHp);
            return;
        }
        hooks.Unpop!(sid, 0);
        // 가상 효과 이후 HP·좌표를 다시 읽는다. 새 HP 가 저 HP 일 때만 현재 위치를 거의 올림한다.
        if (CurrentHitPoints(sid) < _maxHitPoints(sid) / 2)
        {
            Span<byte> target = _pool.AllocatedBytes(sid);
            foreach (int offset in new[] { XOffset, YOffset })
            {
                float coordinate = BitConverter.UInt32BitsToSingle(Read(target, offset));
                if (!float.IsFinite(coordinate) || coordinate < 0 || coordinate >= Side)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), coordinate, "사제 HP 전환 좌표 범위 오류");
                }
                uint bits = BitConverter.SingleToUInt32Bits((float)(int)((double)coordinate + (double)SnapBias));
                // HP 및 다른 좌표 필드 밖은 보존한다.
                for (int i = 0; i < 4; i++)
                {
                    target[offset + i] = (byte)(bits >> (8 * i));
                }
            }
        }
        hooks.Repop!(sid, 0);
    }

    /// <summary>거의 올림에 쓰는 실제 float 비트다.</summary>
    /// <param name="coordinate">원본 좌표.</param>
    private static int Cell(float coordinate)
    {
        double shifted = (double)coordinate + (double)CoordinateBias;
        if (!float.IsFinite(coordinate) || shifted < 0 || shifted >= Side)
        {
            throw new ArgumentOutOfRangeException(nameof(coordinate), coordinate, "사제 지면 조회 좌표 범위 오류");
        }
        return (int)shifted;
    }

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
