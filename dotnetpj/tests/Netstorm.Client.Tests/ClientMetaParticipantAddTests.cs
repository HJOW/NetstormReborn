// 0x20 메타 참가자 추가의 원문 관찰·선택·생성·버전·좌표 분기를 대조한다.
// cpppj/src/client/NativeMetaParticipantCreate.cpp 의 ObserveNativeMetaParticipantAdd와
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientMetaParticipantAdd를 대조한다.
// 생성 본체(ConstructNativeMetaParticipant)는 후속 결선에서 잇는다.
// 0.960에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Client.Player;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x20 메타 참가자 추가의 실제 CS·장치 호출을 기록한다.
public sealed class ClientMetaParticipantAddTests
{
    /// <summary>같은 실제 CS와 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 메타맵 모드다.</summary>
        public bool OwnerMode = true;

        /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new();

        /// <summary>같은 실제 메타 예약 owner다.</summary>
        public readonly UIntBox Pending = new();

        /// <summary>같은 실제 메타 갱신 예약 owner다.</summary>
        public readonly UIntBox Refresh = new();

        /// <summary>같은 실제 Core/Player owner다.</summary>
        public readonly ClientCoreDataGlobals Core = new();

        /// <summary>같은 실제 Player 전역이다.</summary>
        public readonly PlayerWorldState Player = new();

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 생성 호출이다.</summary>
        public readonly List<(int Record, int Length)> Constructs = [];

        /// <summary>실제 전투 표시다.</summary>
        public readonly List<uint> Battles = [];

        /// <summary>실제 좌석 계산 횟수다.</summary>
        public int Seats;

        /// <summary>실제 Chat 호출이다.</summary>
        public readonly List<(uint Subscriber, int Length, uint Flags)> Chats = [];

        /// <summary>실제 표시 갱신이다.</summary>
        public readonly List<(uint Participant, uint Mask)> Updates = [];

        /// <summary>실제 NSInfo 갱신 횟수다.</summary>
        public int Refreshes;

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaParticipantAddHooks Hooks() => new()
        {
            Challenge = Challenge,
            OwnerModeChallenge = () => OwnerMode,
            Subscriber = Subscriber,
            MetaPending = Pending,
            MetaRefresh = Refresh,
            CoreData = Core,
            Player = Player,
            Metamap = () => 1,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Construct = (record, packet) => Constructs.Add((record.Length, packet.Length)),
            Battle = id => Battles.Add(id),
            Seat = (_, _, _) => { Seats++; return [10f, 20f]; },
            Chat = (subscriber, name, flags) => Chats.Add((subscriber, name.Length, flags)),
            Update = (participant, mask) => Updates.Add((participant, mask)),
            RefreshInfo = () => Refreshes++,
        };

        /// <summary>원문 0x20 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(byte id, byte subscriber, int length = 0x35)
        {
            byte[] packet = new byte[length];
            packet[1] = 0x20;
            packet[4] = id;
            packet[5] = subscriber;
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantAdd.Receive(null!, new byte[0x35]));
        var bare = new ClientMetaParticipantAddHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantAdd.Receive(bare, new byte[0x35]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantAdd.RefreshCore(null!, 0, false, 0, scene.Core));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantAdd.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantAdd.Install(Dispatch(), null!));
    }

    /// <summary>원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaParticipantAdd.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x20);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(ushort.MaxValue, row.Length);
        Assert.NotNull(row.PacketObserver);
        Assert.Null(row.Observer);
        Assert.Equal(0x004b4710u, dispatch.SnapshotReceiver(0x20)!.NativeEntry);
    }

    /// <summary>수신 방향 관찰기는 버퍼를 읽지 않는다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ObserveReceivingIgnoresBuffer()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(1, 2);
        packet[0x12] = 0xff;
        ClientMetaParticipantAdd.Observe(packet, 0, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>송신 관찰기는 현재 BYTE를 다시 읽어 선언 WORD를 쓴다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ObserveSendingAssertsAndWritesLength()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(1, 2, 0x40);
        packet[0x12] = 3;
        ClientMetaParticipantAdd.Observe(packet, 1, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
        Assert.Equal((ushort)(3 + 0x35), BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        byte[] full = Scene.Packet(1, 2, 0x40);
        full[0x12] = 0xff;
        ClientMetaParticipantAdd.Observe(full, 1, scene.Hooks().Assertion);
        Assert.Single(scene.Assertions);
    }

    /// <summary>메타맵 모드가 아니면 수신 전체를 건너뛴다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ReceiveIgnoresNonChallengeMode()
    {
        var scene = new Scene();
        scene.OwnerMode = false;
        ClientMetaParticipantAdd.Receive(scene.Hooks(), Scene.Packet(1, 2));
        Assert.Empty(scene.Constructs);
        Assert.Equal(0, scene.Refreshes);
    }

    /// <summary>CS가 없으면 진단 뒤 명시 실패한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ReceiveAssertsWithoutCurrent()
    {
        var scene = new Scene();
        var hooks = scene.Hooks();
        hooks.Challenge = new ClientChallengeGlobals();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantAdd.Receive(hooks, Scene.Packet(1, 2)));
        Assert.Single(scene.Assertions);
    }

    /// <summary>자기 구독자는 Core 선택과 예약 증가를 수행한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ReceiveSelectsSelf()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x77;
        ClientMetaParticipantAdd.Receive(scene.Hooks(), Scene.Packet(3, 0x77));
        Assert.Equal(1u, scene.Pending.Value);
        Assert.Equal(1u, scene.Refresh.Value);
        Assert.Single(scene.Constructs);
        Assert.Equal([(3u, 0xffu)], scene.Updates);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>전투 좌석은 기록·버전·좌표를 잇는다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ReceiveAddsBattleSeatAndVersion()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x77;
        scene.Challenge.Battle(2).Span[0] = 1;
        var hooks = scene.Hooks();
        hooks.Construct = (record, _) =>
        {
            record.Span[0x64] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa4, 4), 2);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa8, 4), 3);
        };
        ClientMetaParticipantAdd.Receive(hooks, Scene.Packet(3, 0x99));
        Assert.Equal([0u], scene.Battles);
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x44, 4)));
        Assert.Equal(1, scene.Seats);
        Assert.Equal(BitConverter.SingleToUInt32Bits(10f), BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(3).Span.Slice(0x9c, 4)));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20f), BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(3).Span.Slice(0xa0, 4)));
    }

    /// <summary>이미 찬 좌석은 진단 뒤 덮어쓴다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ReceiveOverwritesOccupiedSlot()
    {
        var scene = new Scene();
        scene.Challenge.Battle(2).Span[0] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x44, 4), 9);
        var hooks = scene.Hooks();
        hooks.Construct = (record, _) =>
        {
            record.Span[0x64] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa4, 4), 2);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa8, 4), 3);
        };
        ClientMetaParticipantAdd.Receive(hooks, Scene.Packet(3, 0x99));
        Assert.Single(scene.Assertions);
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x44, 4)));
    }

    /// <summary>8 이상 좌석·40 이상 참가자는 명시 거부한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void BadSeatAndIdThrow()
    {
        var scene = new Scene();
        var hooks = scene.Hooks();
        hooks.Construct = (record, _) =>
        {
            record.Span[0x64] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa4, 4), 2);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa8, 4), 8);
        };
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantAdd.Receive(hooks, Scene.Packet(3, 0x99)));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantAdd.Receive(scene.Hooks(), Scene.Packet(40, 0x99)));
    }

    /// <summary>후처리는 전투 소유자의 같은 전투 참가자만 다시 쓴다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void RefreshCoreWritesSameBattleOnly()
    {
        var scene = new Scene();
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(7).Span.Slice(0xa4, 4), 2);
        scene.Challenge.Participant(1).Span[0x64] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(1).Span.Slice(0xa4, 4), 2);
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(1).Span.Slice(0xa8, 4), 5);
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(1).Span.Slice(4, 4), 0x111);
        for (int index = 0; index < 10; index++)
        {
            scene.Challenge.Participant(1).Span[0x54 + index] = (byte)(index + 1);
        }

        byte[] before = scene.Core.Records().ToArray();
        ClientMetaParticipantAdd.RefreshCore(scene.Challenge, battleOwner: 7, missionPost: true, playerKnown: 1, core: scene.Core);
        ClientMetaParticipantAdd.RefreshCore(scene.Challenge, battleOwner: 7, missionPost: false, playerKnown: 0, core: scene.Core);
        Assert.Equal(before, scene.Core.Records().ToArray());
        ClientMetaParticipantAdd.RefreshCore(scene.Challenge, battleOwner: 7, missionPost: false, playerKnown: 1, core: scene.Core);
        Assert.NotEqual(before, scene.Core.Records().ToArray());
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x77;
        var dispatch = Dispatch();
        ClientMetaParticipantAdd.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x20, 9, null, Scene.Packet(4, 0x11));
        Assert.Single(scene.Constructs);
        Assert.Equal([(4u, 0xffu)], scene.Updates);
        Assert.Equal(0u, scene.Pending.Value);
    }

    /// <summary>짧은 패킷은 원문 필드·이름 범위에서 거부한다. 0.960에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantAdd.Receive(scene.Hooks(), new byte[5]));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantAdd.Receive(scene.Hooks(), Scene.Packet(1, 2, 0x10)));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantAdd.Observe(new byte[4], 1, (_, _, _) => { }));
    }
}
