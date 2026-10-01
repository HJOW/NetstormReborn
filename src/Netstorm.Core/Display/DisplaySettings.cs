using System.Text.Json;
using System.Text.Json.Serialization;

namespace Netstorm.Core.Display;

/// <summary>
/// 사용자별 표시 설정 (전체화면·해상도 높이·와이드 처리·가장자리 스크롤). 사용자 설정 폴더의 JSON 으로 저장한다.
/// 원본 <c>options.cfg</c> 는 건드리지 않는다. 원본은 전체화면 저장 뒤 재실행이 실패했으므로(LEFT_JOBS 1.4절)
/// <see cref="StartupInProgress"/> 표식으로 "직전 시작이 끝나지 못했으면 창 모드로 시작"하는 안전장치를 둔다.
/// </summary>
public sealed class DisplaySettings
{
    /// <summary>설정 파일 이름</summary>
    public const string FileName = "settings.json";

    /// <summary>사용자 설정 폴더 이름 (Windows: %APPDATA%, Linux: $XDG_CONFIG_HOME 또는 ~/.config 아래)</summary>
    public const string FolderName = "NetstormReborn";

    /// <summary>창 모드 기본 크기: 원본 기본 해상도 1024×768</summary>
    public const int DefaultWindowWidth = 1024;

    /// <summary>창 모드 기본 높이</summary>
    public const int DefaultWindowHeight = 768;

    /// <summary>원본 Options 의 볼륨 단계 하한 (Sound Effect Volume >·Music Volume > 1~5)</summary>
    public const int MinimumVolume = 1;

    /// <summary>원본 볼륨 단계 상한</summary>
    public const int MaximumVolume = 5;

    /// <summary>창 크기 하한 (이보다 작으면 UI 가 깨진다)</summary>
    private const int MinimumWindowSize = 320;

    /// <summary>창 크기 상한 (손상된 설정 값 방어)</summary>
    private const int MaximumWindowSize = 16384;

    /// <summary>JSON 직렬화 옵션: 읽기 쉬운 들여쓰기, 열거형은 이름 문자열, 읽을 때는 대소문자 무시</summary>
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        PropertyNameCaseInsensitive = true,
        Converters = { new JsonStringEnumConverter() },
    };

    /// <summary>전체화면(테두리 없는 전체 화면 창)으로 실행할지 여부</summary>
    public bool Fullscreen { get; set; }

    /// <summary>논리 높이: 원본 해상도 480·600·768 중 하나</summary>
    public int ViewHeight { get; set; } = ScreenLayoutCalculator.DefaultViewHeight;

    /// <summary>화면비가 4:3 보다 넓을 때의 처리 방식</summary>
    public WideScreenMode WideScreen { get; set; } = WideScreenMode.Extend;

    /// <summary>풀스크린에서 화면 끝 스크롤을 쓸지 여부 (원본 edgeScroll, 기본 켜짐)</summary>
    public bool EdgeScroll { get; set; } = true;

    /// <summary>가장자리 스크롤 속도 상한, 프레임당 픽셀 (원본 edgeScrollSpeed)</summary>
    public int EdgeScrollSpeed { get; set; } = EdgeScrollController.DefaultMaxSpeed;

    /// <summary>창 모드 폭 (창 크기를 바꾸면 기억한다)</summary>
    public int WindowWidth { get; set; } = DefaultWindowWidth;

    /// <summary>창 모드 높이</summary>
    public int WindowHeight { get; set; } = DefaultWindowHeight;

    /// <summary>효과음을 켤지 (원본 Options "Sound On", setup.cfg sound = 1)</summary>
    public bool SoundOn { get; set; } = true;

    /// <summary>배경음악을 켤지 (원본 Options "Play Music", setup.cfg music = 1)</summary>
    public bool PlayMusic { get; set; } = true;

    /// <summary>원본 Options의 독립적인 바람 소리 켜짐 상태.</summary>
    public bool WindNoise { get; set; } = true;

    /// <summary>스테레오 PCM의 좌·우 채널을 교환할지.</summary>
    public bool SpeakerSwap { get; set; }

    /// <summary>효과음 볼륨 단계 1~5 (원본 options.cfg soundVolume 기본 "3")</summary>
    public int SoundVolume { get; set; } = 3;

    /// <summary>음악 볼륨 단계 1~5 (원본 options.cfg musicVolume 기본 "2")</summary>
    public int MusicVolume { get; set; } = 2;

    /// <summary>
    /// 시작 처리 중임을 뜻하는 표식. 시작할 때 true 로 저장하고 첫 프레임을 그린 뒤 false 로 되돌린다.
    /// true 인 채로 읽히면 직전 실행이 화면 초기화에서 죽은 것이므로 전체화면을 끄고 시작한다.
    /// </summary>
    public bool StartupInProgress { get; set; }

    /// <summary>기본 설정 파일 경로 (사용자 설정 폴더). 환경 변수 <c>NETSTORM_SETTINGS_DIR</c> 로 폴더를 바꿀 수 있다.</summary>
    public static string DefaultPath()
    {
        string? overrideDir = Environment.GetEnvironmentVariable("NETSTORM_SETTINGS_DIR");
        string directory = string.IsNullOrWhiteSpace(overrideDir)
            ? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), FolderName)
            : overrideDir;
        return Path.Combine(directory, FileName);
    }

    /// <summary>
    /// 설정 파일을 읽는다. 파일이 없거나 손상됐으면 기본 설정을 돌려주며 예외를 던지지 않는다.
    /// 읽은 값은 <see cref="Normalize"/> 로 허용 범위에 맞춘다.
    /// </summary>
    /// <param name="path">설정 파일 경로</param>
    public static DisplaySettings Load(string path)
    {
        try
        {
            if (File.Exists(path))
            {
                var loaded = JsonSerializer.Deserialize<DisplaySettings>(File.ReadAllText(path), JsonOptions);
                if (loaded != null)
                {
                    loaded.Normalize();
                    return loaded;
                }
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException)
        {
            // 읽을 수 없는 설정은 무시하고 기본값으로 시작한다 (설정 때문에 실행이 막히지 않게).
        }
        return new DisplaySettings();
    }

    /// <summary>
    /// 설정 파일을 저장한다. 임시 파일에 쓴 뒤 바꿔치기해서 저장 도중 종료돼도 기존 파일이 깨지지 않는다.
    /// 저장에 실패해도 게임을 멈추지 않고 false 를 돌려준다.
    /// </summary>
    /// <param name="path">설정 파일 경로</param>
    public bool Save(string path)
    {
        try
        {
            string? directory = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }
            string temporary = path + ".tmp";
            File.WriteAllText(temporary, JsonSerializer.Serialize(this, JsonOptions));
            File.Move(temporary, path, overwrite: true);
            return true;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>값을 허용 범위로 맞춘다: 논리 높이는 원본 해상도 목록에서 가장 가까운 값, 창 크기와 속도는 범위 제한.</summary>
    public void Normalize()
    {
        // 논리 높이는 목록에 없는 값이면 가장 가까운 원본 해상도 높이로 바꾼다.
        if (!ScreenLayoutCalculator.RenderHeights.Contains(ViewHeight))
            ViewHeight = ScreenLayoutCalculator.ViewHeights.MinBy(h => Math.Abs(h - ViewHeight));
        WindowWidth = Math.Clamp(WindowWidth, MinimumWindowSize, MaximumWindowSize);
        WindowHeight = Math.Clamp(WindowHeight, MinimumWindowSize, MaximumWindowSize);
        EdgeScrollSpeed = Math.Clamp(EdgeScrollSpeed, 0, 200);
        SoundVolume = Math.Clamp(SoundVolume, MinimumVolume, MaximumVolume);
        MusicVolume = Math.Clamp(MusicVolume, MinimumVolume, MaximumVolume);
        if (!Enum.IsDefined(WideScreen))
        {
            WideScreen = WideScreenMode.Extend;
        }
    }
}
