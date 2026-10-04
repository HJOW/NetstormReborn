namespace Netstorm.Assets;

/// <summary>저장된 오브젝트와 생성 지면의 클러스터를 본체 셰이프 프레임에 대응시킨다.</summary>
public static class MapSpriteFrames
{
    /// <summary>bridge 바이트와 saveFrame은 모두 클러스터 번호이며, 없는 경우 default를 선택한다.</summary>
    public static int BodyFrame(FortObject obj)
    {
        TypeDefinition definition = obj.Type.Definition;
        int cluster = obj.BridgeShape ?? obj.Frame ?? DefaultCluster(definition);
        return BodyFrame(definition, cluster);
    }

    /// <summary>
    /// 저장 프레임이 없는 randframe 타입(거주지)은 원본 캡처처럼 섬 원소에 맞는 그림을 고른다.
    /// 원본은 원소별 그림 묶음 중 무작위로 고르므로, 개발용 미리보기는 lit 프레임 중 하나를 좌표로 고정 선택한다.
    /// 원소별 그림이 없는 타입이나 저장 프레임이 있는 오브젝트는 BodyFrame(FortObject) 와 같다.
    /// </summary>
    /// <param name="item">월드 좌표가 붙은 오브젝트</param>
    /// <param name="theme">오브젝트가 있는 영역의 원소 (sun, rain, wind, thunder)</param>
    public static int BodyFrame(FortMapObject item, string theme)
    {
        FortObject obj = item.Object;
        TypeDefinition definition = obj.Type.Definition;
        if (obj.BridgeShape != null || obj.Frame != null || !definition.HasFlag("randframe"))
        {
            return BodyFrame(obj);
        }
        // 같은 원소 그림이면서 전투 조명(lit)용인 클러스터를 후보로 모은다.
        var candidates = new List<int>();
        for (int i = 0; i < definition.Clusters.Count; i++)
        {
            Cluster cluster = definition.Clusters[i];
            if (cluster.Layers.Count > 0 && cluster.Flags.Contains("lit", StringComparer.OrdinalIgnoreCase)
                && ImageTheme(cluster.Layers[0].Image) == theme)
            {
                candidates.Add(i);
            }
        }
        return candidates.Count == 0 ? BodyFrame(obj) : candidates[(item.X * 31 + item.Y * 17) % candidates.Count];
    }

    /// <summary>
    /// 원본 그림 이름의 접두어로 원소를 판정한다 (RA = 비, TH = 번개, W = 바람, 그 밖 = 해).
    /// 예: RARESIDENCE·RAGRASS → rain, WRESIDENCE·WIGRASS → wind, residence·rgrass → sun.
    /// </summary>
    public static string ImageTheme(string image) =>
        image.StartsWith("RA", StringComparison.OrdinalIgnoreCase) ? "rain"
        : image.StartsWith("TH", StringComparison.OrdinalIgnoreCase) ? "thunder"
        : image.StartsWith("W", StringComparison.OrdinalIgnoreCase) ? "wind" : "sun";

    /// <summary>레이어별로 클러스터 전체를 저장하므로 본체 레이어의 프레임은 클러스터 번호와 같다.</summary>
    public static int BodyFrame(TypeDefinition definition, int cluster)
    {
        if ((uint)cluster >= (uint)definition.Clusters.Count)
        {
            throw new InvalidDataException($"{definition.Name}: 저장 프레임 {cluster}가 클러스터 범위를 벗어납니다.");
        }
        return cluster;
    }

    /// <summary>원본 로더의 기본 프레임(+0x118): 마지막 default 클러스터, 없으면 첫 클러스터.</summary>
    private static int DefaultCluster(TypeDefinition definition) => definition.Frames.DefaultFrame;
}
