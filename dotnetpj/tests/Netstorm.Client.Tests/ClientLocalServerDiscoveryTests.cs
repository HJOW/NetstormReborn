// 0x66 로컬 서버 발견의 현재 블록 직접 쓰기와 경쟁 검색 signed 비교를 대조한다.
// cpppj/src/o/RawLocalServerDiscovery.cpp 의 ReceiveLocalServerDiscovery(004b8a10)를 대조한다.
// 0.949에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 004b8a10 로컬 서버 발견의 실제 상태·토큰·기한·벽시각·주소·부모 호출을 기록한다.
public sealed class ClientLocalServerDiscoveryTests
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
        /// <summary>실제 벽시각이다.</summary>
        public double Wall = 100.0;
        /// <summary>실제 주소 바이트다.</summary>
        public byte[] Address = [1, 2, 3, 4];
        /// <summary>실제 부모 호출 횟수다.</summary>
        public int Processed;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientLocalServerDiscoveryHooks Hooks() => new()
        {
            State = State,
            Token = Token,
            SearchEnd = SearchEnd,
            Wall = () => Wall,
            PeerAddress = _ => Address,
            ProcessState = () => Processed++,
        };

        /// <summary>원문 0x66 12BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(uint command, uint value)
        {
            byte[] packet = new byte[12];
            packet[1] = 0x66;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), command);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), value);
            return packet;
        }
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerDiscovery.Receive(null!, 0, new byte[12]));
        var bare = new ClientLocalServerDiscoveryHooks();
        Assert.Throws<InvalidOperationException>(() => ClientLocalServerDiscovery.Receive(bare, 0, new byte[12]));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerDiscovery.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientLocalServerDiscovery.Install(new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } }), null!));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientLocalServerDiscovery.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x66);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(12, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b8a10u, dispatch.SnapshotReceiver(0x66)!.NativeEntry);
    }

    /// <summary>명령1은 현재36·주소·포트를 쓰고 부모를 호출한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void CommandOneWritesCurrentAddressPortAndRunsParent()
    {
        var scene = new Scene();
        ClientLocalServerDiscovery.Receive(scene.Hooks(), 9, Scene.Packet(1, 0x4444));
        Assert.Equal(36u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(new byte[] { 1, 2, 3, 4, 0 }, scene.State.Current.Span.Slice(8, 5).ToArray());
        Assert.Equal(0x4444u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0x30, 4)));
        Assert.Equal(1, scene.Processed);
        Assert.Equal(0.0, scene.SearchEnd.Value);
    }

    /// <summary>주소가 현재 블록에 넘치면 쓰기 전에 거부한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void OversizedAddressThrowsBeforeWrite()
    {
        var scene = new Scene();
        scene.Address = new byte[0x90 - 8];
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientLocalServerDiscovery.Receive(scene.Hooks(), 9, Scene.Packet(1, 0)));
        Assert.Equal(0, scene.Processed);
    }

    /// <summary>signed가 작은 토큰일 때만 검색을 12초 연장한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void SmallerSignedTokenExtendsSearch()
    {
        var scene = new Scene();
        scene.Token.Value = 100;
        ClientLocalServerDiscovery.Receive(scene.Hooks(), 9, Scene.Packet(0, 99));
        Assert.Equal(112.0, scene.SearchEnd.Value);
        Assert.Equal(0, scene.Processed);
    }

    /// <summary>signed가 크거나 같은 토큰은 검색 기한을 바꾸지 않는다. 원문은 부호 있는 비교이므로 0x80000000은 음수로 취급한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void LargerOrEqualSignedTokenKeepsSearchEnd()
    {
        var scene = new Scene();
        scene.Token.Value = 100;
        ClientLocalServerDiscovery.Receive(scene.Hooks(), 9, Scene.Packet(0, 100));
        ClientLocalServerDiscovery.Receive(scene.Hooks(), 9, Scene.Packet(0, 200));
        Assert.Equal(0.0, scene.SearchEnd.Value);
        Assert.Equal(0, scene.Processed);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientLocalServerDiscovery.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x66, 9, null, Scene.Packet(1, 0x2222));
        Assert.Equal(36u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(0x2222u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0x30, 4)));
        Assert.Equal(1, scene.Processed);
    }

    /// <summary>짧은 패킷은 원문 DWORD 폭에서 거부한다. 0.949에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketThrows()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientLocalServerDiscovery.Receive(scene.Hooks(), 0, new byte[11]));
    }
}
