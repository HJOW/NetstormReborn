using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>원본 도움말의 내부 링크·본문 드래그·스크롤바·Back/OK를 공유하는 창.</summary>
internal sealed class HelpWindow : IDisposable
{
    /// <summary>
    /// 원본 도움말의 본문 줄 간격 (원본 캡처의 목록 줄 시작이 15픽셀 간격).
    /// 2026-10-11: 이전 값 17 은 녹화에서 어림한 것이었다. 원본 캡처로 목록 8줄의 간격이 모두 15 임을 확인했다.
    /// </summary>
    private const int OriginalLineHeight = 15;
    /// <summary>줄 배치·화살표·휠에 같은 높이를 쓴다. 한국어는 D2Coding의 실제 줄 높이보다 좁게 이동하지 않는다.</summary>
    private int BodyLineHeight => _skin.UsesOriginalFonts ? OriginalLineHeight : Math.Max(OriginalLineHeight, _skin.Plain.LineHeight);
    /// <summary>문단(&lt;p&gt;) 앞에 줄 간격 말고 더하는 간격. 원본 캡처에서 "GAME SUPPORT" 앞이 줄 간격 + 14 였다.</summary>
    private const int ParagraphGap = 14;
    /// <summary>
    /// 원본 글꼴 제목 줄의 높이. 제목 윗부분에서 첫 문단 윗부분까지가 35픽셀(= 21 + 문단 간격 14)이었다.
    /// 한국어(D2Coding)에서는 글꼴이 재는 높이를 쓴다.
    /// </summary>
    private const int HeadingRowHeight = 21;
    /// <summary>스크롤바 폭과 위·아래 화살표 단추 한 변 (녹화 g1940 의 오른쪽 화살표 스크롤바)</summary>
    private const int ScrollBarSize = 11;
    /// <summary>본문 바탕색 (원본 캡처 본문 바탕 (54, 51, 53))</summary>
    private static readonly Color BodyColor = new(54, 51, 53);
    /// <summary>
    /// 본문 글자색 (원본 캡처 보통·굵은 글씨의 값 (217, 212, 203)). 본문은 그림자 없이 그린다.
    /// 흰색에 검은 그림자를 쓰는 것은 제목과 클릭되는 링크뿐이다.
    /// </summary>
    private static readonly Color TextColor = new(217, 212, 203);
    /// <summary>링크 글자색 (원본 캡처의 링크·"blue" 값 (154, 209, 205)). 클릭되는 링크는 그림자와 함께 그린다.</summary>
    private static readonly Color LinkColor = new(154, 209, 205);
    /// <summary>능력치 머리 값의 노란 글자색 (녹화 "Alignment: None" 의 None)</summary>
    private static readonly Color StatValueColor = OriginalUiSkin.ValueColor;
    /// <summary>스크롤바 홈 색 (녹화 표본 약 48,48,48)</summary>
    private static readonly Color TrackColor = new(48, 48, 48);
    /// <summary>_device: 텍스처 생성과 화면 출력을 수행할 그래픽 장치.</summary>
    private readonly GraphicsDevice _device;
    /// <summary>_skin: 원본 UI 프레임·버튼·배경을 그리는 공통 외관 자료.</summary>
    private readonly OriginalUiSkin _skin;
    /// <summary>_resources: 언어·미션·설정·게임 자산을 조회하는 공통 자원.</summary>
    private readonly GameResources _resources;
    /// <summary>_topics: 도움말 주제 이름과 문서 위치를 연결하는 목록.</summary>
    private readonly HelpTopics _topics;
    /// <summary>_navigation: 도움말 앵커 이동과 뒤로 가기를 관리하는 탐색 상태.</summary>
    private readonly HelpNavigation _navigation;
    /// <summary>_shapes: 타입 프레임의 크기와 픽셀을 조회하는 셰이프 DB.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>_palette: 스프라이트 인덱스를 실제 색으로 바꿀 현재 팔레트.</summary>
    private readonly Palette _palette;
    /// <summary>_types: 타입 이름·해시와 프레임 속성을 조회하는 카탈로그.</summary>
    private readonly TypeCatalog _types;
    /// <summary>_pixel: 사각형·선·단색 배경을 그릴 때 늘려 쓰는 1×1 텍스처.</summary>
    private readonly Texture2D _pixel;
    /// <summary>_clip: 본문 영역 밖의 픽셀을 자르도록 Scissor 검사를 켠 래스터 상태.</summary>
    private readonly RasterizerState _clip = new() { ScissorTestEnable = true };
    /// <summary>_pictures: 도움말 그림 이름별 텍스처 캐시. 읽지 못한 그림은 null로 저장한다.</summary>
    private readonly Dictionary<string, Texture2D?> _pictures = new(StringComparer.OrdinalIgnoreCase);
    /// <summary>_fragments: 도움말 본문을 배치한 텍스트·그림·링크 조각.</summary>
    private readonly List<Fragment> _fragments = [];
    /// <summary>_previousMouse: 클릭·해제·휠 변화를 검출할 이전 프레임 마우스 상태.</summary>
    private MouseState _previousMouse;
    /// <summary>Back(0)·OK(1) 버튼의 누름·떼기 처리기 (원본 돌 버튼 규칙).</summary>
    private readonly ButtonGump _gump = new();

    /// <summary>지금 눌린 모양인 버튼의 글자 (없으면 null). 자동 UI 검사가 읽는다.</summary>
    public string? PressedLabel => _gump.Held is int held && _gump.IsPressed(held) ? (held == 0 ? "Back" : "OK") : null;
    /// <summary>_inMission: 도움말 창을 미션 안에서 열었는지.</summary>
    private bool _inMission;
    /// <summary>_dragBody: 본문을 잡고 스크롤하는 중인지.</summary>
    private bool _dragBody;
    /// <summary>_dragBar: 스크롤바를 잡고 이동하는 중인지.</summary>
    private bool _dragBar;
    /// <summary>_dragStartY: 드래그가 시작된 마우스 Y 좌표.</summary>
    private int _dragStartY;
    /// <summary>_dragStartX: 드래그가 시작된 마우스 X 좌표.</summary>
    private int _dragStartX;
    /// <summary>_dragStartScroll: 드래그 시작 시 본문의 스크롤 위치.</summary>
    private int _dragStartScroll;
    /// <summary>_contentHeight: 줄 배치 뒤 계산한 본문 전체 높이.</summary>
    private int _contentHeight;
    /// <summary>_layoutAnchor: 현재 줄 배치를 계산한 도움말 앵커.</summary>
    private string? _layoutAnchor;
    /// <summary>_layoutWidth: 현재 줄 배치를 계산한 본문 너비.</summary>
    private int _layoutWidth;
    /// <summary>클론 입력·가장자리 스크롤을 막아야 하는지.</summary>
    public bool IsOpen => _navigation.Anchor != null;
    /// <summary>UI 검사에서 현재 링크 목적지를 확인한다.</summary>
    public string State => "help:" + _navigation.Anchor;
    /// <summary>화면 자동 검사에 실제 본문 높이·스크롤·언어별 줄 높이를 제공한다. 사용자 화면에는 표시하지 않는다.</summary>
    public string DiagnosticDetail(int width, int height)
    {
        if (!IsOpen) return "";
        Rectangle body = Body(Panel(width, height)); Layout(body.Width - 10);
        return $";help-scroll={_navigation.Scroll};help-max={Math.Max(0, _contentHeight - body.Height)}"
            + $";help-content={_contentHeight};help-line={BodyLineHeight};help-body-top={body.Y}"
            + $";help-mission={(_inMission ? "true" : "false")};help-info={InformationType()?.Name ?? "none"}";
    }
    /// <summary>OK로 닫았을 때 지식 상세창 등의 호출자가 복귀를 처리한다.</summary>
    public Action? Closed { get; set; }
    /// <summary>지식 격자에서 열린 상세창의 Back 버튼 복귀 동작.</summary>
    public Action? BackToParent { get; set; }
    /// <summary>실제 도움말 버튼의 효과음 요청을 게임 공통 재생기로 넘긴다.</summary>
    public Action<string>? SoundRequested { get; set; }

    /// <summary>게임 자산과 같은 도움말 원문·돌 스킨·글꼴을 연결한다.</summary>
    public HelpWindow(GraphicsDevice device, OriginalUiSkin skin, GameResources resources, HelpTopics topics,
        ShapeDatabase shapes, Palette palette)
    {
        _device = device; _skin = skin; _resources = resources; _topics = topics;
        _shapes = shapes; _palette = palette; _types = resources.LoadTypes(); _navigation = new(topics);
        _pixel = new(device, 1, 1); _pixel.SetData(new[] { Color.White });
    }

    /// <summary>일반 도움말이나 About 주제를 새 방문 기록으로 연다.</summary>
    public bool Open(string anchor, bool inMission, MouseState mouse)
    {
        if (!_navigation.Open(anchor)) return false;
        _inMission = inMission; _previousMouse = mouse; _layoutAnchor = null;
        _dragBar = _dragBody = false; return true;
    }

    /// <summary>info 주제의 타입을 찾아 고정 능력치 머리로 쓴다.</summary>
    private TypeInfo? InformationType() => _navigation.Anchor is { } anchor && _topics.HasInformation(anchor)
        && anchor.EndsWith("Type", StringComparison.OrdinalIgnoreCase) ? _types.Find(anchor[..^4]) : null;

    /// <summary>원본 도움말 크기를 쓰되 작은 창에서는 전체 영역을 화면 안에 둔다.</summary>
    private Rectangle Panel(int width, int height)
    {
        int w = Math.Min(450, width - 24); int h = Math.Min(350, height - 24); // 녹화 대조: 능력치 머리가 있는 주제도 목차와 같은 높이 350이다
        return new((width - w) / 2, Math.Min(40, (height - h) / 2), w, h);
    }
    /// <summary>고정 머리를 제외한 독립적인 본문 영역.</summary>
    private Rectangle Body(Rectangle panel)
    {
        int top = InformationType() == null ? 18 : 152; // 녹화 대조: 머리 아래 본문 시작 높이 152
        return new(panel.X + 16, panel.Y + top, panel.Width - 46, panel.Height - top - 44);
    }
    /// <summary>Back·OK의 버튼 영역.</summary>
    private static Rectangle Button(Rectangle panel, int index) => new(panel.Center.X - 68 + index * 80,
        panel.Bottom - 29, 56, OriginalUiSkin.ButtonHeight);
    /// <summary>본문 옆 세로 스크롤바 전체 영역 (화살표 단추 포함).</summary>
    private static Rectangle Bar(Rectangle body) => new(body.Right + 3, body.Y, ScrollBarSize, body.Height);
    /// <summary>위로 한 줄 올리는 화살표 단추.</summary>
    private static Rectangle UpArrow(Rectangle body) => new(body.Right + 3, body.Y, ScrollBarSize, ScrollBarSize);
    /// <summary>아래로 한 줄 내리는 화살표 단추.</summary>
    private static Rectangle DownArrow(Rectangle body) => new(body.Right + 3, body.Bottom - ScrollBarSize, ScrollBarSize, ScrollBarSize);
    /// <summary>두 화살표 사이의 손잡이 홈.</summary>
    private static Rectangle Track(Rectangle body) => new(body.Right + 3, body.Y + ScrollBarSize, ScrollBarSize, Math.Max(1, body.Height - ScrollBarSize * 2));
    /// <summary>현재 본문 길이와 스크롤에 비례하는 손잡이.</summary>
    private Rectangle Thumb(Rectangle body)
    {
        Rectangle track = Track(body);
        int h = Math.Clamp(track.Height * body.Height / Math.Max(body.Height, _contentHeight), Math.Min(18, track.Height), track.Height);
        int max = Math.Max(1, _contentHeight - body.Height);
        return new(track.X, track.Y + _navigation.Scroll * (track.Height - h) / max, track.Width, h);
    }

    /// <summary>태그·링크를 유지한 채 영어 어절과 긴 한국어 문자열을 실제 글꼴 폭으로 감는다.</summary>
    private void Layout(int width)
    {
        if (_layoutAnchor == _navigation.Anchor && _layoutWidth == width) return;
        _layoutAnchor = _navigation.Anchor; _layoutWidth = width; _fragments.Clear();
        float x = 0; int y = 0; int rowHeight = BodyLineHeight;
        // 줄바꿈 시 다음 조각을 새 줄로 옮긴다.
        void NewLine(int extra = 0) { x = 0; y += rowHeight + extra; rowHeight = BodyLineHeight; }
        string html = _topics.Find(_navigation.Anchor!) ?? "";
        // 각 조각의 링크 목적지와 글자 스타일을 줄 감기 후에도 함께 보존한다.
        foreach (HelpTextRun source in HelpDocument.Parse(html, _inMission))
        {
            // 설정 치환기는 끝 공백을 지우므로 태그 양옆 단어가 붙지 않게 원문 공백을 복원한다.
            HelpTextRun run = source with { Text = _resources.Settings.Expand(source.Text) + (source.Text.EndsWith(' ') ? " " : "") };
            if (run.BreakBefore != TutorialTextBreak.None && (x > 0 || y > 0))
                NewLine(run.BreakBefore == TutorialTextBreak.Paragraph ? ParagraphGap : 0);
            if (run.Picture is { } name)
            {
                Texture2D? picture = Picture(name);
                if (picture == null) continue;
                if (picture.Height <= BodyLineHeight * 2 && run.BreakBefore == TutorialTextBreak.None)
                {
                    if (x + picture.Width > width) NewLine();
                    _fragments.Add(new(new((int)x, y, picture.Width, picture.Height), "", run.Style, run.Link, name, run.Bold));
                    x += picture.Width; rowHeight = Math.Max(rowHeight, picture.Height); continue;
                }
                if (x > 0) NewLine();
                int w = Math.Min(width, picture.Width); int h = picture.Height * w / picture.Width;
                _fragments.Add(new(new((width - w) / 2, y, w, h), "", run.Style, run.Link, name, run.Bold));
                y += h + 8; continue;
            }
            SpriteFontBase font = RunFont(run.Style, run.Bold);
            // 어절 사이 공백과 링크 조각 사이 공백을 유지한다.
            foreach (string word in System.Text.RegularExpressions.Regex.Split(run.Text, "( +)"))
            {
                if (word.Length == 0) continue;
                float size = font.MeasureString(word).X;
                bool blank = string.IsNullOrWhiteSpace(word);
                // 줄 끝에 걸린 공백은 새 줄을 만들지 않고 버린다 (이전에는 한 줄이 꽉 차면 빈 줄이 하나 생겼다).
                if (blank && x + size > width) continue;
                if (x + size > width && x > 0) NewLine();
                if (x == 0 && blank) continue;
                string part = "";
                // 창 폭보다 긴 단어도 글자 단위로 나눠 잘림 없이 읽을 수 있게 한다.
                foreach (char letter in word)
                {
                    if (x + font.MeasureString(part + letter).X > width && part.Length > 0)
                    { AddPart(part); NewLine(); part = ""; }
                    part += letter;
                }
                AddPart(part);
                // 그리기와 클릭 판정에 같은 조각의 좌표를 사용한다.
                void AddPart(string value)
                {
                    int w = (int)Math.Ceiling(font.MeasureString(value).X);
                    int h = run.Style == TutorialTextStyle.Heading && _skin.UsesOriginalFonts ? HeadingRowHeight
                        : Math.Max(BodyLineHeight, (int)Math.Ceiling(font.MeasureString(value).Y));
                    _fragments.Add(new(new((int)x, y, w, h), value, run.Style, run.Link, null, run.Bold));
                    x += w; rowHeight = Math.Max(rowHeight, h);
                }
            }
        }
        _contentHeight = y + rowHeight;
    }

    /// <summary>
    /// 본문 조각의 글꼴: 제목은 제목 글꼴, &lt;b&gt; 안의 글씨는 본문·버튼 글꼴, 나머지는 보통 굵기 글꼴.
    /// &lt;c&gt;는 색만 바꾸므로 &lt;b&gt;&lt;c&gt;로 중첩하면 색과 굵기를 함께 적용한다.
    /// 줄 감기(<see cref="Layout"/>)와 그리기가 같은 글꼴을 쓰도록 한 곳에서 정한다. 한국어 화면에서는 굵은·보통이 같은 D2Coding 이다.
    /// </summary>
    /// <param name="style">조각의 스타일</param>
    /// <param name="bold">색·링크 태그 바깥의 굵은 태그가 열려 있는지</param>
    /// <remarks>
    /// 원본 도움말 캡처(screenShots/help - NetStorm Instructions.png)에서 본문은 Arial 14 보통, "GAME HELP" 는 Arial 14 굵게와 일치했다.
    /// 색 글씨와 굵은 글씨는 다른 스타일로 해석하며 실제 그리기에서도 구별한다.
    /// </remarks>
    private SpriteFontBase RunFont(TutorialTextStyle style, bool bold) => style switch
    {
        TutorialTextStyle.Heading => _skin.Title,
        TutorialTextStyle.Highlight => _skin.Body,
        _ => bold ? _skin.Body : _skin.Plain,
    };

    /// <summary>삽화 태그의 타입·클러스터를 원본 셰이프 프레임으로 읽고 캐시한다.</summary>
    private Texture2D? Picture(string name)
    {
        if (_pictures.TryGetValue(name, out Texture2D? cached)) return cached;
        string[] parts = name.Split('.', 2);
        TypeInfo? type = _types.Find(parts[0]);
        ShapeBlock? block = _shapes.FindBlock(parts[0]);
        Texture2D? picture = null;
        if (type != null && block != null)
        {
            int cluster = parts.Length < 2 || parts[1] == "*" ? type.Definition.Frames.HelpFrame
                : type.Definition.Clusters.ToList().FindIndex(c => c.Name.Equals(parts[1], StringComparison.OrdinalIgnoreCase));
            // 숫자는 원본 그림 파일의 프레임 번호다 (예: mana.8 = mana.gif 8번 = 클러스터 A01, help.english 의 "mana.8 - Wind").
            // 그런 클러스터가 없을 때만 클러스터 순번으로 보고, 범위를 벗어나면 기본 프레임을 쓴다 (2026-10-02 미션 도움말 종료 결함 수정).
            if (parts.Length == 2 && int.TryParse(parts[1], out int number))
            {
                int byImage = type.Definition.Clusters.ToList().FindIndex(c => c.Layers.Count > 0 && c.Layers[0].Frame == number);
                cluster = byImage >= 0 ? byImage : number;
            }
            if (cluster < 0 || cluster >= type.Definition.Clusters.Count) cluster = type.Definition.Frames.DefaultFrame;
            int frame = MapSpriteFrames.BodyFrame(type.Definition, cluster);
            if (frame >= 0 && frame < block.Frames.Count && !block.Frames[frame].IsSpecial)
                picture = SpriteAnimation.ToTexture(_device, _shapes.Decode(block.Frames[frame]), _palette);
        }
        _pictures[name] = picture; return picture;
    }

    /// <summary>リンク 클릭·본문 드래그·손잡이 드래그를 처리한다. Esc는 원본처럼 창을 닫지 않는다.</summary>
    public void Update(MouseState mouse, KeyboardState keyboard, int width, int height)
    {
        if (!IsOpen) return;
        Rectangle panel = Panel(width, height); Rectangle body = Body(panel); Layout(body.Width - 10);
        int max = Math.Max(0, _contentHeight - body.Height);
        _navigation.Scroll = Math.Clamp(_navigation.Scroll, 0, max);
        bool down = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        bool up = mouse.LeftButton == ButtonState.Released && _previousMouse.LeftButton == ButtonState.Pressed;
        // Back·OK 는 원본 돌 버튼 규칙: 누르는 순간 소리, 눌린 채 안쪽에서 뗄 때 실행 (Back 은 되돌아갈 곳이 없으면 비활성)
        GumpResult result = _gump.Update(
            [OriginalUiSkin.Hit(0, Button(panel, 0), _navigation.CanGoBack || BackToParent != null), OriginalUiSkin.Hit(1, Button(panel, 1))],
            mouse.X, mouse.Y, mouse.LeftButton == ButtonState.Pressed);
        if (result.Pressed != null) SoundRequested?.Invoke(OriginalUiSkin.ButtonSound);
        if (result.Activated == 0)
        {
            if (!_navigation.Back() && BackToParent != null) { _navigation.Close(); BackToParent(); }
            _previousMouse = mouse;
            return;
        }
        if (result.Activated == 1)
        {
            _navigation.Close(); Closed?.Invoke();
            _previousMouse = mouse;
            return;
        }
        if (down)
        {
            if (Button(panel, 0).Contains(mouse.Position) || Button(panel, 1).Contains(mouse.Position)) { }
            else if (UpArrow(body).Contains(mouse.Position)) _navigation.Scroll = Math.Clamp(_navigation.Scroll - BodyLineHeight, 0, max);
            else if (DownArrow(body).Contains(mouse.Position)) _navigation.Scroll = Math.Clamp(_navigation.Scroll + BodyLineHeight, 0, max);
            else if (Track(body).Contains(mouse.Position))
            {
                _dragBar = true;
                if (!Thumb(body).Contains(mouse.Position)) _navigation.Scroll = Math.Clamp((mouse.Y - body.Y) * max / Math.Max(1, body.Height), 0, max);
            }
            else if (body.Contains(mouse.Position)) _dragBody = true;
            _dragStartX = mouse.X; _dragStartY = mouse.Y; _dragStartScroll = _navigation.Scroll;
        }
        if (mouse.LeftButton == ButtonState.Pressed)
        {
            if (_dragBody) _navigation.Scroll = Math.Clamp(_dragStartScroll + _dragStartY - mouse.Y, 0, max);
            if (_dragBar) _navigation.Scroll = Math.Clamp(_dragStartScroll + (mouse.Y - _dragStartY) * max / Math.Max(1, body.Height - Thumb(body).Height), 0, max);
        }
        if (up)
        {
            if (_dragBody && Math.Abs(mouse.X - _dragStartX) < 4 && Math.Abs(mouse.Y - _dragStartY) < 4 && body.Contains(mouse.Position))
            {
                var point = new Point(mouse.X - body.X - 5, mouse.Y - body.Y + _navigation.Scroll);
                Fragment? hit = _fragments.FirstOrDefault(f => f.Link != null && f.Bounds.Contains(point));
                if (hit?.Link is { } target)
                {
                    if (!_navigation.Follow(target) && Uri.TryCreate(target, UriKind.Absolute, out Uri? uri)
                        && uri.Scheme is "http" or "https")
                    {
                        try { System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true }); }
                        catch (System.ComponentModel.Win32Exception) { }
                    }
                }
            }
            _dragBody = _dragBar = false;
        }
        if (body.Contains(mouse.Position) && mouse.ScrollWheelValue != _previousMouse.ScrollWheelValue)
            _navigation.Scroll = Math.Clamp(_navigation.Scroll - Math.Sign(mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue) * BodyLineHeight * 3, 0, max);
        _previousMouse = mouse;
    }

    /// <summary>고정 머리·본문·손잡이·Back/OK를 그린다. 본문만 시저 영역으로 잘라 스크롤한다.</summary>
    public void Draw(SpriteBatch batch, int width, int height)
    {
        if (!IsOpen) return;
        Rectangle panel = Panel(width, height); Rectangle body = Body(panel); Layout(body.Width - 10);
        _navigation.Scroll = Math.Clamp(_navigation.Scroll, 0, Math.Max(0, _contentHeight - body.Height));
        _skin.Panel(batch, panel);
        if (InformationType() is { } type) DrawInformation(batch, panel, type);
        batch.Draw(_pixel, body, BodyColor);
        _skin.Bevel(batch, body, pressed: true);
        // 원본처럼 어두운 홈 양 끝에 화살표 단추를 두고 사이에 손잡이를 그린다
        batch.Draw(_pixel, Bar(body), TrackColor);
        DrawArrow(batch, UpArrow(body), up: true); DrawArrow(batch, DownArrow(body), up: false);
        _skin.Button(batch, Thumb(body), "");
        _skin.Button(batch, Button(panel, 0), "Back", _navigation.CanGoBack || BackToParent != null, pressed: _gump.IsPressed(0));
        _skin.Button(batch, Button(panel, 1), "OK", pressed: _gump.IsPressed(1));
        batch.End();
        Rectangle old = _device.ScissorRectangle; _device.ScissorRectangle = body;
        batch.Begin(samplerState: SamplerState.PointClamp, rasterizerState: _clip);
        // 스크롤 본문만 잘라 링크와 삽화가 창 밖으로 넘치지 않게 한다.
        foreach (Fragment fragment in _fragments)
        {
            Rectangle rect = fragment.Bounds; rect.Offset(body.X + 5, body.Y - _navigation.Scroll);
            if (!rect.Intersects(body)) continue;
            if (fragment.Picture is { } name && Picture(name) is { } picture) batch.Draw(picture, rect, Color.White);
            else
            {
                bool unsupported = fragment.Link?.StartsWith("cmd:", StringComparison.OrdinalIgnoreCase) == true;
                Color color = unsupported ? Color.Gray
                    : fragment.Link != null || fragment.Style == TutorialTextStyle.LinkColor ? LinkColor
                    : fragment.Style == TutorialTextStyle.Emphasis ? OriginalUiSkin.EmphasisColor
                    : fragment.Style == TutorialTextStyle.ColorText ? OriginalUiSkin.ValueColor
                    : fragment.Style == TutorialTextStyle.Heading ? Color.White : TextColor;
                // 원본 도움말은 제목과 클릭되는 링크만 그림자를 쓰고 본문·굵은 글씨·색 글씨는 그림자 없이 그린다.
                if (fragment.Style == TutorialTextStyle.Heading || fragment.Link != null)
                    OriginalUiSkin.Text(batch, RunFont(fragment.Style, fragment.Bold), fragment.Text, rect.Location.ToVector2(), color);
                else OriginalUiSkin.PlainText(batch, RunFont(fragment.Style, fragment.Bold), fragment.Text, rect.Location.ToVector2(), color);
            }
        }
        batch.End(); _device.ScissorRectangle = old; batch.Begin(samplerState: SamplerState.PointClamp);
    }

    /// <summary>원본 info 태그의 그림과 실제 타입 수치를 본문 위에 고정한다.</summary>
    private void DrawInformation(SpriteBatch batch, Rectangle panel, TypeInfo type)
    {
        var pictureBox = new Rectangle(panel.X + 16, panel.Y + 18, 156, 120); // 녹화 대조: 초상화 칸은 가로 약 156
        batch.Draw(_pixel, pictureBox, new Color(40, 52, 40));
        if (Picture(type.Name + ".*") is { } picture)
        {
            // 원본은 초상화를 칸에 가득 채워 그린다(위아래 빈 띠 없음).
            float scale = Math.Min(pictureBox.Width / (float)picture.Width, pictureBox.Height / (float)picture.Height);
            var size = new Vector2(picture.Width, picture.Height) * scale;
            batch.Draw(picture, new Vector2(pictureBox.Center.X, pictureBox.Center.Y) - size / 2,
                null, Color.White, 0, Vector2.Zero, scale, SpriteEffects.None, 0);
        }
        TypeDefinition d = type.Definition;
        Vector2 pos = new(pictureBox.Right + 8, panel.Y + 16);
        OriginalUiSkin.Text(batch, _skin.Title,
            GameUnitNames.Format(_resources.Settings.Expand(d.GetString("description") ?? type.Name), _resources.Language), pos);
        pos.Y += 23;
        Element? theme = Elements.FromTheme(d.GetString("theme"));
        string cost = StormPower.TypeCost(d) > 0 ? StormPower.TypeCost(d).ToString() : "n/a";
        (string Label, string Value)[] stats = [("Alignment: ", theme?.ToString() ?? "None"), ("Class: ", d.GetString("class") ?? "n/a"),
            ("Hits: ", d.GetInt("maxHitPoints")?.ToString() ?? "n/a"), ("Range: ", d.GetInt("range")?.ToString() ?? "n/a"),
            ("Damage: ", d.GetString("class") == "Shooter" ? "?" : "n/a"), ("Cost in Storm Power: ", cost),
            ("Energy to Build: ", EnergyRequirement.ForType(d).Letters is { Length: > 0 } energy ? energy : "None")];
        // 능력치 머리는 본문 스크롤과 독립적으로 유지한다. 녹화처럼 이름은 흰색, 값은 노란색이다.
        foreach ((string label, string value) in stats)
        {
            OriginalUiSkin.Text(batch, _skin.Small, label, pos);
            OriginalUiSkin.Text(batch, _skin.Small, value, pos + new Vector2(_skin.Small.MeasureString(label).X, 0), StatValueColor);
            pos.Y += 14;
        }
    }

    /// <summary>스크롤바 끝의 돌 화살표 단추를 그린다 (삼각형은 한 줄씩 폭을 줄여 찍는다).</summary>
    private void DrawArrow(SpriteBatch batch, Rectangle area, bool up)
    {
        _skin.Button(batch, area, "");
        int rows = area.Width / 2 - 1;
        // 위에서부터 한 줄씩 그린다: 위 화살표는 꼭짓점이 위(좁게 시작), 아래 화살표는 꼭짓점이 아래(넓게 시작)다
        for (int row = 0; row < rows; row++)
        {
            int half = up ? row : rows - 1 - row;
            batch.Draw(_pixel, new Rectangle(area.Center.X - half - 1, area.Y + 3 + row, half * 2 + 1, 1), Color.Black * 0.8f);
        }
    }

    /// <summary>그리기와 링크 클릭 판정이 공유하는 본문 조각.</summary>
    /// <param name="Bounds">본문 조각의 화면 영역. 출력과 링크 적중 판정에 사용한다.</param>
    /// <param name="Text">이 구간에 출력할 텍스트.</param>
    /// <param name="Style">글꼴·색·정렬에 적용할 텍스트 스타일.</param>
    /// <param name="Link">클릭하면 이동할 도움말 앵커. 없으면 null.</param>
    /// <param name="Picture">텍스트 대신 출력할 그림. 없으면 null.</param>
    /// <param name="Bold">굵은 글씨 효과를 적용할지.</param>
    private sealed record Fragment(Rectangle Bounds, string Text, TutorialTextStyle Style, string? Link, string? Picture, bool Bold);
    /// <summary>본문 그림 캐시와 전용 그리기 자원을 해제한다.</summary>
    public void Dispose()
    {
        _pixel.Dispose(); _clip.Dispose();
        // 각각 생성한 원본 삽화 텍스처를 한 번씩 해제한다.
        foreach (Texture2D? texture in _pictures.Values) texture?.Dispose();
    }
}
