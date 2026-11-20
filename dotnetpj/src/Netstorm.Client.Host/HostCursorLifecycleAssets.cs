using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 장기 최초 자산과 기존 실제 글자 측정기를 커서 수명 장치에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCursorLifecycle.cpp의 BindCursorLifecycleAssets이며 타입 번호와 SHP 토큰을 구분한다.</summary>
public static class HostCursorLifecycleAssets
{
    /// <summary>파일 재적재 없이 같은 최초 TYPE/정의/SHP를 빌리며 미지정 측정기는 기존 장치를 유지한다.</summary>
    public static void Bind(CursorLifecycleHooks hooks, HostAssetTables assets, GumpText? text = null)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(assets);
        if (assets.TypeTable.Edition != OriginalEdition.Patch1078) throw new ArgumentException("커서자산10.78판본계약오류", nameof(assets));
        var shapes = new Netstorm.Client.Display.ShapeDatabase(assets.ShapeImage);
        // 실제 자산 번호를 확인한 뒤에만70을 빼서 원문 블록에 접근한다.
        int Asset(uint type) => type >= RiftTypeTable.FirstAssetTypeNumber && type - RiftTypeTable.FirstAssetTypeNumber < assets.Definitions.Count
            ? (int)type - RiftTypeTable.FirstAssetTypeNumber : throw new ArgumentOutOfRangeException(nameof(type), "커서현재자산TYPE범위");
        hooks.DefaultArgument = type => unchecked((uint)assets.Definitions[Asset(type)].SpecialFrames.DefaultFrame);
        hooks.TypeNumber = type => type < assets.LoadedTypes.Count ? type : throw new ArgumentOutOfRangeException(nameof(type), "커서현재TYPE번호범위");
        hooks.HasRange = type => RawCursorLifecycle.HasCursorRange(assets.LoadedTypes, type);
        hooks.CostImage = (type, frame) =>
        {
            int block = Asset(type); ulong count = (uint)assets.Definitions[block].FrameTable().Codes.Count;
            if (frame >= count * ((assets.LoadedTypes[(int)type].Flags1 & 0x440000) != 0 ? 2u : 1u)) throw new InvalidOperationException("커서비용그림frameCheck원문진단");
            var metrics = shapes.SquidMetrics(block, (int)frame);
            return new((uint)block + 1, unchecked((short)metrics.Width), unchecked((short)metrics.Height), unchecked((short)metrics.HotspotX), unchecked((short)metrics.HotspotY));
        };
        if (text is not null) hooks.Measure = (value, context) =>
        {
            byte[] terminated = Encoding.Latin1.GetBytes(value + "\0"); int width = 0, height = 0;
            text.Measure(terminated, GumpText.DecodeGumpTextContext(context.Span), ref width, ref height, -1); return (width, height);
        };
    }
}
