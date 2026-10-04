using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Assets;

/// <summary>TAFF 아카이브 안의 파일 하나</summary>
/// <param name="Name">원본 경로 (예: "\d\altar.type")</param>
/// <param name="Offset">데이터 영역 기준 오프셋</param>
/// <param name="Size">바이트 크기</param>
public sealed record TaffEntry(string Name, int Offset, int Size);

/// <summary>
/// netstorm.tarc (TAFF 아카이브) 읽기. 포맷: docs/formats/taff.md
/// </summary>
public sealed class TaffArchive
{
    /// <summary>파일 시작 매직 ("TAFF v" 뒤에 버전 문자열과 0x1A 가 온다)</summary>
    private static ReadOnlySpan<byte> Magic => "TAFF v"u8;

    /// <summary>헤더: 엔트리 개수 (u32) 위치</summary>
    private const int OffsetEntryCount = 0x14;

    /// <summary>헤더: 디렉터리 시작 위치 (u32) 위치</summary>
    private const int OffsetDirectory = 0x20;

    /// <summary>헤더: 데이터 영역 시작 위치 (u32) 위치</summary>
    private const int OffsetData = 0x28;

    /// <summary>디렉터리 레코드 고정부 크기 (offset u32 + size u32)</summary>
    private const int RecordFixedSize = 8;

    /// <summary>원본 이름은 Windows-1252 이지만 경로에는 ASCII 만 쓰이므로 Latin-1 로 읽는다</summary>
    private static readonly Encoding NameEncoding = Encoding.Latin1;

    private readonly byte[] _raw;
    private readonly int _dataOffset;
    private readonly Dictionary<string, TaffEntry> _byName;

    /// <summary>매직 문자열에서 읽은 버전 표기 (예: "TAFF v0.2")</summary>
    public string Version { get; }

    /// <summary>디렉터리 순서대로 나열한 엔트리 목록</summary>
    public IReadOnlyList<TaffEntry> Entries { get; }

    /// <summary>메모리에 올린 아카이브 바이트로부터 만든다</summary>
    /// <param name="raw">아카이브 파일 전체 내용</param>
    public TaffArchive(byte[] raw)
    {
        _raw = raw;
        if (!raw.AsSpan().StartsWith(Magic))
        {
            throw new InvalidDataException("TAFF 아카이브가 아닙니다.");
        }
        int eof = Array.IndexOf(raw, (byte)0x1A);
        Version = NameEncoding.GetString(raw, 0, eof);

        int count = ReadInt32(OffsetEntryCount);
        int pos = ReadInt32(OffsetDirectory);
        _dataOffset = ReadInt32(OffsetData);

        var entries = new List<TaffEntry>(count);
        _byName = new Dictionary<string, TaffEntry>(count, StringComparer.OrdinalIgnoreCase);
        // 디렉터리 레코드 [offset u32][size u32][NUL 종료 이름] 을 엔트리 개수만큼 읽는다
        for (int i = 0; i < count; i++)
        {
            int offset = ReadInt32(pos);
            int size = ReadInt32(pos + 4);
            int nameStart = pos + RecordFixedSize;
            int nameEnd = Array.IndexOf(raw, (byte)0, nameStart);
            string name = NameEncoding.GetString(raw, nameStart, nameEnd - nameStart);
            var entry = new TaffEntry(name, offset, size);
            entries.Add(entry);
            _byName[NormalizePath(name)] = entry;
            pos = nameEnd + 1;
        }
        Entries = entries;
    }

    /// <summary>파일에서 아카이브를 연다</summary>
    /// <param name="path">netstorm.tarc 경로</param>
    public static TaffArchive Open(string path) => new(File.ReadAllBytes(path));

    /// <summary>엔트리의 복호화된 데이터를 돌려준다</summary>
    /// <param name="entry">읽을 엔트리</param>
    public byte[] Read(TaffEntry entry)
    {
        byte[] data = _raw.AsSpan(_dataOffset + entry.Offset, entry.Size).ToArray();
        XorCipher.Apply(data);
        return data;
    }

    /// <summary>
    /// 이름으로 엔트리를 찾는다. 대소문자와 경로 구분자(\ /)를 무시하며, 앞의 구분자는 없어도 된다.
    /// 예: "d/altar.type", "\D\ALTAR.TYPE" 모두 같은 엔트리
    /// </summary>
    /// <param name="path">찾을 경로</param>
    /// <param name="entry">찾은 엔트리</param>
    public bool TryFind(string path, out TaffEntry entry) =>
        _byName.TryGetValue(NormalizePath(path), out entry!);

    /// <summary>경로를 비교용 형식("d/altar.type")으로 정규화한다</summary>
    /// <param name="path">원본 또는 사용자 경로</param>
    public static string NormalizePath(string path) =>
        path.Replace('\\', '/').TrimStart('/').ToLowerInvariant();

    /// <summary>리틀 엔디언 int32 읽기</summary>
    /// <param name="offset">파일 내 위치</param>
    private int ReadInt32(int offset) => BinaryPrimitives.ReadInt32LittleEndian(_raw.AsSpan(offset, 4));
}
