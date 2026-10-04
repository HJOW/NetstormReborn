namespace Netstorm.Assets;

/// <summary>원본에서 지면 프레임을 이어받아 흰 가장자리 장식으로 바뀐 칸.</summary>
public sealed record FortEdgeFarmTile(int X, int Y, int Region, int Cluster, int Owner);

/// <summary>Islandbuilder의 edgeFarm 배치를 개발용 맵 뷰어에서 재현한다.</summary>
public static class FortEdgeFarmPreview
{
    /// <summary>원본 0040e560이 영역 청크 하나당 시도하는 배치 개수.</summary>
    private const int FarmsPerChunk = 20;
    /// <summary>좌표를 하나의 정수로 모을 때 사용하는 월드 한 변의 칸 수.</summary>
    private const int WorldSize = FortFile.WorldChunksX * FortMap.CellsPerChunk;
    /// <summary>영역 외관값이 미리보기 선택 순서에 영향을 주도록 섞는 계수.</summary>
    private const uint SeedMultiplier = 0x9E3779B1u;
    /// <summary>좌표 기반 순서의 첫 번째 비트 혼합 계수.</summary>
    private const uint MixMultiplier1 = 0x7FEB352Du;
    /// <summary>좌표 기반 순서의 두 번째 비트 혼합 계수.</summary>
    private const uint MixMultiplier2 = 0x846CA68Bu;

    /// <summary>
    /// 원본의 프레임 대응과 영역별 배치 개수를 적용한다. 전역 난수 상태를 모르는 미리보기에서는
    /// 같은 결과를 반복해서 볼 수 있도록 가장자리 칸을 좌표 기반 순서로 선택한다.
    /// </summary>
    public static IReadOnlyList<FortEdgeFarmTile> Create(FortMap map, FortTerrainPreview terrain,
        TypeDefinition isle, TypeDefinition edgeFarm)
    {
        var result = new List<FortEdgeFarmTile>();
        // 활성 영역마다 원본의 청크 수에 비례하는 배치 목표를 계산한다.
        foreach (FortTerritory territory in map.Territories.Where(item => item.IsActive))
        {
            int count = FortMap.TerritoryChunkShapes(territory).Count * FarmsPerChunk;
            var candidates = terrain.Tiles.Where(tile => tile.Region == territory.Index
                    && CanReplace(tile.Cluster, isle, edgeFarm))
                .OrderBy(tile => Rank(tile.X, tile.Y, territory.Appearance))
                .ThenBy(tile => tile.Y).ThenBy(tile => tile.X)
                .Take(count);
            // matchframe은 기존 isle 프레임 번호를 edgeFarm의 같은 번호에 그대로 대응시킨다.
            foreach (FortTerrainTile tile in candidates)
            {
                result.Add(new FortEdgeFarmTile(tile.X, tile.Y, tile.Region, tile.Cluster, tile.Owner));
            }
        }
        return result.OrderBy(tile => tile.Y).ThenBy(tile => tile.X).ToArray();
    }

    /// <summary>양쪽 타입의 같은 번호가 실제 가장자리 프레임인지 확인한다.</summary>
    private static bool CanReplace(int cluster, TypeDefinition isle, TypeDefinition edgeFarm) =>
        (uint)cluster < (uint)edgeFarm.Clusters.Count
        && isle.Clusters[cluster].Flags.Contains("rim", StringComparer.OrdinalIgnoreCase)
        && string.Equals(isle.Clusters[cluster].Name, edgeFarm.Clusters[cluster].Name,
            StringComparison.OrdinalIgnoreCase);

    /// <summary>전역 난수를 알 수 없는 미리보기에서 좌표와 영역 시드로 고정한 선택 순서.</summary>
    private static uint Rank(int x, int y, int seed)
    {
        uint value = unchecked((uint)(x + y * WorldSize) ^ ((uint)seed * SeedMultiplier));
        value = unchecked((value ^ (value >> 16)) * MixMultiplier1);
        value = unchecked((value ^ (value >> 15)) * MixMultiplier2);
        return value ^ (value >> 16);
    }
}
