using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 전체 복원 옵션 부모의 현재 소유자·앞뒤 효과 순서와 실패 접두를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 Config와 같은 하위 본체로 순서를 검증하며 창 효과만 명시 입력으로 공급한다.</summary>
public sealed class ClientOptionsInterpreterTests
{
    /// <summary>같은 Client 소유자와 자기 Config 구독을 한 수명으로 조립한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 설정 저장소다.</summary>
        public readonly ConfigInterface Config = new(null);
        /// <summary>압축/진단/Player/커서의 현재 물리다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 현재 시각/감독/도움말/주소다.</summary>
        public readonly ClientLocalStateGlobals Local;
        /// <summary>같은 활동/지연 분포다.</summary>
        public readonly ClientConnectionGlobals Connections;
        /// <summary>같은 입력 전환 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Chat = new();
        /// <summary>같은 서버 정보다.</summary>
        public readonly ClientNsInfoGlobals Info = new();
        /// <summary>같은 실제 구름 값이다.</summary>
        public readonly ClientKencloudGlobals Cloud = new();
        /// <summary>같은 장기 옵션/진단/요청 기하다.</summary>
        public readonly ClientOptionGlobals Globals = new();
        /// <summary>파일/enabled와 별개인 기록 요청이다.</summary>
        public readonly UIntBox Record = new();
        /// <summary>같은 파일 저장 억제 값이다.</summary>
        public readonly UIntBox ReadOnly = new();
        /// <summary>현재/공개 언어 소유자다.</summary>
        public readonly ClientLanguageGlobals LanguageGlobals = new();
        /// <summary>동일 Config/감독 구독이다.</summary>
        public readonly ClientMonitorOptions Monitor;
        /// <summary>동일 Config/언어 구독이다.</summary>
        public readonly ClientLanguageOptions Language;
        /// <summary>동일 다섯 값 하위 본체다.</summary>
        public readonly ClientValueOptions Values;
        /// <summary>검사 대상인 실제 복원 부모다.</summary>
        public readonly ClientOptionsInterpreter Parent;
        /// <summary>구독을 먼저 설치하고 나머지 실제 소유자를 대여한다.</summary>
        public Scene()
        {
            Local = new(Game, new()); Connections = new(Local); Monitor = new(Local.MonitorTime, Connections.Activity, Config);
            Language = new(LanguageGlobals, Config); Values = new(Config, Game, Local, Connections, Info, ReadOnly);
            Parent = new(Config, Game, Local, Connections, Chat, Monitor, Language, Values, Cloud, Globals, Record);
        }
        /// <summary>서로 독립인 두 구독만 반납한다.</summary>
        public void Dispose() { Language.Dispose(); Monitor.Dispose(); }
    }
    /// <summary>창 앞단과 뒤 도움말/요청/음향 값/안내/파일/프레임/네트워크가 같은 현재 물리를 사용한다.</summary>
    [Fact]
    public void RealChildrenRespectWindowAndResetOrder()
    {
        using var scene = new Scene();
        scene.Config.Set("cs", "-1"); scene.Config.Set("tracelevel", "-7"); scene.Config.Set("showSoftAsserts", "1"); scene.Config.Set("currentLanguage", "french");
        scene.Config.Set("rotmode", "-2147483648"); scene.Config.Set("SCREENW", "800"); scene.Config.Set("SCREENH", "600");
        scene.Config.Set("highPriorityValue", "-8"); scene.Config.Set("initWindowPos", "5"); scene.Config.Set("helpWord", "new");
        scene.Config.Set("tellTips", "-1"); scene.Config.Set("inputLineOff", "-9"); scene.Config.Set("readOnlyForts", "-2");
        scene.Config.Set("sleepPerLoop", "3"); scene.Config.Set("cloudSpeed", "7"); scene.Config.Set("maxFPS", "120"); scene.Config.Set("gameServerPort", "7002");
        scene.Globals.HighPriority.Value = 17; scene.Local.HelpWord.Interpret("old"); scene.Record.Value = 19; scene.Local.DebugBurst.Value = 51;
        // Config 변경 구독의 앞선 결과를 입력 상태로 되돌려 부모 자체의 감독/언어 실행을 분리해 검증한다.
        scene.Connections.Activity.AutoRestart.Value = 17; scene.LanguageGlobals.Current.Value = -1; scene.LanguageGlobals.Published.Value = -1;
        int moves = 0;
        scene.Parent.Interpret(false, new()
        {
            WindowRectangle = () =>
            {
                Assert.Equal(-1, scene.Game.GameAux.SquidTransmission.Compress); Assert.Equal(-7, scene.Globals.Trace.Log.Level); Assert.Equal(1, scene.Game.AiDebug.Checking.Value);
                Assert.Equal(2, scene.LanguageGlobals.Published.Value); Assert.Equal(0x80000000u, scene.Game.CursorGlobals.RotationMode.Value);
                Assert.Equal(1u, scene.Connections.Activity.AutoRestart.Value);
                Assert.Equal(17, scene.Globals.HighPriority.Value); Assert.Equal("old", scene.Local.HelpWord.Value); Assert.Equal(19u, scene.Record.Value); Assert.Equal(51u, scene.Local.DebugBurst.Value);
                return new(0, 0, 650, 510);
            },
            ClientRectangle = () => new(0, 0, 640, 480), Move = (_, _, _, _, _) => moves++,
        });
        Assert.Equal(1, moves); Assert.Equal(-8, scene.Globals.HighPriority.Value); Assert.Equal(5, scene.Globals.InitialWindowPosition.Value);
        Assert.Equal("new", scene.Local.HelpWord.Value); Assert.Equal(0u, scene.Record.Value); Assert.Equal(0u, scene.Local.DebugBurst.Value);
        Assert.Equal(uint.MaxValue, scene.Local.TellTips.Value); Assert.Equal(-9, scene.Chat.InputLineOff.Value); Assert.Equal(unchecked((uint)-2), scene.ReadOnly.Value);
        Assert.Equal(3, scene.Globals.SleepPerLoop.Value); Assert.Equal(7u, scene.Cloud.Value(ClientKencloudGlobals.Option.Speed));
        Assert.Equal(120, scene.Globals.MaxFramesPerSecond.Value); Assert.Equal(1.0 / 120, scene.Globals.FrameInterval.Value); Assert.Equal(7002u, scene.Game.GameServerPort.Value);
    }
    /// <summary>같은 창에서는 장치를 선택하지 않고 누락 FPS만 원문75에서 다시 읽는다.</summary>
    [Fact]
    public void MissingFpsRestartsAtOriginalDefaultAndPreservesOtherValues()
    {
        using var scene = new Scene(); scene.Globals.Window.PresentWidth = 640; scene.Globals.Window.PresentHeight = 480;
        scene.Globals.MaxFramesPerSecond.Value = 999; scene.Globals.HighPriority.Value = -12; scene.Local.HelpWord.Interpret("kept");
        scene.Parent.Interpret(false, new());
        Assert.Equal(75, scene.Globals.MaxFramesPerSecond.Value); Assert.Equal(1.0 / 75, scene.Globals.FrameInterval.Value);
        Assert.Equal(-12, scene.Globals.HighPriority.Value); Assert.Equal("kept", scene.Local.HelpWord.Value);
    }
    /// <summary>창 효과 실패는 앞단 압축/필터/요청만 남기고 뒤 도움말/기록/프레임 쓰기를 생략한다.</summary>
    [Fact]
    public void WindowFailureKeepsPrefixAndSkipsLaterResets()
    {
        using var scene = new Scene(); scene.Config.Set("SCREENW", "800"); scene.Config.Set("SCREENH", "600"); scene.Config.Set("cs", "-2");
        scene.Record.Value = 11; scene.Local.DebugBurst.Value = 12; scene.Globals.MaxFramesPerSecond.Value = 19;
        Assert.Throws<IOException>(() => scene.Parent.Interpret(false, new() { WindowRectangle = () => throw new IOException("창 효과 실패") }));
        Assert.Equal(-2, scene.Game.GameAux.SquidTransmission.Compress); Assert.Equal(800, scene.Globals.Window.PresentWidth);
        Assert.Equal(11u, scene.Record.Value); Assert.Equal(12u, scene.Local.DebugBurst.Value); Assert.Equal(19, scene.Globals.MaxFramesPerSecond.Value);
    }
    /// <summary>끝 주소 실패는 앞선 요청 초기화/프레임 계산/치트 초기화를 보존하며 현재 lag를 바꾸지 않는다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public void LateFailureKeepsEarlierFrameAndRecordEffects(int fps)
    {
        using var scene = new Scene(); scene.Globals.Window.PresentWidth = 640; scene.Globals.Window.PresentHeight = 480;
        scene.Config.SetInt("maxFPS", fps); scene.Config.Set("testBattleAddress", new string('A', 65)); scene.Config.SetInt("lag", 2);
        scene.Record.Value = 51; scene.Local.DebugBurst.Value = 71; scene.Game.Player.Special = 1; scene.Game.SpecialAuxiliary.Value = 1;
        sbyte[] distribution = scene.Connections.PacketDelays().Distribution;
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Parent.Interpret(false, new()));
        Assert.Equal(0u, scene.Record.Value); Assert.Equal(0u, scene.Local.DebugBurst.Value); Assert.Equal(fps, scene.Globals.MaxFramesPerSecond.Value); Assert.Equal(0.0, scene.Globals.FrameInterval.Value);
        Assert.Equal(0u, scene.Game.Player.Special); Assert.Equal(0u, scene.Game.SpecialAuxiliary.Value); Assert.Same(distribution, scene.Connections.PacketDelays().Distribution);
    }
    /// <summary>다른 Config의 언어 하위는 구성 시 거부해 실제 앞단 쓰기를 시작하지 않는다.</summary>
    [Fact]
    public void RejectsDifferentConfigurationChildrenBeforeAnyEffect()
    {
        using var scene = new Scene(); var otherConfig = new ConfigInterface(null); using var language = new ClientLanguageOptions(new(), otherConfig);
        Assert.Throws<ArgumentException>(() => new ClientOptionsInterpreter(scene.Config, scene.Game, scene.Local, scene.Connections, scene.Chat,
            scene.Monitor, language, scene.Values, scene.Cloud, scene.Globals, scene.Record));
    }
}
