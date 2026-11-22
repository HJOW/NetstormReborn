// 원문38h 한 행의 불변F와 현재 실제 Runtime 배송을 연결한다.
namespace Netstorm.Client;

/// <summary>NativeGameplayPacketReceivers.cpp의 훨리베이스 상태 행이다.</summary>
public static partial class NativeGameplayPacketReceivers
{
    /// <summary>38h/0045e410/ffff/버전1/NULLobserver만 설치한다.</summary>
    public static void InstallSunAviary(ClientGameplayGlobals game, Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(dispatch);
        dispatch.Observe(0x38, null); dispatch.Version(0x38, 1, 0); dispatch.RequireLength(0x38, 0xffff);
        dispatch.RegisterCanonicalDirect(0x38, 0x0045e410, (_, _, packet) =>
        {
            var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedGameplayPacket(0x0045e410, packet);
        });
    }
    /// <summary>같은 현재F/metadata를 확인하고 기존 등록 대여에 연결한다.</summary>
    public static void BindSunAviary(Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch); Net.PacketRegistration row = dispatch.Registration(0x38); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(0x38);
        if (receiver is null || receiver.NativeEntry != 0x0045e410 || row.Version != 1 || row.Mismatch != 0 || row.Length != 0xffff || row.Observer is not null || row.PacketObserver is not null) throw new InvalidOperationException("Native 훨리베이스의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(0x38, receiver);
    }
}
