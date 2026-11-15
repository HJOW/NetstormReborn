using Netstorm.Core.Bridges;

// Graph의 실제 현재 물리 실행과 순수 사전 계산을 구분한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Client의 현재 checking DWORD와 선택된 원문 단언 장치다.</summary>
public sealed class GraphDiagnostics
{
    /// <summary>각 진단 위치에서 현재값을 다시 읽는다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>원문 식·파일·행을 받으며 정상 복귀 또는 예외를 그대로 전달한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawGraphLive.cpp의 같은 현재 기록·스택·슬롯을 사용하는 실행 몸체다.</summary>
public sealed partial class RawGraph
{
    /// <summary>실제 PATCH가 명시 대여한 현재 TYPE다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;
    /// <summary>선택된 현재 TYPE 또는 독립 snapshot이다.</summary>
    private IReadOnlyList<OriginalTypeRecord> CurrentTypes => _currentTypes ?? _types;
    /// <summary>현재 진단 뷰와 함수 사본이다.</summary>
    private GraphDiagnostics _diagnostics = new();
    /// <summary>현재 level0x20 로그 장치다.</summary>
    private Action<int, string>? _trace;
    /// <summary>삭제의 두 시각 읽기가 사용하는 같은 현재 Client 시계다.</summary>
    private Func<double>? _clock;
    /// <summary>현재 graphsWork owner다. 독립 입력의 기본은 활성이다.</summary>
    private RawCheckingFlag _detachEnabled = new(true);
    /// <summary>현재 rebuildGraph owner다. 미연결 독립 입력은 호출 인자를 쓴다.</summary>
    private RawCheckingFlag? _detachRebuild;

    /// <summary>기록/stack/sentinel을 쓰지 않고 실제 장기 배열을 직접 빌린다. 독립 생성자는 기존 값 사본을 유지한다.</summary>
    public RawGraph(RawSidPool pool, RawSquidHash hash, byte[] spots, IReadOnlyList<OriginalTypeRecord> types,
        RawGraphStorage storage, IReadOnlyList<IReadOnlyList<Netstorm.Assets.FrameCode>>? frames = null, GraphRecovery recovery = GraphRecovery.Reject)
        : this(pool, hash, spots, types, (storage ?? throw new ArgumentNullException(nameof(storage))).Records, storage.Stack, frames, recovery)
    {
        _records = storage.Records; _stack = storage.Stack;
    }

    /// <summary>실제 PATCH의 현재188개 표만 빌리고 빈 입력은 기존 snapshot으로 복귀한다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord>? types)
    {
        if (types is { Count: > 0 } && (_pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)) throw new ArgumentException("Graph 현재TYPE 대여의 판본/길이 오류", nameof(types));
        _currentTypes = types is { Count: > 0 } ? types : null;
    }
    /// <summary>현재 checking owner와 함수 사본만 보관하며 그래프를 초기화하지 않는다.</summary>
    public void BindDiagnostics(GraphDiagnostics diagnostics) { ArgumentNullException.ThrowIfNull(diagnostics); _diagnostics = new() { Checking = new(diagnostics.Checking), Assertion = diagnostics.Assertion }; }
    /// <summary>현재 로그 장치만 연결한다.</summary>
    public void BindTrace(Action<int, string>? trace) => _trace = trace;
    /// <summary>현재 시계 장치만 연결하며 시각을 아직 읽지 않는다.</summary>
    public void BindClock(Func<double>? clock) => _clock = clock;
    /// <summary>실제 삭제의 현재 활성/재구성 전역을 값 사본인 진단 뷰로 대여한다.</summary>
    public void BindDetachState(RawCheckingFlag enabled, RawCheckingFlag rebuild) { _detachEnabled = new(enabled); _detachRebuild = new(rebuild); }
    /// <summary>현재 타입의 물리 범위를 확인한 뒤 레코드를 반환한다.</summary>
    private OriginalTypeRecord CurrentType(uint type) => type < CurrentTypes.Count ? CurrentTypes[(int)type] : throw new ArgumentOutOfRangeException(nameof(type), "Graph 현재TYPE 범위 오류");
    /// <summary>현재 타입·가상표가 아닌 원문 그래프 BYTE만 진단 후 읽는다.</summary>
    private byte ReadNumber(int sid)
    {
        byte type = _pool.Slot(sid)[10];
        if (_pool.Edition == OriginalEdition.Patch1078 && (CurrentType(type).Flags1 & OriginalTypeRecord.Surface) == 0 && type is not 90 and not 140)
            Diagnose("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType", 0x739, @"\Ns\O\Squid.cpp");
        return _pool.Slot(sid)[GraphOffset];
    }
    /// <summary>타입/번호 진단이 복귀하면 free 여부를 추가 검사하지 않고 현재 물리 BYTE를 쓴다.</summary>
    private void WriteNumber(int sid, byte graph)
    {
        byte type = _pool.Slot(sid)[10];
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            if ((CurrentType(type).Flags1 & OriginalTypeRecord.Surface) == 0 && type is not 90 and not 140)
                Diagnose("getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType", 0x74b, @"\Ns\O\Squid.cpp");
            if (graph >= Count && graph != Invalid && type is not 90 and not 140)
                Diagnose("( n >=0 && n < MaxGraphs) || ( n==invalidGraphNum) || type == fencemarkType || type == windArcherType", 0x750, @"\Ns\O\Squid.cpp");
        }
        _pool.RawSlot(sid)[GraphOffset] = graph;
    }
    /// <summary>공통 postPop의 noGraph 쓰기도 같은 현재 진단/물리 경로를 사용한다.</summary>
    public void SetNumber(int sid, byte graph) => WriteNumber(sid, graph);
    /// <summary>선택된 원문 단언만 현재 장치를 요구한다.</summary>
    private void Diagnose(string expression, uint line, string? file = null) =>
        (_diagnostics.Assertion ?? throw new InvalidOperationException("Graph 실제 진단 장치 미연결"))(expression, file ?? (_pool.Edition == OriginalEdition.Patch1078 ? @"\Ns\O\Graph.cpp" : @"\ns\o\graph.cpp"), line);
    /// <summary>sentinel 두 조건은 진단 뒤 현재 checking과 같은 행을 각각 다시 읽는다.</summary>
    private void CheckSentinel()
    {
        if (_pool.Edition != OriginalEdition.Patch1078 || !_diagnostics.Checking) return;
        if (_records[Invalid].Surfaces != Sentinel) Diagnose("graphs[INVALID_GRAPH].numSurfaces == 0x7DFD", 0x68);
        if (_diagnostics.Checking && _records[Invalid].InUse != Sentinel) Diagnose("graphs[INVALID_GRAPH].inUse == 0x7DFD", 0x69);
    }
    /// <summary>원문 두 checking 관문 뒤 활성/양수인 현재 행만 감소시킨다.</summary>
    private void RemoveSurface(byte graph)
    {
        if (_pool.Edition == OriginalEdition.Patch1078 && _diagnostics.Checking)
        {
            if (_records[graph].InUse == 0) Diagnose("inUse", 0x59);
            if (_diagnostics.Checking && _records[graph].Surfaces < 1) Diagnose("numSurfaces > 0", 0x5a);
        }
        if (_records[graph].InUse != 0 && _records[graph].Surfaces > 0)
        {
            _records[graph].Surfaces = Word(_records[graph].Surfaces - 1);
            if (_records[graph].Surfaces == 0) _records[graph].InUse = 0;
        }
    }
    /// <summary>원문 WORD 저장의 low16과 signed 해석이다.</summary>
    private static short Word(int value) => unchecked((short)value);
    /// <summary>원문 현재 로그를 계산한 뒤 현재 장치를 호출한다.</summary>
    private void TraceInUse() { if (_trace is not null) _trace(0x20, $"{_records.Take(Count).Count(record => record.InUse != 0)} graphs in use\n"); }
    /// <summary>현재 코드 snapshot과 물리, 명시 현재 TYPE를 같은 이웃 탐색기에 제공한다.</summary>
    private RawSquidNeighbors LiveNeighbors()
    {
        var neighbors = new RawSquidNeighbors(_pool, _hash, _spots, CurrentTypes, _frames);
        if (_currentTypes is not null) neighbors.BindCurrentTypes(_currentTypes);
        return neighbors;
    }
    /// <summary>낮은 빈 행을 선택하고 소진된 경우에만 현재 풀을 재구성한다.</summary>
    private byte ExecuteAllocate(bool recover)
    {
        int found = Array.FindIndex(_records, 0, Count, record => record.InUse == 0);
        if (found < 0)
        {
            if (!recover) throw new InvalidOperationException("그래프가 소진되어 전체 풀 재구성이 필요합니다");
            ExecuteRebuild(true); found = Array.FindIndex(_records, 0, Count, record => record.InUse == 0);
            if (found < 0) { Diagnose("false", 0x25d); throw new InvalidOperationException("원문 Graph 할당의 NULL 반환은 호스트 레코드로 해석할 수 없습니다"); }
        }
        _records[found].Surfaces = 0; _records[found].InUse = 1; return (byte)found;
    }
    /// <summary>현재 LIFO stack과 graph BYTE를 직접 소비하며 진단/로그 변경을 사본으로 덮지 않는다.</summary>
    private uint ExecuteFlood(int sid, byte graph, bool inspect = false)
    {
        CheckSentinel(); ValidateFree(graph); if (inspect) _ = Calculate(sid, Operation.Inspect, 0, null);
        RawSquidNeighbors neighbors = LiveNeighbors(); _stack[0] = (uint)sid; int depth = 1; uint changed = 0;
        // 별도 방문 집합 없이 현재 목적 번호로 중복 push를 판정한다.
        while (depth != 0)
        {
            uint value = _stack[--depth]; if (value == 0 || value > ushort.MaxValue || value >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), "Graph 현재 flood stack SID 범위 오류");
            int current = (int)value; byte previous = ReadNumber(current); if (previous == graph) continue;
            ValidateFree(previous); changed++; if (previous != Invalid) _records[previous].Surfaces = Word(_records[previous].Surfaces - 1);
            if (graph != Invalid) _records[graph].Surfaces = Word(_records[graph].Surfaces + 1); WriteNumber(current, graph);
            var walk = new RawSquidNeighborWalk(neighbors, current, NeighborFlags.SurfaceMap);
            // Next마다 현재 지도/후보를 읽고 원문 순서로 stack에 넣는다.
            for (int candidate = walk.Current; candidate != 0; candidate = walk.Next())
            {
                if (ReadNumber(candidate) == graph) continue;
                if (_pool.Edition == OriginalEdition.Patch1078 && _diagnostics.Checking && candidate == current) Diagnose("cFinder.s().getSid() != sid", 0x176);
                if (depth >= _stack.Length) { Diagnose("d < FLOOD_STACK_SIZE", 0x17a); throw new ArgumentOutOfRangeException(nameof(sid), "Graph flood의 원문 물리 stack 범위를 넘습니다"); }
                _stack[depth++] = (uint)candidate; if (depth >= _stack.Length) Diagnose("d < FLOOD_STACK_SIZE", 0x17a);
            }
        }
        TraceInUse(); CheckSentinel(); return changed;
    }
    /// <summary>sentinel→선택 Reset→SID순 두 순회→임시 행 해제→sentinel의 현재 재구성이다.</summary>
    private void ExecuteRebuild(bool resetAll)
    {
        CheckSentinel(); bool exhausted = !_records.Take(Count).Any(record => record.InUse == 0);
        _ = Calculate(0, !resetAll && exhausted ? Operation.Allocate : Operation.Rebuild, resetAll ? (byte)1 : (byte)0, null);
        if (resetAll) ResetTable(); byte temporary = ExecuteAllocate(true); int firstLimit = _pool.Capacity;
        // 원문 iterator는2부터 시작하고 free만 건너뛰며 최초 끝을 보관한다.
        for (int id = 2; id < firstLimit; id++)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(id); if ((raw[11] & 1) != 0 || (CurrentType(raw[10]).Flags1 & OriginalTypeRecord.Surface) == 0) continue;
            if (!resetAll && ReadNumber(id) != Invalid) continue; float x = CoordinateValue(raw, 14), y = CoordinateValue(raw, 18);
            if (!float.IsFinite(x) || !float.IsFinite(y) || x < 0 || y < 0 || x >= 256 || y >= 256) throw new ArgumentOutOfRangeException(nameof(id), "Graph 재구성 spot 좌표 범위 오류");
            if ((_spots[(int)y * 256 + (int)x] & 8) == 0) { WriteNumber(id, temporary); _records[temporary].Surfaces = Word(_records[temporary].Surfaces + 1); }
        }
        int secondLimit = _pool.Capacity;
        // 앞 flood가 바꾼 현재 번호를 읽어 다음 성분만 새로 할당한다.
        for (int id = 2; id < secondLimit; id++)
            if ((_pool.Slot(id)[11] & 1) == 0 && (CurrentType(_pool.Slot(id)[10]).Flags1 & OriginalTypeRecord.Surface) != 0 && ReadNumber(id) == temporary)
                ExecuteFlood(id, ExecuteAllocate(true));
        _records[temporary].Surfaces = 0; _records[temporary].InUse = 0; CheckSentinel();
    }
    /// <summary>현재 이웃의 가장 큰 첫 연결을 선택하고 나머지 현재 무리를 원문 순서로 병합한다.</summary>
    private void ExecuteAdd(int sid)
    {
        CheckSentinel(); ReadOnlySpan<byte> raw = _pool.Slot(sid); float x = CoordinateValue(raw, 14), y = CoordinateValue(raw, 18);
        bool patch = _pool.Edition == OriginalEdition.Patch1078, valid = float.IsFinite(x) && float.IsFinite(y) && x > 0 && y > 0 && x < 256 && y < 256;
        if (patch && _diagnostics.Checking && !valid) Diagnose("pos.isValid()", 0xbd);
        if (!valid || (!patch && ((int)x <= 0 || (int)y <= 0))) return;
        if (patch && _diagnostics.Checking && (_pool.Slot(sid)[11] & 7) != 0) Diagnose("!s.isVoid() && !s.isDead() && !s.isFree()", 0xc2);
        if ((_pool.Slot(sid)[11] & 7) != 0) return; _ = Calculate(sid, Operation.Inspect, 0, null);
        var walk = new RawSquidNeighborWalk(LiveNeighbors(), sid, NeighborFlags.SurfaceMap); int[] connections = new int[64]; int count = 0, biggestSize = int.MinValue + 1; byte biggest = 0;
        // 기존 SID의 현재 번호와 중복을 비교하며 번호 사본으로 고정하지 않는다.
        for (int candidate = walk.Current; candidate != 0; candidate = walk.Next())
        {
            byte number = ReadNumber(candidate); if (number == Invalid) continue; ValidateFree(number);
            if (connections.Take(count).Any(prior => ReadNumber(prior) == ReadNumber(candidate))) continue;
            if (count == connections.Length) throw new ArgumentOutOfRangeException(nameof(sid), "Graph 연결 목록의 원문 물리 범위를 넘습니다");
            connections[count++] = candidate; byte current = ReadNumber(candidate); ValidateFree(current);
            if (_records[current].Surfaces > biggestSize) { biggest = current; biggestSize = _records[current].Surfaces; }
        }
        if (count == 0) { byte number = ExecuteAllocate(_recovery == GraphRecovery.FullPool); if (_records[number].InUse != 1) Diagnose("g->inUse == true", 0xf3); ExecuteFlood(sid, number); }
        else
        {
            if (patch && _diagnostics.Checking && _records[biggest].InUse != 1) Diagnose("g->inUse == true", 0x100);
            if (_records[biggest].InUse == 0) _records[biggest].InUse = 1; WriteNumber(sid, biggest); _records[biggest].Surfaces = Word(_records[biggest].Surfaces + 1);
            if (count > 1)
            {
                int flooded = 0;
                // 앞 flood/진단/로그가 바꾼 뒤 후보 번호를 다시 읽는다.
                foreach (int candidate in connections)
                {
                    if (candidate == 0 || ReadNumber(candidate) == biggest) continue; byte loser = ReadNumber(candidate);
                    if (loser == Invalid) throw new ArgumentOutOfRangeException(nameof(sid), "Graph 병합의 원문 loser NULL 행입니다"); ValidateFree(loser);
                    if (patch && _diagnostics.Checking && _records[loser].Surfaces > biggestSize) Diagnose("loser->numSurfaces <= biggestSize", 0x113);
                    ExecuteFlood(candidate, biggest); flooded++; _records[loser].Surfaces = 0; _records[loser].InUse = 0;
                }
                if (patch && _diagnostics.Checking && flooded != count - 1) Diagnose("numFlooded == numConnections - 1", 0x11f);
            }
        }
        TraceInUse();
    }
    /// <summary>일반 finder가 저장한 다음 체인을 따라 현재 표면별 Add를 연속 실행한다.</summary>
    private void ExecutePostDestroy(int sid)
    {
        ValidatePostDestroy(sid); ReadOnlySpan<byte> raw = _pool.Slot(sid); OriginalTypeRecord type = CurrentType(raw[10]); float x = CoordinateValue(raw, 14), y = CoordinateValue(raw, 18);
        if (!float.IsFinite(x) || !float.IsFinite(y) || x < 0 || y < 0 || x >= 256 || y >= 256) throw new ArgumentOutOfRangeException(nameof(sid), "Graph postDestroy 좌표 범위 오류");
        int left = Math.Clamp((int)x - type.FootX + 1, 1, 255), top = Math.Clamp((int)y - type.FootY + 1, 1, 255);
        var finder = new RawSquidFinder(_pool, _hash, CurrentTypes);
        // 앞 Add가 바꾼 타입·상태와 현재 해시를 다음 후보에서 다시 읽는다.
        for (int candidate = finder.Begin(new(left, top, x > 0 && y > 0 ? (int)x : left, x > 0 && y > 0 ? (int)y : top)); candidate != 0; candidate = finder.Next())
            if ((CurrentType(_pool.Slot(candidate)[10]).Flags1 & OriginalTypeRecord.Surface) != 0) Add(candidate);
    }
    /// <summary>삭제 정보의 값 원천과 현재 후보/활성/재구성 owner로 연결을 분할한다.</summary>
    private void ExecuteDetach(int sid, bool rebuild, byte removedSurfaces, byte? sourceType)
    {
        if (!_detachEnabled) return; ValidateDetach(sid, _detachRebuild is not null ? _detachRebuild.Read() != 0 : rebuild, removedSurfaces, sourceType);
        RawSquidNeighbors neighbors = LiveNeighbors(); SquidNeighborSource source = neighbors.Source(sid); if (sourceType is { } replacement) source = source with { Type = replacement };
        CheckSentinel(); if (!_detachEnabled) return; int head = RawPathSurface.SurfaceAt(_hash, new() { X = source.X, Y = source.Y }); if (head == 0) return;
        byte number = ReadNumber(head); if (number == Invalid) return; ValidateFree(number); if (_records[number].InUse == 0) return;
        var walk = new RawSquidNeighborWalk(neighbors, source, 0, true); int[] connections = new int[64]; int count = 0;
        // 원문은 그래프별 중복 제거 없이 현재 SID를 모두 보관한다.
        for (int candidate = walk.Current; candidate != 0; candidate = walk.Next())
        {
            if (count == connections.Length) throw new ArgumentOutOfRangeException(nameof(sid), "Graph 삭제 연결의 원문 물리 범위를 넘습니다");
            connections[count++] = candidate; if (count >= connections.Length) Diagnose("numConnections < MAX_CONNECTIONS", 0x1ef);
        }
        _clock?.Invoke();
        // keep 초기값은 한 번 읽고 각 분할 조건은 flood 뒤 현재 전역을 다시 읽는다.
        bool Rebuilding() => _detachRebuild is not null ? _detachRebuild.Read() != 0 : rebuild;
        bool keep = !Rebuilding(); int remaining = count;
        // 초기 목록의 순서와 앞 효과가 고친 후보 graph를 함께 유지한다.
        for (int index = 0; index < count; index++, remaining--)
        {
            int candidate = connections[index]; if (candidate == 0) Diagnose("floodSid != 0", 0x20b); byte current = 0;
            if (candidate == 0) { if (_pool.Edition == OriginalEdition.Patch1078 && _diagnostics.Checking) Diagnose("mySid", 0x93); }
            else if ((CurrentType(_pool.Slot(candidate)[10]).Flags1 & OriginalTypeRecord.Surface) != 0) current = ReadNumber(candidate);
            else { SquidNeighborSource position = neighbors.Source(candidate); int surface = RawPathSurface.SurfaceAt(_hash, new() { X = position.X, Y = position.Y }); current = surface == 0 ? Invalid : ReadNumber(surface); }
            ValidateFree(current); if (current != number) continue;
            if (!Rebuilding() && remaining < 2) { if ((CurrentType(source.Type).Flags2 & 8) == 0) keep = true; }
            else ExecuteFlood(candidate, ExecuteAllocate(_recovery == GraphRecovery.FullPool));
        }
        if (count != 0) _records[number].Surfaces = Word(_records[number].Surfaces - removedSurfaces);
        if (!keep) { _records[number].Surfaces = 0; _records[number].InUse = 0; }
        _clock?.Invoke(); CheckSentinel();
    }
    /// <summary>정렬되지 않은 원문 좌표를 같은 float32 비트로 읽는다.</summary>
    private static float CoordinateValue(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));
}
