using Netstorm.Assets;

namespace Netstorm.Client.Host;

/// <summary>cpppj 편집 작업본의 형제 루트와 원본 설정 경로의 전체 상대 매핑을 계산한다.</summary>
public sealed class EditorWorkspacePaths
{
    /// <summary>게임 자산의 실제 루트다.</summary>
    public string SourceRoot { get; }
    /// <summary>원본 옆의 쓰기 가능한 편집 작업본 루트다.</summary>
    public string WorkRoot { get; }
    /// <summary>실제 자산 경로에서 형제 작업 디렉터리를 선택하며 파일을 만들지 않는다.</summary>
    public EditorWorkspacePaths(string sourceRoot)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(sourceRoot);
        SourceRoot = Canonical(sourceRoot); var source = new DirectoryInfo(SourceRoot);
        if (source.Parent == null || source.Name.Length == 0) throw new ArgumentException("편집 자산 루트의 형제 경로 없음", nameof(sourceRoot));
        string expected = Path.GetFullPath(Path.Combine(source.Parent.FullName, source.Name + "-editor"));
        WorkRoot = Canonical(expected);
        if (!string.Equals(expected, WorkRoot, StringComparison.OrdinalIgnoreCase)) throw new ArgumentException("편집 작업본의 외부 링크 경계");
        if (Inside(SourceRoot, WorkRoot) || Inside(WorkRoot, SourceRoot)) throw new ArgumentException("편집 원본과 작업 경로 중첩");
    }
    /// <summary>기존 경로 요소의 링크를 해석하고 없는 나머지 요소는 정규화해 반환한다.</summary>
    internal static string Canonical(string input)
    {
        string absolute = Path.GetFullPath(input); string root = Path.GetPathRoot(absolute) ?? throw new ArgumentException("절대 경로 루트 없음");
        string current = root;
        // 기존 경로의 각 링크를 실제 대상으로 해석한 뒤 존재하지 않는 끝도 이어 붙인다.
        foreach (string part in absolute[root.Length..].Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar))
        {
            if (part.Length == 0) continue; current = Path.Combine(current, part);
            FileSystemInfo item = Directory.Exists(current) ? new DirectoryInfo(current) : new FileInfo(current);
            if (item.Exists && (item.Attributes & FileAttributes.ReparsePoint) != 0)
                current = item.ResolveLinkTarget(true)?.FullName ?? throw new IOException("편집 경로 링크 해석 실패");
        }
        return Path.TrimEndingDirectorySeparator(Path.GetFullPath(current));
    }
    /// <summary>Windows 원본의 대소문자 무시 비교와 경로 요소 경계를 함께 검사한다.</summary>
    internal static bool Inside(string root, string path) => path.Equals(root, StringComparison.OrdinalIgnoreCase) ||
        path.StartsWith(root + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    /// <summary>상대 또는 원본·작업본 절대 FileSpec을 검증된 같은 작업본 전체 경로에 대응시킨다.</summary>
    public string Resolve(string configured)
    {
        ArgumentNullException.ThrowIfNull(configured); if (configured.Contains('\0')) throw new ArgumentException("편집 경로의 NUL");
        string path = configured.Replace('\\', Path.DirectorySeparatorChar).Replace('/', Path.DirectorySeparatorChar);
        string relative;
        if (Path.IsPathFullyQualified(path))
        {
            string absolute = Path.GetFullPath(path); string canonical = Canonical(absolute);
            if (Inside(WorkRoot, canonical)) relative = Path.GetRelativePath(WorkRoot, canonical);
            else if (Inside(SourceRoot, canonical)) relative = Path.GetRelativePath(SourceRoot, canonical);
            else throw new ArgumentException("편집 경로가 원본 또는 작업본 루트 밖이다", nameof(configured));
        }
        else
        {
            while (path.StartsWith("." + Path.DirectorySeparatorChar, StringComparison.Ordinal)) path = path[2..];
            relative = path.TrimStart(Path.DirectorySeparatorChar);
        }
        string result = Canonical(Path.Combine(WorkRoot, relative));
        if (!Inside(WorkRoot, result)) throw new ArgumentException("편집 경로가 작업본 밖으로 벗어남", nameof(configured));
        return result;
    }
    /// <summary>설정 경로에 해당하는 현재 원본 디스크 파일 또는 디렉터리를 찾는다.</summary>
    public bool SourceExists(string configured)
    {
        string work = Resolve(configured); string relative = Path.GetRelativePath(WorkRoot, work).Replace('\\', '/');
        string? file = GameDataLocator.FindFile(SourceRoot, relative);
        string? directory = GameDataLocator.FindDirectory(SourceRoot, relative);
        return file != null || directory != null;
    }
}
