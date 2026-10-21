namespace Netstorm.Core.Original;

/// <summary>
/// 실제 NoDamage·레벨 상태와 일반 피격의 서버·경고·사망 전역(원본 DamageableHitState).
/// </summary>
public sealed class DamageableHitState
{
    /// <summary>사제와 같은 실제 Challenge rank·CD Player level·NoDamage 자료를 공유한다.</summary>
    public PriestDamageState Levels { get; set; } = new();

    /// <summary>원본 사망 권한.</summary>
    public bool Server { get; set; } = true;

    /// <summary>패치 held 피격 진단 전역.</summary>
    public bool Checking { get; set; }

    /// <summary>원본 현재 localPlayerId 바이트다.</summary>
    public byte LocalOwner { get; set; } = 1;

    /// <summary>Player +0x78 의 현재 주 섬 번호다.</summary>
    public int[] PrimaryIslands { get; set; } = new int[40];

    /// <summary>원본 실제 시계 기준 경고 마감 시각.</summary>
    public double WarningTime { get; set; }

    /// <summary>float 30초 간격의 경고 간격.</summary>
    public float WarningDelay { get; set; } = 30;

    /// <summary>원본 서버 사망 직전에 기록하는 피해자 번호.</summary>
    public int KilledVictim { get; set; }

    /// <summary>원본 서버 사망 직전에 기록하는 공격자 소유자 번호.</summary>
    public uint KillerOwner { get; set; }

    /// <summary>세션의 현재 서버 저장소를 읽는다. 미지정 시 <see cref="Server"/> 를 쓴다.</summary>
    public Func<bool>? ServerRead { get; set; }

    /// <summary>세션의 현재 localPlayerId 를 읽는다. 미지정 시 <see cref="LocalOwner"/> 를 쓴다.</summary>
    public Func<byte>? LocalOwnerRead { get; set; }

    /// <summary>세션의 현재 주 섬 번호를 읽는다. 미지정 시 <see cref="PrimaryIslands"/> 를 쓴다.</summary>
    public Func<byte, int>? PrimaryIslandRead { get; set; }
}

/// <summary>
/// 실제 가상 방어·삭제·소유자 통지·공간·경고 UI 를 같은 월드에 연결하는 콜백 묶음(원본 DamageableHitHooks).
/// </summary>
public sealed class DamageableHitHooks
{
    /// <summary>현재 표적의 가상 +0x9c 공격자 판정. 참이면 HP를 쓰지 않는다.</summary>
    public Func<int, int, bool>? RejectDamage { get; set; }

    /// <summary>원본 가상 +0x10 을 같은 실제 삭제기로 수행한다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>HP 쓰기 뒤 현재 소유자에게 표적·공격자 번호를 통지한다.</summary>
    public Action<byte, int, int>? OwnerDamageNotice { get; set; }

    /// <summary>기존 실제 RawDamageableBurning.Refresh 몸체다.</summary>
    public Action<int>? Refresh { get; set; }

    /// <summary>표적 raw extra bit 4 의 가상 +0x88 공간 재평가다.</summary>
    public Action<int>? Unrealize { get; set; }

    /// <summary>현재 raw 좌표가 놓인 실제 지면 자산 번호를 조회한다.</summary>
    public Func<float, float, int>? IslandAt { get; set; }

    /// <summary>gameTime 과 구별하는 원본 벽 시계의 현재 초를 돌려준다.</summary>
    public Func<double>? RealTime { get; set; }

    /// <summary>패치판만 event 12 의 원본 경고 표시 요청이다.</summary>
    public Action? WarningEvent { get; set; }

    /// <summary>현재 islandUnderAttack.wav 요청이다.</summary>
    public Action? WarningSound { get; set; }

    /// <summary>패치판만 형식화 뒤 islandUnderAttack 원본 알림을 제출한다.</summary>
    public Action? WarningText { get; set; }
}

/// <summary>
/// 일반 자산의 BaseHit·DamageableHit 와 HP·사망·섬 경고 순서를 복원한다(원본 RawDamageableHit, 004add10 ↔ CD 004aeda0).
/// 순서는 원본 그대로 "HP 보유 판정 → 섬 경고 → unrealize → BaseHit(방어·레벨 보정·HP 쓰기·소유자 통지) → burning"이다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawDamageableHit.cpp (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageableHit
{
    /// <summary>슬롯 안 HP 필드의 위치.</summary>
    private const int HealthOffset = 26;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 섬 번호 WORD 의 위치(10.78).</summary>
    private const int IslandOffset = 8;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>타입 플래그2의 제단(dais) 비트.</summary>
    private const uint DaisKind = 0x400000;

    /// <summary>타입 플래그1의 HP 보유 비트.</summary>
    private const uint HasHitPointBit = 0x10;

    /// <summary>타입 플래그2의 섬 비트.</summary>
    private const uint Island = 2;

    /// <summary>타입 플래그2의 3×3 섬 비트.</summary>
    private const uint Island3x3 = 0x1000000;

    /// <summary>타입 플래그2의 다리 비트.</summary>
    private const uint Bridge = 4;

    /// <summary>공격자 타입 플래그2의 유닛 공격 비트(원본 0x100).</summary>
    private const uint AttackerUnit = 0x100;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 원본 전역.</summary>
    private readonly DamageableHitState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly DamageableHitHooks _hooks;

    /// <summary>
    /// 일반 피해 몸체를 만든다. 타입 표 크기를 확인한다.
    /// </summary>
    /// <param name="pool">피격 대상 슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="state">현재 원본 전역.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawDamageableHit(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, DamageableHitState state,
        DamageableHitHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>현재 raw 유효 범위를 검사하며 실제 free·dead 분기를 대체하지 않는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Slot(int sid)
    {
        if (sid <= 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "일반 피해 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), raw[TypeOffset], "일반 피해 타입 오류");
        }
        return raw;
    }

    /// <summary>현재 HP 를 패치판 signed DWORD 로 읽는다.</summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    private int Health(int victim) => unchecked((int)Read(Slot(victim), HealthOffset));

    /// <summary>제단 genus 는 extra bit 0 이며 다른 타입은 실제 HP 보유 비트다.</summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    private bool HasHitPoints(int victim)
    {
        ReadOnlySpan<byte> raw = Slot(victim);
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        return (type.Flags2 & DaisKind) != 0
            ? (raw[ExtraOffset] & 1) != 0
            : (type.Flags1 & HasHitPointBit) != 0;
    }

    /// <summary>현재 실제 주 섬에 대한 경고만 표시하고 원본 double 시계 + float 간격의 저장 순서를 유지한다.</summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    private void WarnOwner(int victim)
    {
        ReadOnlySpan<byte> raw = Slot(victim);
        byte owner = raw[OwnerOffset];
        if (owner != (_state.LocalOwnerRead is not null ? _state.LocalOwnerRead() : _state.LocalOwner))
        {
            return;
        }
        if (owner >= _state.PrimaryIslands.Length || _hooks.IslandAt is null)
        {
            throw new InvalidOperationException("일반 피해 실제 주 섬 조회 경계 누락");
        }
        int island = _hooks.IslandAt(
            BitConverter.UInt32BitsToSingle(Read(raw, XOffset)),
            BitConverter.UInt32BitsToSingle(Read(raw, YOffset)));
        if (island == 0)
        {
            return;
        }
        ReadOnlySpan<byte> ground = Slot(island);
        uint genus = _types[ground[TypeOffset]].Flags2;
        if ((genus & Island) == 0 || (genus & Island3x3) != 0)
        {
            return;
        }
        int islandId = (genus & Bridge) != 0
            ? -1
            : unchecked((int)(ushort)Read(ground, IslandOffset, 2));
        int primary = _state.PrimaryIslandRead is not null
            ? _state.PrimaryIslandRead(owner)
            : _state.PrimaryIslands[owner];
        if (islandId != primary)
        {
            return;
        }
        if (_hooks.RealTime is null)
        {
            throw new InvalidOperationException("일반 피해 실제 경고 시계 누락");
        }
        if (_state.WarningTime <= _hooks.RealTime())
        {
            if (_hooks.WarningSound is null || (_hooks.WarningEvent is null || _hooks.WarningText is null))
            {
                throw new InvalidOperationException("일반 피해 실제 섬 피격 알림 경계 누락");
            }
            _hooks.WarningEvent?.Invoke();
            _hooks.WarningSound();
            _hooks.WarningText?.Invoke();
        }
        double now = _hooks.RealTime();
        _state.WarningTime = now + (double)_state.WarningDelay;
    }

    /// <summary>
    /// 실제 서버 사망은 공통 삭제에 원본 owner 마스크·유닛 공격 플래그를 전달한다.
    /// </summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    /// <param name="damage">피해량.</param>
    /// <param name="attackerOwner">공격자 소유자 번호.</param>
    /// <param name="attacker">공격자 슬롯 번호.</param>
    /// <returns>살아 있으면 참, 사망했으면 거짓.</returns>
    public bool BaseHit(int victim, int damage, uint attackerOwner, int attacker)
    {
        ReadOnlySpan<byte> raw = Slot(victim);
        if ((raw[StateOffset] & 2) != 0)
        {
            throw new InvalidOperationException("일반 피해 죽음 assert 경로");
        }
        if ((raw[StateOffset] & 3) != 0)
        {
            return true;
        }
        byte extra = raw[ExtraOffset];
        if ((extra & 9) != 0)
        {
            if (_state.Checking)
            {
                throw new InvalidOperationException("일반 피해 held assert 경로");
            }
            return true;
        }
        if (_hooks.RejectDamage is null)
        {
            throw new InvalidOperationException("일반 피해 실제 가상 방어 판정 누락");
        }
        if (_state.Levels.Invulnerable)
        {
            damage = 0;
        }
        if (_hooks.RejectDamage(victim, attacker))
        {
            return true;
        }
        byte owner = Slot(victim)[OwnerOffset];
        damage = RawPriestDamage.Adjust(damage, attackerOwner, owner, _state.Levels);
        int next = Subtract(Health(victim), damage);
        bool server = _state.ServerRead is not null ? _state.ServerRead() : _state.Server;
        if (next < 1 && server)
        {
            if (_hooks.Destroy is null)
            {
                throw new InvalidOperationException("일반 피해 실제 서버 삭제 경계 누락");
            }
            _state.KilledVictim = victim;
            _state.KillerOwner = attackerOwner;
            uint ownerMask = attackerOwner != 0 ? attackerOwner << 16 : 0u;
            uint flags = ownerMask | 0x100000u;
            if (attacker != 0 && (_types[Slot(attacker)[TypeOffset]].Flags2 & AttackerUnit) != 0)
            {
                flags = ownerMask | 0x4100000u;
            }
            _hooks.Destroy(victim, flags);
            return false;
        }
        if (_hooks.OwnerDamageNotice is null)
        {
            throw new InvalidOperationException("일반 피해 실제 현재 소유자 통지 누락");
        }
        RawSquidHitPoints.Set(_pool, victim, next < 1 ? 1 : next);
        _hooks.OwnerDamageNotice(Slot(victim)[OwnerOffset], victim, attacker);
        return true;
    }

    /// <summary>
    /// 실제 Damageable 의 HP 보유·주 섬·unrealize·기본 피해·burning 순서를 수행한다.
    /// </summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    /// <param name="damage">피해량.</param>
    /// <param name="attackerOwner">공격자 소유자 번호.</param>
    /// <param name="attacker">공격자 슬롯 번호.</param>
    /// <returns>살아 있으면 참, 사망했으면 거짓.</returns>
    public bool Hit(int victim, int damage, uint attackerOwner, int attacker)
    {
        if (!HasHitPoints(victim))
        {
            return false;
        }
        WarnOwner(victim);
        ReadOnlySpan<byte> raw = Slot(victim);
        if ((raw[ExtraOffset] & 4) != 0)
        {
            if (_hooks.Unrealize is null)
            {
                throw new InvalidOperationException("일반 피해 실제 unrealize 경계 누락");
            }
            _hooks.Unrealize(victim);
        }
        bool alive = BaseHit(victim, damage, attackerOwner, attacker);
        if (alive)
        {
            if (_hooks.Refresh is null)
            {
                throw new InvalidOperationException("일반 피해 실제 burning 몸체 누락");
            }
            _hooks.Refresh(victim);
        }
        return alive;
    }

    /// <summary>원본 signed overflow 를 하위 DWORD 로 보존한다.</summary>
    /// <param name="value">현재 HP.</param>
    /// <param name="damage">피해량.</param>
    private static int Subtract(int value, int damage) => unchecked((int)unchecked((uint)value - (uint)damage));

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint result = 0;
        // 요청한 원본 필드 폭만 낮은 바이트부터 조합한다.
        for (int i = 0; i < width; i++)
        {
            result |= (uint)bytes[offset + i] << (i * 8);
        }
        return result;
    }
}
