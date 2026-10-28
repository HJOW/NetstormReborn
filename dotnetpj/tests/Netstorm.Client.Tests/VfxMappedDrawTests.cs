using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// mapped RLE 그리기의 보호 계약을 확인한다. 같은 셰이프를 기본 그리기와 치환 그리기 두 번 그려
/// 불투명 픽셀만 치환표대로 바뀌고 배경은 그대로임을 고정한다. 원본 기대값 재생은 X86VfxDrawTests가 맡는다.
/// 근거: cpppj/src/client/VFXMappedDraw.cpp (원본 0040229f). (R2b, 2026-10-28)
/// </summary>
public sealed class VfxMappedDrawTests
{
    /// <summary>graphics fixture 첫 행의 셰이프와 화면·pane. L1 행과 같은 입력이다.</summary>
    private static (ShapeDatabase database, int width, int height, ShapeRect pane, int x, int y) Scene()
    {
        string[][] rows = X86Fixture.Read("graphics");
        string[] row = rows[0];
        var database = new ShapeDatabase(X86Fixture.Bytes(row[0]));
        return (database, X86Fixture.Int(row[1]), X86Fixture.Int(row[2]),
            new ShapeRect(X86Fixture.Int(row[3]), X86Fixture.Int(row[4]), X86Fixture.Int(row[5]), X86Fixture.Int(row[6])),
            X86Fixture.Int(row[7]), X86Fixture.Int(row[8]));
    }

    /// <summary>불투명 픽셀만 치환표대로 바뀌고 배경은 그대로 둔다.</summary>
    [Fact]
    public void OnlyOpaquePixelsAreRemapped()
    {
        (ShapeDatabase database, int width, int height, ShapeRect pane, int x, int y) = Scene();
        var plain = new byte[width * height];
        var mapped = new byte[width * height];
        Array.Fill(plain, (byte)200);
        Array.Fill(mapped, (byte)200);
        Assert.Equal(0, VfxDraw.DrawShape(plain, width, height, database, 0, 0, x, y, pane));
        var table = new byte[256];
        // 치환표를 뒤집어 불투명 픽셀의 이동을 드러낸다
        for (int i = 0; i < 256; i++) table[i] = (byte)(255 - i);
        Assert.Equal(0, VfxMappedDraw.DrawMappedShape(mapped, width, height, database, 0, 0, x, y, pane, table));
        // 배경(200)이던 자리는 그대로, 그 외는 치환표를 탄다
        for (int i = 0; i < plain.Length; i++)
        {
            Assert.Equal(plain[i] == 200 ? 200 : 255 - plain[i], mapped[i]);
        }
    }

    /// <summary>치환표가 256바이트가 아니면 쓰기 전에 거부한다. 반환 코드는 기본 그리기와 같다.</summary>
    [Fact]
    public void RejectsBadTableAndMirrorsReturnCodes()
    {
        (ShapeDatabase database, int width, int height, ShapeRect pane, int x, int y) = Scene();
        var canvas = new byte[width * height];
        var table = new byte[256];
        Assert.Throws<ArgumentException>(() => VfxMappedDraw.DrawMappedShape(canvas, width, height, database, 0, 0, x, y, pane, new byte[10]));
        Assert.Equal(-1, VfxMappedDraw.DrawMappedShape(canvas, 0, height, database, 0, 0, x, y, pane, table));
        Assert.Equal(-2, VfxMappedDraw.DrawMappedShape(canvas, width, height, database, 0, 0, x, y, new ShapeRect(5, 5, 2, 2), table));
        Assert.Equal(-3, VfxMappedDraw.DrawMappedShape(canvas, width, height, database, 0, 0, x + 100, y + 100, pane, table));
    }
}
