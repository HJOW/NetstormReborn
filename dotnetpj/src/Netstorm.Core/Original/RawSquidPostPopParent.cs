// 원문 공통 후처리의 현재 조회·저장 genus·외부 효과 순서를 분배한다.
namespace Netstorm.Core.Original;

/// <summary>도달한 실제 하위만 요구하는 공통004b0d30 장치다. 연결 자체는 조회/쓰기를 실행하지 않는다.</summary>
public sealed class SquidPostPopParentHooks
{
    /// <summary>raw/TYPE보다 먼저 읽는 현재 메타맵 억제 관문이다.</summary>
    public Func<bool>? Suppressed { get; set; }
    /// <summary>각 원문 위치에서 읽는 현재 Graph 활성이다.</summary>
    public Func<bool>? GraphsEnabled { get; set; }
    /// <summary>현재 raw TYPE 바이트로 같은 현재 레코드를 조회한다.</summary>
    public Func<byte, OriginalTypeRecord>? Type { get; set; }
    /// <summary>현재 영역 무효화다.</summary>
    public Action<int>? Region { get; set; }
    /// <summary>현재 표면 Graph 추가다.</summary>
    public Action<int>? Add { get; set; }
    /// <summary>현재 invalidGraph와 실제 Graph setter를 사용하는 번호 리셋이다.</summary>
    public Action<int>? ResetGraph { get; set; }
    /// <summary>checking과 무관한80b 단언의 정상/예외 복귀다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>현재 타입으로 비용을 가산한다.</summary>
    public Action<int>? Cost { get; set; }
    /// <summary>현재 공급원 등록이다.</summary>
    public Action<int>? Provider { get; set; }
    /// <summary>저장 genus가 선택한 전역 워크샾 등록이다.</summary>
    public Action<int>? Factory { get; set; }
    /// <summary>호출 직전 현재 owner의 워크샾 목록 등록이다.</summary>
    public Action<uint, int>? OwnerFactory { get; set; }
    /// <summary>호출 직전 현재 owner의 실제 AI 통지다.</summary>
    public Action<uint, int>? AiNotice { get; set; }
    /// <summary>AI 통지 뒤 현재 전체 로컬 owner를 읽는다.</summary>
    public Func<uint>? LocalOwner { get; set; }
    /// <summary>호출 직전 현재 TYPE의 두 로컬 통계다.</summary>
    public Action<uint>? LocalCount { get; set; }
    /// <summary>호출 직전 현재 TYPE의 전역 통계다.</summary>
    public Action<uint>? GlobalCount { get; set; }
    /// <summary>장부 뒤/깊이 감소 전의 실제 선택 재검사다.</summary>
    public Action? RecheckSelection { get; set; }
}

/// <summary>cpppj/src/o/RawSquidPostPopParent.cpp/.h의 공통 현재 부모다. 가상 분배와 깊이 증가는 호출자가 맡는다.</summary>
public static class RawSquidPostPopParent
{
    /// <summary>원문 영역 통지의 건물군/bit8 마스크다.</summary>
    private const uint RegionMask = 0x50444208;
    /// <summary>원문 Graph 억제 플래그다.</summary>
    private const uint NoGraph = 0x2000000;

    /// <summary>선택된 callable을 현재 호출 끝까지 보관하며 미도달 장치는 요구하지 않는다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("공통 후처리의 선택된 실제 장치 미연결");

    /// <summary>현재 조회/저장 genus/단언 복귀/장부 효과와 깊이 한 번 감소를 원문 순서로 실행한다.</summary>
    public static void Run(RawSidPool pool, int sid, uint flags, ref uint depth, SquidPostPopParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Suppressed)()) { depth = unchecked(depth - 1); return; }
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("현재 공통 후처리는10.78 전용", nameof(pool));
        ReadOnlySpan<byte> raw = pool.Slot(sid); uint firstGenus = Need(hooks.Type)(raw[10]).Flags2;
        bool skipGraph = (firstGenus & RegionMask) != 0 && !Need(hooks.GraphsEnabled)();
        if (!skipGraph)
        {
            if ((firstGenus & RegionMask) != 0 && (flags & 3) != 0 && (flags & NoGraph) == 0) Need(hooks.Region)(sid);
            if (Need(hooks.GraphsEnabled)())
            {
                if ((flags & 0x203) == 0 || (flags & NoGraph) != 0)
                {
                    if ((Need(hooks.Type)(raw[10]).Flags1 & 0x800) != 0 && (flags & 1) != 0 && (flags & NoGraph) != 0) Need(hooks.ResetGraph)(sid);
                }
                else
                {
                    if ((flags & 0x800) != 0) Need(hooks.Assertion)("!(popFlags&pfDESTROY_GRAPH)", @"\Ns\O\Squid.cpp", 0x80b);
                    if ((Need(hooks.Type)(raw[10]).Flags1 & 0x800) != 0) Need(hooks.Add)(sid);
                }
            }
        }
        uint savedGenus = Need(hooks.Type)(raw[10]).Flags2;
        if ((flags & 1) != 0 && (raw[40] & 9) == 0)
        {
            Need(hooks.Cost)(sid);
            if ((Need(hooks.Type)(raw[10]).Flags1 & 0x10000000) != 0) Need(hooks.Provider)(sid);
            if ((savedGenus & 0x4200) != 0) { Need(hooks.OwnerFactory)(raw[34], sid); Need(hooks.Factory)(sid); }
            Need(hooks.AiNotice)(raw[34], sid);
            uint savedOwner = raw[34]; uint localOwner = Need(hooks.LocalOwner)();
            if (savedOwner == localOwner) Need(hooks.LocalCount)(raw[10]);
            Need(hooks.GlobalCount)(raw[10]); Need(hooks.RecheckSelection)();
        }
        depth = unchecked(depth - 1);
    }
}
