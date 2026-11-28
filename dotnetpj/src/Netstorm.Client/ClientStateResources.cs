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
    /// <summary>Client의 현재 readOnlyForts DWORD다. State·Session 교체로 초기화하지 않으며 후속 파일 저장 관문도 이 같은 상자를 빌려야 한다.</summary>
    public UIntBox ReadOnlyForts { get; } = new();
    /// <summary>마지막 도움말표식의 같은 명령DWORD/인자80BYTE다.</summary>
    public QuickHelpPopupState Popup { get; } = new();
    /// <summary>005c89b4의 실제전체DWORD다. 새Client만0이며State는lowWORD만지운다.</summary>
    public UIntBox FortCoreSeed { get; } = new();
    /// <summary>State의songEnd/장면음악/음향장치가같이쓰는실제전역이다.</summary>
    public SceneMusicState Music { get; } = new();

    /// <summary>같은 장기 Client의 현재 Player/battle과 선택된 실제 Cloud DWORD만 빌린다. 모든 정체성 검증 뒤 연결하며 값·시각·곡·난수를 소비하지 않는다.</summary>
    public void BindCurrentMusicOwners(ClientGameplayGlobals gameplay, ClientSurfaceGlobals surface,
        ClientLocalStateGlobals local, ClientConnectionGlobals connections, ClientKencloudGlobals? cloud)
    {
        ArgumentNullException.ThrowIfNull(gameplay); ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(local); ArgumentNullException.ThrowIfNull(connections);
        if (!ReferenceEquals(surface.Gameplay, gameplay) || !ReferenceEquals(local.Gameplay, gameplay)
            || !ReferenceEquals(connections.LocalState, local) || !ReferenceEquals(surface.Clock.LocalState, local)
            || !ReferenceEquals(surface.Storage.Authority(), gameplay.Authority)
            || !ReferenceEquals(surface.Storage.GameAux(), gameplay.GameAux)
            || surface.Storage.Pool.Edition != OriginalEdition.Patch1078)
            throw new ArgumentException("장면 음악의 같은 실제 Client/표면/Player/로컬 owner 필요");
        Music.CurrentBattle = () => surface.Storage.OwnerMode.Battle ? 1u : 0u;
        Music.CurrentPlayersReady = () => surface.Gameplay.Player.PlayerKnown;
        Music.CurrentLocalPlayer = () => surface.Gameplay.Player.BattleOwnerBox.Value;
        // 자산 없는 호스트에는 Cloud를 만들지 않으며 이미 가진 독립 dirty 물리를 그대로 둔다.
        if (cloud is not null) Music.BindPaletteDirty(cloud.WorldRedrawBox);
    }

    /// <summary>실제 음향/후속 감독이 같은 장기 Scene을 빌리는지 값을 변경하기 전에 확인한다.</summary>
    public void RequireMusicOwner(SceneMusicState scene)
    {
        ArgumentNullException.ThrowIfNull(scene);
        if (!ReferenceEquals(scene, Music)) throw new ArgumentException("음향과 실제 State의 장면 음악 owner 불일치", nameof(scene));
    }

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
