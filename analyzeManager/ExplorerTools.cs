using System.ComponentModel;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;

namespace Netstorm.AnalyzeManager;

/// <summary>AI 클라이언트에 공개하는 좁은 도구 집합. CLI와 같은 분석 엔진을 호출한다. YouTube 도구는 <see cref="YouTubeTools"/>에 있다.</summary>
[McpServerToolType]
public sealed class ExplorerTools
{
    private readonly AnalysisEngine _engine;

    /// <summary>서버가 관리하는 엔진을 연결한다.</summary>
    public ExplorerTools(AnalysisEngine engine) => _engine = engine;

    /// <summary>도구 결과를 MCP 텍스트/구조화 데이터/선택적 이미지로 변환한다.</summary>
    private async Task<CallToolResult> Run(string name, AnalysisRequest request, CancellationToken cancellation)
        => await YouTubeTools.ToCallToolResult(await _engine.ExecuteAsync(name, request, cancellation), cancellation);

    /// <summary>재접속한 AI가 최근 세션의 ID와 실행 상태를 찾는다.</summary>
    [McpServerTool(Name = "list_sessions", ReadOnly = true, OpenWorld = false)]
    [Description("최근 분석 세션 20개의 sessionId, 실행 상태, 보고서 경로를 반환합니다. 먼저 실행 중인 세션이 있는지 확인하세요.")]
    public Task<CallToolResult> ListSessions(CancellationToken cancellation) => Run("list_sessions", new(), cancellation);

    /// <summary>격리된 원본 게임을 창 모드로 실행한다.</summary>
    [McpServerTool(Name = "start_session", OpenWorld = false, Destructive = false)]
    [Description("실제 게임을 실행합니다. AGENTS.md에 지정된 시스템(IP 10.0.0.15·호스트명 vm-debian-codex, IP 192.168.0.94·호스트명 HJOW-Athlon)에서, 또는 개발자가 기존 게임 수동 컨트롤 방식의 분석을 직접 요청한 작업 단계에서는 개발자 확인 없이 호출할 수 있습니다. 그 밖의 시스템에서는 기존 실행 중단 지시의 재개가 필요하며, 호출 전에 개발자에게 목적과 필요성을 알리고 명시적인 확인을 받으세요. originals를 복사해 창 모드 1024x768로 실행하고 새 sessionId와 첫 화면을 반환합니다.")]
    public Task<CallToolResult> StartSession([Description("분석 목적, 200자 이하")] string label = "",
        [Description("분석·연속 녹화 FPS: 30 또는 60. 생략하면 서버 설정을 사용하며 기본은 30입니다.")] int? fps = null,
        CancellationToken cancellation = default)
        => Run("start_session", new() { Label = label, Fps = fps }, cancellation);

    /// <summary>게임 상태를 입력이나 포커스 변경 없이 조회한다.</summary>
    [McpServerTool(Name = "game_status", ReadOnly = true, OpenWorld = false)]
    [Description("세션의 게임 실행 여부, 현재 창 제목과 물리 픽셀 크기, 보고서 경로를 조회합니다.")]
    public Task<CallToolResult> GameStatus(string sessionId, CancellationToken cancellation)
        => Run("game_status", new() { SessionId = sessionId }, cancellation);

    /// <summary>전체 또는 관심 영역을 캡처하며 동일 PNG는 재사용한다.</summary>
    [McpServerTool(Name = "capture_state", OpenWorld = false, Destructive = false)]
    [Description("게임을 전면에 두고 화면 증거를 저장합니다. region은 전체 클라이언트 기준 x,y,width,height입니다. 동일 화면 파일은 재사용하며 이미지 반환만 생략할 수 있습니다.")]
    public Task<CallToolResult> CaptureState(string sessionId, string region = "", bool includeImage = true, CancellationToken cancellation = default)
        => Run("capture_state", new() { SessionId = sessionId, Region = region, IncludeImage = includeImage }, cancellation);

    /// <summary>한 번의 입력과 전후 화면을 저장한다.</summary>
    [McpServerTool(Name = "game_input", OpenWorld = false, Destructive = true)]
    [Description("소유한 게임에 move/click/drag/key 입력을 보내고 전후 화면을 기록합니다. 좌표는 전체 클라이언트 물리 픽셀입니다. click의 durationMs는 버튼을 누르고 있는 시간, drag의 durationMs는 한 구간을 끄는 시간입니다. drag는 viaX/viaY(경유점)·holdViaMs(경유점에서 누른 채 기다림)·holdMs(도착점에서 떼기 전 기다림)로 '누른 채 바깥으로 나갔다 돌아오기' 같은 동작을 만들 수 있습니다. 키 예: ESCAPE, ENTER, CTRL+A. 전체화면 단축키는 금지합니다. 게임 진행 상태를 변경합니다.")]
    public Task<CallToolResult> GameInput(string sessionId, string kind, int x = 0, int y = 0, int toX = 0, int toY = 0,
        string button = "left", string key = "", int durationMs = 80, int settleMs = 300, bool includeImage = true,
        int? viaX = null, int? viaY = null, int holdViaMs = 0, int holdMs = 0,
        CancellationToken cancellation = default)
        => Run("game_input", new() { SessionId = sessionId, Kind = kind, X = x, Y = y, ToX = toX, ToY = toY,
            Button = button, Key = key, DurationMs = durationMs, SettleMs = settleMs, IncludeImage = includeImage,
            ViaX = viaX, ViaY = viaY, HoldViaMs = holdViaMs, HoldMs = holdMs }, cancellation);

    /// <summary>애니메이션이 적은 관심 영역에서 조건이 변할 때까지 제한된 시간 동안 관찰한다.</summary>
    [McpServerTool(Name = "wait_for_change", OpenWorld = false, Destructive = false)]
    [Description("기준 화면 대비 threshold 비율 이상 픽셀이 바뀌면 반환합니다. 기본은 세션의 30/60FPS로 비교하며 fps로 변경할 수 있습니다. timeoutMs는 최대 30000, pollMs=0은 fps를 사용하고 1~5000은 간격을 직접 지정합니다. 구름/애니메이션을 피하도록 region을 지정하세요. 시작/마지막 PNG와 별도 연속 녹화가 남습니다. 시간 초과는 matched=false입니다.")]
    public Task<CallToolResult> WaitForChange(string sessionId, string region = "", int timeoutMs = 5000, int pollMs = 0,
        double threshold = 0.01, bool includeImage = true, int? fps = null, CancellationToken cancellation = default)
        => Run("wait_for_change", new() { SessionId = sessionId, Region = region, TimeoutMs = timeoutMs,
            PollMs = pollMs, Threshold = threshold, IncludeImage = includeImage, Fps = fps }, cancellation);

    /// <summary>관찰자의 메모에 같은 세션의 증거 해시를 연결한다.</summary>
    [McpServerTool(Name = "record_observation", OpenWorld = false, Destructive = false)]
    [Description("AI가 해석한 관찰을 한국어 보고서에 남깁니다. note는 8000자 이하, evidenceHash는 같은 세션의 캡처 SHA-256입니다. 메모는 검증된 게임 규칙과 구분해 기록됩니다.")]
    public Task<CallToolResult> RecordObservation(string sessionId, string note, string evidenceHash = "", CancellationToken cancellation = default)
        => Run("record_observation", new() { SessionId = sessionId, Note = note, EvidenceHash = evidenceHash }, cancellation);

    /// <summary>사용자 직접 조작 창에 표시할 단계들을 줄 단위로 저장한다.</summary>
    [McpServerTool(Name = "set_guide_steps", OpenWorld = false, Destructive = false)]
    [Description("사용자 직접 조작용 안내를 세션에 저장합니다. steps의 빈 줄을 제외한 각 줄이 한 단계입니다. 녹화 시작 전에 호출하고 guide --session ID로 안내 창을 여세요. 최대 64 KB입니다.")]
    public Task<CallToolResult> SetGuideSteps(string sessionId, string steps, CancellationToken cancellation = default)
        => Run("set_guide_steps", new() { SessionId = sessionId, Steps = steps }, cancellation);

    /// <summary>원본 종료를 요청하거나 해당 세션의 프로세스만 강제 종료한다.</summary>
    [McpServerTool(Name = "end_session", OpenWorld = false, Destructive = true)]
    [Description("해당 세션 게임에 종료를 요청합니다. 확인 창이 남으면 closed=false입니다. force=true는 검증된 세션 게임 프로세스만 강제 종료합니다. 증거와 복사본은 보존합니다.")]
    public Task<CallToolResult> EndSession(string sessionId, bool force = false, CancellationToken cancellation = default)
        => Run("end_session", new() { SessionId = sessionId, Force = force }, cancellation);
}
