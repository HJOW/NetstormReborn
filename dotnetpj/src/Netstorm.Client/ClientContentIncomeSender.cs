using System.Buffers.Binary;
using Netstorm.Net;

namespace Netstorm.Client; // 내용물 수입을 같은 실제 연결과 Player 기록으로 보낸다.

/// <summary>원본 0040abe0의 현재 수입 패킷과 Source 수명을 연결한다.</summary>
public static class ClientContentIncomeSender
{
    /// <summary>현재 헤더와 할당 바이트를 사용하고 선택된 실제 연결만 호출한다.</summary>
    public static void Send(ClientGameplayGlobals game, ClientSurfaceGlobals surface, ClientConnectionGlobals connections, uint player)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(connections);
        if (!ReferenceEquals(surface.Gameplay, game) || !ReferenceEquals(connections.LocalState.Gameplay, game))
            throw new ArgumentException("내용물 수입의 같은 Client 소유자 필요");
        var header = game.GameAux.ContentIncomeHeader;
        byte[] packet = new byte[8]; ClientAllocationBytes.ReadCurrent(packet);
        packet[0] = (byte)(header.Protocol & 0x7f); packet[1] = header.Message;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), header.Length);
        var registry = connections.Registry; var slots = registry.Cleanup().Transceivers;
        uint token = player < slots.Length ? slots[player] : 0;
        using NativeBattleSource? source = token == 0 ? null : connections.PacketSources.Resolve(registry.Node(token))
            ?? throw new InvalidOperationException("내용물 수입의 실제 Source 수명 미공개");
        _ = registry.SendMutableNative(unchecked((int)player), packet, game.PlayerRecords.Records, surface.Storage.AssertDisplay);
    }
}
