using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>
/// 게임 본체. 기본 애니메이션 확인 화면과 --map으로 선택하는 개발용 맵 뷰어를 제공한다.
/// </summary>
internal sealed class NetstormGame : Microsoft.Xna.Framework.Game
{
    /// <summary>기본 창 폭 (원본 options.cfg 의 SCREENW 기본값)</summary>
    private const int DefaultWidth = 1024;

    /// <summary>기본 창 높이 (원본 options.cfg 의 SCREENH 기본값)</summary>
    private const int DefaultHeight = 768;

    /// <summary>한국어 글꼴 파일 (출력 폴더 기준 상대 경로, AGENTS.md 지정 D2Coding)</summary>
    private const string KoreanFontPath = "fonts/D2Coding-Ver1.3.2-20180524-all.ttc";

    /// <summary>D2Coding TTC 에서 사용할 face 번호 (0 = 일반)</summary>
    private const int KoreanFontFace = 0;

    /// <summary>본문 글자 크기(px)</summary>
    private const int BodyFontSize = 20;

    /// <summary>제목 글자 크기(px)</summary>
    private const int TitleFontSize = 32;

    /// <summary>스크린샷 모드에서 저장 전에 기다릴 프레임 수 (애니메이션이 진행된 화면을 찍기 위함)</summary>
    private const int ScreenshotDelayFrames = 30;

    /// <summary>배경색 (원본 하늘색 계열)</summary>
    private static readonly Color BackgroundColor = new(28, 36, 60);

    private readonly GraphicsDeviceManager _graphics;
    private readonly string? _screenshotPath;
    private readonly string? _mapName;
    private readonly string? _languageName;
    private FortMapViewer? _mapViewer;
    private SpriteBatch? _batch;
    private FontSystem? _fonts;
    private readonly List<SpriteAnimation> _animations = [];
    private readonly List<string> _statusLines = [];
    private int _frameCount;

    /// <summary>창과 그래픽 장치 설정</summary>
    public NetstormGame()
    {
        _graphics = new GraphicsDeviceManager(this)
        {
            PreferredBackBufferWidth = DefaultWidth,
            PreferredBackBufferHeight = DefaultHeight,
            IsFullScreen = false,
            SynchronizeWithVerticalRetrace = true,
        };
        string[] args = Environment.GetCommandLineArgs();
        string? screenshot = ParseValueArgument(args, "--screenshot");
        _screenshotPath = screenshot == null ? null : Path.GetFullPath(screenshot);
        _mapName = ParseValueArgument(args, "--map");
        _languageName = ParseValueArgument(args, "--language");
        IsMouseVisible = true;
        Window.AllowUserResizing = true;
        Window.Title = "NetStorm 클론 — 개발 환경 확인";
    }

    /// <summary>글꼴과 원본 데이터를 읽는다</summary>
    protected override void LoadContent()
    {
        _batch = new SpriteBatch(GraphicsDevice);
        _fonts = new FontSystem();
        _fonts.AddFont(LoadKoreanFont());
        LoadOriginalData();
    }

    /// <summary>D2Coding TTC 에서 face 하나를 TTF 로 분리해 돌려준다</summary>
    private static byte[] LoadKoreanFont()
    {
        byte[] data = File.ReadAllBytes(Path.Combine(AppContext.BaseDirectory, KoreanFontPath));
        return TrueTypeCollection.IsCollection(data)
            ? TrueTypeCollection.ExtractFace(data, KoreanFontFace)
            : data;
    }

    /// <summary>원본 데이터 폴더를 찾아 팔레트·셰이프를 읽고 표시할 애니메이션을 만든다</summary>
    private void LoadOriginalData()
    {
        string? dataDir = GameDataLocator.FindDataDirectory();
        if (dataDir == null)
        {
            _statusLines.Add($"원본 데이터 폴더를 찾지 못했습니다. 환경 변수 {GameDataLocator.EnvironmentVariable} 로 지정하세요.");
            return;
        }
        _statusLines.Add($"원본 데이터: {dataDir}");
        GameResources resources;
        Palette palette;
        ShapeDatabase shapes;
        try
        {
            resources = new GameResources(GameFileSystem.Open(dataDir), _languageName);
            palette = resources.LoadPalette(_mapName == null ? "fortPal" : "battlePal");
            shapes = resources.LoadShapes();
        }
        catch (Exception error) when (error is IOException or ArgumentException)
        {
            _statusLines.Add($"원본 자산을 읽지 못했습니다: {error.Message}");
            return;
        }
        _statusLines.Add($"선택 언어: {resources.Language} | 용어표: {resources.LanguageConfigPath ?? "없음"}");
        _statusLines.Add($"번역표: {resources.TranslationPath ?? "없음"} | {resources.Translations.Count}개 문구");
        _statusLines.Add($"용어 확인: {resources.Settings.Expand("{vortex|Temple} / {priest|High Priest}")}");
        int frameTotal = shapes.Blocks.Sum(b => b.Frames.Count);
        _statusLines.Add($"_shapes.shp: 블록 {shapes.Blocks.Count}개, 프레임 {frameTotal}개");

        if (_mapName != null)
        {
            try
            {
                LoadMap(resources, shapes, palette, _mapName);
            }
            catch (Exception error) when (error is IOException or ArgumentException)
            {
                _statusLines.Add($"맵을 읽지 못했습니다: {error.Message}");
            }
            return;
        }

        // 표시할 애니메이션: (타입, 첫 프레임, 프레임 수, 초당 프레임)
        var samples = new (string Type, int First, int Count, double Fps)[]
        {
            ("sunCannon", 0, 28, 4),
            ("priest", 0, 24, 10),
            ("sunWalker", 0, 32, 10),
            ("windArcher", 0, 40, 10),
            ("dude", 0, 32, 10),
        };
        // 샘플마다 애니메이션을 만든다
        foreach ((string type, int first, int count, double fps) in samples)
        {
            _animations.Add(new SpriteAnimation(GraphicsDevice, shapes, palette, type, first, count, fps));
        }
    }

    /// <summary>공통 파일 시스템·설정의 fortSpec으로 맵과 타입을 읽어 뷰어를 만든다.</summary>
    private void LoadMap(GameResources resources, ShapeDatabase shapes, Palette palette, string name)
    {
        TypeCatalog catalog = resources.LoadTypes();
        _mapViewer = new FortMapViewer(GraphicsDevice, shapes, palette, resources.LoadFort(name, catalog), name,
            catalog, resources.Language);
        Window.Title = $"NetStorm 클론 — 맵 뷰어: {name}";
    }

    /// <summary>입력 처리와 애니메이션 진행</summary>
    /// <param name="gameTime">경과 시간</param>
    protected override void Update(GameTime gameTime)
    {
        if (Keyboard.GetState().IsKeyDown(Keys.Escape))
        {
            Exit();
        }
        double dt = gameTime.ElapsedGameTime.TotalSeconds;
        _mapViewer?.Update(dt);
        // 모든 애니메이션 진행
        foreach (SpriteAnimation animation in _animations)
        {
            animation.Update(dt);
        }
        base.Update(gameTime);
    }

    /// <summary>화면 그리기</summary>
    /// <param name="gameTime">경과 시간</param>
    protected override void Draw(GameTime gameTime)
    {
        GraphicsDevice.Clear(BackgroundColor);
        SpriteBatch batch = _batch!;
        batch.Begin(samplerState: SamplerState.PointClamp);

        SpriteFontBase title = _fonts!.GetFont(TitleFontSize);
        SpriteFontBase body = _fonts.GetFont(BodyFontSize);
        if (_mapViewer != null)
        {
            _mapViewer.Draw(batch, body, GraphicsDevice.Viewport.Width, GraphicsDevice.Viewport.Height);
        }
        else
        {
            batch.DrawString(title, "넷스톰 클론 — MonoGame 개발 환경 확인", new Vector2(24, 20), Color.Gold);
            batch.DrawString(body, "한글 출력: D2Coding (가나다라마바사 아자차카타파하)", new Vector2(24, 70), Color.White);
            batch.DrawString(body, "English: NetStorm: Islands at War clone", new Vector2(24, 96), Color.White);
            float y = 130;
            // 데이터 상태 문구를 한 줄씩 출력
            foreach (string line in _statusLines)
            {
                batch.DrawString(body, line, new Vector2(24, y), Color.LightGreen);
                y += 26;
            }

            // 확인 화면의 샘플 애니메이션 확대 배율.
            const float spriteScale = 2f;
            float x = 110;
            // 애니메이션을 가로로 나란히 그리고 아래에 타입 이름을 쓴다
            foreach (SpriteAnimation animation in _animations)
            {
                var anchor = new Vector2(x, 420);
                animation.Draw(batch, anchor, spriteScale);
                batch.DrawString(body, animation.Name, new Vector2(x - 50, 500), Color.LightGray);
                x += 190;
            }

            batch.DrawString(body, "Esc: 종료", new Vector2(24, GraphicsDevice.Viewport.Height - 40), Color.Gray);
        }
        batch.End();
        base.Draw(gameTime);

        _frameCount++;
        if (_screenshotPath != null && _frameCount == ScreenshotDelayFrames)
        {
            SaveScreenshot(_screenshotPath);
            Exit();
        }
    }

    /// <summary>현재 백버퍼를 PNG 로 저장한다 (검증·문서용)</summary>
    /// <param name="path">저장 경로</param>
    private void SaveScreenshot(string path)
    {
        int w = GraphicsDevice.PresentationParameters.BackBufferWidth;
        int h = GraphicsDevice.PresentationParameters.BackBufferHeight;
        var pixels = new Color[w * h];
        GraphicsDevice.GetBackBufferData(pixels);
        using var texture = new Texture2D(GraphicsDevice, w, h);
        texture.SetData(pixels);
        using FileStream stream = File.Create(path);
        texture.SaveAsPng(stream, w, h);
    }

    /// <summary>값이 필요한 명령줄 옵션을 찾고 값이 빠졌으면 오류를 보고한다.</summary>
    private static string? ParseValueArgument(string[] args, string option)
    {
        // 실행 파일 다음 인자부터 옵션과 값을 확인한다.
        for (int i = 1; i < args.Length; i++)
        {
            if (args[i] == option)
            {
                if (i + 1 == args.Length || args[i + 1].StartsWith("--", StringComparison.Ordinal))
                {
                    throw new ArgumentException($"{option} 뒤에 값이 필요합니다.");
                }
                return args[i + 1];
            }
        }
        return null;
    }

    /// <summary>자원 해제</summary>
    /// <param name="disposing">관리 자원 해제 여부</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _mapViewer?.Dispose();
            // 애니메이션 텍스처 해제
            foreach (SpriteAnimation animation in _animations)
            {
                animation.Dispose();
            }
            _batch?.Dispose();
            _fonts?.Dispose();
        }
        base.Dispose(disposing);
    }
}
