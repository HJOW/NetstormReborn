using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 바이트 문자열 측정과 기본 문맥 해석 영역이다.

/// <summary>글꼴 첫 폭 표와 별도 논리 높이의 원본 쓰기 순서다.
/// 근거: cpppj/src/client/GumpText.h/.cpp, 004a3df0/CD00425f90. ref 출력 alias도 허용한다.</summary>
public sealed class GumpText
{
    /// <summary>현재 파일 캐시/플랫폼 생성의 같은 글꼴 저장소.</summary>
    private readonly FontStore _fonts;
    /// <summary>그림자 출력 alias와 스타일 범위를 선택하는 판본.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>원본 슬롯 표와 같은 저장소를 빌린다.</summary>
    public GumpText(FontStore fonts, OriginalEdition edition = OriginalEdition.Patch1078) { ArgumentNullException.ThrowIfNull(fonts); _fonts = fonts; _edition = edition; }

    /// <summary>NUL 또는 명시 길이를 먼저 검사하고 실제 DWORD 합산/출력 순서로 쓴다. -1 이외 음수는0글자다.</summary>
    public static void MeasureGumpText(ReadOnlySpan<byte> bytes, ReadOnlySpan<int> widths, int fontHeight, GumpTextContext context, ref int width, ref int height, int length = -1, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        if (widths.Length != 256) throw new ArgumentException("Gump 글꼴 폭 표 크기 오류"); int count = 0;
        if (length == -1) { count = bytes.IndexOf((byte)0); if (count < 0) throw new ArgumentException("Gump 글자 범위에 NUL 종료 필요"); }
        else if (length > 0) { count = length; if (count > bytes.Length) throw new ArgumentException("Gump 글자 측정 길이 범위 초과"); }
        int total = 0;
        // 명시 길이는 코드0과128~255도 포함하고 합은 DWORD로 감긴다.
        for (int i = 0; i < count; i++) total = unchecked(total + widths[bytes[i]]);
        width = total; height = fontHeight;
        if ((context.Flags & 2) != 0)
        {
            if (edition == OriginalEdition.Patch1078) { width = unchecked(width + context.ShadowX); height = unchecked(height + context.ShadowY); }
            else { width = unchecked(total + context.ShadowX); height = unchecked(fontHeight + context.ShadowY); }
        }
    }

    /// <summary>원본32바이트 기본 문맥을 물리 구조체 재해석 없이 읽고 마지막 두 DWORD의 signed 비트를 유지한다.</summary>
    public static GumpTextContext DecodeGumpTextContext(ReadOnlySpan<uint> words)
    {
        if (words.Length != 8) throw new ArgumentException("Gump 글자 문맥 크기 오류"); return new(words[0], words[1], words[2], words[3], words[4], words[5], unchecked((int)words[6]), unchecked((int)words[7]));
    }

    /// <summary>불법 스타일은 폭만normal 슬롯0으로 대체하고 높이는 요청 슬롯을 쓴다. 비어 있는 조합의 폭은0이다.</summary>
    public void Measure(ReadOnlySpan<byte> bytes, GumpTextContext context, ref int width, ref int height, int length = -1)
    {
        int heightSlot = unchecked((int)context.Font), logicalHeight = FontStore.LogicalHeight(heightSlot); uint maximum = _edition == OriginalEdition.Patch1078 ? 4U : 2U;
        int slot = context.Style <= maximum ? heightSlot : 0; FontStyle style = context.Style <= maximum ? (FontStyle)context.Style : FontStyle.Normal; int[] widths = new int[256];
        bool cdEmpty = _edition != OriginalEdition.Patch1078 && slot == 6 && style == FontStyle.Bold;
        if (FontStore.Available(slot, style) && !cdEmpty)
        {
            BitmapFont font = _fonts.Get(slot, style);
            // 첫 측정 폭만 복사한다. ABC A/셋째 폭/캐시 높이를 섞지 않는다.
            for (int i = 0; i < widths.Length; i++) widths[i] = font.Glyph((byte)i).Advance;
        }
        MeasureGumpText(bytes, widths, logicalHeight, context, ref width, ref height, length, _edition);
    }

    /// <summary>004a3ed0의 캐시 높이/상승/하강. 불법 스타일은normal 슬롯0이고 비어 있는 조합은 세 값0이다.</summary>
    public (int Height, int Ascent, int Descent) Metrics(GumpTextContext context)
    {
        uint maximum = _edition == OriginalEdition.Patch1078 ? 4U : 2U; bool legal = context.Style <= maximum; int slot = legal ? unchecked((int)context.Font) : 0; FontStore.LogicalHeight(slot);
        FontStyle style = legal ? (FontStyle)context.Style : FontStyle.Normal; bool cdEmpty = _edition != OriginalEdition.Patch1078 && slot == 6 && style == FontStyle.Bold;
        if (!FontStore.Available(slot, style) || cdEmpty) return default; BitmapFont font = _fonts.Get(slot, style); return (font.Height(), font.Ascent(), font.Descent());
    }

    /// <summary>실제 라벨/기본 문맥 측정기를 돌 버튼의 같은 출력 주소에 연결한다. Draw용 세 문맥과 혼동하지 않는다.</summary>
    public static void BindStoneButtonText(StoneButtonGumpHooks hooks, GumpText text)
    {
        hooks.Measure = (uint token, byte[] label, uint[] context, ref int width, ref int height, uint length) => text.Measure(label, DecodeGumpTextContext(context), ref width, ref height, unchecked((int)length));
    }
}
