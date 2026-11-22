using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 기존 섬 정리 작성기를 새 기대값 없이 원본 전체20BYTE와 직접 비교한다.
namespace Netstorm.Client.Tests;

/// <summary>실제35h의 독립336관측·경계·같은 원격 전용 방송을 검증한다.</summary>
public sealed class DropIslandPacketNativeTests
{
    /// <summary>원문 좌표 변환 결과를 입력으로 공급하며 전체 패킷/뒤쪽 canary를 대조한다.</summary>
    [Fact]
    public void ExistingWriterMatchesAll336NativePackets()
    {
        int count = 0;
        // 두 정밀도 결과는 기대값으로만 읽고 좌표를 C# 결과로 다시 만들지 않는다.
        foreach (string[] row in X86Fixture.Read("drop-island-packet"))
        {
            Assert.Equal(5, row.Length); string[] input = row[0].Split(','); Assert.Equal(5, input.Length);
            byte seed = (byte)X86Fixture.UInt(input[4]); byte[] packet = Enumerable.Repeat(seed, 24).ToArray();
            BattlePacketWriter.WriteDropIslandPacket(packet, X86Fixture.UInt(row[1]), X86Fixture.UInt(row[2]), X86Fixture.UInt(input[2]), X86Fixture.UInt(input[3]));
            Assert.Equal(row[3], row[4]); Assert.Equal(X86Fixture.Bytes(row[3]), packet[..20]); Assert.Equal(Enumerable.Repeat(seed, 4).ToArray(), packet[20..]); ++count;
        }
        Assert.Equal(336, count);
    }
    /// <summary>0..19바이트의 모든 부족 용량을 부분 헤더 쓰기 전에 거부한다.</summary>
    [Fact]
    public void ShortBuffersAreRejectedWithoutPartialWrites()
    {
        // 길이별 caller 저장소를 독립적으로 보존한다.
        for (int length = 0; length < BattlePacketWriter.PacketLength; ++length)
        {
            byte[] packet = Enumerable.Repeat((byte)0xa5, length).ToArray();
            Assert.Throws<ArgumentOutOfRangeException>(() => BattlePacketWriter.WriteDropIslandPacket(packet, 1, 2, 3, 4));
            Assert.Equal(Enumerable.Repeat((byte)0xa5, length).ToArray(), packet);
        }
    }
    /// <summary>이미 쓴35h를 같은 실제 방송기가 로컬을 제외한 연결에만 보낸다. 수신을 로컬에 되먹이지 않는다.</summary>
    [Fact]
    public void NativeDropPacketBroadcastsOnlyToConnectedRemotePeers()
    {
        byte[] players = new byte[9 * 180]; Action<byte[]>?[] peers = new Action<byte[]>?[9]; int own = 0, remote = 0;
        players[180 + 0x50] = players[2 * 180 + 0x50] = players[3 * 180 + 0x50] = 1;
        peers[1] = _ => ++own; peers[2] = packet => { Assert.Equal(X86Fixture.Bytes("01351400214365870000ffff5010000008000000"), packet); ++remote; };
        peers[4] = _ => throw new InvalidOperationException("비접속 송신 금지"); byte[] packet = new byte[20];
        BattlePacketWriter.WriteDropIslandPacket(packet, 0x87654321, 0xffff0000, 0x1050, 8);
        new RawBattleBroadcast(OriginalEdition.Patch1078, players, peers, () => 1, () => true).Send(packet);
        Assert.Equal(0, own); Assert.Equal(1, remote);
    }
}
