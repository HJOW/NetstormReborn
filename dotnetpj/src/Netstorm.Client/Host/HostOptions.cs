using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 호스트의 실행 인자. 원본 실행 파일의 <c>--run</c> 인자 중 호스트 골격이 쓰는 부분만 받는다.
/// 근거: cpppj/src/app/main.cpp 의 --run/--window/--frames/--ui-script/--ui-report/--set (R1-2a, 2026-10-28).
/// 미션·그리기·소리의 전체 연결 전에는 <c>--mission</c>·<c>--view</c> 등을 받지 않는다. 설정은 시작 Config에 전달한다.
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

    /// <summary>고정 시각 모드의 프레임당 가상 밀리초(1~60000). 0이면 기본 OS 시계다. --fixed-frame-ms 로 정한다.</summary>
    public uint FixedFrameMs { get; private set; }

    /// <summary>고정 시각 원점의 유닉스 초. null이면 기본 2000-01-01이다. --fixed-unix-seconds 로 정하며 --fixed-frame-ms 없이 단독으로 쓸 수 없다.</summary>
    public int? FixedUnixSeconds { get; private set; }

    /// <summary>UI 스크립트 경로. 없으면 null이다. --ui-script 로 정한다.</summary>
    public string? UiScriptPath { get; private set; }

    /// <summary>건너뛴 관측 보고서 경로. 없으면 null이다. --ui-report 로 정한다.</summary>
    public string? UiReportPath { get; private set; }

    /// <summary>음향 장치를 만들지 않는지. --mute-audio 이면 참이다.</summary>
    /// <remarks>2026-11-12 0.384에서 cpppj `--mute-audio`와의 L4a 비교를 맞추려고 추가했다.
    /// 음소거이면 음향 장치 준비 판정(옵션 메뉴의 소리 행 사용 여부)이 거짓이다. 실제 무음향화는 R6에서 잇는다.</remarks>
    public bool MuteAudio { get; private set; }

    /// <summary>--set 으로 받은 현재 설정. 같은 이름은 마지막 값이다.</summary>
    public IReadOnlyDictionary<string, string> Settings => _settings;

    /// <summary>같은 현재 설정을 원본 Config 시작 인자 형식으로 반환한다.</summary>
    public string SettingsText => string.Join(';', _settings.Select(pair => pair.Key + "=" + pair.Value));

    /// <summary>--set 설정 뭉치의 실제 저장소.</summary>
    private readonly Dictionary<string, string> _settings = new(StringComparer.Ordinal);
    private bool _widthExplicit, _heightExplicit, _fpsExplicit, _fullscreenExplicit;

    /// <summary>직접 인자가 없는 값에 현재 설정 파일의 저장값을 적용한다.</summary>
    public void ApplyStartupConfiguration(ConfigInterface config)
    {
        int width = config.GetInt("SCREENW"), height = config.GetInt("SCREENH"), fps = config.GetInt("maxFPS");
        if (!_widthExplicit && width > 0) Width = width;
        if (!_heightExplicit && height > 0) Height = height;
        if (!_fpsExplicit && fps > 0) FramesPerSecond = fps;
        if (!_fullscreenExplicit && config.Configuration.Get("startInFullScreen") != null) Fullscreen = config.GetInt("startInFullScreen") != 0;
    }

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
                    options._fullscreenExplicit = true;
                    break;
                case "--fullscreen":
                    options.Fullscreen = true;
                    options._fullscreenExplicit = true;
                    break;
                case "--width":
                    options.Width = TakePositiveInt(args, ref i, "--width");
                    options._widthExplicit = true;
                    break;
                case "--height":
                    options.Height = TakePositiveInt(args, ref i, "--height");
                    options._heightExplicit = true;
                    break;
                case "--fps":
                    options.FramesPerSecond = TakePositiveInt(args, ref i, "--fps");
                    options._fpsExplicit = true;
                    break;
                case "--frames":
                    options.Frames = TakeNonNegativeLong(args, ref i, "--frames");
                    break;
                case "--fixed-frame-ms":
                    options.FixedFrameMs = TakeFixedFrameMs(args, ref i);
                    break;
                case "--fixed-unix-seconds":
                    options.FixedUnixSeconds = TakeUnixSeconds(args, ref i);
                    break;
                case "--ui-script":
                    options.UiScriptPath = TakeValue(args, ref i, "--ui-script");
                    break;
                case "--ui-report":
                    options.UiReportPath = TakeValue(args, ref i, "--ui-report");
                    break;
                case "--mute-audio":
                    options.MuteAudio = true;
                    break;
                case "--set":
                    options.ApplySettings(TakeValue(args, ref i, "--set"));
                    break;
                default:
                    throw new ArgumentException($"알 수 없는 호스트 인자: {args[i]}", nameof(args));
            }
        }
        // 화면 크기와 프레임 속도는 --set 과 직접 인자 중 뒤에 나온 값이 이긴다
        if (options.FixedUnixSeconds.HasValue && options.FixedFrameMs == 0)
            throw new ArgumentException("호스트 인자 --fixed-unix-seconds 는 --fixed-frame-ms 와 함께 사용해야 합니다", nameof(args));
        return options;
    }

    /// <summary>사용법을 돌려준다. 오류 종료 때 그대로 출력한다.</summary>
    public static string Usage() =>
        "사용법: NetstormClientHost [--window|--fullscreen] [--width N] [--height N] [--fps N] [--frames N] [--fixed-frame-ms N] [--fixed-unix-seconds S] [--ui-script steps.tsv] [--ui-report report.tsv] [--mute-audio] [--set \"k=v;k=v...\"]";

    /// <summary>고정 시각 프레임 밀리초(1~60000)를 읽는다.</summary>
    private static uint TakeFixedFrameMs(string[] args, ref int index)
    {
        string value = TakeValue(args, ref index, "--fixed-frame-ms");
        if (!uint.TryParse(value, out uint number) || number == 0 || number > HostTimeSource.MaxFrameMilliseconds)
            throw new ArgumentException($"호스트 인자 --fixed-frame-ms 는 1~60000이어야 합니다: {value}", nameof(args));
        return number;
    }

    /// <summary>고정 시각 원점의 유닉스 초(0 이상)를 읽는다.</summary>
    private static int TakeUnixSeconds(string[] args, ref int index)
    {
        string value = TakeValue(args, ref index, "--fixed-unix-seconds");
        if (!int.TryParse(value, out int number) || number < 0)
            throw new ArgumentException($"호스트 인자 --fixed-unix-seconds 는 0 이상이어야 합니다: {value}", nameof(args));
        return number;
    }

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
                    _widthExplicit = true;
                }
                else if (key == "SCREENH")
                {
                    Height = number;
                    _heightExplicit = true;
                }
                else
                {
                    FramesPerSecond = number;
                    _fpsExplicit = true;
                }
            }
        }
    }
}
