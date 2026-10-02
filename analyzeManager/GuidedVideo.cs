using System.Text;

namespace Netstorm.AnalyzeManager;

/// <summary>MJPEG AVI를 재생 가능한 크기별 파일로 저장하고 프레임 시각을 CSV로 남긴다.</summary>
public sealed class GuidedVideo : IDisposable
{
    /// <summary>마지막 프레임과 헤더를 갱신할 여유를 둔 분할 크기.</summary>
    public const long SegmentLimitBytes = 48_000_000;
    private readonly string _directory;
    private readonly int _width;
    private readonly int _height;
    private readonly int _fps;
    private readonly long _segmentLimitBytes;
    private FileStream? _stream;
    private BinaryWriter? _writer;
    private StreamWriter? _times;
    private int _segment;
    private int _frames;
    private long _totalFramesPosition;
    private long _streamFramesPosition;
    private long _moviSizePosition;

    /// <summary>기존 녹화 번호 다음부터 시작해 재실행 시 앞선 파일을 보존한다.</summary>
    public GuidedVideo(string directory, int width, int height, int fps = AnalysisFrameRate.Default, long segmentLimitBytes = SegmentLimitBytes)
    {
        if (width <= 0 || height <= 0 || fps is < 1 or > AnalysisFrameRate.High) throw new ArgumentOutOfRangeException(nameof(fps));
        if (segmentLimitBytes is < 512 or > SegmentLimitBytes) throw new ArgumentOutOfRangeException(nameof(segmentLimitBytes));
        SessionStore.RejectReparse(directory);
        Directory.CreateDirectory(directory);
        _directory = directory;
        _width = width;
        _height = height;
        _fps = fps;
        _segmentLimitBytes = segmentLimitBytes;
        _segment = Directory.EnumerateFiles(directory, "video-*.avi")
            .Select(path => int.TryParse(Path.GetFileNameWithoutExtension(path).AsSpan(6), out int number) ? number : 0)
            .DefaultIfEmpty(0).Max();
    }

    /// <summary>JPEG 한 장을 현재 AVI에 추가하고 48 MB에 가까워지면 새 파일을 연다.</summary>
    public string WriteFrame(byte[] jpeg, DateTimeOffset capturedUtc, double elapsedMs)
    {
        if (jpeg.Length == 0 || jpeg.Length + 256 >= _segmentLimitBytes) throw new InvalidOperationException("영상 프레임 크기가 분할 한도를 넘습니다.");
        if (_writer == null) OpenSegment();
        string timestamp = $"{_frames + 1},{capturedUtc:O},{elapsedMs:F3}\n";
        if (_stream!.Length + jpeg.Length + 8 + (jpeg.Length & 1) >= _segmentLimitBytes
            || _times!.BaseStream.Length + Encoding.UTF8.GetByteCount(timestamp) >= SegmentLimitBytes)
        {
            CloseSegment();
            OpenSegment();
            timestamp = $"1,{capturedUtc:O},{elapsedMs:F3}\n";
        }
        // AVI 청크 길이는 패딩 바이트를 제외한다.
        WriteFourCc(_writer!, "00dc");
        _writer!.Write(jpeg.Length);
        _writer.Write(jpeg);
        if ((jpeg.Length & 1) != 0) _writer.Write((byte)0);
        _frames++;
        _times!.Write(timestamp);
        _times.Flush();
        PatchHeaders();
        return $"recording/video-{_segment:0000}.avi";
    }

    /// <summary>AVI 헤더와 같은 번호의 프레임 시각 파일을 만든다.</summary>
    private void OpenSegment()
    {
        _segment++;
        _frames = 0;
        string video = Path.Combine(_directory, $"video-{_segment:0000}.avi");
        string times = Path.Combine(_directory, $"video-{_segment:0000}.frames.csv");
        SessionStore.RejectReparse(video);
        SessionStore.RejectReparse(times);
        _stream = new FileStream(video, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read);
        _writer = new BinaryWriter(_stream, Encoding.ASCII, true);
        _times = new StreamWriter(new FileStream(times, FileMode.CreateNew, FileAccess.Write, FileShare.Read), new UTF8Encoding(false));
        _times.WriteLine("frame,utc,sessionElapsedMs");
        WriteFourCc(_writer, "RIFF");
        _writer.Write(0);
        WriteFourCc(_writer, "AVI ");
        WriteFourCc(_writer, "LIST");
        long headerSize = _stream.Position;
        _writer.Write(0);
        WriteFourCc(_writer, "hdrl");
        WriteFourCc(_writer, "avih");
        _writer.Write(56);
        _writer.Write(1_000_000 / _fps);
        _writer.Write(0);
        _writer.Write(0);
        _writer.Write(0);
        _totalFramesPosition = _stream.Position;
        _writer.Write(0);
        _writer.Write(0);
        _writer.Write(1);
        _writer.Write(0);
        _writer.Write(_width);
        _writer.Write(_height);
        // AVI 메인 헤더의 예약 필드 네 개를 채운다.
        for (int i = 0; i < 4; i++) _writer.Write(0);
        WriteFourCc(_writer, "LIST");
        long streamListSize = _stream.Position;
        _writer.Write(0);
        WriteFourCc(_writer, "strl");
        WriteFourCc(_writer, "strh");
        _writer.Write(56);
        WriteFourCc(_writer, "vids");
        WriteFourCc(_writer, "MJPG");
        _writer.Write(0);
        _writer.Write((short)0);
        _writer.Write((short)0);
        _writer.Write(0);
        _writer.Write(1);
        _writer.Write(_fps);
        _writer.Write(0);
        _streamFramesPosition = _stream.Position;
        _writer.Write(0);
        _writer.Write(0);
        _writer.Write(uint.MaxValue);
        _writer.Write(0);
        _writer.Write((short)0);
        _writer.Write((short)0);
        _writer.Write(checked((short)_width));
        _writer.Write(checked((short)_height));
        WriteFourCc(_writer, "strf");
        _writer.Write(40);
        _writer.Write(40);
        _writer.Write(_width);
        _writer.Write(_height);
        _writer.Write((short)1);
        _writer.Write((short)24);
        WriteFourCc(_writer, "MJPG");
        _writer.Write(checked(_width * _height * 3));
        _writer.Write(0);
        _writer.Write(0);
        _writer.Write(0);
        _writer.Write(0);
        long endHeader = _stream.Position;
        PatchInt32(streamListSize, checked((int)(endHeader - streamListSize - 4)));
        PatchInt32(headerSize, checked((int)(endHeader - headerSize - 4)));
        WriteFourCc(_writer, "LIST");
        _moviSizePosition = _stream.Position;
        _writer.Write(0);
        WriteFourCc(_writer, "movi");
        PatchHeaders();
    }

    /// <summary>녹화 도중 중단되어도 이미 완료한 프레임까지 읽을 수 있도록 길이를 매번 갱신한다.</summary>
    private void PatchHeaders()
    {
        _writer!.Flush();
        long end = _stream!.Position;
        PatchInt32(4, checked((int)(end - 8)));
        PatchInt32(_moviSizePosition, checked((int)(end - _moviSizePosition - 4)));
        PatchInt32(_totalFramesPosition, _frames);
        PatchInt32(_streamFramesPosition, _frames);
        _stream.Position = end;
        _stream.Flush();
    }

    /// <summary>현재 쓰기 위치를 보존하며 32비트 AVI 헤더 필드를 수정한다.</summary>
    private void PatchInt32(long position, int value)
    {
        long current = _stream!.Position;
        _stream.Position = position;
        _writer!.Write(value);
        _stream.Position = current;
    }

    /// <summary>RIFF의 네 글자 식별자를 ASCII 바이트로 기록한다.</summary>
    private static void WriteFourCc(BinaryWriter writer, string value) => writer.Write(Encoding.ASCII.GetBytes(value));

    /// <summary>이 녹화기가 만든 AVI 스트림 헤더에서 FPS를 읽어 과거 10FPS·새 30/60FPS를 구분한다.</summary>
    public static int ReadFramesPerSecond(string path)
    {
        SessionStore.RejectReparse(path);
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        using var reader = new BinaryReader(stream, Encoding.ASCII);
        if (stream.Length < 140 || Encoding.ASCII.GetString(reader.ReadBytes(4)) != "RIFF")
            throw new InvalidDataException($"녹화 AVI 헤더가 손상되었습니다: {path}");
        stream.Position = 100;
        if (Encoding.ASCII.GetString(reader.ReadBytes(4)) != "strh")
            throw new InvalidDataException($"녹화 AVI 스트림 헤더가 없습니다: {path}");
        stream.Position = 128;
        int scale = reader.ReadInt32();
        int rate = reader.ReadInt32();
        if (scale <= 0 || rate <= 0 || rate % scale != 0 || rate / scale > AnalysisFrameRate.High)
            throw new InvalidDataException($"녹화 AVI의 FPS 값이 잘못되었습니다: {path}");
        return rate / scale;
    }

    /// <summary>현재 세그먼트 파일을 닫고 다음 시작 시 새 번호를 쓰도록 한다.</summary>
    private void CloseSegment()
    {
        _times?.Dispose();
        _writer?.Dispose();
        _stream?.Dispose();
        _times = null;
        _writer = null;
        _stream = null;
    }

    /// <summary>보류 중인 영상과 프레임 시각 파일을 닫는다.</summary>
    public void Dispose() => CloseSegment();
}
