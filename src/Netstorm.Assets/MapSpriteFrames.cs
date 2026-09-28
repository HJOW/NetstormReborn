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
