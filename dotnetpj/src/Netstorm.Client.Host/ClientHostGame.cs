using System.Diagnostics;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Rules;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 출력 호스트. 논리 화면(<see cref="Screen"/>)을 정수 최근접 확대·검정 여백으로 내보내고,
/// 창·전체화면 전환, 마우스 역변환·커서 제한, 키·글자·버튼 → <see cref="InputQueue"/>, 프레임 루프를 맡는다.
/// 실제 타이틀 화면을 표시할 수 있으며 메뉴·세션 조립은 아직 남아 있다.
/// 근거: cpppj/src/app/main.cpp 의 --run 고리, Screen.cpp 의 창 복사, InputEvent.cpp 의 창 프로시저 넣기. (R1-2a, 2026-10-28)
/// </summary>
public sealed class ClientHostGame : Game
{
    /// <summary>전체화면 전환 키. 입력 큐에는 넣지 않고 호스트가 가로챈다.</summary>
    private const Keys FullscreenKey = Keys.F11;

    /// <summary>그래픽 장치 관리자.</summary>
    private readonly GraphicsDeviceManager _graphics;

    /// <summary>창 없이도 도는 프레임 골격.</summary>
    private readonly HostFrameDriver _driver;

    /// <summary>실행 인자.</summary>
    private readonly HostOptions _options;

    /// <summary>실제 데이터에서 준비한 시작 화면. 자료가 없으면 null이다.</summary>
    private readonly HostStartupScene? _startup;

    /// <summary>실제 버튼 트리를 소유하는 초기 메인 메뉴.</summary>
    private HostMainMenu? _mainMenu;

    /// <summary>메뉴와 같은 노드·입력 큐를 사용하는 HTML 창 소유자.</summary>
    private HostHtmlWindows? _htmlWindows;

    /// <summary>HTML 다중 줄 본문과 슬라이더 소유자.</summary>
    private HostHtmlText? _htmlText;

    /// <summary>HTML 창의 실제 controlled 메뉴 열.</summary>
    private HostHtmlMenus? _htmlMenus;

    /// <summary>현재 설정과 공통 트리를 사용하는 전체 HTML 창 생성기.</summary>
    private HostHtmlFactory? _htmlFactory;

    /// <summary>공통 트리의 표시 수집·정렬·출력 장치.</summary>
    private HostGumpDrawing? _gumpDrawing;

    /// <summary>현재 메인 메뉴의 공통 입력과 상위 명령 소비자.</summary>
    private HostMenuInput? _menuInput;

    /// <summary>HTML과 상위 메뉴가 공유할 원본 80칸 스크립트 문자열 고리.</summary>
    private readonly ScriptStringRing _scriptStrings = new();

    /// <summary>메뉴 버튼이 쓰는 단조 실시간 시계.</summary>
    private readonly Stopwatch _wallClock = Stopwatch.StartNew();

    /// <summary>메뉴 효과음의 MonoGame 장치 경계.</summary>
    private HostButtonSound? _buttonSound;

    /// <summary>배포 자산의 원본 커서와 SDL 핸들.</summary>
    private HostOriginalCursor? _cursor;

    /// <summary>버튼 트리가 요청한 다음 메뉴 재그리기.</summary>
    private bool _menuDirty;

    /// <summary>그리기 묶음.</summary>
    private SpriteBatch? _batch;

    /// <summary>논리 화면 텍스처 출력.</summary>
    private MonoGameScreenOutput? _output;

    /// <summary>창 모드로 돌아올 때 복구할 클라이언트 크기.</summary>
    private Point _windowedSize;

    /// <summary>마지막 물리 폴링 뒤 관측한 가상 키 눌림.</summary>
    private readonly HashSet<int> _pressedSince = [];

    /// <summary>이전 프레임의 키 상태(눌림·뗌 가림용).</summary>
    private KeyboardState _previousKeys;

    /// <summary>이전 프레임의 마우스 상태(버튼 가림용).</summary>
    private MouseState _previousMouse;

    /// <summary>
    /// 화면·큐·실행기 골격과 인자로 만든다.
    /// </summary>
    /// <param name="driver">프레임 골격.</param>
    /// <param name="options">실행 인자.</param>
    /// <param name="startup">실제 파일에서 읽은 시작 화면.</param>
    public ClientHostGame(HostFrameDriver driver, HostOptions options, HostStartupScene? startup = null)
    {
        ArgumentNullException.ThrowIfNull(driver);
        ArgumentNullException.ThrowIfNull(options);
        _driver = driver;
        _options = options;
        _startup = startup;
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsFixedTimeStep = true;
        TargetElapsedTime = TimeSpan.FromSeconds(1.0 / options.FramesPerSecond);
        IsMouseVisible = true;
        // 글자 입력은 WM_CHAR처럼 아래 코드에 그대로 넣는다
        Window.TextInput += (_, arguments) => _driver.Input.Push(HostInputMapper.CharCode(arguments.Character), 0, 0);
    }

    /// <summary>HTML 창과 메뉴 효과음 장치 및 그래픽 자원을 해제한다.</summary>
    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _htmlWindows?.CloseAll();
            _htmlFactory?.Dispose();
            _buttonSound?.Dispose();
            _cursor?.Dispose();
        }
        base.Dispose(disposing);
    }

    /// <summary>창 크기와 모드를 정한다. 시작 모드는 인자가 정한다.</summary>
    protected override void Initialize()
    {
        _windowedSize = new Point(_driver.Screen.Width, _driver.Screen.Height);
        if (_options.Fullscreen)
        {
            ApplyFullscreen(true);
        }
        else
        {
            ApplyWindowed();
        }
        base.Initialize();
    }

    /// <summary>텍스처 출력을 붙이고 첫 화면 전체를 올린다.</summary>
    protected override void LoadContent()
    {
        _batch = new SpriteBatch(GraphicsDevice);
        _output = new MonoGameScreenOutput(GraphicsDevice, _driver.Screen);
        _driver.Screen.Output = _output;
        if (_startup is null) _driver.Screen.Update(new ScreenRect(0, 0, _driver.Screen.Width, _driver.Screen.Height));
        else
        {
            _startup.Draw(_driver.Screen);
            _buttonSound = new HostButtonSound(_startup);
            _cursor = new HostOriginalCursor(_startup);
            _mainMenu = new HostMainMenu(_startup, _driver.Screen, new GumpButtonTreeHooks
            {
                Now = () => _wallClock.Elapsed.TotalSeconds,
                Poll = Poll,
                Sound = _buttonSound.Play,
                Redraw = (_, _) => _menuDirty = true,
                HandleBase = (_, _, _, _) => 0
            }, action => (_menuInput ?? throw new InvalidOperationException("메인 메뉴 입력 소유자 미연결")).Post(action));
            _htmlWindows = new HostHtmlWindows(_mainMenu, _driver.Input, () => _wallClock.Elapsed.TotalSeconds,
                _ => throw new InvalidOperationException("HTML 링크 명령 소비자 미연결"));
            _htmlWindows.BindParser(_startup, raw => ScriptCommand.QueueScriptCommand(raw, _driver.Input, _scriptStrings,
                new ScriptCommandHooks { Lookup = DialogScriptCommands.Lookup, Unknown = name => Debug.WriteLine("HTML 스크립트 명령 미연결: " + name) }));
            _htmlText = new HostHtmlText(_mainMenu, _htmlWindows, _driver.Screen, Poll, (_, _) => _menuDirty = true,
                cursor => _cursor.Set((GameCursor)cursor), _buttonSound.PlayByName);
            _htmlText.Bind(_htmlWindows);
            _htmlMenus = new HostHtmlMenus(_mainMenu, _htmlWindows, _htmlText, _startup, _driver.Screen, (_, _) => _menuDirty = true);
            _htmlMenus.BindInput(_driver.Input, _buttonSound.PlayByName, (_, _) => _menuDirty = true);
            _htmlFactory = new HostHtmlFactory(_mainMenu, _htmlWindows, _htmlMenus, _startup, _driver.Screen, _driver.Input,
                _scriptStrings, () => _wallClock.Elapsed.TotalSeconds, _ => 0,
                name => Debug.WriteLine("HTML 메타 명령 미연결: " + name));
            _menuInput = new HostMenuInput(_mainMenu, _htmlWindows, _htmlFactory, _driver.Input, _scriptStrings,
                () => _menuDirty = true, action => throw new InvalidOperationException("메뉴 상위 명령 미연결: " + action));
            _menuInput.BindPopup(new HostPopupMenu(_mainMenu, _htmlWindows, _htmlMenus, _driver.Screen, _startup, () => _menuDirty = true, _buttonSound.ApplyOptions));
            _menuInput.BindScreenMode(() => _driver.Screen.Flags, ChangeScreenMode, AcceptScreenMode);
            _gumpDrawing = new HostGumpDrawing(_mainMenu, _htmlText, _htmlMenus, _driver.Screen);
            _driver.BindControlLocator(_mainMenu);
            _driver.BindInputHandler(_menuInput.Dispatch);
            _gumpDrawing.Draw();
            _menuDirty = false;
        }
        _previousKeys = Keyboard.GetState();
        _previousMouse = Mouse.GetState();
    }

    /// <summary>
    /// 스크립트 입력 → 실제 입력 → 연결된 Guide/UI 순으로 처리한다. F11은 전체화면 전환에 쓴다.
    /// </summary>
    /// <param name="gameTime">경과 시각.</param>
    protected override void Update(GameTime gameTime)
    {
        _menuInput?.Tick();
        _driver.BeginFrame();
        PumpKeyboard();
        PumpMouse();
        if (_menuInput?.NotifyPointer(Poll(0xffff)) == 0) _cursor?.Set(GameCursor.Arrow);
        if (_driver.ProcessInput()) { Exit(); return; }
        if (_startup?.Configuration.Configuration.Changed == true) _startup.SaveOptions();
        _menuInput?.FinishFrame();
        if (_menuDirty && _gumpDrawing is not null && _startup is not null)
        { _startup.Draw(_driver.Screen); _gumpDrawing.Draw(); _menuDirty = false; }
        base.Update(gameTime);
    }

    /// <summary>검정 여백 위에 정수 배율 화면을 그리고 스크립트 관측 단계를 처리한 뒤 프레임 제한을 확인한다.</summary>
    /// <param name="gameTime">경과 시각.</param>
    protected override void Draw(GameTime gameTime)
    {
        GraphicsDevice.Clear(Color.Black);
        if (_batch is not null && _output is not null)
        {
            Rectangle client = Window.ClientBounds;
            ScreenRect output = HostPresentation.IntegerFitRect(
                _driver.Screen.Width, _driver.Screen.Height, Math.Max(1, client.Width), Math.Max(1, client.Height));
            var destination = new Rectangle(output.Left, output.Top, output.Right - output.Left, output.Bottom - output.Top);
            // 정수 배율은 PointClamp 로 화소를 뭉개지 않는다
            _batch.Begin(samplerState: SamplerState.PointClamp);
            _batch.Draw(_output.Texture, destination, Color.White);
            _batch.End();
        }
        _driver.EndFrame();
        if (_driver.ShouldExit(_options.Frames))
        {
            _driver.Finish();
            Exit();
        }
        base.Draw(gameTime);
    }

    /// <summary>그리기에 쓰는 정수 맞춤 사각형. 마우스 역변환과 같은 값을 쓴다.</summary>
    /// <param name="clientWidth">클라이언트 폭.</param>
    /// <param name="clientHeight">클라이언트 높이.</param>
    internal ScreenRect OutputRect(int clientWidth, int clientHeight) =>
        HostPresentation.IntegerFitRect(_driver.Screen.Width, _driver.Screen.Height,
            Math.Max(1, clientWidth), Math.Max(1, clientHeight));

    /// <summary>키보드 눌림·뗌을 입력 큐에 넣는다. F11은 전환에 쓰고 큐에는 넣지 않는다.</summary>
    private void PumpKeyboard()
    {
        KeyboardState current = Keyboard.GetState();
        bool shift = current.IsKeyDown(Keys.LeftShift) || current.IsKeyDown(Keys.RightShift);
        bool control = current.IsKeyDown(Keys.LeftControl) || current.IsKeyDown(Keys.RightControl);
        bool alt = current.IsKeyDown(Keys.LeftAlt) || current.IsKeyDown(Keys.RightAlt);
        foreach (Keys key in current.GetPressedKeys())
        {
            // 전환 키는 뗌까지 기다리지 않고 누르는 순간 바꾼다
            if (key == FullscreenKey && !_previousKeys.IsKeyDown(key))
            {
                ToggleFullscreen();
                continue;
            }
            if (!_previousKeys.IsKeyDown(key))
            {
                _pressedSince.Add((int)key);
                if (key is Keys.LeftAlt or Keys.RightAlt) _pressedSince.Add(0x12);
                if (key is Keys.LeftControl or Keys.RightControl) _pressedSince.Add(0x11);
                _driver.Input.Push(HostInputMapper.KeyCode((int)key, released: false, shift, control, alt), 0, 0);
            }
        }
        foreach (Keys key in _previousKeys.GetPressedKeys())
        {
            if (!current.IsKeyDown(key) && key != FullscreenKey)
            {
                _driver.Input.Push(HostInputMapper.KeyCode((int)key, released: true, shift, control, alt), 0, 0);
            }
        }
        _previousKeys = current;
    }

    /// <summary>마우스 버튼 눌림·뗌을 논리 좌표와 함께 입력 큐에 넣는다. 전체화면에서는 창 밖 커서를 안으로 모은다.</summary>
    private void PumpMouse()
    {
        MouseState current = Mouse.GetState();
        Rectangle client = Window.ClientBounds;
        // 전체화면 커서 제한: 창 밖이면 안쪽 가장자리로 옮긴다
        if ((_driver.Screen.Flags & ScreenMode.FullScreen) != 0)
        {
            ScreenPoint? confined = HostInputMapper.ConfinePoint(current.X, current.Y,
                Math.Max(1, client.Width), Math.Max(1, client.Height));
            if (confined is not null)
            {
                Mouse.SetPosition(confined.Value.X, confined.Value.Y);
                current = Mouse.GetState();
            }
        }
        ScreenRect output = OutputRect(client.Width, client.Height);
        ScreenPoint logical = ScreenPresentation.MapScreenPresentationPoint(
            new ScreenPoint(current.X, current.Y), output, _driver.Screen.Width, _driver.Screen.Height);
        PushButton(current.LeftButton, _previousMouse.LeftButton, HostMouseButton.Left, logical);
        PushButton(current.RightButton, _previousMouse.RightButton, HostMouseButton.Right, logical);
        PushButton(current.MiddleButton, _previousMouse.MiddleButton, HostMouseButton.Middle, logical);
        _previousMouse = current;
    }

    /// <summary>버튼 상태가 바뀌었을 때만 누름·뗌 사건을 넣는다.</summary>
    private void PushButton(ButtonState current, ButtonState previous, HostMouseButton button, ScreenPoint logical)
    {
        if (current == previous)
        {
            return;
        }
        if (current == ButtonState.Pressed) _pressedSince.Add(button switch { HostMouseButton.Left => 1, HostMouseButton.Right => 2, _ => 4 });
        _driver.Input.Push(HostInputMapper.ButtonCode(button, released: current == ButtonState.Released),
            logical.X, logical.Y);
    }

    /// <summary>스크립트 포인터를 우선하고 그 외에는 현재 장치와 창 상태를 논리 좌표로 폴링한다.</summary>
    /// <param name="code">원본 가상 키 또는 마우스 버튼 코드.</param>
    private InputEvent Poll(uint code)
    {
        InputEvent? scripted = HostInputMapper.ScriptPoll(code, _driver.ScriptPointer);
        if (scripted is not null) return scripted.Value;
        uint kept = code & 0x43ffffffu;
        int key = (int)(kept >> InputCode.VirtualKeyShift);
        MouseState mouse = Mouse.GetState();
        KeyboardState keyboard = Keyboard.GetState();
        bool down = key switch
        {
            1 => mouse.LeftButton == ButtonState.Pressed,
            2 => mouse.RightButton == ButtonState.Pressed,
            4 => mouse.MiddleButton == ButtonState.Pressed,
            0x11 => keyboard.IsKeyDown(Keys.LeftControl) || keyboard.IsKeyDown(Keys.RightControl),
            0x12 => keyboard.IsKeyDown(Keys.LeftAlt) || keyboard.IsKeyDown(Keys.RightAlt),
            >= 1 and <= 255 => keyboard.IsKeyDown((Keys)key),
            _ => false
        };
        Rectangle client = Window.ClientBounds;
        return HostInputMapper.PhysicalPoll(code, down, _pressedSince.Remove(key), IsActive,
            new ScreenPoint(mouse.X, mouse.Y), OutputRect(client.Width, client.Height), _driver.Screen.Width, _driver.Screen.Height);
    }

    /// <summary>창 모드와 전체화면 borderless 를 바꾼다. 논리 화면 크기는 그대로 둔다.</summary>
    private void ToggleFullscreen()
    {
        if ((_driver.Screen.Flags & ScreenMode.FullScreen) != 0)
        {
            ApplyWindowed();
        }
        else
        {
            ApplyFullscreen(false);
        }
    }

    /// <summary>원본 절대 모드를 같은 화면 장치에 적용하고 실제 현재 비트를 설정에 공개한다.</summary>
    private void ChangeScreenMode(uint requested, bool remember)
    {
        if (!ScreenGeometry.ScreenModeIsLegal(ScreenPresentation.ModernScreenMode(requested), true)) throw new InvalidOperationException("지원하지 않는 화면 모드 조합");
        bool currentFull = (_driver.Screen.Flags & ScreenMode.FullScreen) != 0, nextFull = (requested & ScreenMode.FullScreen) != 0;
        if (nextFull != currentFull) { if (nextFull) ApplyFullscreen(false); else ApplyWindowed(); }
        if (!_driver.Screen.SetMode(requested)) throw new InvalidOperationException("지원하지 않는 화면 모드 조합");
        uint actual = _driver.Screen.Flags;
        if (_startup != null)
        {
            var config = _startup.Configuration;
            config.SetInt("global.ddFullScreen", (actual & ScreenMode.FullScreen) != 0 ? 1 : 0);
            config.SetInt("global.ddBackBufferDib", (actual & ScreenMode.BackBufferDib) != 0 ? 1 : 0);
            config.SetInt("global.ddFlipping", (actual & ScreenMode.Flipping) != 0 ? 1 : 0);
            config.SetInt("global.ddClouds", (actual & ScreenMode.Clouds) != 0 ? 1 : 0);
            config.SetInt("global.ddAnimating", (actual & ScreenMode.Animating) != 0 ? 1 : 0);
            config.SetInt("global.ddParallax", (actual & ScreenMode.Parallax) != 0 ? 1 : 0);
            config.SetInt("global.ddSoftwareMouse", (actual & ScreenMode.SoftwareMouse) != 0 ? 1 : 0);
            config.SetInt("global.ddFourPage", 0);
            if (remember) config.SetInt("startInFullScreen", (actual & ScreenMode.FullScreen) != 0 ? 1 : 0);
        }
        _menuDirty = true;
    }

    /// <summary>미리 본 전체화면 모드만 확인 시 workingFullScreenFlags에 기록한다.</summary>
    private void AcceptScreenMode()
    {
        if (_startup != null && (_driver.Screen.Flags & ScreenMode.FullScreen) != 0)
            _startup.Configuration.SetInt("workingFullScreenFlags", unchecked((int)_driver.Screen.Flags));
    }

    /// <summary>창 모드로 돌린다. 모드 전환은 저장 팔레트를 다시 적용한다(원본과 같은 순서).</summary>
    private void ApplyWindowed()
    {
        _graphics.HardwareModeSwitch = false;
        Window.IsBorderless = false;
        _graphics.PreferredBackBufferWidth = _windowedSize.X;
        _graphics.PreferredBackBufferHeight = _windowedSize.Y;
        _graphics.ApplyChanges();
        _driver.Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
    }

    /// <summary>전체화면 borderless 로 바꾼다. 모니터 장치 모드는 바꾸지 않는다.</summary>
    /// <param name="first">시작부터 전체화면인지.</param>
    private void ApplyFullscreen(bool first)
    {
        if (!first)
        {
            _windowedSize = new Point(
                Math.Max(1, Window.ClientBounds.Width), Math.Max(1, Window.ClientBounds.Height));
        }
        _graphics.HardwareModeSwitch = false;
        Window.IsBorderless = true;
        _graphics.PreferredBackBufferWidth = GraphicsAdapter.DefaultAdapter.CurrentDisplayMode.Width;
        _graphics.PreferredBackBufferHeight = GraphicsAdapter.DefaultAdapter.CurrentDisplayMode.Height;
        Window.Position = Point.Zero;
        _graphics.ApplyChanges();
        _driver.Screen.SetMode(ScreenMode.FullScreen | ScreenMode.BackBufferDib | ScreenMode.ModeSet);
    }
}
