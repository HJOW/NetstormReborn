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

    /// <summary>바이너리·설정·JSON 모두 쓰기 전에 파일별 용량과 링크를 확인한다.</summary>
    public static void WriteSmallFile(string path, byte[] bytes)
    {
        if (bytes.Length >= FileLimitBytes) throw new InvalidOperationException("파일은 50 MB 미만이어야 합니다.");
        RejectReparse(path);
        File.WriteAllBytes(path, bytes);
    }
}
