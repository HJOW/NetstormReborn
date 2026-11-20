using Netstorm.Client.Audio;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 메뉴/Session/음향 장치보다 오래 유지하는 State 정리의 실제 나머지 소유자다.
namespace Netstorm.Client;

/// <summary>현재 Template/도움말 표식/Core seed/장면 음악의 원문 장기 물리다. 구성은 파일·곡·정리를 실행하지 않는다.</summary>
public sealed class ClientStateResources
{
    /// <summary>전체State와 실제 파일/메뉴 소비자가 공유할 현재9개manager 소유자다.</summary>
    public CampaignTemplateStorage Templates { get; } = new();
    /// <summary>마지막 도움말표식의 같은 명령DWORD/인자80BYTE다.</summary>
    public QuickHelpPopupState Popup { get; } = new();
    /// <summary>005c89b4의 실제전체DWORD다. 새Client만0이며State는lowWORD만지운다.</summary>
    public UIntBox FortCoreSeed { get; } = new();
    /// <summary>State의songEnd/장면음악/음향장치가같이쓰는실제전역이다.</summary>
    public SceneMusicState Music { get; } = new();
    /// <summary>등록된005caf28다중선택헤더의실제개수DWORD다. 후속선택UI도같은상자를사용한다.</summary>
    public IntBox MultipleSelectionCount { get; } = new();

    /// <summary>이미 존재하는 실제Client의 다른소유자와 함께 현재State뷰를 만든다. 바이트를 복사하거나 초기화하지 않는다.</summary>
    public ClientStateWorldContext Context(ClientSurfaceGlobals surface, ClientLocalStateGlobals local,
        ClientConnectionGlobals connections, ClientKencloudGlobals cloud, ClientNsInfoGlobals nsInfo)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(local); ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(cloud); ArgumentNullException.ThrowIfNull(nsInfo);
        return new()
        {
            Gameplay = surface.Gameplay, Surface = surface.Storage, Local = local, Connections = connections,
            Cloud = cloud, Templates = Templates, Popup = Popup, NsInfoZone = nsInfo.Zone, FortCoreSeed = FortCoreSeed, SongEnd = Music.SongEnd,
        };
    }
}
