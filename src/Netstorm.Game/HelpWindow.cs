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
    /// <summary>원본 도움말의 본문 줄 높이.</summary>
    private const int LineHeight = 17;
    private readonly GraphicsDevice _device;
    private readonly OriginalUiSkin _skin;
    private readonly GameResources _resources;
    private readonly HelpTopics _topics;
    private readonly HelpNavigation _navigation;
    private readonly ShapeDatabase _shapes;
    private readonly Palette _palette;
    private readonly TypeCatalog _types;
    private readonly Texture2D _pixel;
    private readonly RasterizerState _clip = new() { ScissorTestEnable = true };
    private readonly Dictionary<string, Texture2D?> _pictures = new(StringComparer.OrdinalIgnoreCase);
    private readonly List<Fragment> _fragments = [];
    private MouseState _previousMouse;
    private bool _inMission;
    private bool _dragBody;
    private bool _dragBar;
    private int _dragStartY;
    private int _dragStartX;
    private int _dragStartScroll;
    private int _contentHeight;
    private string? _layoutAnchor;
    private int _layoutWidth;
    /// <summary>클론 입력·가장자리 스크롤을 막아야 하는지.</summary>
    public bool IsOpen => _navigation.Anchor != null;
    /// <summary>UI 검사에서 현재 링크 목적지를 확인한다.</summary>
    public string State => "help:" + _navigation.Anchor;
    /// <summary>OK로 닫았을 때 지식 상세창 등의 호출자가 복귀를 처리한다.</summary>
    public Action? Closed { get; set; }
    /// <summary>지식 격자에서 열린 상세창의 Back 버튼 복귀 동작.</summary>
    public Action? BackToParent { get; set; }

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
    /// <summary>본문 옆 세로 스크롤바 영역.</summary>
    private static Rectangle Track(Rectangle body) => new(body.Right + 3, body.Y, 11, body.Height);
    /// <summary>현재 본문 길이와 스크롤에 비례하는 손잡이.</summary>
    private Rectangle Thumb(Rectangle body)
    {
        Rectangle track = Track(body);
        int h = Math.Clamp(body.Height * body.Height / Math.Max(body.Height, _contentHeight), 18, body.Height);
        int max = Math.Max(1, _contentHeight - body.Height);
        return new(track.X, track.Y + _navigation.Scroll * (track.Height - h) / max, track.Width, h);
    }

    /// <summary>태그·링크를 유지한 채 영어 어절과 긴 한국어 문자열을 실제 글꼴 폭으로 감는다.</summary>
    private void Layout(int width)
    {
        if (_layoutAnchor == _navigation.Anchor && _layoutWidth == width) return;
        _layoutAnchor = _navigation.Anchor; _layoutWidth = width; _fragments.Clear();
        float x = 0; int y = 0; int rowHeight = LineHeight;
        // 줄바꿈 시 다음 조각을 새 줄로 옮긴다.
        void NewLine(int extra = 0) { x = 0; y += rowHeight + extra; rowHeight = LineHeight; }
        string html = _topics.Find(_navigation.Anchor!) ?? "";
        // 각 조각의 링크 목적지와 글자 스타일을 줄 감기 후에도 함께 보존한다.
        foreach (HelpTextRun source in HelpDocument.Parse(html, _inMission))
        {
            // 설정 치환기는 끝 공백을 지우므로 태그 양옆 단어가 붙지 않게 원문 공백을 복원한다.
            HelpTextRun run = source with { Text = _resources.Settings.Expand(source.Text) + (source.Text.EndsWith(' ') ? " " : "") };
            if (run.BreakBefore != TutorialTextBreak.None && (x > 0 || y > 0))
                NewLine(run.BreakBefore == TutorialTextBreak.Paragraph ? 8 : 0);
            if (run.Picture is { } name)
            {
                Texture2D? picture = Picture(name);
                if (picture == null) continue;
                if (picture.Height <= LineHeight * 2 && run.BreakBefore == TutorialTextBreak.None)
                {
                    if (x + picture.Width > width) NewLine();
                    _fragments.Add(new(new((int)x, y, picture.Width, picture.Height), "", run.Style, run.Link, name));
                    x += picture.Width; rowHeight = Math.Max(rowHeight, picture.Height); continue;
                }
                if (x > 0) NewLine();
                int w = Math.Min(width, picture.Width); int h = picture.Height * w / picture.Width;
                _fragments.Add(new(new((width - w) / 2, y, w, h), "", run.Style, run.Link, name));
                y += h + 8; continue;
            }
            SpriteFontBase font = run.Style == TutorialTextStyle.Heading ? _skin.Title : _skin.Body;
            // 어절 사이 공백과 링크 조각 사이 공백을 유지한다.
            foreach (string word in System.Text.RegularExpressions.Regex.Split(run.Text, "( +)"))
            {
                if (word.Length == 0) continue;
                float size = font.MeasureString(word).X;
                if (x + size > width && x > 0) NewLine();
                if (x == 0 && string.IsNullOrWhiteSpace(word)) continue;
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
                    int h = Math.Max(LineHeight, (int)Math.Ceiling(font.MeasureString(value).Y));
                    _fragments.Add(new(new((int)x, y, w, h), value, run.Style, run.Link, null));
                    x += w; rowHeight = Math.Max(rowHeight, h);
                }
            }
        }
        _contentHeight = y + rowHeight;
    }

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
            if (parts.Length == 2 && int.TryParse(parts[1], out int numericCluster)) cluster = numericCluster;
            if (cluster < 0) cluster = type.Definition.Frames.DefaultFrame;
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
        if (down)
        {
            if (Button(panel, 0).Contains(mouse.Position))
            {
                if (!_navigation.Back() && BackToParent != null) { _navigation.Close(); BackToParent(); }
            }
            else if (Button(panel, 1).Contains(mouse.Position)) { _navigation.Close(); Closed?.Invoke(); }
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
            _navigation.Scroll = Math.Clamp(_navigation.Scroll - Math.Sign(mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue) * LineHeight * 3, 0, max);
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
        batch.Draw(_pixel, body, new Color(30, 31, 32));
        _skin.Bevel(batch, body, pressed: true);
        _skin.Menu(batch, Track(body)); _skin.Button(batch, Thumb(body), "");
        _skin.Button(batch, Button(panel, 0), "Back", _navigation.CanGoBack || BackToParent != null, Button(panel, 0).Contains(_previousMouse.Position));
        _skin.Button(batch, Button(panel, 1), "OK", hover: Button(panel, 1).Contains(_previousMouse.Position));
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
                Color color = unsupported ? Color.Gray : fragment.Link != null ? new Color(100, 160, 255) : fragment.Style == TutorialTextStyle.Emphasis ? Color.Wheat : Color.White;
                OriginalUiSkin.Text(batch, fragment.Style == TutorialTextStyle.Heading ? _skin.Title : _skin.Body,
                    fragment.Text, rect.Location.ToVector2(), color);
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
        OriginalUiSkin.Text(batch, _skin.Title, _resources.Settings.Expand(d.GetString("description") ?? type.Name), pos);
        pos.Y += 23;
        Element? theme = Elements.FromTheme(d.GetString("theme"));
        string cost = StormPower.TypeCost(d) > 0 ? StormPower.TypeCost(d).ToString() : "n/a";
        string[] stats = [$"Alignment: {theme?.ToString() ?? "None"}", $"Class: {d.GetString("class") ?? "n/a"}",
            $"Hits: {d.GetInt("maxHitPoints")?.ToString() ?? "n/a"}", $"Range: {d.GetInt("range")?.ToString() ?? "n/a"}",
            $"Damage: {(d.GetString("class") == "Shooter" ? "?" : "n/a")}", $"Cost in Storm Power: {cost}",
            $"Energy to Build: {(EnergyRequirement.ForType(d).Letters is { Length: > 0 } energy ? energy : "None")}"];
        // 능력치 머리는 본문 스크롤과 독립적으로 유지한다.
        foreach (string stat in stats) { OriginalUiSkin.Text(batch, _skin.Small, stat, pos, Color.Wheat); pos.Y += 14; }
    }

    /// <summary>그리기와 링크 클릭 판정이 공유하는 본문 조각.</summary>
    private sealed record Fragment(Rectangle Bounds, string Text, TutorialTextStyle Style, string? Link, string? Picture);
    /// <summary>본문 그림 캐시와 전용 그리기 자원을 해제한다.</summary>
    public void Dispose()
    {
        _pixel.Dispose(); _clip.Dispose();
        // 각각 생성한 원본 삽화 텍스처를 한 번씩 해제한다.
        foreach (Texture2D? texture in _pictures.Values) texture?.Dispose();
    }
}
