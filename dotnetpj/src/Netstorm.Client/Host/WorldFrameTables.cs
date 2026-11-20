using Netstorm.Assets;
using Netstorm.Core.Original;

// 원문MakeRawSurfaceStorage의자산70이후FrameTable배치를Session없는시작에서도사용한다.
namespace Netstorm.Client.Host;

/// <summary>TYPE별프레임의순수작성기다. 파일/게임상태/난수를읽지않는다.</summary>
public static class WorldFrameTables
{
    /// <summary>전체TYPE수만큼빈표를구성하고실제자산의클러스터코드를70번부터넣는다.</summary>
    public static RiftTypeFrames[] Create(IReadOnlyList<RiftTypeDefinition> definitions, int count = 188)
    {
        ArgumentNullException.ThrowIfNull(definitions); if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
        var frames = new RiftTypeFrames[count];
        // 각내장슬롯도자산벡터의독립된빈FrameTable값을소유한다.
        for (int index = 0; index < count; index++) frames[index] = new RiftTypeFrames([]);
        Fill(frames, definitions); return frames;
    }
    /// <summary>기존독립Session의Fill관문도같은코드순서를사용한다. 자산밖의현재슬롯은바꾸지않는다.</summary>
    public static void Fill(RiftTypeFrames[] frames, IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(definitions);
        if ((long)RiftTypeTable.FirstAssetTypeNumber + definitions.Count > frames.Length) throw new ArgumentOutOfRangeException(nameof(definitions), "타입정의가Frame표폭을넘음");
        // 실제자산적재순서대로같은클러스터선언순서를복사한다.
        for (int index = 0; index < definitions.Count; index++)
        {
            var codes = new RiftFrameCode[definitions[index].Clusters.Count];
            // 원문의Side/Variant/Number/Flags BYTE순서를유지한다.
            for (int code = 0; code < codes.Length; code++) codes[code] = definitions[index].Clusters[code].Code;
            frames[RiftTypeTable.FirstAssetTypeNumber + index] = new RiftTypeFrames(codes);
        }
    }
}
