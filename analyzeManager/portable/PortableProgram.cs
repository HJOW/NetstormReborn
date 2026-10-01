using System.Text;
using System.Text.Json;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// 리눅스용 YouTube 전용 진입점. Windows 판(<c>Program</c>)과 같은 <c>call</c>·<c>mcp</c> 형식을 쓰되 <c>youtube_*</c> 도구만 제공한다.
/// 원본 게임을 실행하지 않으므로 AGENTS.md 의 게임 실행 확인 대상이 아니다.
/// </summary>
internal static class PortableProgram
{
    /// <summary>명령줄 설명 (명시적으로 요청할 때만 출력해 MCP stdout 을 오염시키지 않는다)</summary>
    private const string Usage = """
        Netstorm.AnalyzeManager (YouTube 전용, 리눅스 등) — 원본 게임 플레이 영상 분석 도구

        Netstorm.AnalyzeManager [--repo 저장소] mcp
        Netstorm.AnalyzeManager [--repo 저장소] call 도구 [--args-file JSON파일 | --json JSON]

        도구(게임 실행 없음, 인터넷 사용):
              youtube_probe, youtube_list, youtube_frames, youtube_clip, youtube_note, youtube_videos
              예: call youtube_frames --json {"url":"https://youtu.be/CI3dCrUt4tY","times":"600, 30:00"}
        필요 프로그램: yt-dlp, ffmpeg, ffprobe (PATH 또는 NETSTORM_YTDLP·NETSTORM_FFMPEG·NETSTORM_FFPROBE), PREPARE.sh 로 설치.
        원본 게임 조작 도구(start_session 등)는 Windows 판에만 있습니다.
        입력 스키마: docs/analyze-manager.md "YouTube 영상 분석". 결과는 UTF-8 JSON, 오류 종료 코드는 2입니다.
        """;

    /// <summary>CLI 와 MCP 가 같은 YouTube 엔진·경로·오류 정책을 쓰게 한다.</summary>
    private static async Task<int> Main(string[] args)
    {
        Console.InputEncoding = new UTF8Encoding(false);
        Console.OutputEncoding = new UTF8Encoding(false);
        bool mcp = args.Contains("mcp");
        try
        {
            var arguments = args.ToList();
            string? repository = CommandLine.TakeOption(arguments, "--repo");
            if (arguments.Count == 0 || arguments[0] is "--help" or "help")
            {
                Console.WriteLine(Usage);
                return 0;
            }
            var youtube = new YouTubeAnalyzer(CommandLine.FindRepository(repository));
            if (arguments.Count == 1 && arguments[0] == "mcp")
            {
                HostApplicationBuilder builder = Host.CreateApplicationBuilder();
                builder.Logging.ClearProviders();
                // 서버 진단은 stderr 에만 기록하고 stdout 은 MCP 프로토콜 전용으로 둔다.
                builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
                builder.Logging.SetMinimumLevel(LogLevel.Warning);
                builder.Services.AddMcpServer().WithStdioServerTransport().WithTools(new YouTubeTools(youtube));
                using IHost host = builder.Build();
                await host.RunAsync();
                return 0;
            }
            string? jsonPath = CommandLine.TakeOption(arguments, "--args-file");
            string? jsonText = CommandLine.TakeOption(arguments, "--json");
            if (arguments.Count != 2 || arguments[0] != "call" || (jsonPath != null && jsonText != null))
                throw new ArgumentException(Usage);
            if (!arguments[1].StartsWith("youtube_", StringComparison.Ordinal))
                throw new ArgumentException($"이 빌드는 youtube_* 도구만 제공합니다(게임 조작은 Windows 판): {arguments[1]}");
            if (jsonPath != null)
            {
                if (new FileInfo(jsonPath).Length > 64_000) throw new ArgumentException("인자 JSON은 64 KB 이하여야 합니다.");
                jsonText = await File.ReadAllTextAsync(jsonPath, Encoding.UTF8);
            }
            if (jsonText?.Length > 64_000) throw new ArgumentException("인자 JSON이 너무 큽니다.");
            AnalysisRequest request = JsonSerializer.Deserialize<AnalysisRequest>(jsonText ?? "{}", SessionStore.Json)
                ?? throw new ArgumentException("인자는 JSON 객체여야 합니다.");
            using var cancellation = new CancellationTokenSource();
            // Ctrl+C 는 외부 프로그램(yt-dlp·ffmpeg)을 정리한 뒤 끝내도록 취소 토큰으로 전달한다.
            Console.CancelKeyPress += (_, signal) => { signal.Cancel = true; cancellation.Cancel(); };
            AnalysisResult result = await youtube.ExecuteSafeAsync(arguments[1], request, cancellation.Token);
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
