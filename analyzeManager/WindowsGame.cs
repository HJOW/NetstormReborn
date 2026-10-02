using System.Diagnostics;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;

namespace Netstorm.AnalyzeManager;

/// <summary>게임 클라이언트 영역의 물리 픽셀 좌표와 창 상태.</summary>
public sealed record GameWindow(long Handle, string Title, int X, int Y, int Width, int Height, bool Foreground);

/// <summary>관찰할 클라이언트 영역. 빈 문자열은 전체 화면을 뜻한다.</summary>
public sealed record CaptureRegion(int X, int Y, int Width, int Height)
{
    /// <summary>AI가 전달한 x,y,width,height를 파싱하고 창 밖 좌표를 거부한다.</summary>
    public static CaptureRegion Parse(string region, GameWindow window)
    {
        if (string.IsNullOrWhiteSpace(region)) return new(0, 0, window.Width, window.Height);
        int[] values = region.Split(',').Select(int.Parse).ToArray();
        if (values.Length != 4 || values[0] < 0 || values[1] < 0 || values[2] <= 0 || values[3] <= 0
            || (long)values[0] + values[2] > window.Width || (long)values[1] + values[3] > window.Height)
            throw new ArgumentException("region은 창 안의 x,y,width,height여야 합니다.");
        return new(values[0], values[1], values[2], values[3]);
    }
}

/// <summary>
/// 비교용 RGB 픽셀과 증거용 PNG를 함께 보관하는 한 프레임.
/// Method는 캡처 방식이다: screen(화면 복사), wine-window-dc·wine-printwindow(Wine 창 복사), wine-black(Wine 방식 모두 검은색).
/// </summary>
public sealed record CapturedFrame(byte[] Png, byte[] Pixels, CaptureRegion Region, GameWindow Window, string Method = "screen")
{
    /// <summary>RGB 중 하나라도 임계값 이상 달라진 픽셀 비율을 계산한다.</summary>
    public double Difference(CapturedFrame other)
    {
        if (Region != other.Region || Pixels.Length != other.Pixels.Length) return 1;
        int changed = 0;
        // 미세한 디더링은 무시하고 실제 픽셀의 색상 차이를 센다.
        for (int i = 0; i < Pixels.Length; i += 3)
        {
            if (Math.Abs(Pixels[i] - other.Pixels[i]) >= 24
                || Math.Abs(Pixels[i + 1] - other.Pixels[i + 1]) >= 24
                || Math.Abs(Pixels[i + 2] - other.Pixels[i + 2]) >= 24) changed++;
        }
        return (double)changed / (Pixels.Length / 3);
    }
}

/// <summary>특정 원본 게임 프로세스에만 입력을 보내고 실제 표시된 클라이언트 영역을 캡처한다.</summary>
public static class WindowsGame
{
    /// <summary>창 복원 명령(SW_RESTORE).</summary>
    private const int RestoreWindow = 9;
    /// <summary>클라이언트 좌표 탐색의 최상위 창(GA_ROOT).</summary>
    private const uint RootWindow = 2;
    /// <summary>게임 종료 요청(WM_CLOSE). 확인 창이 남으면 강제 종료를 별도로 선택한다.</summary>
    private const uint CloseMessage = 0x10;
    /// <summary>입력 구조체의 키보드 종류(INPUT_KEYBOARD).</summary>
    private const uint KeyboardInput = 1;
    /// <summary>키 해제 플래그(KEYEVENTF_KEYUP).</summary>
    private const uint KeyUp = 2;
    /// <summary>원본 픽셀을 그대로 복사하는 래스터 연산(SRCCOPY).</summary>
    private const uint SourceCopy = 0x00CC0020;
    /// <summary>PrintWindow에서 클라이언트 영역만 그리는 플래그(PW_CLIENTONLY).</summary>
    private const uint PrintClientOnly = 1;

    /// <summary>부모 호스트가 dotnet.exe여도 현재 스레드의 좌표를 물리 픽셀로 고정한다.</summary>
    public static void SetDpiMode() => SetThreadDpiAwarenessContext(new nint(-4));

    /// <summary>게임의 활성 대화상자 또는 가장 큰 표시 창을 찾아 클라이언트 좌표를 읽는다. 안내 창 배치는 주 게임 창을 고른다.</summary>
    public static GameWindow FindWindow(int processId, bool preferForeground = true)
    {
        SetDpiMode();
        nint foreground = GetForegroundWindow();
        nint chosen = 0;
        long largest = 0;
        // 같은 프로세스의 표시 가능한 최상위 창만 후보로 삼는다.
        EnumWindows((handle, _) =>
        {
            GetWindowThreadProcessId(handle, out uint pid);
            if (pid != processId || !IsWindowVisible(handle) || !GetClientRect(handle, out Rect rect)) return true;
            long area = (long)rect.Right * rect.Bottom;
            if (area <= 0) return true;
            if (preferForeground && handle == foreground) { chosen = handle; return false; }
            if (area > largest) { largest = area; chosen = handle; }
            return true;
        }, 0);
        if (chosen == 0) throw new InvalidOperationException("게임의 표시 가능한 창을 찾지 못했습니다.");
        if (!GetClientRect(chosen, out Rect client)) throw new InvalidOperationException("클라이언트 영역 조회 실패.");
        var origin = new Point();
        if (!ClientToScreen(chosen, ref origin)) throw new InvalidOperationException("창 좌표 변환 실패.");
        var title = new StringBuilder(512);
        GetWindowText(chosen, title, title.Capacity);
        return new(chosen.ToInt64(), title.ToString(), origin.X, origin.Y, client.Right, client.Bottom, chosen == foreground);
    }

    /// <summary>입력 훅에서는 창 열거·제목 조회를 생략하고 현재 클라이언트 원점과 크기만 갱신한다.</summary>
    public static GameWindow ReadInputWindow(GameWindow known)
    {
        nint handle = new(known.Handle);
        var origin = new Point();
        if (!GetClientRect(handle, out Rect client) || !ClientToScreen(handle, ref origin))
            throw new InvalidOperationException("입력 기록 중 게임 창 좌표 조회 실패.");
        return known with { X = origin.X, Y = origin.Y, Width = client.Right, Height = client.Bottom, Foreground = true };
    }

    /// <summary>안내 창 배치를 위해 게임의 제목 표시줄과 테두리까지 포함한 화면 좌표를 읽는다.</summary>
    public static Rectangle OuterBounds(GameWindow window)
    {
        SetDpiMode();
        if (!GetWindowRect(new nint(window.Handle), out Rect rect)) throw new InvalidOperationException("게임 창 바깥 좌표 조회 실패.");
        return Rectangle.FromLTRB(rect.Left, rect.Top, rect.Right, rect.Bottom);
    }

    /// <summary>대상 창을 전면에 놓고 실제 전면 창이 되었는지 확인한다.</summary>
    public static async Task<GameWindow> FocusAsync(int processId, CancellationToken cancellation)
    {
        GameWindow window = FindWindow(processId);
        // 백그라운드 프로세스의 전면 전환은 Windows가 거부할 수 있으므로 단계별 방법을 차례로 시도한다.
        for (int attempt = 0; attempt < 3 && (!window.Foreground || IsIconic(new nint(window.Handle))); attempt++)
        {
            nint handle = new(window.Handle);
            ShowWindow(handle, RestoreWindow);
            if (attempt == 0) SetForegroundWindow(handle);
            else if (attempt == 1) ForceForegroundByAttach(handle);
            else ForceForegroundByAltKey(handle);
            await Task.Delay(180, cancellation);
            window = FindWindow(processId);
        }
        if (!window.Foreground) throw new InvalidOperationException("게임 창의 포커스를 얻지 못했습니다. 입력을 보내지 않았습니다.");
        return window;
    }

    /// <summary>
    /// 현재 전면 창의 입력 스레드에 잠시 연결해 전면 전환 제한을 우회한다.
    /// 연결은 반드시 해제하며 대상 창 외의 창에는 입력을 보내지 않는다.
    /// </summary>
    private static void ForceForegroundByAttach(nint handle)
    {
        uint foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), out _);
        uint currentThread = GetCurrentThreadId();
        bool attached = foregroundThread != 0 && foregroundThread != currentThread
            && AttachThreadInput(currentThread, foregroundThread, true);
        try
        {
            BringWindowToTop(handle);
            SetForegroundWindow(handle);
        }
        finally
        {
            if (attached) AttachThreadInput(currentThread, foregroundThread, false);
        }
    }

    /// <summary>
    /// Alt 키를 눌렀다 떼는 신호를 먼저 보내 "마지막 입력 프로세스" 조건을 만족시킨 뒤 전면 전환한다.
    /// Alt 단독 입력은 게임에 전달되기 전에 전환이 일어나므로 게임 메뉴 조작으로 이어지지 않는다.
    /// </summary>
    private static void ForceForegroundByAltKey(nint handle)
    {
        SendUnchecked(new Input { Type = KeyboardInput, Data = new InputData { Keyboard = new Keyboard { VirtualKey = (ushort)Keys.Menu } } });
        SendUnchecked(new Input { Type = KeyboardInput, Data = new InputData { Keyboard = new Keyboard { VirtualKey = (ushort)Keys.Menu, Flags = KeyUp } } });
        SetForegroundWindow(handle);
    }

    /// <summary>가려지지 않은 실제 화면을 캡처한다. DirectDraw의 빈 PrintWindow 결과를 사용하지 않는다.</summary>
    public static CapturedFrame Capture(GameWindow window, string region, bool includePng = true) => CaptureVisible(window, region, true, includePng);

    /// <summary>안내 창을 조작하는 동안에도 가리지 않은 게임 화면을 기록한다.</summary>
    public static CapturedFrame CaptureVisible(GameWindow window, string region, bool requireForeground = false, bool includePng = true)
    {
        using Bitmap bitmap = CaptureBitmapVisible(window, region, requireForeground, false, out CaptureRegion roi, out string method);
        CapturedFrame frame = ToFrame(bitmap, roi, window, method, includePng);
        if (requireForeground) CheckForeground(window);
        return frame;
    }

    /// <summary>녹화 프레임을 곧바로 JPEG로 압축해 PNG 인코딩·디코딩과 비교용 픽셀 복사를 생략한다.</summary>
    public static byte[] CaptureJpegVisible(GameWindow window, ImageCodecInfo codec, EncoderParameters encoder)
    {
        using Bitmap bitmap = CaptureBitmapVisible(window, "", false, true, out _, out _);
        using var stream = new MemoryStream();
        bitmap.Save(stream, codec, encoder);
        return stream.ToArray();
    }

    /// <summary>같은 게임의 대화상자는 영상에 포함하고 다른 프로그램에 가려진 화면은 거부한다.</summary>
    private static Bitmap CaptureBitmapVisible(GameWindow window, string region, bool requireForeground, bool allowGameDialogs,
        out CaptureRegion roi, out string method)
    {
        SetDpiMode();
        roi = CaptureRegion.Parse(region, window);
        var screenRect = new Rectangle(window.X + roi.X, window.Y + roi.Y, roi.Width, roi.Height);
        if (!SystemInformation.VirtualScreen.Contains(screenRect) || roi.Width > 4096 || roi.Height > 4096)
            throw new InvalidOperationException("캡처 영역이 화면 밖에 있거나 4096픽셀 한도를 넘습니다.");
        if (requireForeground) CheckForeground(window);
        // 표본 위치가 다른 창에 덮였으면 다른 프로그램의 화면을 증거로 저장하지 않는다.
        foreach (Point point in new[] { screenRect.Location,
            new Point(screenRect.Right - 1, screenRect.Bottom - 1),
            new Point(screenRect.Left + roi.Width / 2, screenRect.Top + roi.Height / 2) })
        {
            nint covering = GetAncestor(WindowFromPoint(point), RootWindow);
            GetWindowThreadProcessId(covering, out uint coveringProcess);
            GetWindowThreadProcessId(new nint(window.Handle), out uint gameProcess);
            if (covering.ToInt64() != window.Handle && (!allowGameDialogs || coveringProcess != gameProcess))
                throw new InvalidOperationException("게임 캡처 영역이 다른 창에 가려져 있습니다.");
        }
        if (IsWine) return CaptureWineBitmap(window, roi, out method);
        method = "screen";
        return CaptureScreenBitmap(roi, screenRect);
    }

    /// <summary>Windows: 화면에 실제 표시된 픽셀을 복사한다.</summary>
    private static Bitmap CaptureScreenBitmap(CaptureRegion roi, Rectangle screenRect)
    {
        var bitmap = new Bitmap(roi.Width, roi.Height, PixelFormat.Format24bppRgb);
        try
        {
            using Graphics graphics = Graphics.FromImage(bitmap);
            graphics.CopyFromScreen(screenRect.Location, Point.Empty, screenRect.Size, CopyPixelOperation.SourceCopy);
            return bitmap;
        }
        catch { bitmap.Dispose(); throw; }
    }

    /// <summary>
    /// Wine: 화면 전체 DC는 각 창의 그림을 합치지 않아 항상 검게 나온다(2026-09-29 확인).
    /// 대신 게임 창 자체 DC를 복사하고, 그 결과가 전부 검으면 PrintWindow로 다시 시도한다.
    /// </summary>
    private static Bitmap CaptureWineBitmap(GameWindow window, CaptureRegion roi, out string method)
    {
        nint handle = new(window.Handle);
        var bitmap = new Bitmap(roi.Width, roi.Height, PixelFormat.Format24bppRgb);
        Bitmap? cropped = null;
        try
        {
            nint source = GetDC(handle);
            if (source == 0) throw new InvalidOperationException("게임 창 DC를 얻지 못했습니다.");
            try
            {
                using Graphics graphics = Graphics.FromImage(bitmap);
                nint target = graphics.GetHdc();
                try { BitBlt(target, 0, 0, roi.Width, roi.Height, source, roi.X, roi.Y, SourceCopy); }
                finally { graphics.ReleaseHdc(target); }
            }
            finally { ReleaseDC(handle, source); }
            method = "wine-window-dc";
            if (!IsBlack(ReadPixels(bitmap))) return bitmap;

            // 창 DC가 비어 있으면 클라이언트 전체를 PrintWindow로 그린 뒤 관심 영역만 잘라낸다.
            using var full = new Bitmap(window.Width, window.Height, PixelFormat.Format24bppRgb);
            using (Graphics graphics = Graphics.FromImage(full))
            {
                nint target = graphics.GetHdc();
                try { PrintWindow(handle, target, PrintClientOnly); }
                finally { graphics.ReleaseHdc(target); }
            }
            cropped = full.Clone(new Rectangle(roi.X, roi.Y, roi.Width, roi.Height), PixelFormat.Format24bppRgb);
            // 두 방식 모두 검으면 실제 검은 화면일 수도 있으므로 오류 대신 표시만 남긴다.
            if (IsBlack(ReadPixels(cropped)))
            {
                cropped.Dispose();
                method = "wine-black";
                return bitmap;
            }
            bitmap.Dispose();
            method = "wine-printwindow";
            return cropped;
        }
        catch { cropped?.Dispose(); bitmap.Dispose(); throw; }
    }

    /// <summary>모든 RGB 바이트가 0인지 검사한다.</summary>
    private static bool IsBlack(byte[] pixels) => !pixels.AsSpan().ContainsAnyExcept((byte)0);

    /// <summary>24비트 비트맵에서 비교용 RGB 바이트와 증거용 PNG를 만든다.</summary>
    private static CapturedFrame ToFrame(Bitmap bitmap, CaptureRegion roi, GameWindow window, string method, bool includePng)
    {
        byte[] pixels = ReadPixels(bitmap);
        if (!includePng) return new([], pixels, roi, window, method);
        using var stream = new MemoryStream();
        bitmap.Save(stream, ImageFormat.Png);
        return new(stream.ToArray(), pixels, roi, window, method);
    }

    /// <summary>프레임 비교 중에는 압축을 생략하고 24비트 BGR 픽셀만 읽는다.</summary>
    private static byte[] ReadPixels(Bitmap bitmap)
    {
        byte[] pixels = new byte[bitmap.Width * bitmap.Height * 3];
        BitmapData data = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
        try
        {
            // 행 패딩을 빼서 영역 비교에 쓸 RGB 바이트만 보관한다.
            for (int y = 0; y < bitmap.Height; y++)
                Marshal.Copy(data.Scan0 + y * data.Stride, pixels, y * bitmap.Width * 3, bitmap.Width * 3);
        }
        finally { bitmap.UnlockBits(data); }
        return pixels;
    }

    /// <summary>판정에 사용한 정확한 픽셀을 PNG로 만들어 마지막 증거가 다음 화면으로 바뀌지 않게 한다.</summary>
    public static CapturedFrame EncodePng(CapturedFrame frame)
    {
        if (frame.Png.Length > 0) return frame;
        using var bitmap = new Bitmap(frame.Region.Width, frame.Region.Height, PixelFormat.Format24bppRgb);
        BitmapData data = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
        try
        {
            // 저장된 각 행의 BGR 바이트를 비트맵의 행 패딩을 건너뛰며 복원한다.
            for (int y = 0; y < bitmap.Height; y++)
                Marshal.Copy(frame.Pixels, y * bitmap.Width * 3, data.Scan0 + y * data.Stride, bitmap.Width * 3);
        }
        finally { bitmap.UnlockBits(data); }
        using var stream = new MemoryStream();
        bitmap.Save(stream, ImageFormat.Png);
        return frame with { Png = stream.ToArray() };
    }

    /// <summary>Wine에서 실행 중인지 여부. Wine의 ntdll만 wine_get_version을 내보낸다.</summary>
    public static bool IsWine { get; } = GetProcAddress(GetModuleHandle("ntdll.dll"), "wine_get_version") != 0;

    /// <summary>전면 창이 잠시 바뀌었을 때 다시 돌아오기를 기다리는 최대 시간(ms).</summary>
    private const int ForegroundGraceMs = 200;

    /// <summary>
    /// 입력 직전 대상 창을 다시 검사해 다른 창에 잘못 보내는 일을 줄인다.
    /// 전환 중 잠시 전면 창이 없거나 바뀌는 경우가 있어 짧게 다시 확인하고, 끝내 다르면 그 창의 정보를 오류에 남긴다.
    /// </summary>
    private static void CheckForeground(GameWindow window)
    {
        var timer = Stopwatch.StartNew();
        nint current = GetForegroundWindow();
        // 대상 창이 전면으로 돌아올 때까지 10ms 간격으로 짧게 다시 확인한다.
        while (current.ToInt64() != window.Handle && timer.ElapsedMilliseconds < ForegroundGraceMs)
        {
            Thread.Sleep(10);
            current = GetForegroundWindow();
        }
        if (current.ToInt64() == window.Handle) return;
        GetWindowThreadProcessId(current, out uint pid);
        var title = new StringBuilder(256);
        GetWindowText(current, title, title.Capacity);
        throw new InvalidOperationException(
            $"게임 창의 포커스가 바뀌었습니다(현재 전면: handle={current.ToInt64()}, pid={pid}, 제목='{title}'). 작업을 중단합니다.");
    }

    /// <summary>지원하는 키 이름을 Win32 가상 키로 변환한다.</summary>
    public static ushort[] ParseKeys(string text)
    {
        string[] names = text.Split('+', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        if (names.Length is < 1 or > 4) throw new ArgumentException("key는 ENTER, ESCAPE, CTRL+A 같은 1~4개 키입니다.");
        // 별칭을 포함해 허용된 키만 선택한다. 전체화면 단축키는 분석 지침에 따라 제외한다.
        ushort[] keys = names.Select(name => name.ToUpperInvariant() switch
        {
            "CTRL" or "CONTROL" => (ushort)Keys.ControlKey,
            "SHIFT" => (ushort)Keys.ShiftKey,
            "ALT" => (ushort)Keys.Menu,
            "ESC" or "ESCAPE" => (ushort)Keys.Escape,
            "ENTER" => (ushort)Keys.Enter,
            "SPACE" => (ushort)Keys.Space,
            "TAB" => (ushort)Keys.Tab,
            "BACKSPACE" => (ushort)Keys.Back,
            "LEFT" => (ushort)Keys.Left, "RIGHT" => (ushort)Keys.Right,
            "UP" => (ushort)Keys.Up, "DOWN" => (ushort)Keys.Down,
            "HOME" => (ushort)Keys.Home, "END" => (ushort)Keys.End,
            "PAGEUP" => (ushort)Keys.PageUp, "PAGEDOWN" => (ushort)Keys.PageDown,
            string n when n.Length == 1 && char.IsAsciiLetterOrDigit(n[0]) => (ushort)n[0],
            string n when n.StartsWith('F') && int.TryParse(n.AsSpan(1), out int f) && f is >= 1 and <= 12 => (ushort)(0x6f + f),
            _ => throw new ArgumentException($"지원하지 않는 키: {name}"),
        }).Distinct().ToArray();
        if (keys.Contains((ushort)Keys.F11) || (keys.Contains((ushort)Keys.Menu) && keys.Contains((ushort)Keys.Enter)))
            throw new ArgumentException("원본 분석에서는 전체화면 전환 단축키를 사용할 수 없습니다.");
        return keys;
    }

    /// <summary>마우스와 키보드를 조작하며, 취소·오류 때도 눌린 입력을 반드시 해제한다.</summary>
    public static async Task InputAsync(GameWindow window, InputRequest action, CancellationToken cancellation)
    {
        if (action.Kind == "key")
        {
            ushort[] keys = ParseKeys(action.Key);
            var pressed = new List<ushort>();
            try
            {
                // 조합키는 지정 순서대로 누르고 역순으로 해제한다.
                foreach (ushort key in keys)
                {
                    CheckForeground(window);
                    Send(new Input { Type = KeyboardInput, Data = new InputData { Keyboard = new Keyboard { VirtualKey = key } } });
                    pressed.Add(key);
                }
                await Task.Delay(action.DurationMs, cancellation);
            }
            finally
            {
                // 취소되더라도 수정 키가 OS에 눌린 채 남지 않도록 해제한다.
                foreach (ushort key in pressed.AsEnumerable().Reverse())
                    SendUnchecked(new Input { Type = KeyboardInput, Data = new InputData { Keyboard = new Keyboard { VirtualKey = key, Flags = KeyUp } } });
            }
            return;
        }
        Move(window, action.X, action.Y);
        if (action.Kind == "move") return;
        (uint down, uint up) = action.Button switch
        {
            "left" => (2u, 4u), "right" => (8u, 16u), "middle" => (32u, 64u),
            _ => throw new ArgumentException("button은 left, right, middle 중 하나여야 합니다."),
        };
        CheckForeground(window);
        Send(new Input { Data = new InputData { Mouse = new Mouse { Flags = down } } });
        try
        {
            if (action.Kind == "drag")
            {
                // 원본이 이동 중 상태를 갱신할 수 있도록 드래그를 여러 단계로 나눈다.
                for (int step = 1; step <= 10; step++)
                {
                    await Task.Delay(Math.Max(1, action.DurationMs / 10), cancellation);
                    Move(window, action.X + (action.ToX - action.X) * step / 10, action.Y + (action.ToY - action.Y) * step / 10);
                }
            }
            else await Task.Delay(action.DurationMs, cancellation);
        }
        finally { SendUnchecked(new Input { Data = new InputData { Mouse = new Mouse { Flags = up } } }); }
    }

    /// <summary>게임 클라이언트 밖으로 이동하는 요청을 거부한다.</summary>
    private static void Move(GameWindow window, int x, int y)
    {
        SetDpiMode();
        CheckForeground(window);
        if (x < 0 || y < 0 || x >= window.Width || y >= window.Height)
            throw new ArgumentException("마우스 좌표가 게임 클라이언트 밖입니다.");
        var point = new Point(window.X + x, window.Y + y);
        if (GetAncestor(WindowFromPoint(point), RootWindow).ToInt64() != window.Handle)
            throw new InvalidOperationException("입력 대상 위치가 다른 창에 가려져 있습니다.");
        if (!SetCursorPos(point.X, point.Y)) throw new InvalidOperationException("마우스 위치 설정 실패.");
    }

    /// <summary>OS가 입력을 받아들였는지 검사한다.</summary>
    private static void Send(Input input)
    {
        if (SendUnchecked(input) != 1) throw new InvalidOperationException("SendInput 실패. 게임과 도구의 실행 권한을 확인하세요.");
    }

    /// <summary>해제용 입력은 원래 예외를 가리지 않게 성공 여부만 돌려준다.</summary>
    private static uint SendUnchecked(Input input) => SendInput(1, [input], Marshal.SizeOf<Input>());

    /// <summary>소유 창에만 종료 메시지를 보낸다.</summary>
    public static void RequestClose(GameWindow window) => PostMessage(new nint(window.Handle), CloseMessage, 0, 0);

    /// <summary>Win32 RECT와 같은 메모리 배치.</summary>
    [StructLayout(LayoutKind.Sequential)] private struct Rect { public int Left, Top, Right, Bottom; }
    /// <summary>Win32 INPUT의 플랫폼별 정렬을 보존한다.</summary>
    [StructLayout(LayoutKind.Sequential)] private struct Input { public uint Type; public InputData Data; }
    /// <summary>마우스/키보드 입력의 공용체.</summary>
    [StructLayout(LayoutKind.Explicit)] private struct InputData
    {
        [FieldOffset(0)] public Mouse Mouse;
        [FieldOffset(0)] public Keyboard Keyboard;
    }
    /// <summary>Win32 MOUSEINPUT.</summary>
    [StructLayout(LayoutKind.Sequential)] private struct Mouse { public int X, Y; public uint Data, Flags, Time; public nuint Extra; }
    /// <summary>Win32 KEYBDINPUT.</summary>
    [StructLayout(LayoutKind.Sequential)] private struct Keyboard { public ushort VirtualKey, Scan; public uint Flags, Time; public nuint Extra; }
    /// <summary>최상위 창을 열거하는 콜백.</summary>
    private delegate bool EnumWindowCallback(nint window, nint parameter);
    /// <summary>최상위 창 열거.</summary>
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindowCallback callback, nint parameter);
    /// <summary>창의 소유 프로세스 번호 조회.</summary>
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(nint window, out uint processId);
    /// <summary>클라이언트 크기 조회.</summary>
    [DllImport("user32.dll")] private static extern bool GetClientRect(nint window, out Rect rect);
    /// <summary>제목 표시줄과 테두리를 포함한 창 좌표 조회.</summary>
    [DllImport("user32.dll")] private static extern bool GetWindowRect(nint window, out Rect rect);
    /// <summary>클라이언트 좌표의 화면 좌표 변환.</summary>
    [DllImport("user32.dll")] private static extern bool ClientToScreen(nint window, ref Point point);
    /// <summary>창 제목 조회.</summary>
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetWindowText(nint window, StringBuilder text, int count);
    /// <summary>창 표시 여부 조회.</summary>
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(nint window);
    /// <summary>창 최소화 여부 조회.</summary>
    [DllImport("user32.dll")] private static extern bool IsIconic(nint window);
    /// <summary>현재 전면 창 조회.</summary>
    [DllImport("user32.dll")] private static extern nint GetForegroundWindow();
    /// <summary>전면 창 전환 요청.</summary>
    [DllImport("user32.dll")] private static extern bool SetForegroundWindow(nint window);
    /// <summary>창을 Z 순서 맨 위로 올림.</summary>
    [DllImport("user32.dll")] private static extern bool BringWindowToTop(nint window);
    /// <summary>두 스레드의 입력 상태 연결/해제 (전면 전환 제한 우회용).</summary>
    [DllImport("user32.dll")] private static extern bool AttachThreadInput(uint attach, uint attachTo, bool connect);
    /// <summary>현재 스레드 번호.</summary>
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    /// <summary>창 표시 상태 변경.</summary>
    [DllImport("user32.dll")] private static extern bool ShowWindow(nint window, int command);
    /// <summary>화면 좌표에 표시된 창 조회.</summary>
    [DllImport("user32.dll")] private static extern nint WindowFromPoint(Point point);
    /// <summary>자식 창의 최상위 창 조회.</summary>
    [DllImport("user32.dll")] private static extern nint GetAncestor(nint window, uint flags);
    /// <summary>마우스 커서 위치 변경.</summary>
    [DllImport("user32.dll")] private static extern bool SetCursorPos(int x, int y);
    /// <summary>마우스/키보드 입력 주입.</summary>
    [DllImport("user32.dll", SetLastError = true)] private static extern uint SendInput(uint count, Input[] inputs, int size);
    /// <summary>종료 메시지 비동기 전달.</summary>
    [DllImport("user32.dll")] private static extern bool PostMessage(nint window, uint message, nint wparam, nint lparam);
    /// <summary>스레드 DPI 좌표계 설정.</summary>
    [DllImport("user32.dll")] private static extern nint SetThreadDpiAwarenessContext(nint value);
    /// <summary>창 클라이언트 DC 조회 (Wine 캡처용).</summary>
    [DllImport("user32.dll")] private static extern nint GetDC(nint window);
    /// <summary>GetDC로 얻은 DC 반환.</summary>
    [DllImport("user32.dll")] private static extern int ReleaseDC(nint window, nint dc);
    /// <summary>창 내용을 지정 DC에 그리게 함 (Wine 캡처 대체 경로).</summary>
    [DllImport("user32.dll")] private static extern bool PrintWindow(nint window, nint dc, uint flags);
    /// <summary>DC 간 픽셀 복사.</summary>
    [DllImport("gdi32.dll")] private static extern bool BitBlt(nint target, int x, int y, int width, int height, nint source, int sourceX, int sourceY, uint operation);
    /// <summary>로드된 모듈 핸들 조회 (Wine 감지용).</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern nint GetModuleHandle(string name);
    /// <summary>모듈의 내보낸 함수 주소 조회 (Wine 감지용).</summary>
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)] private static extern nint GetProcAddress(nint module, string name);
}

/// <summary>한 번의 제한된 입력 동작. 좌표는 캡처 영역이 아닌 전체 클라이언트 기준이다.</summary>
public sealed record InputRequest(string Kind, int X = 0, int Y = 0, int ToX = 0, int ToY = 0,
    string Button = "left", string Key = "", int DurationMs = 80, int SettleMs = 300);
