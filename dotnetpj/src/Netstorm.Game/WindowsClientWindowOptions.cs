using System.Runtime.InteropServices;
using Netstorm.Client.Display;

// 옵션의 변경 분기에서만 같은 실제 HWND의 기하와 MoveWindow를 선택한다.
namespace Netstorm.Client.Host;

/// <summary>Screen 버퍼/Graphics 장치 교체와 별개인 원문 Win32 창 효과다.</summary>
public static class WindowsClientWindowOptions
{
    /// <summary>GetWindowRect/GetClientRect의 signed 네DWORD 물리다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeRect
    {
        /// <summary>좌측/위쪽/우측/아래쪽의 현재 절대 또는 Client 좌표다.</summary>
        public int Left, Top, Right, Bottom;
        /// <summary>같은 네DWORD를 기존 화면 사각형 값으로 전달한다.</summary>
        public readonly ScreenRect Value() => new(Left, Top, Right, Bottom);
    }
    /// <summary>첫 선택 효과에서 HWND를 한 번 캡처한다. 생성/변경 없는 경로는 getter/OS를 호출하지 않는다.</summary>
    public static ClientWindowOptionHooks CreateHooks(Func<nint> currentWindow) => CreateHooks(currentWindow, null);
    /// <summary>같은 선택 위치의 이동만 호스트의 고정 창 제한 대여 안에서 실행한다. 사각형 조회 순서는 바꾸지 않는다.</summary>
    public static ClientWindowOptionHooks CreateHooks(Func<nint> currentWindow, Action<Action>? moveScope)
    {
        ArgumentNullException.ThrowIfNull(currentWindow); bool captured = false; nint window = 0;
        // 원문 변경 분기의 지역 const HWND를 같은 Interpret 호출 안에서 유지한다.
        nint Handle()
        {
            if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("옵션 창 효과는 Windows 전용입니다");
            if (!captured) { window = currentWindow(); captured = true; }
            return window;
        }
        return new()
        {
            WindowRectangle = () => { _ = GetWindowRect(Handle(), out NativeRect value); return value.Value(); },
            ClientRectangle = () => { _ = GetClientRect(Handle(), out NativeRect value); return value.Value(); },
            Move = (x, y, width, height, repaint) =>
            {
                // HWND는 앞선 사각형 조회의 같은 지역 캡처이며 이동 인자는 원문 값을 그대로 사용한다.
                void Execute() { _ = MoveWindow(Handle(), x, y, width, height, repaint); }
                if (moveScope is null) Execute(); else moveScope(Execute);
            },
        };
    }
    /// <summary>같은 실제 창의 바깥 사각형을 읽는다.</summary>
    [DllImport("user32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetWindowRect(nint window, out NativeRect rectangle);
    /// <summary>같은 실제 창의 Client 사각형을 읽는다.</summary>
    [DllImport("user32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetClientRect(nint window, out NativeRect rectangle);
    /// <summary>원문과 같은 위치/두 크기 인자/repaint를 전달한다.</summary>
    [DllImport("user32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool MoveWindow(nint window, int x, int y, int width, int height, [MarshalAs(UnmanagedType.Bool)] bool repaint);
}
