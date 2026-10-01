using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Display;

namespace Netstorm.Game;

/// <summary>원본 그림·돌 질감을 재사용하는 메인 메뉴, 캠페인 목록, 옵션 창.</summary>
internal sealed class MainMenuView : IDisposable
{
    /// <summary>이번 범위에서 지원하는 원본·와이드 해상도 목록.</summary>
    private static readonly (int Width, int Height)[] Resolutions =
        [(640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1600, 900), (1920, 1080), (1920, 1200)];
    private readonly Texture2D _pixel;
    private readonly Texture2D? _background;
    private readonly Texture2D? _clouds;
    private readonly Texture2D? _stone;
    private readonly DisplayManager _display;
    private readonly AudioPlayer? _audio;
    private readonly bool _korean;
    private readonly Action _play;
    private readonly Action _quit;
    private readonly List<MenuButton> _buttons = [];
    private MouseState _previousMouse;
    private KeyboardState _previousKeyboard;
    private string _page = "main";
    private string _error = "";
    private int _selected;
    /// <summary>자동 UI 검사가 확인할 현재 페이지.</summary>
    public string Page => _page;

    /// <summary>원본 GIF와 셰이프 질감을 읽고 플레이·종료 요청을 본체에 연결한다.</summary>
    public MainMenuView(GraphicsDevice device, GameResources resources, ShapeDatabase shapes, Palette palette,
        DisplayManager display, AudioPlayer? audio, Action play, Action quit)
    {
        _display = display; _audio = audio; _play = play; _quit = quit;
        _korean = resources.Language == GameLanguage.Korean;
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
        _background = LoadImage(device, resources, "d/titleMenu.gif");
        _clouds = LoadImage(device, resources, "d/Gifcloud.gif");
        ShapeFrame? frame = shapes.FindBlock("fortGump")?.Frames[0];
        if (frame is { IsSpecial: false })
        {
            IndexedImage decoded = shapes.Decode(frame);
            _stone = new Texture2D(device, decoded.Width, decoded.Height);
            var colors = new Color[decoded.Indices.Length];
            // 인덱스 이미지를 현재 게임 팔레트의 RGBA 텍스처로 바꾼다.
            for (int i = 0; i < colors.Length; i++)
            {
                Rgb c = palette[decoded.Indices[i]];
                colors[i] = decoded.Opaque[i] ? new Color(c.R, c.G, c.B) : Color.Transparent;
            }
            _stone.SetData(colors);
        }
    }

    /// <summary>파일이 없거나 읽을 수 없으면 단색 화면으로 계속 실행한다.</summary>
    internal static Texture2D? LoadImage(GraphicsDevice device, GameResources resources, string path)
    {
        byte[]? bytes = resources.Files.TryReadAllBytes(path);
        if (bytes == null) return null;
        try { using var stream = new MemoryStream(bytes); return Texture2D.FromStream(device, stream); }
        catch (Exception error) when (error is InvalidOperationException or ArgumentException) { return null; }
    }

    /// <summary>선택 언어에 맞는 UI 문구를 고른다.</summary>
    private string Text(string korean, string english) => _korean ? korean : english;

    /// <summary>페이지를 열고 이전 클릭이 새 페이지에 전달되지 않게 현재 입력을 기억한다.</summary>
    public void Open(string page = "main")
    {
        _page = page is "campaigns" or "missions" or "options" ? page : "main";
        _selected = 0; _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
    }

    /// <summary>활성 버튼만 마우스·키보드로 실행한다. 잠긴 항목은 포커스도 받지 않는다.</summary>
    public void Update(MouseState mouse, KeyboardState keyboard, int width, int height)
    {
        BuildButtons(width, height);
        MenuButton[] enabled = _buttons.Where(b => b.Enabled).ToArray();
        _selected = Math.Clamp(_selected, 0, Math.Max(0, enabled.Length - 1));
        // 새로 누른 키만 페이지 이동으로 취급한다.
        bool Pressed(Keys key) => keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);
        if (enabled.Length > 0)
        {
            if (Pressed(Keys.Tab) || Pressed(Keys.Down)) _selected = (_selected + 1) % enabled.Length;
            if (Pressed(Keys.Up)) _selected = (_selected + enabled.Length - 1) % enabled.Length;
            if (Pressed(Keys.Enter)) enabled[_selected].Action();
            if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
                enabled.FirstOrDefault(b => b.Bounds.Contains(mouse.X, mouse.Y))?.Action();
        }
        if (Pressed(Keys.Escape) && _page != "main") Open(_page == "missions" ? "campaigns" : "main");
        _previousKeyboard = keyboard; _previousMouse = mouse;
    }

    /// <summary>입력과 그리기가 공유하는 버튼 영역·문구·잠금을 현재 논리 해상도에 맞춰 만든다.</summary>
    private void BuildButtons(int width, int height)
    {
        _buttons.Clear();
        int centerX = width / 2;
        int centerY = height / 2;
        if (_page == "main")
        {
            string[] labels = [Text("캠페인", "Campaign"), Text("멀티플레이", "Multiplayer"), Text("데모", "Demo"), Text("도움말", "Help"),
                Text("편집", "Edit"), Text("제작진", "Credits"), Text("옵션", "Options"), Text("종료", "Quit")];
            // 원본의 두 줄·네 열 순서를 유지하고 구현한 항목만 클릭을 허용한다.
            for (int i = 0; i < labels.Length; i++)
            {
                int item = i;
                Add(new Rectangle(centerX - 280 + i % 4 * 140, centerY - 30 + i / 4 * 40, 136, 34), labels[i], i is 0 or 6 or 7,
                    () => { if (item == 7) _quit(); else Open(item == 0 ? "campaigns" : "options"); });
            }
            return;
        }
        Rectangle panel = Panel(width, height);
        int x = panel.X + 28;
        int y = panel.Y + (_page == "missions" ? 134 : 68);
        if (_page == "campaigns")
        {
            string[] labels = [Text("초기 미션", "Early Missions"), Text("사제 훈련", "Priest Training"), Text("자유를 위한 투쟁", "Struggle For Freedom"),
                Text("국가의 탄생", "A Nation Rises"), Text("완전한 승리", "Complete Victory"), Text("사용자 캠페인", "User Made Campaigns")];
            // 원본의 여섯 캠페인 그룹 중 1장 그룹만 활성화한다.
            for (int i = 0; i < labels.Length; i++) Add(new Rectangle(x, y + i * 42, panel.Width - 56, 36), labels[i], i == 2, () => Open("missions"));
        }
        else if (_page == "missions")
        {
            string[] names = [Text("1-1  전쟁의 시작!", "1-1  The War Begins!"), "1-2  Master of Whirligigs", "1-3  Save the Island!", "1-4  Fragile Fortune", "1-5  Thundering Power!", "1-6  Dissolved Alliance"];
            // 완료 상태에 관계없이 1-2 이후는 잠금을 유지한다.
            for (int i = 0; i < names.Length; i++) Add(new Rectangle(x, y + i * 38, panel.Width - 56, 32), names[i], i == 0, _play);
        }
        else
        {
            DisplaySettings s = _display.Settings;
            // 옵션은 적용 직후에도 같은 행을 유지한다.
            void Row(string label, Action action, bool enabled = true)
            { Add(new Rectangle(x, y, panel.Width - 56, 32), label, enabled, action); y += 37; }
            Row(Text("화면", "Display") + ": " + Text(s.Fullscreen ? "전체화면" : "창 모드", s.Fullscreen ? "Fullscreen" : "Windowed"), () => _display.SetFullscreen(!s.Fullscreen));
            Row(Text("해상도", "Resolution") + $": {s.WindowWidth}×{s.WindowHeight}  >", NextResolution);
            Row(Text("와이드 화면", "Widescreen") + ": " + (s.WideScreen == WideScreenMode.Extend ? Text("시야 확장", "Extend") : "4:3"), () =>
            { s.WideScreen = s.WideScreen == WideScreenMode.Extend ? WideScreenMode.Letterbox : WideScreenMode.Extend; _display.SetResolution(s.WindowWidth, s.WindowHeight, s.ViewHeight); });
            Row(Text("효과음", "Sound Effects") + ": " + Text(s.SoundOn ? "켬" : "끔", s.SoundOn ? "On" : "Off"), () => { s.SoundOn = !s.SoundOn; ApplySound(); });
            Row(Text("효과음 볼륨", "Sound Volume") + $": {s.SoundVolume}/5  >", () => { s.SoundVolume = s.SoundVolume % 5 + 1; ApplySound(); _audio?.PlaySound("bell.wav"); });
            Row(Text("음악", "Music") + ": " + Text(s.PlayMusic ? "켬" : "끔", s.PlayMusic ? "On" : "Off"), () => { s.PlayMusic = !s.PlayMusic; ApplySound(); });
            Row(Text("음악 볼륨", "Music Volume") + $": {s.MusicVolume}/5  >", () => { s.MusicVolume = s.MusicVolume % 5 + 1; ApplySound(); });
            Row(Text("전체화면 가장자리 이동", "Edge Scroll in Fullscreen") + ": " + Text(s.EdgeScroll ? "켬" : "끔", s.EdgeScroll ? "On" : "Off"), () => { s.EdgeScroll = !s.EdgeScroll; _display.SaveOptions(); });
            Row(Text("자동 데모 · 바람 소리 · 서버 진단", "Auto Demo / Wind Noise / Diagnostics"), () => { }, false);
        }
        Add(new Rectangle(centerX - 70, panel.Bottom - 48, 140, 32), Text("뒤로", "Back"), true, () => Open(_page == "missions" ? "campaigns" : "main"));
    }

    /// <summary>다음 지원 해상도를 적용한다. 전체화면에서는 내부 렌더링 높이를 바꾼다.</summary>
    private void NextResolution()
    {
        DisplaySettings s = _display.Settings;
        int index = Array.FindIndex(Resolutions, r => r.Width == s.WindowWidth && r.Height == s.WindowHeight);
        var next = Resolutions[(index + 1) % Resolutions.Length];
        _display.SetResolution(next.Width, next.Height, next.Height);
    }

    /// <summary>음향 옵션을 저장하고 현재 곡·효과음에도 반영한다.</summary>
    private void ApplySound()
    {
        DisplaySettings s = _display.Settings;
        if (_audio != null)
        {
            _audio.SoundOn = s.SoundOn;
            if (_audio.MusicOn != s.PlayMusic) _audio.SetMusicOn(s.PlayMusic);
            _audio.SoundVolume = s.SoundVolume; _audio.MusicVolume = s.MusicVolume; _audio.ApplyVolumes();
        }
        _display.SaveOptions();
    }

    /// <summary>클릭 영역과 표시 내용이 어긋나지 않도록 같은 목록에 저장한다.</summary>
    private void Add(Rectangle bounds, string label, bool enabled, Action action) => _buttons.Add(new MenuButton(bounds, label, enabled, action));

    /// <summary>640×480에서도 선택 창이 화면 밖으로 나가지 않게 크기를 제한한다.</summary>
    private static Rectangle Panel(int width, int height)
    { int w = Math.Min(560, width - 32); int h = Math.Min(458, height - 20); return new Rectangle((width - w) / 2, (height - h) / 2, w, h); }

    /// <summary>플레이 시작 실패 원인을 메뉴에 보여 준다.</summary>
    public void ShowError(string error) => _error = error;

    /// <summary>원본 그림 위에 현재 페이지와 잠금 상태를 표시한다.</summary>
    public void Draw(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        BuildButtons(width, height);
        Tile(batch, _clouds, new Rectangle(0, 0, width, height), new Color(108, 104, 123));
        int imageW = Math.Min(640, width); int imageH = Math.Min(480, height);
        if (_background != null) batch.Draw(_background, new Rectangle((width - imageW) / 2, (height - imageH) / 2, imageW, imageH), Color.White);
        else batch.DrawString(font, "NETSTORM: ISLANDS AT WAR", new Vector2(width / 2 - 190, height / 2 - 150), Color.Gold);
        if (_page != "main")
        {
            Rectangle panel = Panel(width, height);
            Tile(batch, _stone, panel, new Color(67, 62, 55)); Border(batch, panel, Color.Goldenrod);
            string title = _page == "campaigns" ? Text("캠페인", "Campaigns") : _page == "missions" ? Text("자유를 위한 투쟁", "Struggle For Freedom") : Text("옵션", "Options");
            batch.DrawString(font, title, new Vector2(panel.X + 28, panel.Y + 22), Color.Gold);
            if (_page == "missions") batch.DrawString(small, Text("님버스의 주민들을 해방하십시오.\n적 사제를 포획하고 내 제단에서 희생하면 승리합니다.", "Free the people of Nimbus.\nCapture the enemy priest and sacrifice him at your altar."), new Vector2(panel.X + 28, panel.Y + 66), Color.White);
        }
        MenuButton[] enabled = _buttons.Where(b => b.Enabled).ToArray();
        // 비활성 버튼은 어둡게, 활성 버튼의 호버·키보드 포커스는 금색으로 표시한다.
        foreach (MenuButton button in _buttons)
        {
            bool hover = button.Bounds.Contains(_previousMouse.X, _previousMouse.Y);
            bool focus = enabled.Length > _selected && enabled[_selected] == button;
            Tile(batch, _stone, button.Bounds, new Color(62, 57, 48));
            batch.Draw(_pixel, button.Bounds, !button.Enabled ? Color.Black * 0.65f : hover ? Color.Goldenrod * 0.3f : Color.Black * 0.2f);
            Border(batch, button.Bounds, button.Enabled ? (hover || focus ? Color.Gold : Color.Tan) : Color.DimGray);
            string label = button.Label + (button.Enabled ? "" : Text(" [잠금]", " [Locked]"));
            SpriteFontBase f = font.MeasureString(label).X > button.Bounds.Width - 12 ? small : font;
            Vector2 size = f.MeasureString(label);
            batch.DrawString(f, label, new Vector2(button.Bounds.Center.X - size.X / 2, button.Bounds.Center.Y - size.Y / 2), button.Enabled ? Color.White : Color.Gray);
        }
        if (_error.Length > 0) batch.DrawString(small, _error, new Vector2(16, height - 24), Color.OrangeRed);
    }

    /// <summary>질감 마지막 타일을 창 경계에 맞춰 잘라 반복한다.</summary>
    private void Tile(SpriteBatch batch, Texture2D? texture, Rectangle area, Color fallback)
    {
        if (texture == null) { batch.Draw(_pixel, area, fallback); return; }
        // 수직 타일을 채운다.
        for (int y = area.Y; y < area.Bottom; y += texture.Height)
        {
            // 수평 타일을 채우고 오른쪽 끝을 잘라 낸다.
            for (int x = area.X; x < area.Right; x += texture.Width)
            {
                var rect = new Rectangle(x, y, Math.Min(texture.Width, area.Right - x), Math.Min(texture.Height, area.Bottom - y));
                batch.Draw(texture, rect, new Rectangle(0, 0, rect.Width, rect.Height), Color.White);
            }
        }
    }

    /// <summary>창과 버튼에 얇은 테두리를 그린다.</summary>
    private void Border(SpriteBatch batch, Rectangle rect, Color color)
    {
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Y, rect.Width, 1), color);
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Bottom - 1, rect.Width, 1), color);
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Y, 1, rect.Height), color);
        batch.Draw(_pixel, new Rectangle(rect.Right - 1, rect.Y, 1, rect.Height), color);
    }

    /// <summary>페이지 버튼의 영역·문구·잠금·동작.</summary>
    private sealed record MenuButton(Rectangle Bounds, string Label, bool Enabled, Action Action);

    /// <summary>메뉴 전용 텍스처를 해제한다.</summary>
    public void Dispose() { _pixel.Dispose(); _background?.Dispose(); _clouds?.Dispose(); _stone?.Dispose(); }
}
