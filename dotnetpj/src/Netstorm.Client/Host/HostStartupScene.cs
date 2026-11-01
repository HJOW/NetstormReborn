using Netstorm.Assets;
using Netstorm.Client.Display;
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
    /// <summary>설정의 fortPal 파일 팔레트.</summary>
    private readonly GamePalette _palette;
    /// <summary>실제 512×512 Kencloud 초기 캐시의 인덱스.</summary>
    private readonly byte[] _cloud;
    /// <summary>설정의 titleMenu GIF 인덱스와 불투명도.</summary>
    private readonly Netstorm.Client.Display.IndexedImage _title;

    /// <summary>실제 설치 경로의 파일과 설정에서 시작 화면 자료를 읽는다.</summary>
    /// <param name="dataDirectory">게임 데이터와 netstorm.tarc가 있는 디렉터리.</param>
    /// <param name="settings">원본 시작 설정 인자. 파일 설정보다 먼저 읽는다.</param>
    public HostStartupScene(string dataDirectory, string settings = "")
    {
        ArgumentException.ThrowIfNullOrEmpty(dataDirectory);
        if (!GameDataLocator.IsDataDirectory(dataDirectory)) throw new DirectoryNotFoundException("게임 데이터 디렉터리 또는 netstorm.tarc가 없다: " + dataDirectory);
        Files = GameFileSystem.Open(dataDirectory);
        Configuration = new ConfigInterface(Files.TryReadAllBytes);
        Configuration.Startup(new ConfigInterface.StartupOptions { InstallDir = Path.GetFullPath(dataDirectory), CommandLine = settings });
        string palettePath = Configuration.PathSpec("GamePalSpec", Configuration.PathSpec("fortPal"));
        string titlePath = Configuration.PathSpec("BackgroundGifSpec", "titleMenu");
        if (palettePath.Length == 0 || titlePath.Length == 0) throw new InvalidDataException("시작 화면 팔레트 또는 타이틀 경로 설정이 없다");
        string cloudPath = Configuration.PathSpec("CloudFileSpec");
        if (cloudPath.Length == 0) throw new InvalidDataException("구름 경로 설정이 없다");
        var cloud = Netstorm.Client.Display.GifImage.DecodeBackgroundGif(Files.ReadAllBytes(cloudPath));
        if (cloud.Width != 512 || cloud.Height != 512) throw new InvalidDataException("구름 생성 대체 경로 미이식: 512×512 GIF 필요");
        _cloud = cloud.Indices;
        _palette = new GamePalette(Files.ReadAllBytes(palettePath));
        _title = Netstorm.Client.Display.GifImage.DecodeGif(Files.ReadAllBytes(titlePath));
    }

    /// <summary>팔레트와 초기 구름 캐시를 전체 화면에 복사한 뒤 타이틀을 640×480 기준 중심에 그린다.</summary>
    /// <param name="screen">출력 장치가 연결된 8비트 논리 화면.</param>
    public void Draw(Screen screen)
    {
        ArgumentNullException.ThrowIfNull(screen);
        screen.LoadPalette(_palette);
        byte[] pixels = screen.Lock();
        try
        {
            // 카메라 원점 0에서 전체 dirty 구름은 512×512 캐시를 화면에 반복 복사한다.
            for (int y = 0; y < screen.Height; y++)
            {
                int source = (y & 511) * 512;
                // 대상 행 너비만큼 현재 구름 색 번호를 복사한다.
                for (int x = 0; x < screen.Width; x++) pixels[y * screen.Pitch + x] = _cloud[source + (x & 511)];
            }
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
}
