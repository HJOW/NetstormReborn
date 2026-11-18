// MayPlace 후반의 현재 표면 소유 관계·특수 지역·최종 거부 조건을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>
/// 일반 타입·패턴 배치 섬 영향을 받는 지역 정보다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 특수 지역 판정에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementRelations.h.
/// </summary>
/// <remarks>2026-11-18 0.609에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 둘째다.</remarks>
public readonly record struct CanonPlacementIslandRegion
{
    /// <summary>IslandList +id*32+4의 제한 값이다.</summary>
    public uint Restricted { get; init; }

    /// <summary>IslandList +id*32+8의 존재 값이다.</summary>
    public uint Exists { get; init; }
}

/// <summary>
/// 일반 타입·패턴 배치 관계 처리 중 조회·갱신하는 상태 묶음이다.
/// </summary>
/// <remarks>2026-11-18 0.609에서 추가했다. 근거: cpppj/src/o/RawCanonPlacementRelations.h.</remarks>
public sealed class CanonPlacementRelationsState
{
    /// <summary>005aab30 / CD 0051cc00: 지면·권한만 우회한다.</summary>
    public uint BypassGroundPermission { get; set; }

    /// <summary>10.78 IslandList 00568b60의 부호 있는 개수 DWORD다.</summary>
    public uint IslandCount { get; set; }
}

/// <summary>
/// 일반 타입·패턴 배치 관계 검사 뒤 반환하는 결과이다.
/// </summary>
/// <remarks>2026-11-18 0.609에서 추가했다. 근거: cpppj/src/o/RawCanonPlacementRelations.h.</remarks>
public sealed class CanonPlacementRelationResult
{
    /// <summary>최종 허용이다.</summary>
    public bool Allowed { get; set; }

    /// <summary>내부 거부값이다.</summary>
    public bool RelationRejected { get; set; }

    /// <summary>외부 소유자 거부값이다.</summary>
    public bool ForeignRejected { get; set; }

    /// <summary>CD 이전 지역 변수 저장값이다.</summary>
    public bool PriorRejection { get; set; }
}

/// <summary>
/// MayPlace 후반의 현재 표면 소유 관계·특수 지역·최종 거부 조건이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 표면 관계 판정에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementRelations.h/.cpp, 원본 0046f710·0046f770.
/// </summary>
/// <remarks>2026-11-18 0.609에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 둘째다.</remarks>
public sealed class RawCanonPlacementRelations
{
    /// <summary>지면 좌표의 원문 절삭 상수(+0.9999)다.</summary>
    private const float Bias = 0.9999f;

    /// <summary>원문 지도 한 변의 칸 수다.</summary>
    private const int Side = 256;

    /// <summary>원문 그래프 바이트 위치다. 무효 번호 기록에 쓴다.</summary>
    private const int GraphOffset = 30;

    /// <summary>같은 풀의 객체 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 표면의 배치·소유권 자료다. 같은 지도 WORD 배열을 복사하지 않는다.</summary>
    private readonly IReadOnlyList<ushort> _surfaces;

    /// <summary>현재 섬의 배치·소유권 자료다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<CanonPlacementIslandRegion> _islands;

    /// <summary>처리 중 조회·갱신하는 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly CanonPlacementRelationsState _state;

    /// <summary>최종 배치 허용 여부를 판정하는 처리기 또는 상태다.</summary>
    private readonly PriestPlacementState _placement;

    /// <summary>소유권·지역 관계에 따른 배치 허용 판정이다.</summary>
    private readonly RawCanonPlacementPermission _permission;

    /// <summary>
    /// 현재 표면 지도는 지형 후보의 고정 섬 지도와 별도다. 자료·공유 상태는 호출자가 소유한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 판본별 개수여야 한다.</param>
    /// <param name="surfaces">현재 표면의 배치·소유권 자료(65,536칸)다.</param>
    /// <param name="islands">현재 섬의 배치·소유권 자료다.</param>
    /// <param name="state">처리 중 조회·갱신하는 상태다.</param>
    /// <param name="placement">최종 배치 허용 여부를 판정하는 처리기 또는 상태다.</param>
    /// <param name="permission">소유권·지역 관계에 따른 배치 허용 판정이다. 같은 풀이어야 한다.</param>
    public RawCanonPlacementRelations(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<ushort> surfaces,
        IReadOnlyList<CanonPlacementIslandRegion> islands,
        CanonPlacementRelationsState state,
        PriestPlacementState placement,
        RawCanonPlacementPermission permission)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(permission);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || surfaces.Count != 65536 || !ReferenceEquals(pool, permission.Pool))
        {
            throw new ArgumentException("최종 배치 관계 자료/풀 오류", nameof(pool));
        }
        _pool = pool;
        _types = types;
        _surfaces = surfaces;
        _islands = islands;
        _state = state;
        _placement = placement;
        _permission = permission;
    }

    /// <summary>
    /// 지도 밖이면 슬롯 0으로 돌려준다. 지도 안 번호 유효성은 별도 검사한다.
    /// 원본 +0.9999 절삭 뒤 칸을 고른다.
    /// </summary>
    /// <param name="x">조회할 월드 X 좌표다.</param>
    /// <param name="y">조회할 월드 Y 좌표다.</param>
    /// <returns>표면 슬롯 번호다. 지도 밖이면 0이다.</returns>
    public int SurfaceAt(float x, float y)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y))
        {
            throw new ArgumentOutOfRangeException(nameof(x), "최종 표면 좌표 절삭 범위 오류");
        }
        // 원본 절삭(+0.9999 뒤 trunc)과 같은 순서로 칸을 고른다.
        double px = Math.Truncate((double)x + Bias);
        double py = Math.Truncate((double)y + Bias);
        if (!double.IsFinite(px) || !double.IsFinite(py) || px < int.MinValue || px > int.MaxValue || py < int.MinValue || py > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "최종 표면 좌표 절삭 범위 오류");
        }
        int ix = (int)px, iy = (int)py;
        if (ix < 0 || iy < 0 || ix >= Side || iy >= Side)
        {
            return 0;
        }
        return _surfaces[iy * Side + ix];
    }

    /// <summary>원문 isValid는 주소 범위만 검사한다. free·dead·void 상태는 추가로 제외하지 않는다.</summary>
    /// <param name="sid">검사할 슬롯 번호다.</param>
    /// <returns>풀 안에 있으면 참이다.</returns>
    private bool Valid(int sid) => sid != 0 && sid < _pool.Capacity;

    /// <summary>후반 직접 관계는 활성 플래그나 편집기 조건을 보지 않는다.</summary>
    /// <param name="currentOwner">현재 소유자다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <returns>관계가 있으면 참이다.</returns>
    private bool Alliance(uint currentOwner, uint owner) => _permission.Alliance(currentOwner, owner);

    /// <summary>원본은 없는 지역에서 assert한다. 새 코드로 임의 허용·0으로 대신하지 않는다.</summary>
    /// <param name="region">조회할 지역 번호다.</param>
    /// <returns>현재 제한 값이다.</returns>
    private uint Restricted(int region)
    {
        long count = unchecked((int)_state.IslandCount);
        if (region < 0 || region >= count || region >= _islands.Count || _islands[region].Exists == 0)
        {
            throw new ArgumentOutOfRangeException(nameof(region), "최종 배치 섬 지역 오류");
        }
        return _islands[region].Restricted;
    }

    /// <summary>
    /// MayPlace 후반의 현재 표면 소유 관계·특수 지역·최종 거부 조건을 검사한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <param name="terrain">지면 누적 뒤의 지형 상태다.</param>
    /// <param name="priorRejection">원본 최종 지역 변수 입력이다.</param>
    /// <returns>최종 허용·거부값 묶음이다.</returns>
    public CanonPlacementRelationResult Inspect(CanonPlacementQuery query, CanonPlacementTerrainState terrain, uint priorRejection = 0)
    {
        ArgumentNullException.ThrowIfNull(terrain);
        if (query.Type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(query), "최종 배치 타입 오류");
        }
        OriginalTypeRecord placing = _types[(int)query.Type];
        uint genus = placing.Flags2;
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 요청 owner 하위 바이트를 부호 확장한 signed BYTE 계약으로 읽는다.
        uint owner = unchecked((uint)(int)(sbyte)(byte)query.Owner);
        var result = new CanonPlacementRelationResult { PriorRejection = priorRejection != 0, ForeignRejected = priorRejection != 0 };
        if ((genus & 0x34000u) != 0 && (genus & 0x200000u) == 0)
        {
            int surface = SurfaceAt(query.X, query.Y);
            if (!Valid(surface))
            {
                result.RelationRejected = true;
            }
            else
            {
                byte current = _pool.Slot(surface)[patch ? 34 : 32];
                if ((genus & 0x4000u) == 0 || current != 0)
                {
                    result.RelationRejected = !_permission.Related(current, owner);
                }
            }
            _placement.BlockedRelation = result.RelationRejected ? 1u : 0u;
            if ((genus & 0x4200u) != 0)
            {
                _placement.BlockedRelation = 0;
            }
        }
        if (patch && terrain.GroundComplete && (genus & 0x200u) != 0)
        {
            int region = unchecked((int)(sbyte)terrain.Regions[0]);
            if ((uint)region != terrain.EmptyRegion && Restricted(region) != 0)
            {
                terrain.CanPlaceGround = false;
            }
        }
        if ((genus & 0x200000u) != 0)
        {
            result.Allowed = true;
            return result;
        }
        int target = SurfaceAt(query.X, query.Y);
        if ((genus & 4u) == 0 && Valid(target))
        {
            byte current = _pool.Slot(target)[patch ? 34 : 32];
            if (current != 0 && !Alliance(current, owner) && (genus & 0x20a000u) == 0)
            {
                result.ForeignRejected = true;
                if (!patch)
                {
                    result.PriorRejection = true;
                }
            }
        }
        result.Allowed = (_state.BypassGroundPermission != 0 || (terrain.Permission && terrain.CanPlaceGround)) && !result.RelationRejected && !result.ForeignRejected;
        return result;
    }

    /// <summary>같은 raw 풀을 사용하는 지형 어댑터인지 확인한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 이전 후보·주변 정책을 유지하며 최종 반환만 실제 관계 구현으로 결정한다.
    /// </summary>
    /// <param name="pool">같은 풀의 실제 Player 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="relations">같은 풀의 최종 관계 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다. 최종 관계만 실제 구현으로 바꾼다.</param>
    /// <returns>최종 관계가 연결된 훅 묶음이다.</returns>
    public static CanonPlacementTerrainHooks MakeCanonRelationsTerrainHooks(RawSidPool pool, RawCanonPlacementRelations relations, CanonPlacementTerrainHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(relations);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, relations.Pool))
        {
            throw new ArgumentException("최종 배치 관계/지형 SID 풀 불일치", nameof(pool));
        }
        CanonPlacementTerrainHooks result = new()
        {
            CandidatePermission = hooks.CandidatePermission,
            ShapePermission = hooks.ShapePermission,
            FinishRelations = hooks.FinishRelations,
        };
        result.FinishRelations = (query, terrain) => relations.Inspect(query, terrain).Allowed;
        return result;
    }
}
