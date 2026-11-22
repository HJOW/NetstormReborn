using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 같은 최초 자산의 실제 Canon 반복자와 SHP 추가 헤더를 공급한다.
namespace Netstorm.Client.Host;

/// <summary>RawCursorPlacement.cpp의 BindCursorPlacementAssets다. GUI/지역/출력 장치는 변경하지 않는다.</summary>
public static class HostCursorPlacementAssets
{
    /// <summary>같은 정의와 현재 패턴 전역을 매 호출 때 읽고 실제 반복자의 false 중단을 보존한다.</summary>
    public static void Bind(CursorPlacementHooks hooks, HostAssetTables assets, PriestPlacementGeometryState geometry)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(assets); ArgumentNullException.ThrowIfNull(geometry);
        if (assets.TypeTable.Edition != OriginalEdition.Patch1078) throw new ArgumentException("커서위치자산10.78필요", nameof(assets));
        var shapes = new Netstorm.Client.Display.ShapeDatabase(assets.ShapeImage);
        // 내장 타입을 자산 블록으로 잘못 접지 않고 실제 자산 번호에서만70을 뺀다.
        int Asset(uint type) => type >= RiftTypeTable.FirstAssetTypeNumber && type - RiftTypeTable.FirstAssetTypeNumber < assets.Definitions.Count
            ? (int)type - RiftTypeTable.FirstAssetTypeNumber : throw new ArgumentOutOfRangeException(nameof(type), "커서위치자산TYPE범위");
        hooks.Walk = (query, visit) =>
        {
            var definition = assets.Definitions[Asset(query.Type)];
            var decoder = CanonTypeDecoder.DecodeCanonType(definition.FrameTable(), definition.SpecialFrames.DefaultFrame, geometry.PatternTypes, query, assets.TypeTable.Edition, geometry.DecoderDiagnostics);
            // false이면 현재 칸에서 즉시 끝내며 다음 프레임 진단도 실행하지 않는다.
            while (decoder.Valid()) { if (!visit(new(decoder.Frame(), decoder.X(), decoder.Y()))) return; decoder.Advance(); }
        };
        hooks.Image = (type, frame) =>
        {
            int block = Asset(type); ulong count = (uint)assets.Definitions[block].FrameTable().Codes.Count;
            if (frame < 0 || (uint)frame >= count * ((assets.LoadedTypes[(int)type].Flags1 & 0x440000) != 0 ? 2u : 1u)) throw new InvalidOperationException("커서위치frameCheck원문진단");
            var image = shapes.SquidMetrics(block, frame);
            return new((uint)block + 1, unchecked((short)image.Width), unchecked((short)image.Height), unchecked((short)image.HotspotX), unchecked((short)image.HotspotY));
        };
    }
}
