// 0x25 메타 항해 오류의 원문 분기·안내·Tell·표시 순서를 대조한다.
// cpppj/src/client/NativeMetaSailError.cpp 의 ReceiveNativeMetaSailError를 대조한다.
// 0.956에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x25 메타 항해 오류의 실제 CS·번역·안내·Tell·표시 호출을 기록한다.
public sealed class ClientMetaSailErrorTests
{
    /// <summary>같은 실제 CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new();

        /// <summary>실제 번역 호출이다.</summary>
        public readonly List<string> Translates = [];

        /// <summary>실제 안내 호출이다.</summary>
        public readonly List<(uint Object, string Text)> Notes = [];

        /// <summary>실제 Tell 호출이다.</summary>
        public readonly List<string> Tells = [];

        /// <summary>실제 표시 갱신이다.</summary>
        public readonly List<(uint Id, uint Mask)> Updates = [];

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaSailErrorHooks Hooks() => new()
        {
            Challenge = Challenge,
            Subscriber = Subscriber,
            Translate = text => { Translates.Add(text); return text; },
            Note = (id, text) => Notes.Add((id, text)),
            Tell = section => Tells.Add(section),
            Update = (id, mask) => Updates.Add((id, mask)),
        };

        /// <summary>실제 참가자 기록에 구독자·객체·식별자를 쓴다.</summary>
        public void Participant(uint index, uint subscriber, uint objectId, uint id)
        {
            Memory<byte> record = Challenge.Participant(index);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), subscriber);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xb8, 4), objectId);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0, 4), id);
        }

        /// <summary>원문 0x25 8BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(ushort code)
        {
            byte[] packet = new byte[8];
            packet[1] = 0x25;
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), code);
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaSailError.Receive(null!, new byte[8]));
        var bare = new ClientMetaSailErrorHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaSailError.Receive(bare, new byte[8]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaSailError.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaSailError.Install(Dispatch(), null!));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaSailError.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x25);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(8, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b47e0u, dispatch.SnapshotReceiver(0x25)!.NativeEntry);
    }

    /// <summary>오류 1~4는 번역 뒤 같은 객체에 안내를 붙인다. 0.956에서 추가됐다.</summary>
    [Theory]
    [InlineData(1, "You have been kicked off!")]
    [InlineData(2, "ERROR: Invalid battle id")]
    [InlineData(3, "ERROR: Invalid battle slot id")]
    [InlineData(4, "That battle began before you got there")]
    public void ErrorMessageAttachesNoteToSameObject(ushort code, string message)
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x555;
        scene.Participant(7, 0x555, 0x901, 0x902);
        ClientMetaSailError.Receive(scene.Hooks(), Scene.Packet(code));
        Assert.Equal([message], scene.Translates);
        Assert.Equal([(0x901u, message)], scene.Notes);
        Assert.Empty(scene.Tells);
        Assert.Equal([(0x902u, 0xffu)], scene.Updates);
    }

    /// <summary>오류 5~7은 Tell 섹션을 실행한다. 0.956에서 추가됐다.</summary>
    [Theory]
    [InlineData(5, "TooManyPirates")]
    [InlineData(6, "BattleVersionMismatch")]
    [InlineData(7, "BattleFailedDiags")]
    public void ErrorSectionRunsTell(ushort code, string section)
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x555;
        scene.Participant(7, 0x555, 0x901, 0x902);
        ClientMetaSailError.Receive(scene.Hooks(), Scene.Packet(code));
        Assert.Empty(scene.Translates);
        Assert.Empty(scene.Notes);
        Assert.Equal([section], scene.Tells);
        Assert.Equal([(0x902u, 0xffu)], scene.Updates);
    }

    /// <summary>알 수 없는 코드는 표시만 갱신한다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void UnknownCodeUpdatesOnly()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x555;
        scene.Participant(7, 0x555, 0x901, 0x902);
        ClientMetaSailError.Receive(scene.Hooks(), Scene.Packet(0));
        Assert.Empty(scene.Translates);
        Assert.Empty(scene.Notes);
        Assert.Empty(scene.Tells);
        Assert.Equal([(0x902u, 0xffu)], scene.Updates);
    }

    /// <summary>미발견 구독자는 호스트 오류로 표시한다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void UnknownSubscriberThrows()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x999;
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaSailError.Receive(scene.Hooks(), Scene.Packet(1)));
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x555;
        scene.Participant(7, 0x555, 0x901, 0x902);
        var dispatch = Dispatch();
        ClientMetaSailError.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x25, 9, null, Scene.Packet(6));
        Assert.Equal(["BattleVersionMismatch"], scene.Tells);
        Assert.Equal([(0x902u, 0xffu)], scene.Updates);
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.956에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaSailError.Receive(scene.Hooks(), new byte[5]));
    }
}
