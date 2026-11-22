// 원문74h 한 행의 불변F와 현재 실제 Runtime 배송을 연결한다.
namespace Netstorm.Client;

/// <summary>NativeGameplayPacketReceivers.cpp의 데빌메이커 상태 행이다.</summary>
public static partial class NativeGameplayPacketReceivers
{
    /// <summary>74h/0045e4f0/ffff/버전1/NULLobserver만 설치한다.</summary>
    public static void InstallWindAviary(ClientGameplayGlobals game, Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(dispatch);
        dispatch.Observe(0x74, null); dispatch.Version(0x74, 1, 0); dispatch.RequireLength(0x74, 0xffff);
        dispatch.RegisterCanonicalDirect(0x74, 0x0045e4f0, (_, _, packet) =>
        {
            var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedGameplayPacket(0x0045e4f0, packet);
        });
    }
    /// <summary>같은 현재F/metadata를 확인하고 기존 등록 대여에 연결한다.</summary>
    public static void BindWindAviary(Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch); Net.PacketRegistration row = dispatch.Registration(0x74); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(0x74);
        if (receiver is null || receiver.NativeEntry != 0x0045e4f0 || row.Version != 1 || row.Mismatch != 0 || row.Length != 0xffff || row.Observer is not null || row.PacketObserver is not null) throw new InvalidOperationException("Native 데빌메이커의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(0x74, receiver);
    }
}
