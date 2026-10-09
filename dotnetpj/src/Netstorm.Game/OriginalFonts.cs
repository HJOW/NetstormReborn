using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>
/// 원본 글꼴로 만든 화면 글꼴에 붙이는 정보 (<see cref="SpriteFontBase.Tag"/>).
/// 그 글꼴에 있는 글자의 집합과, 없는 글자가 섞인 문자열을 대신 그릴 글꼴을 가진다.
/// </summary>
/// <param name="Characters">원본 글꼴에 있는 글자 (Windows-1252 코드 32~255)</param>
/// <param name="Fallback">원본 글꼴에 없는 글자(한글 등)가 있는 문자열을 그릴 글꼴 (D2Coding)</param>
internal sealed record OriginalFontInfo(HashSet<char> Characters, SpriteFontBase Fallback)
{
    /// <summary>문자열의 모든 글자를 원본 글꼴로 그릴 수 있는지. 줄바꿈 문자는 글자로 치지 않는다.</summary>
    /// <param name="text">그릴 문자열</param>
    public bool Covers(string text)
    {
        // 글자마다 원본 글꼴에 있는지 본다.
        foreach (char c in text)
        {
            if (c != '\n' && c != '\r' && !Characters.Contains(c)) return false;
        }
        return true;
    }
}

/// <summary>
/// 원본 비트맵 글꼴 캐시(.chfnt)로 만든 UI 글꼴 네 가지: 본문·버튼(Arial 14픽셀 굵게), 도움말 본문(Arial 14픽셀 보통),
/// 제목(Arial 20픽셀 굵게), 작은 글자(Arial 12픽셀).
/// **언어가 영어일 때** UI 가 이 글꼴을 쓴다 (2026-10-10 사용자 결정). 한국어일 때는 만들지 않고 D2Coding 을 쓴다.
/// 사용법: <c>OriginalFonts? fonts = OriginalFonts.TryLoad(device, resources, bodyFallback, titleFallback, smallFallback);</c>
/// 캐시 파일이 하나라도 없거나 손상돼 있으면 null 을 돌려주며 호출한 쪽은 D2Coding 으로 계속한다.
/// 글자를 그리고 재는 방법은 다른 글꼴과 같다 (<c>batch.DrawString</c>·<c>MeasureString</c>). 폭은 원본의 글자 전진 폭의 합이고
/// 높이는 줄 높이로 일정하다. 원본 글꼴에 없는 글자가 섞인 문자열은 <see cref="UiTextExtensions.DrawString"/> 이 대신 글꼴로 그린다.
/// </summary>
/// <remarks>
/// 슬롯 대응의 근거: 원본 화면 캡처(screenShots/The War Begins! - Briefing.png)의 본문 줄이 슬롯 0 캐시
/// (<c>!Arial.normal.14.700</c>)의 합성과, 제목이 슬롯 3 캐시(<c>!Arial.normal.20.700</c>)의 합성과 픽셀 단위로 일치했다
/// (docs/dotnet-reconstruction-20261010.md). 도움말 본문은 같은 방법으로 슬롯 5 캐시(<c>!Arial.normal.14.0</c>)와 일치했다
/// (screenShots/help - NetStorm Instructions.png). 작은 글자(슬롯 6)는 대조한 근거가 없는 대응이다.
/// 2026-10-10 추가 (LEFT_JOBS.dotnetpj.md 5-3).
/// </remarks>
internal sealed class OriginalFonts : IDisposable
{
    /// <summary>글꼴마다 만든 아틀라스 텍스처 (해제용).</summary>
    private readonly List<Texture2D> _textures = [];

    /// <summary>본문·버튼·목록 행 글꼴 (원본 슬롯 0).</summary>
    public SpriteFontBase Body { get; }

    /// <summary>도움말 본문의 보통 굵기 글꼴 (원본 슬롯 5).</summary>
    public SpriteFontBase Plain { get; }

    /// <summary>제목 글꼴 (원본 슬롯 3).</summary>
    public SpriteFontBase Title { get; }

    /// <summary>작은 글꼴 (원본 슬롯 6).</summary>
    public SpriteFontBase Small { get; }

    /// <summary>본문 한 줄의 높이 (원본 글꼴 높이 14).</summary>
    public int BodyLineHeight => Body.LineHeight;

    /// <summary>세 글꼴을 만든다. 캐시를 읽지 못하면 예외가 나므로 <see cref="TryLoad"/> 로 부른다.</summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="resources">캐시 파일을 읽을 게임 자료</param>
    /// <param name="bodyFallback">본문 글꼴에 없는 글자를 그릴 글꼴</param>
    /// <param name="titleFallback">제목 글꼴에 없는 글자를 그릴 글꼴</param>
    /// <param name="smallFallback">작은 글꼴에 없는 글자를 그릴 글꼴</param>
    private OriginalFonts(GraphicsDevice device, GameResources resources,
        SpriteFontBase bodyFallback, SpriteFontBase titleFallback, SpriteFontBase smallFallback)
    {
        Body = Create(device, resources, BitmapFont.BodySlot, bodyFallback);
        Plain = Create(device, resources, BitmapFont.PlainSlot, bodyFallback);
        Title = Create(device, resources, BitmapFont.TitleSlot, titleFallback);
        Small = Create(device, resources, BitmapFont.SmallSlot, smallFallback);
    }

    /// <summary>
    /// 원본 글꼴 세 가지를 만든다. 캐시가 없거나 손상돼 있으면 만든 텍스처를 정리하고 null 을 돌려준다.
    /// </summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="resources">캐시 파일을 읽을 게임 자료</param>
    /// <param name="bodyFallback">본문 글꼴에 없는 글자를 그릴 글꼴</param>
    /// <param name="titleFallback">제목 글꼴에 없는 글자를 그릴 글꼴</param>
    /// <param name="smallFallback">작은 글꼴에 없는 글자를 그릴 글꼴</param>
    public static OriginalFonts? TryLoad(GraphicsDevice device, GameResources resources,
        SpriteFontBase bodyFallback, SpriteFontBase titleFallback, SpriteFontBase smallFallback)
    {
        try
        {
            return new OriginalFonts(device, resources, bodyFallback, titleFallback, smallFallback);
        }
        catch (Exception error) when (error is IOException or InvalidDataException)
        {
            return null;
        }
    }

    /// <summary>슬롯 하나의 캐시를 읽어 아틀라스 텍스처와 글자 표를 가진 화면 글꼴로 만든다.</summary>
    /// <param name="device">그래픽 장치</param>
    /// <param name="resources">게임 자료</param>
    /// <param name="slot">원본 글꼴 슬롯</param>
    /// <param name="fallback">이 글꼴에 없는 글자를 그릴 글꼴</param>
    private SpriteFontBase Create(GraphicsDevice device, GameResources resources, int slot, SpriteFontBase fallback)
    {
        string path = BitmapFont.CachePath(slot);
        byte[] bytes = resources.Files.TryReadAllBytes(path) ?? throw new FileNotFoundException($"원본 글꼴 캐시가 없습니다: {path}");
        var source = new BitmapFont(bytes);
        var atlas = new BitmapFontAtlas(source);
        var pixels = new Color[atlas.Width * atlas.Height];
        // 글자 픽셀은 흰색, 나머지는 투명으로 둔다. 색은 그릴 때 곱해진다 (원본은 팔레트 100번만 요청 색으로 바꾼다).
        for (int i = 0; i < pixels.Length; i++)
        {
            if (atlas.Coverage[i]) pixels[i] = Color.White;
        }
        var texture = new Texture2D(device, atlas.Width, atlas.Height);
        texture.SetData(pixels);
        _textures.Add(texture);
        var font = new StaticSpriteFont(source.Height, atlas.LineHeight, new Point(atlas.Width, atlas.Height));
        var characters = new HashSet<char>();
        // 글자 칸마다 놓는 점 기준(오프셋 0)의 글리프를 등록한다.
        foreach (BitmapFontCell cell in atlas.Cells)
        {
            // Windows-1252 에 정의되지 않은 코드(제어 문자 자리)는 다른 글자와 겹칠 수 있어 건너뛴다.
            if (!characters.Add(cell.Character)) continue;
            font.Glyphs[cell.Character] = new FontGlyph
            {
                Id = cell.Character,
                Codepoint = cell.Character,
                XAdvance = cell.Advance,
                Texture = texture,
                TextureOffset = new Point(cell.X, cell.Y),
                Size = new Point(cell.Width, cell.Height),
                RenderOffset = Point.Zero,
            };
        }
        font.Tag = new OriginalFontInfo(characters, fallback);
        return font;
    }

    /// <summary>아틀라스 텍스처를 해제한다. 글꼴 객체는 더 쓰지 않는다.</summary>
    public void Dispose()
    {
        // 만든 텍스처를 한 번씩 해제한다.
        foreach (Texture2D texture in _textures) texture.Dispose();
        _textures.Clear();
    }
}

/// <summary>
/// UI 글자 그리기의 공통 진입점. 같은 네임스페이스 안의 <c>batch.DrawString(글꼴, 문자열, 위치, 색)</c> 호출은
/// 글꼴 라이브러리의 같은 이름 확장보다 이 메서드가 먼저 선택된다 (안쪽 네임스페이스의 확장 메서드가 우선한다).
/// </summary>
internal static class UiTextExtensions
{
    /// <summary>
    /// 문자열을 그린다. 글꼴이 원본 비트맵 글꼴이면 위치를 정수 픽셀로 내리고(원본은 정수 좌표에 그린다),
    /// 문자열에 그 글꼴에 없는 글자가 있으면 <see cref="OriginalFontInfo.Fallback"/> 글꼴로 대신 그린다.
    /// 그 밖의 글꼴은 글꼴 라이브러리의 그리기를 그대로 부른다.
    /// </summary>
    /// <param name="batch">스프라이트 배치 (Begin 호출된 상태)</param>
    /// <param name="font">글꼴</param>
    /// <param name="text">그릴 문자열</param>
    /// <param name="position">줄의 왼쪽 위 위치</param>
    /// <param name="color">글자 색</param>
    /// <remarks>2026-10-10 추가: 영어 화면의 한글 문구(개발용 표시 등)가 빈칸으로 나오지 않게 한다 (LEFT_JOBS.dotnetpj.md 5-3).</remarks>
    public static void DrawString(this SpriteBatch batch, SpriteFontBase font, string text, Vector2 position, Color color)
    {
        if (font.Tag is OriginalFontInfo info)
        {
            if (info.Covers(text)) position = new Vector2(MathF.Floor(position.X), MathF.Floor(position.Y));
            else font = info.Fallback;
        }
        font.DrawText(batch, text, position, color);
    }
}
