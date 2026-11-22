// 5차 요구: 게임이 실행 중 자동으로 만드는 모든 파일(설정·진행·커스텀 맵·캡처·진단 등)을 설치 위치가 아닌
// 사용자 홈 아래에 둔다. 관리자 권한이 필요한 위치나 읽기 전용 설치에서도 게임이 정상 동작하게 하려는 호환 처리이며
// 원본에는 없는 새 장치다. Linux 경로는 $HOME/.netstorm 아래다(사용자 결정 D5).
// 2026-11-11 0.359에서 cpppj platform/UserDataRoot.h/.cpp를 이식했다.
using System.Runtime.InteropServices;

namespace Netstorm.Client.Host;

/// <summary>
/// 사용자 데이터 루트와 그 하위 폴더 경로를 계산하는 값 객체다. 경로 계산만 하며 생성자는 파일·폴더를 만들지 않는다
/// (cpppj <c>platform/UserDataRoot</c>).
/// </summary>
/// <remarks>2026-11-11 0.359에서 이식했다. Windows 체인과 Linux 분기는 사용자 결정 D5를 따른다.</remarks>
public sealed class UserDataRoot
{
    /// <summary>설정·진행 파일 폴더 이름이다. options.cfg·!state.dat 등이 든다.</summary>
    public const string SavesFolder = "Saves";
    /// <summary>Edit 메뉴에서 새로 만든 맵(.fort·.english 등) 폴더 이름이다.</summary>
    public const string CustomMapsFolder = "CustomMaps";
    /// <summary>Edit 장부·잠금 같은 편집 보조 metadata 폴더 이름이다.</summary>
    public const string EditorFolder = "Editor";
    /// <summary>GIF 스크린샷·연속 BMP 캡처 폴더 이름이다.</summary>
    public const string CapturesFolder = "Captures";
    /// <summary>다시 만들 수 있는 글꼴 캐시·생성물 폴더 이름이다.</summary>
    public const string CacheFolder = "Cache";
    /// <summary>진단 덤프 폴더 이름이다.</summary>
    public const string DiagnosticsFolder = "Diagnostics";
    /// <summary>7차 요구: 비정상 종료·오류 상세 로그 폴더 이름이다. 소문자 logs가 요구 사양이다.</summary>
    public const string LogsFolder = "logs";
    /// <summary>개발·검사에서 실제 사용자 홈 대신 임시 폴더를 쓰게 하는 환경 변수 이름이다. 값이 비어 있으면 무시한다.</summary>
    public const string OverrideVariable = "NETSTORM_USER_DATA";

    /// <summary>절대화한 사용자 데이터 루트다.</summary>
    private readonly string _root;

    /// <summary>명시한 절대 루트를 그대로 사용한다(검사나 이식 도구용). 상대 경로는 현재 작업 폴더 기준으로 절대화한다.</summary>
    /// <param name="root">사용할 루트다.</param>
    public UserDataRoot(string root)
    {
        ArgumentNullException.ThrowIfNull(root);
        _root = Path.GetFullPath(root);
    }

    /// <summary>환경 변수 재정의 → OS별 사용자 폴더 순으로 루트를 정한다.</summary>
    /// <returns>결정한 루트다.</returns>
    /// <exception cref="InvalidOperationException">루트를 결정할 수 없다. 설치 폴더로 되돌아가지 않는다.</exception>
    /// <remarks>Windows는 알려진 Saved Games 폴더 → %USERPROFILE%\Saved Games → %LOCALAPPDATA% 순이며, Linux는 $HOME/.netstorm이다.</remarks>
    public static UserDataRoot Resolve()
    {
        // 검사·휴대 실행의 명시 재정의다
        string? overridePath = Environment.GetEnvironmentVariable(OverrideVariable);
        if (!string.IsNullOrEmpty(overridePath))
        {
            return new UserDataRoot(overridePath);
        }
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            string? home = Environment.GetEnvironmentVariable("HOME");
            if (!string.IsNullOrEmpty(home))
            {
                return new UserDataRoot(Path.Combine(home, ".netstorm"));
            }
            throw new InvalidOperationException("사용자 데이터 폴더($HOME/.netstorm)를 결정할 수 없습니다");
        }
        // 리디렉션된 Saved Games도 따른다
        string saved = KnownSavedGames();
        if (saved.Length != 0)
        {
            return new UserDataRoot(Path.Combine(saved, "Netstorm"));
        }
        // 알려진 폴더를 못 얻은 환경의 문서화된 기본 위치다
        string? profile = Environment.GetEnvironmentVariable("USERPROFILE");
        if (!string.IsNullOrEmpty(profile))
        {
            return new UserDataRoot(Path.Combine(profile, "Saved Games", "Netstorm"));
        }
        // 마지막 사용자 전용 위치다
        string? local = Environment.GetEnvironmentVariable("LOCALAPPDATA");
        if (!string.IsNullOrEmpty(local))
        {
            return new UserDataRoot(Path.Combine(local, "Netstorm"));
        }
        throw new InvalidOperationException("사용자 데이터 폴더(Saved Games\\Netstorm)를 결정할 수 없습니다");
    }

    /// <summary>Windows가 사용자별로 정한 Saved Games 폴더를 조회한다. 실패하면 빈 문자열이며 폴더를 만들지 않는다.</summary>
    /// <returns>폴더 경로, 실패하면 빈 문자열이다.</returns>
    private static string KnownSavedGames()
    {
        try
        {
            Guid savedGames = new("4C5C32FF-BB70-11D0-8EBA-00C04FD9126B");
            int result = SHGetKnownFolderPath(ref savedGames, 0, IntPtr.Zero, out IntPtr text);
            if (result != 0 || text == IntPtr.Zero)
            {
                return "";
            }
            try
            {
                return Marshal.PtrToStringUni(text) ?? "";
            }
            finally
            {
                Marshal.FreeCoTaskMem(text);
            }
        }
        catch (DllNotFoundException)
        {
            return "";
        }
    }

    /// <summary>알려진 폴더 조회다. 셸이 CoTaskMem으로 할당한 경로는 호출자가 반납한다.</summary>
    /// <param name="folder">폴더 GUID다.</param><param name="flags">조회 플래그다.</param><param name="token">액세스 토큰, 없으면 0이다.</param><param name="path">돌려받은 경로다.</param>
    /// <returns>성공하면 0이다.</returns>
    [DllImport("shell32.dll")]
    private static extern int SHGetKnownFolderPath(ref Guid folder, uint flags, IntPtr token, out IntPtr path);

    /// <summary>사용자 데이터 루트(…\Saved Games\Netstorm) 경로다.</summary>
    public string Root => _root;

    /// <summary>설정·진행 폴더(Root\Saves)다.</summary>
    public string Saves() => Path.Combine(_root, SavesFolder);

    /// <summary>새 커스텀 맵 폴더(Root\CustomMaps)다.</summary>
    public string CustomMaps() => Path.Combine(_root, CustomMapsFolder);

    /// <summary>편집 장부·잠금 metadata 폴더(Root\Editor)다.</summary>
    public string Editor() => Path.Combine(_root, EditorFolder);

    /// <summary>캡처 폴더(Root\Captures)다.</summary>
    public string Captures() => Path.Combine(_root, CapturesFolder);

    /// <summary>재생성 가능한 캐시·생성물 폴더(Root\Cache)다.</summary>
    public string Cache() => Path.Combine(_root, CacheFolder);

    /// <summary>진단 덤프 폴더(Root\Diagnostics)다.</summary>
    public string Diagnostics() => Path.Combine(_root, DiagnosticsFolder);

    /// <summary>오류·비정상 종료 상세 로그 폴더(Root\logs)다. 파일 이름은 yyyyMMddHHmmss.log이며 폴더는 첫 기록 때 만든다.</summary>
    public string Logs() => Path.Combine(_root, LogsFolder);

    /// <summary>옵션 파일(Root\Saves\options.cfg)이다. 원본의 설치\d\options.cfg를 대신한다.</summary>
    public string OptionsFile() => Path.Combine(Saves(), "options.cfg");

    /// <summary>전투 체크포인트(Root\Saves\!state.dat)다. 원본의 설치\d\!state.dat를 대신한다.</summary>
    public string SavedStateFile() => Path.Combine(Saves(), "!state.dat");

    /// <summary>전체화면 시작 표식(Root\Saves\fullscreenStateFile.dat)이다. 호환판은 이 파일을 만들지 않고 정리만 한다.</summary>
    public string FullScreenStateFile() => Path.Combine(Saves(), "fullscreenStateFile.dat");

    /// <summary>폴더가 없으면 상위까지 만든다. 이미 있으면 아무 일도 하지 않으며 만들 수 없으면 예외를 던진다.</summary>
    /// <param name="directory">만들 폴더다.</param>
    /// <exception cref="IOException">같은 이름의 파일이 있거나 권한이 없다.</exception>
    public static void EnsureDirectory(string directory)
    {
        ArgumentNullException.ThrowIfNull(directory);
        try
        {
            // 이미 있으면 false를 돌려주므로 실패 판정은 최종 존재 여부로만 한다
            Directory.CreateDirectory(directory);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or DirectoryNotFoundException or NotSupportedException or PathTooLongException)
        {
            throw new IOException("사용자 데이터 폴더를 만들 수 없습니다: " + directory, error);
        }
        if (!Directory.Exists(directory))
        {
            throw new IOException("사용자 데이터 폴더를 만들 수 없습니다: " + directory);
        }
    }
}
