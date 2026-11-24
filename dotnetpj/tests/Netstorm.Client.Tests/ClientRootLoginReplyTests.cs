// 0x0f Root 로그인의 원문 observer·버전·서버 목록·존·친구·phase 분기를 대조한다.
// cpppj/src/o/RawRootLoginReply.cpp 의 ObserveRootLoginReply/ReceiveRootLoginReply와
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientRootReply를 대조한다.
// 0.953에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x0f Root 로그인의 실제 상태·기록·버전·정리·값·목록·저장 호출을 기록한다.
public sealed class ClientRootLoginReplyTests
{
    /// <summary>같은 실제 상태 블록과 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
        public readonly LocalMissionStateStorage State = new()
        {
            Old = new byte[0x90],
            Phase = new byte[0x90],
            Immediate = new byte[0x90],
            Current = new byte[0x90],
            Next = new byte[0x90],
        };

        /// <summary>같은 실제 51×144BYTE 기록이다.</summary>
        public readonly byte[] Zones = new byte[RawMetaZoneCleanup.Bytes];

        /// <summary>같은 실제 갱신 예약 owner다.</summary>
        public readonly UIntBox Refresh = new();

        /// <summary>받은 버전 쌍이다.</summary>
        public readonly List<(uint Major, uint Minor)> Versions = [];

        /// <summary>버전 결과다.</summary>
        public bool VersionResult = true;

        /// <summary>실제 정리 호출 횟수다.</summary>
        public int Cleared;

        /// <summary>실제 값 갱신이다.</summary>
        public readonly List<uint> Checksums = [];

        /// <summary>실제 서버 행 사본이다.</summary>
        public readonly List<byte[]> Servers = [];

        /// <summary>실제 저장 호출 횟수다.</summary>
        public int Saved;

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientRootLoginReplyHooks Hooks() => new()
        {
            State = State,
            Zones = Zones,
            Refresh = Refresh,
            Version = (major, minor) => { Versions.Add((major, minor)); return VersionResult; },
            ClearZones = () => { Cleared++; Zones.AsSpan().Fill(0); },
            Checksum = value => Checksums.Add(value),
            Server = row => Servers.Add(row.ToArray()),
            SaveServers = () => Saved++,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
        };

        /// <summary>현재 phase를 쓴다.</summary>
        public void Phase(uint value) => BinaryPrimitives.WriteUInt32LittleEndian(State.Phase.Span.Slice(0, 4), value);

        /// <summary>원문 0x0f 최소 22BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(uint major, uint minor, int length = 32)
        {
            byte[] packet = new byte[length];
            packet[1] = 0x0f;
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)major));
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), unchecked((ushort)minor));
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientRootLoginReply.Receive(null!, new byte[32]));
        var bare = new ClientRootLoginReplyHooks();
        Assert.Throws<InvalidOperationException>(() => ClientRootLoginReply.Receive(bare, new byte[32]));
        Assert.Throws<ArgumentNullException>(() => ClientRootLoginReply.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientRootLoginReply.Install(Dispatch(), null!));
    }

    /// <summary>원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientRootLoginReply.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x0f);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(ushort.MaxValue, row.Length);
        Assert.NotNull(row.PacketObserver);
        Assert.Null(row.Observer);
        Assert.Equal(0x004b4250u, dispatch.SnapshotReceiver(0x0f)!.NativeEntry);
    }

    /// <summary>수신 방향 관찰기는 버퍼를 읽지 않는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ObserveReceivingIgnoresBuffer()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(10, 78);
        packet[10] = 0xff;
        ClientRootLoginReply.Observe(packet, 0, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>송신 관찰기는 세 sentinel을 진단하고 선언 WORD를 쓴다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ObserveSendingAssertsSentinelsAndWritesLength()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(10, 78);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12, 4), 10);
        packet[10] = 2;
        packet[11] = 3;
        var assertions = new List<(string, string, uint)>();
        ClientRootLoginReply.Observe(packet, 1, (expression, file, line) => assertions.Add((expression, file, line)));
        Assert.Empty(assertions);
        Assert.Equal((ushort)(10 + 20 + 2 * 47 + 3 * 5), BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));

        byte[] full = Scene.Packet(10, 78);
        full.AsSpan().Fill(0xff);
        ClientRootLoginReply.Observe(full, 1, (expression, file, line) => assertions.Add((expression, file, line)));
        Assert.Equal(3, assertions.Count);
    }

    /// <summary>버전 거부는 next12/+50만 쓰고 목록·존을 건드리지 않는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void VersionRejectionWritesNext12Only()
    {
        var scene = new Scene();
        scene.VersionResult = false;
        ClientRootLoginReply.Receive(scene.Hooks(), Scene.Packet(9, 0));
        Assert.Equal([(9u, 0u)], scene.Versions);
        Assert.Equal(12u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x50, 4)));
        Assert.Equal(0, scene.Cleared);
        Assert.Empty(scene.Checksums);
        Assert.Equal(0, scene.Saved);
    }

    /// <summary>빈 목록 수신은 정리·값·저장과 갱신 예약을 수행한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void EmptyListsClearZonesAndBumpRefresh()
    {
        var scene = new Scene();
        scene.Phase(0);
        byte[] packet = Scene.Packet(10, 78);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), 0x1234);
        ClientRootLoginReply.Receive(scene.Hooks(), packet);
        Assert.Equal([(10u, 78u)], scene.Versions);
        Assert.Equal(1, scene.Cleared);
        Assert.Equal([0x1234u], scene.Checksums);
        Assert.Empty(scene.Servers);
        Assert.Equal(1, scene.Saved);
        Assert.Equal(1u, scene.Refresh.Value);
    }

    /// <summary>서버 문자열은 개수·길이 순서로 소비한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ServerStringsAreConsumedInOrder()
    {
        var scene = new Scene();
        scene.Phase(0);
        byte[] packet = Scene.Packet(10, 78, 40);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12, 4), 5);
        packet[20] = 0;
        packet[21] = 1;
        packet[22] = 0;
        packet[23] = 3;
        packet[24] = (byte)'a';
        packet[25] = (byte)'b';
        packet[26] = (byte)'c';
        ClientRootLoginReply.Receive(scene.Hooks(), packet);
        byte[] row = Assert.Single(scene.Servers);
        Assert.Equal((byte)'a', row[0]);
        Assert.Equal((byte)'b', row[1]);
        Assert.Equal((byte)'c', row[2]);
        Assert.Equal(0, row[3]);
    }

    /// <summary>존 기록은 47BYTE 형식으로 쓰고 이름은 NUL까지 복사한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ZoneRecordsUse47ByteLayout()
    {
        var scene = new Scene();
        scene.Phase(0);
        byte[] packet = Scene.Packet(10, 78, 100);
        packet[10] = 1;
        packet[20] = 5;
        packet[21] = 6;
        packet[22] = 7;
        packet[23] = (byte)'x';
        packet[24] = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(63, 4), 0x55aa);
        ClientRootLoginReply.Receive(scene.Hooks(), packet);
        Assert.Equal(5u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Zones.AsSpan(144, 4)));
        Assert.Equal(6u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Zones.AsSpan(148, 4)));
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Zones.AsSpan(152, 4)));
        Assert.Equal(0x55aau, BinaryPrimitives.ReadUInt32LittleEndian(scene.Zones.AsSpan(276, 4)));
        Assert.Equal((byte)'x', scene.Zones[236]);
        Assert.Equal(0, scene.Zones[237]);
    }

    /// <summary>친구는 빈 0번을 포함해 첫 존 번호 일치에 넣는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void FriendsMatchFirstZoneNumber()
    {
        var scene = new Scene();
        scene.Phase(0);
        byte[] packet = Scene.Packet(10, 78, 30);
        packet[11] = 1;
        packet[20] = 0;
        packet[21] = 0x34;
        packet[22] = 0x12;
        ClientRootLoginReply.Receive(scene.Hooks(), packet);
        Assert.Equal(0x1234u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Zones.AsSpan(12, 4)));
    }

    /// <summary>phase 23이면 next24/+34를 쓰고 아니면 갱신을 올린다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void Phase23WritesNext24OtherwiseBumpsRefresh()
    {
        var scene = new Scene();
        scene.Phase(23);
        byte[] packet = Scene.Packet(10, 78);
        packet[9] = 0xfe;
        ClientRootLoginReply.Receive(scene.Hooks(), packet);
        Assert.Equal(24u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
        Assert.Equal(unchecked((uint)(sbyte)0xfe), BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x34, 4)));
        Assert.Equal(0u, scene.Refresh.Value);
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientRootLoginReply.Receive(scene.Hooks(), new byte[5]));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientRootLoginReply.Observe(new byte[10], 1, (_, _, _) => { }));
    }
}
