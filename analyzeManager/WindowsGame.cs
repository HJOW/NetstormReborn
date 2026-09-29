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

/// <summary>비교용 RGB 픽셀과 증거용 PNG를 함께 보관하는 한 프레임.</summary>
public sealed record CapturedFrame(byte[] Png, byte[] Pixels, CaptureRegion Region, GameWindow Window)
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

    /// <summary>부모 호스트가 dotnet.exe여도 현재 스레드의 좌표를 물리 픽셀로 고정한다.</summary>
    public static void SetDpiMode() => SetThreadDpiAwarenessContext(new nint(-4));

    /// <summary>게임의 활성 대화상자 또는 가장 큰 표시 창을 찾아 클라이언트 좌표를 읽는다.</summary>
    public static GameWindow FindWindow(int processId)
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
            if (handle == foreground) { chosen = handle; return false; }
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

    /// <summary>대상 창을 전면에 놓고 실제 전면 창이 되었는지 확인한다.</summary>
    public static async Task<GameWindow> FocusAsync(int processId, CancellationToken cancellation)
    {
        GameWindow window = FindWindow(processId);
        if (!window.Foreground || IsIconic(new nint(window.Handle)))
        {
            ShowWindow(new nint(window.Handle), RestoreWindow);
            SetForegroundWindow(new nint(window.Handle));
            await Task.Delay(180, cancellation);
            window = FindWindow(processId);
        }
        if (!window.Foreground) throw new InvalidOperationException("게임 창의 포커스를 얻지 못했습니다. 입력을 보내지 않았습니다.");
        return window;
    }

    /// <summary>가려지지 않은 실제 화면을 캡처한다. DirectDraw의 빈 PrintWindow 결과를 사용하지 않는다.</summary>
    public static CapturedFrame Capture(GameWindow window, string region)
    {
        SetDpiMode();
        CaptureRegion roi = CaptureRegion.Parse(region, window);
        var screenRect = new Rectangle(window.X + roi.X, window.Y + roi.Y, roi.Width, roi.Height);
        if (!SystemInformation.VirtualScreen.Contains(screenRect) || roi.Width > 4096 || roi.Height > 4096)
            throw new InvalidOperationException("캡처 영역이 화면 밖에 있거나 4096픽셀 한도를 넘습니다.");
        CheckForeground(window);
        // 표본 위치가 다른 창에 덮였으면 다른 프로그램의 화면을 증거로 저장하지 않는다.
        foreach (Point point in new[] { screenRect.Location,
            new Point(screenRect.Right - 1, screenRect.Bottom - 1),
            new Point(screenRect.Left + roi.Width / 2, screenRect.Top + roi.Height / 2) })
        {
            if (GetAncestor(WindowFromPoint(point), RootWindow).ToInt64() != window.Handle)
                throw new InvalidOperationException("게임 캡처 영역이 다른 창에 가려져 있습니다.");
        }
        using var bitmap = new Bitmap(roi.Width, roi.Height, PixelFormat.Format24bppRgb);
        using (Graphics graphics = Graphics.FromImage(bitmap))
            graphics.CopyFromScreen(screenRect.Location, Point.Empty, screenRect.Size, CopyPixelOperation.SourceCopy);
        byte[] pixels = new byte[roi.Width * roi.Height * 3];
        BitmapData data = bitmap.LockBits(new Rectangle(0, 0, roi.Width, roi.Height), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
        try
        {
            // 행 패딩을 빼서 영역 비교에 쓸 RGB 바이트만 보관한다.
            for (int y = 0; y < roi.Height; y++)
                Marshal.Copy(data.Scan0 + y * data.Stride, pixels, y * roi.Width * 3, roi.Width * 3);
        }
        finally { bitmap.UnlockBits(data); }
        using var stream = new MemoryStream();
        bitmap.Save(stream, ImageFormat.Png);
        CheckForeground(window);
        return new(stream.ToArray(), pixels, roi, window);
    }

    /// <summary>입력 직전 대상 창을 다시 검사해 다른 창에 잘못 보내는 일을 줄인다.</summary>
    private static void CheckForeground(GameWindow window)
    {
        if (GetForegroundWindow().ToInt64() != window.Handle)
            throw new InvalidOperationException("게임 창의 포커스가 바뀌었습니다. 작업을 중단합니다.");
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
}

/// <summary>한 번의 제한된 입력 동작. 좌표는 캡처 영역이 아닌 전체 클라이언트 기준이다.</summary>
public sealed record InputRequest(string Kind, int X = 0, int Y = 0, int ToX = 0, int ToY = 0,
    string Button = "left", string Key = "", int DurationMs = 80, int SettleMs = 300);
