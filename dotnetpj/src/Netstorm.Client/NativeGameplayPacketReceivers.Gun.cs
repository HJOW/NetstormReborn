// 원문26F 중 이식된3ah의 불변 수신 코드/현재 실제 Session 배송만 연결한다.
namespace Netstorm.Client;

/// <summary>NativeGameplayPacketReceivers.cpp의3ah행이다. 전체26행 설치 완료를 뜻하지 않는다.</summary>
public static partial class NativeGameplayPacketReceivers
{
    /// <summary>총 상태의 실제 고정 메시지 번호다.</summary>
    private const uint GunMessage = 0x3a;
    /// <summary>현재 packet종류로 다시 선택하지 않는 원문 수신 코드다.</summary>
    private const uint GunEntry = 0x0046a0c0;
    /// <summary>다른 전역 표를 보존하며 NULL observer/버전1/정책0/가변 길이의 실제F만 설치한다.</summary>
    public static void InstallGun(ClientGameplayGlobals game, Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(dispatch);
        dispatch.Observe(GunMessage, null); dispatch.Version(GunMessage, 1, 0); dispatch.RequireLength(GunMessage, 0xffff);
        dispatch.RegisterCanonicalDirect(GunMessage, GunEntry, (_, _, packet) =>
        {
            var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedGameplayPacket(GunEntry, packet);
        });
    }
    /// <summary>원래 caller 위치에서 같은F/metadata만 확인해 등록 대여에 붙인다.</summary>
    public static void BindGun(Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch); Net.PacketRegistration row = dispatch.Registration(GunMessage); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(GunMessage);
        if (receiver is null || receiver.NativeEntry != GunEntry || row.Version != 1 || row.Mismatch != 0 || row.Length != 0xffff || row.Observer is not null || row.PacketObserver is not null)
            throw new InvalidOperationException("Native Gun의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(GunMessage, receiver);
    }
}
