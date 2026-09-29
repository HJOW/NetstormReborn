using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Core.Display;

namespace Netstorm.Game;

/// <summary>
/// 창·전체화면·화면비를 관리한다. 게임은 논리 해상도 렌더 타깃에 그린 뒤 <see cref="ScreenLayout"/> 의 뷰포트에 늘려 표시한다.
/// 전체화면은 <b>테두리 없는 전체 화면 창</b>(HardwareModeSwitch = false)으로 만든다. 디스플레이 모드를 바꾸지 않으므로
/// 원본처럼 전체화면 저장 뒤 재실행이 실패하는 문제가 생기지 않고, 그래도 시작에 실패한 경우를 대비해
/// <see cref="DisplaySettings.StartupInProgress"/> 표식으로 다음 실행을 창 모드로 시작한다.
/// </summary>
internal sealed class DisplayManager : IDisposable
{
    /// <summary>알림 문구를 화면에 보여 주는 시간(초)</summary>
    private const double NoticeSeconds = 2.5;

    private readonly Microsoft.Xna.Framework.Game _game;
    private readonly GraphicsDeviceManager _graphics;
    /// <summary>설정 저장 경로. null 이면 저장하지 않는다 (명령줄로 덮어쓴 실행·스크린샷 실행).</summary>
    private readonly string? _settingsPath;
    private RenderTarget2D? _target;
    /// <summary>직전에 배치를 계산한 백버퍼 폭</summary>
    private int _lastWidth;
    /// <summary>직전에 배치를 계산한 백버퍼 높이</summary>
    private int _lastHeight;
    /// <summary>설정(와이드 모드·해상도 높이)이 바뀌어 배치를 다시 계산해야 하는지</summary>
    private bool _layoutDirty = true;
    private string _notice = "";
    private double _noticeRemaining;

    /// <summary>현재 표시 설정</summary>
    public DisplaySettings Settings { get; }

    /// <summary>현재 화면 배치 (백버퍼 크기·설정에서 계산)</summary>
    public ScreenLayout Layout { get; private set; }

    /// <summary>화면 배치가 바뀔 때마다 발생한다 (창 제목 갱신용)</summary>
    public event Action? LayoutChanged;

    /// <summary>백버퍼(창 클라이언트) 폭, 픽셀</summary>
    public int ScreenWidth => _game.GraphicsDevice.PresentationParameters.BackBufferWidth;

    /// <summary>백버퍼(창 클라이언트) 높이, 픽셀</summary>
    public int ScreenHeight => _game.GraphicsDevice.PresentationParameters.BackBufferHeight;

    /// <summary>창 테두리가 없는 화면인지: 가장자리 스크롤은 이때만 동작한다 (원본 창 테두리 오프셋 = 0 조건).</summary>
    public bool BorderlessScreen => _graphics.IsFullScreen;

    /// <summary>화면 상태를 한 줄로 설명한 문구 (창 제목·안내 표시용)</summary>
    public string Description =>
        $"화면 {ScreenWidth}×{ScreenHeight} ({ScreenLayoutCalculator.DescribeAspect(ScreenWidth, ScreenHeight)}) → " +
        $"논리 {Layout.LogicalWidth}×{Layout.LogicalHeight} ×{Layout.Scale:0.###} · " +
        $"{(Settings.WideScreen == WideScreenMode.Extend ? "시야 확장" : "4:3 레터박스")} · " +
        $"{(BorderlessScreen ? "전체화면" : "창")} · 가장자리 스크롤 {(Settings.EdgeScroll ? "켜짐" : "꺼짐")}";

    /// <summary>
    /// 설정에 맞춰 그래픽 장치 관리자를 구성한다. 장치가 만들어지기 전에 호출해야 한다.
    /// 직전 시작이 끝나지 못했으면(설정에 표식이 남아 있으면) 전체화면을 끄고 창 모드로 시작한다.
    /// </summary>
    /// <param name="game">게임 본체</param>
    /// <param name="graphics">그래픽 장치 관리자</param>
    /// <param name="settings">표시 설정 (명령줄 덮어쓰기까지 반영된 것)</param>
    /// <param name="settingsPath">저장 경로. null 이면 저장하지 않는다</param>
    public DisplayManager(Microsoft.Xna.Framework.Game game, GraphicsDeviceManager graphics, DisplaySettings settings,
        string? settingsPath)
    {
        _game = game;
        _graphics = graphics;
        Settings = settings;
        _settingsPath = settingsPath;
        if (_settingsPath != null && Settings.StartupInProgress && Settings.Fullscreen)
        {
            // 직전 실행이 화면 초기화 도중 끝났다: 원본처럼 같은 설정으로 다시 실패하지 않도록 창 모드로 되돌린다.
            Settings.Fullscreen = false;
            _notice = "직전 시작이 끝나지 않아 창 모드로 시작했습니다.";
            _noticeRemaining = NoticeSeconds;
        }
        Settings.StartupInProgress = true;
        Save();
        Configure(Settings.Fullscreen);
    }

    /// <summary>
    /// 전체화면 여부에 맞춰 백버퍼 크기·전체화면 방식을 정한다 (ApplyChanges 는 호출하지 않는다).
    /// 전체화면은 디스플레이 모드를 바꾸지 않는 테두리 없는 전체 화면 창이며 크기는 현재 바탕화면과 같다.
    /// </summary>
    /// <param name="fullscreen">전체화면으로 만들지 여부</param>
    private void Configure(bool fullscreen)
    {
        _graphics.HardwareModeSwitch = false;
        if (fullscreen)
        {
            DisplayMode desktop = GraphicsAdapter.DefaultAdapter.CurrentDisplayMode;
            _graphics.PreferredBackBufferWidth = desktop.Width;
            _graphics.PreferredBackBufferHeight = desktop.Height;
        }
        else
        {
            _graphics.PreferredBackBufferWidth = Settings.WindowWidth;
            _graphics.PreferredBackBufferHeight = Settings.WindowHeight;
        }
        _graphics.IsFullScreen = fullscreen;
    }

    /// <summary>첫 프레임을 그린 뒤 호출한다: 시작 처리 중 표식을 지워 다음 실행이 같은 모드로 시작하게 한다.</summary>
    public void CompleteStartup()
    {
        if (Settings.StartupInProgress)
        {
            Settings.StartupInProgress = false;
            Save();
        }
    }

    /// <summary>종료할 때 마지막 창 크기·설정을 저장한다.</summary>
    public void SaveOnExit()
    {
        RememberWindowSize();
        Settings.StartupInProgress = false;
        Save();
    }

    /// <summary>전체화면이 아닐 때의 현재 백버퍼 크기를 창 모드 크기로 기억한다.</summary>
    private void RememberWindowSize()
    {
        if (!_graphics.IsFullScreen && ScreenWidth > 0 && ScreenHeight > 0)
        {
            Settings.WindowWidth = ScreenWidth;
            Settings.WindowHeight = ScreenHeight;
            Settings.Normalize();
        }
    }

    /// <summary>설정 경로가 있으면 저장한다.</summary>
    private void Save()
    {
        if (_settingsPath != null)
        {
            Settings.Save(_settingsPath);
        }
    }

    /// <summary>전체화면과 창 모드를 바꾼다. 실패하면 창 모드로 되돌리고 알림을 띄운다.</summary>
    /// <param name="fullscreen">바꿀 상태</param>
    public void SetFullscreen(bool fullscreen)
    {
        if (fullscreen == _graphics.IsFullScreen)
        {
            return;
        }
        // 창 모드로 돌아올 때 쓸 크기를 전체화면이 되기 전에 기억한다.
        RememberWindowSize();
        try
        {
            Configure(fullscreen);
            _graphics.ApplyChanges();
            Settings.Fullscreen = fullscreen;
            Show(fullscreen ? "전체화면" : "창 모드");
        }
        catch (Exception error) when (error is InvalidOperationException or NotSupportedException or ArgumentException)
        {
            // 전체화면을 만들 수 없으면 창 모드를 유지하고 저장된 설정도 창 모드로 둔다.
            Configure(false);
            _graphics.ApplyChanges();
            Settings.Fullscreen = false;
            Show($"전체화면 전환 실패: {error.Message}");
        }
        _layoutDirty = true;
        Save();
    }

    /// <summary>F11·Alt+Enter 는 전체화면, F10 은 와이드 처리, F9 는 원본 해상도 높이, F7 은 가장자리 스크롤을 바꾼다.</summary>
    /// <param name="keyboard">현재 키 상태</param>
    /// <param name="previous">직전 키 상태</param>
    public void HandleHotkeys(KeyboardState keyboard, KeyboardState previous)
    {
        bool alt = keyboard.IsKeyDown(Keys.LeftAlt) || keyboard.IsKeyDown(Keys.RightAlt);
        if (Pressed(keyboard, previous, Keys.F11) || (alt && Pressed(keyboard, previous, Keys.Enter)))
        {
            SetFullscreen(!_graphics.IsFullScreen);
        }
        if (Pressed(keyboard, previous, Keys.F10))
        {
            Settings.WideScreen = Settings.WideScreen == WideScreenMode.Extend ? WideScreenMode.Letterbox : WideScreenMode.Extend;
            _layoutDirty = true;
            Show(Settings.WideScreen == WideScreenMode.Extend ? "와이드: 시야 확장" : "와이드: 4:3 레터박스");
            Save();
        }
        if (Pressed(keyboard, previous, Keys.F9))
        {
            IReadOnlyList<int> heights = ScreenLayoutCalculator.ViewHeights;
            // 원본 해상도 높이 목록(480·600·768)을 순환한다.
            int next = (heights.ToList().IndexOf(Settings.ViewHeight) + 1) % heights.Count;
            Settings.ViewHeight = heights[next];
            _layoutDirty = true;
            Show($"원본 해상도 기준 높이: {Settings.ViewHeight}");
            Save();
        }
        if (Pressed(keyboard, previous, Keys.F7))
        {
            Settings.EdgeScroll = !Settings.EdgeScroll;
            Show(Settings.EdgeScroll ? "가장자리 스크롤 켜짐" : "가장자리 스크롤 꺼짐");
            Save();
        }
    }

    /// <summary>키를 이번 프레임에 새로 눌렀는지</summary>
    private static bool Pressed(KeyboardState keyboard, KeyboardState previous, Keys key) =>
        keyboard.IsKeyDown(key) && !previous.IsKeyDown(key);

    /// <summary>화면 한 구석에 잠깐 보여 줄 알림 문구를 정한다.</summary>
    /// <param name="text">알림 문구</param>
    public void Show(string text)
    {
        _notice = text;
        _noticeRemaining = NoticeSeconds;
    }

    /// <summary>알림이 남아 있으면 문구를 돌려주고 시간을 줄인다. 없으면 null.</summary>
    /// <param name="seconds">지난 시간(초)</param>
    public string? TickNotice(double seconds)
    {
        if (_noticeRemaining <= 0)
        {
            return null;
        }
        _noticeRemaining -= seconds;
        return _notice;
    }

    /// <summary>백버퍼 크기나 설정이 바뀌었으면 배치를 다시 계산하고 논리 해상도 렌더 타깃을 준비한다.</summary>
    private void Refresh()
    {
        int width = ScreenWidth;
        int height = ScreenHeight;
        if (!_layoutDirty && _target != null && width == _lastWidth && height == _lastHeight)
        {
            return;
        }
        _lastWidth = width;
        _lastHeight = height;
        _layoutDirty = false;
        ScreenLayout layout = ScreenLayoutCalculator.Compute(width, height, Settings.ViewHeight, Settings.WideScreen);
        // 논리 해상도가 달라졌을 때만 렌더 타깃을 다시 만든다.
        if (_target == null || _target.Width != layout.LogicalWidth || _target.Height != layout.LogicalHeight)
        {
            _target?.Dispose();
            _target = new RenderTarget2D(_game.GraphicsDevice, layout.LogicalWidth, layout.LogicalHeight, false,
                SurfaceFormat.Color, DepthFormat.None, 0, RenderTargetUsage.PreserveContents);
        }
        Layout = layout;
        LayoutChanged?.Invoke();
    }

    /// <summary>논리 해상도 렌더 타깃을 켜고 배경색으로 지운다. 이 뒤로 논리 좌표로 그린다.</summary>
    /// <param name="background">배경색</param>
    public void BeginScene(Color background)
    {
        Refresh();
        _game.GraphicsDevice.SetRenderTarget(_target);
        _game.GraphicsDevice.Clear(background);
    }

    /// <summary>논리 해상도 화면을 뷰포트에 늘려 백버퍼에 그린다. 정수 배율이 아니면 선형 보간을 쓴다.</summary>
    /// <param name="batch">스프라이트 배치</param>
    public void EndScene(SpriteBatch batch)
    {
        GraphicsDevice device = _game.GraphicsDevice;
        device.SetRenderTarget(null);
        // 뷰포트 바깥(레터박스·필러박스)은 검게 둔다.
        device.Clear(Color.Black);
        batch.Begin(samplerState: Layout.IsIntegerScale ? SamplerState.PointClamp : SamplerState.LinearClamp);
        batch.Draw(_target!, new Rectangle(Layout.ViewportX, Layout.ViewportY, Layout.ViewportWidth, Layout.ViewportHeight),
            Color.White);
        batch.End();
    }

    /// <summary>창 픽셀 좌표의 마우스 상태를 논리 화면 좌표로 바꾼다 (버튼·휠 값은 그대로).</summary>
    /// <param name="raw">MonoGame 이 준 창 좌표 마우스 상태</param>
    public MouseState ToLogical(MouseState raw)
    {
        var (x, y) = Layout.ToLogical(raw.X, raw.Y);
        return new MouseState((int)Math.Round(x), (int)Math.Round(y), raw.ScrollWheelValue,
            raw.LeftButton, raw.MiddleButton, raw.RightButton, raw.XButton1, raw.XButton2);
    }

    /// <summary>렌더 타깃을 해제한다.</summary>
    public void Dispose() => _target?.Dispose();
}
