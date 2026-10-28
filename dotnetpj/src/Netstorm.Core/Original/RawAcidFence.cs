namespace Netstorm.Core.Original;

/// <summary>
/// 애시드 바리케이드(타입 145)의 Pop 후처리. 원본 0048c7a0의 최초 0.3초 예약 뒤 같은 Damageable 후처리를 실행한다.
/// 전용 주기·양축 이웃·13개 높이 영역 포획 몸체(RawAcidFence 클래스)는 세션 전역(동맹·친군사격·송신·Zap)을 빌려야 하므로 R4 세션 조립에서 이식한다.
/// 근거: cpppj/src/o/RawAcidFence.cpp PostPopAcidFence (R0-4 재대조, 2026-10-28).
/// </summary>
public static class RawAcidFencePostPop
{
    /// <summary>최초 추가 비트(원본 슬롯 +40의 bit0). 켜지면 최초가 아니므로 예약하지 않는다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>공통 Damageable 로 이어지기 전 비트 마스크(추가·매립). 켜진 비트가 하나라도 있으면 예약을 건너뛴다.</summary>
    private const byte SkipReserveMask = 9;

    /// <summary>최초 호출의 사건 번호.</summary>
    private const uint FirstEvent = 0;

    /// <summary>최초 호출의 예약 지연(초). 원본 0048c7a0의 0.3F다.</summary>
    private const float FirstDelay = 0.3f;

    /// <summary>
    /// 애시드 바리케이드 최초 후처리를 실행한다. 비추가·비매립의 최초 호출만 사건 0을 0.3초 뒤로 예약하고, 항상 같은 Damageable 후처리로 이어진다.
    /// 판본 검사는 호출자인 <see cref="RawAssetPostPop"/>이 10.78 타입 표(188개)로 이미 가리므로 여기서는 하지 않는다.
    /// </summary>
    /// <param name="pool">부모 슬롯을 읽는 풀.</param>
    /// <param name="sid">부모 슬롯 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    /// <param name="reserve">각각의 확보(new 0x28) 성공 여부. 최초 관문이 아니면 쓰지 않으므로 비어도 된다.</param>
    /// <param name="regular">확보한 사건의 실제 예약. 최초 관문이 아니면 쓰지 않으므로 비어도 된다.</param>
    /// <param name="damageable">같은 Damageable 후처리. 비어 있으면 안 된다.</param>
    public static void PostPop(RawSidPool pool, int sid, uint flags, Func<bool>? reserve, Action<int, uint, float>? regular, Action<int, uint> damageable)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(damageable);
        // 비추가·비매립의 최초 호출만 최초 예약을 건다. 그 밖에는 효과 뭉치를 건드리지 않는다
        if ((pool.Slot(sid)[ExtraOffset] & SkipReserveMask) == 0 && (flags & 1) != 0 && reserve is not null && reserve())
        {
            ArgumentNullException.ThrowIfNull(regular);
            regular(sid, FirstEvent, FirstDelay);
        }
        damageable(sid, flags);
    }
}
