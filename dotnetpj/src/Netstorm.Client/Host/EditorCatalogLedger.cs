using System.Text;

namespace Netstorm.Client.Host;

/// <summary>cpppj의 편집 이관 장부를 같은 형제 상태 디렉터리에서 잠그고 영속적으로 관리한다.</summary>
public sealed class EditorCatalogLedger : IDisposable
{
    /// <summary>cpppj 버전 1의 8바이트 서명이다.</summary>
    private static readonly byte[] Magic = [0x4e, 0x53, 0x45, 0x46, 0x4f, 0x52, 0x54, 1];
    /// <summary>원본과 작업본의 검증된 경로 선택기다.</summary>
    private readonly EditorWorkspacePaths _paths;
    /// <summary>원본·작업본과 구분된 형제 장부 디렉터리다.</summary>
    private readonly string _metadata;
    /// <summary>장부 갱신을 flush·write-through 교체·일시 거부 재시도로 맡는 형제 metadata 파일 장치다.</summary>
    private readonly EditorFiles _files;
    /// <summary>전체 경로별 현재 이관 상태이며 ASCII 소문자 키를 사용한다.</summary>
    private SortedDictionary<string, uint> _entries = new(StringComparer.Ordinal);
    /// <summary>호스트 수명 동안 유지하는 배타적 장부 잠금이다.</summary>
    private FileStream? _lock;
    /// <summary>검증된 장부를 이미 읽었는지 나타낸다.</summary>
    private bool _loaded;
    /// <summary>쓰기 실패 후 같은 소유자의 재사용을 막는다.</summary>
    private bool _poisoned;
    /// <summary>대상 경로만 선택하며 장부나 디렉터리는 아직 만들지 않는다.</summary>
    /// <param name="paths">원본·작업본 경로 선택기.</param>
    /// <param name="files">
    /// 작업 파일 장치의 형제 metadata 장치(<see cref="EditorFiles.SiblingNamespace"/>). 생략하면 원본과 작업본만 보호하는 같은 위치의 장치를 만든다.
    /// (0.127에서 추가: 장부 갱신이 cpppj <c>WriteAtomic</c>의 write-through 교체와 최대 약 1초 재시도를 쓰게 한다.)
    /// </param>
    public EditorCatalogLedger(EditorWorkspacePaths paths, EditorFiles? files = null)
    {
        _paths = paths ?? throw new ArgumentNullException(nameof(paths)); _metadata = paths.WorkRoot + "-state";
        CheckMetadata(); _files = files ?? new EditorFiles(_metadata, [paths.SourceRoot, paths.WorkRoot]);
        if (!string.Equals(_files.RootPath, EditorWorkspacePaths.Canonical(_metadata), StringComparison.OrdinalIgnoreCase)) throw new ArgumentException("편집 장부 파일 장치가 형제 상태 폴더가 아님", nameof(files));
    }
    /// <summary>형제 상태 경로가 링크로 다른 위치에 매핑되지 않았는지 검사한다.</summary>
    private void CheckMetadata()
    {
        if (!string.Equals(EditorWorkspacePaths.Canonical(_metadata), Path.GetFullPath(_metadata), StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("편집 장부의 외부 링크 경계");
        if (EditorWorkspacePaths.Inside(_paths.SourceRoot, _metadata) || EditorWorkspacePaths.Inside(_paths.WorkRoot, _metadata))
            throw new InvalidOperationException("편집 장부와 원본 또는 작업본 경로 중첩");
    }
    /// <summary>기존 장부 파일과 잠금 파일의 링크 우회를 거부한다.</summary>
    private string MetadataFile(string name)
    {
        CheckMetadata(); string expected = Path.Combine(_metadata, name);
        if (!string.Equals(EditorWorkspacePaths.Canonical(expected), expected, StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("편집 장부 파일의 외부 링크 경계");
        return expected;
    }
    /// <summary>원본처럼 ASCII 대문자만 접고 나머지 문자는 보존한다.</summary>
    private static string Fold(string value) => string.Concat(value.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
    /// <summary>경로를 cpppj 장부의 소문자 UTF-8 표기와 슬래시로 변환한다.</summary>
    private static string Identity(string path) => Fold(path.Replace('\\', '/'));
    /// <summary>검증된 설정 경로를 완전한 작업본 상대 키로 변환한다.</summary>
    private string Key(string path) => Fold(Path.GetRelativePath(_paths.WorkRoot, _paths.Resolve(path)).Replace('\\', '/'));
    /// <summary>길이 접두 문자열을 끝 범위까지 엄격히 읽는다.</summary>
    private static string ReadText(BinaryReader reader)
    {
        uint count = reader.ReadUInt32(); if (count > reader.BaseStream.Length - reader.BaseStream.Position) throw new InvalidDataException("편집 장부 문자열 범위 오류");
        return new UTF8Encoding(false, true).GetString(reader.ReadBytes(checked((int)count)));
    }
    /// <summary>UTF-8 바이트 길이와 문자열을 원본 순서대로 쓴다.</summary>
    private static void WriteText(BinaryWriter writer, string value)
    { byte[] bytes = Encoding.UTF8.GetBytes(value); writer.Write(checked((uint)bytes.Length)); writer.Write(bytes); }
    /// <summary>배타 잠금과 전체 장부 검증을 최초 조회 때 수행한다.</summary>
    private void Load()
    {
        if (_poisoned) throw new InvalidOperationException("편집 장부 갱신 실패 후 재사용 금지"); if (_loaded) return;
        CheckMetadata(); Directory.CreateDirectory(_metadata); CheckMetadata();
        _lock ??= new FileStream(MetadataFile("catalog.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        string path = MetadataFile("catalog-v1.dat");
        if (!File.Exists(path)) { _loaded = true; return; }
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        using var reader = new BinaryReader(stream, Encoding.UTF8);
        if (!reader.ReadBytes(Magic.Length).SequenceEqual(Magic)) throw new InvalidDataException("편집 장부 서명 오류");
        if (ReadText(reader) != Identity(_paths.SourceRoot) || ReadText(reader) != Identity(_paths.WorkRoot)) throw new InvalidDataException("편집 장부 루트 불일치");
        uint count = reader.ReadUInt32(); var entries = new SortedDictionary<string, uint>(StringComparer.Ordinal);
        // 유효한 전체 키와 상태를 모두 확인한 뒤 현재 장부로 채택한다.
        for (uint i = 0; i < count; i++)
        {
            string key = ReadText(reader); uint state = reader.ReadUInt32();
            if (state is not (1 or 2) || Key(key) != key || !entries.TryAdd(key, state)) throw new InvalidDataException("편집 장부 항목 오류");
        }
        if (stream.Position != stream.Length) throw new InvalidDataException("편집 장부 끝 범위 오류");
        _entries = entries; _loaded = true;
    }
    /// <summary>정규화된 현재 항목의 상태를 반환한다. 0은 아직 이관하지 않았음을 뜻한다.</summary>
    public uint State(string path) { Load(); return _entries.GetValueOrDefault(Key(path)); }
    /// <summary>대기 또는 완료 상태를 임시 파일 flush와 같은 폴더 교체로 영속화한다.</summary>
    public void SetState(string path, uint state)
    {
        if (state is not (1 or 2)) throw new ArgumentOutOfRangeException(nameof(state)); Load();
        var next = new SortedDictionary<string, uint>(_entries, StringComparer.Ordinal) { [Key(path)] = state };
        MetadataFile("catalog-v1.dat"); MetadataFile("catalog-v1.dat.pending");
        _loaded = false; _poisoned = true;
        // 전체 장부를 메모리에 만든 뒤 한 번에 임시 파일 flush와 write-through 교체로 영속화한다.
        using var buffer = new MemoryStream();
        using (var writer = new BinaryWriter(buffer, Encoding.UTF8, true))
        {
            writer.Write(Magic);
            WriteText(writer, Identity(_paths.SourceRoot)); WriteText(writer, Identity(_paths.WorkRoot)); writer.Write(checked((uint)next.Count));
            // 원본 장부처럼 전체 경로 순서대로 모든 상태를 기록한다.
            foreach (var item in next) { WriteText(writer, item.Key); writer.Write(item.Value); }
        }
        _files.WriteAtomic("catalog-v1.dat", buffer.ToArray()); _entries = next; _loaded = true; _poisoned = false;
    }
    /// <summary>열려 있는 실제 장부 잠금을 반납한다.</summary>
    public void Dispose() { _lock?.Dispose(); _lock = null; }
}
