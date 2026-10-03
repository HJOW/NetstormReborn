using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// 세션 기록과 YouTube 기록이 함께 쓰는 파일 안전 규칙(용량 한도·링크 거부·JSON 설정).
/// 운영체제 의존 코드가 없어 리눅스용 YouTube 전용 빌드(<c>portable/</c>)에서도 이 파일만 링크해 쓴다.
/// </summary>
public sealed partial class SessionStore
{
    /// <summary>TODO의 파일별 50 MB 미만 제한. MB는 1,000,000바이트 기준이다.</summary>
    public const int FileLimitBytes = 50_000_000;
    /// <summary>한글을 UTF-8로 저장하며 외부 프로토콜과 맞추는 JSON 설정.</summary>
    public static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web)
    {
        Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
    };

    /// <summary>
    /// 기존 경로와 부모 경로에 junction이나 심볼릭 링크가 있으면 거부한다.
    /// 드라이브 루트는 검사하지 않는다. Wine은 리눅스 루트에 연결된 <c>Z:\</c>를 링크로 보고하지만,
    /// 루트 자체는 다른 위치로 우회되는 중간 경로가 아니기 때문이다.
    /// </summary>
    public static void RejectReparse(string path)
    {
        string? current = Path.GetFullPath(path);
        string? root = Path.GetPathRoot(current);
        // 드라이브 루트를 제외한 존재하는 모든 부모를 검사하여 작업 폴더가 저장소 밖으로 우회되지 않게 한다.
        while (current != null && !string.Equals(current, root, StringComparison.OrdinalIgnoreCase))
        {
            if ((File.Exists(current) || Directory.Exists(current)) && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidOperationException($"분석 경로에 링크를 사용할 수 없습니다: {current}");
            current = Path.GetDirectoryName(current);
        }
    }

    /// <summary>백신·탐색기·다른 분석기 프로세스가 파일을 잠시 열어 쓰기·교체가 거부될 때 다시 시도하는 최대 횟수.</summary>
    private const int TransientFileAttempts = 20;
    /// <summary>일시적 파일 접근 충돌 뒤 다시 시도하기 전의 대기 시간.</summary>
    private static readonly TimeSpan TransientFileDelay = TimeSpan.FromMilliseconds(25);

    /// <summary>
    /// 파일 쓰기·교체 작업을 일시적인 공유 충돌에 대비해 다시 시도한다.
    /// Windows는 다른 프로세스가 대상 파일을 열고 있으면 삭제 공유를 허용했더라도
    /// <c>File.Move</c> 덮어쓰기를 오류 5(접근 거부)로 거부한다. 충돌은 곧 풀리므로 잠시 기다렸다가 같은 작업을 반복한다.
    /// </summary>
    public static void RetryTransient(Action action)
    {
        // 시도 횟수 한도까지 반복하고 마지막 시도의 오류만 호출자에게 전달한다.
        for (int attempt = 1; ; attempt++)
        {
            try
            {
                action();
                return;
            }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException && attempt < TransientFileAttempts)
            {
                Thread.Sleep(TransientFileDelay);
            }
        }
    }

    /// <summary>바이너리·설정·JSON 모두 쓰기 전에 파일별 용량과 링크를 확인한다.</summary>
    public static void WriteSmallFile(string path, byte[] bytes)
    {
        if (bytes.Length >= FileLimitBytes) throw new InvalidOperationException("파일은 50 MB 미만이어야 합니다.");
        RejectReparse(path);
        RetryTransient(() => File.WriteAllBytes(path, bytes));
    }
}
