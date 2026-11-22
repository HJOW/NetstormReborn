using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 여러 생산 세대가 같은 원문 안내 전역을 공유한다.

/// <summary>Source RawMenuControls의 현재 생산 안내 serial/색 전역이다.</summary>
public sealed partial class HostHtmlText
{
    /// <summary>같은 UI owner의 현재 안내이며 개별 생산 노드보다 오래 산다.</summary>
    public ProductionHintState ProductionHints { get; } = new();
    /// <summary>같은 실제 최초 TYPE owner의 그림/정의만 생산 자식에 빌린다.</summary>
    internal MarkupGameAssets RequireProductionAssets(IRawSurfaceAssetSource source) => SharesPictureTypes(source) ? _pictureAssets! : throw new InvalidOperationException("생산과 공통 문자의 최초 자산 owner 불일치");
}
