using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>파일을 찾은 위치</summary>
public enum GameFileSource
{
    /// <summary>찾지 못함</summary>
    None,

    /// <summary>데이터 폴더의 느슨한 파일</summary>
    Disk,

    /// <summary>TAFF 아카이브(*.tarc) 안의 파일</summary>
    Archive,
}

/// <summary>
/// 데이터 폴더의 느슨한 파일과 TAFF 아카이브를 하나로 합쳐 조회하는 가상 파일 시스템.
/// 원본 Basefile.cpp(FUN_0041b4c0)와 같은 우선순위를 따른다:
/// (1) 데이터 폴더의 디스크 파일 → (2) 데이터 폴더의 *.tarc 들(이름순) → (3) 보조 폴더의 디스크 파일.
/// 경로는 대소문자와 구분자(\ /)를 무시하며, 원본 경로 표기("\D\x.fort", ".\d\x.fort")를 그대로 받는다.
/// 분석 근거: docs/formats/vfs.md
/// </summary>
public sealed class GameFileSystem
{
    /// <summary>아카이브 파일 확장자 패턴 (원본은 데이터 폴더에서 "*.tarc" 를 찾아 모두 등록한다)</summary>
    private const string ArchivePattern = "*.tarc";

    /// <summary>원본 데이터 폴더 (netstorm.tarc 가 있는 곳)</summary>
    public string BaseDirectory { get; }

    /// <summary>보조 폴더 (원본의 CD 경로에 해당, 없으면 null)</summary>
    public string? SecondaryDirectory { get; }

    /// <summary>조회 순서대로 나열한 아카이브</summary>
    public IReadOnlyList<TaffArchive> Archives { get; }

    /// <summary>기본 폴더에 속한 아카이브 수. 나머지는 보조 폴더에 속한다.</summary>
    private readonly int _primaryArchiveCount;

    /// <summary>폴더와 아카이브 목록으로 만든다</summary>
    /// <param name="baseDirectory">데이터 폴더</param>
    /// <param name="archives">조회 순서대로 나열한 아카이브</param>
    /// <param name="secondaryDirectory">보조 폴더 (선택)</param>
    /// <param name="primaryArchiveCount">기본 폴더의 아카이브 수. 생략하면 전달한 모든 아카이브를 기본 폴더 것으로 취급한다.</param>
    public GameFileSystem(string baseDirectory, IReadOnlyList<TaffArchive> archives, string? secondaryDirectory = null, int? primaryArchiveCount = null)
    {
        BaseDirectory = baseDirectory;
        Archives = archives;
        SecondaryDirectory = secondaryDirectory;
        _primaryArchiveCount = primaryArchiveCount ?? archives.Count;
        if (_primaryArchiveCount < 0 || _primaryArchiveCount > archives.Count) throw new ArgumentOutOfRangeException(nameof(primaryArchiveCount));
    }

    /// <summary>
    /// 데이터 폴더를 열고 그 안의 *.tarc 를 모두 등록한다 (이름순, 원본의 FindFirstFile 순서와 같다고 가정).
    /// </summary>
    /// <param name="baseDirectory">데이터 폴더</param>
    /// <param name="secondaryDirectory">보조 폴더 (선택). 그 안의 *.tarc 도 뒤에 등록된다</param>
    public static GameFileSystem Open(string baseDirectory, string? secondaryDirectory = null)
    {
        var archives = new List<TaffArchive>();
        // 데이터 폴더 → 보조 폴더 순서로 아카이브를 등록한다
        int primaryCount = 0;
        string?[] directories = string.Equals(baseDirectory, secondaryDirectory, StringComparison.Ordinal)
            ? [baseDirectory] : [baseDirectory, secondaryDirectory];
        foreach (string? dir in directories)
        {
            if (dir == null || !Directory.Exists(dir))
            {
                continue;
            }
            IEnumerable<string> files = Directory.EnumerateFiles(dir, ArchivePattern)
                .Order(StringComparer.OrdinalIgnoreCase);
            // 이름순으로 정렬한 아카이브를 차례로 연다
            foreach (string file in files)
            {
                archives.Add(TaffArchive.Open(file));
            }
            if (dir == baseDirectory) primaryCount = archives.Count;
        }
        return new GameFileSystem(baseDirectory, archives, secondaryDirectory, primaryCount);
    }

    /// <summary>
    /// 경로를 비교용 형식("d/altar.type")으로 정규화한다. 앞의 ".\", "\", "/" 는 떼어 낸다.
    /// </summary>
    /// <param name="path">원본 또는 사용자 경로</param>
    public static string NormalizePath(string path)
    {
        string p = path.Replace('\\', '/');
        // 앞의 "./" 와 "/" 를 모두 떼어 낸다
        while (p.StartsWith("./", StringComparison.Ordinal) || p.StartsWith('/'))
        {
            p = p.StartsWith('/') ? p[1..] : p[2..];
        }
        return p.ToLowerInvariant();
    }

    /// <summary>파일이 어디에 있는지 찾는다 (읽기와 같은 우선순위)</summary>
    /// <param name="path">찾을 경로</param>
    public GameFileSource Locate(string path) => Resolve(path, out _, out _);

    /// <summary>파일이 있는지 검사한다</summary>
    /// <param name="path">찾을 경로</param>
    public bool Exists(string path) => Locate(path) != GameFileSource.None;

    /// <summary>파일 내용을 읽는다. 없으면 null</summary>
    /// <param name="path">읽을 경로</param>
    public byte[]? TryReadAllBytes(string path) =>
        Resolve(path, out string? diskPath, out (TaffArchive Archive, TaffEntry Entry) hit) switch
        {
            GameFileSource.Disk => File.ReadAllBytes(diskPath!),
            GameFileSource.Archive => hit.Archive.Read(hit.Entry),
            _ => null,
        };

    /// <summary>파일 내용을 읽는다. 없으면 FileNotFoundException</summary>
    /// <param name="path">읽을 경로</param>
    public byte[] ReadAllBytes(string path) =>
        TryReadAllBytes(path) ?? throw new FileNotFoundException($"게임 파일을 찾지 못했습니다: {path}", path);

    /// <summary>텍스트 파일을 읽는다 (Windows-1252 또는 UTF-8 자동 판별). 없으면 null</summary>
    /// <param name="path">읽을 경로</param>
    public string? TryReadAllText(string path)
    {
        byte[]? data = TryReadAllBytes(path);
        return data == null ? null : OriginalText.Decode(data);
    }

    /// <summary>
    /// 파일 이름에 와일드카드(*, ?)를 쓴 패턴과 일치하는 파일을 디스크와 아카이브에서 모두 찾는다.
    /// 예: "d/offical*.english". 결과는 정규화 경로이며 중복 없이 이름순으로 정렬한다.
    /// </summary>
    /// <param name="pattern">폴더 부분은 와일드카드 없이, 파일 이름 부분에만 와일드카드를 쓴다</param>
    public IReadOnlyList<string> Find(string pattern)
    {
        string normalized = NormalizePath(pattern);
        int slash = normalized.LastIndexOf('/');
        string folder = slash >= 0 ? normalized[..slash] : "";
        var nameRegex = new Regex(
            "^" + Regex.Escape(normalized[(slash + 1)..]).Replace(@"\*", ".*").Replace(@"\?", ".") + "$",
            RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        var result = new SortedSet<string>(StringComparer.Ordinal);

        // 데이터 폴더와 보조 폴더의 해당 하위 폴더에서 찾는다
        foreach (string? dir in new[] { BaseDirectory, SecondaryDirectory })
        {
            if (dir == null)
            {
                continue;
            }
            string? folderPath = folder.Length == 0 ? dir : GameDataLocator.FindDirectory(dir, folder);
            if (folderPath == null)
            {
                continue;
            }
            // 폴더 안 파일 이름을 패턴과 비교한다
            foreach (string file in Directory.EnumerateFiles(folderPath))
            {
                string name = Path.GetFileName(file);
                if (nameRegex.IsMatch(name))
                {
                    result.Add(folder.Length == 0 ? name.ToLowerInvariant() : $"{folder}/{name.ToLowerInvariant()}");
                }
            }
        }
        // 아카이브 엔트리 중 같은 폴더에 있고 이름이 맞는 것을 찾는다
        foreach (TaffArchive archive in Archives)
        {
            foreach (TaffEntry entry in archive.Entries)
            {
                string entryPath = NormalizePath(entry.Name);
                int entrySlash = entryPath.LastIndexOf('/');
                string entryFolder = entrySlash >= 0 ? entryPath[..entrySlash] : "";
                if (entryFolder == folder && nameRegex.IsMatch(entryPath[(entrySlash + 1)..]))
                {
                    result.Add(entryPath);
                }
            }
        }
        return [.. result];
    }

    /// <summary>원본 FileSpec처럼 기본 디스크만 OS 열거 순서로 찾고 디렉터리도 포함한다.</summary>
    public IReadOnlyList<string> MatchDisk(string pattern)
    {
        string raw = pattern.Split('\0')[0].Replace('\\', '/');
        if (raw.StartsWith("./", StringComparison.Ordinal)) raw = raw[2..]; raw = raw.TrimStart('/');
        if (raw.IndexOfAny(['*', '?']) < 0) return [raw];
        int slash = raw.LastIndexOf('/'); string folder = slash >= 0 ? raw[..slash] : "", mask = raw[(slash + 1)..];
        string? directory = folder.Length == 0 ? BaseDirectory : GameDataLocator.FindDirectory(BaseDirectory, folder);
        if (directory == null) return [];
        var options = new EnumerationOptions { MatchType = MatchType.Win32, MatchCasing = MatchCasing.CaseInsensitive, AttributesToSkip = 0, ReturnSpecialDirectories = true };
        return Directory.EnumerateFileSystemEntries(directory, mask, options).Select(path => folder.Length == 0 ? Path.GetFileName(path) : folder + "/" + Path.GetFileName(path)).ToArray();
    }

    /// <summary>원본 FileFinder처럼 기본 디스크·기본 TAFF·보조 디스크·보조 TAFF 순서와 첫 이름을 보존해 찾는다.</summary>
    /// <param name="pattern">폴더에 와일드카드가 없는 원본 경로 패턴.</param>
    public IReadOnlyList<string> MatchFinder(string pattern)
    {
        static string Fold(string value) => string.Concat(value.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
        static string Normalized(string value)
        {
            string path = value.Replace('\\', '/');
            // 원본 상대 경로 접두부만 걷어낸다.
            while (path.StartsWith("./", StringComparison.Ordinal) || path.StartsWith('/')) path = path.StartsWith('/') ? path[1..] : path[2..];
            return Fold(path);
        }
        string raw = pattern.Split('\0')[0].Replace('\\', '/');
        // 디스크 FileSpec은 입력 경로의 대소문자를 유지한 채 상대 접두부만 제거한다.
        if (raw.StartsWith("./", StringComparison.Ordinal)) raw = raw[2..]; raw = raw.TrimStart('/');
        string normalized = Normalized(raw); int slash = normalized.LastIndexOf('/');
        string folder = slash >= 0 ? normalized[..slash] : "", mask = normalized[(slash + 1)..];
        string originalFolder = slash >= 0 ? raw[..slash] : "";
        bool Matches(string value)
        {
            string name = Fold(value); int n = 0, m = 0, star = -1, retry = 0;
            // 마지막 별표의 소비 길이만 늘리며 원본 ASCII 와일드카드를 비교한다.
            while (n < name.Length)
            {
                if (m < mask.Length && (mask[m] == '?' || mask[m] == name[n])) { m++; n++; }
                else if (m < mask.Length && mask[m] == '*') { star = m++; retry = n; }
                else if (star >= 0) { m = star + 1; n = ++retry; }
                else return false;
            }
            // 마지막 별표는 빈 문자열과도 일치한다.
            while (m < mask.Length && mask[m] == '*') m++;
            return m == mask.Length;
        }
        var result = new List<string>(); var seen = new HashSet<string>(StringComparer.Ordinal);
        void Append(string value)
        {
            string name = "\\" + value.Replace('/', '\\').TrimStart('\\');
            if (seen.Add(Fold(name))) result.Add(name);
        }
        void Disk(string root)
        {
            if (raw.IndexOfAny(['*', '?']) < 0) { Append(raw); return; }
            string? directory = folder.Length == 0 ? root : GameDataLocator.FindDirectory(root, folder);
            if (directory is null || !Directory.Exists(directory)) return;
            // 실제 파일 시스템 열거 순서와 이름의 대소문자를 유지하며 디렉터리도 제외하지 않는다.
            foreach (string entry in Directory.EnumerateFileSystemEntries(directory))
            {
                string name = Path.GetFileName(entry);
                if (Matches(name)) Append(originalFolder.Length == 0 ? name : originalFolder + "/" + name);
            }
        }
        void Archive(int first, int last)
        {
            // 등록한 아카이브와 각 색인의 현재 순서를 유지한다.
            for (int i = first; i < last; i++) foreach (TaffEntry entry in Archives[i].Entries)
            {
                string path = Normalized(entry.Name); int at = path.LastIndexOf('/');
                string parent = at < 0 ? "" : path[..at];
                if (parent == folder && Matches(path[(at + 1)..])) Append(entry.Name);
            }
        }
        Disk(BaseDirectory); Archive(0, _primaryArchiveCount);
        if (SecondaryDirectory is not null && !string.Equals(BaseDirectory, SecondaryDirectory, StringComparison.Ordinal))
        { Disk(SecondaryDirectory); Archive(_primaryArchiveCount, Archives.Count); }
        return result;
    }

    /// <summary>원본 우선순위에 따라 파일 위치를 결정한다</summary>
    /// <param name="path">찾을 경로</param>
    /// <param name="diskPath">디스크에서 찾았을 때 실제 경로</param>
    /// <param name="archiveHit">아카이브에서 찾았을 때 아카이브와 엔트리</param>
    private GameFileSource Resolve(string path, out string? diskPath, out (TaffArchive Archive, TaffEntry Entry) archiveHit)
    {
        archiveHit = default;
        string normalized = NormalizePath(path);
        diskPath = GameDataLocator.FindFile(BaseDirectory, normalized);
        if (diskPath != null)
        {
            return GameFileSource.Disk;
        }
        // 기본 아카이브만 등록 순서대로 먼저 검색한다.
        for (int i = 0; i < _primaryArchiveCount; i++)
        {
            TaffArchive archive = Archives[i];
            if (archive.TryFind(normalized, out TaffEntry entry))
            {
                archiveHit = (archive, entry);
                return GameFileSource.Archive;
            }
        }
        if (SecondaryDirectory != null)
        {
            diskPath = GameDataLocator.FindFile(SecondaryDirectory, normalized);
            if (diskPath != null)
            {
                return GameFileSource.Disk;
            }
        }
        // 보조 디스크 다음에 보조 아카이브를 등록 순서대로 검색한다.
        for (int i = _primaryArchiveCount; i < Archives.Count; i++)
        {
            TaffArchive archive = Archives[i];
            if (archive.TryFind(normalized, out TaffEntry entry))
            {
                archiveHit = (archive, entry);
                return GameFileSource.Archive;
            }
        }
        return GameFileSource.None;
    }
}
