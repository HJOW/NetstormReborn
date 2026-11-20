using Netstorm.Core.Original;

// 클라이언트 실제8종 수신을 로그인과 같은 공유표에 설치한다.
namespace Netstorm.Net;

/// <summary>RawMissionClientPackets.cpp의 버전/observer/고정F 등록 부모다.</summary>
public static class MissionClientPacketRegistration
{
    /// <summary>원문 길이표를 유지하고 각 타입의 실제 수신기와 같은 mutable observer를 설치한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawMissionClientPackets packets)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(packets);
        ReadOnlySpan<(uint Type, uint Entry)> types = [(0x29, 0x004b3b40), (0x67, 0x004b4900), (0x2b, 0x004912d0), (0x2a, 0x004b3d30), (0x28, 0x004b8940), (0x81, 0x004b3e90), (0x84, 0x004a6f30), (0x6e, 0x004a7050)];
        // 선택한 type/entry는 각 등록의 값으로 고정하며 단순 빈 원문 observer도 별도로 보존한다.
        foreach ((uint type, uint entry) in types)
        {
            if (type is 0x2a or 0x2b or 0x67 or 0x81) dispatch.ObservePacket(type, (packet, sending) => packets.Observe(type, packet, sending));
            else dispatch.Observe(type, static _ => { });
            dispatch.Version(type, 1, 0); dispatch.Register(type, (sender, _, packet) => packets.ReceiveFixed(entry, sender, packet));
        }
    }
}
