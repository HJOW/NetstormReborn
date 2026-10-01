using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>
/// 지식 창("View Netstorm Knowledge" F6, 브리핑의 Review Knowledge 버튼). 원본 ShowTechnology(exe 0x492b60)와
/// 2026-09-30 사용자 플레이 녹화의 화면(docs/videos/the-war-begins-record-play-20260930.md 3절 5·6번)을 따른다.
/// <list type="bullet">
/// <item>SUN · WIND · RAIN · THUN. 네 행. 행 머리 칸은 원소 이름(위)과 원소 기호(아래), 카드는 그림과 이름(.type description).</item>
/// <item>카드 목록은 맵(.fort) Technology 의 지식 플래그 타입 + 플레이어가 배운 지식이며 순서는 <see cref="KnowledgeCatalog"/> 규칙이다.</item>
/// <item>마우스가 올라간 카드는 어둡게 그린다. 카드를 누르면 상세창(그림·수치·help 본문·Back·OK)이 열린다.</item>
/// <item>원본처럼 게임 코드가 직접 여는 창이라 게임 시간을 멈추지 않는다 (녹화에서 창이 열린 채 SP 가 늘었다).</item>
/// </list>
/// 원본 창 그림(돌 질감 gump)은 아직 찾지 못해 단색 상자로 대신한다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>행 머리 칸 폭 (원본 1024×768 화면 측정 약 70px)</summary>
    private const int KnowledgeHeaderWidth = 70;

    /// <summary>카드 크기 (원본 측정 약 70×103px)</summary>
    private static readonly Point KnowledgeCardSize = new(70, 103);

    /// <summary>카드 가로·세로 간격 (원본 측정 73·106px — 카드 사이 3px 틈)</summary>
    private static readonly Point KnowledgeCardPitch = new(73, 106);

    /// <summary>카드 그림이 들어가는 위쪽 영역 높이</summary>
    private const int KnowledgePictureHeight = 66;

    /// <summary>상세창 크기</summary>
    private static readonly Point KnowledgeDetailSize = new(560, 520);

    /// <summary>상세창 그림 상자 크기</summary>
    private static readonly Point KnowledgeDetailPicture = new(120, 110);

    /// <summary>원소 기호로 쓰는 타입 (mana.type "on" 클러스터: 바람 4 · 비 5 · 번개 6 · 태양 7)</summary>
    private const string ManaTypeName = "mana";

    /// <summary>원소 → mana 타입의 켜진 기호 클러스터</summary>
    private static readonly IReadOnlyDictionary<Element, int> ManaOnCluster = new Dictionary<Element, int>
    {
        [Element.Wind] = 4,
        [Element.Rain] = 5,
        [Element.Thunder] = 6,
        [Element.Sun] = 7,
    };

    /// <summary>행 머리 칸 이름 (원본 화면 표기)</summary>
    private static readonly IReadOnlyDictionary<Element, string> KnowledgeRowNames = new Dictionary<Element, string>
    {
        [Element.Sun] = "SUN",
        [Element.Wind] = "WIND",
        [Element.Rain] = "RAIN",
        [Element.Thunder] = "THUN.",
    };

    /// <summary>카드 바탕색 (원본 돌 질감의 평균 색에 가깝게)</summary>
    private static readonly Color KnowledgeCardColor = new(139, 130, 110);

    /// <summary>카드 테두리 색</summary>
    private static readonly Color KnowledgeCardBorder = new(78, 70, 58);

    private bool _knowledgeOpen;

    /// <summary>상세창으로 연 카드 (없으면 격자)</summary>
    private KnowledgeCard? _knowledgeDetail;

    /// <summary>상세창 본문 스크롤 (픽셀)</summary>
    private int _knowledgeScroll;

    /// <summary>타입 목록 (카드 만들기)</summary>
    private TypeCatalog _knowledgeTypes = null!;

    /// <summary>맵(.fort) Technology 의 지식 플래그 타입</summary>
    private IReadOnlyList<string> _fortKnowledge = [];

    /// <summary>도움말 앵커 절 (상세창 본문, 없으면 null)</summary>
    private HelpTopics? _help;

    /// <summary>mana 타입 (원소 기호, 없으면 null)</summary>
    private TypeInfo? _manaType;

    /// <summary>지식 창이 지도 입력을 가로막는지.</summary>
    public bool KnowledgeOpen => _knowledgeOpen;

    /// <summary>지식 창 자료를 준비한다.</summary>
    /// <param name="fort">맵 (Technology 섹션)</param>
    /// <param name="catalog">타입 목록</param>
    /// <param name="help">도움말 (없으면 상세창 본문을 비운다)</param>
    private void InitializeKnowledge(FortFile fort, TypeCatalog catalog, HelpTopics? help)
    {
        _knowledgeTypes = catalog;
        _fortKnowledge = KnowledgeCatalog.KnownFromFort(fort);
        _help = help;
        _manaType = catalog.Find(ManaTypeName);
    }

    /// <summary>지식 창(격자)을 연다. 안내 창이 열려 있으면 그 위에 겹쳐 그려지며 닫으면 안내 창으로 돌아간다.</summary>
    private void OpenKnowledge()
    {
        _knowledgeOpen = true;
        _knowledgeDetail = null;
        QueueSound(OpenGumpSound);
    }

    /// <summary>
    /// 검증용(--knowledge [타입]): 지식 창을 연 채 시작한다. 타입을 주면 그 카드의 상세창을 연다.
    /// </summary>
    /// <param name="typeName">상세창을 열 타입 이름 (없으면 격자)</param>
    public void StartKnowledge(string? typeName)
    {
        OpenKnowledge();
        if (typeName != null)
        {
            _knowledgeDetail = KnowledgeRows().SelectMany(r => r.Cards)
                .FirstOrDefault(c => c.Type.Name.Equals(typeName, StringComparison.OrdinalIgnoreCase))
                ?? throw new ArgumentException($"지식 창에 없는 타입입니다: {typeName}");
        }
    }

    /// <summary>지금 보여 줄 원소 행 (맵 지식 + 플레이어가 배운 지식)</summary>
    private IReadOnlyList<KnowledgeRow> KnowledgeRows() =>
        KnowledgeCatalog.Rows(_knowledgeTypes, _fortKnowledge.Concat(_session.Player(TestPlayer).Deck.Knowledge));

    /// <summary>격자 전체 사각형 (화면 가운데)</summary>
    private static Rectangle KnowledgeGrid(IReadOnlyList<KnowledgeRow> rows, int width, int height)
    {
        int columns = 1 + Math.Max(1, rows.Max(r => r.Cards.Count));
        int gridWidth = KnowledgeHeaderWidth + (columns - 1) * KnowledgeCardPitch.X;
        int gridHeight = rows.Count * KnowledgeCardPitch.Y - (KnowledgeCardPitch.Y - KnowledgeCardSize.Y);
        return new Rectangle((width - gridWidth) / 2, (height - gridHeight) / 2, gridWidth, gridHeight);
    }

    /// <summary>행 머리 칸 사각형</summary>
    private static Rectangle KnowledgeHeaderCell(Rectangle grid, int row) =>
        new(grid.X, grid.Y + row * KnowledgeCardPitch.Y, KnowledgeHeaderWidth, KnowledgeCardSize.Y);

    /// <summary>카드 사각형</summary>
    private static Rectangle KnowledgeCardCell(Rectangle grid, int row, int column) =>
        new(grid.X + KnowledgeHeaderWidth + 3 + column * KnowledgeCardPitch.X, grid.Y + row * KnowledgeCardPitch.Y,
            KnowledgeCardSize.X, KnowledgeCardSize.Y);

    /// <summary>마우스가 가리키는 카드 (없으면 null)</summary>
    private static KnowledgeCard? KnowledgeCardAt(IReadOnlyList<KnowledgeRow> rows, Rectangle grid, Point mouse)
    {
        // 행·카드를 차례로 훑어 사각형 안에 든 카드를 찾는다
        for (int row = 0; row < rows.Count; row++)
        {
            for (int column = 0; column < rows[row].Cards.Count; column++)
            {
                if (KnowledgeCardCell(grid, row, column).Contains(mouse))
                {
                    return rows[row].Cards[column];
                }
            }
        }
        return null;
    }

    /// <summary>상세창 사각형</summary>
    private static Rectangle KnowledgeDetailPanel(int width, int height)
    {
        int w = Math.Min(KnowledgeDetailSize.X, width - 32);
        int h = Math.Min(KnowledgeDetailSize.Y, height - 32);
        return new Rectangle((width - w) / 2, (height - h) / 2, w, h);
    }

    /// <summary>상세창 Back·OK 버튼 (index 0 = Back, 1 = OK)</summary>
    private static Rectangle KnowledgeDetailButton(Rectangle panel, int index) =>
        new(panel.Center.X - 130 + index * 140, panel.Bottom - 46, 120, 34);

    /// <summary>상세창 본문 영역</summary>
    private static Rectangle KnowledgeDetailBody(Rectangle panel) =>
        new(panel.X + 20, panel.Y + 24 + KnowledgeDetailPicture.Y + 16, panel.Width - 40,
            panel.Height - (24 + KnowledgeDetailPicture.Y + 16) - 60);

    /// <summary>
    /// 지식 창이 열린 동안의 입력. 격자: 카드 클릭 → 상세창, 바깥 클릭·F6·Esc·Enter·Space → 닫기.
    /// 상세창: 휠·↑↓ 스크롤, Back·Esc·Backspace → 격자, OK·Enter·Space·F6 → 닫기.
    /// 세션 시간은 안내 창이 없을 때 계속 흐른다.
    /// </summary>
    private void UpdateKnowledge(KeyboardState keyboard, MouseState mouse, int width, int height, double seconds)
    {
        bool click = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton != ButtonState.Pressed;
        var point = new Point(mouse.X, mouse.Y);
        if (_knowledgeDetail != null)
        {
            Rectangle panel = KnowledgeDetailPanel(width, height);
            int wheel = mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue;
            if (wheel != 0)
            {
                _knowledgeScroll = Math.Max(0, _knowledgeScroll - Math.Sign(wheel) * TutorialLineHeight * 3);
            }
            if (Pressed(keyboard, Keys.Down)) _knowledgeScroll += TutorialLineHeight;
            if (Pressed(keyboard, Keys.Up)) _knowledgeScroll = Math.Max(0, _knowledgeScroll - TutorialLineHeight);
            if (Pressed(keyboard, Keys.Escape) || Pressed(keyboard, Keys.Back)
                || (click && KnowledgeDetailButton(panel, 0).Contains(point)))
            {
                _knowledgeDetail = null;
            }
            else if (Pressed(keyboard, Keys.Enter) || Pressed(keyboard, Keys.Space) || Pressed(keyboard, Keys.F6)
                || (click && KnowledgeDetailButton(panel, 1).Contains(point)))
            {
                _knowledgeDetail = null;
                _knowledgeOpen = false;
            }
        }
        else
        {
            IReadOnlyList<KnowledgeRow> rows = KnowledgeRows();
            Rectangle grid = KnowledgeGrid(rows, width, height);
            bool close = Pressed(keyboard, Keys.Escape) || Pressed(keyboard, Keys.Enter) || Pressed(keyboard, Keys.Space)
                || Pressed(keyboard, Keys.F6);
            if (click)
            {
                KnowledgeCard? card = KnowledgeCardAt(rows, grid, point);
                if (card != null)
                {
                    _knowledgeDetail = card;
                    _knowledgeScroll = 0;
                    QueueSound(OpenGumpSound);
                }
                else if (!grid.Contains(point))
                {
                    close = true;
                }
            }
            if (close)
            {
                _knowledgeOpen = false;
            }
        }
        if (!TutorialDialogOpen)
        {
            // 게임 코드가 직접 여는 창은 시계를 멈추지 않는다 (docs/gameplay/dialog-pause.md, 녹화 확인).
            UpdateSession(seconds, new KeyboardState(), mouse);
        }
    }

    /// <summary>지식 창(격자 또는 상세창)을 그린다.</summary>
    /// <param name="font">본문 글꼴</param>
    /// <param name="small">카드 이름·수치용 작은 글꼴</param>
    private void DrawKnowledge(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        if (!_knowledgeOpen)
        {
            return;
        }
        if (_knowledgeDetail != null)
        {
            DrawKnowledgeDetail(batch, font, small, _knowledgeDetail, width, height);
            return;
        }
        IReadOnlyList<KnowledgeRow> rows = KnowledgeRows();
        Rectangle grid = KnowledgeGrid(rows, width, height);
        KnowledgeCard? hovered = KnowledgeCardAt(rows, grid, new Point(_previousMouse.X, _previousMouse.Y));
        // 원소 행마다 머리 칸과 카드를 그린다
        for (int row = 0; row < rows.Count; row++)
        {
            Element element = rows[row].Element;
            Rectangle header = KnowledgeHeaderCell(grid, row);
            DrawKnowledgeBox(batch, header, false);
            string name = KnowledgeRowNames[element];
            Vector2 size = font.MeasureString(name);
            DrawShadowed(batch, font, name, new Vector2(header.Center.X - size.X / 2, header.Y + 10), Color.White);
            batch.Draw(_pixel, new Rectangle(header.X + 4, header.Y + 48, header.Width - 8, 1), KnowledgeCardBorder);
            DrawManaIcon(batch, element, new Point(header.Center.X, header.Y + 74));
            // 행 안의 카드를 왼쪽부터 그린다
            for (int column = 0; column < rows[row].Cards.Count; column++)
            {
                KnowledgeCard card = rows[row].Cards[column];
                Rectangle cell = KnowledgeCardCell(grid, row, column);
                DrawKnowledgeBox(batch, cell, card == hovered);
                DrawTypePicture(batch, card.Type, new Rectangle(cell.X + 3, cell.Y + 3, cell.Width - 6, KnowledgePictureHeight));
                DrawCardTitle(batch, small, card.Title, cell);
            }
        }
    }

    /// <summary>카드·머리 칸 상자 (마우스가 올라가면 어둡게)</summary>
    private void DrawKnowledgeBox(SpriteBatch batch, Rectangle rect, bool hovered)
    {
        batch.Draw(_pixel, rect, hovered ? Color.Lerp(KnowledgeCardColor, Color.Black, 0.35f) : KnowledgeCardColor);
        Outline(batch, rect, KnowledgeCardBorder);
        batch.Draw(_pixel, new Rectangle(rect.X + 1, rect.Y + 1, rect.Width - 2, 1), Color.White * 0.25f);
    }

    /// <summary>카드 아래쪽에 이름을 두 줄까지 왼쪽 정렬로 쓴다 (원본처럼 단어 단위 줄바꿈)</summary>
    private void DrawCardTitle(SpriteBatch batch, SpriteFontBase small, string title, Rectangle cell)
    {
        var lines = new List<string>();
        string line = "";
        // 단어를 이어 붙이다 카드 폭을 넘으면 줄을 바꾼다
        foreach (string word in title.Split(' ', StringSplitOptions.RemoveEmptyEntries))
        {
            string next = line.Length == 0 ? word : line + " " + word;
            if (line.Length > 0 && small.MeasureString(next).X > cell.Width - 8)
            {
                lines.Add(line);
                next = word;
            }
            line = next;
        }
        lines.Add(line);
        float lineHeight = small.MeasureString("Ag").Y;
        float y = cell.Bottom - 4 - lineHeight * Math.Min(lines.Count, 2);
        // 두 줄까지만 그린다
        foreach (string text in lines.Take(2))
        {
            DrawShadowed(batch, small, text, new Vector2(cell.X + 5, y), Color.White);
            y += lineHeight;
        }
    }

    /// <summary>글자를 1픽셀 그림자와 함께 쓴다 (돌 바탕에서 읽히도록)</summary>
    private static void DrawShadowed(SpriteBatch batch, SpriteFontBase font, string text, Vector2 position, Color color)
    {
        batch.DrawString(font, text, position + Vector2.One, Color.Black * 0.8f);
        batch.DrawString(font, text, position, color);
    }

    /// <summary>
    /// 타입 그림을 상자 안 가운데에 맞춰 그린다 (크면 줄인다). 원본 격자 카드는 유닛 모습(기본 프레임),
    /// 상세창은 도움말 삽화(help 프레임)를 쓴다 (2026-09-30 녹화 화면).
    /// </summary>
    /// <param name="help">도움말 삽화를 쓸지 (없으면 기본 프레임)</param>
    private void DrawTypePicture(SpriteBatch batch, TypeInfo type, Rectangle box, bool help = false)
    {
        TypeFrameTable frames = type.Definition.Frames;
        int cluster = help && frames.HelpFrame >= 0 ? frames.HelpFrame : frames.DefaultFrame;
        if (GetTexture(type.LoadIndex, MapSpriteFrames.BodyFrame(type.Definition, cluster)) is not { } sprite)
        {
            return;
        }
        Texture2D texture = sprite.Texture;
        float scale = Math.Min(1f, Math.Min(box.Width / (float)texture.Width, box.Height / (float)texture.Height));
        var size = new Vector2(texture.Width, texture.Height) * scale;
        var position = new Vector2(box.Center.X - size.X / 2, box.Center.Y - size.Y / 2);
        batch.Draw(texture, position, null, Color.White, 0f, Vector2.Zero, scale, SpriteEffects.None, 0f);
    }

    /// <summary>원소 기호(mana 타입의 켜진 그림)를 중심점에 그린다</summary>
    private void DrawManaIcon(SpriteBatch batch, Element element, Point center)
    {
        if (_manaType == null)
        {
            return;
        }
        if (GetTexture(_manaType.LoadIndex, MapSpriteFrames.BodyFrame(_manaType.Definition, ManaOnCluster[element])) is { } sprite)
        {
            Texture2D texture = sprite.Texture;
            batch.Draw(texture, new Vector2(center.X - texture.Width / 2f, center.Y - texture.Height / 2f), Color.White);
        }
    }

    /// <summary>상세창: 그림, 이름·수치, 도움말 본문(스크롤), Back·OK</summary>
    private void DrawKnowledgeDetail(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, KnowledgeCard card,
        int width, int height)
    {
        Rectangle panel = KnowledgeDetailPanel(width, height);
        batch.Draw(_pixel, panel, new Color(96, 92, 84));
        Outline(batch, panel, new Color(190, 170, 120));
        var picture = new Rectangle(panel.X + 20, panel.Y + 20, KnowledgeDetailPicture.X, KnowledgeDetailPicture.Y);
        batch.Draw(_pixel, picture, new Color(40, 52, 40));
        Outline(batch, picture, Color.Black);
        DrawTypePicture(batch, card.Type, picture, help: true);

        TypeDefinition definition = card.Type.Definition;
        float x = picture.Right + 16;
        float y = panel.Y + 18;
        DrawShadowed(batch, font, card.Title, new Vector2(x, y), Color.White);
        y += font.MeasureString("Ag").Y + 2;
        float line = small.MeasureString("Ag").Y + 1;
        string range = definition.GetInt("range") is int r and > 0 ? r.ToString(System.Globalization.CultureInfo.InvariantCulture) : "n/a";
        string damage = definition.GetString("class") == "Shooter" ? "?" : "n/a";
        string[] stats =
        [
            $"Alignment: {card.Element}",
            $"Class: {definition.GetString("class") ?? "?"}",
            $"Hits: {definition.GetInt("maxHitPoints")?.ToString(System.Globalization.CultureInfo.InvariantCulture) ?? "n/a"}",
            $"Range: {range}",
            $"Damage: {damage}",
            $"Cost in Storm Power: {definition.GetInt("cost") ?? 0}",
        ];
        // 수치 줄을 차례로 쓴다
        foreach (string stat in stats)
        {
            DrawShadowed(batch, small, stat, new Vector2(x, y), Color.Wheat);
            y += line;
        }
        DrawShadowed(batch, small, "Energy to Build:", new Vector2(x, y), Color.Wheat);
        float iconX = x + small.MeasureString("Energy to Build: ").X;
        // 필요한 에너지 글자마다 원소 기호를 하나씩 그린다
        foreach (char letter in EnergyRequirement.ForType(definition).Letters)
        {
            if (Elements.FromLetter(letter) is Element element)
            {
                DrawManaIcon(batch, element, new Point((int)iconX + 8, (int)(y + line / 2)));
                iconX += 18;
            }
        }

        Rectangle body = KnowledgeDetailBody(panel);
        batch.Draw(_pixel, body, new Color(24, 22, 20));
        string? html = _help?.ForType(card.Type.Name);
        IReadOnlyList<TutorialTextRun> runs = html == null ? [new TutorialTextRun("(도움말 본문 없음)", TutorialTextStyle.Body, TutorialTextBreak.None)]
            : HelpTopics.ToRuns(html);
        var inner = new Rectangle(body.X + 8, body.Y + 6, body.Width - 20, body.Height - 12);
        IReadOnlyList<TutorialVisualLine> lines = LayoutTutorialText(runs, font, inner.Width);
        int contentHeight = lines.Sum(l => l.Height);
        _knowledgeScroll = Math.Clamp(_knowledgeScroll, 0, Math.Max(0, contentHeight - inner.Height));
        int lineY = inner.Y - _knowledgeScroll;
        // 본문 영역 안에 완전히 들어오는 줄만 그린다
        foreach (TutorialVisualLine visual in lines)
        {
            if (visual.Spans.Count > 0 && lineY >= inner.Y && lineY + visual.Height <= inner.Bottom)
            {
                float spanX = inner.X;
                // 한 줄의 강조 구간을 이어 그린다
                foreach (TutorialVisualSpan span in visual.Spans)
                {
                    batch.DrawString(font, span.Text, new Vector2(spanX, lineY), TutorialColor(span.Style));
                    spanX += font.MeasureString(span.Text).X;
                }
            }
            lineY += visual.Height;
        }
        if (contentHeight > inner.Height)
        {
            int barHeight = Math.Max(16, inner.Height * inner.Height / contentHeight);
            int barY = inner.Y + _knowledgeScroll * (inner.Height - barHeight) / (contentHeight - inner.Height);
            batch.Draw(_pixel, new Rectangle(body.Right - 6, barY, 3, barHeight), Color.Gold);
        }
        string[] labels = ["Back", "OK"];
        // Back·OK 버튼을 그린다
        for (int index = 0; index < labels.Length; index++)
        {
            Rectangle button = KnowledgeDetailButton(panel, index);
            batch.Draw(_pixel, button, new Color(70, 64, 54));
            Outline(batch, button, new Color(190, 170, 120));
            Vector2 size = font.MeasureString(labels[index]);
            batch.DrawString(font, labels[index], new Vector2(button.Center.X - size.X / 2, button.Y + 5), Color.White);
        }
    }
}
