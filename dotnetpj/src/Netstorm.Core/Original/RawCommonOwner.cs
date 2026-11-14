// 공통 소유자 지정의 현재 모드/TYPE 재조회와 정상 복귀 진단 뒤 부분 효과를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>공통 Owner가 소비 위치에서만 요구하는 실제 함수 객체다.</summary>
public sealed class CommonOwnerHooks
{
    /// <summary>현재 편집 모드를 읽는다.</summary>
    public Func<bool>? Fort { get; set; }
    /// <summary>현재 전투 모드를 읽는다.</summary>
    public Func<bool>? Battle { get; set; }
    /// <summary>현재 도전 모드를 읽는다.</summary>
    public Func<bool>? Challenge { get; set; }
    /// <summary>그 위치의 현재 TYPE BYTE로 현재 genus를 읽는다.</summary>
    public Func<byte, uint>? Genus { get; set; }
    /// <summary>현재 소유자BYTE의 실제 목록에서 SID를 뺀다.</summary>
    public Action<byte, int>? Remove { get; set; }
    /// <summary>현재 소유자BYTE의 실제 목록에 SID를 더한다.</summary>
    public Action<byte, int>? Append { get; set; }
    /// <summary>식/파일/행을 진단하고 정상 복귀하면 원래 요청을 계속 적용한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>004adf00/CD004aefa0의 범위 진단→현재 제거→BYTE 쓰기→현재 추가 몸체다.</summary>
public static class RawCommonOwner
{
    /// <summary>선택된 함수 객체만 요구하며 값 복사한 대리자를 호출 동안 유지한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("공통 소유자의 선택된 실제 장치 미연결");
    /// <summary>전체 TYPE/목록/free 사전 검사를 추가하지 않고 원문 소비 순서를 실행한다.</summary>
    public static void SetCommonSquidOwner(RawSidPool pool, int sid, uint player, CommonOwnerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks); bool patch = pool.Edition == OriginalEdition.Patch1078;
        // 진단 장치가 변경되거나 정상 복귀해도 요청DWORD와 판본별 원문 행을 보존한다.
        void Diagnose(string expression, uint patchLine, uint cdLine) => Need(hooks.Assertion)(expression, patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", patch ? patchLine : cdLine);
        if (Need(hooks.Challenge)())
        {
            if (player == 0 || player >= 0x28) Diagnose("0 < newPlayerId && newPlayerId < CL_NUM_CHAL_PLAYERS", 0x9ff, 0x9be);
        }
        else if (player >= 9) Diagnose("newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)", 0xa02, 0x9c1);
        Span<byte> raw = pool.RawSlot(sid); int owner = patch ? 34 : 32, extra = patch ? 40 : 35;
        if ((Need(hooks.Fort)() || Need(hooks.Battle)()) && (Need(hooks.Genus)(raw[10]) & 0x4200) != 0) Need(hooks.Remove)(raw[owner], sid);
        bool cdActive = false;
        if (!patch) { bool battle = Need(hooks.Battle)(); bool fort = Need(hooks.Fort)(); cdActive = fort || battle; }
        raw[owner] = unchecked((byte)player);
        bool active = patch ? Need(hooks.Fort)() || Need(hooks.Battle)() : cdActive;
        if (active && (Need(hooks.Genus)(raw[10]) & 0x4200) != 0 && (raw[11] & 4) == 0 && (raw[extra] & 8) == 0) Need(hooks.Append)(raw[owner], sid);
    }
}
