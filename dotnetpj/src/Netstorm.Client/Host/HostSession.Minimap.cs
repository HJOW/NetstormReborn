using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 같은 세션의 실제 지형·Player 공간 실행기를 미니맵에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>미션의 동일 물리와 실제 실행기를 사용하는 세션이다.</summary>
public sealed partial class HostSession
{
    /// <summary>지형 실행기 교체에 통지할 현재 미니맵이다. 세션은 UI 수명을 늘리지 않는다.</summary>
    private WeakReference<RawMinimapControls>? _minimapNotifications;

    /// <summary>현재 지형과 Player의 Pop/Unpop 변경을 같은 미니맵 큐에 일괄 연결한다.</summary>
    /// <remarks>기본 저장소 실행기 대여와 지형·Player 장치가 먼저 설치되어야 한다. 큐와 실행기의 반납은 미니맵 소유자가 수행한다.</remarks>
    public void BindMinimapNotifications(RawMinimapControls minimap)
    {
        ArgumentNullException.ThrowIfNull(minimap);
        var terrainPop = _terrainPop ?? throw new InvalidOperationException("미니맵의 실제 지형 Pop 미설치");
        var terrainUnpop = _terrainUnpop ?? throw new InvalidOperationException("미니맵의 실제 지형 Unpop 미설치");
        var playerPop = _livePop ?? throw new InvalidOperationException("미니맵의 실제 Player Pop 미설치");
        var playerUnpop = _liveUnpop ?? throw new InvalidOperationException("미니맵의 실제 Player Unpop 미설치");
        minimap.BindAdditionalWorldPairs((terrainPop, terrainUnpop), (playerPop, playerUnpop));
        _minimapNotifications = new(minimap);
    }

    /// <summary>그래프 재결선이 바꾼 지형 실행기를 현재 미니맵 대여에 반영한다.</summary>
    /// <remarks>미니맵이 먼저 반납됐거나 다른 세션으로 바뀌었다면 이전 쌍을 찾지 못해 아무것도 바꾸지 않는다.</remarks>
    private void RebindMinimapTerrain(RawPop previousPop, RawUnpop previousUnpop)
    {
        if (_minimapNotifications is not null && _minimapNotifications.TryGetTarget(out var minimap))
            minimap.TryReplaceAdditionalWorldPair(previousPop, previousUnpop,
                _terrainPop ?? throw new InvalidOperationException("교체된 지형 Pop 없음"),
                _terrainUnpop ?? throw new InvalidOperationException("교체된 지형 Unpop 없음"));
    }
}
