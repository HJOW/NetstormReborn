using System.Diagnostics;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

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
    private readonly bool _freePlay;
    private readonly int _processId;
    private readonly CancellationTokenSource _stop = new();
    private readonly HookCallback _mouseCallback;
    private readonly HookCallback _keyboardCallback;
    private readonly Thread _videoThread;
    private readonly Thread _hookThread;
    private readonly ManualResetEventSlim _hookReady = new(false);
    private uint _hookThreadId;
    private GameWindow? _inputWindow;
    private GuidedInputBuffer? _inputBuffer;
    private GuidedAudio? _audio;
    private GuidedInputJournal? _inputs;
    private nint _mouseHook;
    private nint _keyboardHook;
    private long _lastMouseMove;
    private volatile Exception? _error;
    private bool _disposed;
    private int _frameCount;
    private long _startedCounter;

    /// <summary>같은 세션의 기존 조각 다음 번호를 사용하도록 녹화기만 준비한다.</summary>
    public GuidedRecorder(SessionStore store, AnalysisSession session, bool freePlay = false)
    {
        _store = store;
        _session = session;
        _freePlay = freePlay;
        _directory = freePlay ? store.FreeplayDirectory(session.Id) : Path.Combine(store.SessionDirectory(session.Id), "recording");
        SessionStore.RejectReparse(_directory);
        Directory.CreateDirectory(_directory);
        _processId = session.ProcessId ?? throw new InvalidOperationException("실행 중인 게임 세션이 아닙니다.");
        _mouseCallback = OnMouse;
        _keyboardCallback = OnKeyboard;
        _videoThread = new Thread(CaptureVideo) { IsBackground = true, Name = "NetStorm 분석 영상 캡처" };
        _hookThread = new Thread(CaptureInput) { IsBackground = true, Name = "NetStorm 분석 입력 훅" };
    }

    /// <summary>녹화 도중 오류와 누적 프레임 수를 안내 창에 제공한다.</summary>
    public Exception? Error => _error ?? _audio?.Error ?? _inputBuffer?.Error;
    public int FrameCount => Volatile.Read(ref _frameCount);
    /// <summary>현재 녹화 시작 버튼을 누른 뒤 지난 시간을 반환한다.</summary>
    public TimeSpan Elapsed => _startedCounter == 0 ? TimeSpan.Zero : Stopwatch.GetElapsedTime(_startedCounter);

    /// <summary>소리·입력·영상을 시작하고 실패 시 부분적으로 만든 리소스를 닫는다.</summary>
    public void Start()
    {
        using Process? process = _store.OwnedProcess(_session);
        if (process == null) throw new InvalidOperationException("게임이 종료되었거나 프로세스가 바뀌었습니다.");
        try
        {
            _audio = new GuidedAudio(_directory);
            // 안내 창 경과 시간과 입력 로그가 같은 녹화 시작 기준을 사용한다.
            _startedCounter = Stopwatch.GetTimestamp();
            _inputs = new GuidedInputJournal(_directory, _session.StartedCounter, _startedCounter);
            _inputWindow = WindowsGame.FindWindow(_processId, preferForeground: false);
            _inputBuffer = new GuidedInputBuffer((input, utc, counter) => _inputs.Write(input, utc, counter));
            _hookThread.Start();
            if (!_hookReady.Wait(TimeSpan.FromSeconds(5))) throw new TimeoutException("입력 훅 스레드가 시작되지 않았습니다.");
            if (_error != null) throw new InvalidOperationException("물리 입력 기록용 Windows 훅을 설치하지 못했습니다.", _error);
            string directory = Path.GetRelativePath(_store.Repository, _directory).Replace('\\', '/');
            _store.Append(_session, _freePlay ? "freeplay_recording_started" : "guided_recording_started",
                new { fps = FramesPerSecond, video = "MJPEG AVI", audio = "기본 출력 장치 루프백", directory,
                    inputFile = _inputs.FileName, recordingId = _inputs.RecordingId, inputSchemaVersion = GuidedInputJournal.SchemaVersion });
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
                Volatile.Write(ref _inputWindow, window);
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
            // 사용자가 중지한 직후의 AVI 닫기 오류도 성공으로 숨기지 않는다.
            _error = error;
        }
    }

    /// <summary>게임이 전면인 경우에만 마우스 좌표와 버튼·휠을 기록한다.</summary>
    private nint OnMouse(int code, nint message, nint data)
    {
        try
        {
            // 좌표 조회 전에 시각을 읽어 후속 처리 지연과 조작 타이밍을 구분한다.
            long counter = Stopwatch.GetTimestamp();
            DateTimeOffset utc = DateTimeOffset.UtcNow;
            if (code >= 0 && IsGameForeground())
            {
                int kind = unchecked((int)message);
                var inputMessage = (GuidedInputMessage)kind;
                if (inputMessage is GuidedInputMessage.MouseMove or GuidedInputMessage.LeftDown or GuidedInputMessage.LeftUp
                    or GuidedInputMessage.RightDown or GuidedInputMessage.RightUp or GuidedInputMessage.MiddleDown or GuidedInputMessage.MiddleUp
                    or GuidedInputMessage.Wheel or GuidedInputMessage.ExtraDown or GuidedInputMessage.ExtraUp or GuidedInputMessage.HorizontalWheel)
                {
                    if (inputMessage != GuidedInputMessage.MouseMove || counter - _lastMouseMove >= MouseMoveIntervalTicks)
                    {
                        if (inputMessage == GuidedInputMessage.MouseMove) _lastMouseMove = counter;
                        MouseHookData mouse = Marshal.PtrToStructure<MouseHookData>(data);
                        // 창 탐색·제목 조회 없이 이미 찾은 창의 현재 물리 좌표만 읽는다.
                        GameWindow window = WindowsGame.ReadInputWindow(Volatile.Read(ref _inputWindow)!);
                        GuidedInput? input = GuidedInput.Mouse(kind, mouse.Position, window, mouse.MouseData, mouse.Flags, mouse.Time);
                        if (input != null) _inputBuffer?.TryWrite(input, utc, counter);
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
            // 키 누름·해제 시각은 파일 쓰기보다 앞서 고정한다.
            long counter = Stopwatch.GetTimestamp();
            DateTimeOffset utc = DateTimeOffset.UtcNow;
            if (code >= 0 && IsGameForeground())
            {
                int kind = unchecked((int)message);
                KeyboardHookData key = Marshal.PtrToStructure<KeyboardHookData>(data);
                GuidedInput? input = GuidedInput.Keyboard(kind, key.VirtualKey, key.ScanCode, key.Flags, key.Time);
                if (input != null) _inputBuffer?.TryWrite(input, utc, counter);
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

    /// <summary>UI 타이머·창 재배치가 저수준 입력을 막지 않게 전용 메시지 루프에서 훅을 실행한다.</summary>
    private void CaptureInput()
    {
        try
        {
            WindowsGame.SetDpiMode();
            _hookThreadId = GetCurrentThreadId();
            // 종료 메시지를 보낼 수 있도록 훅 설치 전에 스레드 메시지 큐를 만든다.
            PeekMessage(out _, 0, 0, 0, 0);
            _mouseHook = SetWindowsHookEx(MouseHookType, _mouseCallback, GetModuleHandle(null), 0);
            _keyboardHook = SetWindowsHookEx(KeyboardHookType, _keyboardCallback, GetModuleHandle(null), 0);
            if (_mouseHook == 0 || _keyboardHook == 0) throw new InvalidOperationException("물리 입력 기록용 Windows 훅을 설치하지 못했습니다.");
            _hookReady.Set();
            // WM_QUIT 또는 녹화 종료 전까지 입력 콜백을 즉시 처리한다.
            while (!_stop.IsCancellationRequested)
            {
                int result = GetMessage(out NativeMessage message, 0, 0, 0);
                if (result == 0) break;
                if (result < 0) throw new InvalidOperationException("입력 훅 메시지 조회 실패.");
                TranslateMessage(ref message);
                DispatchMessage(ref message);
            }
        }
        catch (Exception error) { _error = error; }
        finally
        {
            if (_mouseHook != 0) UnhookWindowsHookEx(_mouseHook);
            if (_keyboardHook != 0) UnhookWindowsHookEx(_keyboardHook);
            _hookReady.Set();
        }
    }

    /// <summary>캡처 스레드와 오디오·입력 기록을 닫고 중단 시점을 세션 로그에 남긴다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _stop.Cancel();
        if (_hookThread.IsAlive)
        {
            // WM_QUIT로 대기 중인 메시지 루프를 깨우고 훅 해제까지 기다린다.
            PostThreadMessage(_hookThreadId, 0x0012, 0, 0);
            _hookThread.Join();
        }
        if (_videoThread.IsAlive) _videoThread.Join();
        // 어느 출력 파일 하나를 닫는 데 실패해도 나머지 파일과 취소 신호는 끝까지 정리한다.
        try { _audio?.Dispose(); }
        catch (Exception error) { _error ??= error; }
        // 수락한 입력을 먼저 모두 저장하고 마지막 중단 경계는 그 뒤에 쓴다.
        _inputBuffer?.Dispose();
        try { _inputs?.Dispose(); }
        catch (Exception error) { _error ??= error; }
        _stop.Dispose();
        _hookReady.Dispose();
        if (_session.EventCount < SessionStore.MaximumEvents)
        {
            try
            {
                _store.Append(_session, _freePlay ? "freeplay_recording_stopped" : "guided_recording_stopped",
                    new { frames = FrameCount, error = Error?.Message, recordingId = _inputs?.RecordingId });
            }
            catch (Exception error) { _error ??= error; }
        }
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

    /// <summary>32·64비트 Windows MSG 구조체의 포인터 정렬을 보존한다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeMessage
    {
        public nint Window;
        public uint Message;
        public nuint WParam;
        public nint LParam;
        public uint Time;
        public Point Position;
        public uint Private;
    }

    /// <summary>훅을 설치한 스레드의 종료 메시지 대상을 읽는다.</summary>
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    /// <summary>전용 스레드에 메시지 큐를 만들고 초기 메시지를 조회한다.</summary>
    [DllImport("user32.dll", EntryPoint = "PeekMessageW")] private static extern bool PeekMessage(out NativeMessage message, nint window, uint minimum, uint maximum, uint remove);
    /// <summary>입력 또는 종료 메시지를 전용 스레드에서 기다린다.</summary>
    [DllImport("user32.dll", EntryPoint = "GetMessageW")] private static extern int GetMessage(out NativeMessage message, nint window, uint minimum, uint maximum);
    /// <summary>네이티브 메시지 루프의 키 메시지를 변환한다.</summary>
    [DllImport("user32.dll")] private static extern bool TranslateMessage(ref NativeMessage message);
    /// <summary>네이티브 메시지를 대상 창에 전달한다.</summary>
    [DllImport("user32.dll", EntryPoint = "DispatchMessageW")] private static extern nint DispatchMessage(ref NativeMessage message);
    /// <summary>중단 시 훅 스레드의 메시지 대기를 깨운다.</summary>
    [DllImport("user32.dll", EntryPoint = "PostThreadMessageW")] private static extern bool PostThreadMessage(uint thread, uint message, nuint wParam, nint lParam);

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
    /// <summary>현재 모듈에서 Windows 훅 콜백 주소를 제공한다.</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern nint GetModuleHandle(string? name);

}
