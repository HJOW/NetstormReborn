using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>원본 fortGump의 돌 질감·모서리·선택 표시를 공유하는 UI 그리기 도구.</summary>
internal sealed class OriginalUiSkin : IDisposable
{
    /// <summary>원본 Buttongump.cpp 004249a0에서 활성 버튼을 **누르는 순간** 재생하는 효과음 (뗄 때는 소리가 없다).</summary>
    public const string ButtonSound = "button.wav";

    /// <summary>
    /// 메뉴 항목(펼침 메뉴·대화상자 목록 행)을 누를 때 재생하는 효과음. 원본 Menugump 의 <c>FUN_00476820</c> 이 활성 항목을 누르는 순간 재생하며
    /// (2026-10-03 Options 펼침 메뉴 행 시험: 누른 뒤 +65ms), 항목은 뗄 때가 아니라 누르는 순간 실행된다.
    /// </summary>
    public const string MenuItemSound = "openSubGump.wav";

    /// <summary>하위 메뉴(">" 항목)를 펼칠 때 <see cref="MenuItemSound"/> 에 더해 재생하는 효과음 (<c>FUN_00476820</c> 정적 분석, 직접 청취로 확인하지는 않았다).</summary>
    public const string MenuOpenSound = "openGump.wav";

    /// <summary>원본 메뉴의 한 행 높이. 버튼과 펼침 목록의 입력 영역에도 사용한다.</summary>
    public const int RowHeight = 18;
    /// <summary>원본의 낮은 텍스트 버튼이 그려지는 높이.</summary>
    public const int ButtonHeight = 19;
    /// <summary>
    /// 버튼 판정이 그려진 높이보다 더 내려가는 픽셀 수. 원본 메인 메뉴 Credits 버튼은 세로 334~352(19px)로 그려지지만
    /// 334~353(20px)까지 눌린다 (2026-10-03 경계 스캔). 가로는 그려진 폭(75px)과 같다.
    /// </summary>
    public const int ButtonHitExtraHeight = 1;
    /// <summary>돌 테두리의 밝은 가장자리 색.</summary>
    private static readonly Color LightEdge = new(191, 178, 139);
    /// <summary>돌 테두리의 어두운 가장자리 색.</summary>
    private static readonly Color DarkEdge = new(49, 44, 36);
    private readonly Texture2D _pixel;
    private readonly Dictionary<string, Texture2D> _frames = new(StringComparer.OrdinalIgnoreCase);
    /// <summary>본문·버튼에 사용하는 작은 D2Coding 글꼴.</summary>
    public SpriteFontBase Body { get; }
    /// <summary>대화상자 제목에 사용하는 D2Coding 글꼴.</summary>
    public SpriteFontBase Title { get; }
    /// <summary>카드 이름·보조 정보에 사용하는 D2Coding 글꼴.</summary>
    public SpriteFontBase Small { get; }

    /// <summary>타입의 클러스터 이름으로 원본 UI 프레임을 찾아 팔레트를 적용한다.</summary>
    public OriginalUiSkin(GraphicsDevice device, ShapeDatabase shapes, Palette palette, TypeDefinition definition,
        SpriteFontBase body, SpriteFontBase title, SpriteFontBase small)
    {
        Body = body; Title = title; Small = small;
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
        ShapeBlock? block = shapes.FindBlock("fortGump");
        if (block == null) return;
        // UI에서 쓰는 클러스터만 읽어 원본 프레임 순서 변화에도 대응한다.
        foreach (string name in new[] { "A00", "A01", "A02", "A03", "A04", "I00", "I01", "I02", "I03", "J02" })
        {
            int cluster = definition.Clusters.ToList().FindIndex(c => c.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
            if (cluster < 0) continue;
            int index = MapSpriteFrames.BodyFrame(definition, cluster);
            if (index < 0 || index >= block.Frames.Count || block.Frames[index].IsSpecial) continue;
            _frames[name] = SpriteAnimation.ToTexture(device, shapes.Decode(block.Frames[index]), palette);
        }
    }

    /// <summary>질감을 확대하지 않고 반복하며 마지막 타일을 영역 경계에 맞춰 자른다.</summary>
    public void Tile(SpriteBatch batch, Rectangle area, bool menu = false)
    {
        if (!_frames.TryGetValue(menu ? "A01" : "A00", out Texture2D? texture))
        { batch.Draw(_pixel, area, new Color(93, 88, 78)); return; }
        // 세로 방향으로 돌 질감을 반복한다.
        for (int y = area.Y; y < area.Bottom; y += texture.Height)
        {
            // 가로 방향으로 돌 질감을 반복하고 끝부분은 잘라 낸다.
            for (int x = area.X; x < area.Right; x += texture.Width)
            {
                var tile = new Rectangle(x, y, Math.Min(texture.Width, area.Right - x), Math.Min(texture.Height, area.Bottom - y));
                batch.Draw(texture, tile, new Rectangle(0, 0, tile.Width, tile.Height), Color.White);
            }
        }
    }

    /// <summary>밝은 위·왼쪽과 어두운 아래·오른쪽으로 얇은 입체 테두리를 그린다.</summary>
    public void Bevel(SpriteBatch batch, Rectangle area, bool pressed = false)
    {
        Color light = pressed ? DarkEdge : LightEdge;
        Color dark = pressed ? LightEdge : DarkEdge;
        batch.Draw(_pixel, new Rectangle(area.X, area.Y, area.Width, 1), light);
        batch.Draw(_pixel, new Rectangle(area.X, area.Y, 1, area.Height), light);
        batch.Draw(_pixel, new Rectangle(area.X, area.Bottom - 1, area.Width, 1), dark);
        batch.Draw(_pixel, new Rectangle(area.Right - 1, area.Y, 1, area.Height), dark);
    }

    /// <summary>안내·선택·결과 창에 원본 회색 돌 바탕과 네 금색 모서리를 그린다.</summary>
    public void Panel(SpriteBatch batch, Rectangle area)
    {
        Tile(batch, area);
        Bevel(batch, area);
        DrawFrame(batch, "I00", new Point(area.X, area.Y));
        DrawFrame(batch, "I01", new Point(area.Right - FrameWidth("I01"), area.Y));
        DrawFrame(batch, "I02", new Point(area.Right - FrameWidth("I02"), area.Bottom - FrameHeight("I02")));
        DrawFrame(batch, "I03", new Point(area.X, area.Bottom - FrameHeight("I03")));
    }

    /// <summary>메뉴 바탕을 채우고 원본처럼 얇은 테두리만 붙인다.</summary>
    public void Menu(SpriteBatch batch, Rectangle area)
    { Tile(batch, area, menu: true); Bevel(batch, area); }

    /// <summary>목록의 그룹 사이에 밝고 어두운 두 선을 붙인다.</summary>
    public void Separator(SpriteBatch batch, int x, int y, int width)
    {
        batch.Draw(_pixel, new Rectangle(x, y, width, 1), DarkEdge);
        batch.Draw(_pixel, new Rectangle(x, y + 1, width, 1), LightEdge);
    }

    /// <summary>글자를 픽셀 좌표에 맞춰 1픽셀 검은 그림자와 함께 쓴다.</summary>
    public static void Text(SpriteBatch batch, SpriteFontBase font, string text, Vector2 position, Color? color = null)
    {
        position = new Vector2(MathF.Round(position.X), MathF.Round(position.Y));
        batch.DrawString(font, text, position + Vector2.One, Color.Black * 0.9f);
        batch.DrawString(font, text, position, color ?? Color.White);
    }

    /// <summary>
    /// 그려진 버튼 영역을 원본의 판정 영역으로 바꾼다 (<see cref="ButtonHitExtraHeight"/> 만큼 아래로 길다).
    /// </summary>
    /// <param name="id">버튼 번호</param>
    /// <param name="drawn">버튼이 그려지는 영역</param>
    /// <param name="enabled">눌릴 수 있는지</param>
    public static Netstorm.Core.Rules.GumpButton Hit(int id, Rectangle drawn, bool enabled = true) =>
        new(id, drawn.X, drawn.Y, drawn.Width, drawn.Height + ButtonHitExtraHeight, enabled);

    /// <summary>
    /// 낮은 돌 버튼을 누름·비활성 상태와 함께 그린다. 원본에는 호버 표시와 키보드 포커스 표시가 없다.
    /// 눌린 모양은 돌 무늬를 어둡게 하지 않고 테두리 명암을 뒤집으며 글자를 오른쪽·아래로 1픽셀 옮긴다
    /// (2026-10-03 녹화: 눌린 프레임과 평소 프레임의 차이는 테두리와 글자뿐이었다).
    /// </summary>
    public void Button(SpriteBatch batch, Rectangle area, string label, bool enabled = true, bool pressed = false)
    {
        Tile(batch, area, menu: true);
        batch.Draw(_pixel, area, Color.Black * (enabled ? 0.16f : 0.45f));
        Bevel(batch, area, pressed);
        SpriteFontBase font = Body.MeasureString(label).X > area.Width - 6 ? Small : Body;
        Vector2 size = font.MeasureString(label);
        Text(batch, font, label, new Vector2(area.Center.X - size.X / 2 + (pressed ? 1 : 0),
            area.Center.Y - size.Y / 2 + (pressed ? 1 : 0)), enabled ? Color.White : new Color(145, 141, 130));
    }

    /// <summary>원본의 작은 파란 원으로 선택·켜짐 상태를 표시한다.</summary>
    public void Pip(SpriteBatch batch, Point center, bool selected, bool enabled = true)
    {
        if (!selected) return;
        if (_frames.TryGetValue("J02", out Texture2D? texture))
            batch.Draw(texture, new Vector2(center.X - texture.Width / 2, center.Y - texture.Height / 2), enabled ? Color.White : Color.Gray);
        else batch.Draw(_pixel, new Rectangle(center.X - 1, center.Y - 1, 3, 3), enabled ? Color.LightSkyBlue : Color.Gray);
    }

    /// <summary>찾아 둔 프레임을 원본 픽셀 크기로 표시한다.</summary>
    public void DrawFrame(SpriteBatch batch, string name, Point position)
    { if (_frames.TryGetValue(name, out Texture2D? frame)) batch.Draw(frame, position.ToVector2(), Color.White); }

    /// <summary>원본 UI 프레임의 폭. 자산이 없으면 장식도 공간을 차지하지 않는다.</summary>
    public int FrameWidth(string name) => _frames.TryGetValue(name, out Texture2D? frame) ? frame.Width : 0;
    /// <summary>원본 UI 프레임의 높이.</summary>
    public int FrameHeight(string name) => _frames.TryGetValue(name, out Texture2D? frame) ? frame.Height : 0;

    /// <summary>문단의 빈 줄을 보존하고 실제 글꼴 폭으로 한국어·영어 본문을 감는다.</summary>
    public IReadOnlyList<string> WrapText(string text, int width)
    {
        var lines = new List<string>();
        // 명시한 줄바꿈은 유지하면서 각 문단을 감는다.
        foreach (string paragraph in text.Split('\n'))
        {
            string line = "";
            // 단어를 우선 묶어 영어·한국어의 어절을 보존한다.
            foreach (string word in paragraph.Split(' ', StringSplitOptions.RemoveEmptyEntries))
            {
                string next = line.Length == 0 ? word : line + " " + word;
                if (line.Length > 0 && Body.MeasureString(next).X > width) { lines.Add(line); line = ""; }
                if (Body.MeasureString(word).X <= width) line = line.Length == 0 ? word : line + " " + word;
                else
                {
                    // 한 단어가 창보다 길면 글자별로 나눠 화면 밖으로 넘치지 않게 한다.
                    foreach (char letter in word)
                    {
                        if (line.Length > 0 && Body.MeasureString(line + letter).X > width) { lines.Add(line); line = ""; }
                        line += letter;
                    }
                }
            }
            lines.Add(line);
        }
        return lines;
    }

    /// <summary>게임이 끝날 때 공유 UI 텍스처를 해제한다.</summary>
    public void Dispose()
    {
        _pixel.Dispose();
        // 생성한 프레임 텍스처를 한 번씩 해제한다.
        foreach (Texture2D frame in _frames.Values) frame.Dispose();
    }
}
