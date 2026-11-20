using Netstorm.Core.Original;

// 실제 서버 metadata 본체를 로그인과 같은 공유 분배기에 설치한다.
namespace Netstorm.Net;

/// <summary>RawMissionServerPackets.cpp의 72h 등록 순서를 공유한다.</summary>
public static class MissionMetadataRegistration
{
    /// <summary>version1/정책0·현재 mutable observer·같은 수신기를 설치하며 기존 원문 길이표를 보존한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawMissionServerPackets packets)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(packets);
        dispatch.Version(0x72, 1, 0); dispatch.ObservePacket(0x72, packets.ObserveMetadata);
        dispatch.Register(0x72, (_, _, packet) => packets.ReceiveMetadata(packet));
    }
}
