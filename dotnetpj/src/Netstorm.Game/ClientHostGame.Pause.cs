// 실제 장기 시계와 UI 보유 표식을 같은 제품 호스트의 현재 밀리초에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>전체 State와 후속 미션 진행이 재사용할 실제 Client 정지 입구다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 실제 시계의 현재 중첩 정지 여부다. UI 단일 표식으로 대체하지 않는다.</summary>
    public bool NativeGamePaused => _clientClock.Clock.IsPaused();
    /// <summary>UI 정지 요청이 바뀐 실제 선택 위치에서만 현재 밀리초를 읽는다.</summary>
    public void PauseNativeGame(bool paused) => _clientClock.Pause(paused, () => _clock.Milliseconds);
    /// <summary>미션/오류/영상 부모가 실제 현재 밀리초로 한 정지 깊이를 보유한다.</summary>
    public void PushNativeGamePause() => _clientClock.PushGamePause(_clock.Milliseconds);
    /// <summary>State 정리 등의 실제 현재 밀리초로 한 깊이를 반납하고 최종 UI 보유 표식도 함께 해제한다.</summary>
    public void PopNativeGamePause() => _clientClock.PopGamePause(_clock.Milliseconds);
}
