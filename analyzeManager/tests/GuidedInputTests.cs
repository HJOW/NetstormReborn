using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Text.Json;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>실제 게임·전역 훅 없이 입력 해석·녹화 시각·분할·재시작의 저장 결과를 검사한다.</summary>
public sealed class GuidedInputTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "NetstormInputTests", Guid.NewGuid().ToString("N"));

    /// <summary>일반·시스템 키의 누름과 해제를 읽기 쉬운 이름으로 구분하고 원시 코드도 보존한다.</summary>
    [Theory]
    [InlineData(0x0100, 0x41, "A", "down")]
    [InlineData(0x0101, 0x75, "F6", "up")]
    [InlineData(0x0104, 0xA4, "LeftAlt", "down")]
    [InlineData(0x0105, 0xA4, "LeftAlt", "up")]
    [InlineData(0x0100, 0x0D, "Enter", "down")]
    [InlineData(0x0100, 0xA2, "LeftCtrl", "down")]
    [InlineData(0x0100, 0x07, "VK_0x07", "down")]
    public void DecodesKeyboard(int message, uint virtualKey, string name, string action)
    {
        GuidedInput input = Assert.IsType<GuidedInput>(GuidedInput.Keyboard(message, virtualKey, 42, 0, 1234));
        Assert.Equal("keyboard", input.Type);
        Assert.Equal(action, input.Action);
        Assert.Equal(message, input.Message);
        Assert.Equal(virtualKey, input.VirtualKey);
        Assert.Equal(42u, input.ScanCode);
        Assert.Equal(name, input.Key);
        Assert.Equal(1234u, input.HookTimeMs);
        Assert.False(input.Injected);
        Assert.Null(input.X);
    }

    /// <summary>확장 키·Alt 조합·프로그램 생성 플래그를 기록하고 지원하지 않는 메시지는 무시한다.</summary>
    [Fact]
    public void KeepsKeyboardFlagsAndRejectsOtherMessages()
    {
        GuidedInput input = Assert.IsType<GuidedInput>(GuidedInput.Keyboard(0x0104, 0x25, 75, 0x31, 1234));
        Assert.Equal("Left", input.Key);
        Assert.Equal(0x31u, input.Flags);
        Assert.True(input.Extended);
        Assert.True(input.AltDown);
        Assert.True(input.Injected);
        Assert.Null(GuidedInput.Keyboard(0x0102, 0x41, 0, 0, 0));
    }

    /// <summary>모든 마우스 버튼의 누름·해제와 휠의 축·부호를 실제 Windows 데이터 형식으로 검사한다.</summary>
    [Theory]
    [InlineData(0x0200, 0u, "move", null, 0, null)]
    [InlineData(0x0201, 0u, "down", "left", 0, null)]
    [InlineData(0x0202, 0u, "up", "left", 0, null)]
    [InlineData(0x0204, 0u, "down", "right", 0, null)]
    [InlineData(0x0205, 0u, "up", "right", 0, null)]
    [InlineData(0x0207, 0u, "down", "middle", 0, null)]
    [InlineData(0x0208, 0u, "up", "middle", 0, null)]
    [InlineData(0x020B, 0x00010000u, "down", "x1", 0, null)]
    [InlineData(0x020C, 0x00020000u, "up", "x2", 0, null)]
    [InlineData(0x020A, 0x00780000u, "wheel", null, 120, "vertical")]
    [InlineData(0x020A, 0xFF880000u, "wheel", null, -120, "vertical")]
    [InlineData(0x020E, 0xFF880000u, "wheel", null, -120, "horizontal")]
    public void DecodesMouse(int message, uint data, string action, string? button, short wheel, string? axis)
    {
        var window = new GameWindow(12, "게임", -1200, 100, 1024, 768, true);
        GuidedInput input = Assert.IsType<GuidedInput>(GuidedInput.Mouse(message, new(-1150, 175), window, data, 0, 9876));
        Assert.Equal("mouse", input.Type);
        Assert.Equal(action, input.Action);
        Assert.Equal(message, input.Message);
        Assert.Equal(button, input.Button);
        Assert.Equal(wheel, input.Wheel);
        Assert.Equal(axis, input.WheelAxis);
        Assert.Equal(50, input.X);
        Assert.Equal(75, input.Y);
        Assert.Equal(-1150, input.ScreenX);
        Assert.Equal(175, input.ScreenY);
        Assert.True(input.Inside);
        Assert.Equal(window, input.Window);
        Assert.Equal(9876u, input.HookTimeMs);
    }

    /// <summary>게임 창 이동 후의 새 원점과 클라이언트 경계를 적용하고 화면 밖 좌표도 잃지 않는다.</summary>
    [Fact]
    public void KeepsOutsideCoordinatesAfterWindowMoves()
    {
        var window = new GameWindow(12, "게임", 200, 100, 1024, 768, true);
        GuidedInput outside = Assert.IsType<GuidedInput>(GuidedInput.Mouse(0x0200, new(199, 150), window, 0, 1, 0));
        Assert.Equal(-1, outside.X);
        Assert.False(outside.Inside);
        Assert.True(outside.Injected);
        GuidedInput edge = Assert.IsType<GuidedInput>(GuidedInput.Mouse(0x0200, new(1224, 150), window, 0, 0, 0));
        Assert.False(edge.Inside);
        GuidedInput moved = Assert.IsType<GuidedInput>(GuidedInput.Mouse(0x0200, new(225, 150), window with { X = 220 }, 0, 0, 0));
        Assert.Equal(5, moved.X);
        Assert.True(moved.Inside);
        Assert.Null(GuidedInput.Mouse(0x0203, new(225, 150), window, 0, 0, 0));
    }

    /// <summary>저장 시각 대신 전달받은 입력 시각을 사용하고 프레임 CSV와 같은 세션 기준에 맞춘다.</summary>
    [Fact]
    public void StoresInputTimesAndRecordingBoundaries()
    {
        long sessionCounter = Stopwatch.GetTimestamp() - 10 * Stopwatch.Frequency;
        long recordingCounter = sessionCounter + 5 * Stopwatch.Frequency;
        var utc = new DateTimeOffset(2026, 10, 1, 12, 0, 0, TimeSpan.Zero);
        string recordingId;
        using (var journal = new GuidedInputJournal(_root, sessionCounter, recordingCounter))
        {
            recordingId = journal.RecordingId;
            journal.Write(GuidedInput.Keyboard(0x0100, 0x75, 64, 0, 1234)!, utc, recordingCounter + Stopwatch.Frequency);
            journal.Write(GuidedInput.Keyboard(0x0101, 0x75, 64, 0, 1314)!, utc.AddMilliseconds(80),
                recordingCounter + Stopwatch.Frequency + Stopwatch.Frequency * 80 / 1000);
        }
        JsonElement[] rows = ReadRows();
        Assert.Equal(4, rows.Length);
        Assert.Equal("started", rows[0].GetProperty("input").GetProperty("action").GetString());
        Assert.Equal(0, rows[0].GetProperty("recordingElapsedMs").GetDouble());
        Assert.Equal("stopped", rows[^1].GetProperty("input").GetProperty("action").GetString());
        Assert.All(rows, row => Assert.Equal(recordingId, row.GetProperty("recordingId").GetString()));
        Assert.Equal(new long[] { 1, 2, 3, 4 }, rows.Select(row => row.GetProperty("sequence").GetInt64()));
        Assert.Equal(utc, rows[1].GetProperty("utc").GetDateTimeOffset());
        Assert.Equal(6000, rows[1].GetProperty("sessionElapsedMs").GetDouble());
        Assert.Equal(1000, rows[1].GetProperty("recordingElapsedMs").GetDouble());
        Assert.Equal(1080, rows[2].GetProperty("recordingElapsedMs").GetDouble(), 3);
        Assert.Equal(2, rows[1].GetProperty("schemaVersion").GetInt32());
        Assert.Equal("F6", rows[1].GetProperty("input").GetProperty("key").GetString());
    }

    /// <summary>UTF-8 바이트 한도로 분할하되 모든 조작을 보존하고 다시 시작할 때 앞선 파일을 덮지 않는다.</summary>
    [Fact]
    public void RotatesUtf8FilesAndPreservesPreviousRecording()
    {
        long counter = Stopwatch.GetTimestamp();
        var window = new GameWindow(12, new string('한', 160), 100, 100, 1024, 768, true);
        using (var journal = new GuidedInputJournal(_root, counter, counter, 1600))
        {
            // 한글 창 제목을 포함한 조작을 반복해 문자 수보다 UTF-8 바이트 수가 먼저 한도에 닿게 한다.
            for (int i = 0; i < 6; i++)
                journal.Write(GuidedInput.Mouse(0x0201, new(120 + i, 150), window, 0, 0, 0)!, DateTimeOffset.UtcNow, counter);
        }
        string[] initial = Directory.GetFiles(_root, "input-*.jsonl").Order().ToArray();
        Assert.True(initial.Length > 1);
        Assert.All(initial, file => Assert.InRange(new FileInfo(file).Length, 1, 1600));
        Assert.Equal(8, ReadRows().Length);
        Dictionary<string, byte[]> saved = initial.ToDictionary(file => file, File.ReadAllBytes);
        using (var resumed = new GuidedInputJournal(_root, counter, Stopwatch.GetTimestamp(), 1600))
        {
            // 입력 없는 구간도 시작·중단 기록을 남겨 재녹화 구간을 찾을 수 있다.
        }
        Assert.All(saved, pair => Assert.Equal(pair.Value, File.ReadAllBytes(pair.Key)));
        JsonElement[] rows = ReadRows();
        Assert.Equal(10, rows.Length);
        Assert.NotEqual(rows[0].GetProperty("recordingId").GetString(), rows[^1].GetProperty("recordingId").GetString());
        Assert.Equal(1, rows[8].GetProperty("sequence").GetInt64());
        Assert.Equal(Enumerable.Range(120, 6), rows.Where(row => row.GetProperty("input").GetProperty("type").GetString() == "mouse")
            .Select(row => row.GetProperty("input").GetProperty("screenX").GetInt32()));
        byte[] firstBytes = File.ReadAllBytes(initial[0]);
        Assert.False(firstBytes.AsSpan().StartsWith(Encoding.UTF8.GetPreamble()));
    }

    /// <summary>한 건이 분할 한도를 넘으면 저장 전에 거부하고 마지막 파일은 계속 안전하게 닫는다.</summary>
    [Fact]
    public void RejectsOversizedInputAndWritesAfterDispose()
    {
        long counter = Stopwatch.GetTimestamp();
        var journal = new GuidedInputJournal(_root, counter, counter, 1600);
        var window = new GameWindow(12, new string('한', 1000), 0, 0, 1024, 768, true);
        Assert.Throws<InvalidDataException>(() => journal.Write(GuidedInput.Mouse(0x0201, new(20, 50), window, 0, 0, 0)!, DateTimeOffset.UtcNow, counter));
        journal.Dispose();
        journal.Dispose();
        Assert.Equal(2, ReadRows().Length);
        Assert.Throws<ObjectDisposedException>(() => journal.Write(new("recording", "started"), DateTimeOffset.UtcNow, counter));
    }

    /// <summary>자유 플레이의 색인에서 모든 입력 조각을 찾아 영상과 같은 세션 폴더에서 분석할 수 있다.</summary>
    [Fact]
    public void FreeplayIndexIncludesEveryInputPart()
    {
        var store = new SessionStore(_root);
        string sessionId = "20261001T120000000Z-123456789abc";
        string directory = store.FreeplayDirectory(sessionId);
        long counter = Stopwatch.GetTimestamp();
        using (var journal = new GuidedInputJournal(directory, counter, counter, 1600))
        {
            // 여러 입력 조각이 생기도록 키 누름·해제를 교대로 기록한다.
            for (int i = 0; i < 8; i++)
                journal.Write(GuidedInput.Keyboard(i % 2 == 0 ? 0x0100 : 0x0101, 0x75, 64, 0, (uint)i)!, DateTimeOffset.UtcNow, counter);
        }
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(FreeplayRecordingIndex.Write(store, sessionId)));
        JsonElement index = document.RootElement;
        string[] files = Directory.GetFiles(directory, "input-*.jsonl").Order().ToArray();
        Assert.True(files.Length > 1);
        Assert.Equal(sessionId, index.GetProperty("sessionId").GetString());
        Assert.Contains("recordingElapsedMs", index.GetProperty("inputFormat").GetString());
        Assert.Equal(files.Select(Path.GetFileName), index.GetProperty("inputs").EnumerateArray().Select(part => part.GetProperty("file").GetString()));
        Assert.All(index.GetProperty("inputs").EnumerateArray(), part =>
            Assert.Equal(new FileInfo(Path.Combine(directory, part.GetProperty("file").GetString()!)).Length, part.GetProperty("bytes").GetInt64()));
    }

    /// <summary>저장된 모든 JSONL 줄을 파일 번호순으로 읽고 독립 JSON 값으로 복사한다.</summary>
    private JsonElement[] ReadRows() => Directory.GetFiles(_root, "input-*.jsonl").Order().SelectMany(File.ReadAllLines)
        .Select(line =>
        {
            using JsonDocument document = JsonDocument.Parse(line);
            return document.RootElement.Clone();
        }).ToArray();

    /// <summary>임시 테스트 경로의 절대 위치를 확인한 뒤 자신이 만든 파일만 정리한다.</summary>
    public void Dispose()
    {
        string resolved = Path.GetFullPath(_root);
        string expected = Path.Combine(Path.GetTempPath(), "NetstormInputTests") + Path.DirectorySeparatorChar;
        if (!resolved.StartsWith(expected, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("테스트 경로 이탈.");
        if (Directory.Exists(resolved)) Directory.Delete(resolved, true);
    }
}
