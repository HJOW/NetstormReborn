// 원문26F 중3b/3c의 불변 수신 코드와 현재 실제 Session 배송을 연결한다.
namespace Netstorm.Client;

/// <summary>NativeGameplayPacketReceivers.cpp의 두 애니메이션 행이다.</summary>
public static partial class NativeGameplayPacketReceivers
{
    /// <summary>한 행의 실제 메시지/불변 코드/길이/빈 observer 구별이다.</summary>
    /// <param name="Type">원문 메시지 번호다.</param><param name="Entry">원문 불변 수신 코드다.</param><param name="Length">원문 고정 길이 또는ffff다.</param>
    private readonly record struct ArcherAnimationReceiverSpec(uint Type, uint Entry, ushort Length);
    /// <summary>3b 화염과3c 회전의 검토한 두 원문 행이다.</summary>
    private static readonly ArcherAnimationReceiverSpec[] ArcherAnimationReceivers = [new(0x3b, 0x00466d30, 0xffff), new(0x3c, 0x00466b60, 0xffff)];
    /// <summary>두F만 설치하며 다른 표/현재 Session/Player 준비를 건드리지 않는다.</summary>
    public static void InstallArcherAnimation(ClientGameplayGlobals game, Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(dispatch);
        // 원문 NULL 빈 observer를 구분하고 같은 현재 Runtime을 매 배송에서 선택한다.
        foreach (ArcherAnimationReceiverSpec spec in ArcherAnimationReceivers)
        {
            dispatch.Observe(spec.Type, null); dispatch.Version(spec.Type, 1, 0); dispatch.RequireLength(spec.Type, spec.Length);
            dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (_, _, packet) =>
            {
                var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedGameplayPacket(spec.Entry, packet);
            });
        }
    }
    /// <summary>원래 caller 위치에서 같은F/metadata가 일치할 때만 현재 등록 대여에 붙인다.</summary>
    public static void BindArcherAnimation(Net.RawPacketDispatch dispatch, uint type)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        // 검토한 두 행만 선택하며 모르는 번호를 다른F로 넘기지 않는다.
        foreach (ArcherAnimationReceiverSpec spec in ArcherAnimationReceivers)
        {
            if (type != spec.Type) continue; Net.PacketRegistration row = dispatch.Registration(type); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(type);
            if (receiver is null || receiver.NativeEntry != spec.Entry || row.Version != 1 || row.Mismatch != 0 || row.Length != spec.Length || row.Observer is not null || row.PacketObserver is not null)
                throw new InvalidOperationException("Native 애니메이션의 실제 고정F/metadata 불일치");
            dispatch.RegisterCanonical(type, receiver); return;
        }
        throw new ArgumentException("미등록 Native 애니메이션 메시지", nameof(type));
    }
}
