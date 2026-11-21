// 실제 경로 생성/검색/명령 필드/준비/부착과 부모 통지를 원문 순서로 실행한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPathStart.cpp의 실제 이동 시작 부모다.</summary>
public sealed class RawPathStart
{
    /// <summary>무효 경로에서 실제 겹침 정리를 요청하는 원문 부류다.</summary>
    private const uint FailureGenus = 0x8c044200;
    /// <summary>부모/실제 Form이 함께 쓰는 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 실제 일반 finder의 공간 지도다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재TYPE/부류/발자국을 조회하는 같은 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>실제 Kernel/Form을 소유하는 같은 Host다.</summary>
    private readonly SquidProcessHost _host;
    /// <summary>현재 종속 목록의 기존 같은 종류 경로를 찾는다.</summary>
    private readonly RawContainedLookup _lookup;
    /// <summary>사제 통지의 기본 프레임을 실제 공간/표시에 반영한다.</summary>
    private readonly SquidFrame _frame;
    /// <summary>같은 시계와 참조 전역 owner다.</summary>
    private readonly RawPathStartState _state;
    /// <summary>원문 값형으로 복사한 실제 외부 함수다.</summary>
    private readonly RawPathStartHooks _hooks;
    /// <summary>첫 생성/삭제 전에 같은 Pool/TYPE와 필수 실제 장치를 확인한다.</summary>
    public RawPathStart(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, SquidProcessHost host, RawContainedLookup lookup, SquidFrame frame, RawPathStartState state, RawPathStartHooks hooks)
    {
        Pool = pool; _hash = hash; _types = types; _host = host; _lookup = lookup; _frame = frame; _state = state; _hooks = hooks with { };
        if (!ReferenceEquals(host.Pool(), pool) || !ReferenceEquals(lookup.Pool, pool) || !ReferenceEquals(frame.Pool, pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.ProcessType is null || hooks.Create is null || hooks.Metadata is null || hooks.ClearBlocked is null) throw new ArgumentException("Path 시작 월드/필수 효과 연결 누락");
    }
    /// <summary>같은 현재 부모 위치의 float 두 개다.</summary>
    private RawPathPoint Position(int sid) => RawPathSlot.Position(Pool, sid);
    /// <summary>원문 double 덧셈/0쪽 절삭 후 float 좌표를 반환하며 int 범위를 추가하지 않는다.</summary>
    private static RawPathPoint CeilPoint(RawPathPoint point)
    {
        if (!float.IsFinite(point.X) || !float.IsFinite(point.Y)) throw new ArgumentOutOfRangeException(nameof(point), "Path 시작 비정상 좌표");
        return new((float)Math.Truncate((double)point.X + 0.9999f), (float)Math.Truncate((double)point.Y + 0.9999f));
    }
    /// <summary>반대쪽 끝만1..255로 자른 뒤 유효한 원래 점과 합치는 실제 finder 발자국이다.</summary>
    private SquidSearchArea Footprint(int sid)
    {
        RawPathPoint point = Position(sid); OriginalTypeRecord type = _types[Pool.Slot(sid)[10]];
        if (!float.IsFinite(point.X) || !float.IsFinite(point.Y) || type.FootX < 0 || type.FootY < 0) throw new ArgumentOutOfRangeException(nameof(sid), "Path 시작 부모 발자국");
        RawPathPoint other = new(Math.Clamp(point.X - (float)(type.FootX - 1), 1f, 255f), Math.Clamp(point.Y - (float)(type.FootY - 1), 1f, 255f)); bool valid = point.X > 0 && point.Y > 0 && point.X < 256 && point.Y < 256;
        RawPathPoint low = valid ? new(Math.Min(point.X, other.X), Math.Min(point.Y, other.Y)) : other, high = valid ? new(Math.Max(point.X, other.X), Math.Max(point.Y, other.Y)) : other, end = CeilPoint(high); return new((int)low.X, (int)low.Y, (int)end.X, (int)end.Y);
    }
    /// <summary>원문 clamp→기존 경로→Init→필드→실패재귀→Prepare/Attach→통지를 실행한다.</summary>
    public RawPathProcess? Start(RawPathStartRequest request)
    {
        if (request.Parent <= 0 || request.Parent >= Pool.Capacity || !float.IsFinite(request.Destination.X) || !float.IsFinite(request.Destination.Y)) throw new ArgumentOutOfRangeException(nameof(request), "Path 시작 입력 범위");
        RawPathPoint destination = new(Math.Clamp(request.Destination.X, 0f, 254f), Math.Clamp(request.Destination.Y, 0f, 254f)); request = request with { Destination = destination };
        uint parentType = Pool.Slot(request.Parent)[10], processType = _hooks.ProcessType!(parentType), flags = parentType == _state.DudeType || parentType == _state.ManaBoltType || parentType == _state.TeleportType ? 0x10u : 0u;
        if (flags == 0 && _state.SnapPriestDestination && parentType == _state.PriestType) { destination = CeilPoint(destination); destination.X -= .5f; destination.Y -= .5f; request = request with { Destination = destination }; }
        RawPathPoint start = Position(request.Parent); uint first = 0, second = 0;
        if (_state.Clock.Boss)
        {
            int oldForm = _lookup.Find(request.Parent, processType);
            if (oldForm != 0)
            {
                if (_host.FromForm(oldForm) is not RawPathProcess old || !ReferenceEquals(old.Pool, Pool)) throw new InvalidOperationException("기존 다른 파생 Path 수명 미복원 경계");
                if (Pool.Edition == OriginalEdition.Patch1078) { if (start.X <= destination.X) start.X -= .5f; if (start.Y <= destination.Y) start.Y -= .5f; }
                first = old.Runtime.TargetA; second = old.Runtime.TargetB; _host.Kill(old, 0);
            }
        }
        if (processType is not (23 or 54 or 39 or 40 or 42 or 24 or 41 or 38)) throw new InvalidOperationException("다른 파생 이동 생성/Init 미복원 경계");
        if (processType is 39 or 40 or 42 or 24 or 41 or 38 && _hooks.CreateType is null) throw new InvalidOperationException("파생 이동의 실제 생성자 미연결");
        RawPathProcess? owned = (_hooks.CreateType is { } create ? create(processType) : _hooks.Create!(processType == 54)) ?? throw new OutOfMemoryException("Path 실제 생성 실패");
        try
        {
            if (!ReferenceEquals(owned.Pool, Pool)) throw new ArgumentException("Path 시작 생성 풀 불일치"); if (owned.MovementType() != processType) throw new ArgumentException("Path 시작의 실제 생성 종류 불일치");
            owned.InitializePath(request.Parent, destination, start); RawPathRuntime runtime = owned.Runtime; runtime.SetTargets(first, second); runtime.SetFinished(false); runtime.SetCommandFields(request.Helper, request.TailWord, request.TailByte, destination);
            if (runtime.Count < 1)
            {
                bool cleared = false;
                if (Pool.Edition == OriginalEdition.Patch1078)
                {
                    var finder = new RawSquidFinder(Pool, _hash, _types); int candidate = finder.Begin(Footprint(request.Parent), 1);
                    // 원문 현재 후보마다 현재 부모 위치/TYPE/owner로 정리를 요청한다.
                    while (candidate != 0) { if ((_types[Pool.Slot(candidate)[10]].Flags2 & FailureGenus) != 0) { ReadOnlySpan<byte> current = Pool.Slot(request.Parent); _hooks.ClearBlocked!(Position(request.Parent), current[10], current[34]); cleared = true; } candidate = finder.Next(); }
                }
                owned.Dispose(); owned = null; if (Pool.Edition == OriginalEdition.Cd1072) return null; if (_state.Retrying) { _state.Retrying = false; return null; } if (cleared) { _state.Retrying = true; _ = Start(request); } return null;
            }
            if (request.ReplaceStart != 0)
            {
                RawPathPoint rounded = CeilPoint(Position(request.Parent));
                // 시작에 연속한 같은 꼭짓점만 현재 시작 위치로 바꾸며 시각/방향은 유지한다.
                for (int index = 0; index < runtime.Count; index++) { RawPathPoint point = runtime.Point(index); if (point.X != rounded.X || point.Y != rounded.Y) break; runtime.ReplacePoint(index, start); }
            }
            RawPathStartType metadata = _hooks.Metadata!(request.Parent); owned.Prepare(0, metadata.Speed, metadata.TurningSpeed);
            RawPathProcess pending = owned; owned = null; RawPathProcess? result = RawPathProcess.Attach(_host, pending, request.Parent, flags); if (result is null) throw new InvalidOperationException("Path 시작 중 부모 소멸 계약 미복원 경계"); NotifyStarted(request.Parent, destination, result); return result;
        }
        finally { owned?.Dispose(); }
    }
    /// <summary>실제 빈 부모 ret와 간헐천/사제 통지의 서로 다른 몸체를 보존한다.</summary>
    private void NotifyStarted(int parent, RawPathPoint destination, RawPathProcess process)
    {
        uint type = Pool.Slot(parent)[10]; if (type == _state.TeleportType || type == _state.ManaBoltType || type == _state.DudeType) return;
        if (type != _state.PriestType && process.MovementType() is not (38 or 40 or 41 or 42)) throw new InvalidOperationException("다른 부모 이동 통지 미복원 경계");
        var finder = new RawSquidFinder(Pool, _hash, _types); int x = (int)destination.X, y = (int)destination.Y, candidate = finder.Begin(new(x, y, x, y), 1);
        // 첫 후보에서 멈추지 않고 마지막 간헐천을 현재 targetA에 쓴다.
        while (candidate != 0) { if (Pool.Slot(candidate)[10] == _state.GeyserType) process.Runtime.SetTargets((uint)candidate, process.Runtime.TargetB); candidate = finder.Next(); }
        if (process.MovementType() is 38 or 40 or 41 or 42) return;
        // 각 사건을 삭제할 때마다 머리부터 찾아 callback이 추가한 같은 사건도 처리한다.
        foreach (uint ev in new uint[] { 0x25c, 0x25d })
            // 해당 사건의 실제 Regular가 더 없을 때만 다음 사건으로 넘어간다.
            while (_host.FindEvent(parent, ev) is { } regular) _host.Kill(regular, 0);
        _frame.Set(parent, _hooks.Metadata!(parent).DefaultFrame, 0);
    }
}
