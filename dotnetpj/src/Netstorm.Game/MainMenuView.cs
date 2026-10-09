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
    /// <summary>원본 메인 메뉴의 버튼 폭 (2026-10-03 측정: Credits 버튼 가로 435~509 = 75px, 이웃과의 간격 4px).</summary>
    private const int MainButtonWidth = 75;
    /// <summary>원본 메인 메뉴의 버튼 가로 간격.</summary>
    private const int MainButtonPitch = 79;
    /// <summary>
    /// 캠페인 설명·팁 본문의 줄 높이. 글꼴에 따라 다르다: 원본 글꼴(영어) 14, D2Coding(한국어) 16.
    /// 2026-10-10: 고정값 16 에서 글꼴을 따르도록 바꿨다.
    /// </summary>
    private int DescriptionLineHeight => _skin.LineHeight;
    /// <summary>시작 팁 창의 폭 (2026-10-01 녹화 g5: 약 424px)</summary>
    private const int TipPanelWidth = 424;
    /// <summary>시작 팁 창의 높이 (녹화 g5: 약 158px)</summary>
    private const int TipPanelHeight = 158;
    /// <summary>시작 팁 창 버튼 폭 (녹화 약 84px)</summary>
    private const int TipButtonWidth = 84;
    /// <summary>시작 팁 창 버튼 간격 (녹화 약 100px)</summary>
    private const int TipButtonPitch = 100;
    /// <summary>펼침 목록의 단축키 글자색 (녹화 "General Help - F1" 의 노란 F1)</summary>
    private static readonly Color MenuKeyColor = OriginalUiSkin.ValueColor;
    private readonly Texture2D _pixel;
    private readonly Texture2D? _background;
    private readonly Texture2D? _clouds;
    private readonly OriginalUiSkin _skin;
    private readonly DisplayManager _display;
    private readonly AudioPlayer? _audio;
    private readonly bool _korean;
    private readonly Action<string> _play;
    private readonly Action _quit;
    private readonly Action<string> _help;
    /// <summary>원본 tell.english [TipList] 의 시작 팁 (없으면 팁 창을 열지 않는다)</summary>
    private readonly StartupTips? _tips;
    /// <summary>지금 팁 창에 보여 주는 팁 번호</summary>
    private int _tipShown;
    /// <summary>메뉴 위에 떠 있는 작은 창 ("tips" = 시작 팁, "version" = 버전 정보, null = 없음)</summary>
    private string? _modal;
    private readonly List<MenuButton> _buttons = [];
    private readonly List<Rectangle> _lists = [];
    private readonly List<int> _separators = [];
    private MouseState _previousMouse;
    private KeyboardState _previousKeyboard;
    private string _page = "main";
    private string? _submenu;
    private string _error = "";
    /// <summary>돌 버튼(메인 메뉴 버튼·Back)의 누름·떼기 처리기. 번호는 <see cref="_buttons"/> 안의 순번이다.</summary>
    private readonly ButtonGump _gump = new();
    /// <summary>팁 창·버전 창 버튼의 누름·떼기 처리기. 번호는 팁 창 버튼 순번(버전 창은 0)이다.</summary>
    private readonly ButtonGump _modalGump = new();
    /// <summary>지금 눌린 모양인 버튼의 글자 (없으면 null). 자동 UI 검사가 읽는다.</summary>
    public string? PressedLabel { get; private set; }
    /// <summary>자동 UI 검사가 확인할 현재 페이지 (작은 창이 떠 있으면 그 이름).</summary>
    public string Page => _modal ?? _page;
    /// <summary>미션 지도 위에 옵션 목록만 표시하는 상태.</summary>
    public bool OptionsOnly { get; private set; }

    /// <summary>
    /// 원본 GIF를 읽고 메뉴와 미션이 공유하는 UI 장식을 연결한다.
    /// 2026-10-10: 배경 GIF 를 게임 팔레트로 칠하도록 <paramref name="palette"/> 를 받는다 (<see cref="LoadImage"/>).
    /// </summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="resources">게임 자료 (GIF·팁·언어)</param>
    /// <param name="palette">타이틀·구름 GIF 의 색 번호에 적용할 메뉴 화면 팔레트 (fortPal)</param>
    /// <param name="skin">공유 UI 그리기 도구</param>
    /// <param name="display">해상도·옵션 관리자</param>
    /// <param name="audio">효과음 재생기 (없으면 소리 없이 동작)</param>
    /// <param name="play">미션 이름을 받아 캠페인을 시작하는 동작</param>
    /// <param name="quit">게임을 끝내는 동작</param>
    /// <param name="help">도움말 주제를 여는 동작</param>
    public MainMenuView(GraphicsDevice device, GameResources resources, Palette palette, OriginalUiSkin skin,
        DisplayManager display, AudioPlayer? audio, Action<string> play, Action quit, Action<string> help)
    {
        _display = display; _audio = audio; _play = play; _quit = quit; _skin = skin;
        _help = help;
        string? tipList = resources.TryLoadMission("tell")?.Script.GetSection(StartupTips.SectionName);
        _tips = tipList == null ? null : new StartupTips(tipList);
        _korean = resources.Language == GameLanguage.Korean;
        _pixel = new Texture2D(device, 1, 1); _pixel.SetData(new[] { Color.White });
        _background = LoadImage(device, resources, "d/titleMenu.gif", palette);
        _clouds = LoadImage(device, resources, "d/Gifcloud.gif", palette);
    }

    /// <summary>
    /// 배경 GIF 를 색 번호로 해독해 화면 팔레트로 칠한 텍스처를 만든다. 원본처럼 GIF 안의 RGB 색 표는 쓰지 않는다.
    /// 파일이 없거나 읽을 수 없으면 null 을 돌려주며 호출한 쪽은 단색 화면으로 계속 실행한다.
    /// </summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="resources">게임 자료</param>
    /// <param name="path">자료 폴더 기준 GIF 경로 (예: "d/titleMenu.gif")</param>
    /// <param name="palette">색 번호에 적용할 화면 팔레트</param>
    /// <remarks>
    /// 원본 00419ec0 → 004dae60 은 GIF 색 번호를 화면에 그대로 복사한다 (docs/exe/cpp-menu-reconstruction.md).
    /// 2026-10-10 수정: 이전에는 Texture2D.FromStream 으로 GIF 내장 RGB 표를 써서 titleMenu.gif 의 295,080픽셀이
    /// 원본 화면 색과 달랐다 (채널당 최대 12, LEFT_JOBS.dotnetpj.md 5-5).
    /// </remarks>
    internal static Texture2D? LoadImage(GraphicsDevice device, GameResources resources, string path, Palette palette)
    {
        byte[]? bytes = resources.Files.TryReadAllBytes(path);
        if (bytes == null) return null;
        try { return SpriteAnimation.ToTexture(device, GifImage.Decode(bytes), palette); }
        catch (InvalidDataException) { return null; }
    }

    /// <summary>선택 언어에 맞는 UI 문구를 고른다.</summary>
    private string Text(string korean, string english) => _korean ? korean : english;

    /// <summary>
    /// 원본처럼 시작할 때 "Did You Know?" 팁 창을 연다 (Options "Tell Tips at Startup" 이 켜져 있을 때).
    /// 표시한 팁이 마지막 팁(원본 tip39 의 &lt;$Config,tipNumber=0&gt;)이면 다음 시작은 0번부터 한다.
    /// </summary>
    public void ShowStartupTip()
    {
        if (_tips == null || _tips.Count == 0 || !_display.Settings.TellTips) return;
        _tipShown = _display.Settings.TipNumber;
        _modal = "tips";
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
    }

    /// <summary>
    /// 팁 창 버튼 동작 (원본 [Tips]: Prior Tip = TellTip,1 · Next Tip = TellTip,2 · No More Tips = tellTips=0 · OK = TellTip,3).
    /// 다음 시작에는 지금 본 팁의 다음 번호를 보여 준다.
    /// </summary>
    /// <param name="button">0 이전 · 1 다음 · 2 그만 보기 · 3 확인</param>
    private void TipButton(int button)
    {
        DisplaySettings s = _display.Settings;
        bool resets = _tips?.ResetsNumber(_tipShown) == true;
        if (button == 0) { _tipShown = Math.Max(0, _tipShown - 1); return; }
        if (button == 1) { _tipShown = resets ? 0 : _tipShown + 1; return; }
        if (button == 2) s.TellTips = false;
        s.TipNumber = resets ? 0 : _tipShown + 1;
        _display.SaveOptions();
        _modal = null;
    }

    /// <summary>시작 팁 창의 영역 (화면 가운데)</summary>
    private static Rectangle TipPanel(int width, int height) =>
        new((width - TipPanelWidth) / 2, (height - TipPanelHeight) / 2, TipPanelWidth, TipPanelHeight);

    /// <summary>팁 창 버튼 영역 (녹화처럼 아래쪽에 네 개를 같은 간격으로 둔다)</summary>
    private static Rectangle TipButtonBounds(Rectangle panel, int index) =>
        new(panel.X + 20 + index * TipButtonPitch, panel.Bottom - 31, TipButtonWidth, OriginalUiSkin.ButtonHeight);

    /// <summary>버전 창의 영역</summary>
    private static Rectangle VersionPanel(int width, int height) => new((width - 380) / 2, (height - 170) / 2, 380, 170);

    /// <summary>작은 창(팁·버전)의 버튼 판정 영역. 팁 창의 "이전 팁"은 팁 번호가 0 이 아닐 때만 있다 (원본 &lt;?{tipNumber}&gt;).</summary>
    private List<GumpButton> ModalButtons(int width, int height)
    {
        var buttons = new List<GumpButton>();
        if (_modal == "version")
        {
            buttons.Add(OriginalUiSkin.Hit(0, VersionOkBounds(VersionPanel(width, height))));
            return buttons;
        }
        Rectangle tip = TipPanel(width, height);
        // 보이는 팁 창 버튼마다 판정 영역을 만든다
        for (int index = _tipShown > 0 ? 0 : 1; index < 4; index++) buttons.Add(OriginalUiSkin.Hit(index, TipButtonBounds(tip, index)));
        return buttons;
    }

    /// <summary>버전 창 OK 버튼의 그려지는 영역.</summary>
    private static Rectangle VersionOkBounds(Rectangle panel) => new(panel.Center.X - 30, panel.Bottom - 31, 60, OriginalUiSkin.ButtonHeight);

    /// <summary>
    /// 작은 창(팁·버전)의 버튼 입력을 처리한다. 창이 떠 있는 동안 뒤쪽 메뉴는 누를 수 없다.
    /// 원본 돌 버튼처럼 누르는 순간 소리가 나고 눌린 채 안쪽에서 뗄 때 실행한다 (<see cref="ButtonGump"/>). 키보드는 버튼에 영향을 주지 않는다.
    /// </summary>
    private void UpdateModal(MouseState mouse, int width, int height)
    {
        GumpResult result = _modalGump.Update(ModalButtons(width, height), mouse.X, mouse.Y, mouse.LeftButton == ButtonState.Pressed);
        if (result.Pressed != null) _audio?.PlaySound(OriginalUiSkin.ButtonSound);
        if (result.Activated is int id)
        {
            if (_modal == "version") _modal = null;
            else TipButton(id);
        }
        PressedLabel = _modalGump.Held is int held && _modalGump.IsPressed(held) ? ModalLabel(held) : null;
    }

    /// <summary>팁 창 버튼 번호의 글자 (버전 창은 OK).</summary>
    private string ModalLabel(int index) => _modal == "version" ? Text("확인", "OK")
        : new[] { Text("이전 팁", "Prior Tip"), Text("다음 팁", "Next Tip"), Text("그만 보기", "No More Tips"), Text("확인", "OK") }[index];

    /// <summary>시작 팁 창 또는 버전 창을 그린다.</summary>
    private void DrawModal(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (_modal == "version")
        {
            Rectangle panel = VersionPanel(width, height); _skin.Panel(batch, panel);
            OriginalUiSkin.Text(batch, _skin.Title, "NetStorm", new Vector2(panel.X + 24, panel.Y + 18));
            // 원본 tell.english [About] 문구와 클론 표기를 함께 보여 준다
            string[] lines = [Text("원본 버전 10.78 기준 클론", "Clone of version 10.78"), "(c) 1997 Titanic Entertainment, Inc. and Activision Inc.",
                "10.78 Patch by Ticonderoga Entertainment.", "NetStorm Reborn"];
            for (int i = 0; i < lines.Length; i++) OriginalUiSkin.Text(batch, font, lines[i], new Vector2(panel.X + 24, panel.Y + 50 + i * 18));
            _skin.Button(batch, VersionOkBounds(panel), Text("확인", "OK"), pressed: _modalGump.IsPressed(0));
            return;
        }
        Rectangle tip = TipPanel(width, height); _skin.Panel(batch, tip);
        OriginalUiSkin.Text(batch, _skin.Title, Text("알고 계셨나요?", "Did You Know?"), new Vector2(tip.X + 22, tip.Y + 20));
        int y = tip.Y + 52;
        // 팁 본문을 창 폭에 맞춰 감는다
        foreach (string line in _skin.WrapText(_tips!.Text(_tipShown, _korean), tip.Width - 50))
        { OriginalUiSkin.Text(batch, font, line, new Vector2(tip.X + 26, y)); y += DescriptionLineHeight; }
        string[] labels = [Text("이전 팁", "Prior Tip"), Text("다음 팁", "Next Tip"), Text("그만 보기", "No More Tips"), Text("확인", "OK")];
        // 이전 팁 단추는 팁 번호가 0 이 아닐 때만 그린다
        for (int index = _tipShown > 0 ? 0 : 1; index < 4; index++)
        {
            _skin.Button(batch, TipButtonBounds(tip, index), labels[index], pressed: _modalGump.IsPressed(index));
        }
    }

    /// <summary>페이지를 열고 같은 클릭이 새 창으로 전달되지 않게 입력을 기억한다. 눌러 붙잡고 있던 버튼은 놓는다.</summary>
    public void Open(string page = "main", bool optionsOnly = false)
    {
        OptionsOnly = optionsOnly;
        _page = page is "campaigns" or "missions" or "options" or "help" ? page : "main";
        _submenu = null;
        _gump.Cancel(); _modalGump.Cancel(); PressedLabel = null;
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
    }

    /// <summary>
    /// 마우스 입력을 처리한다. 돌 버튼은 누르는 순간 소리가 나고 뗄 때 실행하며(<see cref="ButtonGump"/>),
    /// 펼침 메뉴·대화상자 목록 행은 누르는 순간 실행한다 (원본 Menugump <c>FUN_00476820</c>). 호버 표시와 키보드 조작은 없다.
    /// 펼침 메뉴가 열려 있으면 메뉴가 누름을 독점하고, 메뉴 바깥을 누르면 그 자리에서 닫는다 (소리 없음, 뒤쪽 버튼은 눌리지 않는다).
    /// </summary>
    public void Update(MouseState mouse, KeyboardState keyboard, int width, int height)
    {
        if (width < 320 || height < 320) return;
        if (_modal != null)
        {
            UpdateModal(mouse, width, height);
            _previousKeyboard = keyboard; _previousMouse = mouse;
            return;
        }
        BuildButtons(width, height);
        bool leftDown = mouse.LeftButton == ButtonState.Pressed;
        bool pressEdge = leftDown && _previousMouse.LeftButton != ButtonState.Pressed;
        bool consumed = false;
        if (pressEdge)
        {
            MenuButton? row = _buttons.LastOrDefault(b => b.ListRow && b.Bounds.Contains(mouse.Position));
            if (row != null)
            {
                // 목록 행: 활성이면 소리를 내고 누르는 순간 실행한다 (비활성 행은 아무 반응이 없다)
                consumed = true;
                if (row.Enabled) ActivateRow(row);
            }
            else if (_page is "options" or "help")
            {
                // 펼침 메뉴 안의 빈 곳(구분선 등)은 무시하고, 메뉴 바깥은 그 자리에서 닫는다
                consumed = true;
                if (!_lists.Any(r => r.Contains(mouse.Position))) Open(OptionsOnly ? "options" : "main", OptionsOnly);
            }
        }
        var stone = new List<GumpButton>();
        // 돌 버튼(목록 행이 아닌 것)만 누름·떼기 처리기에 넘긴다. 펼침 메뉴 아래에 깔린 메인 버튼은 누를 수 없다
        for (int i = 0; i < _buttons.Count; i++)
        {
            if (_buttons[i].ListRow) continue;
            bool covered = _page is "options" or "help" && _lists.Any(r => r.Contains(_buttons[i].Bounds.Center));
            stone.Add(OriginalUiSkin.Hit(i, _buttons[i].Bounds, _buttons[i].Enabled && !covered));
        }
        GumpResult result = _gump.Update(stone, mouse.X, mouse.Y, leftDown, canPress: !consumed);
        if (result.Pressed != null) _audio?.PlaySound(OriginalUiSkin.ButtonSound);
        if (result.Activated is int id && id < _buttons.Count) _buttons[id].Action();
        PressedLabel = _gump.Held is int held && _gump.IsPressed(held) && held < _buttons.Count ? _buttons[held].Label : null;
        if (Pressed(Keys.Escape) && _page != "main")
        {
            if (_submenu != null) _submenu = null;
            else Open(_page == "missions" ? "campaigns" : "main");
        }
        _previousKeyboard = keyboard; _previousMouse = mouse;

        // 새로 누른 키만 페이지 이동으로 취급한다.
        bool Pressed(Keys key) => keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);
    }

    /// <summary>
    /// 활성 목록 행을 실행한다. 원본처럼 누르는 순간 <c>openSubGump.wav</c> 를 내고, 하위 메뉴(">")를 펼치는 행은
    /// <c>openGump.wav</c> 도 낸다 (<c>FUN_00476820</c> 정적 분석).
    /// </summary>
    private void ActivateRow(MenuButton row)
    {
        _audio?.PlaySound(OriginalUiSkin.MenuItemSound);
        if (row.Arrow) _audio?.PlaySound(OriginalUiSkin.MenuOpenSound);
        row.Action();
    }

    /// <summary>모든 클릭 영역을 그리기와 같은 원본 기준 좌표로 만든다.</summary>
    private void BuildButtons(int width, int height)
    {
        _buttons.Clear(); _lists.Clear(); _separators.Clear();
        int cx = width / 2; int cy = height / 2;
        if (_page is "main" or "options" or "help")
        {
            if (OptionsOnly) { BuildOptions(width, height); return; }
            string[] labels = [Text("캠페인", "Campaign"), Text("멀티플레이", "Multiplayer"), Text("데모", "Demo"), Text("도움말", "Help"),
                Text("편집", "Edit"), Text("제작진", "Credits"), Text("옵션", "Options"), Text("종료", "Quit")];
            // 원본 1024×768의 (356,311)에서 시작하는 두 줄·네 열 버튼을 화면 중심에 맞춘다.
            for (int i = 0; i < labels.Length; i++)
            {
                int item = i;
                Add(new Rectangle(cx - 156 + i % 4 * MainButtonPitch, cy - 73 + i / 4 * 23, MainButtonWidth, OriginalUiSkin.ButtonHeight),
                    labels[i], i is 0 or 3 or 6 or 7, () =>
                    {
                        if (item == 7) _quit();
                        else if (item == 3) Open(_page == "help" ? "main" : "help");
                        else Open(item == 0 ? "campaigns" : _page == "options" ? "main" : "options");
                    });
            }
            if (_page == "options") BuildOptions(width, height);
            if (_page == "help") BuildHelpMenu(width, height);
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
            // 구분선은 구간 위쪽을 넘긴다 (선은 +8 에 그려진다). 이전과 같은 줄(list.Y + 42·+110)에 오도록 맞춘다.
            _separators.Add(list.Y + 42 - OriginalUiSkin.MenuSeparatorLine); _separators.Add(list.Y + 110 - OriginalUiSkin.MenuSeparatorLine);
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

    /// <summary>
    /// 원본 Help 버튼의 펼침 목록 (2026-10-01 녹화 g1015): General Help - F1 · Technical Help · Version.
    /// Technical Help 는 원본에서 별도 Windows 도움말 파일을 여는 항목이라 클론에서는 비활성이다.
    /// </summary>
    /// <remarks>
    /// 2026-10-10: 위치·폭을 원본 캡처(screenShots/mainMenu - Help.png)에 맞췄다. 목록 왼쪽 = Help 버튼의 가운데 x,
    /// 위쪽 = 버튼 위쪽, 폭 = 가장 긴 행 + 31, 행 높이 17.
    /// </remarks>
    private void BuildHelpMenu(int width, int height)
    {
        int cx = width / 2; int cy = height / 2;
        (string Label, string Key, bool Enabled, Action Action)[] rows =
        [
            (Text("일반 도움말", "General Help"), "F1", true, () => { Open(); _help("F1Help"); }),
            (Text("기술 도움말", "Technical Help"), "", false, () => { }),
            (Text("버전", "Version"), "", true, () => { Open(); _modal = "version"; }),
        ];
        int menuWidth = OriginalUiSkin.MenuWidth(rows.Select(r => _skin.MenuLabelWidth(r.Key.Length > 0 ? r.Label + " - " + r.Key : r.Label)));
        int x = Math.Clamp(cx - 156 + 3 * MainButtonPitch + MainButtonWidth / 2, 2, width - menuWidth - 2);
        var menu = new Rectangle(x, cy - 73, menuWidth, OriginalUiSkin.RowHeight * rows.Length); _lists.Add(menu);
        // 행을 목록 맨 위부터 같은 높이로 쌓는다.
        for (int i = 0; i < rows.Length; i++)
        {
            Add(new Rectangle(x, menu.Y + i * OriginalUiSkin.RowHeight, menuWidth, OriginalUiSkin.RowHeight), rows[i].Label, rows[i].Enabled,
                rows[i].Action, listRow: true, key: rows[i].Key);
        }
    }

    /// <summary>옵션 목록의 한 행 (배치 전에 폭을 재려고 먼저 모은다).</summary>
    /// <param name="Label">문구</param>
    /// <param name="Action">고르면 할 일</param>
    /// <param name="Check">켜짐 표시 (null 이면 표시 없는 명령)</param>
    /// <param name="Arrow">하위 목록을 여는 행인지</param>
    /// <param name="Key">단축키 표기 (없으면 빈 문자열)</param>
    /// <param name="GapAfter">이 행 뒤에 구분 구간이 있는지</param>
    /// <param name="Submenu">여는 하위 목록 이름 (없으면 null)</param>
    private sealed record OptionRow(string Label, Action Action, bool? Check = null, bool Arrow = false, string Key = "",
        bool GapAfter = false, string? Submenu = null);

    /// <summary>
    /// 옵션을 버튼 옆 펼침 목록과 클릭으로 여는 하위 목록에 둔다. 행은 목록 맨 위부터 17픽셀씩 쌓이고 그룹 사이에 16픽셀 구분 구간이 있다.
    /// 목록 폭은 가장 긴 행 + 31, 하위 목록은 부모 목록의 오른쪽에 붙고 첫 행이 부모 행과 같은 높이에 온다.
    /// </summary>
    /// <param name="width">논리 화면 폭</param>
    /// <param name="height">논리 화면 높이</param>
    /// <remarks>
    /// 2026-10-10: 원본 캡처(screenShots/mainMenu - Options*.png)의 치수로 바꿨다 — 영어 1024×768 에서 목록 (551, 294) 166×269,
    /// 행 글자 x 566, 하위 목록 x 717. "Pause - Shift-F9" 행을 더했다. 세로 위치(화면 중심 − 90)는 캡처 한 장의 값이며
    /// 원본이 어떤 규칙으로 정하는지는 확인하지 못했다. 미션 중 목록의 위치는 메뉴 막대의 Options 항목 아래다.
    /// </remarks>
    private void BuildOptions(int width, int height)
    {
        DisplaySettings s = _display.Settings;
        OptionRow[] rows =
        [
            new(Text("전체화면", "Direct Draw / Full Screen"), () => { _display.SetFullscreen(!s.Fullscreen); Open(); }),
            new(Text("해상도", "Resolution"), () => ToggleSubmenu("resolution"), Arrow: true, GapAfter: true, Submenu: "resolution"),
            new(Text("효과음", "Sound On"), () => { s.SoundOn = !s.SoundOn; ApplySound(); Open(); }, s.SoundOn),
            new(Text("음악 재생", "Play Music"), () => { s.PlayMusic = !s.PlayMusic; ApplySound(); Open(); }, s.PlayMusic),
            new(Text("바람 소리", "Wind Noise"), () => { s.WindNoise = !s.WindNoise; ApplySound(); Open(); }, s.WindNoise),
            new(Text("스피커 좌우 교환", "Speaker Swap L/R"), () => { s.SpeakerSwap = !s.SpeakerSwap; ApplySound(); Open(); }, s.SpeakerSwap),
            new(Text("효과음 볼륨", "Sound Effect Volume"), () => ToggleSubmenu("sound"), Arrow: true, Submenu: "sound"),
            new(Text("음악 볼륨", "Music Volume"), () => ToggleSubmenu("music"), Arrow: true, GapAfter: true, Submenu: "music"),
            new(Text("전체화면 가장자리 이동", "Edge Scroll in Fullscreen"), () => { s.EdgeScroll = !s.EdgeScroll; _display.SaveOptions(); Open(); }, s.EdgeScroll),
            // 원본 녹화(00:05~00:38)처럼 세 항목도 켜고 끌 수 있다. 자동 데모 재생·서버 진단은 클론에 없어 값만 저장한다.
            new(Text("자동 데모", "Auto-Demo"), () => { s.AutoDemo = !s.AutoDemo; _display.SaveOptions(); Open(); }, s.AutoDemo),
            new(Text("시작할 때 팁 표시", "Tell Tips at Startup"), () => { s.TellTips = !s.TellTips; _display.SaveOptions(); Open(); }, s.TellTips),
            // 원본 옵션 목록의 Pause 행. 미션 중이면 일시정지를 바꾸고 메인 메뉴에서는 아무 일도 하지 않는다.
            new(Text("일시정지", "Pause"), () => { PauseRequested?.Invoke(); Open(); }, Key: "Shift-F9", GapAfter: true),
            new(Text("서버 진단 통과", "Pass Server Diagnostic"), () => { s.PassServerDiagnostic = !s.PassServerDiagnostic; _display.SaveOptions(); Open(); }, s.PassServerDiagnostic),
        ];
        int menuWidth = OriginalUiSkin.MenuWidth(rows.Select(r => _skin.MenuLabelWidth(r.Key.Length > 0 ? r.Label + " - " + r.Key : r.Label, r.Arrow)));
        int menuHeight = rows.Length * OriginalUiSkin.RowHeight + rows.Count(r => r.GapAfter) * OriginalUiSkin.MenuSeparatorHeight;
        int buttonCenter = width / 2 - 156 + 2 * MainButtonPitch + MainButtonWidth / 2;
        int x = Math.Clamp(OptionsOnly ? MissionOptionsLeft : buttonCenter, 2, Math.Max(2, width - menuWidth - 2));
        int y = OptionsOnly ? MissionOptionsTop : Math.Clamp(height / 2 - 90, 2, Math.Max(2, height - menuHeight - 2));
        var menu = new Rectangle(x, y, menuWidth, menuHeight); _lists.Add(menu);
        int rowY = y;
        int submenuY = y;
        // 입력과 표시를 같은 행에 두고 그룹 뒤에는 구분 구간을 둔다.
        foreach (OptionRow row in rows)
        {
            if (row.Submenu != null && row.Submenu == _submenu) submenuY = rowY;
            Add(new Rectangle(x, rowY, menuWidth, OriginalUiSkin.RowHeight), row.Label, true, row.Action, listRow: true, check: row.Check,
                key: row.Key, arrow: row.Arrow);
            rowY += OriginalUiSkin.RowHeight;
            if (row.GapAfter) { _separators.Add(rowY); rowY += OriginalUiSkin.MenuSeparatorHeight; }
        }
        if (_submenu == null) return;
        bool resolution = _submenu == "resolution";
        bool sound = _submenu == "sound";
        int count = resolution ? Resolutions.Length : 5;
        // 원본 해상도 표기는 "1024 by 768" 이다. 한국어 화면은 곱셈 기호를 쓴다.
        string[] labels = [.. Enumerable.Range(0, count).Select(i => resolution
            ? Text($"{Resolutions[i].Width} × {Resolutions[i].Height}", $"{Resolutions[i].Width} by {Resolutions[i].Height}")
            : Text($"음량 {i + 1}", $"Volume {i + 1}"))];
        int subWidth = OriginalUiSkin.MenuWidth(labels.Select(label => _skin.MenuLabelWidth(label)));
        int subX = menu.Right;
        if (subX + subWidth > width - 2) subX = menu.X - subWidth;
        int subY = Math.Clamp(submenuY, 2, Math.Max(2, height - count * OriginalUiSkin.RowHeight - 2));
        var sub = new Rectangle(subX, subY, subWidth, count * OriginalUiSkin.RowHeight); _lists.Add(sub);
        // 클릭한 값을 적용한 뒤 원본처럼 상위 옵션 메뉴까지 닫는다.
        for (int i = 0; i < count; i++)
        {
            int index = i;
            var r = Resolutions[i];
            bool selected = resolution ? s.WindowWidth == r.Width && s.WindowHeight == r.Height : (sound ? s.SoundVolume : s.MusicVolume) == i + 1;
            Add(new Rectangle(sub.X, sub.Y + i * OriginalUiSkin.RowHeight, sub.Width, OriginalUiSkin.RowHeight), labels[i], true, () =>
            {
                if (resolution) _display.SetResolution(Resolutions[index].Width, Resolutions[index].Height, Resolutions[index].Height);
                else { if (sound) s.SoundVolume = index + 1; else s.MusicVolume = index + 1; ApplySound(); if (sound) _audio?.PlaySound("bell.wav"); }
                Open();
            }, listRow: true, check: selected);
        }
    }

    /// <summary>미션 중 옵션 목록의 왼쪽 끝: 메뉴 막대의 Options 항목 아래 (막대 왼쪽 84 + 1 + 항목 간격 53 × 2).</summary>
    private const int MissionOptionsLeft = 191;

    /// <summary>미션 중 옵션 목록의 위쪽: 메뉴 막대(높이 17) 바로 아래.</summary>
    private const int MissionOptionsTop = 17;

    /// <summary>옵션 목록의 Pause 행을 골랐을 때 부르는 동작 (미션 중 일시정지 전환). 연결하지 않으면 아무 일도 하지 않는다.</summary>
    public Action? PauseRequested { get; set; }

    /// <summary>같은 항목을 다시 누르면 하위 목록을 닫고 다른 항목이면 전환한다.</summary>
    private void ToggleSubmenu(string name) { _submenu = _submenu == name ? null : name; }

    /// <summary>음량을 저장하고 현재 음악·효과음에 즉시 반영한다.</summary>
    private void ApplySound()
    {
        DisplaySettings s = _display.Settings;
        if (_audio != null)
        {
            _audio.SoundOn = s.SoundOn;
            _audio.WindNoise = s.WindNoise;
            _audio.SetSpeakerSwap(s.SpeakerSwap);
            if (_audio.MusicOn != s.PlayMusic) _audio.SetMusicOn(s.PlayMusic);
            _audio.SoundVolume = s.SoundVolume; _audio.MusicVolume = s.MusicVolume; _audio.ApplyVolumes();
        }
        _display.SaveOptions();
    }

    /// <summary>버튼과 펼침 목록의 클릭 영역·표시 상태를 함께 기록한다.</summary>
    /// <param name="bounds">클릭·표시 영역</param>
    /// <param name="label">문구</param>
    /// <param name="enabled">고를 수 있는지</param>
    /// <param name="action">고르면 할 일</param>
    /// <param name="listRow">목록 행인지 (아니면 돌 버튼)</param>
    /// <param name="check">켜짐 표시 (null 이면 없음)</param>
    /// <param name="key">문구 뒤에 노란색으로 쓸 단축키 표기</param>
    /// <param name="arrow">하위 목록 화살표가 붙는지</param>
    private void Add(Rectangle bounds, string label, bool enabled, Action action, bool listRow = false, bool? check = null,
        string key = "", bool arrow = false) =>
        _buttons.Add(new MenuButton(bounds, label, enabled, action, listRow, check, key, arrow));

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
        if (!OptionsOnly && _clouds != null)
        {
            // 구름 그림을 세로로 반복한다.
            for (int y = 0; y < height; y += _clouds.Height)
                // 구름 그림을 가로로 반복한다.
                for (int x = 0; x < width; x += _clouds.Width) batch.Draw(_clouds, new Vector2(x, y), Color.White);
        }
        // 타이틀 그림은 640×480 자리의 왼쪽 위에 **제 크기 그대로**(titleMenu.gif 는 639×480) 놓는다. 원본 004d0530 도 늘리지 않고 붙인다
        // (cpppj UberGump 의 Paste 위치와 같다). 2026-10-10 수정: 이전에는 640 폭으로 늘려 그려 가로로 1픽셀 번졌다.
        if (!OptionsOnly && _background != null) batch.Draw(_background, new Vector2((width - 640) / 2, (height - 480) / 2), Color.White);
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
        if (_page is "options" or "help")
        {
            // 펼침 목록 뒤에 놓이는 타이틀 버튼만 먼저 표시한다.
            foreach (MenuButton button in _buttons.Where(b => !b.ListRow))
                _skin.Button(batch, button.Bounds, button.Label, button.Enabled);
        }
        // 목록 바탕을 먼저 그린다.
        foreach (Rectangle list in _lists) _skin.Menu(batch, list);
        // 캠페인·옵션 목록의 그룹 구분선을 덧붙인다 (테두리 안쪽 폭).
        foreach (int y in _separators) _skin.Separator(batch, _lists[0].X + OriginalUiSkin.FrameThickness, y, _lists[0].Width - OriginalUiSkin.FrameThickness * 2);
        bool popup = _page is "options" or "help";
        // 목록 행은 왼쪽 정렬하고 일반 버튼만 작은 입체 테두리를 그린다. 돌 버튼에는 호버 표시가 없고 목록 행에는 있다.
        for (int index = 0; index < _buttons.Count; index++)
        {
            MenuButton button = _buttons[index];
            if (popup && !button.ListRow) continue;
            bool hover = button.Bounds.Contains(_previousMouse.Position);
            Color labelColor = button.Enabled ? Color.White : new Color(185, 180, 166);
            if (!button.ListRow) _skin.Button(batch, button.Bounds, button.Label, button.Enabled, pressed: _gump.IsPressed(index));
            else if (popup)
            {
                // 펼침 목록(옵션·도움말·하위 목록)의 행: 행 영역이 목록 폭 전체이고 글자는 목록 왼쪽 + 15, 켜짐 표시는 + 8 에 온다.
                if (button.Enabled && hover) batch.Draw(_pixel, button.Bounds, Color.Black * 0.25f);
                if (button.Check.HasValue)
                    _skin.Pip(batch, new Point(button.Bounds.X + OriginalUiSkin.MenuPipInset, button.Bounds.Y + OriginalUiSkin.RowHeight / 2), button.Check.Value, button.Enabled);
                _skin.MenuRow(batch, button.Bounds.X, button.Bounds, button.Label, labelColor, button.Key, button.Enabled ? MenuKeyColor : labelColor, button.Arrow);
            }
            else
            {
                // 창 안의 선택 목록(캠페인·미션)의 행.
                if (button.Enabled && hover) batch.Draw(_pixel, button.Bounds, Color.Black * 0.25f);
                int inset = button.Check.HasValue ? 13 : 10;
                if (button.Check.HasValue) _skin.Pip(batch, new Point(button.Bounds.X + 5, button.Bounds.Center.Y), button.Check.Value, button.Enabled);
                SpriteFontBase rowFont = font.MeasureString(button.Label).X > button.Bounds.Width - inset - 3 ? small : font;
                var labelPosition = new Vector2(button.Bounds.X + inset, button.Bounds.Center.Y - rowFont.MeasureString(button.Label).Y / 2);
                OriginalUiSkin.Text(batch, rowFont, button.Label, labelPosition, labelColor);
            }
        }
        if (_error.Length > 0) OriginalUiSkin.Text(batch, small, _error, new Vector2(16, height - 24), Color.OrangeRed);
        if (_modal != null) DrawModal(batch, font, width, height);
    }

    /// <summary>버튼과 목록 행의 영역·표시·동작.</summary>
    /// <param name="Bounds">클릭·표시 영역</param>
    /// <param name="Label">문구</param>
    /// <param name="Enabled">고를 수 있는지</param>
    /// <param name="Action">고르면 할 일</param>
    /// <param name="ListRow">목록 행인지 (아니면 돌 버튼)</param>
    /// <param name="Check">켜짐 표시 (null 이면 없음)</param>
    /// <param name="Key">문구 뒤에 노란색으로 쓸 단축키 표기 (없으면 빈 문자열)</param>
    /// <param name="Arrow">하위 목록 화살표가 붙는지</param>
    private sealed record MenuButton(Rectangle Bounds, string Label, bool Enabled, Action Action, bool ListRow, bool? Check,
        string Key = "", bool Arrow = false);

    /// <summary>메뉴 전용 텍스처를 해제한다. 공통 스킨은 게임 본체가 소유한다.</summary>
    public void Dispose() { _pixel.Dispose(); _background?.Dispose(); _clouds?.Dispose(); }
}
