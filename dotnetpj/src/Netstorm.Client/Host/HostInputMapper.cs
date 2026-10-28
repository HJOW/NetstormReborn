using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host;

/// <summary>
/// 호스트가 누르는 마우스 버튼. MonoGame 값에 묶이지 않도록 입력 코드 쪽에서 다시 정의한다.
/// </summary>
public enum HostMouseButton
{
    /// <summary>왼쪽 버튼.</summary>
    Left,

    /// <summary>오른쪽 버튼.</summary>
    Right,

    /// <summary>가운데 버튼.</summary>
    Middle,
}

/// <summary>
/// 실제 키·마우스 상태를 <see cref="InputQueue"/> 코드로 바꾸는 순수 변환. 창이 없어도 검사할 수 있다.
/// 글자는 WM_CHAR처럼 아래 코드에 그대로 넣고, 이름 키는 가상 키 번호를 위로 민다(<see cref="UiScriptRunner"/>의 key 단계와 같은 규칙).
/// 마우스 좌표는 호출자가 <see cref="Screen.ToLogicalPoint"/>로 논리 좌표로 바꾼 값을 넣는다.
/// 근거: cpppj/src/client/InputEvent.cpp 의 창 프로시저 넣기. (R1-2a, 2026-10-28)
/// </summary>
public static class HostInputMapper
{
    /// <summary>가상 키 번호의 상한. Win32 VK는 한 바이트다.</summary>
    private const int MaxVirtualKey = 255;

    /// <summary>
    /// 키 누름·뗌을 입력 코드로 바꾼다. F11(전체화면 전환)은 호스트가 먼저 가로채므로 여기서는 거부한다.
    /// </summary>
    /// <param name="virtualKey">Win32 가상 키 번호(0~255). MonoGame Keys 값과 같은 번호를 그대로 넘긴다.</param>
    /// <param name="released">뗌이면 참이다.</param>
    /// <param name="shift">Shift가 함께 눌렸는지.</param>
    /// <param name="control">Control이 함께 눌렸는지.</param>
    /// <param name="alt">Alt가 함께 눌렸는지.</param>
    public static uint KeyCode(int virtualKey, bool released, bool shift, bool control, bool alt)
    {
        if (virtualKey is 0x7A or 0x7B)
        {
            throw new ArgumentException("호스트 전환 키(F11·F12)는 입력 큐에 넣지 않는다", nameof(virtualKey));
        }
        if (virtualKey < 0 || virtualKey > MaxVirtualKey)
        {
            throw new ArgumentOutOfRangeException(nameof(virtualKey), virtualKey, "가상 키 번호는 0~255 이어야 합니다");
        }
        uint code = (uint)virtualKey << InputCode.VirtualKeyShift;
        if (shift)
        {
            code |= InputCode.Shift;
        }
        if (control)
        {
            code |= InputCode.Control;
        }
        if (alt)
        {
            code |= InputCode.Alt;
        }
        if (released)
        {
            code |= InputCode.Release;
        }
        return code;
    }

    /// <summary>
    /// 글자 입력을 입력 코드로 바꾼다. WM_CHAR처럼 글자를 아래 코드에 그대로 넣는다.
    /// </summary>
    /// <param name="character">받은 글자.</param>
    public static uint CharCode(char character) => character;

    /// <summary>
    /// 마우스 버튼 누름·뗌을 입력 코드로 바꾼다.
    /// </summary>
    /// <param name="button">누른 버튼.</param>
    /// <param name="released">뗌이면 참이다.</param>
    public static uint ButtonCode(HostMouseButton button, bool released)
    {
        uint code = button switch
        {
            HostMouseButton.Left => InputCode.LeftButton,
            HostMouseButton.Right => InputCode.RightButton,
            HostMouseButton.Middle => InputCode.MiddleButton,
            _ => throw new ArgumentOutOfRangeException(nameof(button), button, "마우스 버튼은 왼쪽·오른쪽·가운데뿐이다"),
        };
        if (released)
        {
            code |= InputCode.Release;
        }
        return code;
    }

    /// <summary>
    /// 전체화면 커서 제한: 창 밖이면 안쪽 가장자리로 모은 점을 돌려주고, 안이면 null이다. 호출자가 실제 커서를 그 위치로 옮긴다.
    /// 창 모드에서는 호출하지 않는다(사용자가 창 밖으로 나갈 수 있어야 한다).
    /// </summary>
    /// <param name="x">클라이언트 x.</param>
    /// <param name="y">클라이언트 y.</param>
    /// <param name="width">클라이언트 폭.</param>
    /// <param name="height">클라이언트 높이.</param>
    public static ScreenPoint? ConfinePoint(int x, int y, int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            throw new ArgumentException("커서 제한의 클라이언트 크기는 양수여야 합니다");
        }
        // 창 안에 있으면 옮기지 않는다
        if (x >= 0 && x < width && y >= 0 && y < height)
        {
            return null;
        }
        return new ScreenPoint(Math.Clamp(x, 0, width - 1), Math.Clamp(y, 0, height - 1));
    }
}
