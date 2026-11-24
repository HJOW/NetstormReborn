// 0x71 전투 설명 수신의 원문 observer·버전·길이와 CS0 관문·고정 F 설치를 대조한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaDescriptionReceiver(0x71 0x004b49c0)와 o/RawStateMetaBattleDescription.cpp 의 RegisterStateMetaBattleDescription(0040b0d0)을 대조한다.
// 0.943에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x71 분배 설치의 실제 분배기·CS·진단 호출을 기록한다.
public sealed class ClientMetaBattleDescriptionReceiverTests
{
    /// <summary>같은 실제 CS·분배기·진단과 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();
        /// <summary>같은 실제 장기 진단 owner다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>같은 장기 분배기다.</summary>
        public readonly RawPacketDispatch Dispatch;
        /// <summary>실제 수신한 송신자 번호다.</summary>
        public readonly List<uint> Senders = [];
        /// <summary>실제 수신한 패킷 사본이다.</summary>
        public readonly List<byte[]> Packets = [];

        /// <summary>같은 실제 CS·분배기·진단을 준비한다.</summary>
        public Scene()
        {
            var game = new ClientGameplayGlobals();
            Surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0);
            Dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        }

        /// <summary>같은 실제 수신 입구다.</summary>
        public void Receive(uint sender, ReadOnlyMemory<byte> packet)
        {
            Senders.Add(sender);
            Packets.Add(packet.ToArray());
        }
    }

    /// <summary>원문 observer·버전·길이와 고정 F를 설치한다. 0.943에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, scene.Receive);
        var row = scene.Dispatch.Registration(0x71);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(0xffff, row.Length);
        Assert.NotNull(row.PacketObserver);
        Assert.Null(row.Observer);
        var saved = scene.Dispatch.SnapshotReceiver(0x71);
        Assert.NotNull(saved);
        Assert.Equal(0x004b49c0u, saved.NativeEntry);
    }

    /// <summary>송신 관찰기는 길이 BYTE에 10을 더해 선언 WORD를 쓰고 수신 방향은 버퍼를 읽지 않는다. 0.943에서 추가됐다.</summary>
    [Fact]
    public void ObserverWritesSendingLengthAndIgnoresReceivingDirection()
    {
        var scene = new Scene();
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, scene.Receive);
        var row = scene.Dispatch.Registration(0x71);
        byte[] outgoing = new byte[12];
        outgoing[9] = 5;
        row.NotifyObserver(outgoing, 1);
        Assert.Equal(15, BinaryPrimitives.ReadUInt16LittleEndian(outgoing.AsSpan(2, 2)));
        byte[] incoming = new byte[12];
        incoming[9] = 5;
        row.NotifyObserver(incoming, 0);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(incoming.AsSpan(2, 2)));
        Assert.Throws<ArgumentOutOfRangeException>(() => row.NotifyObserver(new byte[3], 1));
    }

    /// <summary>길이 BYTE가 ff이면 원문 진단 식·파일·줄을 기록한다. 0.943에서 추가됐다.</summary>
    [Fact]
    public void ObserverAssertsFfDescByte()
    {
        var scene = new Scene();
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, scene.Receive);
        var row = scene.Dispatch.Registration(0x71);
        int calls = 0;
        scene.Surface.BindDisplayDiagnostics((expression, file, line) =>
        {
            calls++;
            Assert.Equal("descLen != (unsigned char)-1", expression);
            Assert.Equal(@"\Ns\O\_p_packets.cpp", file);
            Assert.Equal(0xae3u, line);
        });
        byte[] outgoing = new byte[12];
        outgoing[9] = 0xff;
        row.NotifyObserver(outgoing, 1);
        Assert.Equal(1, calls);
        Assert.Equal(265, BinaryPrimitives.ReadUInt16LittleEndian(outgoing.AsSpan(2, 2)));
    }

    /// <summary>CS0이면 버퍼·연결을 요구하지 않고 자식을 부르지 않는다. 0.943에서 추가됐다.</summary>
    [Fact]
    public void GateSkipsReceiveWhenCurrentZero()
    {
        var scene = new Scene();
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, scene.Receive);
        Assert.Equal(0u, scene.Challenge.Current);
        byte[] packet = new byte[12];
        packet[1] = 0x71;
        scene.Dispatch.InvokeDirect(0x71, 7, null, packet);
        Assert.Empty(scene.Senders);
        Assert.Empty(scene.Packets);
    }

    /// <summary>CS가 비영이면 자식을 부르고 재설치는 같은 장기 F를 대여한다. 0.943에서 추가됐다.</summary>
    [Fact]
    public void GateReceivesWhenCurrentSetAndReinstallReusesBaseline()
    {
        var scene = new Scene();
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, scene.Receive);
        Assert.True(scene.Challenge.Prepare());
        byte[] packet = new byte[12];
        packet[1] = 0x71;
        packet[4] = 3;
        scene.Dispatch.InvokeDirect(0x71, 7, null, packet);
        Assert.Equal([7u], scene.Senders);
        Assert.Equal(packet, scene.Packets.Single());
        var saved = scene.Dispatch.SnapshotReceiver(0x71);
        int secondCalls = 0;
        ClientMetaBattleDescriptionReceiver.Install(scene.Dispatch, scene.Challenge, scene.Surface.Storage, (_, _) => secondCalls++);
        Assert.Same(saved, scene.Dispatch.SnapshotReceiver(0x71));
        scene.Dispatch.InvokeDirect(0x71, 9, null, packet);
        Assert.Equal([7u, 9u], scene.Senders);
        Assert.Equal(0, secondCalls);
    }
}
