using Netstorm.Client.Gump;

// 같은 세션의 실제 지형·Player 공간 실행기를 미니맵에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>미션의 동일 물리와 실제 실행기를 사용하는 세션이다.</summary>
public sealed partial class HostSession
{
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
    }
}
