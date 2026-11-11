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
    /// <summary>게임 데이터의 파일과 아카이브를 공유하는 읽기 장치.</summary>
    public GameFileSystem Files { get; }
    /// <summary>같은 파일 장치에서 초기화한 원본 설정.</summary>
    public ConfigInterface Configuration { get; }
    /// <summary>원본 options.cfg가 놓인 실제 설치 경로다.</summary>
    private readonly string _optionsPath;
    /// <summary>같은 클라이언트 수명에서 현재 Tell 이름을 보존한다.</summary>
    public TellSectionState TellState { get; } = new();
    /// <summary>설정의 fortPal 파일 팔레트.</summary>
    private readonly GamePalette _palette;
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
    public HostStartupScene(string dataDirectory, string settings = "", GameRandom? random = null)
    {
        Random = random ?? new GameRandom();
        ArgumentException.ThrowIfNullOrEmpty(dataDirectory);
        if (!GameDataLocator.IsDataDirectory(dataDirectory)) throw new DirectoryNotFoundException("게임 데이터 디렉터리 또는 netstorm.tarc가 없다: " + dataDirectory);
        Files = GameFileSystem.Open(dataDirectory);
        _optionsPath = Path.Combine(Path.GetFullPath(dataDirectory), "d", "options.cfg");
        Configuration = new ConfigInterface(Files.TryReadAllBytes);
        Configuration.Startup(new ConfigInterface.StartupOptions { InstallDir = Path.GetFullPath(dataDirectory), CommandLine = settings });
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
        _palette = new GamePalette(Files.ReadAllBytes(palettePath));
        _title = Netstorm.Client.Display.GifImage.DecodeGif(Files.ReadAllBytes(titlePath));
    }

    /// <summary>현재 설정을 원본의 options.cfg 위치에 기록한다.</summary>
    public void SaveOptions() => HostOptionsFile.Save(Configuration, (_, bytes) => File.WriteAllBytes(_optionsPath, bytes));

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
    public void Draw(Screen screen)
    {
        ArgumentNullException.ThrowIfNull(screen);
        if (!ReferenceEquals(_preparedScreen, screen))
        {
            // 모드가 정해지지 않은 화면에서는 검정 채움(clear)을 쓸 수 없어 0으로 읽는다. 호스트는 모드를 정한 뒤 불러 원문대로 1이다.
            screen.LoadPalette(_palette, (screen.Flags & ScreenMode.ModeSet) != 0 ? 1u : 0u);
            _preparedScreen = screen;
            ChangeFortPalette(screen);
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
    /// <remarks>
    /// 0.381에서 추가했다. C#에서 달라진 모양: 색 표 갱신(<c>RebuildColorSortMaps</c>·<c>RefreshPaletteConsumers</c>)은 이 호스트의 <see cref="Screen"/>이 팔레트를 읽을 때 직접 계산하므로 비운다.
    /// 진단 로그(<c>Palette 29 init palette</c>)는 아직 호스트 로그 장치가 없어 남기지 않는다.
    /// </remarks>
    private void ChangeFortPalette(Screen screen)
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
                screen.LoadPalette(new GamePalette(Files.ReadAllBytes(file)), (screen.Flags & ScreenMode.ModeSet) != 0 ? clear : 0u);
            },
            BuildMaps = () => { },
        };
        Cloud.Transition(name, Files, Configuration, Random, devices);
    }
}
