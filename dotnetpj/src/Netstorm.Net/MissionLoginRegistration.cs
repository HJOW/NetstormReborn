using Netstorm.Core.Original;

// 실제 PacketConnection을 형식 그대로 Core 로그인 부모에 전달한다.
namespace Netstorm.Net;

/// <summary>RawMissionLogin.cpp의27h/63h 원문 등록 부모다.</summary>
public static class MissionLoginRegistration
{
    /// <summary>현재 같은 표에 버전1/정책0·원문 NULL observer와 실제 로그인/재접속 수신기를 설치한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawMissionLogin<PacketConnection> login)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(login);
        dispatch.Observe(0x27, null); dispatch.Version(0x27, 1, 0); dispatch.Register(0x27, (pending, connection, packet) => login.Receive(unchecked((int)pending), connection, packet));
        dispatch.Observe(0x63, null); dispatch.Version(0x63, 1, 0); dispatch.RequireLength(0x63, 8); dispatch.Register(0x63, (pending, _, packet) => login.ReceiveReconnect(unchecked((int)pending), packet));
    }
}
