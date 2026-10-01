using System.Diagnostics;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Netstorm.AnalyzeManager;

/// <summary>두 사용자 녹화 모드에서 조작·구간 경계·동기화 시각을 별도 UTF-8 JSONL 파일에 저장한다.</summary>
public sealed class GuidedInputJournal : IDisposable
{
    /// <summary>기존 입력 로그와 구분할 수 있는 확장 형식 번호.</summary>
    public const int SchemaVersion = 2;
    /// <summary>키와 마우스에 해당하지 않는 선택 필드를 생략해 긴 녹화의 용량을 줄인다.</summary>
    private static readonly JsonSerializerOptions Json = new(SessionStore.Json) { DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull };
    private readonly string _directory;
    private readonly long _sessionCounter;
    private readonly long _recordingCounter;
    private readonly int _partBytes;
    private StreamWriter? _writer;
    private long _bytes;
    private long _sequence;
    private int _part;
    private bool _disposed;

    /// <summary>다시 녹화할 때 새로 부여하며 해당 구간의 분할 파일들에는 같은 ID를 쓴다.</summary>
    public string RecordingId { get; } = Guid.NewGuid().ToString("N");
    /// <summary>현재 조각의 파일명. 녹화 시작 이벤트에 넣어 기록 위치를 찾게 한다.</summary>
    public string FileName => $"input-{_part:0000}.jsonl";

    /// <summary>앞선 파일을 덮지 않고 새 조각을 즉시 만들어 입력 없는 녹화도 시작 시각을 남긴다.</summary>
    public GuidedInputJournal(string directory, long sessionCounter, long recordingCounter, int partBytes = SessionStore.DefaultPartBytes)
    {
        if (partBytes is <= 0 or > SessionStore.DefaultPartBytes) throw new ArgumentOutOfRangeException(nameof(partBytes));
        SessionStore.RejectReparse(directory);
        Directory.CreateDirectory(directory);
        _directory = directory;
        _sessionCounter = sessionCounter;
        _recordingCounter = recordingCounter;
        _partBytes = partBytes;
        _part = Directory.EnumerateFiles(directory, "input-*.jsonl")
            .Select(path => int.TryParse(Path.GetFileNameWithoutExtension(path).AsSpan(6), out int number) ? number : 0)
            .DefaultIfEmpty(0).Max();
        try { Write(new("recording", "started"), DateTimeOffset.UtcNow, recordingCounter); }
        catch { _writer?.Dispose(); throw; }
    }

    /// <summary>훅 진입 때 채취한 시각을 저장해 좌표 조회·디스크 쓰기 시간을 조작 시각에 더하지 않는다.</summary>
    public void Write(GuidedInput input, DateTimeOffset utc, long counter)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        string line = JsonSerializer.Serialize(new
        {
            schemaVersion = SchemaVersion, recordingId = RecordingId, sequence = _sequence + 1, utc,
            sessionElapsedMs = (counter - _sessionCounter) * 1000.0 / Stopwatch.Frequency,
            recordingElapsedMs = (counter - _recordingCounter) * 1000.0 / Stopwatch.Frequency,
            input,
        }, Json) + "\n";
        int size = Encoding.UTF8.GetByteCount(line);
        if (size > _partBytes) throw new InvalidDataException("입력 한 건이 로그 조각의 크기 한도를 넘습니다.");
        if (_writer == null || _bytes + size > _partBytes)
        {
            _writer?.Dispose();
            _writer = null;
            _part++;
            string path = Path.Combine(_directory, FileName);
            SessionStore.RejectReparse(path);
            _writer = new StreamWriter(new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read), new UTF8Encoding(false));
            _bytes = 0;
        }
        _writer.Write(line);
        _writer.Flush();
        _bytes += size;
        _sequence++;
    }

    /// <summary>중단 경계를 기록하고 쓰기 실패가 나더라도 마지막 파일 핸들을 닫는다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        try { Write(new("recording", "stopped"), DateTimeOffset.UtcNow, Stopwatch.GetTimestamp()); }
        finally { _disposed = true; _writer?.Dispose(); }
    }
}
