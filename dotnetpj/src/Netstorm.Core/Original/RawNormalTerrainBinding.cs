// 일반 지형 소비자의 분기·범위·타입 연결 계약을 공유한다.
namespace Netstorm.Core.Original;

// 일반지형 바인딩 계약 모듈을 담는 네임스페이스다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.

// 군도와 미복원 부모를 정상 지형으로 대체하지 않는 검사다. 바인딩 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.
public static class RawNormalTerrainChecks
{
    /// <summary>일반 분기가 아니면 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">확인한 분기 요청이다.</param>
    public static void RequireBranch(NormalTerrainRequest request)
    {
        if (request.Branch != NormalTerrainBranch.Normal)
        {
            throw new InvalidOperationException("군도·전체 부모를 일반 지형으로 실행할 수 없습니다.");
        }
    }

    /// <summary>유효 내부 청크 범위를 요구한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">검사할 요청이다.</param>
    public static void RequireBounds(NormalTerrainRequest request)
    {
        ChunkRect bounds = request.Bounds;
        if (bounds.Left < 1 || bounds.Top < 1 || bounds.Right > 15 || bounds.Bottom > 15 ||
            bounds.Left >= bounds.Right || bounds.Top >= bounds.Bottom)
        {
            throw new ArgumentOutOfRangeException(nameof(request), "일반 지형 소비자의 유효 내부 청크 범위가 필요합니다.");
        }
    }

    /// <summary>군도와 서로 다른 원본 전역을 묵시적인 정상 지형·지역 기본값으로 바꾸지 않는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="farm">군집 상태다.</param>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireNormal(FarmClusterState farm, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(farm);
        if (farm.CurrentArchipelago())
        {
            throw new InvalidOperationException("일반 전용 지형 요청에 군도 상태를 사용할 수 없습니다.");
        }

        RequireStorage(globals, placement, stamp, types, pool);
    }

    /// <summary>현재 원본 모드의 선택과 분리하여 같은 실제 TYPE·지역·생성자 연결을 확인한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireStorage(TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(stamp);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(pool);
        if (globals.IslandType != placement.IslandType || globals.IslandType != stamp.IslandType ||
            globals.EdgeType != stamp.EdgeType || globals.NoRegion != stamp.NoRegion)
        {
            throw new ArgumentException("일반 지형 같은 타입·지역 전역 연결 오류");
        }

        // 지면 97의 실제 0 슬롯·base 계약은 보존하며 121의 실제 생성자를 임의 0 슬롯으로 대신하지 않는다
        foreach (uint type in new[] { globals.IslandType, placement.EdgeFarmType })
        {
            if (type >= (uint)types.Count ||
                types[(int)type].ConstructorAddress != OriginalConstructors.Address(pool.Edition, (int)type))
            {
                throw new ArgumentException("일반 지형 실제 타입 생성자 연결 누락");
            }
        }
    }
}
