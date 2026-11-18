// 배치 decoder의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하고 주변 권한을 누적한다.
namespace Netstorm.Core.Original;

/// <summary>
/// 일반 타입·패턴 배치 서라운딩 처리 중 참조하는 상태는 두지 않는다. 지도·타입·프레임 자료와 권한 몸체를 빌린다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 주변 관계 판정에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementSurrounding.h.
/// </summary>
/// <remarks>2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 서라운딩이다.</remarks>
public sealed class RawCanonPlacementSurrounding
{
    /// <summary>같은 풀의 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 프레임 자료다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;

    /// <summary>현재 섬의 배치·소유권 자료다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<ushort> _islands;

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>소유권·지역 관계에 따른 배치 허용 판정이다. 같은 풀이어야 한다.</summary>
    private readonly RawCanonPlacementPermission _permission;

    /// <summary>
    /// 실제 후보 권한 helper와 같은 풀을 요구하며 지도/타입/프레임 자료는 호출자가 소유한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 판본별 개수여야 한다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="islands">현재 섬의 배치·소유권 자료(65,536칸)다.</param>
    /// <param name="spots">칸별 점유 비트 자료(65,536칸)다.</param>
    /// <param name="permission">소유권·지역 관계 판정이다. 같은 풀이어야 한다.</param>
    public RawCanonPlacementSurrounding(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<PriestPlainCanonType> frames,
        IReadOnlyList<ushort> islands,
        IReadOnlyList<byte> spots,
        RawCanonPlacementPermission permission)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(permission);
        if (!ReferenceEquals(pool, permission.Pool))
        {
            throw new ArgumentException("주변 표면/권한 SID 풀 불일치", nameof(pool));
        }

        _pool = pool;
        _types = types;
        _frames = frames;
        _islands = islands;
        _spots = spots;
        _permission = permission;
    }

    /// <summary>같은 풀의 실제 Player 조회인지 확인한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 권한을 얻어도 남은 후보를 계속 읽으며 요청 owner의 하위 바이트를 부호 확장한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <param name="cell">현재 모양의 격자 정보다.</param>
    /// <returns>주변에서 하나라도 허용되면 참이다.</returns>
    public bool Inspect(CanonPlacementQuery query, CanonPlacementCell cell)
    {
        var walk = new RawCanonSurfaceWalk(_pool, _types, _frames, _islands, _spots, query, cell);
        // 요청 owner 하위 바이트를 부호 확장한 signed BYTE 계약으로 읽는다.
        uint owner = unchecked((uint)(int)(sbyte)(byte)query.Owner);
        bool result = false;
        // 처음 권한을 얻은 뒤에도 원본은 현재 모양의 표면 순회를 계속하는 반복문이다.
        for (int candidate = walk.Current(); candidate != 0; candidate = walk.Next())
        {
            if (_permission.Surrounding(candidate, owner) != 0)
            {
                result = true;
            }
        }

        return result;
    }

    /// <summary>
    /// 모양 종료에서 초기/후보 권한이 false일 때만 실제 주변 순회에 들어간다.
    /// </summary>
    /// <param name="pool">같은 풀의 실제 Player 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="surrounding">같은 풀의 주변 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다. 모양 권한만 실제 구현으로 바꾼다.</param>
    /// <returns>모양 권한이 연결된 훅 묶음이다.</returns>
    public static CanonPlacementTerrainHooks MakeCanonSurroundingTerrainHooks(RawSidPool pool, RawCanonPlacementSurrounding surrounding, CanonPlacementTerrainHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(surrounding);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, surrounding.Pool))
        {
            throw new ArgumentException("주변 표면/지형 SID 풀 불일치", nameof(pool));
        }

        CanonPlacementTerrainHooks result = new()
        {
            CandidatePermission = hooks.CandidatePermission,
            ShapePermission = hooks.ShapePermission,
            FinishRelations = hooks.FinishRelations,
        };
        result.ShapePermission = (query, cell) => surrounding.Inspect(query, cell);
        return result;
    }
}
