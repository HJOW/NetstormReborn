// 사제 비패턴 계약과 일반 칸 순회를 같은 지역 정책/미리보기/충돌 경계로 연결한다.
namespace Netstorm.Core.Original;

/// <summary>사제 칸 순회가 소비하는 실제 지역 정책 함수 객체다.</summary>
public sealed class PriestPlacementGeometryHooks
{
    /// <summary>decoder 생성 뒤 호출하는 지역 초기화다.</summary>
    public Action<PriestPlacementQuery>? BeginRegions { get; set; }
    /// <summary>각 후보 소진 뒤 프레임/스냅 좌표로 지형과 관계를 판정한다.</summary>
    public Func<PriestPlacementQuery, int, float, float, bool>? EndShape { get; set; }
    /// <summary>모든 모양 진행 뒤 최종 지역/관계를 판정한다.</summary>
    public Func<PriestPlacementQuery, bool>? FinishRegions { get; set; }
    /// <summary>finder 호출 전에 현재 모양의 지역 배열/발자국 원점을 준비한다.</summary>
    public Action<PriestPlacementQuery, int, float, float>? BeginShape { get; set; }
    /// <summary>함수 객체 사본을 만들어 호출자 속성 변경과 분리한다.</summary>
    internal PriestPlacementGeometryHooks Copy() => new() { BeginRegions = BeginRegions, EndShape = EndShape, FinishRegions = FinishRegions, BeginShape = BeginShape };
}

/// <summary>실제 사제 기하/후보 충돌이 공유하는 모양 순회와 후보별 지형 경계다.</summary>
public sealed class PriestPlacementCollisionHooks
{
    /// <summary>실제 모양마다 finder의 사각형 검사 함수를 호출하는 지역 정책이다.</summary>
    public Func<PriestPlacementQuery, Func<SquidSearchArea, bool>, bool>? InspectRegions { get; set; }
    /// <summary>무시된 후보도 처리하는 후보별 현재 지형/지역 효과다.</summary>
    public Action<PriestPlacementQuery, int>? InspectCandidateTerrain { get; set; }
    /// <summary>대리자를 복사하며 실제 장기 모듈은 빌린다.</summary>
    internal PriestPlacementCollisionHooks Copy() => new() { InspectRegions = InspectRegions, InspectCandidateTerrain = InspectCandidateTerrain };
}

/// <summary>현재 사제 genus·비패턴·양수 발자국 계약을 일반 기하 순회 위에 적용한다.</summary>
public sealed class RawPriestPlacementGeometry
{
    /// <summary>판본과 동일 어댑터를 확인하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 genus와 모양별 발자국을 읽는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원래 번호 체계의 현재 프레임 표다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;
    /// <summary>공유 현재 패턴 타입과 진단 상태다.</summary>
    private readonly PriestPlacementGeometryState _state;
    /// <summary>구성 때 복사한 실제 지역 효과다.</summary>
    private readonly PriestPlacementGeometryHooks _hooks;
    /// <summary>자료와 필수 세 지역 정책을 먼저 확인한다.</summary>
    public RawPriestPlacementGeometry(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, PriestPlacementGeometryState state, PriestPlacementGeometryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || frames.Count != types.Count || hooks.BeginRegions is null || hooks.EndShape is null || hooks.FinishRegions is null) throw new ArgumentException("사제 모양 자료/지역 경계 누락");
        _pool = pool; _types = types; _frames = frames; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>일반 기하 위임 전에 현재 사제 genus와 비패턴 계약을 확인한다.</summary>
    private void CheckQuery(PriestPlacementQuery query)
    {
        if (query.Type >= _types.Count || (_types[(int)query.Type].Flags2 & 0x200000) == 0 || Array.IndexOf(_state.PatternTypes, query.Type) >= 0) throw new ArgumentException("사제 비패턴 CanonDecoder 타입 계약 오류");
    }
    /// <summary>원래 타입 번호를 첫 인자로 전달하고 명시 프레임을 끈 범위다.</summary>
    public SquidSearchArea Bounds(PriestPlacementQuery query)
    {
        CheckQuery(query); return new RawCanonPlacementGeometry(_pool, _types, _frames, _state).Bounds(new(query.Type, (int)query.Type, unchecked((int)query.Flags), query.X, query.Y));
    }
    /// <summary>사제의 양수 발자국 계약을 확인한 뒤 공통 실제 finder 산술을 사용한다.</summary>
    public static SquidSearchArea CollisionArea(float x, float y, int footX, int footY)
    {
        if (footX < 1 || footY < 1) throw new ArgumentOutOfRangeException(nameof(footX), "사제 모양 발자국 오류");
        return RawCanonPlacementGeometry.CollisionArea(x, y, footX, footY);
    }
    /// <summary>값 인자를 저장해 지역/모양 정책에 전달하고 실제 사각형만 scan에 공급한다.</summary>
    public bool Inspect(PriestPlacementQuery query, Func<SquidSearchArea, bool> scan)
    {
        if (scan is null) throw new ArgumentException("사제 모양 충돌 탐색 경계 누락"); CheckQuery(query);
        var hooks = new CanonPlacementGeometryHooks
        {
            BeginRegions = _ => _hooks.BeginRegions!(query), EndShape = (_, cell) => _hooks.EndShape!(query, cell.Frame, cell.SnappedX, cell.SnappedY), FinishRegions = _ => _hooks.FinishRegions!(query),
            BeginShape = (_, cell) => { if (_types[(int)query.Type].FootX < 1 || _types[(int)query.Type].FootY < 1) throw new ArgumentOutOfRangeException(nameof(query), "사제 모양 발자국 오류"); _hooks.BeginShape?.Invoke(query, cell.Frame, cell.SnappedX, cell.SnappedY); },
        };
        return new RawCanonPlacementGeometry(_pool, _types, _frames, _state).Inspect(new(query.Type, (int)query.Type, unchecked((int)query.Flags), query.X, query.Y), cell => scan(cell.Area), hooks);
    }
    /// <summary>실제 미리보기/충돌과 동일 풀을 공유하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>미리보기의 범위만 실제 decoder에 연결하고 후보 정책은 보존한다.</summary>
    public static PriestPlacementPreviewHooks MakePriestGeometryPreviewHooks(RawSidPool pool, RawPriestPlacementGeometry geometry, PriestPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(geometry); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, geometry.Pool)) throw new ArgumentException("사제 모양/미리보기의 SID 풀이 다릅니다");
        PriestPlacementPreviewHooks result = hooks.Copy(); result.Bounds = geometry.Bounds; return result;
    }
    /// <summary>후보별 지형 경계를 보존하고 실제 모양/정수 사각형 순회만 연결한다.</summary>
    public static PriestPlacementCollisionHooks MakePriestGeometryCollisionHooks(RawSidPool pool, RawPriestPlacementGeometry geometry, PriestPlacementCollisionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(geometry); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, geometry.Pool)) throw new ArgumentException("사제 모양/충돌의 SID 풀이 다릅니다");
        PriestPlacementCollisionHooks result = hooks.Copy(); result.InspectRegions = geometry.Inspect; return result;
    }
}
