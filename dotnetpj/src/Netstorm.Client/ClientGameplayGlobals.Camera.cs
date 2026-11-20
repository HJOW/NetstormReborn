// 현재 카메라의 두64BYTE와 목표/저장 위치는 모든 Session이 같은 장기 소유자를 빌린다.
namespace Netstorm.Client;

/// <summary>Gameplay의 실제 카메라 소유 부분이며 생성/일반 reset으로 Native Renderer를 초기화하지 않는다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>World/Session/호스트 화면 교체로 다시 만들지 않는 실제 카메라 소유자다.</summary>
    public ClientCameraGlobals Camera { get; } = new();
}
