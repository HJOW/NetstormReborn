using System.Diagnostics;

namespace Netstorm.Client.Tests;

/// <summary>
/// 편집 작업본 검사 전용 임시 루트다. 검사마다 새 디렉터리를 만들고 끝나면 읽기 전용 속성을 풀어 지우므로
/// 실제 자산 폴더 옆에 <c>-editor</c> 작업본이 생기지 않는다. 지우기 전 임시 상위 경계를 확인한다.
/// </summary>
internal sealed class EditorTempRoot : IDisposable
{
    /// <summary>모든 검사 루트를 모으는 임시 상위 폴더다.</summary>
    private static readonly string Parent = Path.Combine(Path.GetTempPath(), "NetstormEditorWorkspaceTests");

    /// <summary>이 검사만의 새 루트다.</summary>
    public string Root { get; } = Path.Combine(Parent, Guid.NewGuid().ToString("N"));

    /// <summary>원본 역할을 하는 자산 폴더 경로이며 만들지는 않는다.</summary>
    public string Source => Path.Combine(Root, "source");

    /// <summary>쓰기 가능한 작업본 경로이며 만들지는 않는다.</summary>
    public string Work => Path.Combine(Root, "work");

    /// <summary>루트 디렉터리를 만든다.</summary>
    public EditorTempRoot() => Directory.CreateDirectory(Root);

    /// <summary>원본 쪽에 검사용 파일을 만든다. 부모 폴더도 만든다.</summary>
    public string Seed(string relative, byte[] bytes)
    {
        string path = Path.Combine(Source, relative.Replace('/', Path.DirectorySeparatorChar));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!); File.WriteAllBytes(path, bytes); return path;
    }

    /// <summary>NTFS 디렉터리 접합(권한 상승이 필요 없음)으로 링크를 만든다.</summary>
    public static void Junction(string link, string target)
    {
        var start = new ProcessStartInfo("cmd.exe", $"/c mklink /J \"{link}\" \"{target}\"") { RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true };
        using var process = Process.Start(start)!; process.WaitForExit();
        if (process.ExitCode != 0) throw new InvalidOperationException("검사용 디렉터리 접합 생성 실패");
    }

    /// <summary>접합은 대상을 지우지 않고 링크만 제거한 뒤 임시 루트 전체를 삭제한다.</summary>
    public void Dispose()
    {
        string target = Path.GetFullPath(Root); string boundary = Path.GetFullPath(Parent) + Path.DirectorySeparatorChar;
        if (!target.StartsWith(boundary, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("검사 디렉터리 경계 오류");
        if (!Directory.Exists(target)) return;
        // 접합은 먼저 링크로만 지운다.
        foreach (string directory in Directory.EnumerateDirectories(target, "*", SearchOption.AllDirectories).ToArray())
            if (Directory.Exists(directory) && (new DirectoryInfo(directory).Attributes & FileAttributes.ReparsePoint) != 0) Directory.Delete(directory);
        // 읽기 전용으로 복사된 파일도 지울 수 있게 속성을 푼다.
        foreach (string file in Directory.EnumerateFiles(target, "*", SearchOption.AllDirectories)) File.SetAttributes(file, FileAttributes.Normal);
        Directory.Delete(target, true);
    }
}
