using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 출력 호스트의 별도 진입. 기존 <c>Netstorm.Game</c> 실행은 건드리지 않고 이 진입으로만 돈다.
/// 인자: [--window|--fullscreen] [--width N] [--height N] [--fps N] [--frames N]
/// [--ui-script steps.tsv] [--ui-report report.tsv] [--set "k=v;..."].
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
        var screen = new Screen(options.Width, options.Height);
        var input = new InputQueue();
        UiScriptRunner? runner = options.UiScriptPath is null
            ? null
            : new UiScriptRunner(UiScript.ParseFile(options.UiScriptPath), input, screen);
        var driver = new HostFrameDriver(screen, input, runner);
        try
        {
            using var game = new ClientHostGame(driver, options);
            game.Run();
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception.Message);
            return RuntimeError;
        }
        finally
        {
            // 보고서는 실패해도 남긴다. 경로가 없으면 쓰지 않는다
            if (options.UiReportPath is not null)
            {
                File.WriteAllLines(options.UiReportPath, driver.SkippedObservations);
            }
        }
        return Ok;
    }
}
