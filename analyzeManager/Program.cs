using System.Text;
using System.Text.Json;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Netstorm.AnalyzeManager;

/// <summary>사람용 UI 없이 AI가 명령줄 또는 stdio MCP로 호출하는 진입점.</summary>
internal static class Program
{
    /// <summary>명령줄 설명은 명시적으로 요청할 때만 출력해 MCP stdout을 오염시키지 않는다.</summary>
    private const string Usage = """
        Netstorm.AnalyzeManager — 원본 게임 분석 도구 (Windows 10/11)

        Netstorm.AnalyzeManager [--repo 저장소] [--fps 30|60] mcp
        Netstorm.AnalyzeManager [--repo 저장소] call 도구 [--args-file JSON파일 | --json JSON] [--fps 30|60]
        Netstorm.AnalyzeManager [--repo 저장소] guide --session 세션ID [--steps-file UTF8파일] [--fps 30|60]
        Netstorm.AnalyzeManager [--repo 저장소] record-play --session 세션ID [--fps 30|60]

        도구: list_sessions, start_session, game_status, capture_state,
              game_input, wait_for_change, record_observation, set_guide_steps, end_session
        YouTube 영상 분석(게임 실행 없음, 인터넷 사용):
              youtube_probe, youtube_list, youtube_frames, youtube_clip, youtube_note, youtube_videos
              예: call youtube_frames --json {"url":"https://youtu.be/CI3dCrUt4tY","times":"600, 30:00"}
        AGENTS.md에 지정된 시스템, 또는 개발자가 수동 컨트롤 분석을 직접 요청한 작업 단계에서만 별도 확인 없이 게임을 실행할 수 있습니다.
        그 밖의 경우에는 start_session 전에 목적과 필요성을 설명하고 개발자 확인을 받으세요.
        guide는 이미 실행 중인 세션의 게임 옆에 안내 창을 열며 게임을 새로 실행하지 않습니다.
        record-play는 지침 없는 녹화 창을 열고 playingVideos/세션ID에 분할 저장합니다.
        분석·녹화는 기본 30FPS이며 --fps 60 또는 JSON/MCP의 fps=60으로 선택합니다.
        start_session은 세션의 recording/에 영상·음성·입력을 연속 녹화하며 end_session 때 닫습니다.
        guide/record-play로 전환하면 자동 녹화를 닫고 사용자 녹화 창을 엽니다.
        예: call start_session --json {"label":"메뉴 관찰"}
        입력 스키마와 MCP 설정: docs/analyze-manager.md
        결과는 UTF-8 JSON, 오류 종료 코드는 2입니다. 게임과 일반 증거는 세션 폴더에 남습니다.
        record-play 녹화물은 playingVideos/세션ID에 남습니다.
        """;

    /// <summary>CLI와 MCP가 동일한 엔진·경로·예외 정책을 사용하게 한다.</summary>
    [STAThread]
    private static async Task<int> Main(string[] args)
    {
        Console.InputEncoding = new UTF8Encoding(false);
        Console.OutputEncoding = new UTF8Encoding(false);
        bool mcp = args.Contains("mcp");
        try
        {
            var arguments = args.ToList();
            string? repository = CommandLine.TakeOption(arguments, "--repo");
            int? cliFps = AnalysisFrameRate.Parse(CommandLine.TakeOption(arguments, "--fps"));
            if (arguments.Count == 0 || arguments[0] is "--help" or "help")
            {
                Console.WriteLine(Usage);
                return 0;
            }
            var engine = new AnalysisEngine(CommandLine.FindRepository(repository), cliFps ?? AnalysisFrameRate.Default);
            if (arguments[0] == "record-auto")
            {
                arguments.RemoveAt(0);
                string sessionId = CommandLine.TakeOption(arguments, "--session") ?? throw new ArgumentException("--session이 필요합니다.");
                string runId = CommandLine.TakeOption(arguments, "--run") ?? throw new ArgumentException("--run이 필요합니다.");
                if (arguments.Count != 0) throw new ArgumentException(Usage);
                return await AutomaticRecording.RunAsync(engine.Store, sessionId, runId, cliFps ?? AnalysisFrameRate.Default);
            }
            if (arguments.Count > 0 && (arguments[0] is "guide" or "record-play"))
            {
                bool freePlay = arguments[0] == "record-play";
                arguments.RemoveAt(0);
                string sessionId = CommandLine.TakeOption(arguments, "--session") ?? throw new ArgumentException("안내 창에는 --session이 필요합니다.");
                string? stepsPath = freePlay ? null : CommandLine.TakeOption(arguments, "--steps-file");
                if (arguments.Count != 0) throw new ArgumentException(Usage);
                // WinForms를 STA 진입 스레드에서 시작하도록 녹화 프로세스 마감을 동기적으로 기다린다.
                AutomaticRecording.StopAsync(engine.Store, sessionId).GetAwaiter().GetResult();
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new GuidedForm(engine.Store, sessionId, stepsPath, freePlay, cliFps));
                return 0;
            }
            if (arguments.Count == 1 && arguments[0] == "mcp")
            {
                HostApplicationBuilder builder = Host.CreateApplicationBuilder();
                builder.Logging.ClearProviders();
                // 서버 진단은 stderr에만 기록하고 stdout은 MCP 프로토콜 전용으로 둔다.
                builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
                builder.Logging.SetMinimumLevel(LogLevel.Warning);
                builder.Services.AddMcpServer().WithStdioServerTransport().WithTools(new ExplorerTools(engine)).WithTools(new YouTubeTools(engine.YouTube));
                using IHost host = builder.Build();
                await host.RunAsync();
                return 0;
            }
            string? jsonPath = CommandLine.TakeOption(arguments, "--args-file");
            string? jsonText = CommandLine.TakeOption(arguments, "--json");
            if (arguments.Count != 2 || arguments[0] != "call" || (jsonPath != null && jsonText != null))
                throw new ArgumentException(Usage);
            if (jsonPath != null)
            {
                if (new FileInfo(jsonPath).Length > 64_000) throw new ArgumentException("인자 JSON은 64 KB 이하여야 합니다.");
                jsonText = await File.ReadAllTextAsync(jsonPath, Encoding.UTF8);
            }
            if (jsonText?.Length > 64_000) throw new ArgumentException("인자 JSON이 너무 큽니다.");
            AnalysisRequest request = JsonSerializer.Deserialize<AnalysisRequest>(jsonText ?? "{}", SessionStore.Json)
                ?? throw new ArgumentException("인자는 JSON 객체여야 합니다.");
            if (cliFps.HasValue && request.Fps.HasValue) throw new ArgumentException("--fps와 JSON fps를 중복 지정하지 마세요.");
            request.Fps ??= cliFps;
            using var cancellation = new CancellationTokenSource();
            // Ctrl+C도 입력 해제와 실패 기록을 거친 뒤 끝내도록 취소 토큰으로 전달한다.
            Console.CancelKeyPress += (_, signal) => { signal.Cancel = true; cancellation.Cancel(); };
            AnalysisResult result = await engine.ExecuteAsync(arguments[1], request, cancellation.Token);
            Console.WriteLine(JsonSerializer.Serialize(new { isError = result.IsError, data = result.Data, imagePath = result.ImagePath }, SessionStore.Json));
            return result.IsError ? 2 : 0;
        }
        catch (Exception error)
        {
            string json = JsonSerializer.Serialize(new { isError = true, error = error.Message }, SessionStore.Json);
            if (mcp) Console.Error.WriteLine(json); else Console.WriteLine(json);
            return 2;
        }
    }
}
