// 사제의 원래 여섯 값 인자를 일반 로컬 미리보기와 같은 현재 자료에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>사제 비패턴 범위와 전체 후보/지역 정책을 연결하는 함수 객체다.</summary>
public sealed class PriestPlacementPreviewHooks
{
    /// <summary>현재 비패턴 decoder의 정수 범위를 조회한다.</summary>
    public Func<PriestPlacementQuery, SquidSearchArea>? Bounds { get; set; }
    /// <summary>미리보기 뒤 실제 충돌/지역 정책을 호출한다.</summary>
    public Func<PriestPlacementQuery, bool, bool>? InspectCollisions { get; set; }
    /// <summary>대리자를 원문 값 객체처럼 복사한다.</summary>
    internal PriestPlacementPreviewHooks Copy() => new() { Bounds = Bounds, InspectCollisions = InspectCollisions };
}

/// <summary>사제의 요청을 공통 미리보기로 변환해 같은 배열/관계 전역을 공유한다.</summary>
public sealed class RawPriestPlacementPreview
{
    /// <summary>현재 지도/관계/배열과 실제 정책을 공유하는 일반 미리보기다.</summary>
    private readonly RawCanonPlacementPreview _preview;
    /// <summary>비어 있는 함수를 람다로 가리지 않고 구성 때 진단한 뒤 원문 값 인자로 변환한다.</summary>
    private static CanonPlacementPreviewHooks AdaptHooks(PriestPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Bounds is null || hooks.InspectCollisions is null) throw new ArgumentException("사제 미리보기 하위 경계 누락");
        Func<PriestPlacementQuery, SquidSearchArea> bounds = hooks.Bounds; Func<PriestPlacementQuery, bool, bool> collisions = hooks.InspectCollisions;
        return new() { Bounds = q => bounds(new(q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode)), InspectCollisions = (q, local) => collisions(new(q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode), local) };
    }
    /// <summary>고정 지도/판본 계약을 공통 미리보기로 확인하고 실제 자료를 빌린다.</summary>
    public RawPriestPlacementPreview(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, ushort[] surfaceMap, CanonPlacementPreviewState state, PriestPlacementPreviewHooks hooks)
    {
        _preview = new(pool, types, spots, surfaceMap, state, AdaptHooks(hooks));
    }
    /// <summary>타입 번호를 원래 첫 인자로 보존하고 접두에서 저장한 로컬 비교를 재사용한다.</summary>
    public bool Inspect(PriestPlacementQuery query, bool localOwner) => _preview.Inspect(new(query.Type, query.Type, query.X, query.Y, query.Flags, query.Owner, query.Mode), localOwner);
    /// <summary>실제 공통 미리보기가 빌린 풀이다.</summary>
    public RawSidPool Pool => _preview.Pool;
    /// <summary>사제 접두의 후반만 미리보기에 연결하고 픽셀 모양 사본은 유지한다.</summary>
    public static PriestPlacementHooks MakePriestPlacementPreviewHooks(RawSidPool pool, RawPriestPlacementPreview preview, PriestPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(preview); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, preview.Pool)) throw new ArgumentException("사제 미리보기/배치의 SID 풀이 다릅니다");
        PriestPlacementHooks result = hooks.Copy(); result.InspectGeometry = preview.Inspect; return result;
    }
}
