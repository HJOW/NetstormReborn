using System.Diagnostics;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>사람이 조작하는 게임의 영상·출력 소리·물리 입력을 함께 기록한다.</summary>
public sealed class GuidedRecorder : IDisposable
{
    /// <summary>화면 샘플링 속도. 원본 게임 조작 중 CPU 점유를 줄이기 위해 10 FPS를 쓴다.</summary>
    private const int FramesPerSecond = 10;
    /// <summary>드래그 경로는 최대 50회/초만 기록한다.</summary>
    private static readonly long MouseMoveIntervalTicks = 20 * Stopwatch.Frequency / 1000;
    /// <summary>전역 마우스 입력을 읽는 Windows 저수준 훅 종류.</summary>
    private const int MouseHookType = 14;
    /// <summary>전역 키 입력을 읽는 Windows 저수준 훅 종류.</summary>
    private const int KeyboardHookType = 13;
    private readonly SessionStore _store;
    private readonly AnalysisSession _session;
    private readonly string _directory;
    private readonly int _processId;
    private readonly CancellationTokenSource _stop = new();
    private readonly HookCallback _mouseCallback;
    private readonly HookCallback _keyboardCallback;
    private readonly Thread _videoThread;
    private GuidedAudio? _audio;
    private InputJournal? _inputs;
    private nint _mouseHook;
    private nint _keyboardHook;
    private long _lastMouseMove;
    private volatile Exception? _error;
    private bool _disposed;
    private int _frameCount;

    /// <summary>같은 세션의 기존 조각 다음 번호를 사용하도록 녹화기만 준비한다.</summary>
    public GuidedRecorder(SessionStore store, AnalysisSession session)
    {
        _store = store;
        _session = session;
        _directory = Path.Combine(store.SessionDirectory(session.Id), "recording");
        SessionStore.RejectReparse(_directory);
        Directory.CreateDirectory(_directory);
        _processId = session.ProcessId ?? throw new InvalidOperationException("실행 중인 게임 세션이 아닙니다.");
        _mouseCallback = OnMouse;
        _keyboardCallback = OnKeyboard;
        _videoThread = new Thread(CaptureVideo) { IsBackground = true, Name = "NetStorm 분석 영상 캡처" };
    }

    /// <summary>녹화 도중 오류와 누적 프레임 수를 안내 창에 제공한다.</summary>
    public Exception? Error => _error ?? _audio?.Error;
    public int FrameCount => Volatile.Read(ref _frameCount);

    /// <summary>소리·입력·영상을 시작하고 실패 시 부분적으로 만든 리소스를 닫는다.</summary>
    public void Start()
    {
        using Process? process = _store.OwnedProcess(_session);
        if (process == null) throw new InvalidOperationException("게임이 종료되었거나 프로세스가 바뀌었습니다.");
        try
        {
            _audio = new GuidedAudio(_directory);
            _inputs = new InputJournal(_directory, _session.StartedCounter);
            _mouseHook = SetWindowsHookEx(MouseHookType, _mouseCallback, GetModuleHandle(null), 0);
            _keyboardHook = SetWindowsHookEx(KeyboardHookType, _keyboardCallback, GetModuleHandle(null), 0);
            if (_mouseHook == 0 || _keyboardHook == 0) throw new InvalidOperationException("물리 입력 기록용 Windows 훅을 설치하지 못했습니다.");
            _store.Append(_session, "guided_recording_started", new { fps = FramesPerSecond, video = "MJPEG AVI", audio = "기본 출력 장치 루프백", directory = "recording" });
            _videoThread.Start();
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    /// <summary>게임이 보이는 영역만 JPEG로 압축하며 종료 신호 때 AVI 조각을 닫는다.</summary>
    private void CaptureVideo()
    {
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process == null) throw new InvalidOperationException("녹화 중 게임이 종료되었습니다.");
            GameWindow first = WindowsGame.FindWindow(_processId);
            using var video = new GuidedVideo(_directory, first.Width, first.Height, FramesPerSecond);
            ImageCodecInfo codec = ImageCodecInfo.GetImageEncoders().First(c => c.MimeType == "image/jpeg");
            using var encoder = new EncoderParameters(1);
            encoder.Param[0] = new EncoderParameter(System.Drawing.Imaging.Encoder.Quality, 75L);
            var timer = Stopwatch.StartNew();
            long frameNumber = 0;
            // 매 프레임 목표 시각을 기준으로 기다려 캡처·압축에 쓴 시간이 게임 시간에 누적되지 않게 한다.
            while (!_stop.IsCancellationRequested)
            {
                if (process.HasExited) throw new InvalidOperationException("녹화 중 게임이 종료되었습니다.");
                GameWindow window = WindowsGame.FindWindow(_processId);
                if (window.Width != first.Width || window.Height != first.Height)
                    throw new InvalidOperationException("녹화 중 게임 창 크기가 바뀌었습니다. 녹화를 다시 시작하세요.");
                CapturedFrame frame = WindowsGame.CaptureVisible(window, "");
                using var png = new MemoryStream(frame.Png, false);
                using Image image = Image.FromStream(png);
                using var jpeg = new MemoryStream();
                image.Save(jpeg, codec, encoder);
                DateTimeOffset captured = DateTimeOffset.UtcNow;
                double elapsedMs = Stopwatch.GetElapsedTime(_session.StartedCounter).TotalMilliseconds;
                video.WriteFrame(jpeg.ToArray(), captured, elapsedMs);
                Interlocked.Increment(ref _frameCount);
                frameNumber++;
                long remaining = frameNumber * 1000 / FramesPerSecond - timer.ElapsedMilliseconds;
                if (remaining > 0) _stop.Token.WaitHandle.WaitOne((int)Math.Min(remaining, 1000));
            }
        }
        catch (Exception error)
        {
            if (!_stop.IsCancellationRequested) _error = error;
        }
    }

    /// <summary>게임이 전면인 경우에만 마우스 좌표와 버튼·휠을 기록한다.</summary>
    private nint OnMouse(int code, nint message, nint data)
    {
        try
        {
            if (code >= 0 && IsGameForeground())
            {
                int kind = unchecked((int)message);
                if (kind is 0x0200 or 0x0201 or 0x0202 or 0x0204 or 0x0205 or 0x0207 or 0x0208 or 0x020A)
                {
                    long now = Stopwatch.GetTimestamp();
                    if (kind != 0x0200 || now - _lastMouseMove >= MouseMoveIntervalTicks)
                    {
                        if (kind == 0x0200) _lastMouseMove = now;
                        MouseHookData mouse = Marshal.PtrToStructure<MouseHookData>(data);
                        Point point = mouse.Position;
                        GameWindow window = WindowsGame.FindWindow(_processId);
                        ScreenToClient(new nint(window.Handle), ref point);
                        _inputs?.Write(new { type = "mouse", message = kind, x = point.X, y = point.Y,
                            inside = point.X >= 0 && point.Y >= 0 && point.X < window.Width && point.Y < window.Height,
                            wheel = kind == 0x020A ? (short)(mouse.MouseData >> 16) : (short)0 });
                    }
                }
            }
        }
        catch (Exception error) { _error = error; }
        return CallNextHookEx(0, code, message, data);
    }

    /// <summary>게임에 전달되는 키 누름·해제의 가상 키 번호를 기록한다.</summary>
    private nint OnKeyboard(int code, nint message, nint data)
    {
        try
        {
            if (code >= 0 && IsGameForeground())
            {
                int kind = unchecked((int)message);
                if (kind is 0x0100 or 0x0101 or 0x0104 or 0x0105)
                {
                    KeyboardHookData key = Marshal.PtrToStructure<KeyboardHookData>(data);
                    _inputs?.Write(new { type = "keyboard", message = kind, virtualKey = key.VirtualKey, scanCode = key.ScanCode });
                }
            }
        }
        catch (Exception error) { _error = error; }
        return CallNextHookEx(0, code, message, data);
    }

    /// <summary>안내 창 입력과 다른 프로그램 입력을 게임 입력으로 오인하지 않는다.</summary>
    private bool IsGameForeground()
    {
        nint foreground = GetForegroundWindow();
        GetWindowThreadProcessId(foreground, out uint processId);
        return processId == _processId;
    }

    /// <summary>캡처 스레드와 오디오·입력 기록을 닫고 중단 시점을 세션 로그에 남긴다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        if (_mouseHook != 0) UnhookWindowsHookEx(_mouseHook);
        if (_keyboardHook != 0) UnhookWindowsHookEx(_keyboardHook);
        _stop.Cancel();
        if (_videoThread.IsAlive) _videoThread.Join();
        _audio?.Dispose();
        _inputs?.Dispose();
        _stop.Dispose();
        if (_session.EventCount < SessionStore.MaximumEvents)
            _store.Append(_session, "guided_recording_stopped", new { frames = FrameCount, error = Error?.Message });
    }

    /// <summary>Windows 훅 콜백의 원형.</summary>
    private delegate nint HookCallback(int code, nint message, nint data);

    /// <summary>저수준 마우스 훅에서 전달하는 화면 좌표와 휠 값.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct MouseHookData
    {
        public Point Position;
        public uint MouseData;
        public uint Flags;
        public uint Time;
        public nuint Extra;
    }

    /// <summary>저수준 키보드 훅에서 전달하는 키 코드.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct KeyboardHookData
    {
        public uint VirtualKey;
        public uint ScanCode;
        public uint Flags;
        public uint Time;
        public nuint Extra;
    }

    /// <summary>Windows 훅을 설치한다.</summary>
    [DllImport("user32.dll", SetLastError = true)] private static extern nint SetWindowsHookEx(int kind, HookCallback callback, nint module, uint threadId);
    /// <summary>Windows 훅을 제거한다.</summary>
    [DllImport("user32.dll", SetLastError = true)] [return: MarshalAs(UnmanagedType.Bool)] private static extern bool UnhookWindowsHookEx(nint hook);
    /// <summary>현재 훅이 입력을 소비하지 않고 다음 대상에 전달한다.</summary>
    [DllImport("user32.dll")] private static extern nint CallNextHookEx(nint hook, int code, nint message, nint data);
    /// <summary>게임 창이 전면인지 확인한다.</summary>
    [DllImport("user32.dll")] private static extern nint GetForegroundWindow();
    /// <summary>전면 창을 소유한 프로세스 ID를 읽는다.</summary>
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(nint window, out uint processId);
    /// <summary>물리 화면 좌표를 게임 클라이언트 좌표로 바꾼다.</summary>
    [DllImport("user32.dll")] [return: MarshalAs(UnmanagedType.Bool)] private static extern bool ScreenToClient(nint window, ref Point point);
    /// <summary>현재 모듈에서 Windows 훅 콜백 주소를 제공한다.</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern nint GetModuleHandle(string? name);

    /// <summary>많은 입력 이벤트를 4 MB 이하 JSONL 파일로 순서대로 분할한다.</summary>
    private sealed class InputJournal : IDisposable
    {
        private readonly string _directory;
        private readonly long _sessionCounter;
        private StreamWriter? _writer;
        private long _bytes;
        private int _part;

        /// <summary>앞선 기록의 다음 번호를 찾는다.</summary>
        public InputJournal(string directory, long sessionCounter)
        {
            _directory = directory;
            _sessionCounter = sessionCounter;
            _part = Directory.EnumerateFiles(directory, "input-*.jsonl")
                .Select(path => int.TryParse(Path.GetFileNameWithoutExtension(path).AsSpan(6), out int number) ? number : 0)
                .DefaultIfEmpty(0).Max();
        }

        /// <summary>물리 입력과 세션 경과 시간을 같은 JSON 줄에 기록한다.</summary>
        public void Write(object input)
        {
            string line = JsonSerializer.Serialize(new { utc = DateTimeOffset.UtcNow,
                sessionElapsedMs = Stopwatch.GetElapsedTime(_sessionCounter).TotalMilliseconds, input }, SessionStore.Json) + "\n";
            int size = Encoding.UTF8.GetByteCount(line);
            if (_writer == null || _bytes + size > SessionStore.DefaultPartBytes)
            {
                _writer?.Dispose();
                _part++;
                string path = Path.Combine(_directory, $"input-{_part:0000}.jsonl");
                SessionStore.RejectReparse(path);
                _writer = new StreamWriter(new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read), new UTF8Encoding(false));
                _bytes = 0;
            }
            _writer.Write(line);
            _writer.Flush();
            _bytes += size;
        }

        /// <summary>마지막 입력 조각을 닫는다.</summary>
        public void Dispose() => _writer?.Dispose();
    }
}
