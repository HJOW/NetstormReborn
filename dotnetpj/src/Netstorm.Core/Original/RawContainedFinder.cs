namespace Netstorm.Core.Original;

/// <summary>
/// 내용물(contained) 검색기가 조회·갱신하는 전역 묶음(원본 ContainedFinderState).
/// boss 는 현재 권한 전역이고, containedType 은 후보 바이트와 비교하는 타입 번호다.
/// checkingDead 는 패치판의 dead 부모 조회 assert 를 활성화한다.
/// 근거: cpppj/src/o/RawContainedFinder.h (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class ContainedFinderState
{
    /// <summary>현재 권한 전역(원본 00540bc4). 죽은 부모를 조회해도 되는지 함께 정한다.</summary>
    public bool Boss { get; set; } = true;

    /// <summary>후보 바이트와 비교하는 타입 번호(원본 00541088). 기본값은 6 이다.</summary>
    public uint ContainedType { get; set; } = 6;

    /// <summary>원문 진단뷰의 독립값/장기 owner를 보관하며 외부뷰 객체의 주소 자체는 빌리지 않는다.</summary>
    private RawCheckingFlag _checkingDead = new();

    /// <summary>패치판 dead 부모 조회의 현재 전체DWORD다. 대입은 원문 값 사본으로 같은 장기 owner를 빌린다.</summary>
    public RawCheckingFlag CheckingDead { get => _checkingDead; set => _checkingDead = new(value); }

    /// <summary>효과 뒤 Carrier/내용물 조회가 같은 실제 Client boss를 읽는 장기 경계다.</summary>
    public Func<bool>? SharedBoss { get; set; }

    /// <summary>dead 부모 진단의 식/파일/행을 전달하며 정상 복귀하면 현재head로 조회를 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>원문 관문에서 같은 현재 공유boss 또는 독립값을 읽는다.</summary>
    public bool ReadBoss() => SharedBoss?.Invoke() ?? Boss;
}

/// <summary>
/// 내용물(종속) 체인을 앞에서부터 훑는 커서(원본 RawContainedFinder, 004b2270 ↔ CD 004eb8d0).
/// 부모 슬롯의 head WORD 에서 시작해 타입이 일치하는 후보만 돌려주되, 다음 링크는 반환 전에 저장한다.
/// 원본처럼 후보의 free/dead/void/contained 상태는 거르지 않는다.
/// PATCH의 현재checking/dead 진단과 CD의 진단 생략을 구별한다.
/// 근거: cpppj/src/o/RawContainedFinder.cpp (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawContainedFinder
{
    /// <summary>슬롯 안 종속 다음 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>상태 바이트의 죽음 비트(삭제 준비).</summary>
    private const byte DeadBit = 2;

    /// <summary>객체 슬롯을 읽는 풀. 탐색기보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>권한·타입 전역을 읽는 상태.</summary>
    private readonly ContainedFinderState _state;

    /// <summary>다음에 돌려볼 항목의 번호.</summary>
    private int _next;

    /// <summary>이미 지나온 번호. 순환을 거부하는 데 쓴다.</summary>
    private readonly HashSet<int> _visited = [];

    /// <summary>내용물 검색기를 만든다. 풀과 전역은 탐색기보다 오래 살아야 한다.</summary>
    /// <param name="pool">종속 체인을 읽을 슬롯 풀.</param>
    /// <param name="state">권한·타입 전역.</param>
    public RawContainedFinder(RawSidPool pool, ContainedFinderState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        _pool = pool;
        _state = state;
    }

    /// <summary>
    /// 부모 슬롯의 head WORD 에서 탐색을 시작한다.
    /// dead 부모 조회는 권한이 없고 allowDead 가 거짓일 때만 거부한다(원본 assert 경로).
    /// </summary>
    /// <param name="parent">부모 슬롯 번호.</param>
    /// <param name="allowDead">죽은 부모를 허용할지.</param>
    /// <returns>처음 찾은 후보 번호. 없으면 0.</returns>
    public int Begin(int parent, bool allowDead = false)
    {
        if (parent < 0 || parent >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), parent, "내용물 검색기 부모 번호 오류");
        }
        // 권한 없는 dead 부모 조회는 패치판 assert 경로다.
        if (_pool.Edition == OriginalEdition.Patch1078 && _state.CheckingDead && (_pool.Slot(parent)[StateOffset] & DeadBit) != 0 && !_state.ReadBoss() && !allowDead)
        {
            Action<string, string, uint> assertion = _state.Assertion ?? throw new InvalidOperationException("권한 없는 죽은 부모의 내용물 조회 경로");
            assertion("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy", @"\Ns\O\Squidfinder.cpp", 0x13c);
        }
        _next = Link(_pool.Slot(parent), HeadOffset);
        _visited.Clear();
        return Next();
    }

    /// <summary>
    /// 다음 후보를 돌려준다. 반환할 후보의 다음 링크는 먼저 저장하므로 호출자의 중첩 효과에도 안전하다.
    /// </summary>
    /// <returns>다음 후보 번호. 없으면 0.</returns>
    public int Next()
    {
        // 타입 전역은 후보마다 읽고 순환·범위 오류는 방문 시점에 거부한다.
        while (_next != 0)
        {
            int current = _next;
            if (current >= _pool.Capacity || !_visited.Add(current))
            {
                throw new InvalidOperationException("내용물 검색기 종속 체인 손상");
            }
            ReadOnlySpan<byte> raw = _pool.Slot(current);
            _next = Link(raw, NextOffset);
            if (raw[TypeOffset] == _state.ContainedType)
            {
                return current;
            }
        }
        return 0;
    }

    /// <summary>004acc60의같은 현재 커서로 종류DWORD와 maskWORD가 일치하는 모든 후보를 원문 순서로 센다.</summary>
    public static int CountContained(RawSidPool pool, ContainedFinderState state, int parent, uint mask, uint kind)
    {
        var finder = new RawContainedFinder(pool, state); uint count = 0;
        // 상태/free/dead 추가 필터 없이 각 후보의 종류 판정 뒤 전체WORD mask도 읽는다.
        for (int sid = finder.Begin(parent); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid); uint candidateKind = (uint)(raw[18] | raw[19] << 8); bool matches = kind == 0 || kind == candidateKind; uint candidateMask = (uint)(raw[20] | raw[21] << 8);
            if ((mask == 0 || (mask & candidateMask) != 0) && matches) count = unchecked(count + 1);
        }
        return unchecked((int)count);
    }

    /// <summary>정렬되지 않은 16비트 값을 낮은 바이트부터 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static int Link(ReadOnlySpan<byte> raw, int offset) => raw[offset] | (raw[offset + 1] << 8);
}
