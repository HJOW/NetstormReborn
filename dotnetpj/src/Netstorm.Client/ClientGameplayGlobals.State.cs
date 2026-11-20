using Netstorm.Core.Original;

// State와 실제 미션 소비자가 공유하는 장기 Client 필드를 유지한다.
namespace Netstorm.Client;

/// <summary>월드 교체로 새로 만들지 않는 실제 Native 모드와 표시 DWORD다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>현재 두 Subscriber·편집·미션 후처리 모드의 같은 소유자다.</summary>
    public CampaignWorldGlobals World { get; } = new();
    /// <summary>005949d4의 같은 미니맵 갱신 DWORD이며 실제 지형 소비자가 직접 쓴다.</summary>
    public UIntBox MapRevision { get; } = new();
    /// <summary>0059a8ac의 현재 건물 숨김 DWORD이며 State 정리로 같은 값을 쓴다.</summary>
    public UIntBox HideBuildings { get; } = new();
}
