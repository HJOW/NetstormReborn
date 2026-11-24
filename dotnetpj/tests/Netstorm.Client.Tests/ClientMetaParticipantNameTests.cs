// 0x88 메타 참가자 이름의 원문 등급 합계·조회·이름표·수신 분기를 대조한다.
// cpppj/src/client/NativeMetaParticipantName.cpp 의 Write/Rebuild/Refresh/Receive와
// cpppj/src/o/RawPlayerNotification.cpp 의 CoreTechnologyWeight 를 대조한다.
// 0.955에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x88 메타 참가자 이름의 실제 CS·등급·번역·표시 호출을 기록한다.
public sealed class ClientMetaParticipantNameTests
{
    /// <summary>같은 실제 CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>실제 번역 호출이다.</summary>
        public readonly List<string> Translates = [];

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
        public ClientMetaParticipantNameHooks Hooks(byte level = 42) => new()
        {
            Challenge = Challenge,
            Translate = text => { Translates.Add(text); return text; },
            Weight = _ => level,
            Update = (participant, mask) => Updates.Add((participant, mask)),
            RefreshInfo = () => Refreshes++,
        };

        /// <summary>실제 참가자 기록에 이름·구독자·활성을 쓴다.</summary>
        public void Participant(uint index, uint subscriber, string name, bool active = true)
        {
            Memory<byte> record = Challenge.Participant(index);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), subscriber);
            record.Span[0x64] = (byte)(active ? 1 : 0);
            byte[] body = System.Text.Encoding.ASCII.GetBytes(name);
            body.CopyTo(record.Span.Slice(0x26));
            record.Span[0x26 + body.Length] = 0;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>460BYTE 참가자 기록을 준비한다.</summary>
    private static byte[] Record() => new byte[RawChallengeState.ParticipantBytes];

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantName.Receive(null!, new byte[98]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantName.Rebuild(new byte[460], null!));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantName.Refresh(null!));
        var bare = new ClientMetaParticipantNameHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantName.Receive(bare, new byte[98]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantName.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantName.Install(Dispatch(), null!));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaParticipantName.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x88);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(98, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b4680u, dispatch.SnapshotReceiver(0x88)!.NativeEntry);
    }

    /// <summary>기술 비트 합계에 level+1과 Core+5를 더한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void WeightSumsLevelsPlusBase()
    {
        byte[] core = new byte[10];
        BinaryPrimitives.WriteUInt32LittleEndian(core.AsSpan(6, 4), 0b101);
        core[5] = 7;
        byte weight = ClientMetaCoreWeight.Weight(core, () => 2, index => index switch
        {
            0 => (0, 10),
            _ => (2, 20),
        });
        Assert.Equal(unchecked((byte)(11 + 21 + 7)), weight);
        Assert.Equal(0, ClientMetaCoreWeight.Weight(new byte[10], () => 0, _ => (0, 0)));
    }

    /// <summary>짧은 Core와 미연결 조회는 명시 실패한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void WeightRejectsShortCoreAndMissingDevices()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaCoreWeight.Weight(new byte[9], () => 0, _ => (0, 0)));
        byte[] needsCount = new byte[10];
        needsCount[6] = 1;
        Assert.Throws<InvalidOperationException>(() => ClientMetaCoreWeight.Weight(needsCount, null!, _ => (0, 0)));
        byte[] single = new byte[10];
        single[6] = 1;
        Assert.Throws<InvalidOperationException>(() => ClientMetaCoreWeight.Weight(single, () => 1, null!));
    }

    /// <summary>구독자 조회는 배열 순서의 처음 일치를 반환한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void FindParticipantBySubscriberReturnsFirstMatch()
    {
        var scene = new Scene();
        scene.Participant(0, 0xaaa, "a");
        scene.Participant(2, 0xaaa, "b");
        scene.Participant(5, 0xbbb, "c");
        Assert.Equal(0u, scene.Challenge.FindParticipantBySubscriber(0xaaa));
        Assert.Equal(5u, scene.Challenge.FindParticipantBySubscriber(0xbbb));
        Assert.Null(scene.Challenge.FindParticipantBySubscriber(0xccc));
    }

    /// <summary>이름표는 길드·경고·이름·등급·순위를 서식에 넣는다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void WriteLabelFormatsFields()
    {
        var scene = new Scene();
        byte[] record = Record();
        record[0x58] = 4;
        System.Text.Encoding.ASCII.GetBytes("Bob").CopyTo(record.AsSpan(0x26));
        ClientMetaParticipantName.Write(record, "~aTag:", scene.Hooks(42));
        string label = System.Text.Encoding.Latin1.GetString(record.AsSpan(0xd8));
        label = label[..label.IndexOf('\0')];
        Assert.Equal("~j~aTag:~wBob\n~yLevel ~w42 ~yRank ~w5\n", label);
        Assert.Single(scene.Translates);
    }

    /// <summary>NB09: 긴 길드 태그는 끝에서 줄이고 형식을 모르면 생략한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void WriteLabelTrimsGuildTagNb09()
    {
        var scene = new Scene();
        byte[] record = Record();
        System.Text.Encoding.ASCII.GetBytes("Bob").CopyTo(record.AsSpan(0x26));
        string tag = "~a" + new string('G', 300) + ":";
        ClientMetaParticipantName.Write(record, tag, scene.Hooks());
        string label = System.Text.Encoding.Latin1.GetString(record.AsSpan(0xd8));
        label = label[..label.IndexOf('\0')];
        Assert.StartsWith("~j~a", label, StringComparison.Ordinal);
        Assert.EndsWith("\n", label, StringComparison.Ordinal);
        Assert.True(label.Length < 244);

        byte[] plain = Record();
        System.Text.Encoding.ASCII.GetBytes("Bob").CopyTo(plain.AsSpan(0x26));
        ClientMetaParticipantName.Write(plain, new string('Z', 300), scene.Hooks());
        string cleared = System.Text.Encoding.Latin1.GetString(plain.AsSpan(0xd8));
        cleared = cleared[..cleared.IndexOf('\0')];
        Assert.StartsWith("~j~wBob", cleared, StringComparison.Ordinal);

        byte[] overflow = Record();
        System.Text.Encoding.ASCII.GetBytes(new string('N', 200)).CopyTo(overflow.AsSpan(0x26));
        overflow[0x26 + 200] = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantName.Write(overflow, string.Empty, scene.Hooks()));
    }

    /// <summary>빈 +8이면 접두사를 지운다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void RebuildClearsPrefixWhenEmpty()
    {
        var scene = new Scene();
        byte[] record = Record();
        System.Text.Encoding.ASCII.GetBytes("Al").CopyTo(record.AsSpan(0x26));
        ClientMetaParticipantName.Rebuild(record, scene.Hooks());
        string label = System.Text.Encoding.Latin1.GetString(record.AsSpan(0xd8));
        label = label[..label.IndexOf('\0')];
        Assert.StartsWith("~j~wAl", label, StringComparison.Ordinal);
    }

    /// <summary>수신은 이름을 복사하고 이름표·표시·NSInfo를 갱신한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void ReceiveCopiesNameAndRefreshes()
    {
        var scene = new Scene();
        scene.Participant(3, 0x777, "Old");
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(3).Span.Slice(0, 4), 7);
        byte[] packet = new byte[98];
        packet[1] = 0x88;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 0x777);
        packet[8] = (byte)'N';
        packet[9] = (byte)'e';
        packet[10] = 0;
        ClientMetaParticipantName.Receive(scene.Hooks(), packet);
        Assert.Equal((byte)'N', scene.Challenge.Participant(3).Span[8]);
        Assert.Equal((byte)'e', scene.Challenge.Participant(3).Span[9]);
        Assert.Equal(0, scene.Challenge.Participant(3).Span[10]);
        Assert.Equal([(7u, 0xffu)], scene.Updates);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>비활성 기록은 입력 이름도 읽지 않는다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void ReceiveInactiveReturns()
    {
        var scene = new Scene();
        scene.Participant(3, 0x777, "Old", active: false);
        ClientMetaParticipantName.Receive(scene.Hooks(), new byte[98]);
        Assert.Empty(scene.Updates);
        Assert.Equal(0, scene.Refreshes);
    }

    /// <summary>없는 구독자는 호스트 오류로 거부한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void ReceiveUnknownSubscriberThrows()
    {
        var scene = new Scene();
        byte[] packet = new byte[98];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 0x999);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantName.Receive(scene.Hooks(), packet));
    }

    /// <summary>전체 갱신은 활성 기록만 다시 만든다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void RefreshRebuildsOnlyActive()
    {
        var scene = new Scene();
        scene.Participant(1, 0x111, "One");
        scene.Participant(2, 0x222, "Two");
        scene.Participant(3, 0x333, "Three", active: false);
        ClientMetaParticipantName.Refresh(scene.Hooks());
        Assert.Equal([(1u, 0xffu), (2u, 0xffu)], scene.Updates);
        Assert.Equal(2, scene.Refreshes);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Participant(4, 0x444, "Old");
        var dispatch = Dispatch();
        ClientMetaParticipantName.Install(dispatch, scene.Hooks());
        byte[] packet = new byte[98];
        packet[1] = 0x88;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 0x444);
        packet[8] = (byte)'Q';
        packet[9] = 0;
        dispatch.InvokeDirect(0x88, 9, null, packet);
        Assert.Equal((byte)'Q', scene.Challenge.Participant(4).Span[8]);
        Assert.Single(scene.Updates);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.955에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantName.Receive(scene.Hooks(), new byte[5]));
    }
}
