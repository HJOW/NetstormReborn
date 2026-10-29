using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 버튼 문맥/선/배치와 저장 팔레트의 픽셀 계약을 대조한다.

/// <summary>원본 전체 버튼 생성/Draw 기록과 출력 보호 계약을 검사한다.
/// 근거: cpppj/tests/GumpVisualTests.cpp. 창/GDI 장치 대신 기존 순수 Screen을 사용한다.</summary>
public sealed class GumpVisualTests
{
    /// <summary>현재32바이트 문맥을 필드 재배치 없이 원본 offset으로 기록한다.</summary>
    private static byte[] ContextBytes(GumpTextContext c)
    {
        uint[] values = [c.Flags, c.Font, c.Color, c.Style, c.Extra, c.ShadowColor, unchecked((uint)c.ShadowX), unchecked((uint)c.ShadowY)]; var bytes = new byte[32];
        // 현재 각 DWORD만 원본 메모리 순서로 쓴다.
        for (int i = 0; i < values.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(i * 4), values[i]);
        return bytes;
    }

    /// <summary>세 판본192행의 문맥96바이트·선 호출 순서·문자 위치/문맥 선택을 원본 그대로 대조한다.</summary>
    [Fact]
    public void MatchesThreeNativeButtonBodies()
    {
        string[][] rows = X86Fixture.Read("gumpvisual"); Assert.Equal(192, rows.Length);
        // 기대 배치를 계산하지 않고 각 독립 원본 입력/출력만 읽는다.
        foreach (string[] r in rows)
        {
            var state = new ButtonVisualState(new(X86Fixture.Int(r[2]), X86Fixture.Int(r[3]), X86Fixture.Int(r[4]), X86Fixture.Int(r[5])),
                X86Fixture.Int(r[6]), X86Fixture.Int(r[7]), X86Fixture.Int(r[8]), X86Fixture.Int(r[9]), X86Fixture.UInt(r[10]));
            var colors = new ButtonPalette(X86Fixture.UInt(r[11]), X86Fixture.UInt(r[12]), X86Fixture.UInt(r[13])); GumpTextContext[] contexts = GumpVisual.ButtonTextContexts(colors);
            byte[] expected = X86Fixture.Bytes(r[14]); Assert.Equal(96, expected.Length); Assert.Equal(expected, contexts.SelectMany(ContextBytes).ToArray());
            ButtonDrawPlan plan = GumpVisual.PlanButtonDraw(state, contexts, colors.Edge);
            Assert.Equal(new ScreenPoint(X86Fixture.Int(r[15]), X86Fixture.Int(r[16])), plan.TextOffset); int index = X86Fixture.Int(r[17]);
            Assert.Equal(expected.AsSpan(index * 32, 32).ToArray(), ContextBytes(plan.Text));
            Assert.Equal(r[18], plan.Lines.Count == 0 ? "-" : string.Join('|', plan.Lines.Select(l => $"{l.First.X},{l.First.Y},{l.Last.X},{l.Last.Y},{l.Color}")));
        }
    }

    /// <summary>일시 팔레트 효과는 저장 버튼 표를 바꾸지 않고 새 파일 로딩만 새 번호를 선택한다.</summary>
    [Fact]
    public void UsesLoadedPaletteUntilNextFile()
    {
        var screen = new Screen(640, 480); byte[] col = new byte[776]; col.AsSpan(8).Fill(255);
        col[8 + 41 * 3] = col[8 + 199 * 3] = 55; col[9 + 41 * 3] = col[9 + 199 * 3] = 51; col[10 + 41 * 3] = col[10 + 199 * 3] = 54;
        // 전체 표 위치가 다른 두 판본에서 같은 실제 버튼 역할을 읽는다.
        foreach (bool patch in new[] { true, false })
        {
            screen.LoadPalette(new GamePalette(col), patch1078: patch); ButtonPalette colors = GumpVisual.ButtonColors(screen.Colors());
            Assert.Equal(new ButtonPalette(0, 1, 41), colors); screen.SetPalette(41, 1, [new(255, 255, 255, 0)], true);
            Assert.Equal(199U, screen.FindColor(55, 51, 54)); Assert.Equal(41U, GumpVisual.ButtonColors(screen.Colors()).Edge);
        }
        col[8 + 41 * 3] = col[9 + 41 * 3] = col[10 + 41 * 3] = 255; col[8 + 81 * 3] = 55; col[9 + 81 * 3] = 51; col[10 + 81 * 3] = 54;
        screen.LoadPalette(new GamePalette(col)); Assert.Equal(81U, GumpVisual.ButtonColors(screen.Colors()).Edge);
    }

    /// <summary>선 끝점/역순/겹침/클리핑과 DWORD 색의 BYTE 절삭을 독립 픽셀 배열로 고정한다.</summary>
    [Fact]
    public void LinePixelsIncludeBothEndsAndClip()
    {
        var canvas = new IndexedImage { Width = 6, Height = 5, Indices = Enumerable.Repeat((byte)9, 30).ToArray(), Opacity = Enumerable.Repeat((byte)255, 30).ToArray() };
        GumpVisual.DrawGumpLines(canvas, [new(new(4, 1), new(1, 1), 257), new(new(2, -7), new(2, 3), 2), new(new(-3, 4), new(1, 4), 3),
            new(new(-5, 0), new(-5, 4), 4), new(new(int.MinValue, 0), new(int.MaxValue, 0), 5)]);
        Assert.Equal(new byte[] { 5, 5, 5, 5, 5, 5, 9, 1, 2, 1, 1, 9, 9, 9, 2, 9, 9, 9, 9, 9, 2, 9, 9, 9, 3, 3, 9, 9, 9, 9 }, canvas.Indices);
        Assert.All(canvas.Opacity, value => Assert.Equal(255, value));
    }

    /// <summary>아직 읽지 않은 색 표·잘못된 이미지 용량·버튼이 쓰지 않는 대각선은 거부한다.</summary>
    [Fact]
    public void RejectsInvalidOutputInputs()
    {
        Assert.Throws<ArgumentException>(() => GumpVisual.ButtonColors(new())); var canvas = new IndexedImage { Width = 2, Height = 2, Indices = [0, 0, 0] };
        Assert.Throws<ArgumentException>(() => GumpVisual.DrawGumpLines(canvas, [])); canvas.Indices = [0, 0, 0, 0];
        Assert.Throws<ArgumentException>(() => GumpVisual.DrawGumpLines(canvas, [new(new(0, 0), new(1, 1), 7)]));
    }
}
