using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 장기 CS만 보유하는7bh 공통 고정 수신기를 연결한다.
namespace Netstorm.Client;

/// <summary>ClientCampaignTransport.cpp의7bh 기본 등록이다. 현재 Session이나 연결을 선조회하지 않는다.</summary>
public static class ClientChallengePacketDefaults
{
    /// <summary>같은 CS owner/현재 진단으로 observer와 고정F004b4790·version1/policy0/ffff를 설치한다.</summary>
    public static void Install(RawPacketDispatch dispatch, ClientChallengeGlobals challenge, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(challenge); ArgumentNullException.ThrowIfNull(storage);
        dispatch.ObservePacket(0x7b, (packet, sending) => ClientPacketDefaultObservers.Observe(0x7b, packet, sending, storage.AssertDisplay));
        dispatch.Version(0x7b, 1, 0); dispatch.RequireLength(0x7b, 0xffff);
        dispatch.RegisterCanonicalDirect(0x7b, 0x004b4790, (_, _, packet) => challenge.ReceivePeerAddress(packet));
    }
}
