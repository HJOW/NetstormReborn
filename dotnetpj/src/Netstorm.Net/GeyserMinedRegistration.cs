using Netstorm.Core.Original;

// 같은 원문5bh handler를 실제 단일 패킷 분배기에 등록한다.
namespace Netstorm.Net;

/// <summary>cpppj RawGeyserMined.cpp의 양판본 등록 helper다.</summary>
public static class GeyserMinedRegistration
{
    /// <summary>원문 NULL observer·version1/mismatch0과 같은 handler를 설치하고 기존 길이는 보존한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawGeyserMined handler)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(handler);
        dispatch.Observe(0x5b, null); dispatch.Version(0x5b, 1, 0); dispatch.Register(0x5b, (_, _, packet) => handler.HandlePacket(packet));
    }
}
