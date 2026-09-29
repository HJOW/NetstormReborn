using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Display;

namespace Netstorm.Game;

/// <summary>
/// 게임 본체. 기본 확인 화면, --map 맵 뷰어, --sprites 셰이프 뷰어를 제공한다.
/// </summary>
internal sealed class NetstormGame : Microsoft.Xna.Framework.Game
{
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
    private readonly DisplayManager _display;
    private readonly EdgeScrollController _edgeScroll = new();
    private readonly string? _screenshotPath;
    /// <summary>스크린샷을 저장할 프레임 번호 (--screenshot-frames, 기본 ScreenshotDelayFrames)</summary>
    private readonly int _screenshotFrames;
    private readonly string? _mapName;
    private readonly string? _spriteName;
    private readonly string? _languageName;
    private readonly string? _spriteFrame;
    private readonly string? _spritePalette;
    private readonly bool _spritePlay;
    private readonly bool _spriteProperties;
    private FortMapViewer? _mapViewer;
    private SpriteBrowser? _spriteBrowser;
    private SpriteBatch? _batch;
    private FontSystem? _fonts;
    private readonly List<SpriteAnimation> _animations = [];
    private readonly List<string> _statusLines = [];
    private int _frameCount;
    private KeyboardState _previousKeyboard;
    /// <summary>모드별 창 제목 (화면 상태 문구를 뒤에 붙인다)</summary>
    private string _baseTitle = "NetStorm 클론 — 개발 환경 확인";

    /// <summary>창과 그래픽 장치 설정</summary>
    public NetstormGame()
    {
        _graphics = new GraphicsDeviceManager(this)
        {
            SynchronizeWithVerticalRetrace = true,
        };
        string[] args = Environment.GetCommandLineArgs();
        string? screenshot = ParseValueArgument(args, "--screenshot");
        _screenshotPath = screenshot == null ? null : Path.GetFullPath(screenshot);
        string? screenshotFrames = ParseValueArgument(args, "--screenshot-frames");
        _screenshotFrames = screenshotFrames == null ? ScreenshotDelayFrames
            : Math.Max(1, int.Parse(screenshotFrames, System.Globalization.CultureInfo.InvariantCulture));
        _mapName = ParseValueArgument(args, "--map");
        _spriteName = ParseValueArgument(args, "--sprites");
        _languageName = ParseValueArgument(args, "--language");
        _spriteFrame = ParseValueArgument(args, "--frame");
        _spritePalette = ParseValueArgument(args, "--palette");
        _spritePlay = args.Contains("--play");
        _spriteProperties = args.Contains("--props");
        if (_mapName != null && _spriteName != null)
        {
            throw new ArgumentException("--map과 --sprites는 함께 사용할 수 없습니다.");
        }
        // 표시 설정: 사용자 설정 파일 → 명령줄 덮어쓰기. 덮어쓴 실행과 스크린샷 실행은 설정을 저장하지 않는다.
        (DisplaySettings settings, bool overridden) = ParseDisplayOptions(args);
        _display = new DisplayManager(this, _graphics, settings,
            overridden || _screenshotPath != null ? null : DisplaySettings.DefaultPath());
        _display.LayoutChanged += UpdateWindowTitle;
        IsMouseVisible = true;
        Window.AllowUserResizing = true;
        Window.Title = _baseTitle;
    }

    /// <summary>
    /// 표시 설정을 읽고 명령줄로 덮어쓴다: <c>--fullscreen</c>, <c>--windowed</c>, <c>--window 폭x높이</c>,
    /// <c>--wide extend|letterbox</c>, <c>--view-height 480|600|768</c>, <c>--no-edge-scroll</c>.
    /// </summary>
    /// <param name="args">명령줄 인자</param>
    /// <returns>설정, 명령줄로 덮어썼는지 여부</returns>
    private static (DisplaySettings Settings, bool Overridden) ParseDisplayOptions(string[] args)
    {
        // 저장된 설정을 읽고 화면 관련 옵션이 있으면 이번 실행에서만 바꾼다.
        DisplaySettings settings = DisplaySettings.Load(DisplaySettings.DefaultPath());
        bool overridden = false;
        if (args.Contains("--fullscreen"))
        {
            settings.Fullscreen = true;
            overridden = true;
        }
        if (args.Contains("--windowed"))
        {
            settings.Fullscreen = false;
            overridden = true;
        }
        if (args.Contains("--no-edge-scroll"))
        {
            settings.EdgeScroll = false;
            overridden = true;
        }
        string? window = ParseValueArgument(args, "--window");
        if (window != null)
        {
            string[] parts = window.Split('x', 'X');
            if (parts.Length != 2 || !int.TryParse(parts[0], out int width) || !int.TryParse(parts[1], out int height))
            {
                throw new ArgumentException("--window 값은 1920x1080 형식이어야 합니다.");
            }
            settings.WindowWidth = width;
            settings.WindowHeight = height;
            settings.Fullscreen = false;
            overridden = true;
        }
        string? wide = ParseValueArgument(args, "--wide");
        if (wide != null)
        {
            settings.WideScreen = wide.ToLowerInvariant() switch
            {
                "extend" => WideScreenMode.Extend,
                "letterbox" => WideScreenMode.Letterbox,
                _ => throw new ArgumentException("--wide 값은 extend 또는 letterbox 여야 합니다."),
            };
            overridden = true;
        }
        string? viewHeight = ParseValueArgument(args, "--view-height");
        if (viewHeight != null)
        {
            settings.ViewHeight = int.Parse(viewHeight, System.Globalization.CultureInfo.InvariantCulture);
            overridden = true;
        }
        settings.Normalize();
        return (settings, overridden);
    }

    /// <summary>화면 배치가 바뀌면 창 제목에 상태를 덧붙인다.</summary>
    private void UpdateWindowTitle()
    {
        Window.Title = $"{_baseTitle} | {_display.Description}";
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
            palette = resources.LoadPalette(_mapName == null && _spriteName == null ? "fortPal" : "battlePal");
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

        if (_spriteName != null)
        {
            try
            {
                _spriteBrowser = new SpriteBrowser(GraphicsDevice, shapes, resources.LoadTypes(), resources.FindPaletteNames(),
                    resources.LoadNamedPalette, _spritePalette ?? resources.PaletteName("battlePal"), _spriteName)
                {
                    Playing = _spritePlay,
                    ShowProperties = _spriteProperties,
                };
                if (_spriteFrame != null)
                {
                    _spriteBrowser.SelectFrameAt(int.Parse(_spriteFrame, System.Globalization.CultureInfo.InvariantCulture));
                }
                _baseTitle = "NetStorm 클론 — 스프라이트 뷰어";
            }
            catch (Exception error) when (error is IOException or ArgumentException or FormatException)
            {
                _statusLines.Add($"스프라이트를 읽지 못했습니다: {error.Message}");
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
        _baseTitle = $"NetStorm 클론 — 맵 뷰어: {name}";
    }

    /// <summary>입력 처리와 애니메이션 진행</summary>
    /// <param name="gameTime">경과 시간</param>
    protected override void Update(GameTime gameTime)
    {
        KeyboardState keyboard = Keyboard.GetState();
        if (keyboard.IsKeyDown(Keys.Escape))
        {
            Exit();
        }
        double dt = gameTime.ElapsedGameTime.TotalSeconds;
        _display.HandleHotkeys(keyboard, _previousKeyboard);
        _previousKeyboard = keyboard;
        MouseState rawMouse = Mouse.GetState();
        MouseState mouse = _display.ToLogical(rawMouse);
        _mapViewer?.Update(dt, mouse, EdgeScrollDelta(rawMouse, keyboard, dt));
        _spriteBrowser?.Update(dt, mouse, _display.Layout.LogicalWidth, _display.Layout.LogicalHeight);
        // 모든 애니메이션 진행
        foreach (SpriteAnimation animation in _animations)
        {
            animation.Update(dt);
        }
        base.Update(gameTime);
    }

    /// <summary>
    /// 창 픽셀 기준 마우스 위치로 가장자리 스크롤이 옮길 논리 픽셀을 구한다.
    /// 창이 비활성이거나 창 테두리가 있으면 0 이다 (원본: 전체화면에서만 동작).
    /// </summary>
    /// <param name="rawMouse">창 좌표의 마우스 상태</param>
    /// <param name="keyboard">키 상태 (Shift 확인)</param>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    private Vector2 EdgeScrollDelta(MouseState rawMouse, KeyboardState keyboard, double seconds)
    {
        _edgeScroll.MaxSpeed = _display.Settings.EdgeScrollSpeed;
        var input = new EdgeScrollInput(rawMouse.X, rawMouse.Y, _display.ScreenWidth, _display.ScreenHeight,
            _display.Settings.EdgeScroll && IsActive, _display.BorderlessScreen,
            rawMouse.LeftButton == ButtonState.Pressed,
            keyboard.IsKeyDown(Keys.LeftShift) || keyboard.IsKeyDown(Keys.RightShift),
            PopupOpen: false, TopEdgeBlocked: false);
        (double x, double y) = _edgeScroll.Update(input, seconds);
        return new Vector2((float)x, (float)y);
    }

    /// <summary>화면 그리기</summary>
    /// <param name="gameTime">경과 시간</param>
    protected override void Draw(GameTime gameTime)
    {
        // 논리 해상도 렌더 타깃에 그린 뒤 마지막에 뷰포트로 늘려 표시한다.
        _display.BeginScene(BackgroundColor);
        int width = _display.Layout.LogicalWidth;
        int height = _display.Layout.LogicalHeight;
        SpriteBatch batch = _batch!;
        batch.Begin(samplerState: SamplerState.PointClamp);

        SpriteFontBase title = _fonts!.GetFont(TitleFontSize);
        SpriteFontBase body = _fonts.GetFont(BodyFontSize);
        if (_mapViewer != null)
        {
            _mapViewer.Draw(batch, body, width, height, _display.Description);
        }
        else if (_spriteBrowser != null)
        {
            _spriteBrowser.Draw(batch, body, width, height);
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

            batch.DrawString(body, _display.Description, new Vector2(24, height - 70), Color.LightGray);
            batch.DrawString(body, "Esc: 종료 · F11: 전체화면 · F10: 와이드 처리 · F9: 해상도 높이 · F7: 가장자리 스크롤",
                new Vector2(24, height - 40), Color.Gray);
        }
        // 화면 설정을 바꿨을 때 잠깐 보이는 알림 (오른쪽 아래)
        string? notice = _display.TickNotice(gameTime.ElapsedGameTime.TotalSeconds);
        if (notice != null)
        {
            Vector2 size = body.MeasureString(notice);
            batch.DrawString(body, notice, new Vector2(width - size.X - 16, height - 34), Color.Yellow);
        }
        batch.End();
        _display.EndScene(batch);
        base.Draw(gameTime);

        _frameCount++;
        // 첫 프레임까지 그렸으면 시작에 성공한 것이므로 다음 실행이 같은 화면 모드로 시작해도 된다.
        if (_frameCount == 1)
        {
            _display.CompleteStartup();
        }
        if (_screenshotPath != null && _frameCount == _screenshotFrames)
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

    /// <summary>종료할 때 마지막 창 크기·화면 설정을 저장한다.</summary>
    /// <param name="sender">이벤트 보낸 객체</param>
    /// <param name="args">종료 이벤트 인자</param>
    protected override void OnExiting(object sender, ExitingEventArgs args)
    {
        _display.SaveOnExit();
        base.OnExiting(sender, args);
    }

    /// <summary>자원 해제</summary>
    /// <param name="disposing">관리 자원 해제 여부</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _mapViewer?.Dispose();
            _spriteBrowser?.Dispose();
            // 애니메이션 텍스처 해제
            foreach (SpriteAnimation animation in _animations)
            {
                animation.Dispose();
            }
            _batch?.Dispose();
            _fonts?.Dispose();
            _display.Dispose();
        }
        base.Dispose(disposing);
    }
}
