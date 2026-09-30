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

        Netstorm.AnalyzeManager [--repo 저장소] mcp
        Netstorm.AnalyzeManager [--repo 저장소] call 도구 [--args-file JSON파일 | --json JSON]
        Netstorm.AnalyzeManager [--repo 저장소] guide --session 세션ID [--steps-file UTF8파일]

        도구: list_sessions, start_session, game_status, capture_state,
              game_input, wait_for_change, record_observation, set_guide_steps, end_session
        AGENTS.md에 지정된 시스템, 또는 개발자가 수동 컨트롤 분석을 직접 요청한 작업 단계에서만 별도 확인 없이 게임을 실행할 수 있습니다.
        그 밖의 경우에는 start_session 전에 목적과 필요성을 설명하고 개발자 확인을 받으세요.
        guide는 이미 실행 중인 세션의 게임 옆에 안내 창을 열며 게임을 새로 실행하지 않습니다.
        예: call start_session --json {"label":"메뉴 관찰"}
        입력 스키마와 MCP 설정: docs/analyze-manager.md
        결과는 UTF-8 JSON, 오류 종료 코드는 2입니다. 게임과 증거는 세션 폴더에 남습니다.
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
            string? repository = TakeOption(arguments, "--repo");
            if (arguments.Count == 0 || arguments[0] is "--help" or "help")
            {
                Console.WriteLine(Usage);
                return 0;
            }
            var engine = new AnalysisEngine(FindRepository(repository));
            if (arguments.Count > 0 && arguments[0] == "guide")
            {
                arguments.RemoveAt(0);
                string sessionId = TakeOption(arguments, "--session") ?? throw new ArgumentException("guide에는 --session이 필요합니다.");
                string? stepsPath = TakeOption(arguments, "--steps-file");
                if (arguments.Count != 0) throw new ArgumentException(Usage);
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new GuidedForm(engine.Store, sessionId, stepsPath));
                return 0;
            }
            if (arguments.Count == 1 && arguments[0] == "mcp")
            {
                HostApplicationBuilder builder = Host.CreateApplicationBuilder();
                builder.Logging.ClearProviders();
                // 서버 진단은 stderr에만 기록하고 stdout은 MCP 프로토콜 전용으로 둔다.
                builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
                builder.Logging.SetMinimumLevel(LogLevel.Warning);
                builder.Services.AddMcpServer().WithStdioServerTransport().WithTools(new ExplorerTools(engine));
                using IHost host = builder.Build();
                await host.RunAsync();
                return 0;
            }
            string? jsonPath = TakeOption(arguments, "--args-file");
            string? jsonText = TakeOption(arguments, "--json");
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

    /// <summary>옵션 중복과 누락을 거부하며 소비한 인자만 제거한다.</summary>
    private static string? TakeOption(List<string> arguments, string name)
    {
        int index = arguments.IndexOf(name);
        if (index < 0) return null;
        if (index + 1 >= arguments.Count || arguments.LastIndexOf(name) != index)
            throw new ArgumentException($"옵션 값 누락 또는 중복: {name}");
        string value = arguments[index + 1];
        arguments.RemoveRange(index, 2);
        return value;
    }

    /// <summary>명시 경로나 현재 작업/실행 경로의 부모에서 원본이 있는 저장소를 찾는다.</summary>
    private static string FindRepository(string? explicitPath)
    {
        if (explicitPath != null)
        {
            string path = Path.GetFullPath(explicitPath);
            if (!File.Exists(Path.Combine(path, "originals", "Netstorm.exe"))) throw new ArgumentException("저장소에 originals/Netstorm.exe가 없습니다.");
            return path;
        }
        // MCP 클라이언트의 작업 경로가 달라도 빌드 출력의 부모로 저장소를 찾는다.
        foreach (string start in new[] { Environment.CurrentDirectory, AppContext.BaseDirectory })
        {
            DirectoryInfo? directory = new(start);
            // 루트까지 올라가되 파일을 바꾸지 않고 존재 여부만 검사한다.
            while (directory != null)
            {
                if (File.Exists(Path.Combine(directory.FullName, "AGENTS.md")) && File.Exists(Path.Combine(directory.FullName, "originals", "Netstorm.exe"))) return directory.FullName;
                directory = directory.Parent;
            }
        }
        throw new ArgumentException("--repo로 NetstormReborn 저장소를 지정하세요.");
    }
}
