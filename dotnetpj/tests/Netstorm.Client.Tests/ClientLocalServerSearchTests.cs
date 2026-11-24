// 0x66 로컬 서버 검색의 송신 패킷 작성과 검색 기한·재송신 순서를 대조한다.
// cpppj/src/o/RawLocalServerDiscovery.cpp 의 MakeLocalServerDiscoveryPacket/AnnounceLocalServer/BroadcastLocalServerDiscovery/BeginLocalServerDiscovery/StartLocalServerSearch/ContinueLocalServerSearch 를 대조한다.
// 0.950에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 004b3fc0/004b4050/State34·35 검색의 실제 패킷·기한·장치 호출을 기록한다.
public sealed class ClientLocalServerSearchTests
{
    /// <summary>같은 실제 상태·토큰·기한과 호출을 기록하는 실제 장치다.</summary>
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
        /// <summary>같은 실제 검색 토큰 owner다.</summary>
        public readonly UIntBox Token = new();
        /// <summary>같은 실제 검색 만료 기한 owner다.</summary>
        public readonly DoubleBox SearchEnd = new();
        /// <summary>같은 실제 재송신 기한 owner다.</summary>
        public readonly DoubleBox RetryEnd = new();
        /// <summary>현재 벽시각이다.</summary>
        public double Wall = 100.0;
        /// <summary>실제 열기 횟수다.</summary>
        public int Opens;
        /// <summary>실제 송신한 패킷 사본이다.</summary>
        public readonly List<byte[]> Sent = [];
        /// <summary>실제 안내 호출 횟수다.</summary>
        public int Progressed;
        /// <summary>실제 팔레트 호출 횟수다.</summary>
        public int Paletted;
        /// <summary>실제 서버 창 존재 결과다.</summary>
        public bool WindowExists;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientLocalServerSearchHooks Hooks() => new()
        {
            State = State,
            Token = Token,
            SearchEnd = SearchEnd,
            RetryEnd = RetryEnd,
            Wall = () => Wall,
            Open = () => Opens++,
            Broadcast = packet => Sent.Add((byte[])packet.Clone()),
            Progress = () => Progressed++,
            Palette = () => Paletted++,
            ServerWindowExists = () => WindowExists,
        };
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        var bare = new ClientLocalServerSearchHooks();
        Assert.Throws<InvalidOperationException>(() => ClientLocalServerSearch.Broadcast(bare));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerSearch.Broadcast(null!));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerSearch.Begin(null!, new CrtRandom()));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerSearch.Begin(scene.Hooks(), null!));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerSearch.Announce(0, null!));
    }

    /// <summary>원문 header 뒤 두 DWORD만 caller 값으로 쓰는 12BYTE를 작성한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void MakePacketWritesHeaderAndPayloads()
    {
        byte[] packet = ClientLocalServerSearch.MakePacket(1, 0x4444);
        Assert.Equal(12, packet.Length);
        Assert.Equal((byte)1, packet[0]);
        Assert.Equal((byte)0x66, packet[1]);
        Assert.Equal((byte)12, packet[2]);
        Assert.Equal((byte)0, packet[3]);
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4, 4)));
        Assert.Equal(0x4444u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(8, 4)));
    }

    /// <summary>같은 패킷을 세번 전달한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void AnnounceSendsSamePacketThreeTimes()
    {
        var scene = new Scene();
        ClientLocalServerSearch.Announce(6799, scene.Hooks().Broadcast!);
        Assert.Equal(3, scene.Sent.Count);
        foreach (byte[] packet in scene.Sent)
        {
            Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4, 4)));
            Assert.Equal(6799u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(8, 4)));
        }
    }

    /// <summary>현재 토큰을 명령0으로 한 번 송신한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void BroadcastSendsCurrentTokenOnce()
    {
        var scene = new Scene();
        scene.Token.Value = 0x1234;
        ClientLocalServerSearch.Broadcast(scene.Hooks());
        var sent = Assert.Single(scene.Sent);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(4, 4)));
        Assert.Equal(0x1234u, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(8, 4)));
    }

    /// <summary>열기·재시드·네 번 lowBYTE 토큰 저장·첫 송신을 순서대로 수행한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void BeginOpensReseedsStoresTokenAndSends()
    {
        var scene = new Scene();
        var random = new CrtRandom();
        ClientLocalServerSearch.Begin(scene.Hooks(), random);
        Assert.Equal(1, scene.Opens);
        uint token = scene.Token.Value;
        Assert.NotEqual(0u, token);
        var sent = Assert.Single(scene.Sent);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(4, 4)));
        Assert.Equal(token, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(8, 4)));
    }

    /// <summary>안내→첫 검색→5초 기한→재송신 기한→팔레트→즉시35를 수행한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void StartRunsFullSequenceWithDeadlines()
    {
        var scene = new Scene();
        scene.Wall = 100.0;
        var random = new CrtRandom(17);
        ClientLocalServerSearch.Start(scene.Hooks(), random);
        Assert.Equal(1, scene.Progressed);
        Assert.Equal(1, scene.Paletted);
        Assert.Equal(105.0, scene.SearchEnd.Value);
        Assert.InRange(scene.RetryEnd.Value, 100.5, 100.599 + 1e-9);
        Assert.Equal(35u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Immediate.Span.Slice(0, 4)));
    }

    /// <summary>재송신 기한이 지나면 다시 송신하고 기한을 갱신한다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void ContinueRebroadcastsWhenRetryDue()
    {
        var scene = new Scene();
        scene.Wall = 200.0;
        scene.RetryEnd.Value = 100.0;
        scene.SearchEnd.Value = 300.0;
        var random = new CrtRandom(23);
        ClientLocalServerSearch.Continue(scene.Hooks(), random);
        Assert.Single(scene.Sent);
        Assert.InRange(scene.RetryEnd.Value, 200.5, 200.599 + 1e-9);
        Assert.Equal(35u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
    }

    /// <summary>만료 때 창이 없으면 즉시37·다음35를 쓰고 돌아간다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void ContinueExpiresToImmediate37WhenWindowMissing()
    {
        var scene = new Scene();
        scene.Wall = 200.0;
        scene.RetryEnd.Value = 500.0;
        scene.SearchEnd.Value = 150.0;
        scene.WindowExists = false;
        ClientLocalServerSearch.Continue(scene.Hooks(), new CrtRandom(29));
        Assert.Equal(0.0, scene.SearchEnd.Value);
        Assert.Equal(37u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Immediate.Span.Slice(0, 4)));
        Assert.Equal(35u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
        Assert.Empty(scene.Sent);
    }

    /// <summary>만료 때 창이 있으면 4초 유예 뒤 다음35를 둔다. 0.950에서 추가됐다.</summary>
    [Fact]
    public void ContinueDefersFourSecondsWhenWindowExists()
    {
        var scene = new Scene();
        scene.Wall = 200.0;
        scene.RetryEnd.Value = 500.0;
        scene.SearchEnd.Value = 150.0;
        scene.WindowExists = true;
        ClientLocalServerSearch.Continue(scene.Hooks(), new CrtRandom(31));
        Assert.Equal(204.0, scene.SearchEnd.Value);
        Assert.Equal(35u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Immediate.Span.Slice(0, 4)));
    }
}
