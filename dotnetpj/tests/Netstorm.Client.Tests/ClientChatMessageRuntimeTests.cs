using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 실제 장기 소유자의 일반 조합/대상 조회/수신자 확장/패킷을 함께 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 UI 없이 원문 부모의 연결과 선택 장치 관문을 확인한다.</summary>
public sealed class ClientChatMessageRuntimeTests
{
    /// <summary>실제 같은 소유자와 명시 송신 관찰 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 구독자와 이름의 실제 소유자다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();
        /// <summary>같은 실제 게임/Player/Core다.</summary>
        public readonly ClientGameplayGlobals Game;
        /// <summary>같은 실제 표면 모드다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>같은 실제 현재100행이다.</summary>
        public readonly ClientChatGlobals Chat;
        /// <summary>같은 실제 manager/수신자 할당이다.</summary>
        public readonly ClientChatManagerGlobals Manager = new();
        /// <summary>같은 현재 창 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Window = new();
        /// <summary>현재 이름 getter 횟수다.</summary>
        public int NameReads;
        /// <summary>현재 모드와 구독자를 같은 주소에서 읽는 패킷 장치다.</summary>
        public readonly ChatPacketHooks Packet;
        /// <summary>현재 이름/실제 Player와 같은 facade다.</summary>
        public readonly ClientChatIdentityRuntime Identity;
        /// <summary>관찰된 실제 송신 채널/패킷이다.</summary>
        public readonly List<(int Channel, byte[] Bytes)> Sent = new();
        /// <summary>관찰된 실제 입력줄 변경이다.</summary>
        public readonly List<(uint Root, uint Visible)> Visibility = new();
        /// <summary>실제 전역을 준비하되 manager 초기화는 명시 호출자에 남긴다.</summary>
        public Scene()
        {
            Game = new(Subscribers); Surface = new(Game, new(new ClientLocalStateGlobals(Game, new())), new(), () => 0); Chat = new(OriginalEdition.Patch1078, () => Subscribers.MySubscriber.Value); ClientPlayerRuntime.Initialize(Game); Subscribers.MySubscriber.Value = 100; Game.Player.BattleOwner = 1;
            Identity = new(Game, Surface, Subscribers, Chat, Name); Packet = Manager.MakePacketHooks(); Packet.CurrentName = () => Encoding.Latin1.GetBytes(Name() + '\0');
            Packet.Read = address => address switch { RawChatPacket.SubscriberAddress => Subscribers.MySubscriber.Value, RawChatPacket.BattleAddress => Surface.Storage.OwnerMode.Battle ? 1u : 0, RawChatPacket.ChannelOneAddress => Surface.Storage.OwnerMode.Battle ? 0u : 1, RawChatPacket.MetamapAddress or 0x544c5c => 0, 0x52edf6 => 0x41, _ => throw new ArgumentOutOfRangeException(nameof(address)) };
            Packet.Send = (channel, bytes) => { ChatPacketBody.ObserveChatPacket(bytes, 1, number => throw new InvalidOperationException("예상하지 않은 진단:" + number)); Sent.Add((channel, bytes)); };
        }
        /// <summary>매호출 현재 표시 BYTE 이름을 조회한다.</summary>
        public string Name() { NameReads++; return "Local"; }
        /// <summary>실제 원문 생성자로 manager를 준비하고 현재 슬롯을 명시 게시한다.</summary>
        public uint Initialize()
        {
            uint manager = Manager.Allocate(2712); RawChatManagerPrefix.ConstructChatManager(manager, Manager.MakePrefixHooks()); Manager.Current.Value = manager; Put(Manager.Bytes(manager), 12, 0); Put(Manager.Bytes(manager), 16, 32); Manager.Bytes(manager)[0x14] = 0; Manager.Bytes(manager)[0x50] = 0; return manager;
        }
        /// <summary>같은 실제 장치로 일반 부모를 설치하며 이름은 선행 조회하지 않는다.</summary>
        public ClientChatMessageRuntime Runtime() => new(Manager, Subscribers, Identity, Name, Packet, Window, (root, visible) => Visibility.Add((root, visible)));
    }
    /// <summary>현재 실제 DWORD 입력을 작성한다.</summary>
    private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);
    /// <summary>현재 실제 DWORD 출력을 읽는다.</summary>
    private static uint Read(byte[] bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(at, 4));
    /// <summary>실제 패킷 본문 길이/위치에서 표시 BYTE를 읽는다.</summary>
    private static string Body(byte[] packet) { int name = BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(10, 2)), length = BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(12, 2)); return Encoding.Latin1.GetString(packet, 15 + name, length); }
    /// <summary>manager0/앞space/빈입력 관문이 이름·패킷을 조회하지 않고 현재 root만 선택한다.</summary>
    [Fact]
    public void NaturalGatesKeepNamesAndPacketsLazy()
    {
        var scene = new Scene(); var runtime = scene.Runtime(); Assert.Equal(0, scene.NameReads); Assert.Equal(0u, runtime.Say([])); scene.Initialize(); Assert.Equal(0u, runtime.Say([32, 120, 0])); Assert.Equal(0u, runtime.Say([0])); Assert.Empty(scene.Visibility); scene.Window.Current.Value = 77; runtime.Say([0]); Assert.Equal(new[] { (77u, 0u) }, scene.Visibility); Assert.Equal(0, scene.NameReads); Assert.Empty(scene.Sent);
    }
    /// <summary>자기 추가가 실제32→64 확장/반납을 수행하고 전송 observer가 같은 패킷 길이를 확정한다.</summary>
    [Fact]
    public void ActualOwnerGrowsRecipientsAndProducesNativeBattleBody()
    {
        var scene = new Scene(); uint manager = scene.Initialize(); scene.Surface.Storage.OwnerMode.Battle = true; var add = scene.Manager.MakeSubmitHooks();
        // 초기32슬롯을 실제 추가 본체로 채운다.
        for (uint index = 0; index < 32; index++) RawChatSubmit.AddChatRecipient(manager, 200 + index, add);
        uint old = Read(scene.Manager.Bytes(manager), 8); var runtime = scene.Runtime(); Assert.Equal(0u, runtime.Say(Encoding.Latin1.GetBytes("hello\0"))); var sent = Assert.Single(scene.Sent); Assert.Equal(9, sent.Channel); Assert.Equal("032~[Iicon.D41]Local> hello", Body(sent.Bytes)); Assert.Equal(33u, Read(scene.Manager.Bytes(manager), 0)); Assert.Equal(64u, Read(scene.Manager.Bytes(manager), 4)); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Manager.Bytes(old)); Assert.Equal(sent.Bytes.Length, BinaryPrimitives.ReadUInt16LittleEndian(sent.Bytes.AsSpan(2, 2))); Assert.Equal(100u, Read(sent.Bytes, sent.Bytes.Length - 4));
        uint grown = Read(scene.Manager.Bytes(manager), 8); runtime.Say(Encoding.Latin1.GetBytes("again\0")); Assert.Equal(grown, Read(scene.Manager.Bytes(manager), 8)); Assert.Equal(33u, Read(scene.Manager.Bytes(manager), 0));
    }
    /// <summary>직접 대상은 실제 Chat/Identity 이름을 쓰고 binding 이후 원본 장치 변경은 캡처를 바꾸지 않는다.</summary>
    [Fact]
    public void DirectTargetUsesActualIdentityAndCapturedPacketDevices()
    {
        var scene = new Scene(); uint manager = scene.Initialize(); scene.Chat.Channels.Add(200, "Peer", 0); RawChatSubmit.AddChatRecipient(manager, 200, scene.Manager.MakeSubmitHooks()); Put(scene.Manager.Bytes(manager), 12, 1); Put(scene.Manager.Bytes(manager), 16, 0); var runtime = scene.Runtime(); scene.Packet.Send = (_, _) => throw new InvalidOperationException("이후 교체 장치"); runtime.Say(Encoding.Latin1.GetBytes("hi\0")); var sent = Assert.Single(scene.Sent); Assert.Equal(1, sent.Channel); Assert.Equal("~[Iicon.D38]Local (to Peer)> hi\0", Body(sent.Bytes)); Assert.Equal(2u, Read(scene.Manager.Bytes(manager), 0)); Assert.Equal(3, scene.NameReads);
    }
    /// <summary>미연결 송신은 성공으로 바꾸지 않으며 앞선 실제 자기 추가와 본문은 유지한다.</summary>
    [Fact]
    public void MissingSelectedSendPreservesEarlierWrites()
    {
        var scene = new Scene(); uint manager = scene.Initialize(); scene.Packet.Send = null; var runtime = scene.Runtime(); Assert.Throws<InvalidOperationException>(() => runtime.Say([120, 0])); Assert.Equal(1u, Read(scene.Manager.Bytes(manager), 0)); Assert.Equal(100u, Read(scene.Manager.Bytes(Read(scene.Manager.Bytes(manager), 8)), 0)); Assert.Equal("~[Iicon.D41]Local> x", Encoding.Latin1.GetString(scene.Manager.Bytes(manager).AsSpan(0x298, "~[Iicon.D41]Local> x".Length)));
    }
    /// <summary>같은 실제 제출→분배→Identity→일반 조합→패킷의 끝까지 연결한다.</summary>
    [Fact]
    public void SubmitDispatchAndMessageShareActualRecipientsAndSubscriber()
    {
        var scene = new Scene(); var commands = new ClientDotCommandGlobals(); var config = new ConfigInterface(_ => null); ClientChatRuntime.InitializeClientIssueCommands(commands); ClientChatRuntime.InitializeClientChatManager(scene.Manager, commands, scene.Subscribers, scene.Game.CoreData.ActiveBox, config, number => throw new InvalidOperationException("예상하지 않은 진단:" + number)); scene.Chat.Channels.Add(200, "Peer", 0); var message = scene.Runtime(); var submit = new ClientChatSubmitRuntime(scene.Manager, commands, scene.Subscribers, scene.Game, scene.Chat, scene.Window, config, new() { Resolve = scene.Identity.Resolve, Translate = text => text, Message = message.Say }); submit.Submit(scene.Manager.Current.Value, Encoding.Latin1.GetBytes("Peer: hi\0")); var sent = Assert.Single(scene.Sent); Assert.Equal("~[Iicon.D38]Local (to Peer)> hi\0", Body(sent.Bytes)); Assert.Equal(100u, Read(sent.Bytes, 6)); Assert.Equal(2, sent.Bytes[14]);
    }
}
