// 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 일반 타입·패턴 배치 처리 흐름 처리 중 조회·갱신하는 상태 묶음이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 전체 파이프라인 상태에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementPipeline.h.
/// </summary>
/// <remarks>2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 파이프라인이다.</remarks>
public sealed class CanonPlacementPipelineState
{
    /// <summary>최종 배치 허용 여부를 판정하는 처리기 또는 상태다.</summary>
    public PriestPlacementState Placement { get; } = new();

    /// <summary>실제 배치 전의 미리보기 판정·출력 처리기 또는 상태다.</summary>
    public CanonPlacementPreviewState Preview { get; } = new();

    /// <summary>후보 객체의 배치 충돌을 판정하는 처리기 또는 상태다.</summary>
    public CanonPlacementCollisionState Collision { get; } = new();

    /// <summary>지형 상태다.</summary>
    public CanonPlacementTerrainState Terrain { get; } = new();

    /// <summary>소유권·지역 관계에 따른 배치 허용 판정 상태다.</summary>
    public CanonPlacementPermissionState Permission { get; } = new();

    /// <summary>표면·섬·지역 사이의 배치 관계 판정 상태다.</summary>
    public CanonPlacementRelationsState Relations { get; } = new();

    /// <summary>앵커 상태다.</summary>
    public PlayerPlacementAnchorState Anchor { get; } = new();

    /// <summary>프레임 발자국·사각형·칸 순회를 계산하는 기하 처리기 상태다.</summary>
    public PriestPlacementGeometryState Geometry { get; } = new();

    /// <summary>프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료 상태다.</summary>
    public PriestPlacementShapeState Shape { get; } = new();
}

/// <summary>
/// 일반 타입·패턴 배치 처리 흐름 관련 처리기가 공유하는 입력 자료다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 전체 파이프라인 입력에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementPipeline.h.
/// </summary>
/// <remarks>2026-11-18 0.610에서 추가했다.</remarks>
public sealed class CanonPlacementPipelineData
{
    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다.</summary>
    public required IReadOnlyList<OriginalTypeRecord> Types { get; init; }

    /// <summary>타입별 프레임 자료다.</summary>
    public required IReadOnlyList<PriestPlainCanonType> Frames { get; init; }

    /// <summary>프레임의 크기·그림 데이터를 조회하는 셰이프 자료다.</summary>
    public required IReadOnlyList<SquidDisplayShape> Shapes { get; init; }

    /// <summary>프레임 기준점을 찾는 타입별 핫스폿 자료다.</summary>
    public required IReadOnlyList<PriestTypeHotspot> Hotspots { get; init; }

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도다.</summary>
    public required byte[] Spots { get; init; }

    /// <summary>현재 섬의 배치·소유권 자료다.</summary>
    public required ushort[] Islands { get; init; }

    /// <summary>지역 자료다.</summary>
    public required IReadOnlyList<CanonPlacementIslandRegion> Regions { get; init; }
}

/// <summary>
/// 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결한다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 실제 판단에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementPipeline.h/.cpp.
/// </summary>
/// <remarks>2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 파이프라인이다.</remarks>
public sealed class RawCanonPlacementPipeline
{
    /// <summary>처리 중 조회·갱신하는 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly CanonPlacementPipelineState _state;

    /// <summary>앵커 모듈이다.</summary>
    private readonly RawPlayerPlacementAnchor _anchor;

    /// <summary>권한 모듈이다.</summary>
    private readonly RawCanonPlacementPermission _permission;

    /// <summary>주변 모듈이다.</summary>
    private readonly RawCanonPlacementSurrounding _surrounding;

    /// <summary>관계 모듈이다.</summary>
    private readonly RawCanonPlacementRelations _relations;

    /// <summary>지형 모듈이다.</summary>
    private readonly RawCanonPlacementTerrain _terrain;

    /// <summary>기하 모듈이다.</summary>
    private readonly RawCanonPlacementGeometry _geometry;

    /// <summary>셰이프 모듈이다.</summary>
    private readonly RawCanonPixelShape _shape;

    /// <summary>충돌 모듈이다.</summary>
    private readonly RawCanonPlacementCollision _collision;

    /// <summary>미리보기 모듈이다.</summary>
    private readonly RawCanonPlacementPreview _preview;

    /// <summary>배치 모듈이다.</summary>
    private readonly RawCanonPlacement _placement;

    /// <summary>
    /// 모든 자료/장부/상태는 호출자가 소유하고 이 객체보다 오래 살아야 한다. 현재 표면은 해시의 0단계 배열을 직접 공유한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="hash">현재 표면의 0단계 배열을 공유하는 해시다.</param>
    /// <param name="data">타입·프레임·셰이프·핫스폿·spot·섬·지역 입력 자료다.</param>
    /// <param name="bookkeeping">객체 종류·소유자별 등록 목록과 개수다.</param>
    /// <param name="additional">원본 별도 추가 목록이다.</param>
    /// <param name="contained">객체 내용물 처리 자료다. 이 파이프라인보다 오래 살아야 한다.</param>
    /// <param name="state">처리 중 조회·갱신하는 상태다.</param>
    public RawCanonPlacementPipeline(
        RawSidPool pool,
        RawSquidHash hash,
        CanonPlacementPipelineData data,
        RawPostPopState bookkeeping,
        RawPostPopList additional,
        ContainedFinderState contained,
        CanonPlacementPipelineState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(data);
        ArgumentNullException.ThrowIfNull(data.Types);
        ArgumentNullException.ThrowIfNull(data.Frames);
        ArgumentNullException.ThrowIfNull(data.Shapes);
        ArgumentNullException.ThrowIfNull(data.Hotspots);
        ArgumentNullException.ThrowIfNull(data.Spots);
        ArgumentNullException.ThrowIfNull(data.Islands);
        ArgumentNullException.ThrowIfNull(data.Regions);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(additional);
        ArgumentNullException.ThrowIfNull(contained);
        ArgumentNullException.ThrowIfNull(state);
        _state = state;
        ushort[] surfaces = hash.BorrowEntries(0);
        _anchor = new RawPlayerPlacementAnchor(pool, data.Types, surfaces, bookkeeping, additional, contained, state.Anchor);
        _permission = new RawCanonPlacementPermission(pool, state.Permission, RawPlayerPlacementAnchor.MakePlayerAnchorQuery(pool, _anchor));
        _surrounding = new RawCanonPlacementSurrounding(pool, data.Types, data.Frames, data.Islands, data.Spots, _permission);
        _relations = new RawCanonPlacementRelations(pool, data.Types, surfaces, data.Regions, state.Relations, state.Placement, _permission);
        _terrain = new RawCanonPlacementTerrain(pool, data.Types, data.Frames, data.Islands, state.Terrain,
            RawCanonPlacementRelations.MakeCanonRelationsTerrainHooks(pool, _relations,
                RawCanonPlacementSurrounding.MakeCanonSurroundingTerrainHooks(pool, _surrounding,
                    RawCanonPlacementPermission.MakeCanonPermissionTerrainHooks(pool, _permission, new CanonPlacementTerrainHooks
                    {
                        CandidatePermission = (_, _) => false,
                        ShapePermission = (_, _) => false,
                        FinishRelations = (_, _) => false,
                    }))));
        _geometry = new RawCanonPlacementGeometry(pool, data.Types, data.Frames, state.Geometry);
        _shape = new RawCanonPixelShape(pool, data.Types, data.Frames, data.Shapes, data.Hotspots, state.Geometry, state.Shape);
        _collision = new RawCanonPlacementCollision(pool, hash, data.Types, state.Collision, state.Preview,
            RawCanonPlacementCollision.MakeCanonGeometryCollisionHooks(pool, _geometry,
                query => RawCanonPlacementTerrain.MakeCanonTerrainGeometryHooks(_terrain.Pool, _terrain, query),
                RawCanonPlacementTerrain.MakeCanonTerrainCollisionHooks(pool, _terrain, new CanonPlacementCollisionHooks())));
        _preview = new RawCanonPlacementPreview(pool, data.Types, data.Spots, surfaces, state.Preview,
            RawCanonPlacementPreview.MakeCanonGeometryPreviewHooks(pool, _geometry,
                RawCanonPlacementCollision.MakeCanonPlacementCollisionHooks(pool, _collision, new CanonPlacementPreviewHooks())));
        _placement = new RawCanonPlacement(pool, data.Types, state.Placement,
            RawCanonPlacement.MakeCanonShapePlacementHooks(pool, _shape,
                RawCanonPlacementPreview.MakeCanonPlacementPreviewHooks(pool, _preview, new CanonPlacementHooks())));
    }

    /// <summary>같은 풀의 실제 배치인지 확인한다.</summary>
    /// <returns>이 파이프라인이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _placement.Pool;

    /// <summary>
    /// 공통 전역의 중복 표현을 현재 값으로 맞춘 뒤 실제 접두부터 최종 반환까지 실행한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <returns>허용되면 참이다.</returns>
    public bool MayPlace(CanonPlacementQuery query)
    {
        _state.Preview.Editor = _state.Permission.Editor;
        _state.Preview.UseAlliances = _state.Permission.UseAlliances;
        Array.Copy(_state.Permission.Alliances, _state.Preview.Alliances, _state.Preview.Alliances.Length);
        _state.Anchor.GraphReady = _state.Permission.GraphReady;
        _state.Terrain.NoIslandType = _state.Geometry.PatternTypes.Length > 3 ? _state.Geometry.PatternTypes[3] : 0;
        return _placement.MayPlace(query);
    }
}
