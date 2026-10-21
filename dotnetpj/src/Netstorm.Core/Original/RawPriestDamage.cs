using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 플레이어 레벨 보정과 피격 권한 전역을 같은 월드에서 공유하는 상태(원본 PriestDamageState).
/// </summary>
public sealed class PriestDamageState
{
    /// <summary>원본 NoDamage 전역. 참이면 피격량을 0 으로 만든다.</summary>
    public bool Invulnerable { get; set; }

    /// <summary>HP 1 사제의 현재 의존 감시를 끊는 원본 권한이다.</summary>
    public bool Boss { get; set; } = true;

    /// <summary>원본 00594fa4 가 0 이 아니면 레벨 피해 보정을 생략한다.</summary>
    public bool LevelCorrectionDisabled { get; set; }

    /// <summary>소유자 번호별 현재 레벨(패치 Challenge rank·CD Player level).</summary>
    public int[] Levels { get; set; } = new int[40];
}

/// <summary>
/// 사제 피격의 현재 가상 프레임·HP·소유자·효과를 실제 월드에 연결하는 콜백 묶음(원본 PriestDamageHooks).
/// </summary>
public sealed class PriestDamageHooks
{
    /// <summary>현재 Regular 0x25d 를 찾는다. 없으면 미복원이다.</summary>
    public Func<int, bool>? HasRegenEvent { get; set; }

    /// <summary>0x25c·0x25d 의 모든 현재 Regular 를 같은 Host 에서 없앤다.</summary>
    public Action<int, uint>? CancelEvent { get; set; }

    /// <summary>현재 타입의 실제 defaultFrame 을 읽는다.</summary>
    public Func<int, int>? DefaultFrame { get; set; }

    /// <summary>기존 실제 SquidFrame.Set(대상,프레임,0) 을 실행한다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }

    /// <summary>같은 월드의 RawPriestState.SetHitPoints(공간 경계 전환 포함)다.</summary>
    public Action<int, int>? SetHitPoints { get; set; }

    /// <summary>현재 소유자에게 피격 사제·공격자 번호를 통지한다.</summary>
    public Action<byte, int, int>? OwnerDamageNotice { get; set; }

    /// <summary>실제 burning 판정을 수행한다.</summary>
    public Action<int>? DamageableRefresh { get; set; }
}

/// <summary>
/// 사제 HP 1 감시 쌍의 통지·제거를 담당하는 연결 지점(원본 DependFormHost).
/// 사제 HP 가 1 로 떨어질 때 감시 쌍을 끊을 때만 필요하다.
/// </summary>
public interface IPriestDependentHost
{
    /// <summary>감시되는 Form 에 통지한다.</summary>
    /// <param name="watched">감시되는 종속 번호.</param>
    /// <param name="watcher">감시자 번호.</param>
    void Notify(int watched, int watcher);

    /// <summary>감시되는 Form 의 감시자 번호를 찾는다.</summary>
    /// <param name="watched">감시되는 종속 번호.</param>
    /// <returns>감시자 번호.</returns>
    int Watcher(int watched);

    /// <summary>감시 쌍을 제거한다.</summary>
    /// <param name="watcher">감시자 번호.</param>
    /// <param name="watched">감시되는 번호.</param>
    /// <param name="flags">제거 플래그.</param>
    void Remove(int watcher, int watched, uint flags);
}

/// <summary>
/// 기본 피해 레벨 보정과 사제 피격 몸체(원본 RawPriestDamage, 0044bfe0 ↔ CD 00462280).
/// 패치 레벨 차 +20 / CD BYTE 레벨 차 +8 을 기준으로 정수 피해를 보정한다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPriestDamage.cpp (W03 삭제·수명 아홉째 조각, 2026-10-21).
/// </summary>
public sealed class RawPriestDamage
{
    /// <summary>패치판 Challenge rank 별 레벨 표(원본 0041c5b0).</summary>
    private static readonly int[] PatchRankLevels =
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100];

    /// <summary>패치판의 레벨 차 기준값.</summary>
    private const int PatchScale = 20;

    /// <summary>CD 판의 레벨 차 기준값.</summary>
    private const int CdScale = 8;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 HP 필드의 위치.</summary>
    private const int HealthOffset = 26;

    /// <summary>슬롯 안 종속 다음 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 프레임 번호의 위치(10.78, 4바이트).</summary>
    private const int FrameOffset = 36;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>사제의 held 방향을 판정하는 프레임 코드 표.</summary>
    private readonly IReadOnlyList<IReadOnlyList<FrameCode>> _frames;

    /// <summary>현재 NoDamage·권한·플레이어 레벨 전역.</summary>
    private readonly PriestDamageState _state;

    /// <summary>실제 하위 경계 묶음.</summary>
    private readonly PriestDamageHooks _hooks;

    /// <summary>HP 1 감시 쌍을 끊는 연결 지점. 없으면 감시 쌍이 있을 때 거부한다.</summary>
    private readonly IPriestDependentHost? _depend;

    /// <summary>
    /// 사제 피격 몸체를 만든다. 타입·프레임 표 크기를 확인한다.
    /// </summary>
    /// <param name="pool">피격 대상 슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="state">현재 NoDamage·권한·레벨 전역.</param>
    /// <param name="hooks">실제 하위 경계 묶음.</param>
    /// <param name="depend">HP 1 감시 쌍 연결 지점. 필요할 때만 연결한다.</param>
    public RawPriestDamage(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<IReadOnlyList<FrameCode>> frames, PriestDamageState state, PriestDamageHooks hooks,
        IPriestDependentHost? depend = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != OriginalConstructors.TypeCount || frames.Count != types.Count)
        {
            throw new ArgumentException("사제 피격 타입·프레임 표 크기 오류", nameof(types));
        }
        _pool = pool;
        _types = types;
        _frames = frames;
        _state = state;
        _hooks = hooks;
        _depend = depend;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 피해량을 현재 소유자 레벨 차로 보정한다. 공격자나 피해자가 0 이면 보정하지 않는다.
    /// </summary>
    /// <param name="damage">원본 피해량.</param>
    /// <param name="attackerOwner">공격자 소유자 번호.</param>
    /// <param name="victimOwner">피해자 소유자 번호.</param>
    /// <param name="state">현재 레벨·권한 전역.</param>
    /// <param name="patch">10.78 판인지.</param>
    /// <returns>보정된 피해량.</returns>
    public static int Adjust(int damage, uint attackerOwner, uint victimOwner, PriestDamageState state, bool patch = true)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (state.LevelCorrectionDisabled || attackerOwner == 0 || victimOwner == 0)
        {
            return damage;
        }
        if (!patch && (attackerOwner >= state.Levels.Length || victimOwner >= state.Levels.Length))
        {
            throw new ArgumentOutOfRangeException(nameof(attackerOwner), attackerOwner, "피해 레벨 범위 오류");
        }
        // 원본은 패치판이면 소유자 9 를 넘는 번호의 레벨을 0 으로 본다.
        int Level(uint owner)
        {
            if (patch && owner > 8)
            {
                return 0;
            }
            uint raw = (uint)state.Levels[owner];
            if (!patch)
            {
                return (int)raw;
            }
            if (raw >= PatchRankLevels.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(owner), raw, "Challenge rank 범위 오류");
            }
            return PatchRankLevels[raw];
        }
        int attacker = Level(attackerOwner);
        int victim = Level(victimOwner);
        int scale = patch ? PatchScale : CdScale;
        if (victim < attacker)
        {
            return Multiply(Difference(Difference(attacker, victim), -scale), damage) / scale;
        }
        if (attacker < victim)
        {
            int divisor = Difference(Difference(victim, attacker), -scale);
            if (divisor == 0)
            {
                throw new ArgumentOutOfRangeException(nameof(damage), "피해 레벨 차 0 오류");
            }
            int product = Multiply(damage, scale);
            if (product == int.MinValue && divisor == -1)
            {
                throw new ArgumentOutOfRangeException(nameof(damage), "피해 레벨 보정 overflow");
            }
            return product / divisor;
        }
        return damage;
    }

    /// <summary>현재 사제의 raw HP 를 10.78 의 signed DWORD 로 읽는다.</summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    private int Health(int victim) => unchecked((int)Read(_pool.Slot(victim), HealthOffset));

    /// <summary>
    /// 사제 피격을 수행한다. 회복 취소·HP 1 제한·소유자 통지·HP 1 감시 해제를 원본 순서로 처리한다.
    /// </summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    /// <param name="damage">피해량.</param>
    /// <param name="attackerOwner">공격자 소유자 번호.</param>
    /// <param name="attacker">공격자 슬롯 번호.</param>
    /// <returns>살아 있으면 참, HP 1 감시 해제면 거짓.</returns>
    public bool Hit(int victim, int damage, uint attackerOwner, int attacker)
    {
        if (victim <= 0 || victim >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(victim), victim, "사제 피격 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(victim);
        if ((raw[StateOffset] & 2) != 0)
        {
            throw new InvalidOperationException("사제 죽음 assert 경로");
        }
        if ((raw[StateOffset] & 1) != 0 || raw[TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("사제 live·타입 오류");
        }
        if (_hooks.HasRegenEvent is null || _hooks.SetHitPoints is null || _hooks.OwnerDamageNotice is null
            || _hooks.DamageableRefresh is null)
        {
            throw new InvalidOperationException("사제 피격 필수 하위 효과 누락");
        }
        if (_state.Invulnerable)
        {
            damage = 0;
        }
        bool cancel = _hooks.HasRegenEvent(victim);
        if (!cancel)
        {
            uint frame = Read(raw, FrameOffset);
            int direction = RawPathAnimation.Direction(_frames[raw[TypeOffset]], (int)frame);
            cancel = direction > 9 && direction < 15;
        }
        if (cancel)
        {
            if (_hooks.CancelEvent is null || _hooks.DefaultFrame is null || _hooks.SetFrame is null)
            {
                throw new InvalidOperationException("사제 회복 취소·프레임 하위 효과 누락");
            }
            _hooks.CancelEvent(victim, 0x25c);
            _hooks.CancelEvent(victim, 0x25d);
            _hooks.SetFrame(victim, _hooks.DefaultFrame(victim), 0);
        }
        damage = Adjust(damage, attackerOwner, _pool.Slot(victim)[OwnerOffset], _state);
        int next = unchecked((int)unchecked((uint)Health(victim) - (uint)damage));
        _hooks.SetHitPoints(victim, next < 1 ? 1 : next);
        _hooks.OwnerDamageNotice(_pool.Slot(victim)[OwnerOffset], victim, attacker);
        _hooks.DamageableRefresh(victim);
        if (_state.Boss && Health(victim) == 1)
        {
            DropDependents(victim);
            return false;
        }
        return true;
    }

    /// <summary>현재 watched 쪽 감시 Form 을 저장된 next 순서로 순회하고 통지·제거를 수행한다.</summary>
    /// <param name="victim">피해자 슬롯 번호.</param>
    private void DropDependents(int victim)
    {
        int current = (int)Read(_pool.Slot(victim), HeadOffset, 2);
        var visited = new HashSet<int>();
        // 감시 쌍이 있으면 저장된 next 순서로 제거한다.
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current))
            {
                throw new InvalidOperationException("사제 감시 쌍 체인 손상");
            }
            ReadOnlySpan<byte> raw = _pool.Slot(current);
            int next = (int)Read(raw, NextOffset, 2);
            if (raw[TypeOffset] == 1)
            {
                if (_depend is null)
                {
                    throw new InvalidOperationException("사제 HP 1 감시 쌍 연결이 없습니다");
                }
                _depend.Notify(current, victim);
                if ((_pool.Slot(current)[StateOffset] & 1) == 0)
                {
                    int watcher = _depend.Watcher(current);
                    _depend.Remove(watcher, victim, 0);
                }
            }
            current = next;
        }
    }

    /// <summary>signed DWORD 곱셈의 오버플로를 하위 32비트로 보존한다.</summary>
    /// <param name="first">첫 번째 값.</param>
    /// <param name="second">두 번째 값.</param>
    private static int Multiply(int first, int second) => unchecked((int)unchecked((uint)first * (uint)second));

    /// <summary>signed DWORD 뺄셈의 오버플로를 하위 32비트로 보존한다.</summary>
    /// <param name="first">첫 번째 값.</param>
    /// <param name="second">두 번째 값.</param>
    private static int Difference(int first, int second) => unchecked((int)unchecked((uint)first - (uint)second));

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
