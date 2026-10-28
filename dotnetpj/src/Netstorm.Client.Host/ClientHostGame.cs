using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Client.Automation;
using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

/// <summary>
/// MonoGame 출력 호스트. 논리 화면(<see cref="Screen"/>)을 정수 최근접 확대·검정 여백으로 내보내고,
/// 창·전체화면 전환, 마우스 역변환·커서 제한, 키·글자·버튼 → <see cref="InputQueue"/>, 프레임 루프를 맡는다.
/// 그리기 내용·세션은 아직 없으므로 화면 골격과 UI 스크립트 실행까지만 돈다.
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

    /// <summary>그리기 묶음.</summary>
    private SpriteBatch? _batch;

    /// <summary>논리 화면 텍스처 출력.</summary>
    private MonoGameScreenOutput? _output;

    /// <summary>창 모드로 돌아올 때 복구할 클라이언트 크기.</summary>
    private Point _windowedSize;

    /// <summary>이전 프레임의 키 상태(눌림·뗌 가림용).</summary>
    private KeyboardState _previousKeys;

    /// <summary>이전 프레임의 마우스 상태(버튼 가림용).</summary>
    private MouseState _previousMouse;

    /// <summary>
    /// 화면·큐·실행기 골격과 인자로 만든다.
    /// </summary>
    /// <param name="driver">프레임 골격.</param>
    /// <param name="options">실행 인자.</param>
    public ClientHostGame(HostFrameDriver driver, HostOptions options)
    {
        ArgumentNullException.ThrowIfNull(driver);
        ArgumentNullException.ThrowIfNull(options);
        _driver = driver;
        _options = options;
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsFixedTimeStep = true;
        TargetElapsedTime = TimeSpan.FromSeconds(1.0 / options.FramesPerSecond);
        IsMouseVisible = true;
        // 글자 입력은 WM_CHAR처럼 아래 코드에 그대로 넣는다
        Window.TextInput += (_, arguments) => _driver.Input.Push(HostInputMapper.CharCode(arguments.Character), 0, 0);
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
        _driver.Screen.Update(new ScreenRect(0, 0, _driver.Screen.Width, _driver.Screen.Height));
        _previousKeys = Keyboard.GetState();
        _previousMouse = Mouse.GetState();
    }

    /// <summary>
    /// 스크립트 입력 → 실제 입력 순으로 펌프한다. F11은 전체화면 전환에 쓴다.
    /// </summary>
    /// <param name="gameTime">경과 시각.</param>
    protected override void Update(GameTime gameTime)
    {
        _driver.BeginFrame();
        PumpKeyboard();
        PumpMouse();
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
        _driver.Input.Push(HostInputMapper.ButtonCode(button, released: current == ButtonState.Released),
            logical.X, logical.Y);
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
