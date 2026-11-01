using Netstorm.Assets;

namespace Netstorm.Client.Host;

/// <summary>
/// 편집 맵 목록과 직접 Edit 진입이 함께 쓰는 단일 작업 namespace다. 원본 loose 파일과 빈 디렉터리를 최초 한 번만 작업본에 이관하고
/// 이관 상태를 <see cref="EditorCatalogLedger"/> 장부에 영속화하므로, 사용자가 지운 이름은 원본이나 아카이브로 되살아나지 않는다.
/// 이관 뒤 목록은 실제 작업 디렉터리의 OS 순서만 쓴다. cpppj <c>client/EditorMapCatalogFiles</c>의 <c>EditorCatalogWorkspace</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.126에서 추가했다. 장부 파일 형식·잠금은 0.125의 <see cref="EditorCatalogLedger"/>를 그대로 쓴다. 이 클래스는 호스트 수명 동안
/// 하나만 두고 목록, 새 이름 확인, 이후 실제 Editor 경로가 공유해야 하며 <see cref="Dispose"/>로 장부 잠금을 반납한다.
/// </remarks>
public sealed class EditorCatalogWorkspace : IDisposable
{
    /// <summary>원본·작업본 경로 선택기다.</summary>
    private readonly EditorWorkspacePaths _paths;
    /// <summary>검사된 작업본 파일 장치다.</summary>
    private readonly EditorFiles _files;
    /// <summary>이관 상태 장부다.</summary>
    private readonly EditorCatalogLedger _ledger;
    /// <summary>원본 loose 디스크 열거와 등록 아카이브 읽기를 빌려 주는 읽기 전용 원본 파일 시스템이다.</summary>
    private readonly GameFileSystem _source;

    /// <summary>
    /// 원본 파일 시스템과 작업 루트를 정한다. 생성은 경로만 검증하고 장부·잠금·작업본은 첫 실제 사용 전에는 만들지 않는다.
    /// </summary>
    /// <param name="source">원본 데이터 폴더의 읽기 전용 파일 시스템.</param>
    /// <param name="workRoot">작업본 루트. 생략하면 원본 폴더의 형제 <c>이름-editor</c>다.</param>
    /// <param name="extraProtectedRoots">원본 폴더 외에 쓰기를 막을 루트. 생략하면 작업본 상위의 <c>originals</c>, <c>originalCD</c>다.</param>
    public EditorCatalogWorkspace(GameFileSystem source, string? workRoot = null, IEnumerable<string>? extraProtectedRoots = null)
    {
        _source = source ?? throw new ArgumentNullException(nameof(source));
        _paths = new EditorWorkspacePaths(source.BaseDirectory, workRoot);
        string parent = Path.GetDirectoryName(_paths.WorkRoot) ?? _paths.WorkRoot;
        IEnumerable<string> extra = extraProtectedRoots ?? new[] { Path.Combine(parent, "originals"), Path.Combine(parent, "originalCD") };
        _files = new EditorFiles(_paths.WorkRoot, new[] { _paths.SourceRoot }.Concat(extra));
        // 장부 갱신은 작업 파일 장치의 형제 metadata 장치를 거쳐 write-through 교체와 일시 거부 재시도를 쓴다.
        _ledger = new EditorCatalogLedger(_paths, _files.SiblingNamespace("-state"));
    }

    /// <summary>같은 작업 namespace의 파일 장치다. 새 이름 확인과 이후 저장·삭제가 이 장치를 빌린다.</summary>
    public EditorFiles Files => _files;

    /// <summary>원본·작업본 루트와 경로 대응을 제공한다.</summary>
    public EditorWorkspacePaths Paths => _paths;

    /// <summary>검증된 설정 경로를 작업 루트 기준 상대 키로 바꾼다. 대소문자는 보존하고 구분자는 슬래시다.</summary>
    private string Key(string configured) => Path.GetRelativePath(_paths.WorkRoot, _paths.Resolve(configured)).Replace('\\', '/');

    /// <summary>키가 와일드카드를 가지는지 판정한다.</summary>
    private static bool HasWildcard(string key) => key.IndexOfAny(['*', '?']) >= 0;

    /// <summary>키에 해당하는 원본 물리 경로를 돌려준다. 링크가 원본 루트 밖으로 향하면 거부하며 원본에는 쓰지 않는다.</summary>
    private string SourcePath(string key)
    {
        string path = Path.Combine(_paths.SourceRoot, key.Replace('/', Path.DirectorySeparatorChar));
        if (!EditorWorkspacePaths.Inside(_paths.SourceRoot, EditorWorkspacePaths.Canonical(path))) throw new InvalidOperationException("편집 원본 입력의 외부 링크 경계");
        return path;
    }

    /// <summary>
    /// 한 경로를 장부에 따라 이관한다. 완료된 키는 작업본이 사라져도 다시 복사하지 않고, 중단된 Pending은 작업본이 있을 때만 완료로 바꾼다.
    /// 원본이 명시적으로 없으면 완료도 Pending도 기록하지 않는다. 아카이브 내용을 주면 그 실제 설치만 작업 파일과 이관 상태를 만든다.
    /// </summary>
    /// <param name="key">작업본 기준 상대 키.</param>
    /// <param name="archive">명시적으로 설치할 아카이브 내용. 없으면 null.</param>
    private void Import(string key, byte[]? archive = null)
    {
        uint found = _ledger.State(key); string target = _files.ResolvePath(key);
        bool exists = File.Exists(target) || Directory.Exists(target);
        if (found != 0)
        {
            if (found == 2) return;
            if (!exists) throw new InvalidOperationException("편집 이관 중단 Pending 항목의 작업본이 없습니다:" + key);
            _ledger.SetState(key, 2); return;
        }
        // 장부에 없지만 사용자가 이미 만든 작업본은 원본 복사 없이 완료로 기록한다.
        if (exists) { _ledger.SetState(key, 2); return; }
        string physical = SourcePath(key); bool isDirectory = Directory.Exists(physical), present = isDirectory || File.Exists(physical);
        if (!present && archive is null) return;
        _ledger.SetState(key, 1);
        if (present && isDirectory) _files.EnsureDirectory(key);
        else if (present) _files.CopyMissing(physical, key);
        else _files.PreparePath(key, archive!);
        _ledger.SetState(key, 2);
    }

    /// <summary>
    /// 설정 경로를 같은 작업본의 검사된 절대 경로로 해석한다. 상대 경로, 원본 안의 절대 경로, 작업본 안의 절대 경로를 모두 같은 키로 대응시킨다.
    /// 파일을 읽거나 만들지 않는다.
    /// </summary>
    /// <param name="configured">설정 경로.</param>
    public string Resolve(string configured) => _files.ResolvePath(Key(configured));

    /// <summary>
    /// 현재 원본의 미이관 loose 파일과 빈 디렉터리를 한 번만 작업본으로 복사한다. 와일드카드 패턴이 가리키는 원본 폴더가 있으면 빈 작업 폴더도
    /// 만들어 OS의 "."·".." 후보를 실제로 열거하게 한다. 명시 부재는 기록하지 않는다.
    /// </summary>
    /// <param name="configuredPattern">폴더에는 와일드카드가 없는 패턴.</param>
    public void ImportDisk(string configuredPattern)
    {
        string pattern = Key(configuredPattern); string name = Path.GetFileName(pattern);
        string parent = Path.GetDirectoryName(pattern)?.Replace('\\', '/') ?? "", folder = parent.Length == 0 ? "." : parent;
        if (HasWildcard(name) && Directory.Exists(SourcePath(folder))) _files.EnsureDirectory(folder);
        // 원본 loose 디스크 열거 결과마다 장부를 거쳐 한 번씩 이관한다. 점 두 후보는 복사하지 않는다.
        foreach (string path in _source.MatchDisk(pattern))
        {
            string leaf = Path.GetFileName(path.Replace('\\', '/'));
            if (leaf is "." or "..") continue;
            Import(Key(path));
        }
    }

    /// <summary>
    /// 직접 Edit 진입도 같은 장부를 지키도록 한 파일을 준비한다. 완료된 키에서는 원본과 아카이브를 다시 조회하지 않으며,
    /// 아카이브 설치는 장부에 없고 작업본도 없을 때만 명시적으로 이관한다.
    /// </summary>
    /// <param name="configured">설정 경로.</param>
    /// <returns>준비한 작업본의 절대 경로.</returns>
    public string Prepare(string configured)
    {
        string key = Key(configured); Import(key);
        if (_ledger.State(key) != 0 || File.Exists(_files.ResolvePath(key)) || Directory.Exists(_files.ResolvePath(key))) return _files.ResolvePath(key);
        byte[]? bytes = _source.TryReadArchiveOnly(key);
        if (bytes is not null) Import(key, bytes);
        return _files.ResolvePath(key);
    }

    /// <summary>
    /// 최초 이관을 마친 뒤 실제 작업 디렉터리의 OS 결과만 돌려준다. 논리 중복·디렉터리·"."·".." 후보를 제거하거나 합성하지 않는다.
    /// </summary>
    /// <param name="configuredPattern">폴더에는 와일드카드가 없는 패턴.</param>
    public IReadOnlyList<string> Enumerate(string configuredPattern)
    {
        string key = Key(configuredPattern); ImportDisk(configuredPattern);
        return _files.MatchDisk(key);
    }

    /// <summary>
    /// 분류와 메타 조회마다 현재 작업 파일을 읽는다. 장부에 완료로 있는 키가 작업본에서 사라졌으면 어떤 원본·아카이브 fallback도 없이 null이다.
    /// 이관되지 않은 아카이브 전용 내용은 읽기만 하고 목록이나 장부에 더하지 않는다.
    /// </summary>
    /// <param name="configured">설정 경로.</param>
    public byte[]? Read(string configured)
    {
        string key = Key(configured);
        if (!HasWildcard(key))
        {
            Import(key);
            if (_ledger.State(key) != 0) return _files.TryRead(key);
        }
        return _files.TryRead(key) ?? _source.TryReadArchiveOnly(key);
    }

    /// <summary>장부 잠금을 반납한다. 이미 기록한 장부와 작업본은 그대로 보존한다.</summary>
    public void Dispose() => _ledger.Dispose();
}
