using System.Runtime.InteropServices;
using System.Runtime.Versioning;

// DesktopGL의 SDL_Window 포인터에서 DirectSound가 요구하는 실제 HWND만 빌린다.
namespace Netstorm.Client.Host;

/// <summary>MonoGame 3.8.5.1의 Window.Handle과 Windows 네이티브 창 핸들의 경계다. 창과 SDL 수명은 소유하지 않는다.</summary>
[SupportedOSPlatform("windows")]
public static class WindowsSdlWindow
{
    /// <summary>SDL2의 실제 세 BYTE 버전 구조체다.</summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    private struct SdlVersion
    {
        /// <summary>실제로 적재한 SDL의 주 버전이다.</summary>
        public byte Major;
        /// <summary>실제로 적재한 SDL의 부 버전이다.</summary>
        public byte Minor;
        /// <summary>실제로 적재한 SDL의 패치 버전이다.</summary>
        public byte Patch;
    }

    /// <summary>SDL2 SDL_syswm.h의 고정64BYTE union을 포함하는72BYTE ABI다. Windows 창 포인터는+8이다.</summary>
    [StructLayout(LayoutKind.Explicit, Size = 72)]
    private struct WindowInformation
    {
        /// <summary>호출 전에 같은 적재 DLL의 버전으로 초기화해야 한다.</summary>
        [FieldOffset(0)] public SdlVersion Version;
        /// <summary>SDL_SYSWM_WINDOWS의 값1인지 확인하는 실제 하위 시스템이다.</summary>
        [FieldOffset(4)] public int Subsystem;
        /// <summary>현재 Windows 창을 빌리는 포인터 폭의 HWND다.</summary>
        [FieldOffset(8)] public nint Window;
    }

    /// <summary>같은 SDL DLL의 실제 버전을 읽는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void GetVersion(ref SdlVersion version);
    /// <summary>SDL_Window에서 네이티브 창 정보를 읽는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int GetWindowInformation(nint window, ref WindowInformation information);
    /// <summary>실제 HWND의 생존만 확인하며 다른 창을 만들거나 활성화하지 않는다.</summary>
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWindow(nint window);

    /// <summary>같은 적재 SDL의 SysWM 조회로 현재 Windows HWND를 얻고 추가 DLL 참조만 반납한다.
    /// 호출자는 최종 창 생성 뒤 UI 스레드에서 같은 SDL DLL이 소유한 생존 창만 넘긴다. 임의/반납된 포인터의 안전성 검사가 아니다.</summary>
    public static nint NativeHandle(nint sdlWindow)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("SDL의 Windows HWND 조회는 Windows 전용이다");
        if (sdlWindow == 0) throw new ArgumentException("실제 DesktopGL SDL 창 포인터가 필요합니다", nameof(sdlWindow));
        nint library = NativeLibrary.Load("SDL2.dll");
        try
        {
            var version = Marshal.GetDelegateForFunctionPointer<GetVersion>(NativeLibrary.GetExport(library, "SDL_GetVersion"));
            var information = Marshal.GetDelegateForFunctionPointer<GetWindowInformation>(NativeLibrary.GetExport(library, "SDL_GetWindowWMInfo"));
            var result = new WindowInformation();
            version(ref result.Version);
            if (result.Version.Major != 2 || information(sdlWindow, ref result) == 0
                || result.Subsystem != 1 || result.Window == 0 || !IsWindow(result.Window))
                throw new InvalidOperationException("현재 SDL 창의 실제 Windows HWND를 읽지 못했습니다");
            return result.Window;
        }
        finally { NativeLibrary.Free(library); }
    }
}
