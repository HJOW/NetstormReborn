using Netstorm.Core.Original;

// 실제 단일 분배기에 가변 길이 개발자 수신과 송신 관찰기를 설치한다.
namespace Netstorm.Net;

/// <summary>RawDeveloperCommandPacket.cpp의31h/version1/정책0 등록이다.</summary>
public static class DeveloperCommandRegistration
{
    /// <summary>같은 실제 명령 실행기를 저장하며 수신에서 송신 길이 관찰을 실행하지 않는다.</summary>
    public static void Register(RawPacketDispatch dispatch, Action<ReadOnlyMemory<byte>> execute, Action<string, string, uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(execute);
        dispatch.Version(0x31, 1, 0); dispatch.RequireLength(0x31, 0xffff);
        dispatch.ObservePacket(0x31, (packet, sending) => RawDeveloperCommandPacket.Observe(packet, sending, assertion));
        dispatch.Register(0x31, (_, _, packet) => RawDeveloperCommandPacket.Receive(packet, execute));
    }
}
