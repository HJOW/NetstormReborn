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

    /// <summary>원문 PathCleanup 상위 설치의 실제 예약·의식·가상 삭제 효과를 같은 세션 장치에 연결한다.</summary>
    /// <returns>MakeHooks가 채우는 조회·환불과 합칠 상위 효과다. 가상 삭제는 호출자가 VirtualDestroy로 덮을 수 있다.</returns>
    /// <remarks>2026-11-25 1.034에서 WP-1 Cleanup 상위 설치용으로 추가했다. 근거: cpppj src/client/RawCampaignMotionBindings.cpp 39~44행의 cleanupHooks 조립과 RawCampaignSession.cpp 3819행의 ritual 결선이다. 예약은 Dais 의식과 같은 항상 성공이며 의식은 세션 신전 의식 본체다. Rune 생성·HP·소유자·배치·위치 음향 하위는 R6에서 잇는다.</remarks>
    public PathCleanupHooks BuildPathCleanupRootHooks()
    {
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("Path 정리 상위 프로세스 미연결");
        Action<int, uint> destroy = BuildSessionDestroy("Path 정리", processes);
        RawTempleRitual ritual = TempleRitual;
        return new PathCleanupHooks
        {
            ReserveShared = static () => true,
            Ritual = (temple, parent, mode) => ritual.Ritual(temple, parent, mode),
            Destroy = (sid, flags) => destroy(sid, flags),
        };
    }
    /// <summary>실제 프로세스 정리 뒤 세션의 환불 참조만 반납한다.</summary>
    private void ReleaseConstructionRefundDevices() => _constructionRefund = null;
}
