using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 세션 지형 Pop의 SHP 크기 조회용 표시 셰이프 목록을 만든다. 2026-11-12 R4 실제 호스트·월드 조립(0.466)에서 추가했다.
public static class TerrainDisplayShapes
{
    /// <summary>타입 번호의 첫 자산 번호다. 원본 kFirstAssetTypeNumber(70)와 같다. 2026-11-12 0.466에서 추가됐다.</summary>
    public const int FirstAssetType = 70;

    /// <summary>
    /// 자산 정의 순서대로 표시 셰이프 목록을 만든다. 원본 SquidRenderer::Shapes(cpppj client/SquidRenderer.cpp 17~35행)와 같은 순서다:
    /// 70번부터 자산 순서대로 셰이프를 두고, 클러스터 수를 프레임 개수로 쓰며, SHP 블록의 모든 프레임을 물리 순서대로 넣는다.
    /// </summary>
    /// <param name="definitions">자산 정의 목록이다. 70번부터 순서대로 쓴다. SHP 블록과 같은 파일 로딩 순서다.</param>
    /// <param name="shapes">SHP 데이터베이스다. 정의 순서와 같은 블록 순서로 읽는다.</param>
    /// <returns>타입 번호와 같은 인덱스의 셰이프 목록이다. 자산 밖 번호는 빈 셰이프다.</returns>
    public static List<SquidDisplayShape> Build(IReadOnlyList<RiftTypeDefinition> definitions, ShapeDatabase shapes)
    {
        ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(shapes);
        if (definitions.Count > shapes.Blocks.Count)
        {
            throw new InvalidDataException("SHP 블록 수 부족: 정의 " + definitions.Count + " > 블록 " + shapes.Blocks.Count);
        }
        var result = new List<SquidDisplayShape>(OriginalConstructors.TypeCount);
        // 모든 타입 자리를 빈 셰이프로 먼저 둔다
        for (int index = 0; index < OriginalConstructors.TypeCount; index++)
        {
            result.Add(new SquidDisplayShape());
        }
        // 자산 정의 순서대로 셰이프를 채운다
        for (int asset = 0; asset < definitions.Count; asset++)
        {
            int type = FirstAssetType + asset;
            if (type >= result.Count)
            {
                break;
            }
            RiftTypeDefinition definition = definitions[asset];
            SquidDisplayShape shape = result[type];
            shape.FrameCount = definition.Clusters.Count;
            shape.Loaded = true;
            // 원본처럼 블록 순서로 찾는다. 정의 안의 typename(예: Man)이 아니라 파일 로딩 순서 이름에 대응한다.
            ShapeBlock block = shapes.Blocks[asset];
            // SHP 블록의 모든 프레임을 물리 순서대로 넣는다
            for (int frame = 0; frame < block.Frames.Count; frame++)
            {
                SquidFrameMetrics metrics = shapes.SquidMetrics(block.Index, frame);
                ShapeFrame source = block.Frames[frame];
                shape.Frames.Add(new SquidDisplayFrame
                {
                    Width = unchecked((short)source.Width),
                    Height = unchecked((short)source.Height),
                    HotspotX = metrics.HotspotX,
                    HotspotY = metrics.HotspotY,
                    CellWidth = metrics.CellWidth,
                    CellHeight = metrics.CellHeight,
                });
            }
        }
        return result;
    }
}
