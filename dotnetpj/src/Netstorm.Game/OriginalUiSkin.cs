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

    /// <summary>
    /// 원본 메뉴 목록의 한 행 높이. 펼침 목록·하위 목록·우클릭 메뉴의 입력 영역에도 쓴다.
    /// 2026-10-10: 18 → 17. 원본 캡처의 옵션·도움말·미션 메뉴·우클릭 메뉴 행 글자가 모두 17픽셀 간격이다.
    /// </summary>
    public const int RowHeight = 17;
    /// <summary>목록의 그룹 사이 구분 구간 높이 (원본 16). 구간 위에서 <see cref="MenuSeparatorLine"/> 만큼 내려간 곳에 두 줄 선을 긋는다.</summary>
    public const int MenuSeparatorHeight = 16;
    /// <summary>구분 구간의 위에서 어두운 선까지의 거리 (바로 아래 줄이 밝은 선).</summary>
    public const int MenuSeparatorLine = 8;
    /// <summary>목록 왼쪽 끝에서 행 글자까지의 거리.</summary>
    public const int MenuTextInset = 15;
    /// <summary>목록 폭이 가장 긴 행의 글자 폭보다 넓은 정도 (왼쪽 15 + 오른쪽 16).</summary>
    public const int MenuWidthPadding = 31;
    /// <summary>행 글자 끝에서 하위 목록 화살표(&gt;)까지의 거리.</summary>
    public const int MenuArrowGap = 4;
    /// <summary>목록 왼쪽 끝에서 켜짐 표시(파란 원) 중심까지의 거리.</summary>
    public const int MenuPipInset = 8;
    /// <summary>창·목록 테두리의 두께 (원본은 밝은 선 2줄·어두운 선 2줄).</summary>
    public const int FrameThickness = 2;
    /// <summary>원본의 낮은 텍스트 버튼이 그려지는 높이.</summary>
    public const int ButtonHeight = 19;
    /// <summary>
    /// 버튼 판정이 그려진 높이보다 더 내려가는 픽셀 수. 원본 메인 메뉴 Credits 버튼은 세로 334~352(19px)로 그려지지만
    /// 334~353(20px)까지 눌린다 (2026-10-03 경계 스캔). 가로는 그려진 폭(75px)과 같다.
    /// </summary>
    public const int ButtonHitExtraHeight = 1;
    /// <summary>돌 버튼 테두리의 밝은 가장자리 색.</summary>
    private static readonly Color LightEdge = new(191, 178, 139);
    /// <summary>돌 버튼 테두리의 어두운 가장자리 색.</summary>
    private static readonly Color DarkEdge = new(49, 44, 36);
    /// <summary>
    /// 원본 UI 의 노란 글자색 (단축키·정보 값·강조 이름). 게임 팔레트 197번 (242, 228, 152).
    /// 2026-10-10: 원본 캡처의 "Shift-F9"·"You"·"Sun"·미션 제목 글자 색(중앙값 (242, 222, 153))에 가장 가까운 팔레트 색으로 정했다.
    /// 이전 값 (240, 214, 90) 은 밝게 찍힌 녹화에서 어림한 것이었다.
    /// </summary>
    public static readonly Color ValueColor = new(242, 228, 152);

    /// <summary>
    /// 원본 안내 창의 기울임(&lt;i&gt;) 글자색. 게임 팔레트 96번 (233, 214, 186) — 원본 브리핑 인용문 글자 색(중앙값 (229, 214, 187))에 가장 가깝다.
    /// 2026-10-10: 이전에는 Wheat (245, 222, 179) 를 썼다.
    /// </summary>
    public static readonly Color EmphasisColor = new(233, 214, 186);

    /// <summary>회색 돌 창(안내·확인·우클릭 창) 테두리의 밝은 선 색. 원본 캡처의 테두리 행 평균이다 (캡처가 손실 압축이라 ±10 정도의 오차가 있다).</summary>
    private static readonly Color PanelLight = new(155, 147, 138);
    /// <summary>메뉴 돌 바탕(펼침 목록·메뉴 막대·안쪽 판) 테두리와 구분선의 밝은 선 색. 원본 캡처의 테두리 행 평균이다.</summary>
    private static readonly Color MenuLight = new(199, 185, 165);
    /// <summary>테두리·구분선의 어두운 선. 바탕 질감을 약 0.61배로 어둡게 한다 (원본 캡처: 창 103 → 65, 메뉴 131 → 80).</summary>
    private static readonly Color FrameDark = Color.Black * 0.39f;
    private readonly Texture2D _pixel;
    private readonly Dictionary<string, Texture2D> _frames = new(StringComparer.OrdinalIgnoreCase);
    /// <summary>D2Coding(한국어 화면)으로 그릴 때의 본문 줄 높이.</summary>
    private const int FallbackLineHeight = 16;
    /// <summary>D2Coding(한국어 화면)으로 그릴 때 문단 사이에 더하는 간격.</summary>
    private const int FallbackParagraphGap = 12;

    /// <summary>본문·버튼·목록 행 글꼴. 영어 화면은 원본 Arial 14픽셀 굵은 글꼴(<see cref="OriginalFonts"/>), 한국어 화면은 D2Coding.</summary>
    public SpriteFontBase Body { get; }
    /// <summary>
    /// 도움말 본문의 보통 굵기 글꼴. 영어 화면은 원본 Arial 14픽셀 보통 글꼴, 한국어 화면은 <see cref="Body"/> 와 같은 D2Coding.
    /// 2026-10-10 추가: 원본 도움말은 본문이 보통 굵기이고 굵은 글씨(&lt;b&gt;)만 <see cref="Body"/> 글꼴이다.
    /// </summary>
    public SpriteFontBase Plain { get; }
    /// <summary>대화상자 제목 글꼴. 영어 화면은 원본 Arial 20픽셀 굵은 글꼴, 한국어 화면은 D2Coding.</summary>
    public SpriteFontBase Title { get; }
    /// <summary>카드 이름·보조 정보 글꼴. 영어 화면은 원본 Arial 12픽셀 글꼴, 한국어 화면은 D2Coding.</summary>
    public SpriteFontBase Small { get; }

    /// <summary>본문을 원본 비트맵 글꼴로 그리는지 (언어가 영어이고 글꼴 캐시를 읽었을 때).</summary>
    public bool UsesOriginalFonts => IsOriginal(Body);

    /// <summary>
    /// 본문 한 줄의 높이. 원본 글꼴이면 글꼴 높이 14(원본 브리핑의 줄 간격), D2Coding 이면 16.
    /// 줄을 감아 그리는 화면(안내 창·캠페인 설명·팁)이 줄 위치를 정할 때 쓴다.
    /// </summary>
    public int LineHeight => UsesOriginalFonts ? Body.LineHeight : FallbackLineHeight;

    /// <summary>문단 사이에 더하는 간격. 원본 글꼴이면 빈 줄 하나(14), D2Coding 이면 12.</summary>
    public int ParagraphGap => UsesOriginalFonts ? Body.LineHeight : FallbackParagraphGap;

    /// <summary>글꼴이 원본 비트맵 글꼴로 만든 것인지.</summary>
    /// <param name="font">확인할 글꼴</param>
    public static bool IsOriginal(SpriteFontBase font) => font.Tag is OriginalFontInfo;

    /// <summary>원본 대화상자에서 버튼 폭이 가장 긴 문구의 폭보다 넓은 정도.</summary>
    private const int EqualButtonPadding = 11;

    /// <summary>원본 대화상자 버튼의 최소 폭 (짧은 문구 하나뿐일 때. 원본 근거가 없는 클론 값이다).</summary>
    private const int EqualButtonMinimum = 36;

    /// <summary>
    /// 원본 대화상자의 버튼 폭: 한 창의 버튼은 모두 같은 폭이고 그 값은 가장 긴 문구의 폭 + 11 이다.
    /// 원본 글꼴(영어 화면)로 그리는 창이 버튼을 나란히 놓을 때 쓴다. 버튼 사이 간격은 16 이다.
    /// </summary>
    /// <param name="font">버튼 글꼴</param>
    /// <param name="labels">그 창의 버튼 문구 전부</param>
    public static int EqualButtonWidth(SpriteFontBase font, IEnumerable<string> labels) =>
        Math.Max(EqualButtonMinimum, labels.Select(label => (int)font.MeasureString(label).X).DefaultIfEmpty(0).Max() + EqualButtonPadding);

    /// <summary>
    /// 타입의 클러스터 이름으로 원본 UI 프레임을 찾아 팔레트를 적용하고 UI 글꼴을 연결한다.
    /// 글꼴은 호출한 쪽이 언어에 맞게 고른다: 영어는 <see cref="OriginalFonts"/> 의 원본 글꼴, 한국어는 D2Coding.
    /// </summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="shapes">셰이프 데이터베이스</param>
    /// <param name="palette">UI 프레임에 적용할 팔레트</param>
    /// <param name="definition">fortGump 타입 정의</param>
    /// <param name="body">본문·버튼 글꼴</param>
    /// <param name="title">제목 글꼴</param>
    /// <param name="small">작은 글꼴</param>
    /// <param name="plain">도움말 본문의 보통 굵기 글꼴 (없으면 본문 글꼴)</param>
    public OriginalUiSkin(GraphicsDevice device, ShapeDatabase shapes, Palette palette, TypeDefinition definition,
        SpriteFontBase body, SpriteFontBase title, SpriteFontBase small, SpriteFontBase? plain = null)
    {
        Body = body; Title = title; Small = small; Plain = plain ?? body;
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

    /// <summary>
    /// 영역 안쪽 가장자리에 2픽셀 테두리를 그린다. 도드라진 테두리는 위·왼쪽이 밝고 아래·오른쪽이 어둡다. 들어간 테두리(<paramref name="inset"/>)는 반대다.
    /// 밝은 선은 바탕 종류의 색으로, 어두운 선은 바탕을 어둡게 겹쳐 그린다. 창·목록 바탕을 채운 뒤에 부른다.
    /// </summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="area">테두리를 두를 영역 (테두리는 영역 안쪽에 그린다)</param>
    /// <param name="menu">메뉴 돌 바탕(밝은 쪽) 위인지. 아니면 회색 돌 창 위</param>
    /// <param name="inset">들어간 모양(위·왼쪽이 어두움)인지 — 우클릭 창의 안쪽 판</param>
    /// <remarks>
    /// 2026-10-10 추가: 원본 캡처에서 안내 창·옵션 목록·미션 메뉴·우클릭 창의 테두리가 모두 2줄이었다
    /// (docs/dotnet-reconstruction-20261010.md 8절). 이전에는 1줄 단색(<see cref="Bevel"/>)이었다.
    /// </remarks>
    public void Frame(SpriteBatch batch, Rectangle area, bool menu = false, bool inset = false)
    {
        Color light = menu ? MenuLight : PanelLight;
        Color topLeft = inset ? FrameDark : light;
        Color bottomRight = inset ? light : FrameDark;
        const int t = FrameThickness;
        batch.Draw(_pixel, new Rectangle(area.X, area.Y, area.Width, t), topLeft);
        batch.Draw(_pixel, new Rectangle(area.X, area.Y + t, t, area.Height - t), topLeft);
        batch.Draw(_pixel, new Rectangle(area.X + t, area.Bottom - t, area.Width - t, t), bottomRight);
        batch.Draw(_pixel, new Rectangle(area.Right - t, area.Y + t, t, area.Height - t * 2), bottomRight);
    }

    /// <summary>안내·선택·결과 창에 원본 회색 돌 바탕과 2픽셀 테두리, 네 금색 모서리를 그린다.</summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="area">창 영역</param>
    public void Panel(SpriteBatch batch, Rectangle area)
    {
        Tile(batch, area);
        Frame(batch, area);
        DrawFrame(batch, "I00", new Point(area.X, area.Y));
        DrawFrame(batch, "I01", new Point(area.Right - FrameWidth("I01"), area.Y));
        DrawFrame(batch, "I02", new Point(area.Right - FrameWidth("I02"), area.Bottom - FrameHeight("I02")));
        DrawFrame(batch, "I03", new Point(area.X, area.Bottom - FrameHeight("I03")));
    }

    /// <summary>메뉴 바탕을 채우고 2픽셀 테두리를 붙인다. 행은 영역의 맨 위부터 놓이며 테두리가 첫 행·끝 행의 가장자리 2픽셀과 겹친다.</summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="area">목록 영역</param>
    public void Menu(SpriteBatch batch, Rectangle area)
    { Tile(batch, area, menu: true); Frame(batch, area, menu: true); }

    /// <summary>
    /// 목록의 그룹 사이 구분 구간에 어둡고 밝은 두 줄 선을 긋는다. <paramref name="top"/> 은 구분 구간(높이
    /// <see cref="MenuSeparatorHeight"/>)의 위쪽이고 선은 거기서 <see cref="MenuSeparatorLine"/> 내려간 곳이다.
    /// 2026-10-10: 선의 y 를 직접 받던 것을 구간 위쪽을 받도록 바꿨다 (원본 구간 16, 선 +8·+9).
    /// </summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="x">선의 왼쪽 끝</param>
    /// <param name="top">구분 구간의 위쪽</param>
    /// <param name="width">선의 길이</param>
    public void Separator(SpriteBatch batch, int x, int top, int width)
    {
        batch.Draw(_pixel, new Rectangle(x, top + MenuSeparatorLine, width, 1), FrameDark);
        batch.Draw(_pixel, new Rectangle(x, top + MenuSeparatorLine + 1, width, 1), MenuLight);
    }

    /// <summary>
    /// 목록 행 글자의 폭: 글자 폭에, 하위 목록 화살표가 있으면 화살표 자리(간격 4 + &gt; 폭)를 더한다.
    /// 목록 폭은 이 값의 최댓값 + <see cref="MenuWidthPadding"/> 이다 (<see cref="MenuWidth"/>).
    /// </summary>
    /// <param name="label">행 문구 (화살표 제외, 단축키 표기 포함)</param>
    /// <param name="arrow">하위 목록 화살표가 붙는지</param>
    public int MenuLabelWidth(string label, bool arrow = false) =>
        (int)Math.Ceiling(Body.MeasureString(label).X) + (arrow ? MenuArrowGap + (int)Math.Ceiling(Body.MeasureString(">").X) : 0);

    /// <summary>원본 목록의 폭: 가장 긴 행의 글자 폭 + 31. 예: 옵션 목록 166 = "Edge Scroll in Fullscreen" 135 + 31.</summary>
    /// <param name="labelWidths">각 행의 <see cref="MenuLabelWidth"/></param>
    public static int MenuWidth(IEnumerable<int> labelWidths) => labelWidths.DefaultIfEmpty(0).Max() + MenuWidthPadding;

    /// <summary>
    /// 목록 행의 글자를 놓을 위치. 원본 글꼴은 행 아래에서 2픽셀 띄운 자리(높이 17 인 행이면 위에서 +1, 아이콘이 있는 19 행이면 +3),
    /// D2Coding 은 행 가운데다. 가로는 목록 왼쪽 끝 + <see cref="MenuTextInset"/>.
    /// </summary>
    /// <param name="menuLeft">목록(또는 안쪽 판)의 왼쪽 끝</param>
    /// <param name="row">행 영역</param>
    /// <param name="label">행 문구 (D2Coding 의 세로 가운데 정렬에 쓴다)</param>
    public Vector2 MenuTextPosition(int menuLeft, Rectangle row, string label) => UsesOriginalFonts
        ? new Vector2(menuLeft + MenuTextInset, row.Bottom - 2 - Body.LineHeight)
        : new Vector2(menuLeft + MenuTextInset, row.Center.Y - Body.MeasureString(label).Y / 2);

    /// <summary>
    /// 목록 행 한 줄을 그린다: 문구, 노란 단축키(" - 키"), 하위 목록 화살표. 켜짐 표시와 커서 강조는 호출한 쪽이 그린다.
    /// </summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="menuLeft">목록(또는 안쪽 판)의 왼쪽 끝</param>
    /// <param name="row">행 영역</param>
    /// <param name="label">행 문구</param>
    /// <param name="color">문구 색</param>
    /// <param name="key">단축키 표기 (없으면 빈 문자열). 문구 뒤에 " - " 를 두고 <paramref name="keyColor"/> 로 쓴다</param>
    /// <param name="keyColor">단축키 색</param>
    /// <param name="arrow">하위 목록 화살표를 붙일지</param>
    /// <returns>그린 글자의 오른쪽 끝 x (뒤에 아이콘을 붙일 때 쓴다)</returns>
    public float MenuRow(SpriteBatch batch, int menuLeft, Rectangle row, string label, Color color,
        string key = "", Color? keyColor = null, bool arrow = false)
    {
        string head = key.Length > 0 ? label + " - " : label;
        Vector2 position = MenuTextPosition(menuLeft, row, head + key);
        Text(batch, Body, head, position, color);
        float x = position.X + Body.MeasureString(head).X;
        if (key.Length > 0)
        {
            Text(batch, Body, key, new Vector2(x, position.Y), keyColor ?? color);
            x += Body.MeasureString(key).X;
        }
        if (arrow) Text(batch, Body, ">", new Vector2(x + MenuArrowGap, position.Y), color);
        return x;
    }

    /// <summary>
    /// 글자를 픽셀 좌표에 맞춰 오른쪽 아래 1픽셀의 검은 그림자와 함께 쓴다. 위치는 줄의 왼쪽 위다.
    /// 원본 글꼴은 좌표를 정수로 내리고 그림자를 불투명한 검정으로 그린다 (원본 004a3430 의 그림자: 색 0, 이동 (1,1)).
    /// </summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="font">글꼴</param>
    /// <param name="text">문자열</param>
    /// <param name="position">줄의 왼쪽 위</param>
    /// <param name="color">글자 색 (없으면 흰색)</param>
    public static void Text(SpriteBatch batch, SpriteFontBase font, string text, Vector2 position, Color? color = null)
    {
        bool original = IsOriginal(font);
        position = original ? new Vector2(MathF.Floor(position.X), MathF.Floor(position.Y))
            : new Vector2(MathF.Round(position.X), MathF.Round(position.Y));
        batch.DrawString(font, text, position + Vector2.One, original ? Color.Black : Color.Black * 0.9f);
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
        int shift = pressed ? 1 : 0;
        Color color = enabled ? Color.White : new Color(145, 141, 130);
        if (IsOriginal(font))
        {
            // 원본 돌 버튼의 글자 위치: x + 1 + (폭 − 글자 폭) ÷ 2, y + (버튼 높이 − 글꼴 높이) ÷ 2 (정수 나눗셈).
            // 가로의 +1 은 원본 메인 메뉴 캡처(screenShots/mainMenu.png)의 Campaign·Help·Options·Quit 글자 위치를 대어 확인했다
            // (글자 폭이 짝수·홀수인 버튼 모두 1픽셀 오른쪽). cpppj UberGump 의 식에는 이 +1 이 없다.
            Text(batch, font, label, new Vector2(area.X + 1 + (area.Width - (int)size.X) / 2 + shift,
                area.Y + (area.Height - font.LineHeight) / 2 + shift), color);
            return;
        }
        Text(batch, font, label, new Vector2(area.Center.X - size.X / 2 + shift, area.Center.Y - size.Y / 2 + shift), color);
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
