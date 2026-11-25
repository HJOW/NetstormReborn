using System.Buffers.Binary;
using System.Text;
using Netstorm.Client;
using Netstorm.Client.Chat;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 실제 Root 장치에 묶은 제출 부모의 점 명령 효과를 창 없이 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>RU-5 제출→분배 효과가 실제 치트 상자·설정·입력줄·메뉴·패킷 경계에 닿는지 검증한다. 0.995에서 추가됐다.</summary>
public sealed class ChatSubmitEffectsTests
{
    /// <summary>제품 InitializeNativeChatSubmit과 같은 실제 owner·명령표·장치 모양이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 구독자의 실제 소유자다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();
        /// <summary>같은 구독자와005cab30을 소유하는 실제 게임 전역이다.</summary>
        public readonly ClientGameplayGlobals Game;
        /// <summary>같은 실제 표면 모드다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>같은480BYTE 현재 기록이다.</summary>
        public readonly ClientChatGlobals Chat;
        /// <summary>실제 할당/current 소유자다.</summary>
        public readonly ClientChatManagerGlobals Manager = new();
        /// <summary>같은 실제 현재 창 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Window = new();
        /// <summary>원문 Issue 네 등록과 같은 실제 명령표다.</summary>
        public readonly ClientDotCommandGlobals Commands = new();
        /// <summary>같은 UTF8 설정 소유자다.</summary>
        public readonly ConfigInterface Configuration = new(_ => null);
        /// <summary>같은 실제 Identity 저장소다.</summary>
        public readonly ClientChatIdentityRuntime Identity;
        /// <summary>원문 공유 zero 배열과 같이 송신보다 먼저 무장하는 guard다.</summary>
        public readonly byte[] Zero = new byte[20];
        /// <summary>로컬 loopback으로 받은 실제 패킷이다.</summary>
        public readonly List<byte[]> Looped = [];
        /// <summary>설치된 입력줄 장치의 실제 호출이다.</summary>
        public readonly List<(uint Root, uint Visible)> Visibility = [];
        /// <summary>설치된 메뉴 장치의 실제 호출이다.</summary>
        public readonly List<uint> MenuCommands = [];
        /// <summary>설치된 출력 장치의 실제 호출이다.</summary>
        public readonly List<string> Prints = [];
        /// <summary>설치된 진단 장치의 실제 호출이다.</summary>
        public readonly List<(uint Level, string Text)> Diagnostics = [];
        /// <summary>설치된 음향 장치의 실제 호출이다.</summary>
        public readonly List<string> Sounds = [];
        /// <summary>설치된 진단 단언 횟수다.</summary>
        public int Assertions;
        /// <summary>매호출 현재 표시 이름이다.</summary>
        public string Name() => "Local";
        /// <summary>장기 owner와 원문 Issue→manager 초기 등록을 같은 순서로 실행한다.</summary>
        public Scene()
        {
            Game = new(Subscribers);
            Surface = new(Game, new(new ClientLocalStateGlobals(Game, new())), new(), () => 0);
            Chat = new(OriginalEdition.Patch1078, () => Subscribers.MySubscriber.Value);
            ClientPlayerRuntime.Initialize(Game);
            Subscribers.MySubscriber.Value = 100;
            Game.Player.BattleOwner = 1;
            Identity = new(Game, Surface, Subscribers, Chat, Name);
            ClientChatRuntime.InitializeClientIssueCommands(Commands);
            ClientChatRuntime.InitializeClientChatManager(Manager, Commands, Subscribers, Game.CoreData.ActiveBox, Configuration, _ => throw new InvalidOperationException("예상하지 않은 진단"));
            Commands.Register("clear\0"u8, 0x42f680, 1);
            Commands.Register("quit\0"u8, 0x42e520, 1);
        }
        /// <summary>현재 실제 DWORD 입력을 작성한다.</summary>
        private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);
        /// <summary>실제 원문 생성자로 manager를 준비하고 현재 슬롯을 게시한다.</summary>
        public uint Initialize()
        {
            uint manager = Manager.Allocate(2712);
            RawChatManagerPrefix.ConstructChatManager(manager, Manager.MakePrefixHooks());
            Manager.Current.Value = manager;
            Put(Manager.Bytes(manager), 12, 0);
            Put(Manager.Bytes(manager), 16, 32);
            Manager.Bytes(manager)[0x14] = 0;
            Manager.Bytes(manager)[0x50] = 0;
            return manager;
        }
        /// <summary>제품과 같은 실제 패킷 후크 모양이다. 송신만 R11 후속이다.</summary>
        public ChatPacketHooks Packet()
        {
            ChatPacketHooks packet = Manager.MakePacketHooks();
            packet.CurrentName = () => Encoding.Latin1.GetBytes(Name() + '\0');
            packet.Read = address => address switch
            {
                RawChatPacket.SubscriberAddress => Subscribers.MySubscriber.Value,
                RawChatPacket.BattleAddress => Surface.Storage.OwnerMode.Battle ? 1u : 0,
                RawChatPacket.ChannelOneAddress => Surface.Storage.OwnerMode.Fort ? 1u : 0,
                RawChatPacket.MetamapAddress => Surface.Storage.OwnerMode.Challenge ? 1u : 0,
                0x544c5cu => 0u,
                0x52edf6u => 0x41u,
                _ => throw new ArgumentOutOfRangeException(nameof(address)),
            };
            packet.Send = (channel, body) => throw new InvalidOperationException("Chat 일반 송신은 R11 수송에서 잇는다");
            packet.Log = (level, text) => Diagnostics.Add((level, text));
            packet.ZeroAddress = () => { Array.Clear(Zero); Zero[0] = 2; };
            packet.LocalReceive = body => Looped.Add(body.ToArray());
            return packet;
        }
        /// <summary>제품과 같은 실제 일반 say 부모다.</summary>
        public ClientChatMessageRuntime Message() => new(Manager, Subscribers, Identity, Name, Packet(), Window, (root, visible) => Visibility.Add((root, visible)));
        /// <summary>제품과 같은 실제 장치 묶음으로 제출 부모를 설치한다.</summary>
        public ClientChatSubmitRuntime Submit() => new(Manager, Commands, Subscribers, Game, Chat, Window, Configuration, new()
        {
            Resolve = Identity.Resolve,
            Translate = text => text,
            Assertion = _ => Assertions++,
            SetInputVisible = (root, visible) => Visibility.Add((root, visible)),
            Flash = brightness =>
            {
                if (Game.Player.PlayerKnown == 0) return;
                throw new InvalidOperationException("Chat 번쩍임 표시는 R6 월드 표시 연결에서 잇는다");
            },
            Sound = Sounds.Add,
            MenuCommand = callback => { MenuCommands.Add(callback); return 0; },
            Message = Message().Say,
            PrintChat = Prints.Add,
            DiagnosticLog = (level, text) => Diagnostics.Add((level, text)),
        });
    }

    /// <summary>올바른 치트 비밀번호만 실제 UserCheat 상자에 쓰고 틀리면 유지한다.</summary>
    [Fact]
    public void CheatPasswordSetsActualUserCheat()
    {
        var scene = new Scene();
        Assert.Equal(0u, scene.Game.UserCheat.Value);
        scene.Submit().Submit(scene.Manager.Current.Value, ".cheatorama 8675309"u8);
        Assert.Equal(0x845fedu, scene.Game.UserCheat.Value);
        scene.Submit().Submit(scene.Manager.Current.Value, ".cheatorama 0"u8);
        Assert.Equal(0x845fedu, scene.Game.UserCheat.Value);
    }

    /// <summary>번쩍임은 known 0에서 조용히 넘기고 실제 음향을 재생하며 known 뒤 R6 경계를 드러낸다.</summary>
    [Fact]
    public void FlashGatesOnPlayerKnown()
    {
        var scene = new Scene();
        scene.Submit().Submit(scene.Manager.Current.Value, ".flash 7"u8);
        Assert.Equal(["thunderQuietDistant.wav"], scene.Sounds);
        scene.Game.Player.PlayerKnown = 1;
        var error = Assert.Throws<InvalidOperationException>(() => scene.Submit().Submit(scene.Manager.Current.Value, ".flash 7"u8));
        Assert.Contains("R6", error.Message);
    }

    /// <summary>설명 장치는 원문처럼 미연결이며 명시 실패는 cpp와 같은 위치다.</summary>
    [Fact]
    public void DescWithoutDeviceFailsWithExplicitBoundary()
    {
        var scene = new Scene();
        var error = Assert.Throws<InvalidOperationException>(() => scene.Submit().Submit(scene.Manager.Current.Value, ".desc"u8));
        Assert.Contains("선택 장치 미연결", error.Message);
    }

    /// <summary>빈 say는 실제 현재 root에서만 입력줄 장치를 부르고 root 0에서는 조회하지 않는다.</summary>
    [Fact]
    public void EmptySayUsesActualRootGate()
    {
        var scene = new Scene();
        scene.Submit().Submit(scene.Manager.Current.Value, ":"u8);
        Assert.Empty(scene.Visibility);
        scene.Window.Current.Value = 7;
        scene.Submit().Submit(scene.Manager.Current.Value, ".say"u8);
        Assert.Equal([(7u, 0u)], scene.Visibility);
    }

    /// <summary>등록된 메뉴 명령은 설치된 실제 장치에 원문 callback으로 전달된다.</summary>
    [Fact]
    public void MenuCommandsReachInstalledDevice()
    {
        var scene = new Scene();
        scene.Submit().Submit(scene.Manager.Current.Value, ".clear;.quit"u8);
        Assert.Equal(new uint[] { 0x42f680, 0x42e520 }, scene.MenuCommands);
    }

    /// <summary>일반 say는 실제 수신자 확장과 패킷 framing 뒤 로컬 loopback guard를 무장한다.</summary>
    [Fact]
    public void SayFramesActualPacketToLoopbackBoundary()
    {
        var scene = new Scene();
        uint manager = scene.Initialize();
        scene.Submit().Submit(manager, "hello\0"u8);
        Assert.Contains((9u, "ERROR: No transceiver on which to chat!\n"), scene.Diagnostics);
        Assert.Equal(2, scene.Zero[0]);
        byte[] packet = Assert.Single(scene.Looped);
        Assert.Contains("hello", Encoding.Latin1.GetString(packet));
    }

    /// <summary>전투 say는 실제 송신 경계에서 R11 후속을 명시한다.</summary>
    [Fact]
    public void BattleSayReachesTransportBoundary()
    {
        var scene = new Scene();
        uint manager = scene.Initialize();
        scene.Surface.Storage.OwnerMode.Battle = true;
        var error = Assert.Throws<InvalidOperationException>(() => scene.Submit().Submit(manager, "test\0"u8));
        Assert.Contains("R11", error.Message);
    }

    /// <summary>지원하지 않는 callback은 명시 실패하고 비영 say 오류는 설치된 출력에 닿는다.</summary>
    [Fact]
    public void UnknownCallbackAndErrorReachInstalledDevices()
    {
        var scene = new Scene();
        scene.Commands.Register("other\0"u8, 0x12345678, 1);
        var error = Assert.Throws<InvalidOperationException>(() => scene.Submit().Submit(scene.Manager.Current.Value, ".other"u8));
        Assert.Contains("callback 미연결", error.Message);
        var runtime = new ClientChatSubmitRuntime(scene.Manager, scene.Commands, scene.Subscribers, scene.Game, scene.Chat, scene.Window, scene.Configuration, new()
        {
            Resolve = _ => false, Translate = key => key, Message = _ => 0x80000001, PrintChat = scene.Prints.Add,
        });
        runtime.Submit(scene.Manager.Current.Value, "hello\0"u8);
        Assert.Equal(["Error in command \".say\": code == 80000001\n"], scene.Prints);
    }
}
