// 0x23 메타 전투 생성의 원문 기록 쓰기·공장·표시 분기를 대조한다.
// cpppj/src/client/NativeMetaBattleCreate.cpp 의 ConstructNativeMetaBattle/ReceiveNativeMetaBattleAdd를 대조한다.
// 0.958에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x23 메타 전투 생성의 실제 CS·공장·표시 호출을 기록한다.
public sealed class ClientMetaBattleCreateTests
{
    /// <summary>같은 실제 CS와 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 로그 호출이다.</summary>
        public readonly List<string> Logs = [];

        /// <summary>실제 전투 TYPE 조회 횟수다.</summary>
        public int TypeQueries;

        /// <summary>실제 생성 호출이다.</summary>
        public readonly List<(uint Type, uint Flags)> Creates = [];

        /// <summary>다음 객체 번호다.</summary>
        public uint NextObject = 0x5000;

        /// <summary>실제 객체 물리다.</summary>
        public readonly Dictionary<uint, byte[]> Objects = [];

        /// <summary>실제 종류 지정이다.</summary>
        public readonly List<(uint Object, uint Kind)> Kinds = [];

        /// <summary>실제 배치 호출이다.</summary>
        public readonly List<(uint Object, float X, float Y, uint Flags)> Pops = [];

        /// <summary>실제 애니메이션 호출이다.</summary>
        public readonly List<(uint Object, uint Delay)> Animates = [];

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaBattleCreateHooks Hooks() => new()
        {
            Challenge = Challenge,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Log = text => Logs.Add(text),
            BattleType = () => { TypeQueries++; return 126; },
            Create = (type, flags) => { Creates.Add((type, flags)); uint created = NextObject; NextObject++; Objects[created] = new byte[50]; return created; },
            Raw = created => Objects[created],
            SetKind = (created, kind) => Kinds.Add((created, kind)),
            Pop = (created, x, y, flags) => Pops.Add((created, x, y, flags)),
            Animate = (created, delay) => Animates.Add((created, delay)),
        };

        /// <summary>원문 0x23 20BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(byte id, byte existing, uint word, ushort tag, uint x, uint y)
        {
            byte[] packet = new byte[20];
            packet[1] = 0x23;
            packet[4] = id;
            packet[5] = existing;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(6, 4), word);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(10, 2), tag);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(0xc, 4), x);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(0x10, 4), y);
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleCreate.Receive(null!, new byte[20]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleCreate.Construct(new byte[292], new(1, new byte[6], 0, 0, 0), null!));
        var bare = new ClientMetaBattleCreateHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaBattleCreate.Receive(bare, new byte[20]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleCreate.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleCreate.Install(Dispatch(), null!));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleCreate.Construct(new byte[292], new(1, new byte[5], 0, 0, 0), scene.Hooks()));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaBattleCreate.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x23);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(20, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b4640u, dispatch.SnapshotReceiver(0x23)!.NativeEntry);
    }

    /// <summary>이미 있으면 진단 뒤 반환한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveLogsExistingBattle()
    {
        var scene = new Scene();
        scene.Challenge.Battle(2).Span[0] = 1;
        ClientMetaBattleCreate.Receive(scene.Hooks(), Scene.Packet(2, 0, 0x11223344, 0x5566, 0, 0));
        Assert.Single(scene.Logs);
        Assert.Empty(scene.Creates);
        Assert.Equal(0, scene.TypeQueries);
    }

    /// <summary>생성은 기록·공장·표시·Rig 순서로 효과를 둔다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveConstructsBattle()
    {
        var scene = new Scene();
        uint x = BitConverter.SingleToUInt32Bits(1.0f);
        uint y = BitConverter.SingleToUInt32Bits(2.0f);
        ClientMetaBattleCreate.Receive(scene.Hooks(), Scene.Packet(3, 0, 0x11223344, 0x5566, x, y));
        Assert.Empty(scene.Logs);
        Assert.Empty(scene.Assertions);
        Memory<byte> record = scene.Challenge.Battle(3);
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(4, 4)));
        Assert.Equal(0x11223344u, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(8, 4)));
        Assert.Equal(0x5566u, BinaryPrimitives.ReadUInt16LittleEndian(record.Span.Slice(0xc, 2)));
        Assert.Equal(x, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x10, 4)));
        Assert.Equal(1, record.Span[0]);
        Assert.Equal(y, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x14, 4)));
        Assert.True(record.Span.Slice(0x38, 32).ToArray().All(value => value == 0));
        Assert.Equal(0xffffffffu, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x18, 4)));
        Assert.Equal(0x100u, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x1c, 4)));
        Assert.Equal([(126u, 2u)], scene.Creates);
        Assert.Equal([(0x5000u, 3u)], scene.Kinds);
        Assert.Equal(0, scene.Objects[0x5000][8]);
        Assert.Equal(0, scene.Objects[0x5000][9]);
        Assert.Equal(0, scene.Objects[0x5000][0x1a]);
        Assert.Equal([(0x5000u, 1.0f, 2.0f, 0u)], scene.Pops);
        Assert.Equal(0x5000u, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x20, 4)));
        Assert.Equal([(0x5000u, 10u)], scene.Animates);
    }

    /// <summary>기존 전투는 애니메이션을 요구하지 않는다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveSkipsAnimateWhenExisting()
    {
        var scene = new Scene();
        ClientMetaBattleCreate.Receive(scene.Hooks(), Scene.Packet(3, 2, 0, 0, 0, 0));
        Assert.Single(scene.Creates);
        Assert.Empty(scene.Animates);
    }

    /// <summary>무효 기록에도 진단을 남기고 쓰기를 계속한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ConstructAssertsInvalidRecord()
    {
        var scene = new Scene();
        byte[] record = new byte[292];
        record[0] = 1;
        ClientMetaBattleCreate.Construct(record, new(1, new byte[6], 0, 0, 0), scene.Hooks());
        Assert.Single(scene.Assertions);
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(4, 4)));
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaBattleCreate.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x23, 9, null, Scene.Packet(4, 0, 0xaa, 0xbb, 0, 0));
        Assert.Single(scene.Creates);
        Assert.Equal(4u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(4).Span.Slice(4, 4)));
    }

    /// <summary>짧은 패킷·잘못된 번호는 원문 필드·기록 범위에서 거부한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsAndBadIdsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleCreate.Receive(scene.Hooks(), new byte[4]));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleCreate.Receive(scene.Hooks(), Scene.Packet(10, 0, 0, 0, 0, 0)));
    }
}
