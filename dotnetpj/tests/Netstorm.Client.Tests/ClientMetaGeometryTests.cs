// 0x26 메타 geometry의 원문 observer·버전·한계·원점/반경 분기를 대조한다.
// cpppj/src/client/NativeMetaGeometry.cpp 의 ObserveNativeMetaGeometry/ReceiveNativeMetaGeometry를 대조한다.
// 0.954에서 R4-6 전송·수신 묶음용으로 추가됐다.
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x26 메타 geometry의 실제 좌표·모드·한계 호출을 기록한다.
public sealed class ClientMetaGeometryTests
{
    /// <summary>같은 실제 좌표·모드·한계·진단 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 최초 배치 좌표다.</summary>
        public readonly ChallengeGeometry Geometry = new();

        /// <summary>같은 실제 모드 owner다.</summary>
        public readonly UIntBox Mode = new();

        /// <summary>실제 한계 호출이다.</summary>
        public readonly List<(float Width, float Height)> Limits = [];

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaGeometryHooks Hooks() => new()
        {
            Geometry = Geometry,
            Mode = Mode,
            Limits = (width, height) => Limits.Add((width, height)),
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
        };

        /// <summary>원문 0x26 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(ushort modLen, byte contactLen, byte width, byte height)
        {
            byte[] packet = new byte[64];
            packet[1] = 0x26;
            System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), modLen);
            packet[6] = contactLen;
            packet[7] = width;
            packet[8] = height;
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaGeometry.Receive(null!, new byte[16]));
        var bare = new ClientMetaGeometryHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaGeometry.Receive(bare, new byte[16]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaGeometry.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaGeometry.Install(Dispatch(), null!));
    }

    /// <summary>원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaGeometry.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x26);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(ushort.MaxValue, row.Length);
        Assert.NotNull(row.PacketObserver);
        Assert.Null(row.Observer);
        Assert.Equal(0x004b45a0u, dispatch.SnapshotReceiver(0x26)!.NativeEntry);
    }

    /// <summary>수신 방향 관찰기는 버퍼를 읽지 않는다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ObserveReceivingIgnoresBuffer()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(0xffff, 0xff, 0, 0);
        ClientMetaGeometry.Observe(packet, 0, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>송신 관찰기는 두 미작성 길이를 진단하고 현재 길이로 WORD를 쓴다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ObserveSendingAssertsAndWritesLength()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(4, 5, 0, 0);
        ClientMetaGeometry.Observe(packet, 1, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
        Assert.Equal((ushort)(4 + 5 + 9), System.Buffers.Binary.BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));

        byte[] full = Scene.Packet(0xffff, 0xff, 0, 0);
        var assertions = new List<(string, string, uint)>();
        ClientMetaGeometry.Observe(full, 1, (expression, file, line) => assertions.Add((expression, file, line)));
        Assert.Equal(2, assertions.Count);
    }

    /// <summary>contact 셋 이하는 한계·mode 0만 쓰고 좌표를 건드리지 않는다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ReceiveShortContactsWritesLimitsAndClearsMode()
    {
        var scene = new Scene();
        scene.Mode.Value = 7;
        scene.Geometry.OriginX = 1;
        byte[] packet = Scene.Packet(2, 8, 100, 50);
        ClientMetaGeometry.Receive(scene.Hooks(), packet);
        Assert.Single(scene.Limits);
        Assert.Equal(100f, scene.Limits[0].Width);
        Assert.Equal((float)((double)50 + (double)BitConverter.UInt32BitsToSingle(0x411ba2e9u)), scene.Limits[0].Height);
        Assert.Equal(0u, scene.Mode.Value);
        Assert.Equal(1f, scene.Geometry.OriginX);
    }

    /// <summary>contact 넷 이상은 원점과 두 반경을 쓰고 mode 1을 둔다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ReceiveLongContactsWritesGeometryAndSetsMode()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(2, 16, 10, 20);
        BitConverter.GetBytes(1.5f).CopyTo(packet, 11);
        BitConverter.GetBytes(2.5f).CopyTo(packet, 15);
        BitConverter.GetBytes(3.5f).CopyTo(packet, 19);
        BitConverter.GetBytes(4.5f).CopyTo(packet, 23);
        ClientMetaGeometry.Receive(scene.Hooks(), packet);
        Assert.Equal(1.5f, scene.Geometry.OriginX);
        Assert.Equal(2.5f, scene.Geometry.OriginY);
        Assert.Equal(3.5f, scene.Geometry.FirstRadius);
        Assert.Equal(4.5f, scene.Geometry.SecondRadius);
        Assert.Equal(1u, scene.Mode.Value);
    }

    /// <summary>첫 세 DWORD일 때 둘째 반경의 오래된 값을 보존한다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ReceiveThreeContactsPreservesStaleSecondRadius()
    {
        var scene = new Scene();
        scene.Geometry.SecondRadius = 9.25f;
        byte[] packet = Scene.Packet(2, 12, 10, 20);
        BitConverter.GetBytes(1.5f).CopyTo(packet, 11);
        BitConverter.GetBytes(2.5f).CopyTo(packet, 15);
        BitConverter.GetBytes(3.5f).CopyTo(packet, 19);
        ClientMetaGeometry.Receive(scene.Hooks(), packet);
        Assert.Equal(3.5f, scene.Geometry.FirstRadius);
        Assert.Equal(9.25f, scene.Geometry.SecondRadius);
        Assert.Equal(1u, scene.Mode.Value);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaGeometry.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x26, 9, null, Scene.Packet(2, 8, 30, 40));
        Assert.Single(scene.Limits);
        Assert.Equal(30f, scene.Limits[0].Width);
        Assert.Equal(0u, scene.Mode.Value);
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.954에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaGeometry.Receive(scene.Hooks(), new byte[5]));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaGeometry.Observe(new byte[3], 1, (_, _, _) => { }));
    }
}
