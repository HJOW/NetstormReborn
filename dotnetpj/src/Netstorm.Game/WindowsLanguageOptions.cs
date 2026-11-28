using System.Runtime.InteropServices;

// 실제 Windows 시작 LANGID만 원문 선택 본체로 전달한다.
namespace Netstorm.Client.Host;

/// <summary>최초 설정 적재 전에 OS의 현재 사용자 언어 번호를 감지하는 플랫폼 경계다.</summary>
public static class WindowsLanguageOptions
{
    /// <summary>언어 표/파일/난수를 읽지 않고 같은 GetUserDefaultLangID 결과를 해석한다.</summary>
    public static uint Detect()
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 언어 감지는 Windows 전용입니다");
        return LanguageOptions.Detect(GetUserDefaultLangID());
    }
    /// <summary>원문004de390의 실제 낮은 WORD OS 반환값이다.</summary>
    [DllImport("kernel32.dll", ExactSpelling = true)]
    private static extern ushort GetUserDefaultLangID();
}
