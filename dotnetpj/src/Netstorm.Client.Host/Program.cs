using Netstorm.Assets;
using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 출력 호스트의 별도 진입. 기존 <c>Netstorm.Game</c> 실행은 건드리지 않고 이 진입으로만 돈다.
/// 인자: [--window|--fullscreen] [--width N] [--height N] [--fps N] [--frames N]
/// [--fixed-frame-ms N] [--fixed-unix-seconds S] [--ui-script steps.tsv] [--ui-report report.tsv] [--set "k=v;..."] [--log-crash-test 종류].
/// 종료 코드 0(정상)·1(실행 오류)·2(인자 오류).
/// 근거: cpppj/src/app/main.cpp 의 --run 진입. (R1-2a, 2026-10-28)
/// </summary>
internal static class Program
{
    /// <summary>정상 종료.</summary>
    private const int Ok = 0;

    /// <summary>실행 오류(스크립트 미완료·그리기 실패 등).</summary>
    private const int RuntimeError = 1;

    /// <summary>인자 오류.</summary>
    private const int UsageError = 2;

    /// <summary>호스트를 만들고 프레임 고리를 돈다.</summary>
    /// <param name="args">명령줄 인자.</param>
    [STAThread]
    private static int Main(string[] args)
    {
        // 7차: 사용자 로그 폴더 구성·갈고리·스트림 복제와 시작 기록이다. 폴더를 못 정해도 시작은 계속한다.
        HostErrorLog.Setup();
        // 7차 검사용: 지정한 방식으로 의도적으로 비정상 종료해 로그 파일을 확인한다.
        if (args.Length == 2 && args[0] == "--log-crash-test")
        {
            try
            {
                ErrorLog.CrashForTest(args[1]);
                return Ok;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(HostErrorLog.RecordTopLevelFailure(exception));
                return RuntimeError;
            }
#pragma warning disable CS1058 // 원문 catch(...)처럼 비예외 종료도 같은 로그에 남긴다.
            catch
            {
                Console.Error.WriteLine(HostErrorLog.RecordUnknownFailure());
                return RuntimeError;
            }
#pragma warning restore CS1058
        }
        HostOptions options;
        try
        {
            options = HostOptions.Parse(args);
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception.Message);
            Console.Error.WriteLine(HostOptions.Usage());
            return UsageError;
        }
        if (options.UiScriptPath is not null && !File.Exists(options.UiScriptPath))
        {
            Console.Error.WriteLine($"UI 스크립트가 없습니다: {options.UiScriptPath}");
            return UsageError;
        }
        HostFrameDriver? driver = null;
        try
        {
            string? data = GameDataLocator.FindDataDirectory();
            HostStartupScene? startup = data is null ? null : new HostStartupScene(data, options.SettingsText);
            startup?.SaveOptions();
            if (startup != null) options.ApplyStartupConfiguration(startup.Configuration);
            var screen = new Screen(options.Width, options.Height);
            var input = new InputQueue();
            UiScriptRunner? runner = options.UiScriptPath is null ? null : new UiScriptRunner(UiScript.ParseFile(options.UiScriptPath), input, screen);
            // 고정 시각 모드는 호스트 시계 원점보다 먼저 켠다. 기본 모드에서는 OS 시계를 그대로 읽는다.
            // 근거: cpppj/src/app/main.cpp 의 --fixed-frame-ms 연결. (2026-11-10 L2·L3 장치에서 추가했다)
            var clock = new HostTimeSource();
            if (options.FixedFrameMs != 0)
                clock.EnableFixed(new HostFixedTime(options.FixedFrameMs, UnixSeconds: options.FixedUnixSeconds ?? HostTimeSource.DefaultUnixSeconds));
            driver = new HostFrameDriver(screen, input, runner, clock);
            using var game = new ClientHostGame(driver, options, startup, clock);
            game.Run();
        }
        catch (Exception exception)
        {
            // 7차: 종료 직전 판정이 자동 로그에 남았으면 그 경로를 안내 문구에 더한다
            string message = HostErrorLog.RecordTopLevelFailure(exception);
            Console.Error.WriteLine(message);
            return RuntimeError;
        }
#pragma warning disable CS1058 // 원문 catch(...)처럼 비예외 종료도 같은 로그에 남긴다.
        catch
        {
            string message = HostErrorLog.RecordUnknownFailure();
            Console.Error.WriteLine(message);
            return RuntimeError;
        }
#pragma warning restore CS1058
        finally
        {
            // 보고서는 실패해도 남긴다. 경로가 없으면 쓰지 않는다
            if (options.UiReportPath is not null && driver != null)
            {
                File.WriteAllLines(options.UiReportPath, driver.SkippedObservations);
            }
        }
        return Ok;
    }
}
