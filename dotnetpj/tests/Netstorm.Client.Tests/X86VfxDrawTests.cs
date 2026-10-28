using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(graphics-x86.tsv: 709행)로 VFX 셰이프 그리기의 L1 일치를 검사한다.
/// 근거: cpppj/tests/AssetTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며 C# 결과로 바꾸지 않는다. (R2b, 2026-10-28)
/// </summary>
public sealed class X86VfxDrawTests
{
    /// <summary>fixture 행 수(색 0/255·모든 방향 클리핑·원점 이동·오류 반환).</summary>
    private const int Rows = 709;

    /// <summary>원본 VFX 셰이프 그리기가 모든 관측에서 같은 반환과 같은 화면을 낸다.</summary>
    [Fact]
    public void ShapeDrawMatchesNative()
    {
        string[][] rows = X86Fixture.Read("graphics");
        Assert.Equal(Rows, rows.Length);
        // 관측마다 같은 셰이프·화면·pane로 그리고 반환과 전체 화면을 비교한다
        foreach (string[] row in rows)
        {
            var database = new ShapeDatabase(X86Fixture.Bytes(row[0]));
            int width = X86Fixture.Int(row[1]);
            int height = X86Fixture.Int(row[2]);
            var pane = new ShapeRect(X86Fixture.Int(row[3]), X86Fixture.Int(row[4]), X86Fixture.Int(row[5]), X86Fixture.Int(row[6]));
            int x = X86Fixture.Int(row[7]);
            int y = X86Fixture.Int(row[8]);
            byte background = (byte)X86Fixture.Int(row[9]);
            int expectedResult = X86Fixture.Int(row[10]);
            byte[] expected = X86Fixture.Bytes(row[11]);
            var canvas = new byte[width > 0 && height > 0 ? width * height : 0];
            Array.Fill(canvas, background);
            int actual = VfxDraw.DrawShape(canvas, width, height, database, 0, 0, x, y, pane);
            Assert.Equal(expectedResult, actual);
            Assert.Equal(expected, canvas);
        }
    }
}
