// 실제 자산 상태를 Chat/수신 소비자가 현재 조회한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession::BattleActive의 동일 자산 상태 조회다.</summary>
public sealed partial class HostSession
{
    /// <summary>00594fbc 자산 관문을 현재 읽는다.표시 phase/독립 OwnerMode 사본으로 대신하지 않는다.</summary>
    public uint BattleActive() => (_vortexAssetState ?? throw new InvalidOperationException("Chat 현재 자산 Battle 상태 미설치")).InBattle ? 1u : 0;
}
