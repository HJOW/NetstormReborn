using System.Security.Cryptography;

namespace Netstorm.AnalyzeManager; // 원본 작업 복사본의 재실행은 기록·소유권 계층에서 검증한다.

/// <summary>이미 종료한 세션의 작업 맵과 설정을 보존하는 재실행 검증이다.</summary>
public sealed partial class SessionStore
{
    /// <summary>종료 상태와 코드 바이트를 확인하며 저장 파일·설정·세션 정보는 쓰지 않는다.</summary>
    public void ValidateRestartCopy(AnalysisSession session)
    {
        if (session.Phase != "ended") throw new InvalidOperationException("종료한 세션만 같은 작업 복사본으로 다시 실행할 수 있습니다.");
        string path = GamePath(session);
        RejectReparse(path);
        var file = new FileInfo(path);
        if (!file.Exists || file.Length >= FileLimitBytes) throw new InvalidDataException("재실행할 작업 복사본 실행 파일이 없거나 크기 제한을 넘었습니다.");
        string actual = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path))).ToLowerInvariant();
        if (!string.Equals(actual, session.ExeSha256, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("작업 복사본 실행 코드가 최초 장부와 다릅니다. 다시 실행하지 않았습니다.");
    }
}
