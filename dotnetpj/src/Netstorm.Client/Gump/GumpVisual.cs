using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 원본 버튼의 팔레트·문자 문맥·선/배치 계획 영역이다.

/// <summary>파일 팔레트 로더의 저장 표에서 복사하는 버튼 색 번호다.</summary>
/// <param name="Black">기본 검정 번호.</param>
/// <param name="White">기본 흰색 번호.</param>
/// <param name="Edge">RGB55/51/54의 저장 외곽선 번호.</param>
public readonly record struct ButtonPalette(uint Black, uint White, uint Edge);

/// <summary>원본32바이트 TextContext의 필드 순서다. 문맥은 값으로 복사하며 각 출력기가 현재 필드를 해석한다.</summary>
/// <param name="Flags">그림자 등 출력 플래그.</param>
/// <param name="Font">글꼴 슬롯.</param>
/// <param name="Color">글자 팔레트 번호.</param>
/// <param name="Style">글꼴/정렬 스타일.</param>
/// <param name="Extra">+0x10 추가 필드.</param>
/// <param name="ShadowColor">그림자 색 번호.</param>
/// <param name="ShadowX">그림자 가로 오프셋.</param>
/// <param name="ShadowY">그림자 세로 오프셋.</param>
public record struct GumpTextContext(uint Flags, uint Font, uint Color, uint Style, uint Extra, uint ShadowColor, int ShadowX, int ShadowY);

/// <summary>양 끝을 포함하는 원본 VFX 선 요청이다. 실제8비트 출력에서만 색을 BYTE로 좁힌다.</summary>
/// <param name="First">첫 끝점.</param>
/// <param name="Last">마지막 끝점.</param>
/// <param name="Color">전체 DWORD 색 번호.</param>
public readonly record struct GumpLine(ScreenPoint First, ScreenPoint Last, uint Color);

/// <summary>현재 버튼의 화면 영역·측정 문자 크기·두 눌림 상태와 플래그다.</summary>
/// <param name="Rect">버튼의 화면 사각형.</param>
/// <param name="TextWidth">측정한 글자 폭.</param>
/// <param name="TextHeight">측정한 글자 높이.</param>
/// <param name="Value">첫 눌림 상태+a0.</param>
/// <param name="Pressed">두 번째 눌림 상태+a4.</param>
/// <param name="Flags">버튼 플래그+ac.</param>
public readonly record struct ButtonVisualState(ScreenRect Rect, int TextWidth, int TextHeight, int Value, int Pressed, uint Flags);

/// <summary>원본 선 호출 순서와 상대 문자 위치/문맥을 분리한 출력 계획이다.</summary>
public sealed class ButtonDrawPlan
{
    /// <summary>모서리 덮어쓰기를 유지하는 선 순서.</summary>
    public List<GumpLine> Lines { get; } = [];
    /// <summary>버튼에 상대적인 문자 기준점.</summary>
    public ScreenPoint TextOffset { get; set; }
    /// <summary>선택된 문맥의 값 복사본.</summary>
    public GumpTextContext Text { get; set; }
}

/// <summary>버튼의 정상/눌림/비활성 문맥과 외곽선/문자 배치를 계산한다.
/// 근거: cpppj/src/client/GumpVisual.h·GumpVisual.cpp, 10.78 004254b0/00424ff0.</summary>
public static class GumpVisual
{
    /// <summary>파일 로더의 기본 검정/흰색과 저장된 외곽선 번호를 읽는다. 일시 번개 팔레트에서 다시 검색하지 않는다.</summary>
    public static ButtonPalette ButtonColors(PaletteColorTable palette)
    {
        ArgumentNullException.ThrowIfNull(palette);
        // 판본별 위치가 다른 전체 표에서 RGB 역할을 찾아 저장된 번호만 사용한다.
        foreach (NamedPaletteColor color in palette.Named)
            if (color.Red == 55 && color.Green == 51 && color.Blue == 54) return new(palette.Basic[0], palette.Basic[7], color.Index);
        throw new ArgumentException("버튼 팔레트 표를 먼저 읽어야 합니다", nameof(palette));
    }

    /// <summary>정상/눌림은 흰 글자와 검정1/1 그림자, 비활성은 그림자 없는 검정 문맥이다. 원본에서 읽지 않는 필드는0으로 둔다.</summary>
    public static GumpTextContext[] ButtonTextContexts(ButtonPalette colors)
    {
        var active = new GumpTextContext(2, 0, colors.White, 0, 0, colors.Black, 1, 1);
        var disabled = new GumpTextContext(0, 0, colors.Black, 0, 0, 0, 0, 0);
        return [active, active, disabled];
    }

    /// <summary>두 상태 중 하나라도0이 아니면 눌림 배치를 쓰고 flag1 비활성 문맥을 우선한다. flag8은 외곽선을 생략한다.</summary>
    public static ButtonDrawPlan PlanButtonDraw(ButtonVisualState state, ReadOnlySpan<GumpTextContext> contexts, uint edge)
    {
        if (contexts.Length != 3) throw new ArgumentException("버튼 문맥은3개여야 합니다", nameof(contexts));
        ScreenRect rect = state.Rect; var result = new ButtonDrawPlan(); bool down = state.Value != 0 || state.Pressed != 0;
        if ((state.Flags & 8) == 0)
        {
            result.Lines.Add(new(new(unchecked(rect.Left + 1), rect.Top), new(unchecked(rect.Right - 2), rect.Top), edge));
            result.Lines.Add(new(new(unchecked(rect.Left + 1), unchecked(rect.Bottom - 2)), new(unchecked(rect.Right - 2), unchecked(rect.Bottom - 2)), edge));
            result.Lines.Add(new(new(rect.Left, unchecked(rect.Top + 1)), new(rect.Left, unchecked(rect.Bottom - 3)), edge));
            result.Lines.Add(new(new(unchecked(rect.Right - 1), unchecked(rect.Top + 1)), new(unchecked(rect.Right - 1), unchecked(rect.Bottom - 3)), edge));
            rect = new(unchecked(rect.Left + 1), unchecked(rect.Top + 1), unchecked(rect.Right - 1), unchecked(rect.Bottom - 2));
        }
        result.TextOffset = new(unchecked(unchecked(unchecked(rect.Right - state.TextWidth) - rect.Left) / 2 + (down ? 3 : 2)),
            unchecked(unchecked(unchecked(rect.Bottom - state.TextHeight) - rect.Top) / 2 + (down ? 2 : 1)));
        result.Text = contexts[(state.Flags & 1) != 0 ? 2 : down ? 1 : 0]; return result;
    }

    /// <summary>직교 버튼 선을 순서대로8비트 그림에 합성한다. 역방향/양 끝 포함/극단 끝점과 불투명 마스크 불변을 유지한다.</summary>
    public static void DrawGumpLines(IndexedImage canvas, ReadOnlySpan<GumpLine> lines)
    {
        ArgumentNullException.ThrowIfNull(canvas);
        if ((long)canvas.Width * canvas.Height != canvas.Indices.Length) throw new ArgumentException("GUI 그림 크기 오류", nameof(canvas));
        // 원본 호출 순서대로 뒤 선이 겹친 픽셀을 덮는다.
        foreach (GumpLine line in lines)
        {
            if (line.First.X != line.Last.X && line.First.Y != line.Last.Y) throw new ArgumentException("GUI 직교선 필요", nameof(lines));
            long left = Math.Max(0L, Math.Min(line.First.X, line.Last.X)); long top = Math.Max(0L, Math.Min(line.First.Y, line.Last.Y));
            long right = Math.Min(canvas.Width, (long)Math.Max(line.First.X, line.Last.X) + 1); long bottom = Math.Min(canvas.Height, (long)Math.Max(line.First.Y, line.Last.Y) + 1);
            if (left >= right || top >= bottom) continue;
            // 유효 행의 양 끝 포함 구간만 채우고 opacity는 바꾸지 않는다.
            for (long y = top; y < bottom; y++) canvas.Indices.AsSpan((int)(y * canvas.Width + left), (int)(right - left)).Fill(unchecked((byte)line.Color));
        }
    }
}
