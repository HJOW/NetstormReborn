namespace Netstorm.Assets;

/// <summary>지면과 별도로 그리는 절벽 스프라이트의 월드 기준점과 클러스터.</summary>
public sealed record FortTerrainFringeSprite(int X, int Y, int Cluster);

/// <summary>Terrainbuilder 004bfe50의 fringe 생성 조건·방향·위치·조명 필터를 적용한다.</summary>
public static class FortTerrainFringe
{
    /// <summary>원본 0x502314의 4.0은 화면 픽셀이 아닌 월드 세로 칸 이동량이다.</summary>
    public const int OffsetY = 4;

    /// <summary>선택된 지면 타일의 fringe 플래그에 맞춰 절벽을 생성한다. 변형은 좌표로 고정한다.</summary>
    public static IReadOnlyList<FortTerrainFringeSprite> Create(FortTerrainPreview terrain,
        TypeDefinition isle, TypeDefinition fringe, bool battleMode = true)
    {
        var result = new List<FortTerrainFringeSprite>();
        // 지면의 저장 순서를 유지하여 절벽끼리의 겹침이 반복 실행에서도 일정하게 한다.
        foreach (FortTerrainTile tile in terrain.Tiles)
        {
            Cluster source = isle.Clusters[tile.Cluster];
            int? cluster = SelectCluster(source, fringe, tile.X, tile.Y, battleMode);
            if (!cluster.HasValue)
            {
                continue;
            }
            result.Add(new FortTerrainFringeSprite(tile.X, tile.Y + OffsetY, cluster.Value));
        }
        return result;
    }

    /// <summary>0049aa90의 방향 폴백 이후 전투 조명 필터를 적용한다. 장식 없는 타일에는 null을 반환한다.</summary>
    public static int? SelectCluster(Cluster source, TypeDefinition fringe, int x, int y, bool battleMode = true)
    {
        if (!source.Flags.Contains("fringe", StringComparer.OrdinalIgnoreCase))
        {
            return null;
        }
        char a = source.Name[0];
        char b = source.Name[1];
        int[] candidates = Candidates(fringe, $"{a}{b}");
        if (candidates.Length == 0)
        {
            (char normalizedA, char normalizedB) = FortTerrainPreview.NormalizeOrientation(a, b);
            candidates = Candidates(fringe, $"{normalizedA}{normalizedB}");
        }
        if (candidates.Length == 0)
        {
            candidates = Candidates(fringe, $"{a}A");
        }
        if (candidates.Length == 0)
        {
            candidates = Candidates(fringe, $"{a}");
        }
        // 원본은 방향 후보에서 무작위로 뽑은 뒤 금지 조명일 때 다시 뽑는다. 허용 후보 집합은 같다.
        candidates = candidates.Where(index => !battleMode
            || !fringe.Clusters[index].Flags.Contains("unlit", StringComparer.OrdinalIgnoreCase)).ToArray();
        if (candidates.Length == 0)
        {
            return null;
        }
        // 원본의 전역 난수 소비 순서는 미확정이므로 허용 변형 중 좌표로 고정한 하나를 고른다.
        return candidates[(x * 31 + y * 17) % candidates.Length];
    }

    /// <summary>방향이 일치하는 클러스터를 원본 정의 순서대로 반환한다.</summary>
    private static int[] Candidates(TypeDefinition fringe, string orientation) => Enumerable.Range(0, fringe.Clusters.Count)
        .Where(index => fringe.Clusters[index].Name.StartsWith(orientation, StringComparison.OrdinalIgnoreCase)).ToArray();
}
