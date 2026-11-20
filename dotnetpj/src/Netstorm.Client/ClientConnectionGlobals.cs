using Netstorm.Net;

// 같은 Client의 실제 동기화 DWORD와 연결표/약한 Source 장부를 공유한다.
namespace Netstorm.Client;

/// <summary>Session 밖에서 같은 실제 Registry와 현재 LocalState 수명을 유지한다.</summary>
/// <remarks>PacketDelays의 실제 분포/장치/큐는 후속이다.</remarks>
public sealed class ClientConnectionGlobals
{
    /// <summary>현재 동기화DWORD의 실제 장기 소유자다.</summary>
    public ClientLocalStateGlobals LocalState { get; }
    /// <summary>같은 실제22/32칸과 노드/호출 수명 장부다.</summary>
    public RawConnectionRegistry Registry { get; } = new();
    /// <summary>실제 Source만 약하게 식별하는 같은 Client 장부다.</summary>
    public ClientPacketSources PacketSources { get; } = new();
    /// <summary>실제 동기화/Source 장치를 빌리며 표본/시각/소켓/난수/성공 상태를 변경하지 않는다.</summary>
    public ClientConnectionGlobals(ClientLocalStateGlobals local)
    {
        ArgumentNullException.ThrowIfNull(local); LocalState = local;
        Registry.BindSynchronization(new ConnectionSyncState(() => local.ClockSynchronization.Connection.Value, value => local.ClockSynchronization.Connection.Value = value)); PacketSources.BindRegistry(Registry);
    }
    /// <summary>실제 UI/시계/전역이 사라지기 전에 노드/Source/listener 순환을 명시 정리한다.</summary>
    public void ShutdownDevices() => PacketSources.Shutdown(Registry.ShutdownTrackedDevices);
}
