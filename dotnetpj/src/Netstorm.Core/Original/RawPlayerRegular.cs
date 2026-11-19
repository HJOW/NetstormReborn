// 실제 Player152 가상+5c의 두 소어 루저 검사 사건을 현재 두 owner·실제 Regular Host에 연결한다.
namespace Netstorm.Core.Original;

// Player 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.643에서 R4-4 정기 처리용으로 추가했다.

// 알 수 없는 사건에서 진단 뒤 payload를 반환하는 원문 분류다.
public enum PlayerRegularAssertion
{
    /// <summary>로컬·개수 조건인데 사건이 1·2가 아니면 보고한다. 0.643에서 추가했다.</summary>
    UnknownEvent,
}

// 실제 로그·새예약·Template State·변경통지를 필수 하위 효과로 연결한다.
public sealed class PlayerRegularHooks
{
    /// <summary>진단 뒤 현재 payload 반환을 계속 수행한다. 0.643에서 추가했다.</summary>
    public Action<PlayerRegularAssertion>? Assertion { get; set; }

    /// <summary>그룹 29의 원본 문자열을 순서대로 전달한다. 0.643에서 추가했다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>새 Regular 할당(new 40) 성공·실패를 보존한다. 0.643에서 추가했다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>실제 같은 Host가 사건 2·payload 60·현재 시각·개수 0을 만든다. 0.643에서 추가했다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>현재 Template의 실제 State를 빌린다. 0.643에서 추가했다.</summary>
    public Func<uint, Memory<byte>>? TemplateState { get; set; }

    /// <summary>State 쓰기 뒤 다시 읽은 현재 Template의 변경을 통지한다. 0.643에서 추가했다.</summary>
    public Action<uint>? TemplateChanged { get; set; }

    /// <summary>Native의 Get·반환 span 쓰기·현재 owner 통지 전체의 실제 수명을 보유한다. 독립 typed는 null이다. 0.643에서 추가했다.</summary>
    public Func<IDisposable?>? TemplatePin { get; set; }
}

/// <summary>PATCH 00490810·CD 00407ef0의 로컬·개수 조건→사건 1 로그·정기 2 또는 사건 2 State 갱신→반환을 같은 순서로 실행한다. 난수를 소비하지 않는다.</summary>
/// <remarks>2026-11-19 0.643에서 추가했다. 근거: cpppj <c>o/RawPlayerRegular.cpp</c>의 <c>RunPlayerRegular</c>다. 사건 1·개수 0의 자연 반환은 원래 payload를 그대로 돌려준다.</remarks>
public static class RawPlayerRegular
{
    /// <summary>실제 Player 생성자 가상표(PATCH)다. 0.643에서 추가했다.</summary>
    private const uint PatchPlayerVtable = 0x0050ee08u;

    /// <summary>실제 Player 생성자 가상표(CD)다. 0.643에서 추가했다.</summary>
    private const uint CdPlayerVtable = 0x005002a8u;

    /// <summary>원본 로그 그룹이다. 0.643에서 추가했다.</summary>
    private const uint LogGroup = 29u;

    /// <summary>소어 루저 확인의 첫 사건이다. 0.643에서 추가했다.</summary>
    private const uint FirstEvent = 1u;

    /// <summary>소어 루저 확인의 둘째 사건이다. 0.643에서 추가했다.</summary>
    private const uint SecondEvent = 2u;

    /// <summary>새 정기 2의 payload다. 최초 지연이 아니다. 0.643에서 추가했다.</summary>
    private const float SecondPayload = 60.0f;

    /// <summary>둘째 사건이 State에 세우는 비트다. 0.643에서 추가했다.</summary>
    private const byte TemplateMark = 4;

    /// <summary>같은 풀의 실제 Player 가상표인지 확인한다. 다른 타입의 처리기를 기본 본문으로 덮지 않는다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>Player 가상표이면 참이다.</returns>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        uint table = Read(pool.Slot(sid), 0);
        return table == (pool.Edition == OriginalEdition.Patch1078 ? PatchPlayerVtable : CdPlayerVtable);
    }

    /// <summary>Player 정기 사건을 실행한다. 로컬 소유자가 아니면 개수와 무관하게 원래 payload를 돌려준다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="battleOwner">현재 로컬(전투) 소유자다.</param>
    /// <param name="templateOwner">저장 Template 선택용 별도 owner다. 둘째 사건의 State 조회에 쓴다.</param>
    /// <param name="battle">실제 전투 모드다. 로그 이후에도 다시 읽은 값을 쓴다.</param>
    /// <param name="sid">처리할 Player 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다. 0이면 효과 없이 원래 payload를 돌려준다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="hooks">실제 효과 묶음이다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <exception cref="ArgumentException">Player 가상 표 불일치다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 장치가 미연결이다.</exception>
    public static float Run(RawSidPool pool, uint battleOwner, uint templateOwner, bool battle, int sid, uint @event, uint count, float payload, PlayerRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!Handles(pool, sid))
        {
            throw new ArgumentException("Player 정기 가상 Handler 표 미복원", nameof(sid));
        }

        if (hooks.Assertion is null || hooks.Log is null || hooks.ReserveRegular is null
            || hooks.AddRegular is null || hooks.TemplateState is null || hooks.TemplateChanged is null)
        {
            throw new InvalidOperationException("Player 정기 실제 로그·예약·현재 Template 효과 누락");
        }

        ReadOnlySpan<byte> raw = pool.Slot(sid);
        byte owner = raw[pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
        if (owner != battleOwner || count == 0)
        {
            return payload;
        }

        if (@event == FirstEvent)
        {
            hooks.Log(LogGroup, "SORE LOSER TEST: ");
            if (battle)
            {
                hooks.Log(LogGroup, "STARTING.\n");
                if (hooks.ReserveRegular())
                {
                    hooks.AddRegular(sid, SecondEvent, SecondPayload);
                }

                return 0.0f;
            }

            hooks.Log(LogGroup, "NOT STARTING YET.\n");
            return payload;
        }

        if (@event == SecondEvent)
        {
            if (battle)
            {
                // Native의 실제 owner 수명은 반환 span 쓰기와 현재 owner 통지까지 유지한다. 독립 typed는 외곽 수명을 쓴다.
                using IDisposable? hold = hooks.TemplatePin?.Invoke();
                Memory<byte> bytes = hooks.TemplateState(templateOwner);
                if (bytes.IsEmpty)
                {
                    throw new ArgumentOutOfRangeException(nameof(hooks), "Player 실제 Template State 바이트 누락");
                }

                bytes.Span[0] |= TemplateMark;
                hooks.TemplateChanged(templateOwner);
            }

            return 0.0f;
        }

        hooks.Assertion(PlayerRegularAssertion.UnknownEvent);
        return payload;
    }

    /// <summary>정렬되지 않은 little endian 값을 현재 슬롯에서 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 현재 필드의 네 바이트를 조합하는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)raw[offset + i] << (i * 8);
        }

        return value;
    }
}
