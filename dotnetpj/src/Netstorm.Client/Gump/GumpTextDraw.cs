using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 글리프 반복과 같은 트리/표면 출력 영역이다.

/// <summary>현재 색표 설치·실제 포인터 글리프·불법 스타일 로그 경계다.</summary>
public sealed class GumpGlyphHooks
{
    /// <summary>각 패스가 먼저 설치하는 BYTE 글자 색.</summary>
    public Action<byte>? Color { get; set; }
    /// <summary>실제 저장소의 글꼴 객체/코드/기준점 출력.</summary>
    public Action<BitmapFont, byte, ScreenPoint>? Glyph { get; set; }
    /// <summary>불법 스타일을normal 슬롯0으로 대체하기 전 진단.</summary>
    public Action? IllegalStyle { get; set; }
}

/// <summary>글꼴 전용 소스 치환표와 마지막 설치한 현재 VFX 표다.</summary>
public sealed class GumpGlyphColorState
{
    /// <summary>원본 글꼴 전용256색 표.100번만 현재 글자 색으로 바뀐다.</summary>
    public byte[] FontColors { get; } = new byte[256];
    /// <summary>배경과 공유 가능한 현재 VFX 표.내용만 복사하고 참조를 유지한다.</summary>
    public byte[] VfxColors { get; } = new byte[256];
}

/// <summary>원본의 그림자/외곽선/본문 반복과 현재 Gump 위치를 같은 글꼴/표면에 연결한다.
/// 근거: cpppj/src/client/GumpTextDraw.h/.cpp, 004a3f70·004a3430/CD004260f0·00426250.</summary>
public sealed class GumpTextDraw
{
    /// <summary>위→오른쪽→아래→왼쪽의 원본 외곽선 고정 순서.</summary>
    private static readonly ScreenPoint[] Outline = [new(0, -1), new(1, 0), new(0, 1), new(-1, 0)];
    /// <summary>현재 실제 글꼴 저장소.</summary>
    private readonly FontStore _fonts;
    /// <summary>현재 출력/색/진단 경계.</summary>
    private readonly GumpGlyphHooks _hooks;
    /// <summary>스타일과CD 빈bold 조합을 선택하는 판본.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>빈 문자열에도 색 설치가 필요하므로 색/글리프 경계를 생성 시 요구한다.</summary>
    public GumpTextDraw(FontStore fonts, GumpGlyphHooks hooks, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        ArgumentNullException.ThrowIfNull(fonts); ArgumentNullException.ThrowIfNull(hooks); if (hooks.Color is null || hooks.Glyph is null) throw new ArgumentException("Gump 색/글리프 출력 경계 필요"); _fonts = fonts; _hooks = hooks; _edition = edition;
    }

    /// <summary>각 패스의 색을 설치하고 포인터가 있는 글자만 출력/셋째 폭 전진한다.</summary>
    private static void Pass(ReadOnlySpan<byte> bytes, BitmapFont? font, ScreenPoint position, ScreenPoint offset, uint color, GumpGlyphHooks hooks)
    {
        hooks.Color!(unchecked((byte)color)); ScreenPoint at = new(unchecked(position.X + offset.X), unchecked(position.Y + offset.Y));
        // 특수 빈 프레임도 포인터가 있으므로 콜백과 전진을 수행한다. 첫 폭/ABC A는 쓰지 않는다.
        foreach (byte code in bytes)
        {
            if (font is null) continue; FontGlyph glyph = font.Glyph(code); if (glyph.Shape is null) continue;
            hooks.Glyph!(font, code, at); at = new(unchecked(at.X + glyph.DrawAdvance), at.Y);
        }
    }

    /// <summary>범위 계약을 먼저 확인한 뒤 그림자→고정4방향 외곽선→본문 순서로 출력한다. 명시 길이는 중간NUL도 포함한다.</summary>
    public static void DrawBitmapGlyphText(ReadOnlySpan<byte> bytes, BitmapFont? font, ScreenPoint position, GumpTextContext context, GumpGlyphHooks hooks, int length = -1)
    {
        int count = length;
        if (length == -1) { count = bytes.IndexOf((byte)0); if (count < 0) throw new ArgumentException("Gump 글자 출력 범위에 NUL 종료 필요"); }
        else if (length < 0 || length > bytes.Length) throw new ArgumentException("Gump 글자 출력 길이 범위 오류");
        if (hooks.Color is null || hooks.Glyph is null) throw new ArgumentException("Gump 색/글리프 출력 경계 필요"); ReadOnlySpan<byte> text = bytes[..count];
        if ((context.Flags & 2) != 0) Pass(text, font, position, new(context.ShadowX, context.ShadowY), context.ShadowColor, hooks);
        if ((context.Flags & 4) != 0)
        {
            // 원본의 두 PE가 쓰는 같은 고정표 순서다. 그림자와 독립 플래그다.
            foreach (ScreenPoint offset in Outline) Pass(text, font, position, offset, context.ShadowColor, hooks);
        }
        Pass(text, font, position, default, context.Color, hooks);
    }

    /// <summary>슬롯 범위를 먼저 검사하고 불법 스타일은 진단 뒤normal 슬롯0으로 대체한다. 빈 표도 색 설치는 수행한다.</summary>
    public void Draw(ReadOnlySpan<byte> bytes, ScreenPoint position, GumpTextContext context, int length = -1)
    {
        int slot = unchecked((int)context.Font); FontStore.LogicalHeight(slot); uint maximum = _edition == OriginalEdition.Patch1078 ? 4U : 2U, style = context.Style;
        if (style > maximum) { _hooks.IllegalStyle?.Invoke(); slot = 0; style = 0; }
        FontStyle selected = (FontStyle)style; bool cdEmpty = _edition != OriginalEdition.Patch1078 && slot == 6 && selected == FontStyle.Bold;
        BitmapFont? font = FontStore.Available(slot, selected) && !cdEmpty ? _fonts.Get(slot, selected) : null; DrawBitmapGlyphText(bytes, font, position, context, _hooks, length);
    }

    /// <summary>현재 Position의 투영 뒤 기본 문맥을 해석한다. 상대 오프셋은 DWORD 감김이며 항상 NUL 종료 출력이다.</summary>
    public void DrawGump(GumpCollection collection, uint token, ReadOnlySpan<byte> bytes, ScreenPoint offset, uint[] defaultContext)
    {
        ScreenPoint position = collection.Position(token); GumpTextContext selected = GumpText.DecodeGumpTextContext(defaultContext);
        Draw(bytes, new(unchecked(position.X + offset.X), unchecked(position.Y + offset.Y)), selected);
    }

    /// <summary>명시 문맥도 원본 포인터처럼 Position 뒤 현재 필드를 읽는다. in 참조로 투영 콜백의 문맥 변경을 보존한다.</summary>
    public void DrawGump(GumpCollection collection, uint token, ReadOnlySpan<byte> bytes, ScreenPoint offset, uint[] defaultContext, in GumpTextContext context)
    {
        ScreenPoint position = collection.Position(token); GumpTextContext selected = context;
        Draw(bytes, new(unchecked(position.X + offset.X), unchecked(position.Y + offset.Y)), selected);
    }

    /// <summary>같은 현재 VFX 클립에 실제 글리프를 해제/합성한다. 본문 뒤 이전 색표는 복구하지 않는다.</summary>
    public static GumpGlyphHooks MakeGumpGlyphPixelHooks(GumpPixelTarget target, GumpGlyphColorState state) => new()
    {
        Color = color => { state.FontColors[100] = color; state.FontColors.CopyTo(state.VfxColors, 0); },
        Glyph = (font, code, at) =>
        {
            ShapeDatabase shape = font.Glyph(code).Shape!; ShapeFrame frame = shape.Blocks()[0].Frames[0];
            if (!frame.IsSpecial()) target.Image(shape.Decode(0, 0), new(unchecked(at.X + frame.Rect.Left), unchecked(at.Y + frame.Rect.Top)), state.VfxColors);
        },
    };
}
