// 호스트 시작의 오류 로그 구성과 최상위 실패 기록이다.
// 2026-11-11 0.361에서 cpppj app/main.cpp의 SetupErrorLog·RecordTopLevelFailure를 이식했다.
using Netstorm.Client;

namespace Netstorm.Client.Host;

/// <summary>
/// 프로그램 시작에서 로그 폴더 구성·갈고리 설치·스트림 복제를 하고, 최상위 예외를 치명 기록으로 남긴다
/// (cpppj <c>app/main.cpp</c>의 시작·종료 직전 처리).
/// </summary>
/// <remarks>2026-11-11 0.361에서 이식했다. 게임 실행 없이 검사할 수 있게 진입(`Program`)과 분리했다.</remarks>
public static class HostErrorLog
{
    /// <summary>7차: 사용자 logs 폴더에 공통 오류 로그를 구성하고 비정상 종료 갈고리를 설치한다.</summary>
    /// <remarks>사용자 폴더를 정하지 못해도 시작은 계속하고 표준 오류에만 알린다.</remarks>
    public static void Setup()
    {
        try
        {
            ErrorLog.Configure(UserDataRoot.Resolve().Logs(), ProductVersion.Text);
        }
        catch (Exception error)
        {
            Console.Error.WriteLine("오류 로그 폴더를 정하지 못했습니다: " + error.Message);
        }
        ErrorLog.InstallCrashHandlers();
        ErrorLog.CaptureStandardStreams();
        ErrorLog.Note("시작: NetStorm " + ProductVersion.Text);
    }

    /// <summary>최상위 예외를 로그에 치명 기록으로 남기고 파일이 만들어졌으면 그 경로를 안내 문구에 더한다.</summary>
    /// <param name="error">최상위 예외다.</param>
    /// <returns>보여줄 메시지다.</returns>
    public static string RecordTopLevelFailure(Exception error)
    {
        ArgumentNullException.ThrowIfNull(error);
        // 7차: 종료 직전 판정을 자동 로그에 남긴다
        ErrorLog.RecordException("main 최상위", error, LogLevel.Fatal);
        string file = ErrorLog.CurrentFile();
        if (file.Length == 0)
        {
            return error.Message;
        }
        return error.Message + "\n\n상세: 로그 " + file;
    }

    /// <summary>형식을 알 수 없는 종료를 같은 로그에 남긴다.</summary>
    /// <returns>보여줄 메시지다.</returns>
    public static string RecordUnknownFailure()
    {
        const string what = "알 수 없는 형식의 예외로 종료합니다";
        ErrorLog.RecordException("main 최상위", new InvalidOperationException(what), LogLevel.Fatal);
        string file = ErrorLog.CurrentFile();
        if (file.Length == 0)
        {
            return what;
        }
        return what + "\n\n상세: 로그 " + file;
    }
}
