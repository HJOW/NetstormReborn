using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>실제 데이터의 설정·팔레트·메뉴 타이틀을 같은 Client 수명에서 보관한다.
/// 근거: cpppj/src/client/ClientMain.cpp와 UberGump.cpp의 fortPal 및 titleMenu 적재.</summary>
public sealed class HostStartupScene
{
    /// <summary>서명 뒤 UTF-8 BOM을 명시한 사용자 파일은 잘못된 바이트를 치환하지 않고 전체 덧읽기를 거부한다.</summary>
    private static readonly UTF8Encoding UserOptionsUtf8 = new(false, true);
    /// <summary>게임 데이터의 파일과 아카이브를 공유하는 읽기 장치.</summary>
    public GameFileSystem Files { get; }
    /// <summary>설정 적재 전 최초 확인과 이후 State가 공유하는 같은 Client CD 소유자다.</summary>
    public ClientCdGlobals CdGlobals { get; } = new();
    /// <summary>설정 적재 전 감지와 이후 Game가 공유하는 같은 Client 언어 전역이다.</summary>
    public ClientLanguageGlobals LanguageGlobals { get; } = new();
    /// <summary>같은 파일 장치에서 초기화한 원본 설정.</summary>
    public ConfigInterface Configuration { get; }
    /// <summary>원본 options.cfg가 놓인 실제 설치 경로다.</summary>
    private readonly string _optionsPath;
    /// <summary>사용자 루트의 옵션 파일 경로다. 지정하면 읽기·쓰기 모두 여기를 쓰고 설치 경로는 건드리지 않는다.</summary>
    private readonly string? _userOptionsPath;
    /// <summary>같은 클라이언트 수명에서 현재 Tell 이름을 보존한다.</summary>
    public TellSectionState TellState { get; } = new();
    /// <summary>최초 팔레트 적용 위치에서 선택한 자산 경로다. 화면의 현재 날씨 팔레트와 구별한다.</summary>
    private string _palettePath;
    /// <summary>입자 색 띠가 빌리는 최초 불변 자산이다. 실제 FILE 화면 로더와 별도로 필요할 때만 해독한다.</summary>
    private GamePalette? _palette;
    /// <summary>CPP GameAssets.Palette처럼 최초 자산을 유지하며 화면 파일 적재를 대신하지 않는다.</summary>
    public GamePalette Palette => _palette ??= new GamePalette(Files.ReadAllBytes(_palettePath));
    /// <summary>새 제품 Credits 상세를 같은 Config 수명에 보관하는 등록 객체다.</summary>
    private readonly Config _productCredits;
    /// <summary>구름(Kencloud) 전역·주소 공간 소유자다. 메인 메뉴 진입의 팔레트 전환이 구름을 만들고 이동시킨다(0.381).</summary>
    public ClientKencloudGlobals Cloud { get; } = new();
    /// <summary>구름 생성이 소비하는 같은 게임 난수다. 호스트의 상태 덤프·세션도 같은 객체를 쓴다(0.381).</summary>
    public GameRandom Random { get; }
    /// <summary>시작 팔레트와 팔레트 전환을 마친 화면이다. 새 화면이 오면 다시 한다.</summary>
    private Screen? _preparedScreen;
    /// <summary>설정의 titleMenu GIF 인덱스와 불투명도.</summary>
    private readonly Netstorm.Client.Display.IndexedImage _title;

    /// <summary>실제 설치 경로의 파일과 설정에서 시작 화면 자료를 읽는다.</summary>
    /// <param name="dataDirectory">게임 데이터와 netstorm.tarc가 있는 디렉터리.</param>
    /// <param name="settings">원본 시작 설정 인자. 파일 설정보다 먼저 읽는다.</param>
    /// <param name="random">구름 생성이 소비할 같은 게임 난수다. null이면 새로 만든다(0.381에서 추가).</param>
    /// <param name="userOptionsPath">사용자 루트의 옵션 파일 경로. 지정하면 설치 options.cfg 위에 덧읽고 저장도 여기로 한다(5차). null이면 종전 설치 경로를 쓴다.</param>
    public HostStartupScene(string dataDirectory, string settings = "", GameRandom? random = null, string? userOptionsPath = null)
        : this(dataDirectory, settings, random, userOptionsPath, null) { }

    /// <summary>같은 파일 이름 소유자를 만든 뒤 최초 CD 확인→설정 Startup 순서로 실제 시작 자료를 적재한다.</summary>
    /// <param name="dataDirectory">실제 게임 자료 디렉터리다.</param>
    /// <param name="settings">원문 시작 설정 인자다.</param>
    /// <param name="random">구름/게임이 공유할 같은 난수다.</param>
    /// <param name="userOptionsPath">설치 경로에 덧읽을 사용자 설정 경로다.</param>
    /// <param name="initializeCd">설정 전에 CD를 확인하는 실제 플랫폼 경계다. null이면 독립 조합의 BSS 값을 유지한다.</param>
    public HostStartupScene(string dataDirectory, string settings, GameRandom? random, string? userOptionsPath,
        Action<ClientCdGlobals, RawFileName>? initializeCd)
        : this(dataDirectory, settings, random, userOptionsPath, initializeCd, null) { }

    /// <summary>같은 파일 이름 생성→최초 CD 확인→OS 언어 감지→설정 적재의 실제 상대 순서다.</summary>
    public HostStartupScene(string dataDirectory, string settings, GameRandom? random, string? userOptionsPath,
        Action<ClientCdGlobals, RawFileName>? initializeCd, Action<ClientLanguageGlobals>? detectLanguage)
    {
        Random = random ?? new GameRandom();
        ArgumentException.ThrowIfNullOrEmpty(dataDirectory);
        if (!GameDataLocator.IsDataDirectory(dataDirectory)) throw new DirectoryNotFoundException("게임 데이터 디렉터리 또는 netstorm.tarc가 없다: " + dataDirectory);
        Files = GameFileSystem.Open(dataDirectory);
        initializeCd?.Invoke(CdGlobals, Files.FileNameOwner);
        detectLanguage?.Invoke(LanguageGlobals);
        _optionsPath = Path.Combine(Path.GetFullPath(dataDirectory), "d", "options.cfg");
        _userOptionsPath = string.IsNullOrEmpty(userOptionsPath) ? null : Path.GetFullPath(userOptionsPath);
        Configuration = new ConfigInterface(Files.TryReadAllBytes);
        Configuration.Startup(new ConfigInterface.StartupOptions { InstallDir = Path.GetFullPath(dataDirectory), CommandLine = settings });
        OverlayUserOptions();
        // 원문 Startup의 최종 경로 쓰기처럼 오래된 사용자 END 저장값보다 이번 실제 설치 루트를 우선한다.
        Configuration.Set("InstallDir", Path.GetFullPath(dataDirectory));
        Configuration.Set("CDDir", Path.GetFullPath(dataDirectory));
        // 새 제품 표기 11.01을 적용하고 추가 Credits 상세를 같은 수명에 등록한다
        // (2026-11-03 11.01 요구에서 추가했다. 원본 데이터 판본은 바꾸지 않는다)
        ProductPresentation.ApplyProductVersion(Configuration);
        _productCredits = new Config(Configuration.Registry, ProductVersion.CreditsObject);
        _productCredits.Append(ProductPresentation.PatchCreditsBody);
        string palettePath = Configuration.PathSpec("GamePalSpec", Configuration.PathSpec("fortPal"));
        string titlePath = Configuration.PathSpec("BackgroundGifSpec", "titleMenu");
        if (palettePath.Length == 0 || titlePath.Length == 0) throw new InvalidDataException("시작 화면 팔레트 또는 타이틀 경로 설정이 없다");
        // 구름 옵션(cloudiness·cloudSpeed·cloudFury 등)을 읽는다. 구름 파일은 메인 메뉴 진입의 팔레트 전환에서 읽는다(원문 435220 위치, 0.381).
        Cloud.ReadOptions(Configuration);
        _palettePath = palettePath;
        _title = Netstorm.Client.Display.GifImage.DecodeGif(Files.ReadAllBytes(titlePath));
    }

    /// <summary>사용자 파일 전체를 먼저 해석한 뒤 options.cfg 이후의 유효 키 중복만 지우고 같은 옵션 위치에 원문 본문을 넣는다.</summary>
    /// <remarks>5차 사용자 덮어쓰기를 Host에서 처리하고 Core의 첫 키 우선·Append 문법은 바꾸지 않는다. 실패한 파일은 설치 값을 보존한다.</remarks>
    private void OverlayUserOptions()
    {
        if (_userOptionsPath is null) return;
        byte[] bytes;
        try
        {
            bytes = File.ReadAllBytes(_userOptionsPath);
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or NotSupportedException)
        {
            return;
        }
        using var user = new Config(new ConfigRegistry());
        var keys = new List<string>();
        try
        {
            ValidateUserOptionsEncoding(bytes);
            user.LoadBytes(bytes);
            // 값은 다시 쓰지 않고 원문 GetRaw가 인식하는 선언의 키만 수집한다. 중복 본문은 원래 순서를 유지한다.
            foreach (string line in user.Text.Split('\n'))
            {
                string? key = UserOptionDeclaration(user, line);
                if (key is not null) keys.Add(key);
            }
        }
        catch (Exception exception) when (exception is ArgumentException or InvalidDataException or DecoderFallbackException)
        {
            return;
        }
        // ARGS와 앞선 user.cfg는 원문 우선순위를 보존한다. 사용자 루트는 설치 options.cfg 위치에서만 우선한다.
        string original = Configuration.Configuration.Text;
        int? options = Config.FindSection(original, 0, "[options.cfg]");
        if (options is null) return;
        string installedBody = Configuration.Configuration.Section("options.cfg") ?? "";
        using var installed = new Config(new ConfigRegistry());
        installed.Assign(installedBody);
        using var fallback = new Config(new ConfigRegistry());
        // 설치의 실제 원시 선언만 보충한다. 설치 주석/빈줄/단독서명을 매 재개 때 사용자 저장물에 복제하지 않는다.
        foreach (string line in installedBody.Split('\n'))
            if (UserOptionDeclaration(installed, line) is not null) fallback.Append(line);
        using var suffix = new Config(new ConfigRegistry());
        suffix.Assign(fallback.Text + original[(options.Value + installedBody.Length)..]);
        // 전체 디코드/선언 확인 뒤에도 실제 설정은 아직 바꾸지 않는다. 옵션 이후 같은 키의 중복만 모두 제거한다.
        foreach (string key in keys) suffix.RemoveKey(key);
        using var merged = new Config(new ConfigRegistry());
        merged.Assign(original[..options.Value]);
        merged.Append(user.Text);
        merged.Append(suffix.Text);
        Configuration.Configuration.Assign(merged.Text);
    }

    /// <summary>선언의 키만 찾고 원문 GetRaw로 확인한다. 원시 값·주석·백틱·중복 순서는 다시 직렬화하지 않는다.</summary>
    private static string? UserOptionDeclaration(Config config, string line)
    {
        int equal = line.IndexOf('=');
        if (equal < 0) return null;
        string key = line[..equal].Trim(' ', '\t');
        if (key.Length == 0 || key.StartsWith("//", StringComparison.Ordinal) || key[0] == '[') return null;
        return config.GetRaw(key) is not null ? key : null;
    }

    /// <summary>원문 XOR와 첫NUL 경계를 유지하고 명시 UTF-8 BOM 본문만 엄격검증한다. BOM 없는 원본 CP1252는 그대로 허용한다.</summary>
    private static void ValidateUserOptionsEncoding(byte[] bytes)
    {
        byte[] plain = (byte[])bytes.Clone();
        bool encoded = plain.Length >= 3 && plain[0] == 0 && plain[2] == 0;
        if (encoded) XorCipher.Apply(plain);
        int nul = Array.IndexOf(plain, (byte)0);
        ReadOnlySpan<byte> text = plain.AsSpan(0, nul < 0 ? plain.Length : nul);
        bool signature = text.StartsWith("mQdsT"u8);
        if (encoded && !signature) throw new InvalidDataException("사용자 옵션 파일의 실제 XOR 서명 누락");
        if (signature) text = text[ConfigTextOps.Signature.Length..];
        if (text.StartsWith(new byte[] { 0xef, 0xbb, 0xbf })) _ = UserOptionsUtf8.GetString(text[3..]);
    }

    /// <summary>현재 설정을 옵션 파일에 기록한다. 사용자 경로가 있으면 그곳에만 쓰고 설치 경로는 건드리지 않는다.</summary>
    public void SaveOptions()
    {
        if (_userOptionsPath is not null)
        {
            string? directory = Path.GetDirectoryName(_userOptionsPath);
            if (!string.IsNullOrEmpty(directory)) Directory.CreateDirectory(directory);
            HostOptionsFile.Save(Configuration, (_, bytes) => File.WriteAllBytes(_userOptionsPath, bytes));
            return;
        }

        HostOptionsFile.Save(Configuration, (_, bytes) => File.WriteAllBytes(_optionsPath, bytes));
    }

    /// <summary>같은 시작 Config와 파일에서 Tell을 조회하고 현재 HTML 창 생성기에 전달한다.</summary>
    /// <param name="target">원본 BYTE 표현의 섹션 이름과 선택적 매개변수.</param>
    /// <param name="suppress">원본 창 생성 억제 값.</param>
    /// <param name="mission">현재 미션 섹션 조회. 미션이 없으면 null.</param>
    /// <param name="show">같은 수명에서 동기 실행하는 HTML 창 생성기.</param>
    public (bool Found, uint Serial) ShowTell(string target, uint suppress, Func<string, string?>? mission,
        Func<string, uint, uint, uint, uint> show) => TellSection.ShowInConfig(TellState, target, suppress, Configuration, mission, show);

    /// <summary>
    /// 시작 팔레트와 메인 메뉴 진입의 팔레트 전환(구름 생성·행 이동 포함)을 실행하고, 구름을 전체 화면에 그린 뒤 타이틀을 640×480 기준 중심에 그린다.
    /// 화면이 새로 오면 시작 팔레트(ClientMain 38dc0, clear 1)부터 다시 읽는다. 구름·팔레트 이름은 같은 소유자(<see cref="Cloud"/>)에 남아 같은 이름 전환은 조기 반환한다.
    /// </summary>
    /// <param name="screen">출력 장치가 연결된 8비트 논리 화면.</param>
    /// <remarks>
    /// 0.381에서 512×512 구름 타일을 원점 0에 그대로 반복하던 임시 그리기를 실제 Kencloud(cpppj 팔레트 전환 + <c>DrawKencloud</c>)로 바꿨다.
    /// 첫 L4a 비교(0.380)에서 cloudSpeed(2)번 행 이동이 없어 구름 띠 24개가 1픽셀 어긋나던 차이를 닫는다.
    /// </remarks>
    /// <param name="prepareColors">최초 팔레트 직후 실제 Client의 초기화·색표 소유자를 연결한다. 독립 장면에는 생략한다.</param>
    /// <param name="rebuildColors">fort 팔레트 전환 뒤 같은 고정 입력으로 실제9표를 갱신한다.</param>
    /// <param name="loadPalette">실제10.78 Game가 공급하는 현재 FILE 로더다. 없으면 독립 장면의 값형 입구를 쓴다.</param>
    public void Draw(Screen screen, Action<Screen>? prepareColors = null, Action<Screen>? rebuildColors = null,
        Action<Screen, string, uint>? loadPalette = null)
    {
        ArgumentNullException.ThrowIfNull(screen);
        if (!ReferenceEquals(_preparedScreen, screen))
        {
            // 모드가 정해지지 않은 화면에서는 검정 채움(clear)을 쓸 수 없어 0으로 읽는다. 호스트는 모드를 정한 뒤 불러 원문대로 1이다.
            string path = Configuration.PathSpec("GamePalSpec", Configuration.PathSpec("fortPal"));
            if (path.Length == 0) throw new InvalidDataException("시작 화면 팔레트 경로 설정이 없다");
            // 최초 실제 선택 경로를 입자용 고정 자산에도 빌린다. 재생성은 이미 만든 자산을 바꾸지 않는다.
            if (_preparedScreen is null && _palette is null) _palettePath = path;
            uint clear = (screen.Flags & ScreenMode.ModeSet) != 0 ? 1u : 0u;
            if (loadPalette is null) screen.LoadPalette(Palette, clear);
            else loadPalette(screen, path, clear);
            prepareColors?.Invoke(screen);
            _preparedScreen = screen;
            ChangeFortPalette(screen, rebuildColors, loadPalette);
        }
        // 구름: 원문 렌더러 앞단이 모드 변경·force 때 하는 전체 표시 요청 → 현재 화면 전체(카메라 원점 0) 표시 → 정리 순서다.
        Cloud.MarkDirty();
        Cloud.Draw(screen, [0u, 0u, (uint)screen.Width, (uint)screen.Height, 0u, 0u], Files, Configuration, Random);
        Cloud.CleanDirty();
        byte[] pixels = screen.Lock();
        try
        {
            int left = (screen.Width - 640) / 2, top = (screen.Height - 480) / 2;
            // 화면에 들어오는 각 GIF 행과 불투명 픽셀만 원본 색 번호로 복사한다.
            for (int y = 0; y < _title.Height; y++)
            {
                int targetY = top + y;
                if (targetY < 0 || targetY >= screen.Height) continue;
                // GIF의 투명 픽셀은 현재 화면을 보존한다.
                for (int x = 0; x < _title.Width; x++)
                {
                    int targetX = left + x;
                    if (targetX < 0 || targetX >= screen.Width) continue;
                    int source = y * _title.Width + x;
                    if (_title.Opacity[source] != 0) pixels[targetY * screen.Pitch + targetX] = _title.Indices[source];
                }
            }
        }
        finally { screen.Unlock(); }
        screen.Update(new ScreenRect(0, 0, screen.Width, screen.Height));
    }

    /// <summary>
    /// 004b6100 팔레트 전환(<c>Client::ChangeFortPalette</c>)을 현재 fortPal 이름으로 실행한다. 이름이 같으면 아무것도 하지 않고,
    /// 다르면 이름 복사·구름 반납/생성·cloudSpeed번 행 이동·GamePalSpec 팔레트 읽기(clear 1)·색 표 갱신·dirty를 원문 순서로 한다.
    /// </summary>
    /// <param name="screen">팔레트를 읽을 화면이다.</param>
    /// <param name="rebuildColors">실제 Client의 고정 입력/현재9표를 갱신하는 장치다. 독립 장면에는 없다.</param>
    /// <remarks>
    /// 실제 Game은 같은 최초 ColorSort 입력과9표를 갱신한다. 독립 장면은 Screen의 독립 계산을 사용한다.
    /// 진단 로그(<c>Palette 29 init palette</c>)는 아직 호스트 로그 장치가 없어 남기지 않는다.
    /// </remarks>
    private void ChangeFortPalette(Screen screen, Action<Screen>? rebuildColors, Action<Screen, string, uint>? loadPalette)
    {
        byte[] name = Encoding.Latin1.GetBytes(Configuration.PathSpec("fortPal") + '\0');
        var devices = new FortPaletteTransitionHooks
        {
            Log = (_, _) => { },
            LoadPalette = (path, clear) =>
            {
                int end = path.IndexOf((byte)0);
                if (end < 0) throw new ArgumentOutOfRangeException(nameof(path), "전체 팔레트 현재 경로 NUL 누락");
                string file = Encoding.Latin1.GetString(path[..end]);
                uint selectedClear = (screen.Flags & ScreenMode.ModeSet) != 0 ? clear : 0u;
                if (loadPalette is null) screen.LoadPalette(new GamePalette(Files.ReadAllBytes(file)), selectedClear);
                else loadPalette(screen, file, selectedClear);
            },
            BuildMaps = () => rebuildColors?.Invoke(screen),
        };
        Cloud.Transition(name, Files, Configuration, Random, devices);
    }
}
