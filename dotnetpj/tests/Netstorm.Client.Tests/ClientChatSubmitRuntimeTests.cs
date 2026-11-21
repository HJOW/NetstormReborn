using System.Text;
using System.Globalization;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 같은 실제 소유자의 제출/형식/명령/진단과 현재 root 자연 관문을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 Chat 부모를 GUI/네트워크 재현 없이 조립하는 집중 검사다.</summary>
public sealed class ClientChatSubmitRuntimeTests
{
    /// <summary>최초 실제 Host와 같은 장기 전역/명령표/manager 조합이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 두 구독자/이름의 장기 소유자다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();
        /// <summary>같은 구독자와005cab30을 소유하는 실제 게임 전역이다.</summary>
        public readonly ClientGameplayGlobals Game;
        /// <summary>실제 할당/current/캡처 소유자다.</summary>
        public readonly ClientChatManagerGlobals Manager = new();
        /// <summary>최초 say 표와 같은 장기 검색이다.</summary>
        public readonly ClientDotCommandGlobals Commands = new();
        /// <summary>같은4800BYTE 현재 기록이다.</summary>
        public readonly ClientChatGlobals Chat;
        /// <summary>같은 실제 현재 root 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Window = new();
        /// <summary>같은 UTF8 설정/변경 알림 소유자다.</summary>
        public readonly ConfigInterface Configuration = new(_ => null);
        /// <summary>원문 Issue4→Chat3 초기화로 실제 current manager를 만든다.</summary>
        public Scene()
        {
            Game = new(Subscribers); Chat = new(OriginalEdition.Patch1078, () => Subscribers.MySubscriber.Value);
            ClientChatRuntime.InitializeClientIssueCommands(Commands); ClientChatRuntime.InitializeClientChatManager(Manager, Commands, Subscribers, Game.CoreData.ActiveBox, Configuration, _ => throw new IOException("초기정상manager진단"));
        }
        /// <summary>실제 필수 장치만 기본 제공하며 나머지는 선택한 경우 직접 설치한다.</summary>
        public ClientChatSubmitRuntime Bind(ClientChatSubmitDevices? devices = null) => new(Manager, Commands, Subscribers, Game, Chat, Window, Configuration, devices ?? new() { Resolve = _ => false, Translate = key => key });
    }
    /// <summary>NUL 앞의 실제 표시 BYTE 문자열을 읽는다.</summary>
    private static string Text(ReadOnlySpan<byte> bytes) => Encoding.Latin1.GetString(bytes[..bytes.IndexOf((byte)0)]);
    /// <summary>기존 독립 값과 실제 두 상자의 늦은 재조회가 함께 유지된다.</summary>
    [Fact]
    public void FormatGlobalsBorrowCurrentBoxesAfterSelectedCallbacks()
    {
        var scene = new Scene(); uint first = scene.Manager.Current.Value, second = scene.Manager.Allocate(RawChatManagerPrefix.ManagerBytes); RawChatManagerPrefix.ConstructChatManager(second, scene.Manager.MakePrefixHooks()); var memory = scene.Manager.MakePrefixHooks(); string key = "";
        var globals = new ChatFormatGlobals(scene.Manager.Current, scene.Subscribers.MySubscriber);
        ChatFormatBody.ApplyChatFormat(true, globals, "~rTest", scene.Manager.FormatCapture, new()
        {
            Memory = scene.Manager.Bytes, CopyFormat = memory.CopyFormat,
            StripSlots = bytes => { ChatFormatBody.StripChatFormatSlots(bytes); scene.Manager.Current.Value = second; scene.Subscribers.MySubscriber.Value = 9; },
            WriteConfig = (name, _) => key = name,
        });
        Assert.Equal(0, scene.Manager.Bytes(first)[0x50]); Assert.Equal((byte)'~', scene.Manager.Bytes(second)[0x50]); Assert.Equal("9chatFormat2", key); globals.Subscriber = uint.MaxValue; Assert.Equal(uint.MaxValue, scene.Subscribers.MySubscriber.Value);
        var independent = new ChatFormatGlobals { Current = 7, Subscriber = 8 }; Assert.Equal(7u, independent.Current); Assert.Equal(8u, independent.Subscriber); Assert.Equal(second, scene.Manager.Current.Value);
    }
    /// <summary>같은 캡처/실제CRT/UTF8 설정과 종류29 진단이 원문 위치에서 연결된다.</summary>
    [Fact]
    public void ActualFormatFacadeKeepsDisplayBytesUtf8ConfigAndDiagnosticText()
    {
        var scene = new Scene(); string display = "~r한글", raw = Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(display)); var diagnostics = new List<(uint, string)>();
        Assert.Equal(0u, ClientChatRuntime.ApplyClientChatFormat(scene.Manager, scene.Subscribers, scene.Configuration, true, raw, null, (kind, text) => diagnostics.Add((kind, text))));
        Assert.Equal(display, scene.Configuration.Configuration.Get("-1chatFormat2")); Assert.Equal(ExtendedText.EncodeDisplayText(display), scene.Manager.Bytes(scene.Manager.Current.Value).AsSpan(0x50, raw.Length).ToArray()); Assert.Empty(diagnostics);
        scene.Subscribers.MySubscriber.Value = 9; ClientChatRuntime.ApplyClientChatFormat(scene.Manager, scene.Subscribers, scene.Configuration, false, "~bBack", null, (kind, text) => diagnostics.Add((kind, text))); Assert.Equal("~bBack", scene.Configuration.Configuration.Get("9chatFormat"));
        ClientChatRuntime.ApplyClientChatFormat(scene.Manager, scene.Subscribers, scene.Configuration, false, "invalid", null, (kind, text) => diagnostics.Add((kind, text))); Assert.Equal(new[] { (29u, "Illegal format \"invalid\"\n") }, diagnostics);
    }
    /// <summary>실제 제출→분배→Issue 효과 순서와 같은 장기 치트 주소를 유지한다.</summary>
    [Fact]
    public void SubmittedIssueCommandsKeepCurrentCheatAndEffectOrder()
    {
        var scene = new Scene(); var events = new List<string>(); Assert.Equal(0u, scene.Game.UserCheat.Value);
        var runtime = scene.Bind(new()
        {
            Resolve = _ => false, Translate = key => key,
            Flash = brightness => { events.Add("flash:" + brightness); scene.Game.UserCheat.Value = 1234; },
            Sound = name => { Assert.Equal(1234u, scene.Game.UserCheat.Value); events.Add("sound:" + name); },
        });
        runtime.Submit(scene.Manager.Current.Value, ".flash 7;.cheatorama 8675309"u8); Assert.Equal(new[] { "flash:7", "sound:thunderQuietDistant.wav" }, events); Assert.Equal(0x845fedu, scene.Game.UserCheat.Value);
        scene.Bind().Submit(scene.Manager.Current.Value, ".cheatorama 0"u8); Assert.Equal(0x845fedu, scene.Game.UserCheat.Value); Assert.Throws<InvalidOperationException>(() => scene.Bind().Submit(scene.Manager.Current.Value, ".desc"u8));
    }
    /// <summary>root0에서는 geometry 장치를 조회하지 않고 현재 비영 root에만 표시0을 전달한다.</summary>
    [Fact]
    public void InputVisibilityReadsActualCurrentRootAndSelectedDevice()
    {
        var scene = new Scene(); var runtime = scene.Bind(); runtime.Submit(scene.Manager.Current.Value, ":"u8); scene.Window.Current.Value = 7;
        Assert.Throws<InvalidOperationException>(() => runtime.Submit(scene.Manager.Current.Value, ":"u8)); var seen = new List<(uint, uint)>();
        runtime = scene.Bind(new() { Resolve = _ => false, Translate = key => key, SetInputVisible = (root, visible) => seen.Add((root, visible)) }); runtime.Submit(scene.Manager.Current.Value, ".say"u8); Assert.Equal(new[] { (7u, 0u) }, seen);
        scene.Window.Current.Value = 19; runtime.Submit(scene.Manager.Current.Value, ":"u8); Assert.Equal((19u, 0u), seen[1]);
    }
    /// <summary>일반 실제 메시지 반환의 비영 오류만 현재 이름/16진 코드로 출력하고 current0은 송신을 선택하지 않는다.</summary>
    [Fact]
    public void MessageAndFormattedCommandErrorUseActualBodyAndNaturalCurrentGuard()
    {
        var scene = new Scene(); var messages = new List<string>(); var errors = new List<string>();
        var runtime = scene.Bind(new() { Resolve = _ => false, Translate = key => key, Message = input => { messages.Add(Text(input)); return 0x80000001; }, PrintChat = errors.Add });
        runtime.Submit(scene.Manager.Current.Value, "hello"u8); Assert.Equal(new[] { "hello" }, messages); Assert.Equal(new[] { "Error in command \".say\": code == 80000001\n" }, errors);
        uint manager = scene.Manager.Current.Value; scene.Manager.Current.Value = 0; scene.Bind().Submit(manager, "not-sent"u8); Assert.Single(messages); Assert.Single(errors);
    }
    /// <summary>등록된 실제 메뉴 명령만 해당 장치에 전달하며 미복원 callback은 명시 실패한다.</summary>
    [Fact]
    public void MenuCommandsAndUnknownCallbacksKeepSelectedBoundaries()
    {
        var scene = new Scene(); scene.Commands.Register("clear\0"u8, 0x42f680, 1); scene.Commands.Register("quit\0"u8, 0x42e520, 1); scene.Commands.Register("other\0"u8, 0x12345678, 1); var calls = new List<uint>();
        var runtime = scene.Bind(new() { Resolve = _ => false, Translate = key => key, MenuCommand = callback => { calls.Add(callback); return 0; } }); runtime.Submit(scene.Manager.Current.Value, ".clear;.quit"u8); Assert.Equal(new uint[] { 0x42f680, 0x42e520 }, calls); Assert.Throws<InvalidOperationException>(() => runtime.Submit(scene.Manager.Current.Value, ".other"u8));
    }
    /// <summary>Identity/번역과 실제 Subscriber/Game 주소가 없으면 binding에서 먼저 거부한다.</summary>
    [Fact]
    public void BindingRequiresActualIdentityAndMatchingLongLivedOwners()
    {
        var scene = new Scene(); uint before = scene.Manager.Current.Value; Assert.Throws<ArgumentException>(() => scene.Bind(new())); Assert.Throws<ArgumentException>(() => scene.Bind(new() { Resolve = _ => false }));
        Assert.Throws<ArgumentException>(() => new ClientChatSubmitRuntime(scene.Manager, scene.Commands, new(), scene.Game, scene.Chat, scene.Window, scene.Configuration, new() { Resolve = _ => false, Translate = key => key })); Assert.Equal(before, scene.Manager.Current.Value); Assert.Equal(0u, scene.Game.UserCheat.Value);
    }
    /// <summary>관리 문화권의 음수 표식이 원문 signed%d 설정 키에 섞이지 않아야 한다.</summary>
    [Fact]
    public void FormatKeyKeepsNativeSignedDigitsAcrossManagedCulture()
    {
        var scene = new Scene(); CultureInfo previous = CultureInfo.CurrentCulture; var selected = (CultureInfo)CultureInfo.InvariantCulture.Clone(); selected.NumberFormat.NegativeSign = "negative:";
        try
        {
            CultureInfo.CurrentCulture = selected;
            ClientChatRuntime.ApplyClientChatFormat(scene.Manager, scene.Subscribers, scene.Configuration, true, "~rTest", null, null);
            Assert.Equal("~rTest", scene.Configuration.Configuration.Get("-1chatFormat2")); Assert.Null(scene.Configuration.Configuration.Get("negative:1chatFormat2"));
        }
        finally { CultureInfo.CurrentCulture = previous; }
    }
}
