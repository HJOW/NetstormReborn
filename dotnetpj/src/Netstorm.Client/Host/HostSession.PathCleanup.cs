using Netstorm.Core.Original;

// 경로 helper 정리가 같은 실제 건설 비용·SP·Host를 사용하도록 연결한다.
namespace Netstorm.Client.Host;

/// <summary>실제 건설 환불과 경로 정리 어댑터의 Host 소유권이다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 TYPE/Player/SP를 빌리는 건설 환불 본체다.</summary>
    private RawConstructionPlace? _constructionRefund;
    /// <summary>설치 전과 세션 반납 뒤의 환불 호출을 거부한다.</summary>
    public RawConstructionPlace ConstructionRefund => FixedPacketNeed(_constructionRefund);
    /// <summary>현재 장기 SP·PlayerKnown·로컬 번호·너겟 TYPE에 환불 본체를 설치한다.</summary>
    private void BindConstructionRefundDevices()
    {
        if (_constructionRefund is not null) return;
        var state = new ConstructionPlaceState
        {
            SharedLocalPlayer = () => MissionPlayer.BattleOwner, SharedPlayerKnown = () => MissionPlayer.PlayerKnown,
            SharedNuggetType = GameAux.NuggetTypeBox,
        };
        ConstructionPlaceHooks hooks = RawConstructionPlace.MakeHooks(Pool, Reward, Sp, state, new()
        {
            SendMoney = (player, notice) => FixedPacketNeed(_squidPacketEffects?.ConstructionMoney)(player, notice),
        });
        _constructionRefund = new(Pool, FixedPacketNeed(_terrainPopTypes), state, hooks);
    }
    /// <summary>현재 실제 carrier/anchor/Host/환불을 연결한다. 의식·예약·가상 삭제는 호출자가 실제 장치를 제공해야 한다.</summary>
    public RawPathCleanup BuildPathCleanupWorldDevices(RawCarrierPreDestroy carrier, RawPlayerPlacementAnchor anchor, PathCleanupState state, PathCleanupHooks effects)
    {
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new ObjectDisposedException(nameof(HostSession));
        return new(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), state,
            RawPathCleanup.MakeHooks(Pool, carrier, anchor, PlayerProcesses, ConstructionRefund, effects));
    }
    /// <summary>실제 프로세스 정리 뒤 세션의 환불 참조만 반납한다.</summary>
    private void ReleaseConstructionRefundDevices() => _constructionRefund = null;
}
