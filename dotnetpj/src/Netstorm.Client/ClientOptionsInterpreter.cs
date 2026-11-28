using Netstorm.Client.Audio;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 복원된435220의 값/장치 순서를 실제 같은 장기 소유자에 연결한다.
namespace Netstorm.Client;

/// <summary>옵션 reader와 후속 프레임/명령이 공유하는 Client 값이다. 생성은 설정/장치를 실행하지 않는다.</summary>
public sealed class ClientOptionGlobals
{
    /// <summary>원문 ClientMain.h의 기본 프레임 제한이다.</summary>
    public const int DefaultMaxFramesPerSecond = 75;
    /// <summary>같은 요청 논리/표시/창 기하다.</summary>
    public ClientWindowOptions Window { get; } = new();
    /// <summary>현재 원문 로그 필터/FILE/공유 버퍼의 장기 소유자다.</summary>
    public ClientTraceGlobals Trace { get; } = new();
    /// <summary>0054db2c의 최초0인 스레드 우선순위 값이다.</summary>
    public IntBox HighPriority { get; } = new();
    /// <summary>0054db40의 최초0인 초기 창 위치 값이다.</summary>
    public IntBox InitialWindowPosition { get; } = new();
    /// <summary>0054db0c의 최초0인 루프 양보 값이다.</summary>
    public IntBox SleepPerLoop { get; } = new();
    /// <summary>005318d8의 최초75이며 전체 해석마다75에서 다시 읽는다.</summary>
    public IntBox MaxFramesPerSecond { get; } = new() { Value = DefaultMaxFramesPerSecond };
    /// <summary>005318e0의 최초 BSS0인 초 단위 최소 갱신 간격이다.</summary>
    public DoubleBox FrameInterval { get; } = new();
}

/// <summary>ClientMain::InterpretOptions의 현재 복원 몸체다. 음향 값 읽기는 장치 Apply와 구별한다.</summary>
public sealed class ClientOptionsInterpreter
{
    /// <summary>현재 같은 설정/Registry다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>같은 압축/진단/커서/Player 값 소유자다.</summary>
    private readonly ClientGameplayGlobals _game;
    /// <summary>같은 도움말/디버그 현재 값이다.</summary>
    private readonly ClientLocalStateGlobals _local;
    /// <summary>같은 채팅 입력 전환 DWORD다.</summary>
    private readonly ClientChatWindowGlobals _chat;
    /// <summary>기한과 별개인 두 옵션을 해석하는 같은 부모다.</summary>
    private readonly ClientMonitorOptions _monitor;
    /// <summary>현재/공개 언어와 같은 실제 두 표 적재다.</summary>
    private readonly ClientLanguageOptions _language;
    /// <summary>같은 다섯 값 하위 몸체다.</summary>
    private readonly ClientValueOptions _values;
    /// <summary>같은 현재 구름 설정이다.</summary>
    private readonly ClientKencloudGlobals _cloud;
    /// <summary>현재 창/필터/프레임 값이다.</summary>
    private readonly ClientOptionGlobals _globals;
    /// <summary>열린 파일/enabled와 별개인 같은 기록 요청 DWORD다.</summary>
    private readonly UIntBox _recordRequested;
    /// <summary>같은 Client 물리를 검증하고 빌린다. 구성만으로 재해석하지 않는다.</summary>
    public ClientOptionsInterpreter(ConfigInterface configuration, ClientGameplayGlobals game, ClientLocalStateGlobals local, ClientConnectionGlobals connections,
        ClientChatWindowGlobals chat, ClientMonitorOptions monitor, ClientLanguageOptions language, ClientValueOptions values,
        ClientKencloudGlobals cloud, ClientOptionGlobals globals, UIntBox recordRequested)
    {
        ArgumentNullException.ThrowIfNull(configuration); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(local);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(chat); ArgumentNullException.ThrowIfNull(monitor); ArgumentNullException.ThrowIfNull(language);
        ArgumentNullException.ThrowIfNull(values); ArgumentNullException.ThrowIfNull(cloud); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(recordRequested);
        if (!ReferenceEquals(game, local.Gameplay) || !ReferenceEquals(connections.LocalState, local)
            || !monitor.Borrows(local.MonitorTime, connections.Activity, configuration) || !language.Borrows(configuration)
            || !values.Borrows(configuration, game, local, connections)) throw new ArgumentException("전체 옵션의 같은 Client 소유자 필요", nameof(local));
        _configuration = configuration; _game = game; _local = local; _chat = chat; _monitor = monitor; _language = language;
        _values = values; _cloud = cloud; _globals = globals; _recordRequested = recordRequested;
    }
    /// <summary>현재 signed DWORD를 읽으며 누락 값은 유지한다.</summary>
    private int Read(string key, int value) { _configuration.ReadInt(key, ref value); return value; }
    /// <summary>압축/감독/필터/언어/창→도움말/기록→음향 값/안내→파일/구름/프레임→치트/네트워크 순서다.</summary>
    public void Interpret(bool fsrAvailable, ClientWindowOptionHooks window)
    {
        _game.GameAux.SquidTransmission.Compress = Read("cs", _game.GameAux.SquidTransmission.Compress);
        _monitor.Interpret(); _globals.Trace.Log.Level = Read("tracelevel", _globals.Trace.Log.Level); _game.AiDebug.ReadOptions(_configuration);
        _language.Interpret(); _values.InterpretCursor(); _globals.Window.Interpret(_configuration, fsrAvailable, window);
        _globals.HighPriority.Value = Read("highPriorityValue", _globals.HighPriority.Value);
        _globals.InitialWindowPosition.Value = Read("initWindowPos", _globals.InitialWindowPosition.Value);
        _local.HelpWord.ReadOptions(_configuration); _recordRequested.Value = 0; _local.DebugBurst.Value = 0;
        _ = ClientAudioOptions.Read(_configuration); _values.InterpretMainMenu(); _chat.ReadOptions(_configuration); _values.InterpretFortFiles();
        _globals.SleepPerLoop.Value = Read("sleepPerLoop", _globals.SleepPerLoop.Value); _cloud.ReadOptions(_configuration);
        _globals.MaxFramesPerSecond.Value = ClientOptionGlobals.DefaultMaxFramesPerSecond;
        _globals.MaxFramesPerSecond.Value = Read("maxFPS", _globals.MaxFramesPerSecond.Value);
        _globals.FrameInterval.Value = _globals.MaxFramesPerSecond.Value > 0 ? 1.0 / _globals.MaxFramesPerSecond.Value : 0.0;
        _values.InterpretScriptMenu(); _values.InterpretNetwork();
    }
}
