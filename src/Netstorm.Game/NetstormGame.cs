using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Display;
using Netstorm.Core.Rules;

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

    /// <summary>작은 글자 크기 (지식 창 카드 이름·수치 — 원본 카드 이름은 약 11px 글꼴)</summary>
    private const int SmallFontSize = 13;

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
    /// <summary>--mission 으로 지정한 미션 이름 (맵과 시작 조건을 미션 스크립트에서 읽는다)</summary>
    private readonly string? _missionName;

    /// <summary>현재 열린 미션 이름. 튜토리얼 버튼으로 다음 미션을 연 경우에도 재시작 대상이 현재 미션이 되게 한다.</summary>
    private string? _currentMissionName;
    /// <summary>--script 로 지정한 검증용 명령 스크립트 (맵 뷰어에서 시작할 때 실행)</summary>
    private readonly string? _scriptText;
    private readonly string? _spriteName;
    private readonly string? _languageName;
    private readonly string? _spriteFrame;
    private readonly string? _spritePalette;
    private readonly bool _spritePlay;
    private readonly bool _spriteProperties;
    private FortMapViewer? _mapViewer;

    /// <summary>원본 효과음·배경음악 재생기 (원본 데이터가 없으면 null)</summary>
    private AudioPlayer? _audio;

    /// <summary>도움말 앵커 절 (지식 상세창 본문)</summary>
    private HelpTopics? _help;
    /// <summary>튜토리얼 버튼으로 다음 미션을 열 때 다시 사용할 원본 자산.</summary>
    private GameResources? _resources;
    private ShapeDatabase? _shapes;
    private Palette? _palette;
    /// <summary>명령줄 시험 옵션을 처음 열린 맵에만 적용했는지.</summary>
    private bool _startupOptionsApplied;
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
        _missionName = ParseValueArgument(args, "--mission");
        _scriptText = ParseValueArgument(args, "--script");
        _spriteName = ParseValueArgument(args, "--sprites");
        _languageName = ParseValueArgument(args, "--language");
        _spriteFrame = ParseValueArgument(args, "--frame");
        _spritePalette = ParseValueArgument(args, "--palette");
        _spritePlay = args.Contains("--play");
        _spriteProperties = args.Contains("--props");
        if ((_mapName != null || _missionName != null) && _spriteName != null)
        {
            throw new ArgumentException("--map/--mission과 --sprites는 함께 사용할 수 없습니다.");
        }
        if (_mapName != null && _missionName != null)
        {
            throw new ArgumentException("--map과 --mission은 함께 사용할 수 없습니다 (미션이 맵을 정한다).");
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
    /// <c>--wide extend|letterbox</c>, <c>--view-height 480|600|768</c>, <c>--no-edge-scroll</c>, <c>--no-sound</c>, <c>--no-music</c>.
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
        if (args.Contains("--no-sound"))
        {
            settings.SoundOn = false;
            overridden = true;
        }
        if (args.Contains("--no-music"))
        {
            settings.PlayMusic = false;
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
            palette = resources.LoadPalette(_mapName == null && _missionName == null && _spriteName == null ? "fortPal" : "battlePal");
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
        _resources = resources;
        _shapes = shapes;
        _palette = palette;
        _help = resources.TryLoadHelp();
        DisplaySettings settings = _display.Settings;
        _audio = new AudioPlayer(dataDir, settings.SoundOn, settings.PlayMusic, settings.SoundVolume, settings.MusicVolume);
        if (_missionName == null)
        {
            // 미션 밖(개발용 기본 화면·맵 시험·스프라이트 뷰어)은 메뉴 음악이다. 미션은 LoadMission 이 전투 음악을 시작한다.
            _audio.StartMenu();
        }

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

        if (_missionName != null)
        {
            try
            {
                LoadMission(resources, shapes, palette, _missionName);
            }
            catch (Exception error) when (error is IOException or ArgumentException)
            {
                _statusLines.Add($"미션을 읽지 못했습니다: {error.Message}");
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

    /// <summary>
    /// 미션 스크립트에서 시작 조건(시작 Storm Power·지식·기술 허용·회수 금지·전투 옵션)과 맵 이름(loadFort)을 읽고,
    /// 그 맵을 게임 세션과 함께 연다. 미션으로 열면 세션 시간이 계속 흐르고 생산 규칙이 켜진다.
    /// </summary>
    private void LoadMission(GameResources resources, ShapeDatabase shapes, Palette palette, string missionName)
    {
        LoadedMission loaded = resources.TryLoadMission(missionName)
            ?? throw new ArgumentException($"미션을 찾을 수 없습니다: {missionName}");
        MissionStart start = MissionStart.FromScript(loaded.Script);
        LoadMap(resources, shapes, palette, start.LoadFort ?? missionName, start, loaded.Script);
        _currentMissionName = missionName;
        // 미션(전투)에 들어갈 때마다 원소 곡 순환을 새 난수로 시작한다 (exe FUN_00469fc0)
        _audio?.StartBattle();
        _baseTitle = $"NetStorm 클론 — 미션: {start.Title ?? missionName}";
        UpdateWindowTitle();
    }

    /// <summary>공통 파일 시스템·설정의 fortSpec으로 맵과 타입을 읽어 뷰어를 만든다.</summary>
    private void LoadMap(GameResources resources, ShapeDatabase shapes, Palette palette, string name,
        MissionStart? mission = null, MissionScript? tutorialScript = null)
    {
        TypeCatalog catalog = resources.LoadTypes();
        FortMapViewer nextViewer = new(GraphicsDevice, shapes, palette, resources.LoadFort(name, catalog), name,
            catalog, resources.Language, mission, tutorialScript, resources.Settings, _help);
        _mapViewer?.Dispose();
        _mapViewer = nextViewer;
        _baseTitle = $"NetStorm 클론 — 맵 뷰어: {name}";
        bool applyStartupOptions = !_startupOptionsApplied;
        _startupOptionsApplied = true;
        // 검증용: --placement 타입 [--probe x,y] 로 배치 시험 모드를 켠 채 시작한다.
        string[] args = Environment.GetCommandLineArgs();
        string? placement = ParseValueArgument(args, "--placement");
        if (applyStartupOptions && placement != null)
        {
            string? probe = ParseValueArgument(args, "--probe");
            (int, int)? cell = null;
            if (probe != null)
            {
                string[] parts = probe.Split(',');
                if (parts.Length != 2 || !int.TryParse(parts[0], out int px) || !int.TryParse(parts[1], out int py))
                {
                    throw new ArgumentException("--probe 값은 x,y 칸 좌표여야 합니다.");
                }
                cell = (px, py);
            }
            _mapViewer.StartPlacement(placement, cell);
        }
        // 검증용: --bridges 초 [--bridge-hold 모양,회전] [--probe x,y] 로 다리 조각 시험 모드를 켠 채 시작한다.
        string? bridges = ParseValueArgument(args, "--bridges");
        if (applyStartupOptions && bridges != null)
        {
            if (!double.TryParse(bridges, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out double warmup))
            {
                throw new ArgumentException("--bridges 값은 미리 흘릴 초여야 합니다.");
            }
            (int, int)? hold = ParsePair(ParseValueArgument(args, "--bridge-hold"), "--bridge-hold");
            (int, int)? probe = ParsePair(ParseValueArgument(args, "--probe"), "--probe");
            _mapViewer.StartBridges(warmup, hold, probe);
        }
        // 검증용: --knowledge [타입] 으로 지식 창(타입을 주면 상세창)을 연 채 시작한다.
        if (applyStartupOptions && args.Contains("--knowledge"))
        {
            // 값은 생략할 수 있다 (다음 인자가 없거나 다른 옵션이면 격자만 연다)
            int at = Array.IndexOf(args, "--knowledge");
            string? knowledgeType = at + 1 < args.Length && !args[at + 1].StartsWith("--", StringComparison.Ordinal) ? args[at + 1] : null;
            _mapViewer.StartKnowledge(knowledgeType);
        }
        // 검증용: --script "명령; 명령" 으로 세션 명령을 미리 실행한다 (예: construct windVortex 100,120; wait 17)
        if (applyStartupOptions && _scriptText != null)
        {
            _mapViewer.RunScript(_scriptText);
        }
    }

    /// <summary>"a,b" 형식의 정수 쌍을 읽는다 (값이 없으면 null)</summary>
    /// <param name="value">명령줄 값</param>
    /// <param name="option">오류 문구용 옵션 이름</param>
    private static (int, int)? ParsePair(string? value, string option)
    {
        if (value == null)
        {
            return null;
        }
        string[] parts = value.Split(',');
        if (parts.Length != 2 || !int.TryParse(parts[0], out int a) || !int.TryParse(parts[1], out int b))
        {
            throw new ArgumentException($"{option} 값은 a,b 정수 쌍이어야 합니다.");
        }
        return (a, b);
    }

    /// <summary>입력 처리와 애니메이션 진행</summary>
    /// <param name="gameTime">경과 시간</param>
    protected override void Update(GameTime gameTime)
    {
        KeyboardState keyboard = Keyboard.GetState();
        bool tutorialOpen = _mapViewer?.TutorialDialogOpen == true || _mapViewer?.KnowledgeOpen == true;
        if (keyboard.IsKeyDown(Keys.Escape) && !_previousKeyboard.IsKeyDown(Keys.Escape) && !tutorialOpen
            && _mapViewer?.IsMissionMode != true)
        {
            Exit();
        }
        double dt = gameTime.ElapsedGameTime.TotalSeconds;
        if (!tutorialOpen)
        {
            _display.HandleHotkeys(keyboard, _previousKeyboard);
        }
        _previousKeyboard = keyboard;
        MouseState rawMouse = Mouse.GetState();
        MouseState mouse = _display.ToLogical(rawMouse);
        _mapViewer?.Update(dt, mouse, EdgeScrollDelta(rawMouse, keyboard, dt),
            _display.Layout.LogicalWidth, _display.Layout.LogicalHeight);
        TutorialDialogAction? tutorialAction = _mapViewer?.TakeTutorialAction();
        if (tutorialAction != null)
        {
            HandleTutorialAction(tutorialAction);
        }
        MissionMenuAction? menuAction = _mapViewer?.TakeMissionMenuAction();
        if (menuAction != null)
        {
            HandleMissionMenuAction(menuAction.Value);
        }
        _spriteBrowser?.Update(dt, mouse, _display.Layout.LogicalWidth, _display.Layout.LogicalHeight);
        UpdateAudio();
        // 모든 애니메이션 진행
        foreach (SpriteAnimation animation in _animations)
        {
            animation.Update(dt);
        }
        base.Update(gameTime);
    }

    /// <summary>뷰어가 요청한 효과음을 틀고 배경음악 교체·스트리밍을 진행한다.</summary>
    private void UpdateAudio()
    {
        if (_audio == null)
        {
            return;
        }
        // 이번 프레임에 쌓인 효과음을 차례로 튼다
        foreach (string sound in _mapViewer?.TakeSoundCues() ?? [])
        {
            _audio.PlaySound(sound);
        }
        _audio.Update(_mapViewer?.MySacrificeInProgress ?? false);
    }

    /// <summary>안내 버튼이 요청한 미션 종료나 다음 미션 시작을 처리한다.</summary>
    private void HandleTutorialAction(TutorialDialogAction action)
    {
        if (action.Kind == TutorialDialogActionKind.LeaveBattle)
        {
            _mapViewer?.Dispose();
            _mapViewer = null;
            _audio?.StartMenu();
            _baseTitle = "NetStorm 클론 — 개발 환경 확인";
            _statusLines.Add("튜토리얼을 종료했습니다.");
            UpdateWindowTitle();
        }
        else if (action.Kind == TutorialDialogActionKind.MissionBegin)
        {
            try
            {
                LoadMission(_resources!, _shapes!, _palette!, action.Argument);
            }
            catch (Exception error) when (error is IOException or ArgumentException)
            {
                _statusLines.Add($"다음 튜토리얼을 열지 못했습니다: {error.Message}");
            }
        }
    }

    /// <summary>미션 메뉴의 재시작·재플레이는 현재 미션을 다시 로드하고, 떠나기는 개발용 기본 화면으로 돌아간다.</summary>
    private void HandleMissionMenuAction(MissionMenuAction action)
    {
        if (action is MissionMenuAction.Restart or MissionMenuAction.Replay)
        {
            try
            {
                LoadMission(_resources!, _shapes!, _palette!, _currentMissionName!);
            }
            catch (Exception error) when (error is IOException or ArgumentException)
            {
                _statusLines.Add($"미션을 다시 열지 못했습니다: {error.Message}");
            }
        }
        else if (action == MissionMenuAction.MainMenu)
        {
            _mapViewer?.Dispose();
            _mapViewer = null;
            _audio?.StartMenu();
            _currentMissionName = null;
            _baseTitle = "NetStorm 클론 — 개발 환경 확인";
            _statusLines.Add("미션을 종료했습니다.");
            UpdateWindowTitle();
        }
        else if (action == MissionMenuAction.Quit)
        {
            Exit();
        }
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
            PopupOpen: _mapViewer?.TutorialDialogOpen == true || _mapViewer?.KnowledgeOpen == true || _mapViewer?.MissionMenuOpen == true, TopEdgeBlocked: false);
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
            _mapViewer.Draw(batch, body, width, height, _display.Description, _fonts.GetFont(SmallFontSize));
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
            if (_audio != null)
            {
                // 소리 장치·지금 곡은 계속 바뀌므로 매 프레임 새로 쓴다
                batch.DrawString(body, _audio.Describe(), new Vector2(24, y), Color.LightGreen);
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
            _audio?.Dispose();
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
