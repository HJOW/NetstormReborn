// 공통 Activate의 현재 권한·void 해제·가상 후처리·같은 깊이를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>원본004ac560이 도달 시점에 선택하는 현재 효과다. 실행 중 장치 교체를 허용한다.</summary>
public sealed class SquidActivateHooks
{
    /// <summary>void·전송중 진단 뒤 현재 서버 권한을 읽는다.</summary>
    public Func<bool>? Server { get; set; }
    /// <summary>선택된 flags로 실제 송신 경계를 실행한다.</summary>
    public Action<int, uint>? Transmit { get; set; }
    /// <summary>깊이를 증가시킨 뒤 현재 가상 후처리를 실행한다.</summary>
    public Action<int, uint>? PostPop { get; set; }
    /// <summary>checking과 무관한 원문 진단이며 정상 복귀할 수 있다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>원본10.78의 공통 Activate 전체 부모다. 부분 변경은 진단·효과 실패에서도 남긴다.</summary>
public static class RawSquidActivate
{
    /// <summary>원문 진단 파일의 동일한 경로다.</summary>
    private const string Source = @"\Ns\O\Squid.cpp";

    /// <summary>도달한 대리자만 값으로 선택해 자기 연결 교체 중에도 현재 호출 수명을 보존한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("Activate의 선택된 실제 장치 미연결");

    /// <summary>같은 raw·참조 깊이에 void→현재 권한/송신→가상 후처리→현재 깊이 진단을 적용한다.</summary>
    /// <remarks>근거: cpppj/src/o/RawSquidActivate.h/.cpp. CD는 별도 기존 몸체를 유지한다.</remarks>
    public static void Run(RawSidPool pool, int sid, uint flags, ref uint depth, SquidActivateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("현재 Activate는10.78 전용");
        if (sid == 0 || (uint)sid >= (uint)pool.Capacity) Need(hooks.Assertion)("isValid()", Source, 0x35b);
        Span<byte> raw = pool.RawSlot(sid);
        if ((raw[11] & 4) == 0) Need(hooks.Assertion)("isVoid()", Source, 0x35c);
        byte state = raw[11]; raw[11] &= 0xfb;
        if ((state & 0x10) != 0) Need(hooks.Assertion)("!xmitJustCreatedFlag", Source, 0x35f);
        if (Need(hooks.Server)())
        {
            if (sid >= 5 && sid < 15000)
            {
                if ((flags & 8) != 0) Need(hooks.Assertion)("!(popFlags & pfSERVER_POP)", Source, 0x364);
                flags |= 0x10;
            }
            else if ((flags & 0x10) == 0) flags |= 8;
            if ((flags & 0x50) == 0)
            {
                if ((flags & 7) != 0) raw[11] |= 0x10;
                Need(hooks.Transmit)(sid, flags); raw[11] &= 0xef;
            }
        }
        else if ((flags & 8) == 0) flags |= 0x10;
        uint before = depth; depth = unchecked(depth + 1);
        Need(hooks.PostPop)(sid, flags);
        if (depth != before) Need(hooks.Assertion)("curPostPop == basePostPop", Source, 0x392);
    }
}
