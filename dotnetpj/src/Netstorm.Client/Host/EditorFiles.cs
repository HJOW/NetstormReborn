using System.Runtime.InteropServices;

namespace Netstorm.Client.Host;

/// <summary>
/// 편집 작업본(원본 옆의 쓰기 가능한 형제 디렉터리)만 읽고 쓰는 실제 파일 장치다. 모든 경로는 사용 직전에 링크를 해석해
/// 작업 루트 안이면서 보호된 원본 루트 밖인지 검사하며, 원본 게임 폴더에는 어떤 쓰기도 하지 않는다.
/// cpppj <c>client/EditorFiles</c>의 경로 해석·최초 복사·목록·읽기·준비·존재·readonly·권한·Template 쓰기·삭제·형제 metadata·원자 쓰기를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.126에서 추가했고 같은 날 0.127에서 <see cref="Exists"/>, <see cref="ReadOnly"/>, <see cref="MakeWritable"/>,
/// <see cref="Writer"/>, <see cref="SiblingNamespace"/>, <see cref="WriteAtomic"/>를 더했다. cpppj의 <c>RelativePath</c>는 옮기지 않았다.
/// 상대 키 계산은 <see cref="EditorWorkspacePaths.Resolve"/>와 <see cref="EditorCatalogWorkspace"/>가 맡는다.
/// 장부 파일은 <see cref="EditorCatalogLedger"/>가 따로 관리하며 갱신만 이 클래스의 <see cref="WriteAtomic"/>을 거친다.
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

    /// <summary>원본 CRT 파일 열기 실패로 보는 예외다. 경로 거부(<see cref="InvalidOperationException"/>)와 구별한다.</summary>
    private static bool OpenFailed(Exception exception) => exception is IOException or UnauthorizedAccessException;

    /// <summary>
    /// 원문 0041ab10의 직접 파일 분기다. <c>fopen("rb")</c>가 열리면 출처 5, 열리지 않으면 0이다.
    /// 서명 파싱이나 manager 상태는 조회하지 않으며 디렉터리는 CRT처럼 열리지 않아 0이다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <remarks>2026-11-01 0.127에서 추가했다. CRT 기본 공유(읽기·쓰기 허용)를 따른다.</remarks>
    public int Exists(string configured)
    {
        string path = ResolvePath(configured);
        try { using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite); return 5; }
        catch (Exception exception) when (OpenFailed(exception)) { return 0; }
    }

    /// <summary>
    /// 작업 파일의 쓰기 금지 여부다. CRT <c>stat</c>의 쓰기 비트(Windows에서는 readonly 속성)만 읽으며 Config의 <c>readOnlyForts</c>와 독립이다.
    /// 빈 이름(진단 0x480)과 stat 실패(진단 0x48e)는 원문처럼 진단을 낸 뒤 명시 오류로 멈추고, 진단 장치가 없으면 그 자체가 연결 누락 오류다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <param name="assertion">원문 진단 번호를 받는 장치. 진단 지점에 이르면 필수다.</param>
    /// <remarks>2026-11-01 0.127에서 추가했다. ACL 쓰기 성공 검사는 하지 않는다.</remarks>
    public bool ReadOnly(string configured, Action<uint>? assertion = null)
    {
        if (configured.Length == 0)
        {
            if (assertion is null) throw new InvalidOperationException("편집 readonly의 원문 빈 이름 진단 미연결");
            assertion(0x480); throw new ArgumentException("편집 readonly의 빈 실제 경로", nameof(configured));
        }
        string path = ResolvePath(configured);
        // stat은 파일과 디렉터리 모두 조회하므로 둘 다 없을 때만 실패다.
        if (!File.Exists(path) && !Directory.Exists(path))
        {
            if (assertion is null) throw new InvalidOperationException("편집 readonly의 원문 stat 실패 진단 미연결");
            assertion(0x48e); throw new FileNotFoundException("편집 readonly의 stat 실패 뒤 미정 mode 접근 미복원");
        }
        return (File.GetAttributes(path) & FileAttributes.ReadOnly) != 0;
    }

    /// <summary>
    /// 원문 chmod(0x180) 요청을 수행한다. 쓰기 경로를 확인한 뒤 readonly 속성만 푼다.
    /// chmod 실패는 원본처럼 쓰기를 중단시키지 않으며 실제 쓰기 실패는 <see cref="WriteTemplate"/>과 <see cref="Write"/>가 알린다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <param name="mode">원본 CRT 권한 모드. 0x180(읽기+쓰기)만 지원한다.</param>
    /// <remarks>2026-11-01 0.127에서 추가했다.</remarks>
    public void MakeWritable(string configured, uint mode)
    {
        if (mode != 0x180u) throw new ArgumentException("편집 파일 장치의 미지원 원본 권한 모드", nameof(mode));
        string path = Checked(Native(configured));
        try
        {
            FileAttributes attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReadOnly) == 0) return;
            // 다른 속성은 보존하고 readonly만 끈다. 속성이 비면 Normal로 써야 한다.
            attributes &= ~FileAttributes.ReadOnly;
            File.SetAttributes(path, attributes == 0 ? FileAttributes.Normal : attributes);
        }
        catch (Exception exception) when (OpenFailed(exception)) { }
    }

    /// <summary>
    /// 원문 0041bb90의 Template 저장 쓰기다. 기존 파일은 자르지 않는 <c>r+b</c>로 먼저 열고, 열기에 실패할 때만 새로 만드는 <c>w+b</c>를 쓴다.
    /// 이미지 전체를 현재 0 위치부터 한 번에 쓰므로 기존 파일이 더 길면 뒤쪽 바이트(tail)가 남는다. 부모 폴더는 만들지 않으며 두 열기가 모두 실패하면 오류다.
    /// </summary>
    /// <param name="configured">작업본 안의 파일 경로.</param>
    /// <param name="bytes">쓸 Template 이미지 원문. 빈 이미지는 파일을 만들 뿐 자르지 않는다.</param>
    /// <remarks>
    /// 2026-11-01 0.127에서 추가했다. cpppj 단위669가 일반 <see cref="Write"/>의 잘라쓰기로 짧아진 Fort의 파일 끝을 줄이던 결함을 바로잡은 동작이다.
    /// 원문 fwrite/fclose 반환값은 오류로 쓰지 않는다.
    /// </remarks>
    private void WriteTemplate(string configured, ReadOnlySpan<byte> bytes)
    {
        string path = Checked(Native(configured)); FileStream stream;
        try { stream = new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite); }
        catch (Exception exception) when (OpenFailed(exception))
        {
            try { stream = new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite); }
            catch (Exception second) when (OpenFailed(second)) { throw new IOException("편집 Template 작업본 파일 열기 실패", second); }
        }
        using (stream) stream.Write(bytes);
    }

    /// <summary>이 객체보다 짧게 사는 실제 Template 저장 장치다. 권한은 <see cref="MakeWritable"/>, 쓰기는 tail을 보존하는 <see cref="WriteTemplate"/>로 인계한다.</summary>
    /// <remarks>2026-11-01 0.127에서 추가했다.</remarks>
    public TemplateFileWriteHooks Writer() => new(MakeWritable, WriteTemplate);

    /// <summary>형제 metadata 이름에 허용하지 않는 문자다. 점·경로 구분자·와일드카드·NUL을 막는다.</summary>
    private static readonly char[] SiblingForbidden = "./\\:*?\"<>|\0".ToCharArray();

    /// <summary>
    /// 작업 루트의 형제 디렉터리(<c>작업루트이름</c> + <paramref name="suffix"/>)를 쓰는 metadata 장치를 만든다.
    /// 새 장치는 현재 보호 루트와 이 작업 루트를 모두 보호하며, 임의 fortSpec 목록에 끼지 않도록 링크로 다른 위치에 매핑된 형제는 거부한다. 파일은 쓰지 않는다.
    /// </summary>
    /// <param name="suffix">작업 루트 이름 뒤에 붙일 접미(예: <c>-state</c>). 점·경로 문자는 허용하지 않는다.</param>
    /// <remarks>2026-11-01 0.127에서 추가했다.</remarks>
    public EditorFiles SiblingNamespace(string suffix)
    {
        if (suffix.Length == 0 || suffix.IndexOfAny(SiblingForbidden) >= 0) throw new ArgumentException("편집 metadata 형제 이름 범위 위반", nameof(suffix));
        string parent = Path.GetDirectoryName(RootPath) ?? throw new InvalidOperationException("편집 metadata 형제를 둘 상위 폴더 없음");
        string expected = Path.Combine(parent, Path.GetFileName(RootPath) + suffix);
        if (!string.Equals(EditorWorkspacePaths.Canonical(expected), Path.GetFullPath(expected), StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("편집 metadata 형제의 외부 링크 경계");
        return new EditorFiles(expected, _protected.Append(RootPath));
    }

    /// <summary>외부 인덱서 등의 짧은 공유 거부만 기다리는 원자 교체의 최대 시도 횟수다(원문 51회, 약 1초).</summary>
    private const int ReplaceAttempts = 51;

    /// <summary>실패한 OS 교체 사이에 기다리는 시간(밀리초)이다. 게임 상태나 진행값을 바꾸지 않는 실제 장치 대기다.</summary>
    private const int ReplaceWaitMilliseconds = 20;

    /// <summary>
    /// metadata용 임시 파일(<c>이름.pending</c>)을 쓰고 flush한 뒤 같은 검사된 namespace에서 대상으로 교체한다.
    /// Windows에서는 write-through 교체를 쓰고 접근 거부·공유/잠금 위반·매핑 거부만 최대 약 1초 다시 시도하며, 그래도 실패하면 이전 파일과 임시 파일을 모두 남긴 채 오류다.
    /// </summary>
    /// <param name="configured">작업 namespace 안의 대상 파일.</param>
    /// <param name="bytes">쓸 내용.</param>
    /// <remarks>
    /// 2026-11-01 0.127에서 추가했다. 0.126의 장부 갱신은 이 재시도 없이 한 번 <see cref="File.Move(string, string, bool)"/>만 했다.
    /// Windows 외에서는 같은 재시도를 하지 않는 준비용 이동이며 durable 검증으로 주장하지 않는다.
    /// </remarks>
    public void WriteAtomic(string configured, ReadOnlySpan<byte> bytes)
    {
        string target = ResolvePath(configured); EnsureDirectory(Path.GetDirectoryName(target)!);
        string pending = ResolvePath(target + ".pending");
        // 공유 없이 열어 쓴 뒤 디스크까지 flush하고 닫는다. 닫기 실패도 예외로 알린다.
        using (var stream = new FileStream(pending, FileMode.Create, FileAccess.Write, FileShare.None)) { stream.Write(bytes); stream.Flush(true); }
        // 매 교체 직전에 같은 원본·외부 링크 보호 경로를 다시 검사하며, 일시적 거부일 때만 같은 교체를 다시 요청한다.
        for (int attempt = 0; ; attempt++)
        {
            string from = ResolvePath(pending), to = ResolvePath(target); int error;
            if (OperatingSystem.IsWindows()) error = WindowsFiles.Replace(from, to);
            else { File.Move(from, to, true); return; }
            if (error == 0) return;
            if (!WindowsFiles.Transient(error) || attempt + 1 == ReplaceAttempts) throw new IOException("편집 장부 durable 교체 실패:" + error);
            Thread.Sleep(ReplaceWaitMilliseconds);
        }
    }

    /// <summary>
    /// Windows 파일 교체의 write-through 이동과 일시 오류 판정만 맡는 OS 경계다. 게임 규칙은 없으며 <see cref="WriteAtomic"/>만 쓴다.
    /// </summary>
    /// <remarks>2026-11-01 0.127에서 추가했다. .NET의 <c>File.Move</c>는 MOVEFILE_WRITE_THROUGH를 줄 수 없어 직접 호출한다.</remarks>
    private static class WindowsFiles
    {
        /// <summary>대상이 있으면 덮어쓰는 이동 플래그(MOVEFILE_REPLACE_EXISTING)다.</summary>
        private const uint ReplaceExisting = 0x1;
        /// <summary>디스크에 반영될 때까지 반환하지 않는 플래그(MOVEFILE_WRITE_THROUGH)다.</summary>
        private const uint WriteThrough = 0x8;

        /// <summary>Win32 <c>MoveFileExW</c>다.</summary>
        [DllImport("kernel32.dll", EntryPoint = "MoveFileExW", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool MoveFileEx(string existing, string target, uint flags);

        /// <summary>임시 파일을 대상으로 write-through 교체한다. 성공이면 0, 실패하면 직후의 Win32 오류 번호다.</summary>
        public static int Replace(string pending, string target) =>
            MoveFileEx(pending, target, ReplaceExisting | WriteThrough) ? 0 : Marshal.GetLastPInvokeError();

        /// <summary>접근 거부(5)·공유 위반(32)·잠금 위반(33)·사용자 매핑 파일(1224)만 다시 시도할 일시 오류로 본다.</summary>
        public static bool Transient(int error) => error is 5 or 32 or 33 or 1224;
    }
}
