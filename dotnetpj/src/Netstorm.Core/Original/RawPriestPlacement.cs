// 사제 배치의 상태·값 인자·일반 배치 접두와 실제 나선 생성 연결을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>픽셀 getter가 반환하는 원문 float 사각형이다.</summary>
/// <param name="Left">왼쪽 좌표다.</param>
/// <param name="Top">위쪽 좌표다.</param>
/// <param name="Right">오른쪽 좌표다.</param>
/// <param name="Bottom">아래쪽 좌표다.</param>
public readonly record struct PriestPlacementRect(float Left, float Top, float Right, float Bottom);

/// <summary>일반/사제 배치가 함께 읽고 쓰는 현재 전역이다.</summary>
public sealed class PriestPlacementState
{
    /// <summary>공유 전역을 연결하지 않았을 때 즉시 허용에 사용하는 전체 DWORD다.</summary>
    public uint ForcePlacement { get; set; }
    /// <summary>부호 확장한 owner BYTE와 모양 조회 전에 비교하는 로컬 번호다.</summary>
    public int LocalPlayer { get; set; }
    /// <summary>유효 검사 진입마다 0으로 지우며 후반 정책이 기록하는 관계 값이다.</summary>
    public uint BlockedRelation { get; set; }
    /// <summary>연결 시 독립 필드 대신 실제 Client의 현재 전체 DWORD를 빌린다.</summary>
    public Func<uint>? SharedForcePlacement { get; set; }
    /// <summary>blockedRelation 초기화 뒤 선택 관문에서 현재 공유 값 또는 독립 값을 읽는다.</summary>
    public uint ReadForcePlacement() => SharedForcePlacement is { } read ? read() : ForcePlacement;
}

/// <summary>사제 배치의 픽셀 모양과 전체 후반 정책 경계다.</summary>
public sealed class PriestPlacementHooks
{
    /// <summary>타입/첫 인자/방향으로 실제 픽셀 사각형을 조회한다.</summary>
    public Func<uint, uint, uint, PriestPlacementRect>? Shape { get; set; }
    /// <summary>원래 값 인자와 모양 조회 전에 저장한 로컬 비교로 미리보기/충돌/지역을 처리한다.</summary>
    public Func<PriestPlacementQuery, bool, bool>? InspectGeometry { get; set; }
    /// <summary>원문 함수 객체처럼 대리자는 복사하고 장기 자료는 빌린다.</summary>
    internal PriestPlacementHooks Copy() => new() { Shape = Shape, InspectGeometry = InspectGeometry };
}

/// <summary>사제 genus를 확인한 뒤 일반 배치 접두에 원래 여섯 값 인자를 전달한다.</summary>
public sealed class RawPriestPlacement
{
    /// <summary>판본과 연결 동일성을 확인하는 원래 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>각 호출 때 현재 사제 genus를 읽는 대여 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>공유 즉시 허용/로컬/차단 관계 전역이다.</summary>
    private readonly PriestPlacementState _state;
    /// <summary>구성 때 검증한 함수 객체 사본이다.</summary>
    private readonly PriestPlacementHooks _hooks;
    /// <summary>판본별 표 길이와 필수 두 경계를 효과 호출 전에 확인한다.</summary>
    public RawPriestPlacement(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, PriestPlacementState state, PriestPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.Shape is null || hooks.InspectGeometry is null) throw new ArgumentException("사제 배치 자료/하위 경계 누락");
        _pool = pool; _types = types; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>사제만 허용하며 좌표/값 인자를 보존해 일반 접두의 현재 상태 처리로 넘긴다.</summary>
    public bool MayPlace(PriestPlacementQuery query)
    {
        if (query.Type < OriginalConstructors.FirstAssetType || query.Type >= _types.Count || (_types[(int)query.Type].Flags2 & 0x200000) == 0) throw new ArgumentException("사제 배치 타입/genus 범위 오류");
        if (!float.IsFinite(query.X) || !float.IsFinite(query.Y)) throw new ArgumentException("사제 배치의 비유한 지도 좌표");
        var placement = new RawCanonPlacement(_pool, _types, _state, new CanonPlacementHooks { Shape = _hooks.Shape, InspectGeometry = (_, localOwner) => _hooks.InspectGeometry!(query, localOwner) });
        return placement.MayPlace(new(query.Type, query.Type, query.X, query.Y, query.Flags, query.Owner, query.Mode));
    }
    /// <summary>생성 장치가 동일한 실제 풀을 빌리는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>동일 풀의 실제 배치만 생성의 MayPlace에 연결하고 다른 함수 사본은 보존한다.</summary>
    public static PriestSpawnHooks MakePriestPlacementHooks(RawSidPool pool, RawPriestPlacement placement, PriestSpawnHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(placement); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, placement.Pool)) throw new ArgumentException("사제 배치/생성의 SID 풀이 다릅니다");
        PriestSpawnHooks result = hooks.Copy(); result.MayPlace = placement.MayPlace; return result;
    }
}
