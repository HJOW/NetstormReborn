using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Display;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>원본 타이틀 위의 작은 버튼·캠페인 선택창·옵션 펼침 메뉴.</summary>
internal sealed class MainMenuView : IDisposable
{
    /// <summary>원본 세 해상도와 클론의 와이드 해상도 목록.</summary>
    private static readonly (int Width, int Height)[] Resolutions =
        [(640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1600, 900), (1920, 1080), (1920, 1200)];
    /// <summary>원본 메인 메뉴의 버튼 폭.</summary>
    private const int MainButtonWidth = 74;
    /// <summary>원본 메인 메뉴의 버튼 가로 간격.</summary>
    private const int MainButtonPitch = 79;
    /// <summary>캠페인 설명의 작은 본문 줄 높이.</summary>
    private const int DescriptionLineHeight = 16;
    private readonly Texture2D _pixel;
    private readonly Texture2D? _background;
    private readonly Texture2D? _clouds;
    private readonly OriginalUiSkin _skin;
    private readonly DisplayManager _display;
    private readonly AudioPlayer? _audio;
    private readonly bool _korean;
    private readonly Action<string> _play;
    private readonly Action _quit;
    private readonly List<MenuButton> _buttons = [];
    private readonly List<Rectangle> _lists = [];
    private readonly List<int> _separators = [];
    private MouseState _previousMouse;
    private KeyboardState _previousKeyboard;
    private string _page = "main";
    private string? _submenu;
    private string _error = "";
    private int _selected;
    private bool _keyboardFocus;
    /// <summary>자동 UI 검사가 확인할 현재 페이지.</summary>
    public string Page => _page;

    /// <summary>원본 GIF를 읽고 메뉴와 미션이 공유하는 UI 장식을 연결한다.</summary>
    public MainMenuView(GraphicsDevice device, GameResources resources, OriginalUiSkin skin,
        DisplayManager display, AudioPlayer? audio, Action<string> play, Action quit)
    {
        _display = display; _audio = audio; _play = play; _quit = quit; _skin = skin;
        _korean = resources.Language == GameLanguage.Korean;
        _pixel = new Texture2D(device, 1, 1); _pixel.SetData(new[] { Color.White });
        _background = LoadImage(device, resources, "d/titleMenu.gif");
        _clouds = LoadImage(device, resources, "d/Gifcloud.gif");
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

    /// <summary>페이지를 열고 같은 클릭이 새 창으로 전달되지 않게 입력을 기억한다.</summary>
    public void Open(string page = "main")
    {
        _page = page is "campaigns" or "missions" or "options" ? page : "main";
        _submenu = null; _selected = 0; _keyboardFocus = false;
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
    }

    /// <summary>활성 항목의 클릭·키보드 선택과 펼침 메뉴의 바깥 클릭을 처리한다.</summary>
    public void Update(MouseState mouse, KeyboardState keyboard, int width, int height)
    {
        if (width < 320 || height < 320) return;
        BuildButtons(width, height);
        MenuButton[] enabled = FocusButtons();
        _selected = Math.Clamp(_selected, 0, Math.Max(0, enabled.Length - 1));
        // 새로 누른 키만 페이지 이동으로 취급한다.
        bool Pressed(Keys key) => keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);
        if (mouse.Position != _previousMouse.Position) _keyboardFocus = false;
        if (enabled.Length > 0)
        {
            if (Pressed(Keys.Tab) || Pressed(Keys.Down)) { _selected = (_selected + 1) % enabled.Length; _keyboardFocus = true; }
            if (Pressed(Keys.Up)) { _selected = (_selected + enabled.Length - 1) % enabled.Length; _keyboardFocus = true; }
            if (Pressed(Keys.Enter)) enabled[_selected].Action();
            if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
            {
                MenuButton? hit = _buttons.LastOrDefault(b => b.Bounds.Contains(mouse.Position));
                if (hit != null)
                {
                    bool covered = _page == "options" && !hit.ListRow && _lists.Any(r => r.Contains(mouse.Position));
                    if (hit.Enabled && !covered) hit.Action();
                }
                else if (_page == "options" && !_lists.Any(r => r.Contains(mouse.Position))) Open();
            }
        }
        if (Pressed(Keys.Escape) && _page != "main")
        {
            if (_submenu != null) _submenu = null;
            else Open(_page == "missions" ? "campaigns" : "main");
        }
        _previousKeyboard = keyboard; _previousMouse = mouse;
    }

    /// <summary>모든 클릭 영역을 그리기와 같은 원본 기준 좌표로 만든다.</summary>
    private void BuildButtons(int width, int height)
    {
        _buttons.Clear(); _lists.Clear(); _separators.Clear();
        int cx = width / 2; int cy = height / 2;
        if (_page is "main" or "options")
        {
            string[] labels = [Text("캠페인", "Campaign"), Text("멀티플레이", "Multiplayer"), Text("데모", "Demo"), Text("도움말", "Help"),
                Text("편집", "Edit"), Text("제작진", "Credits"), Text("옵션", "Options"), Text("종료", "Quit")];
            // 원본 1024×768의 (356,311)에서 시작하는 두 줄·네 열 버튼을 화면 중심에 맞춘다.
            for (int i = 0; i < labels.Length; i++)
            {
                int item = i;
                Add(new Rectangle(cx - 156 + i % 4 * MainButtonPitch, cy - 73 + i / 4 * 23, MainButtonWidth, OriginalUiSkin.ButtonHeight),
                    labels[i], i is 0 or 6 or 7, () => { if (item == 7) _quit(); else Open(item == 0 ? "campaigns" : _page == "options" ? "main" : "options"); });
            }
            if (_page == "options") BuildOptions(width, height);
            return;
        }
        Rectangle panel = Panel(width, height);
        if (_page == "campaigns")
        {
            string[] labels = [Text("초기 미션", "Early Missions"), Text("사제 훈련", "Priest Training"), Text("자유를 위한 투쟁", "Struggle For Freedom"),
                Text("국가의 탄생", "A Nation Rises"), Text("완전한 승리", "Complete Victory"), Text("사용자 캠페인", "User Made Campaigns")];
            var list = new Rectangle(cx - 76, panel.Y + 124, 152, 134); _lists.Add(list);
            int[] offsets = [0, 18, 50, 68, 86, 118];
            // 튜토리얼·공식 캠페인·사용자 캠페인 세 그룹을 작은 목록으로 묶는다.
            for (int i = 0; i < labels.Length; i++) Add(new Rectangle(list.X + 2, list.Y + offsets[i] + 1, list.Width - 4, 17), labels[i], i == 2, () => Open("missions"), listRow: true);
            _separators.Add(list.Y + 42); _separators.Add(list.Y + 110);
        }
        else
        {
            string[] names = [Text("1 전쟁의 시작!", "1 The War Begins!"), Text("2 휘리기그의 지배자", "2 Master of Whirligigs"), "3 Save the Island!", "4 Fragile Fortune", "5 Thundering Power!", "6 Dissolved Alliance"];
            int listWidth = Math.Max(152, (int)names.Max(n => _skin.Body.MeasureString(n).X) + 18);
            var list = new Rectangle(cx - listWidth / 2, panel.Bottom - 146, listWidth, 108); _lists.Add(list);
            // 파란 선택 표시와 비활성 글자로 아직 구현하지 않은 미션을 구분한다. 공개 여부는 CampaignAccess 하나로 정한다.
            for (int i = 0; i < names.Length; i++)
            {
                string file = CampaignAccess.FirstChapter[i];
                Add(new Rectangle(list.X + 2, list.Y + 1 + i * 18, list.Width - 4, 18), names[i], CampaignAccess.IsAvailable(file), () => _play(file), listRow: true, check: false);
            }
        }
        Add(new Rectangle(cx - 22, panel.Bottom - 31, 44, OriginalUiSkin.ButtonHeight), Text("뒤로", "Back"), true,
            () => Open(_page == "missions" ? "campaigns" : "main"));
    }

    /// <summary>옵션을 버튼 옆 펼침 목록과 클릭으로 여는 하위 목록에 둔다.</summary>
    private void BuildOptions(int width, int height)
    {
        DisplaySettings s = _display.Settings;
        int menuWidth = 180;
        int x = Math.Clamp(width / 2 + 41, 2, width - menuWidth - 2);
        int y = Math.Clamp(height / 2 - 88, 2, height - 276);
        var menu = new Rectangle(x, y, menuWidth, 274); _lists.Add(menu);
        int rowY = y + 2;
        // 입력과 표시를 같은 행에 두고 선택형 항목에는 파란 표시를 붙인다.
        void Row(string label, Action action, bool enabled = true, bool? check = null)
        { Add(new Rectangle(x + 2, rowY, menuWidth - 4, OriginalUiSkin.RowHeight), label, enabled, action, listRow: true, check: check); rowY += OriginalUiSkin.RowHeight; }
        // 원본의 메뉴 그룹 사이에 구분선을 넣는다.
        void Gap() { _separators.Add(rowY + 3); rowY += 12; }
        Row(Text("전체화면", "Direct Draw / Full Screen"), () => _display.SetFullscreen(!s.Fullscreen), check: s.Fullscreen);
        int resolutionY = rowY;
        Row(Text("해상도", "Resolution") + " >", () => ToggleSubmenu("resolution"));
        Gap();
        Row(Text("효과음", "Sound On"), () => { s.SoundOn = !s.SoundOn; ApplySound(); }, check: s.SoundOn);
        Row(Text("음악 재생", "Play Music"), () => { s.PlayMusic = !s.PlayMusic; ApplySound(); }, check: s.PlayMusic);
        Row(Text("바람 소리", "Wind Noise"), () => { }, false, false);
        Row(Text("스피커 좌우 교환", "Speaker Swap L/R"), () => { }, false);
        int soundY = rowY;
        Row(Text("효과음 볼륨", "Sound Effect Volume") + " >", () => ToggleSubmenu("sound"));
        int musicY = rowY;
        Row(Text("음악 볼륨", "Music Volume") + " >", () => ToggleSubmenu("music"));
        Gap();
        Row(Text("전체화면 가장자리 이동", "Edge Scroll in Fullscreen"), () => { s.EdgeScroll = !s.EdgeScroll; _display.SaveOptions(); }, check: s.EdgeScroll);
        Row(Text("자동 데모", "Auto-Demo"), () => { }, false, false);
        Row(Text("시작할 때 팁 표시", "Tell Tips at Startup"), () => { }, false, false);
        Row(Text("일시 정지 - Shift-F9", "Pause - Shift-F9"), () => { }, false);
        Gap();
        Row(Text("서버 진단", "Pass Server Diagnostic"), () => { }, false);
        if (_submenu == null) return;
        int subWidth = _submenu == "resolution" ? 130 : 40;
        int count = _submenu == "resolution" ? Resolutions.Length : 5;
        int subX = menu.Right - 1;
        if (subX + subWidth > width - 2) subX = menu.X - subWidth + 1;
        int subY = _submenu == "resolution" ? resolutionY : _submenu == "sound" ? soundY : musicY;
        subY = Math.Clamp(subY, 2, height - count * OriginalUiSkin.RowHeight - 4);
        var sub = new Rectangle(subX, subY, subWidth, count * OriginalUiSkin.RowHeight + 4); _lists.Add(sub);
        // 클릭한 값을 적용하고 하위 목록만 닫아 상위 옵션 목록을 유지한다.
        for (int i = 0; i < count; i++)
        {
            int index = i;
            bool resolution = _submenu == "resolution";
            bool sound = _submenu == "sound";
            var r = Resolutions[i];
            string label = resolution ? $"{r.Width} × {r.Height}" : (i + 1).ToString();
            bool selected = resolution ? s.WindowWidth == r.Width && s.WindowHeight == r.Height : (sound ? s.SoundVolume : s.MusicVolume) == i + 1;
            Add(new Rectangle(sub.X + 2, sub.Y + 2 + i * OriginalUiSkin.RowHeight, sub.Width - 4, OriginalUiSkin.RowHeight), label, true, () =>
            {
                if (resolution) _display.SetResolution(Resolutions[index].Width, Resolutions[index].Height, Resolutions[index].Height);
                else { if (sound) s.SoundVolume = index + 1; else s.MusicVolume = index + 1; ApplySound(); if (sound) _audio?.PlaySound("bell.wav"); }
                _submenu = null;
            }, listRow: true, check: selected);
        }
    }

    /// <summary>같은 항목을 다시 누르면 하위 목록을 닫고 다른 항목이면 전환한다.</summary>
    private void ToggleSubmenu(string name) { _submenu = _submenu == name ? null : name; _selected = 0; }

    /// <summary>키보드 포커스는 현재 펼침 목록 안에서만 이동하고 뒤쪽 타이틀 버튼은 제외한다.</summary>
    private MenuButton[] FocusButtons() => _buttons.Where(b => b.Enabled && (_page != "options"
        || b.ListRow && (_submenu == null || _lists[^1].Contains(b.Bounds.Center)))).ToArray();

    /// <summary>음량을 저장하고 현재 음악·효과음에 즉시 반영한다.</summary>
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

    /// <summary>버튼과 펼침 목록의 클릭 영역·표시 상태를 함께 기록한다.</summary>
    private void Add(Rectangle bounds, string label, bool enabled, Action action, bool listRow = false, bool? check = null) =>
        _buttons.Add(new MenuButton(bounds, label, enabled, action, listRow, check));

    /// <summary>캠페인 설명을 원본과 같은 세 문단으로 제공한다.</summary>
    private IReadOnlyList<string> MissionLines(int width) => _skin.WrapText(Text(
        "님버스의 선량한 주민들은 바람, 비, 번개의 어둠의 군주들에게 노예가 되고 있습니다. 섬들이 하나씩 정복되고 지도자들은 살해되었으며, 주민들은 사악한 전쟁 기계에 굴복했습니다.\n\n님버스에서 정의를 지키는 유일한 수호자로서 어둠의 군주들을 물리치고 주민들을 노예 생활에서 해방해야 합니다.\n\n점점 강해지는 어둠의 세력에 맞설 동맹을 모으십시오.",
        "The good people of Nimbus are being enslaved by the Dark Lords of Wind, Rain and Thunder. One by one islands have been conquered, leaders slaughtered, and citizens bent to the will of the evil war machine.\n\nAs the sole guardian of justice in Nimbus, you must defeat the Dark Lords and free the people of Nimbus from slavery.\n\nYour goal is to gather allies to oppose the rising forces of darkness."), width);

    /// <summary>선택창을 원본 크기와 본문 높이에 맞춰 화면 중심에 둔다.</summary>
    private Rectangle Panel(int width, int height)
    {
        int w = _page == "campaigns" ? 238 : Math.Min(458, width - 32);
        int h = _page == "campaigns" ? 304 : 198 + MissionLines(w - 60).Count * DescriptionLineHeight;
        h = Math.Min(h, height - 24);
        return new Rectangle((width - w) / 2, (height - h) / 2, w, h);
    }

    /// <summary>플레이 시작 실패 원인을 메뉴에 보여 준다.</summary>
    public void ShowError(string error) => _error = error;

    /// <summary>원본 타이틀·돌 선택창·작은 목록·회색 비활성 글자를 표시한다.</summary>
    public void Draw(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        BuildButtons(width, height);
        if (_clouds != null)
        {
            // 구름 그림을 세로로 반복한다.
            for (int y = 0; y < height; y += _clouds.Height)
                // 구름 그림을 가로로 반복한다.
                for (int x = 0; x < width; x += _clouds.Width) batch.Draw(_clouds, new Vector2(x, y), Color.White);
        }
        if (_background != null) batch.Draw(_background, new Rectangle((width - 640) / 2, (height - 480) / 2, 640, 480), Color.White);
        if (_page is "campaigns" or "missions")
        {
            Rectangle panel = Panel(width, height); _skin.Panel(batch, panel);
            OriginalUiSkin.Text(batch, _skin.Title, _page == "campaigns" ? Text("캠페인", "Campaign") : Text("자유를 위한 투쟁", "Struggle For Freedom"), new Vector2(panel.X + 30, panel.Y + 20));
            if (_page == "campaigns") OriginalUiSkin.Text(batch, font, Text("어느 부분을\n플레이하시겠습니까?", "Which section would\nyou like to play?"), new Vector2(panel.X + 50, panel.Y + 57));
            else
            {
                int y = panel.Y + 54;
                // 원본 캠페인 설명의 문단 간격을 작은 본문으로 유지한다.
                foreach (string line in MissionLines(panel.Width - 60))
                { OriginalUiSkin.Text(batch, font, line, new Vector2(panel.X + 30, y)); y += DescriptionLineHeight; }
            }
        }
        // 옵션이 메인 버튼을 덮는 원본 순서를 지키기 위해 타이틀 버튼을 먼저 그린다.
        if (_page == "options")
        {
            // 펼침 목록 뒤에 놓이는 타이틀 버튼만 먼저 표시한다.
            foreach (MenuButton button in _buttons.Where(b => !b.ListRow))
                _skin.Button(batch, button.Bounds, button.Label, button.Enabled);
        }
        // 목록 바탕을 먼저 그린다.
        foreach (Rectangle list in _lists) _skin.Menu(batch, list);
        // 캠페인·옵션 목록의 그룹 구분선을 덧붙인다.
        foreach (int y in _separators) _skin.Separator(batch, _lists[0].X + 1, y, _lists[0].Width - 2);
        MenuButton[] enabled = FocusButtons();
        // 목록 행은 왼쪽 정렬하고 일반 버튼만 작은 입체 테두리를 그린다.
        foreach (MenuButton button in _buttons)
        {
            if (_page == "options" && !button.ListRow) continue;
            bool hover = button.Bounds.Contains(_previousMouse.Position);
            bool focus = _keyboardFocus && enabled.Length > _selected && enabled[_selected] == button;
            if (!button.ListRow) _skin.Button(batch, button.Bounds, button.Label, button.Enabled, hover, hover && _previousMouse.LeftButton == ButtonState.Pressed, focus);
            else
            {
                if (button.Enabled && (hover || focus)) batch.Draw(_pixel, button.Bounds, Color.Black * 0.25f);
                int inset = button.Check.HasValue ? 13 : 10;
                if (button.Check.HasValue) _skin.Pip(batch, new Point(button.Bounds.X + 5, button.Bounds.Center.Y), button.Check.Value, button.Enabled);
                OriginalUiSkin.Text(batch, font, button.Label, new Vector2(button.Bounds.X + inset, button.Bounds.Center.Y - font.MeasureString(button.Label).Y / 2), button.Enabled ? Color.White : new Color(185, 180, 166));
            }
        }
        if (_error.Length > 0) OriginalUiSkin.Text(batch, small, _error, new Vector2(16, height - 24), Color.OrangeRed);
    }

    /// <summary>버튼과 목록 행의 영역·표시·동작.</summary>
    private sealed record MenuButton(Rectangle Bounds, string Label, bool Enabled, Action Action, bool ListRow, bool? Check);

    /// <summary>메뉴 전용 텍스처를 해제한다. 공통 스킨은 게임 본체가 소유한다.</summary>
    public void Dispose() { _pixel.Dispose(); _background?.Dispose(); _clouds?.Dispose(); }
}
