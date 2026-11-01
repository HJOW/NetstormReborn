namespace Netstorm.Client.Host;

/// <summary>
/// 편집 작업본(원본 옆의 쓰기 가능한 형제 디렉터리)만 읽고 쓰는 실제 파일 장치다. 모든 경로는 사용 직전에 링크를 해석해
/// 작업 루트 안이면서 보호된 원본 루트 밖인지 검사하며, 원본 게임 폴더에는 어떤 쓰기도 하지 않는다.
/// cpppj <c>client/EditorFiles</c>의 경로 해석·최초 복사·목록·읽기·준비·삭제 부분을 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.126에서 추가했다. cpppj의 readonly 조회·권한 변경·Template 쓰기(<c>ReadOnly</c>, <c>MakeWritable</c>,
/// <c>WriteTemplate</c>, <c>Writer</c>)와 형제 metadata 장치는 실제 Editor 저장 단위에서 옮기므로 이 클래스에는 아직 없다.
/// 장부 파일은 <see cref="EditorCatalogLedger"/>가 따로 관리한다.
/// </remarks>
public sealed class EditorFiles
{
    /// <summary>검사된 실제 작업 루트다. 파일 장치 수명 동안 바뀌지 않는다.</summary>
    public string RootPath { get; }

    /// <summary>쓰기를 거부할 원본·CD 루트의 실제 경로 목록이다.</summary>
    private readonly string[] _protected;

    /// <summary>
    /// 작업 루트와 보호 루트를 정한다. 보호 루트는 하나 이상이어야 하고 작업 루트가 그 안에 있으면 거부한다.
    /// 생성만으로 파일을 만들거나 쓰지 않는다.
    /// </summary>
    /// <param name="root">쓰기 가능한 편집 작업 루트.</param>
    /// <param name="protectedRoots">쓰기를 막을 원본·CD 루트들.</param>
    public EditorFiles(string root, IEnumerable<string> protectedRoots)
    {
        ArgumentNullException.ThrowIfNull(protectedRoots);
        RootPath = EditorWorkspacePaths.Canonical(root);
        _protected = protectedRoots.Select(EditorWorkspacePaths.Canonical).ToArray();
        if (_protected.Length == 0) throw new ArgumentException("편집 파일 장치의 원본 보호 경로 누락", nameof(protectedRoots));
        // 모든 보호 루트와 작업 루트의 포함 관계를 확인한다.
        foreach (string source in _protected)
            if (EditorWorkspacePaths.Inside(source, RootPath)) throw new ArgumentException("편집 작업 디렉터리가 보호 원본 내부입니다", nameof(root));
    }

    /// <summary>설정 경로의 구분자를 현재 플랫폼 표기로 통일한다. NUL이 있으면 거부한다.</summary>
    private static string Native(string configured)
    {
        ArgumentNullException.ThrowIfNull(configured);
        if (configured.Contains('\0')) throw new ArgumentException("편집 경로의 NUL 경계 위반", nameof(configured));
        return configured.Replace('\\', Path.DirectorySeparatorChar).Replace('/', Path.DirectorySeparatorChar);
    }

    /// <summary>현재 링크를 해석한 실제 경로가 작업 루트 안이고 모든 보호 루트 밖인지 확인해 그 경로를 돌려준다.</summary>
    private string Checked(string path)
    {
        string target = EditorWorkspacePaths.Canonical(Path.IsPathFullyQualified(path) ? path : Path.Combine(RootPath, path));
        if (!EditorWorkspacePaths.Inside(RootPath, target)) throw new InvalidOperationException("편집 파일 쓰기가 현재 작업 디렉터리를 벗어납니다");
        // 작업 루트가 원본의 상위 폴더인 경우도 각 보호 루트를 다시 검사한다.
        foreach (string source in _protected)
            if (EditorWorkspacePaths.Inside(source, target)) throw new InvalidOperationException("보호 원본 파일에 대한 편집 쓰기 요청");
        return target;
    }

    /// <summary>설정 경로를 같은 작업 namespace의 검사된 절대 경로로 해석한다. 파일을 읽거나 만들지 않는다.</summary>
    /// <param name="configured">작업 루트 기준 상대 경로 또는 작업 루트 안의 절대 경로.</param>
    public string ResolvePath(string configured) => Checked(Native(configured));

    /// <summary>검사된 작업 namespace 안에 디렉터리를 만들고 생성 뒤 같은 경로를 다시 검사한다.</summary>
    /// <param name="configured">만들 디렉터리 경로.</param>
    public void EnsureDirectory(string configured)
    {
        string path = ResolvePath(configured); Directory.CreateDirectory(path); ResolvePath(configured);
    }

    /// <summary>
    /// 원본 파일을 작업본에 한 번만 복사한다. 이미 있는 대상(파일·디렉터리)은 덮어쓰지 않고 속성도 보존한다.
    /// 원본 쪽 검사는 호출자(<see cref="EditorCatalogWorkspace"/>)가 이미 했다고 본다.
    /// </summary>
    /// <param name="source">읽기만 할 원본 파일의 실제 경로.</param>
    /// <param name="configured">작업본 안의 대상 경로.</param>
    public void CopyMissing(string source, string configured)
    {
        string path = ResolvePath(configured);
        if (File.Exists(path) || Directory.Exists(path)) return;
        EnsureDirectory(Path.GetDirectoryName(path)!); string checkedPath = ResolvePath(configured);
        try { File.Copy(source, checkedPath, false); }
        // 다른 프로세스가 먼저 만든 경우만 성공으로 보고 그 밖의 복사 실패는 알린다.
        catch (IOException) when (File.Exists(checkedPath)) { }
    }

    /// <summary>
    /// 작업 디렉터리의 한 파일 요소 와일드카드를 OS 열거 순서 그대로 나열한다. 디렉터리와 "."·".." 후보도 제거하지 않으며
    /// 와일드카드가 없으면 존재 여부와 무관하게 그 한 경로를 돌려준다. 각 실제 결과가 작업 루트 밖 링크이면 거부한다.
    /// </summary>
    /// <param name="pattern">폴더에는 와일드카드가 없는 작업본 안의 패턴.</param>
    /// <returns>작업 디렉터리의 절대 경로로 이어 붙인 결과.</returns>
    public IReadOnlyList<string> MatchDisk(string pattern)
    {
        string requested = Native(pattern); string parent = Path.GetDirectoryName(requested) ?? ""; string name = Path.GetFileName(requested);
        if (parent.IndexOfAny(['*', '?']) >= 0) throw new ArgumentException("편집 목록의 폴더 와일드카드 범위 위반", nameof(pattern));
        string directory = ResolvePath(parent.Length == 0 ? "." : parent); var result = new List<string>();
        if (name.IndexOfAny(['*', '?']) < 0) result.Add(Path.Combine(directory, name));
        else if (Directory.Exists(directory))
        {
            var options = new EnumerationOptions { MatchType = MatchType.Win32, MatchCasing = MatchCasing.CaseInsensitive, AttributesToSkip = 0, ReturnSpecialDirectories = true };
            // 원본 FindFirst/Next 순서를 유지하며 이름만 취해 작업 디렉터리 경로에 붙인다.
            foreach (string entry in Directory.EnumerateFileSystemEntries(directory, name, options).ToArray())
                result.Add(Path.Combine(directory, Path.GetFileName(entry)));
        }
        // OS 자체의 "."·".." 후보는 읽기 요청이 아니므로 부모 검사만 통과하면 보존하고 나머지는 링크 경계를 다시 확인한다.
        foreach (string listed in result)
        {
            string leaf = Path.GetFileName(listed);
            if (leaf is not "." and not "..") ResolvePath(listed);
        }
        return result;
    }

    /// <summary>
    /// 검사된 작업본 파일 전체를 읽는다. 파일이 없거나 디렉터리이거나 열 수 없으면 null이며 경로 거부는 예외로 알린다.
    /// 파일 이름의 와일드카드는 열기에 쓰지 않으므로 항상 null이다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    public byte[]? TryRead(string configured)
    {
        string native = Native(configured); string name = Path.GetFileName(native);
        string path;
        if (name.IndexOfAny(['*', '?']) < 0) path = ResolvePath(configured);
        else path = Path.Combine(ResolvePath(Path.GetDirectoryName(native) is { Length: > 0 } parent ? parent : "."), name);
        if (!File.Exists(path)) return null;
        // 원본 fopen 실패처럼 열 수 없는 파일은 부재와 같은 빈 결과다.
        try { return File.ReadAllBytes(path); }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException) { return null; }
    }

    /// <summary>검사된 경로가 파일이든 디렉터리든 존재하는지 알려 준다. 원본 GetFileAttributes처럼 디렉터리도 존재로 본다.</summary>
    /// <param name="configured">작업본 안의 경로.</param>
    public bool PathExists(string configured)
    {
        string path = ResolvePath(configured); return File.Exists(path) || Directory.Exists(path);
    }

    /// <summary>
    /// 같은 경로의 기존 작업본은 보존하고 없을 때만 원문 바이트로 만든다. 일반 파일이 아닌 기존 대상은 거부한다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <param name="original">없을 때 쓸 원문.</param>
    public string PreparePath(string configured, ReadOnlySpan<byte> original)
    {
        string target = ResolvePath(configured);
        if (File.Exists(target)) return target;
        if (Directory.Exists(target)) throw new InvalidOperationException("편집 작업본이 일반 파일이 아닙니다");
        Directory.CreateDirectory(Path.GetDirectoryName(target)!); Write(target, original);
        return target;
    }

    /// <summary>
    /// Fort 이름으로 <c>d/이름.fort</c> 작업본을 준비한다. 원본 pending 이름의 점·경로 문자 계약을 어기면 거부한다.
    /// </summary>
    /// <param name="name">점과 경로 문자가 없는 Fort 이름.</param>
    /// <param name="original">작업본이 없을 때 쓸 원문.</param>
    public string Prepare(string name, ReadOnlySpan<byte> original)
    {
        if (name.Length == 0 || name.IndexOfAny("./\\:*?\"<>|\0".ToCharArray()) >= 0) throw new ArgumentException("편집 Fort 이름의 원본 점/경로 문자 계약 위반", nameof(name));
        return PreparePath("d/" + name + ".fort", original);
    }

    /// <summary>검사된 작업본 파일을 잘라 쓴다. 부모 디렉터리는 만들지 않으며 실패는 예외로 알린다.</summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <param name="bytes">쓸 내용.</param>
    public void Write(string configured, ReadOnlySpan<byte> bytes)
    {
        using var stream = new FileStream(ResolvePath(configured), FileMode.Create, FileAccess.ReadWrite, FileShare.None);
        stream.Write(bytes); stream.Flush();
    }

    /// <summary>
    /// 삭제 직전에 같은 경로 검사를 다시 하고 작업본 파일을 지운다. 원본 DeleteFileA처럼 실패 결과를 사용하지 않으며
    /// 디렉터리와 재귀 삭제는 하지 않는다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    public void Delete(string configured)
    {
        string path = ResolvePath(configured);
        try { File.Delete(path); }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException) { }
    }
}
