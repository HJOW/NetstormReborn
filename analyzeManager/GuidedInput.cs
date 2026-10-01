using System.Drawing;

namespace Netstorm.AnalyzeManager;

/// <summary>녹화할 Windows 입력 메시지 번호. 원시 번호와 읽기 쉬운 조작 이름을 함께 보존한다.</summary>
public enum GuidedInputMessage
{
    /// <summary>일반 키 누름.</summary>
    KeyDown = 0x0100,
    /// <summary>일반 키 해제.</summary>
    KeyUp = 0x0101,
    /// <summary>Alt 조합 등 시스템 키 누름.</summary>
    SystemKeyDown = 0x0104,
    /// <summary>시스템 키 해제.</summary>
    SystemKeyUp = 0x0105,
    /// <summary>마우스 이동.</summary>
    MouseMove = 0x0200,
    /// <summary>왼쪽 버튼 누름.</summary>
    LeftDown = 0x0201,
    /// <summary>왼쪽 버튼 해제.</summary>
    LeftUp = 0x0202,
    /// <summary>오른쪽 버튼 누름.</summary>
    RightDown = 0x0204,
    /// <summary>오른쪽 버튼 해제.</summary>
    RightUp = 0x0205,
    /// <summary>가운데 버튼 누름.</summary>
    MiddleDown = 0x0207,
    /// <summary>가운데 버튼 해제.</summary>
    MiddleUp = 0x0208,
    /// <summary>세로 휠 회전.</summary>
    Wheel = 0x020A,
    /// <summary>보조 버튼 누름.</summary>
    ExtraDown = 0x020B,
    /// <summary>보조 버튼 해제.</summary>
    ExtraUp = 0x020C,
    /// <summary>가로 휠 회전.</summary>
    HorizontalWheel = 0x020E,
}

/// <summary>사용자 조작의 종류·키·버튼·좌표. 기존 message·virtualKey·x·y 필드도 보존한다.</summary>
public sealed record GuidedInput(string Type, string Action)
{
    /// <summary>키보드 확장 키 플래그.</summary>
    private const uint ExtendedKeyFlag = 0x01;
    /// <summary>프로그램이 생성한 키 입력 플래그.</summary>
    private const uint InjectedKeyFlag = 0x10;
    /// <summary>키 입력 당시 Alt가 눌린 상태를 나타내는 플래그.</summary>
    private const uint AltKeyFlag = 0x20;
    /// <summary>프로그램이 생성한 마우스 입력 플래그.</summary>
    private const uint InjectedMouseFlag = 0x01;

    public int? Message { get; init; }
    public uint? VirtualKey { get; init; }
    public uint? ScanCode { get; init; }
    public string? Key { get; init; }
    public uint? Flags { get; init; }
    public uint? HookTimeMs { get; init; }
    public bool? Injected { get; init; }
    public bool? Extended { get; init; }
    public bool? AltDown { get; init; }
    public string? Button { get; init; }
    public int? X { get; init; }
    public int? Y { get; init; }
    public int? ScreenX { get; init; }
    public int? ScreenY { get; init; }
    public bool? Inside { get; init; }
    public short? Wheel { get; init; }
    public string? WheelAxis { get; init; }
    public GameWindow? Window { get; init; }

    /// <summary>지원하는 키 메시지에 키 이름과 누름·해제 구분을 붙인다. 자동 반복 누름도 그대로 기록한다.</summary>
    public static GuidedInput? Keyboard(int message, uint virtualKey, uint scanCode, uint flags, uint hookTimeMs)
    {
        string? action = (GuidedInputMessage)message switch
        {
            GuidedInputMessage.KeyDown or GuidedInputMessage.SystemKeyDown => "down",
            GuidedInputMessage.KeyUp or GuidedInputMessage.SystemKeyUp => "up",
            _ => null,
        };
        if (action == null) return null;
        return new("keyboard", action)
        {
            Message = message, VirtualKey = virtualKey, ScanCode = scanCode, Key = KeyName(virtualKey),
            Flags = flags, HookTimeMs = hookTimeMs, Injected = (flags & InjectedKeyFlag) != 0,
            Extended = (flags & ExtendedKeyFlag) != 0, AltDown = (flags & AltKeyFlag) != 0,
        };
    }

    /// <summary>マウス 메시지를 버튼·축·휠 변화량과 게임 클라이언트/화면의 물리 픽셀 좌표로 변환한다.</summary>
    public static GuidedInput? Mouse(int message, Point screen, GameWindow window, uint mouseData, uint flags, uint hookTimeMs)
    {
        var kind = (GuidedInputMessage)message;
        string? action = kind switch
        {
            GuidedInputMessage.MouseMove => "move",
            GuidedInputMessage.LeftDown or GuidedInputMessage.RightDown or GuidedInputMessage.MiddleDown or GuidedInputMessage.ExtraDown => "down",
            GuidedInputMessage.LeftUp or GuidedInputMessage.RightUp or GuidedInputMessage.MiddleUp or GuidedInputMessage.ExtraUp => "up",
            GuidedInputMessage.Wheel or GuidedInputMessage.HorizontalWheel => "wheel",
            _ => null,
        };
        if (action == null) return null;
        string? button = kind switch
        {
            GuidedInputMessage.LeftDown or GuidedInputMessage.LeftUp => "left",
            GuidedInputMessage.RightDown or GuidedInputMessage.RightUp => "right",
            GuidedInputMessage.MiddleDown or GuidedInputMessage.MiddleUp => "middle",
            GuidedInputMessage.ExtraDown or GuidedInputMessage.ExtraUp => (mouseData >> 16) switch { 1 => "x1", 2 => "x2", _ => "unknown" },
            _ => null,
        };
        int x = screen.X - window.X;
        int y = screen.Y - window.Y;
        return new("mouse", action)
        {
            Message = message, Button = button, X = x, Y = y, ScreenX = screen.X, ScreenY = screen.Y,
            Inside = x >= 0 && y >= 0 && x < window.Width && y < window.Height,
            Wheel = action == "wheel" ? unchecked((short)(mouseData >> 16)) : (short)0,
            WheelAxis = kind == GuidedInputMessage.Wheel ? "vertical" : kind == GuidedInputMessage.HorizontalWheel ? "horizontal" : null,
            Flags = flags, HookTimeMs = hookTimeMs, Injected = (flags & InjectedMouseFlag) != 0, Window = window,
        };
    }

    /// <summary>가상 키 번호를 읽기 쉬운 이름으로 바꾸고 알 수 없는 번호도 16진수로 보존한다.</summary>
    private static string KeyName(uint virtualKey) => (Keys)virtualKey switch
    {
        Keys.Return => "Enter",
        Keys.Escape => "Escape",
        Keys.ShiftKey => "Shift",
        Keys.LShiftKey => "LeftShift",
        Keys.RShiftKey => "RightShift",
        Keys.ControlKey => "Ctrl",
        Keys.LControlKey => "LeftCtrl",
        Keys.RControlKey => "RightCtrl",
        Keys.Menu => "Alt",
        Keys.LMenu => "LeftAlt",
        Keys.RMenu => "RightAlt",
        _ => Enum.GetName((Keys)virtualKey) ?? $"VK_0x{virtualKey:X2}",
    };
}
