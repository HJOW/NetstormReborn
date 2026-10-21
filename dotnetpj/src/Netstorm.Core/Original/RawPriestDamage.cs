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
/// 피해량 레벨 보정(원본 RawPriestDamage::Adjust, 0044bfe0 ↔ CD 00462280).
/// 패치는 레벨 차 +20, CD 는 레벨 차 +8 을 기준으로 정수 피해를 보정한다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPriestDamage.cpp (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public static class RawPriestDamage
{
    /// <summary>패치판 Challenge rank 별 레벨 표(원본 0041c5b0).</summary>
    private static readonly int[] PatchRankLevels =
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100];

    /// <summary>패치판의 레벨 차 기준값.</summary>
    private const int PatchScale = 20;

    /// <summary>CD 판의 레벨 차 기준값.</summary>
    private const int CdScale = 8;

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

    /// <summary>signed DWORD 곱셈의 오버플로를 하위 32비트로 보존한다.</summary>
    /// <param name="first">첫 번째 값.</param>
    /// <param name="second">두 번째 값.</param>
    private static int Multiply(int first, int second) => unchecked((int)unchecked((uint)first * (uint)second));

    /// <summary>signed DWORD 뺄셈의 오버플로를 하위 32비트로 보존한다.</summary>
    /// <param name="first">첫 번째 값.</param>
    /// <param name="second">두 번째 값.</param>
    private static int Difference(int first, int second) => unchecked((int)unchecked((uint)first - (uint)second));
}
