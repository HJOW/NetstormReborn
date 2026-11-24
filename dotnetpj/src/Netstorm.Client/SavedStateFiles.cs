using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client; // 설치 폴더와 사용자 홈의 같은 저장 상태 경로를 같은 고리로 계산한다.

// 게임 설치 폴더를 프로세스 CWD 변경 없이 사용할 실제 장치다. 0.985에서 추가됐다.
public static class SavedStateFiles
{
    /// <summary>004b5470의 빈 root와 d와 !state.dat를 같은 경로 고리 작성 뒤 실제 파일 위치로 바꾼다. 0.985에서 추가됐다.</summary>
    /// <param name="gameDirectory">명시 설치 폴더다.</param>
    /// <param name="formatter">같은 Client와 VFS가 공유하는 원문 경로 고리다.</param>
    /// <param name="userFile">비어 있지 않으면 설치 폴더 대신 사용자 홈 Saves의 !state.dat를 쓴다.</param>
    /// <returns>절대 파일 경로다. 파일을 만들지 않는다.</returns>
    public static string SavedStateFilePath(string gameDirectory, RawFileName formatter, string userFile = "")
    {
        ArgumentNullException.ThrowIfNull(gameDirectory);
        ArgumentNullException.ThrowIfNull(formatter);
        ReadOnlyMemory<byte> path = formatter.Format([
            new(-2, Encoding.Latin1.GetBytes("")),
            new(-2, "d"u8.ToArray()),
            new(-3, "!state.dat"u8.ToArray())]);
        string text = Encoding.Latin1.GetString(path.Span);
        if (!text.StartsWith(".\\", StringComparison.Ordinal)) throw new InvalidOperationException("원문 저장 상태 이름의 상대 접두 불일치");
        if (!string.IsNullOrEmpty(userFile)) return Path.GetFullPath(userFile);
        return Path.GetFullPath(Path.Combine(gameDirectory, text[2..]));
    }

    /// <summary>원문 unlink와 오류 매핑을 수행하고 0과 -1을 돌려준다. Native State3는 이 반환을 원문처럼 무시한다. 0.985에서 추가됐다.</summary>
    /// <param name="gameDirectory">명시 설치 폴더다.</param>
    /// <param name="formatter">같은 Client와 VFS가 공유하는 원문 경로 고리다.</param>
    /// <param name="userFile">비어 있지 않으면 사용자 홈 Saves의 !state.dat를 지운다.</param>
    /// <returns>성공하면 0이며 없거나 실패하면 -1이다.</returns>
    public static int DeleteSavedStateFile(string gameDirectory, RawFileName formatter, string userFile = "")
    {
        string path = SavedStateFilePath(gameDirectory, formatter, userFile);
        try
        {
            File.Delete(path);
            return 0;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or NotSupportedException)
        {
            return -1;
        }
    }
}
