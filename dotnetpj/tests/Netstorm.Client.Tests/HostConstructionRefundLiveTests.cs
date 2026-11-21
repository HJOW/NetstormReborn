using Netstorm.Core.Original;

// 실제 Host의 장기 SP/Player와 건설 환불의 연결을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 자료 구성기를 사용해 환불 본체의 세션 소유권을 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>현재 TYPE/PlayerKnown·로컬 번호와 같은 SP 저장소를 사용하며 반납 뒤 API를 거부한다.</summary>
    [Fact]
    public void ActualConstructionRefundUsesCurrentSpAndPlayerGlobals()
    {
        var (session, types, surface) = SquidPacketScene(); var notices = new List<(uint Player, ConstructionMoneyNotice Notice)>();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { ConstructionMoney = (player, notice) => notices.Add((player, notice)) });
        Assert.Same(session.Pool, session.ConstructionRefund.Pool); types[100].Cost = 37.75f;
        session.Sp.Set(2, BitConverter.SingleToUInt32Bits(100));
        session.ConstructionRefund.Refund(100, 2); Assert.Equal(137f, BitConverter.UInt32BitsToSingle(session.Sp.Get(2)));
        session.MissionPlayer.BattleOwner = 2; session.MissionPlayer.PlayerKnown = 2;
        session.ConstructionRefund.Refund(100, 2); Assert.Equal(137f, BitConverter.UInt32BitsToSingle(session.Sp.Get(2)));
        session.MissionPlayer.PlayerKnown = 0;
        session.ConstructionRefund.Refund(100, 2); Assert.Equal(174f, BitConverter.UInt32BitsToSingle(session.Sp.Get(2)));
        Assert.Equal(3, notices.Count); Assert.All(notices, item => { Assert.Equal(2u, item.Player); Assert.Equal(37, item.Notice.Amount); });
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.ConstructionRefund);
    }
    /// <summary>실제 통지 장치 미설치는 양수 환불의 원문 가산 이후 위치에서 드러나고 너겟 제외는 소비하지 않는다.</summary>
    [Fact]
    public void MissingConstructionMoneyDeviceFailsOnlyWhenSelected()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.MissionPlayer.PlayerKnown = 0; types[100].Cost = 4; session.Sp.Set(1, BitConverter.SingleToUInt32Bits(10));
        Assert.Throws<InvalidOperationException>(() => session.ConstructionRefund.Refund(100, 1)); Assert.Equal(14f, BitConverter.UInt32BitsToSingle(session.Sp.Get(1)));
        session.GameAux.NuggetTypeBox.Value = 100; session.ConstructionRefund.Refund(100, 1); Assert.Equal(14f, BitConverter.UInt32BitsToSingle(session.Sp.Get(1)));
        session.ReleasePlayerProcesses();
    }
}
