using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>번호 소진 때 전체 재구성을 허용할지. 전체 표면을 모두 담은 입력에서만 <see cref="FullPool"/> 을 고른다.</summary>
public enum GraphRecovery
{
    /// <summary>소진되면 예외로 거부한다 (일부 표면만 담은 입력)</summary>
    Reject,
    /// <summary>소진되면 전체를 재구성한 뒤 번호를 확보한다 (원본 00463330 의 소진 경로)</summary>
    FullPool,
}

/// <summary>원본 그래프 레코드: 부호 있는 16비트 세 개 (6바이트)</summary>
public struct GraphRecord
{
    /// <summary>이 그래프에 속한 표면 오브젝트 수. 여러 칸 발자국도 한 오브젝트로 센다</summary>
    public short Surfaces;

    /// <summary>사용 중인지 (0 이 아니면 사용 중)</summary>
    public short InUse;

    /// <summary>원본이 일반 경로에서 건드리지 않는 값. 전체 초기화만 0번 레코드의 하위 바이트를 바꾼다</summary>
    public short Reserved;
}

/// <summary>표면 오브젝트 하나의 그래프 번호와 상태 바이트</summary>
/// <param name="Id">오브젝트 번호</param>
/// <param name="Graph">그래프 번호 (0~250, 무효는 254)</param>
/// <param name="State">상태 바이트 (free 1, dead 2, void 4)</param>
public readonly record struct GraphMembership(int Id, byte Graph = SurfaceGraph.Invalid, byte State = 0);

/// <summary>
/// 표면 연결 그래프의 표: 이어진 표면 오브젝트 무리마다 번호와 표면 수를 가진다. 원본 Graph.cpp(10.78)의
/// 00463330 할당·00462e50 반납·00462a50 감소·00462f10 flood·004633c0 등록·004637b0 삭제 준비·00463110 전체 재구성을 옮겼다
/// (docs/exe/cpp-graph-reconstruction.md 외 graph* 문서. 기준 구현은 cpppj/src/o/Graph.cpp).
/// 다리 붕괴 스캔은 이 표의 표면 수가 5 미만인 다리를 곧바로 없앤다.
/// 원본의 메모리 배치는 복제하지 않지만, 기대값이 표와 flood 스택 전체의 체크섬을 비교하므로 쓰지 않는 칸의 흔적까지 같게 둔다.
/// 월드에는 아직 연결하지 않았다. 검증은 원본 기계어 기대값 graph-x86.tsv(등록·flood·할당·반납·감소)로 한다.
/// 삭제 준비와 전체 재구성은 기대값이 원본 슬롯 바이트를 입력으로 삼아 직접 대조하지 못했고 cpppj 의 단위 검사를 옮겨 확인한다.
/// </summary>
public sealed class SurfaceGraph
{
    /// <summary>정상 그래프 번호의 개수 (0~250)</summary>
    public const int Count = 251;

    /// <summary>표의 전체 레코드 수 (251~253 은 쓰지 않는 칸, 254 는 무효 번호의 자리)</summary>
    public const int TableSize = 255;

    /// <summary>flood 스택의 칸 수</summary>
    public const int FloodSize = 4096;

    /// <summary>무효 그래프 번호</summary>
    public const byte Invalid = 254;

    /// <summary>등록·삭제 준비가 다루는 연결 목록의 용량 (원본 지역 배열)</summary>
    private const int ConnectionCapacity = 64;

    /// <summary>원본이 무효 번호 자리의 앞 두 값에 두는 표시</summary>
    private const short InvalidMarker = 0x7dfd;

    /// <summary>상태 바이트의 free 비트</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 dead 비트</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 free·dead·void 비트</summary>
    private const byte FreeDeadOrVoid = 7;

    /// <summary>삭제 준비가 마지막 이웃 무리를 남길지 볼 때 읽는 타입 플래그 2 비트 (cpppj Graph.cpp DetachImpl 의 flags2 &amp; 8)</summary>
    private const uint SpecialSurfaceBit = 8;

    /// <summary>표면 스냅샷</summary>
    private readonly SurfaceFinder _surfaces;

    /// <summary>그래프 표</summary>
    private GraphRecord[] _records = new GraphRecord[TableSize];

    /// <summary>flood 스택 (쓰고 난 흔적도 남는다)</summary>
    private uint[] _stack = new uint[FloodSize];

    /// <summary>번호 오름차순의 표면 소속 정보</summary>
    private SortedDictionary<int, GraphMembership> _members = [];

    /// <summary>그래프 표를 만든다. 표면 스냅샷은 읽기만 한다.</summary>
    /// <param name="surfaces">표면 스냅샷</param>
    /// <param name="members">표면마다의 그래프 번호와 상태</param>
    /// <param name="records">이어받을 기존 표 (없으면 빈 표와 무효 표시)</param>
    /// <param name="stack">이어받을 기존 flood 스택 (없으면 0)</param>
    public SurfaceGraph(SurfaceFinder surfaces, IEnumerable<GraphMembership> members, IReadOnlyList<GraphRecord>? records = null,
        IReadOnlyList<uint>? stack = null)
    {
        _surfaces = surfaces;
        if ((records != null && records.Count != TableSize) || (stack != null && stack.Count != FloodSize))
        {
            throw new ArgumentException("그래프 표 또는 스택의 크기가 잘못됐습니다");
        }
        if (records == null)
        {
            _records[Invalid] = new GraphRecord { Surfaces = InvalidMarker, InUse = InvalidMarker };
        }
        else
        {
            _records = [.. records];
        }
        if (stack != null)
        {
            _stack = [.. stack];
        }
        // 소속 정보마다 번호·표면 플래그·죽음 상태·중복을 확인한다
        foreach (GraphMembership member in members)
        {
            CheckNumber(member.Graph, true);
            SurfaceObject item = _surfaces.Object(member.Id);
            if ((item.Flags1 & TypeFlagBits.Surface) == 0 || item.Dead != ((member.State & DeadBit) != 0)
                || !_members.TryAdd(member.Id, member))
            {
                throw new ArgumentException($"그래프 소속 정보의 표면·상태·번호가 잘못됐습니다: {member.Id}");
            }
        }
    }

    /// <summary>그래프 표 전체 (무효 번호 자리와 쓰지 않는 칸 포함)</summary>
    public IReadOnlyList<GraphRecord> Records => _records;

    /// <summary>flood 스택 전체 (쓰고 난 흔적 포함)</summary>
    public IReadOnlyList<uint> FloodStack => _stack;

    /// <summary>표면 오브젝트의 현재 그래프 번호</summary>
    /// <param name="id">오브젝트 번호</param>
    public byte Number(int id) => Member(id).Graph;

    /// <summary>표면 오브젝트의 상태 바이트 (그래프 연산은 바꾸지 않는다)</summary>
    /// <param name="id">오브젝트 번호</param>
    public byte State(int id) => Member(id).State;

    /// <summary>표면 오브젝트가 속한 그래프의 표면 수. 무효 번호면 null 이다 (원본은 NULL 레코드를 읽는다)</summary>
    /// <param name="id">오브젝트 번호</param>
    public short? Surfaces(int id) => Member(id).Graph == Invalid ? null : _records[Member(id).Graph].Surfaces;

    /// <summary>사용 중인 정상 그래프의 수 (무효 번호 자리는 세지 않는다)</summary>
    public int InUse => _records.Take(Count).Count(record => record.InUse != 0);

    /// <summary>
    /// 첫 미사용 번호를 확보한다 (원본 00463330 의 일반 경로). 낮은 번호부터 찾고 표면 수를 0 으로 만든다.
    /// 번호가 소진되면 예외다. 전체 재구성은 <see cref="AllocateWithRecovery"/> 로 따로 부른다.
    /// </summary>
    public byte Allocate()
    {
        // 정상 번호 0~250 을 낮은 것부터 본다
        for (int i = 0; i < Count; i++)
        {
            if (_records[i].InUse == 0)
            {
                _records[i].Surfaces = 0;
                _records[i].InUse = 1;
                return (byte)i;
            }
        }
        throw new InvalidOperationException("그래프 번호가 소진되어 전체 재구성이 필요합니다");
    }

    /// <summary>번호가 소진됐으면 전체를 재구성한 뒤 확보한다 (원본 00463330 의 소진 경로). 전체 표면 입력이 필요하다.</summary>
    public byte AllocateWithRecovery() => Transact(() => AllocateForOperation(GraphRecovery.FullPool));

    /// <summary>
    /// 전체 재구성 (원본 00463110): 임시 번호에 대상 표면을 모은 뒤 번호 오름차순으로 무리마다 새 번호를 flood 한다.
    /// </summary>
    /// <param name="resetAll">참이면 표를 지우고 모든 표면을 다시 나눈다. 거짓이면 무효 번호인 표면만 모은다</param>
    public void Rebuild(bool resetAll) => Transact(() =>
    {
        RebuildCore(resetAll);
        return 0;
    });

    /// <summary>번호를 반납한다 (원본 00462e50): 앞 두 값만 지운다. 무효 번호는 그대로 둔다.</summary>
    /// <param name="graph">그래프 번호</param>
    public void Free(byte graph)
    {
        CheckNumber(graph, true);
        if (graph == Invalid)
        {
            return;
        }
        _records[graph].Surfaces = 0;
        _records[graph].InUse = 0;
    }

    /// <summary>표면 수를 1 줄인다 (원본 00462a50): 사용 중이고 양수일 때만 줄이고, 0 이 되면 미사용으로 바꾼다.</summary>
    /// <param name="graph">그래프 번호</param>
    public void Remove(byte graph)
    {
        CheckNumber(graph, false);
        ref GraphRecord record = ref _records[graph];
        if (record.InUse != 0 && record.Surfaces > 0)
        {
            record.Surfaces = Word(record.Surfaces - 1);
            if (record.Surfaces == 0)
            {
                record.InUse = 0;
            }
        }
    }

    /// <summary>
    /// 한 표면에서 이어진 무리 전체를 지정 번호로 바꾼다 (원본 00462f10). 바뀐 표면 수를 돌려준다.
    /// 스택은 후입선출이고 마지막 이웃부터 방문한다. 이미 목적 번호인 표면은 이웃을 넓히지 않는다.
    /// </summary>
    /// <param name="id">시작 표면 번호</param>
    /// <param name="graph">목적 그래프 번호 (무효 번호도 허용)</param>
    public uint Flood(int id, byte graph)
    {
        CheckNumber(graph, true);
        return Transact(() => FloodCore(id, graph));
    }

    /// <summary>
    /// 표면을 그래프에 등록한다 (원본 004633c0). 이웃 그래프 가운데 표면 수가 가장 큰 것에 붙고, 같으면 먼저 탐색한 번호가 남는다.
    /// 다른 연결은 승자 번호로 flood 하고 패배한 번호를 반납한다. 이웃 그래프가 없으면 새 번호를 확보한다.
    /// 같은 표면에 다시 부르면 표면 수가 또 늘어난다 (원본 그대로).
    /// </summary>
    /// <param name="id">표면 번호</param>
    /// <param name="recovery">번호 소진 때의 처리</param>
    public void Add(int id, GraphRecovery recovery = GraphRecovery.Reject) => Transact(() =>
    {
        AddCore(id, recovery);
        return 0;
    });

    /// <summary>
    /// 삭제 준비 (원본 004637b0). 죽은 표면의 이웃 무리를 탐색 순서대로 새 번호로 나누고 마지막 하나는 기존 번호에 남긴다.
    /// 그 뒤 기존 번호의 표면 수를 줄인다 (보통 1, 특수 타입은 9).
    /// </summary>
    /// <param name="id">죽음 비트가 켜진 표면 번호</param>
    /// <param name="connections">삭제 전에 모아 둔 이웃 번호 (탐색 순서)</param>
    /// <param name="rebuild">참이면 마지막 이웃 무리도 새 번호로 옮기고 기존 번호를 비운다</param>
    /// <param name="removedSurfaces">줄일 표면 수 (1 또는 9)</param>
    /// <param name="recovery">번호 소진 때의 처리</param>
    public void Detach(int id, IReadOnlyList<int> connections, bool rebuild = false, byte removedSurfaces = 1,
        GraphRecovery recovery = GraphRecovery.Reject) =>
        DetachAt(id, Member(id).Graph, connections, rebuild, removedSurfaces, recovery);

    /// <summary>
    /// 삭제 준비를 지정한 그래프 번호로 한다 (위치 조회로 다른 오브젝트에서 고른 번호를 쓰는 원본 경로).
    /// </summary>
    /// <param name="id">죽음 비트가 켜진 표면 번호</param>
    /// <param name="graph">대상 그래프 번호</param>
    /// <param name="connections">삭제 전에 모아 둔 이웃 번호</param>
    /// <param name="rebuild">마지막 이웃 무리도 옮길지</param>
    /// <param name="removedSurfaces">줄일 표면 수 (1 또는 9)</param>
    /// <param name="recovery">번호 소진 때의 처리</param>
    /// <param name="sourceFlags2">삭제 원천의 타입 플래그 2 를 바꿔 볼 때의 값 (없으면 스냅샷의 값)</param>
    public void DetachAt(int id, byte graph, IReadOnlyList<int> connections, bool rebuild = false, byte removedSurfaces = 1,
        GraphRecovery recovery = GraphRecovery.Reject, uint? sourceFlags2 = null)
    {
        CheckNumber(graph, true);
        if ((Member(id).State & DeadBit) == 0 || (removedSurfaces != 1 && removedSurfaces != 9))
        {
            throw new ArgumentException("그래프 삭제 준비의 죽음 상태 또는 감소 수가 잘못됐습니다");
        }
        Transact(() =>
        {
            DetachCore(id, graph, connections, rebuild, removedSurfaces, recovery, sourceFlags2);
            return 0;
        });
    }

    /// <summary>원본의 16비트 증가·감소처럼 하위 16비트만 남기고 부호 있게 읽는다.</summary>
    private static short Word(int value) => unchecked((short)value);

    /// <summary>번호가 정상 범위(0~250)인지, 또는 허용된 무효 번호인지 확인한다. 251~253 은 어느 쪽도 아니다.</summary>
    private static void CheckNumber(byte graph, bool allowInvalid)
    {
        if (graph >= Count && !(allowInvalid && graph == Invalid))
        {
            throw new ArgumentOutOfRangeException(nameof(graph), graph, "그래프 번호가 범위를 벗어났습니다");
        }
    }

    /// <summary>번호의 소속 정보를 읽는다. 없는 번호는 입력 오류다.</summary>
    private GraphMembership Member(int id) => _members.TryGetValue(id, out GraphMembership member)
        ? member : throw new KeyNotFoundException($"그래프 소속 정보가 없습니다: {id}");

    /// <summary>
    /// 연산을 실행하고, 예외가 나면 표·스택·소속 정보를 연산 전으로 되돌린다.
    /// 원본은 손상된 입력에서 일부만 바꾼 채 진행하지만 새 코드는 그 결과를 월드에 퍼뜨리지 않는다.
    /// </summary>
    private T Transact<T>(Func<T> operation)
    {
        var records = (GraphRecord[])_records.Clone();
        var stack = (uint[])_stack.Clone();
        var members = new SortedDictionary<int, GraphMembership>(_members);
        try
        {
            return operation();
        }
        catch
        {
            _records = records;
            _stack = stack;
            _members = members;
            throw;
        }
    }

    /// <summary>필요하면 전체 재구성을 한 뒤 번호를 확보한다.</summary>
    private byte AllocateForOperation(GraphRecovery recovery)
    {
        if (recovery == GraphRecovery.FullPool && _records.Take(Count).All(record => record.InUse != 0))
        {
            RebuildCore(true);
        }
        return Allocate();
    }

    /// <summary>전체 재구성의 본체.</summary>
    private void RebuildCore(bool resetAll)
    {
        if (resetAll)
        {
            // 앞 251개 레코드의 앞 두 값만 지운다. 251~253 과 Reserved 는 보존한다
            for (int i = 0; i < Count; i++)
            {
                _records[i].Surfaces = 0;
                _records[i].InUse = 0;
            }
            _records[0].InUse = 1;
            _records[0].Reserved = Word(((ushort)_records[0].Reserved & 0xff00) | 1);
            _records[Invalid].Surfaces = InvalidMarker;
            _records[Invalid].InUse = InvalidMarker;
        }
        byte temporary = Allocate();
        // free 표면은 건너뛴다. dead·void·buried 는 이 순회의 제외 조건이 아니다. 기준점이 안쪽인 표면도 모으지 않는다
        foreach (int id in _members.Keys.ToArray())
        {
            GraphMembership member = _members[id];
            if ((member.State & FreeBit) != 0 || (!resetAll && member.Graph != Invalid) || _surfaces.OriginInterior(id))
            {
                continue;
            }
            _members[id] = member with { Graph = temporary };
            _records[temporary].Surfaces = Word(_records[temporary].Surfaces + 1);
        }
        // 앞선 flood 가 바꾼 번호를 매번 다시 읽는다. 아직 임시 번호인 표면마다 새 번호로 무리를 옮긴다
        foreach (int id in _members.Keys.ToArray())
        {
            GraphMembership member = _members[id];
            if ((member.State & FreeBit) == 0 && member.Graph == temporary)
            {
                FloodCore(id, Allocate());
            }
        }
        Free(temporary);
    }

    /// <summary>flood 의 본체. 먼저 현재 표면의 번호를 바꾼 뒤 이웃을 탐색 순서대로 쌓고 마지막 이웃부터 꺼낸다.</summary>
    private uint FloodCore(int id, byte graph)
    {
        Member(id);
        _stack[0] = (uint)id;
        int depth = 1;
        uint changed = 0;
        // 스택이 빌 때까지 꺼낸다. 따로 방문 표를 두지 않고 "이미 목적 번호인가"만 본다
        while (depth > 0)
        {
            int current = (ushort)_stack[--depth];
            GraphMembership member = Member(current);
            if (member.Graph == graph)
            {
                continue;
            }
            if (member.Graph != Invalid)
            {
                _records[member.Graph].Surfaces = Word(_records[member.Graph].Surfaces - 1);
            }
            if (graph != Invalid)
            {
                _records[graph].Surfaces = Word(_records[graph].Surfaces + 1);
            }
            _members[current] = member with { Graph = graph };
            changed++;
            // 목적 번호가 아닌 이웃을 쌓는다. 같은 이웃이 여러 번 쌓일 수 있다
            foreach (int neighbor in _surfaces.Neighbors(current))
            {
                if (Member(neighbor).Graph == graph)
                {
                    continue;
                }
                if (depth + 1 >= FloodSize)
                {
                    throw new InvalidOperationException("그래프 flood 스택이 넘쳤습니다");
                }
                _stack[depth++] = (uint)neighbor;
            }
        }
        return changed;
    }

    /// <summary>등록의 본체.</summary>
    private void AddCore(int id, GraphRecovery recovery)
    {
        GraphMembership source = Member(id);
        SurfaceObject item = _surfaces.Object(id);
        if (item.X <= 0 || item.Y <= 0 || item.X >= SurfaceFinder.WorldCells || item.Y >= SurfaceFinder.WorldCells
            || (source.State & FreeDeadOrVoid) != 0)
        {
            return;
        }
        var connections = new List<int>();
        int biggestSize = int.MinValue;
        byte biggest = 0;
        // 이웃을 탐색 순서로 보며 그래프 번호가 처음 나온 것만 연결로 센다
        foreach (int neighbor in _surfaces.Neighbors(id))
        {
            byte graph = Member(neighbor).Graph;
            if (graph == Invalid || connections.Any(other => Member(other).Graph == graph))
            {
                continue;
            }
            if (connections.Count == ConnectionCapacity)
            {
                throw new InvalidOperationException("그래프 연결 목록이 넘쳤습니다");
            }
            connections.Add(neighbor);
            if (_records[graph].Surfaces > biggestSize)
            {
                biggest = graph;
                biggestSize = _records[graph].Surfaces;
            }
        }
        if (connections.Count == 0)
        {
            FloodCore(id, AllocateForOperation(recovery));
            return;
        }
        if (_records[biggest].InUse == 0)
        {
            _records[biggest].InUse = 1;
        }
        _members[id] = source with { Graph = biggest };
        _records[biggest].Surfaces = Word(_records[biggest].Surfaces + 1);
        if (connections.Count > 1)
        {
            // 앞서 flood 한 연결의 번호도 다시 읽는다. 승자가 아닌 무리를 승자로 옮기고 패배한 번호를 반납한다
            foreach (int neighbor in connections)
            {
                byte loser = Member(neighbor).Graph;
                if (loser != biggest)
                {
                    FloodCore(neighbor, biggest);
                    Free(loser);
                }
            }
        }
    }

    /// <summary>삭제 준비의 본체. 남은 연결의 마지막 하나를 기존 그래프에 남긴다.</summary>
    private void DetachCore(int id, byte number, IReadOnlyList<int> connections, bool rebuild, byte removedSurfaces,
        GraphRecovery recovery, uint? sourceFlags2)
    {
        if (number == Invalid || _records[number].InUse == 0)
        {
            return;
        }
        if (connections.Count >= ConnectionCapacity)
        {
            throw new InvalidOperationException("그래프 삭제 연결 목록이 넘쳤습니다");
        }
        int remaining = connections.Count;
        bool keep = !rebuild;
        // 앞선 flood 가 바꾼 번호를 매번 다시 읽는다. 연결의 그래프 번호를 미리 중복 제거하지 않는다
        foreach (int neighbor in connections)
        {
            if (Member(neighbor).Graph == number)
            {
                if (!rebuild && remaining < 2)
                {
                    if (((sourceFlags2 ?? _surfaces.Object(id).Flags2) & SpecialSurfaceBit) == 0)
                    {
                        keep = true;
                    }
                }
                else
                {
                    FloodCore(neighbor, AllocateForOperation(recovery));
                }
            }
            remaining--;
        }
        // 이웃이 없는 원천은 줄이지 않는다 (원본 분기)
        if (connections.Count != 0)
        {
            _records[number].Surfaces = Word(_records[number].Surfaces - removedSurfaces);
        }
        if (!keep)
        {
            _records[number].Surfaces = 0;
            _records[number].InUse = 0;
        }
    }
}
