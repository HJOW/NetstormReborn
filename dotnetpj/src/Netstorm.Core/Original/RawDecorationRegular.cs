// 장식·가장자리 팜의 단계별 HP 감소와 마지막 삭제를 원문 순서대로 공유한다.
namespace Netstorm.Core.Original;

// 장식 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.642에서 R4-4 정기 처리용으로 추가했다.

// 장식 정기가 요구하는 실제 후단 묶음이다.
public sealed class DecorationRegularHooks
{
    /// <summary>현재 TYPE·제단 역할의 최대 HP 조회다. 0.642에서 추가했다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }

    /// <summary>현재 가상98(amount, owner 0, attacker 0)의 실제 분배다. 0.642에서 추가했다.</summary>
    public Action<int, int, uint, uint>? HitPoints { get; set; }

    /// <summary>개수가 3 이상일 때 현재 가상10(flags 0)의 실제 삭제다. 서버만 호출한다. 0.642에서 추가했다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>나머지 사건의 실제 공통 분배다. 반환 payload를 그대로 전달한다. 0.642에서 추가했다.</summary>
    public RegularHandler? Damageable { get; set; }
}

/// <summary>장식 정기(원문 00462420)의 signed 개수·DWORD 곱 감김·0방향 나눗셈·현재 HP 읽기 순서를 유지한다. 난수를 소비하지 않는다.</summary>
/// <remarks>2026-11-19 0.642에서 추가했다. 근거: cpppj <c>o/RawDecorationRegular.cpp</c>의 <c>RunDecorationRegular</c>(원문 00462420)다.</remarks>
public static class RawDecorationRegular
{
    /// <summary>장식 가상표 2종(10.78)이다. 이 표의 슬롯만 처리한다. 0.642에서 추가했다.</summary>
    private static readonly uint[] DecorationVtables = [0x0050a820u, 0x0050a8e8u];

    /// <summary>슬롯 안 HP(signed DWORD) 필드의 위치다. 0.642에서 추가했다.</summary>
    private const int HealthOffset = 26;

    /// <summary>삭제·분배 경계의 개수 상한이다. 0.642에서 추가했다.</summary>
    private const uint DestroyCount = 3u;

    /// <summary>같은 풀의 장식 가상표인지 확인한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>장식 가상표이면 참이다.</returns>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        uint table = Read(pool.Slot(sid), 0);
        // 두 가상표 중 하나와 일치하는지 확인하는 반복문이다.
        for (int i = 0; i < DecorationVtables.Length; i++)
        {
            if (table == DecorationVtables[i])
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>장식 정기 사건을 실행한다. 사건 0의 단계별 HP 감소·삭제 뒤 나머지는 공통 분배로 넘긴다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="boss">현재 권한이다. 참일 때만 개수 3 이상에서 실제 삭제를 호출한다.</param>
    /// <param name="sid">처리할 장식 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다. signed로 비교하고 감김 곱에 쓴다.</param>
    /// <param name="payload">전달 값이다. 공통 분배로 그대로 넘긴다.</param>
    /// <param name="hooks">실제 효과 묶음이다.</param>
    /// <returns>삭제 시 -1, HP 감소·공통 분배 시 해당 반환 값이다.</returns>
    /// <exception cref="ArgumentException">풀 판본·가상 표 불일치다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 장치가 미연결이다.</exception>
    public static float Run(RawSidPool pool, bool boss, int sid, uint @event, uint count, float payload, DecorationRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("장식 정기는 PATCH10.78 본문이다", nameof(pool));
        }

        if (!Handles(pool, sid))
        {
            throw new ArgumentException("장식 정기 가상 표·타입 미복원", nameof(sid));
        }

        if (@event == 0)
        {
            // signed 개수 3 이상이면 서버만 실제 삭제를 호출한다. 비서버는 공통 분배로 떨어진다.
            if (unchecked((int)count) >= 3)
            {
                if (boss)
                {
                    DecorationNeed(hooks.Destroy)(sid, 0);
                    return -1.0f;
                }
            }
            else
            {
                // 최대 HP의 실제 효과 뒤 현재 HP를 읽는다. 곱은 unsigned 감김 뒤 signed 재해석이며 나눗셈은 0방향 절삭이다.
                int maximum = DecorationNeed(hooks.MaxHitPoints)(sid);
                int product = unchecked((int)unchecked((uint)maximum * (DestroyCount - count)));
                int threshold = product / 3;
                int current = unchecked((int)Read(pool.Slot(sid), HealthOffset));
                if (threshold < current)
                {
                    DecorationNeed(hooks.HitPoints)(sid, unchecked(current - threshold), 0, 0);
                }
            }
        }

        return DecorationNeed(hooks.Damageable)(sid, @event, count, payload);
    }

    /// <summary>정렬되지 않은 little endian 값을 현재 슬롯에서 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 현재 필드의 지정 바이트만 조합하는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)raw[offset + i] << (i * 8);
        }

        return value;
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다.</summary>
    private static T DecorationNeed<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("장식 정기의 선택된 실제 하위 미연결");
}
