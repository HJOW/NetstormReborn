using Netstorm.Core.Original;

// 장기Client의같은실제저장소와불변Native엔트리를전역패킷표에연결한다.
namespace Netstorm.Client;

/// <summary>원문App15등록의설치와현재Session대여를분리한다.</summary>
public static class NativeLoginPacketReceivers
{
    /// <summary>비NULL빈observer와실제mutable버퍼observer를구별한다.</summary>
    private enum ObserverKind
    {
        /// <summary>버퍼를읽지않는비NULL ret4 observer다.</summary>
        Empty,
        /// <summary>27h의현재이름/이미지길이를확정한다.</summary>
        Login,
        /// <summary>2a/2b/67/81h의현재가변길이를확정한다.</summary>
        Client,
        /// <summary>72h의현재metadata길이를확정한다.</summary>
        Metadata,
    }
    /// <summary>원문수신한행의불변코드/길이/observer종류다.</summary>
    /// <param name="Type">패킷번호다.</param>
    /// <param name="Entry">원문코드주소다.</param>
    /// <param name="Length">길이WORD이며ffff는가변이다.</param>
    /// <param name="Observer">원문observer슬롯이다.</param>
    private readonly record struct ReceiverSpec(uint Type, uint Entry, ushort Length, ObserverKind Observer);
    /// <summary>67h는이미설치된App전역F를보존하는원문15행이다.</summary>
    private static readonly ReceiverSpec[] Receivers =
    [
        new(0x27, 0x004a7800, 0xffff, ObserverKind.Login), new(0x63, 0x004a7510, 8, ObserverKind.Empty),
        new(0x29, 0x004b3b40, 34, ObserverKind.Empty), new(0x67, 0x004b4900, 0xffff, ObserverKind.Client),
        new(0x2b, 0x004912d0, 0xffff, ObserverKind.Client), new(0x2a, 0x004b3d30, 0xffff, ObserverKind.Client),
        new(0x28, 0x004b8940, 18, ObserverKind.Empty), new(0x81, 0x004b3e90, 0xffff, ObserverKind.Client),
        new(0x84, 0x004a6f30, 8, ObserverKind.Empty), new(0x6e, 0x004a7050, 8, ObserverKind.Empty),
        new(0x72, 0x004b3bc0, 0xffff, ObserverKind.Metadata), new(0x3d, 0x00491220, 9, ObserverKind.Empty),
        new(0x78, 0x0048f880, 6, ObserverKind.Empty), new(0x52, 0x004a71a0, 5, ObserverKind.Empty),
        new(0x64, 0x004b6180, 5, ObserverKind.Empty),
    ];

    /// <summary>같은장기소유자의14F만설치한다. Session/Player할당/현재모드조회는하지않는다.</summary>
    public static void Install(ClientGameplayGlobals game, ClientConnectionGlobals connections, Net.RawPacketDispatch dispatch,
        Action<string, string, uint> assertion, Net.ConnectionActivityHooks activity)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(assertion);
        if (!ReferenceEquals(connections.LocalState.Gameplay, game)) throw new ArgumentException("고정App의실제Client소유자불일치", nameof(connections));
        var activityBody = new Net.RawConnectionActivity(connections.Activity.Tables, activity);
        // 기존67h를유지하고각F는변하지않는자기Native엔트리만캡처한다.
        foreach (ReceiverSpec spec in Receivers)
        {
            if (spec.Type == 0x67) continue;
            switch (spec.Observer)
            {
                case ObserverKind.Login: dispatch.ObservePacket(spec.Type, (packet, sending) => RawMissionPacketFields.ObserveLogin(packet, sending, assertion)); break;
                case ObserverKind.Client: dispatch.ObservePacket(spec.Type, (packet, sending) => RawMissionPacketFields.ObserveClient(spec.Type, packet, sending, assertion)); break;
                case ObserverKind.Metadata: dispatch.ObservePacket(spec.Type, (packet, sending) => RawMissionPacketFields.ObserveMetadata(packet, sending, expression => assertion(expression, @"\Ns\O\_p_packets.cpp", 0xb03))); break;
                default: dispatch.Observe(spec.Type, static _ => { }); break;
            }
            dispatch.Version(spec.Type, 1, 0); dispatch.RequireLength(spec.Type, spec.Length);
            if (spec.Type == 0x52)
            {
                var players = game.PlayerRecords;
                dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (sender, _, _) => RawBattleLeave.Receive(sender, players.Records));
            }
            else if (spec.Type == 0x64) dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (sender, _, _) => activityBody.Receive(unchecked((int)sender)));
            else dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (sender, node, packet) =>
            {
                var session = game.RequireSession();
                using var runtime = game.PinCurrentRuntime(session);
                session.ReceiveFixedLoginPacket(spec.Entry, sender, node, packet);
            });
        }
    }

    /// <summary>현재고정F와검토한metadata가같을때만이번등록대여에같은F를붙인다.</summary>
    public static void Bind(Net.RawPacketDispatch dispatch, uint type)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        // 알려진실제행만선택하고누락/ordinary수신기/NULLobserver를명시거부한다.
        foreach (ReceiverSpec expected in Receivers)
        {
            if (expected.Type != type) continue;
            Net.PacketRegistration row = dispatch.Registration(type);
            Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(type);
            bool observerMatches = expected.Observer == ObserverKind.Empty
                ? row.Observer is not null && row.PacketObserver is null : row.PacketObserver is not null && row.Observer is null;
            if (receiver is null || receiver.NativeEntry != expected.Entry || row.Version != 1 || row.Mismatch != 0 || row.Length != expected.Length || !observerMatches)
                throw new InvalidOperationException("고정App의실제F/metadata불일치");
            dispatch.RegisterCanonical(type, receiver); return;
        }
        throw new ArgumentException("미등록고정App패킷번호", nameof(type));
    }
}
