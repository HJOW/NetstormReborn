using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 호스트의 실행 인자. 원본 실행 파일의 <c>--run</c> 인자 중 호스트 골격이 쓰는 부분만 받는다.
/// 근거: cpppj/src/app/main.cpp 의 --run/--window/--frames/--ui-script/--ui-report/--set (R1-2a, 2026-10-28).
/// 미션·그리기·소리는 아직 없으므로 <c>--mission</c>·<c>--view</c> 등은 받지 않고, 세션 키는 <see cref="Settings"/>에 보관만 한다.
/// </summary>
public sealed class HostOptions
{
    /// <summary>논리 화면 폭(기본 1024). --width/--set SCREENW 로 정한다.</summary>
    public int Width { get; private set; } = 1024;

    /// <summary>논리 화면 높이(기본 768). --height/--set SCREENH 로 정한다.</summary>
    public int Height { get; private set; } = 768;

    /// <summary>초당 프레임 수(기본 40). --fps/--set maxFPS 로 정한다.</summary>
    public int FramesPerSecond { get; private set; } = 40;

    /// <summary>시작부터 전체화면 borderless 로 여는지. --fullscreen이면 참이다.</summary>
    public bool Fullscreen { get; private set; }

    /// <summary>실행할 프레임 수. 0이면 창을 닫을 때까지 돈다. --frames 로 정한다.</summary>
    public ulong Frames { get; private set; }

    /// <summary>UI 스크립트 경로. 없으면 null이다. --ui-script 로 정한다.</summary>
    public string? UiScriptPath { get; private set; }

    /// <summary>건너뛴 관측 보고서 경로. 없으면 null이다. --ui-report 로 정한다.</summary>
    public string? UiReportPath { get; private set; }

    /// <summary>--set 으로 받은 원본 설정 뭉치. 세션이 생기기 전에는 보관만 한다.</summary>
    public IReadOnlyDictionary<string, string> Settings => _settings;

    /// <summary>--set 설정 뭉치의 실제 저장소.</summary>
    private readonly Dictionary<string, string> _settings = new(StringComparer.Ordinal);

    /// <summary>기본값으로 만든다.</summary>
    private HostOptions()
    {
    }

    /// <summary>
    /// 명령줄 인자를 해석한다. 알 수 없는 인자·빠진 값·범위 밖 숫자는 사용법 오류로 거부한다.
    /// </summary>
    /// <param name="args">프로그램 인자.</param>
    public static HostOptions Parse(string[] args)
    {
        ArgumentNullException.ThrowIfNull(args);
        var options = new HostOptions();
        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "--window":
                    options.Fullscreen = false;
                    break;
                case "--fullscreen":
                    options.Fullscreen = true;
                    break;
                case "--width":
                    options.Width = TakePositiveInt(args, ref i, "--width");
                    break;
                case "--height":
                    options.Height = TakePositiveInt(args, ref i, "--height");
                    break;
                case "--fps":
                    options.FramesPerSecond = TakePositiveInt(args, ref i, "--fps");
                    break;
                case "--frames":
                    options.Frames = TakeNonNegativeLong(args, ref i, "--frames");
                    break;
                case "--ui-script":
                    options.UiScriptPath = TakeValue(args, ref i, "--ui-script");
                    break;
                case "--ui-report":
                    options.UiReportPath = TakeValue(args, ref i, "--ui-report");
                    break;
                case "--set":
                    options.ApplySettings(TakeValue(args, ref i, "--set"));
                    break;
                default:
                    throw new ArgumentException($"알 수 없는 호스트 인자: {args[i]}", nameof(args));
            }
        }
        // 화면 크기와 프레임 속도는 --set 과 직접 인자 중 뒤에 나온 값이 이긴다
        return options;
    }

    /// <summary>사용법을 돌려준다. 오류 종료 때 그대로 출력한다.</summary>
    public static string Usage() =>
        "사용법: NetstormClientHost [--window|--fullscreen] [--width N] [--height N] [--fps N] [--frames N] [--ui-script steps.tsv] [--ui-report report.tsv] [--set \"k=v;k=v...\"]";

    /// <summary>다음 인자를 양의 정수로 읽는다.</summary>
    private static int TakePositiveInt(string[] args, ref int index, string flag)
    {
        string value = TakeValue(args, ref index, flag);
        if (!int.TryParse(value, out int number) || number <= 0)
        {
            throw new ArgumentException($"호스트 인자 {flag} 는 양의 정수여야 합니다: {value}", nameof(args));
        }
        return number;
    }

    /// <summary>다음 인자를 0 이상 정수로 읽는다.</summary>
    private static ulong TakeNonNegativeLong(string[] args, ref int index, string flag)
    {
        string value = TakeValue(args, ref index, flag);
        if (!ulong.TryParse(value, out ulong number))
        {
            throw new ArgumentException($"호스트 인자 {flag} 는 0 이상 정수여야 합니다: {value}", nameof(args));
        }
        return number;
    }

    /// <summary>다음 인자를 값으로 읽는다.</summary>
    private static string TakeValue(string[] args, ref int index, string flag)
    {
        if (index + 1 >= args.Length || args[index + 1].StartsWith("--", StringComparison.Ordinal))
        {
            throw new ArgumentException($"호스트 인자 {flag} 뒤에 값이 필요합니다", nameof(args));
        }
        return args[++index];
    }

    /// <summary>
    /// <c>k=v;k=v</c> 뭉치를 해석한다. 아는 호스트 키(SCREENW·SCREENH·maxFPS)는 바로 적용하고, 나머지는 세션용으로 보관한다.
    /// 빈 조각은 건너뛰고, <c>=</c>가 없는 조각은 거부한다.
    /// </summary>
    /// <param name="settings"><c>;</c> 로 이은 설정 뭉치.</param>
    private void ApplySettings(string settings)
    {
        foreach (string piece in settings.Split(';'))
        {
            if (string.IsNullOrWhiteSpace(piece))
            {
                continue;
            }
            int equals = piece.IndexOf('=');
            if (equals <= 0)
            {
                throw new ArgumentException($"호스트 --set 조각에 = 이 없습니다: {piece}", nameof(settings));
            }
            string key = piece[..equals].Trim();
            string value = piece[(equals + 1)..].Trim();
            _settings[key] = value;
            // 원본 설정 이름(대소문자 구분)을 호스트 기본값에 바로 반영한다
            if (key is "SCREENW" or "SCREENH" or "maxFPS")
            {
                if (!int.TryParse(value, out int number) || number <= 0)
                {
                    throw new ArgumentException($"호스트 --set {key} 는 양의 정수여야 합니다: {value}", nameof(settings));
                }
                if (key == "SCREENW")
                {
                    Width = number;
                }
                else if (key == "SCREENH")
                {
                    Height = number;
                }
                else
                {
                    FramesPerSecond = number;
                }
            }
        }
    }
}
