// 0x1d 챌린지 로그인 응답의 필드·버전 검사와 동기 Run 순서를 대조한다.
// cpppj/src/o/RawChallengeLoginReply.cpp 의 ReceiveChallengeLoginReply(004b8ac0)를 대조한다.
// 0.947에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 004b8ac0 로그인 응답의 실제 진단·버전·Run·상태·구독자 호출을 기록한다.
public sealed class ClientChallengeLoginReplyTests
{
    /// <summary>같은 실제 상태·구독자와 호출을 기록하는 실제 장치다.</summary>
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
        /// <summary>같은 실제 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new();
        /// <summary>실제 진단 기록이다.</summary>
        public readonly List<string> Logs = [];
        /// <summary>실제 버전 검사 인자다.</summary>
        public readonly List<(uint Major, uint Minor)> Versions = [];
        /// <summary>실제 버전 검사 결과다.</summary>
        public bool VersionResult = true;
        /// <summary>실제 Run 호출 횟수다.</summary>
        public int Runs;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientChallengeLoginReplyHooks Hooks() => new()
        {
            Log = text => Logs.Add(text),
            Version = (major, minor) => { Versions.Add((major, minor)); return VersionResult; },
            Run = () => Runs++,
            State = State,
            Subscriber = Subscriber,
        };

        /// <summary>원문 0x1d 13BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(uint major, uint minor, uint subscriber, byte code)
        {
            byte[] packet = new byte[13];
            packet[1] = 0x1d;
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)major));
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), unchecked((ushort)minor));
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), subscriber);
            packet[12] = code;
            return packet;
        }
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientChallengeLoginReply.Receive(null!, new byte[13]));
        var bare = new ClientChallengeLoginReplyHooks();
        Assert.Throws<InvalidOperationException>(() => ClientChallengeLoginReply.Receive(bare, new byte[13]));
        bare.Log = _ => { };
        Assert.Throws<InvalidOperationException>(() => ClientChallengeLoginReply.Receive(bare, new byte[13]));
        Assert.Throws<ArgumentNullException>(() => ClientChallengeLoginReply.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientChallengeLoginReply.Install(new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } }), null!));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientChallengeLoginReply.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x1d);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(13, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b8ac0u, dispatch.SnapshotReceiver(0x1d)!.NativeEntry);
    }

    /// <summary>수락 버전은 로그·구독자·28/코드를 쓰고 즉시 Run한다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void AcceptedVersionKeepsCode28AndRuns()
    {
        var scene = new Scene();
        ClientChallengeLoginReply.Receive(scene.Hooks(), Scene.Packet(10, 78, 0x4444, 3));
        Assert.Equal(["Received ZChalLoginReply, code=3\n"], scene.Logs);
        Assert.Equal(0x4444u, scene.Subscriber.Value);
        Assert.Equal(28u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0x38, 4)));
        Assert.Equal([(10u, 78u)], scene.Versions);
        Assert.Equal(1, scene.Runs);
    }

    /// <summary>거부 버전도 Run하되 현재 블록을 12/0으로 되돌린다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void RejectedVersionResetsTo12AndZeroButRuns()
    {
        var scene = new Scene();
        scene.VersionResult = false;
        ClientChallengeLoginReply.Receive(scene.Hooks(), Scene.Packet(11, 79, 0x5555, 7));
        Assert.Equal(["Received ZChalLoginReply, code=7\n"], scene.Logs);
        Assert.Equal(0x5555u, scene.Subscriber.Value);
        Assert.Equal(12u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0x38, 4)));
        Assert.Equal([(11u, 79u)], scene.Versions);
        Assert.Equal(1, scene.Runs);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본체를 수행한다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientChallengeLoginReply.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x1d, 9, null, Scene.Packet(10, 78, 0x6666, 5));
        Assert.Equal(0x6666u, scene.Subscriber.Value);
        Assert.Equal(28u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(1, scene.Runs);
    }

    /// <summary>짧은 패킷은 원문 폭에서 거부한다. 0.947에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketThrows()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientChallengeLoginReply.Receive(scene.Hooks(), new byte[12]));
    }
}
