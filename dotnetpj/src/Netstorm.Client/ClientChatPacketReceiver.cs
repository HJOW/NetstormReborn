using System.Buffers.Binary;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;
using Netstorm.Net;

// 장기 Client의 현재 연결/Player/마스크를41h의 고정 원문 F에 연결한다.
namespace Netstorm.Client;

/// <summary>ClientChatPacketReceiver.cpp의 설치 부모다.짧은 Session/World를 캡처하지 않는다.</summary>
public static class ClientChatPacketReceiver
{
    /// <summary>현재 Player의 지정 DWORD만 읽으며 active/known 조건을 추가하지 않는다.</summary>
    private static uint Subscriber(ReadOnlySpan<byte> bytes, int at)
    {
        if (at < 0 || at > bytes.Length || bytes.Length - at < 4) throw new ArgumentOutOfRangeException(nameof(at), "Chat 현재 Player subscriber 물리 범위"); return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(at, 4));
    }
    /// <summary>같은 장기 Client 장치만 등록하며 실제 UI는 비영 현재 창에서만 요구한다.</summary>
    public static void Install(ClientGameplayGlobals game, ClientSurfaceGlobals surface, ClientConnectionGlobals connections,
        ClientChatWindowGlobals globals, RawPacketDispatch dispatch, Action<Memory<byte>>? receive)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(connections); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(dispatch);
        if (!ReferenceEquals(surface.Gameplay, game) || !ReferenceEquals(connections.LocalState.Gameplay, game)) throw new ArgumentException("41h의 같은 실제 Client 소유자 필요");
        var hooks = new ChatRelayHooks
        {
            Read = address => address switch
            {
                0x594fbc => surface.Storage.OwnerMode.Battle ? 1u : 0,
                0x540bc0 => game.Authority.Server ? 1u : 0,
                0x540c74 => (game.Subscribers ?? throw new InvalidOperationException("41h 현재 실제 Subscriber 소유자 없음")).MySubscriber.Value,
                _ => throw new ArgumentOutOfRangeException(nameof(address), "41h 현재 전역 주소 범위"),
            },
            Address = sender => connections.Registry.Address(unchecked((int)sender)),
            Player = subscriber =>
            {
                if (subscriber == 0) surface.Storage.AssertDisplay("subId != INVALID_SUBSCRIBER_ID", @"\Ns\O\Player.cpp", 0xa5); ReadOnlyMemory<byte> players = game.PlayerRecords.Records;
                // 같은 현재9행에서 첫 subscriber 일치만 찾는다.
                for (uint owner = 1; owner < 9; owner++) if (Subscriber(players.Span, (int)owner * 180 + 44) == subscriber) return owner;
                return 0;
            },
            Channel = owner =>
            {
                if (surface.Storage.OwnerMode.Challenge) surface.Storage.AssertDisplay("!inMetamapMode", @"\Ns\O\Player.cpp", 0x134); if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "41h 현재 Player 번호 물리 범위"); return (int)(owner & 255);
            },
            Connected = id =>
            {
                var current = connections.Registry.Cleanup().Transceivers; if (id < 0 || id >= current.Length) throw new ArgumentOutOfRangeException(nameof(id), "41h 현재 연결 번호 범위"); uint token = current[id]; return token != 0 && (connections.Registry.Node(token).Flags & 0x40) == 0;
            },
            Send = (id, packet) => connections.Registry.SendMutableNative(id, packet, game.PlayerRecords.Records, surface.Storage.AssertDisplay),
            Receive = (packet, _) => { if (globals.Current.Value == 0) return; (receive ?? throw new InvalidOperationException("41h 현재 실제 Chat UI 없음"))(packet); },
            Assertion = line => surface.Storage.AssertDisplay(line == 0x81d ? "nameLen != (short)-1" : line == 0x81e ? "bufferLen != (short)-1" : "addrLen != (unsigned char)-1", @"\Ns\O\_p_packets.cpp", line),
        };
        ChatRelayBody.RegisterCanonicalChatPacket(dispatch, globals.ReceivedMask, hooks);
    }
}
