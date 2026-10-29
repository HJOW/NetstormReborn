using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 형식의 독립 입력 글꼴과 조각 관측만 공유한다.

/// <summary>실제 글꼴/픽셀에 조각을 연결하기 위한 입력 생성기다. 기대 배치나 픽셀은 여기서 계산하지 않는다.</summary>
internal static class StyleTextTestSupport
{
    /// <summary>A/공백의 실제 독립VFX와 폭2/3을 가진 입력 캐시.title은 원본 제목 슬롯3 캐시도 명시적으로 공급한다.</summary>
    public static FontStore PixelFonts(bool title = false)
    {
        byte[] widths = new byte[1024], mask = new byte[256]; GumpFontFixture.Put(widths, 65 * 4, 2); GumpFontFixture.Put(widths, 32 * 4, 3); mask[65] = mask[32] = 1; byte[] cache = GumpFontFixture.GlyphCache(widths, mask); cache.AsSpan(36, 1024).Clear(); GumpFontFixture.Put(cache, 36 + 65 * 4, 2); GumpFontFixture.Put(cache, 36 + 32 * 4, 3);
        return new(path => path == "d/!Arial.normal.14.700.chfnt" || (title && path == "d/!Arial.normal.20.700.chfnt") ? cache : null, "Arial", (_, _, _, _) => throw new InvalidOperationException("검사 입력 글꼴 없음"));
    }
    /// <summary>안전한 관측용 체인 번호 목록이다. 예상 연결 순서를 만들지 않는다.</summary>
    public static uint[] Chain(StyleTextState state, uint head)
    {
        var result = new List<uint>(); var seen = new HashSet<uint>();
        // 실제 현재next만 순회한다.
        while (head != 0) { Assert.True(seen.Add(head)); result.Add(head); head = state.Fragments[head].Next; } return result.ToArray();
    }
    /// <summary>저장NUL 앞 바이트를 관측용 일대일 문자열로 돌려준다.</summary>
    public static string Text(StyleTextFragment fragment)
    {
        int end = fragment.Text.AsSpan().IndexOf((byte)0); return System.Text.Encoding.Latin1.GetString(end < 0 ? fragment.Text : fragment.Text.AsSpan(0, end));
    }
}
