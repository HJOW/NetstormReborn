using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Host;

// 실제 숨긴 SDL 창만 잠시 소유해 SDL_Window와 HWND의 ABI 경계를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>음향 장치를 열지 않고 실제 SDL 창의 네이티브 Windows 핸들을 검증한다.</summary>
[Collection("SDL 실제 창")]
[SupportedOSPlatform("windows")]
public sealed class WindowsSdlWindowTests
{
    /// <summary>SDL video 하위 시스템 참조를 추가하는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int InitializeVideo(uint flags);
    /// <summary>이 검사에서 추가한 video 하위 시스템 참조만 반납하는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void QuitVideo(uint flags);
    /// <summary>화면에 표시하지 않는 자기 SDL 창을 생성하는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate nint CreateWindow([MarshalAs(UnmanagedType.LPUTF8Str)] string title, int x, int y, int width, int height, uint flags);
    /// <summary>자기가 만든 SDL 창만 반납하는 C ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void DestroyWindow(nint window);
    /// <summary>현재 HWND가 속한 실제 프로세스를 읽는다.</summary>
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(nint window, out uint process);
    /// <summary>검사 창이 화면에 표시되지 않는지 Windows의 실제 가시성 비트를 읽는다.</summary>
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)] private static extern bool IsWindowVisible(nint window);

    /// <summary>실제 SDL2의 숨긴 창에서 서로 다른 SDL/Windows 핸들과 반복 조회·생존 프로세스를 검증한다.</summary>
    [Fact]
    public void HiddenSdlWindowExposesItsActualWindowsHandle()
    {
        if (!OperatingSystem.IsWindows()) return;
        string architecture = RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "win-arm64" : "win-x64";
        nint library = NativeLibrary.Load(Path.Combine(AppContext.BaseDirectory, "runtimes", architecture, "native", "SDL2.dll"));
        try
        {
            var quit = Marshal.GetDelegateForFunctionPointer<QuitVideo>(NativeLibrary.GetExport(library, "SDL_QuitSubSystem"));
            var destroy = Marshal.GetDelegateForFunctionPointer<DestroyWindow>(NativeLibrary.GetExport(library, "SDL_DestroyWindow"));
            nint window = 0;
            bool initialized = false;
            try
            {
                var initialize = Marshal.GetDelegateForFunctionPointer<InitializeVideo>(NativeLibrary.GetExport(library, "SDL_InitSubSystem"));
                Assert.Equal(0, initialize(0x20));
                initialized = true;
                var create = Marshal.GetDelegateForFunctionPointer<CreateWindow>(NativeLibrary.GetExport(library, "SDL_CreateWindow"));
                window = create("Netstorm HWND 검사", 0, 0, 32, 32, 8);
                Assert.NotEqual((nint)0, window);
                nint native = WindowsSdlWindow.NativeHandle(window);
                Assert.NotEqual((nint)0, native);
                Assert.NotEqual(window, native);
                Assert.False(IsWindowVisible(native));
                Assert.Equal(native, WindowsSdlWindow.NativeHandle(window));
                Assert.NotEqual(0u, GetWindowThreadProcessId(native, out uint process));
                Assert.Equal((uint)Environment.ProcessId, process);
            }
            finally
            {
                if (window != 0) destroy(window);
                if (initialized) quit(0x20);
            }
        }
        finally { NativeLibrary.Free(library); }
    }

    /// <summary>없는 SDL 창 포인터는 DLL·OS 호출 전에 명시 거부한다.</summary>
    [Fact]
    public void MissingSdlPointerIsRejectedBeforeNativeLookup()
    {
        if (!OperatingSystem.IsWindows()) return;
        Assert.Throws<ArgumentException>(() => WindowsSdlWindow.NativeHandle(0));
    }
}

/// <summary>같은 프로세스의 SDL video 전역 초기화/종료를 다른 검사와 동시에 변경하지 않는다.</summary>
[CollectionDefinition("SDL 실제 창", DisableParallelization = true)]
public sealed class WindowsSdlWindowCollection { }
