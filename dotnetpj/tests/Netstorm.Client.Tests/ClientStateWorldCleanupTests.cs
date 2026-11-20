using System.Buffers.Binary;
using Netstorm.Client.Audio;
using Netstorm.Client.Chat;
using Netstorm.Client.Display;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 전체정리부모가실제저장소/시계/SP/Player를변경하고미연결위치에서부분prefix를유지하는지검사한다.
namespace Netstorm.Client.Tests;

/// <summary>첫메뉴의실제무Runtime경로와선택효과/초기화관문을검증한다.</summary>
public sealed class ClientStateWorldCleanupTests
{
    /// <summary>같은실제Client장기소유자를구성하고UI/재생경계만명시입력으로제공한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제권한/Player/Core전역이다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>동일현재State/기한/Zone전역이다.</summary>
        public readonly ClientLocalStateGlobals Local;
        /// <summary>동일현재연결표다.</summary>
        public readonly ClientConnectionGlobals Connections;
        /// <summary>동일현재시계/Frame이다.</summary>
        public readonly ClientClockGlobals Clock;
        /// <summary>동일현재물리/Kernel소유자다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>같은실제SP와난수다.</summary>
        public readonly ClientSpGlobals Sp = new(new(1), new() { Value = 1 }, new(), () => 123);
        /// <summary>실제Template/표식/seed/음악상태다.</summary>
        public readonly ClientStateResources Resources = new();
        /// <summary>같은현재NSInfo zone이다.</summary>
        public readonly ClientNsInfoGlobals NsInfo = new();
        /// <summary>같은현재구름dirty소유자다.</summary>
        public readonly ClientKencloudGlobals Cloud = new();
        /// <summary>외부선택효과의순서를보존한다.</summary>
        public readonly List<string> Events = [];
        /// <summary>같은실제채팅4800BYTE다.</summary>
        public readonly byte[] ChatBytes = new byte[RawChatChannels.Bytes];
        /// <summary>같은실제점유채팅행을정리한다.</summary>
        public readonly RawChatChannels Chat;
        /// <summary>동일songEnd와실제장면본체를쓴다. 장치입력만명시한다.</summary>
        public readonly SceneMusic Music;
        /// <summary>현재실제밀리초의조회횟수다. Frame캐시대체를감지한다.</summary>
        public int ClockReads;
        /// <summary>원문의현재벽시각을결정하는명시밀리초다.</summary>
        public uint Now = 110000;
        /// <summary>실제선택된곡의현재이름이다.</summary>
        private string _song = "before.mus";

        /// <summary>첫메뉴의known0/기록초기화와원문장기물리를준비한다.</summary>
        public Scene()
        {
            Game.Player.TemplateOwner = 1; Game.Player.BattleOwner = 1;
            Local = new(Game, new()); Connections = new(Local); Clock = new(Local); Clock.InitializeFullTime(10000);
            Surface = new(Game, Clock, new(), () => { ClockReads++; return Now; });
            // 원문첫메뉴보다앞서설치되는188개TYPE의명시입력이다. 이검사는현재Geyser칸의Cost쓰기만소비한다.
            Surface.Storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
            Clock.Frame.Value = new() { Game = 7.5, Wall = 90, Number = 55 };
            ClientPlayerRuntime.Initialize(Game); Sp.Store.Initialize();
            Resources.FortCoreSeed.Value = 0xabcd1234; Resources.Popup.Mark(71, "preserved"); NsInfo.Zone.Value = 0x12345678;
            Chat = new(OriginalEdition.Patch1078, ChatBytes, () => throw new IOException("점유행정리중구독자조회"));
            ChatBytes[0] = 0x71; BinaryPrimitives.WriteUInt32LittleEndian(ChatBytes.AsSpan(0x2c, 4), 0x80000001);
            Resources.Music.CurrentBattle = () => Surface.Storage.OwnerMode.Battle ? 1u : 0u;
            Music = new(Resources.Music, Sp.Random, new()
            {
                WallSeconds = Surface.WallSeconds, CurrentName = () => _song, Select = name => { _song = name; Events.Add("song:" + name); }, Duration = () => 0,
                Sacrificing = _ => throw new IOException("초기미준비Player희생판정"), WaitingRoom = () => false,
                Refresh = () => Events.Add("weather"), AscendancyPalette = () => false, LoadPalette = _ => throw new IOException("옵션0팔레트선택"),
                ThunderFlash = () => Events.Add("thunder"), WeatherSound = name => Events.Add("sound:" + name),
            });
        }
        /// <summary>같은장기저장소만가리키는현재뷰를반환한다.</summary>
        public ClientStateWorldContext Context() => Resources.Context(Surface, Local, Connections, Cloud, NsInfo);
        /// <summary>실제부모를호출하며다른저장소나시각사본을만들지않는다.</summary>
        public void Cleanup(ClientStateWorldDevices devices, uint reason = 1, Action<uint>? selectCursor = null) => ClientStateWorldCleanup.Cleanup(reason, Context(), Surface, Sp, devices, selectCursor);
        /// <summary>복원대기UI경계는명시사건으로제공하고채팅/장면음악은실제본체를호출한다.</summary>
        public ClientStateWorldDevices Devices() => new()
        {
            CancelCursor = () => Events.Add("cursor"), ResetChatChannels = () => { Events.Add("chat"); Chat.ResetOccupied(); },
            SelectCursor = cursor => Events.Add("select-cursor:" + cursor), KillGumps = () => Events.Add("kill"), CleanupGumps = () => Events.Add("cleanup"), ReplaceUber = () => Events.Add("uber"),
            RefreshSp = () => Events.Add("sp"), RetireRuntime = () => throw new IOException("무Runtime에서owner반납선택"),
            LeaveMetamap = () => throw new IOException("비메타에서Leave선택"), SaveFort = _ => throw new IOException("빈manager에서Save선택"), SaveBattle = _ => throw new IOException("초기phase에서전투Save선택"),
            Assertion = line => throw new IOException("정상모드에서진단:" + line),
            StartSceneMusic = battle => { Events.Add("music:" + battle); Resources.Music.Battle = battle ? 1u : 0u; Music.Start(); },
            StopSoundLoops = () => Events.Add("loops"), InterpretOptions = () => Events.Add("options"),
        };
    }

    /// <summary>실제첫메뉴정리가같은전체물리를쓰고현재battle은음악Start뒤에0으로바뀐다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void FirstMenuUsesRealOwnersAndKeepsMusicBeforeBattleReset(bool battle)
    {
        var scene = new Scene(); uint oldBase = scene.Game.PlayerRecords.Base; scene.Surface.Storage.OwnerMode.Battle = battle;
        scene.Local.ZoneStorage.Span.Fill(0xa5); scene.Game.Selection.MultipleCount.Value = 17; scene.Game.Selection.PriestSelections.Value = 0xdeadbeef;
        scene.Game.Selection.Mode.Value = 9; scene.Game.Selection.Selected.Value = 0xfedcba98; scene.Game.Selection.MouseType.Value = 0x89abcdef;
        scene.Cleanup(scene.Devices(), selectCursor: cursor => scene.Events.Add("root-cursor:" + cursor));
        Assert.NotEqual(oldBase, scene.Game.PlayerRecords.Base); Assert.True(scene.Game.PlayerRecords.Initialized);
        Assert.Equal(1u, scene.Game.Player.PlayerKnown); Assert.False(scene.Game.CoreData.Current.IsEmpty); Assert.False(scene.Surface.Storage.OwnerMode.Battle);
        Assert.Equal(0xabcd0000u, scene.Resources.FortCoreSeed.Value); Assert.Equal(0u, scene.Resources.Popup.Command); Assert.Equal("preserved", scene.Resources.Popup.Argument());
        Assert.Equal(0u, scene.NsInfo.Zone.Value); Assert.All(scene.Local.Zone.Span.ToArray(), value => Assert.Equal(0, value)); Assert.Equal(0xa5, scene.Local.ZoneStorage.Span[6]);
        Assert.All(scene.ChatBytes[..RawChatChannels.Stride], value => Assert.Equal(0, value)); Assert.Contains("music:" + battle, scene.Events);
        Assert.Equal(280.0, scene.Resources.Music.SongEnd.Value); Assert.Equal(110.0, scene.Local.ReconnectSaveEnd.Value);
        Assert.Equal(7.5 + scene.Surface.Storage.BridgeDecayPeriod, scene.Surface.Storage.BridgeDecayScan.Next); Assert.Equal(15000, scene.Surface.Storage.BridgeDecayScan.First);
        Assert.Equal(7.5, scene.Clock.Frame.Value.Game); Assert.Equal(55ul, scene.Clock.Frame.Value.Number); Assert.Equal(3, scene.ClockReads);
        Assert.True(scene.Events.IndexOf("kill") < scene.Events.IndexOf("cleanup")); Assert.True(scene.Events.IndexOf("cleanup") < scene.Events.IndexOf("uber"));
        Assert.False(scene.Game.Authority.Server); Assert.False(scene.Game.Authority.Boss); Assert.Equal(0, scene.Game.Selection.MultipleCount.Value); Assert.Equal(0xdeadbeefu, scene.Game.Selection.PriestSelections.Value);
        Assert.Equal(0u, scene.Game.Selection.Mode.Value); Assert.Equal(0u, scene.Game.Selection.Selected.Value); Assert.Equal(0u, scene.Game.Selection.MouseType.Value); Assert.Contains("root-cursor:1", scene.Events); Assert.DoesNotContain("select-cursor:1", scene.Events);
    }

    /// <summary>최초커서장치누락은그위치에서실패하며이후전역을정리한것처럼보이지않는다.</summary>
    [Fact]
    public void MissingCursorFailsAtSelectedPrefix()
    {
        var scene = new Scene(); scene.Game.FullDeckBox.Value = 7;
        Assert.Throws<InvalidOperationException>(() => scene.Cleanup(new()));
        Assert.Equal(0u, scene.Game.FullDeckBox.Value); Assert.Equal(0x12345678u, scene.NsInfo.Zone.Value); Assert.Equal(0xabcd1234u, scene.Resources.FortCoreSeed.Value); Assert.Equal(0, scene.ClockReads);
    }

    /// <summary>게임정지해제뒤음향장치가실패하면원문처럼Clock반납prefix를유지하고뒤커서는선택하지않는다.</summary>
    [Fact]
    public void MissingAudioPreservesSuccessfulGameResume()
    {
        var scene = new Scene(); scene.Clock.Clock.Pause(scene.Now); int resumes = 0;
        Assert.Throws<InvalidOperationException>(() => scene.Cleanup(new() { ResumeGame = () => { resumes++; scene.Clock.Clock.Resume(scene.Now); } }));
        Assert.Equal(1, resumes); Assert.False(scene.Clock.Clock.IsPaused()); Assert.Equal(0x12345678u, scene.NsInfo.Zone.Value); Assert.Empty(scene.Events);
    }

    /// <summary>현재Fort저장장치가없으면선행전역만반영하고AI/SP/Template반납은시작하지않는다.</summary>
    [Fact]
    public void MissingSelectedFortSavePreservesManagerAndBrain()
    {
        var scene = new Scene(); scene.Resources.Templates.Install(1, new(), "named"); scene.Surface.Storage.OwnerMode.Fort = true;
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Game.PlayerRecords.Records.Span.Slice(180 + 0xb0, 4), 0x12345678); uint random = scene.Sp.Random.State();
        Assert.Throws<InvalidOperationException>(() => scene.Cleanup(new() { CancelCursor = () => scene.Events.Add("cursor") }));
        Assert.True(scene.Resources.Templates.HasManager(1)); Assert.Equal(0u, scene.NsInfo.Zone.Value); Assert.Equal(random, scene.Sp.Random.State());
        Assert.Equal(0x12345678u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Game.PlayerRecords.Records.Span.Slice(180 + 0xb0, 4))); Assert.Equal(new[] { "cursor" }, scene.Events);
    }

    /// <summary>다른물리소유자를넘기면첫효과전에거부하고기존전역은유지한다.</summary>
    [Fact]
    public void WrongSurfaceFailsBeforeAnyEffect()
    {
        var scene = new Scene(); var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var other = new ClientSurfaceGlobals(game, new(local), new(), () => 0);
        scene.Game.FullDeckBox.Value = 71;
        Assert.Throws<ArgumentException>(() => ClientStateWorldCleanup.Cleanup(1, scene.Context(), other, scene.Sp, scene.Devices()));
        Assert.Equal(71u, scene.Game.FullDeckBox.Value); Assert.Empty(scene.Events); Assert.Equal(0, scene.ClockReads);
    }

    /// <summary>실제known0이아니면Player초기화는이전배열/주소/공장을먼저반납하지않는다.</summary>
    [Fact]
    public void PlayerInitializationRequiresKnownZeroBeforeReallocation()
    {
        var scene = new Scene(); uint before = scene.Game.PlayerRecords.Base; scene.Game.Player.PlayerKnown = 7;
        Assert.Throws<InvalidOperationException>(() => ClientPlayerRuntime.Initialize(scene.Game)); Assert.Equal(before, scene.Game.PlayerRecords.Base);
    }
}
