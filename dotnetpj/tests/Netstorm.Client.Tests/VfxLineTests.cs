using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// VFX 일반 선의 보호 계약을 확인한다. 기대 픽셀은 cpppj/src/client/VFXLine.cpp 와 같은 알고리즘을
/// 그대로 옮긴 별도 Python 참조 구현(vfxline_ref.py, 저장소에 두지 않음)으로 계산한 값이며 C# 결과로 바꾸지 않는다.
/// 원본 기계어 대조 fixture는 없으므로 L1이 아니라 계약 검사다.
/// 근거: cpppj/src/client/VFXLine.cpp (원본 004011f1). (R2b, 2026-10-28)
/// </summary>
public sealed class VfxLineTests
{
    /// <summary>선 경우 하나(화면·클립·끝점·색·기대 픽셀 "x,y;..." 순서).</summary>
    private sealed record VfxLineCase(int Width, int Height, ScreenRect Clip, ScreenPoint First, ScreenPoint Last, uint Color, string Pixels);

    /// <summary>참조 구현이 만든 10가지 경우(대각·역대각·클립 절단·수직·한 점·완만·가파름).</summary>
    private static readonly VfxLineCase[] Cases =
    [
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(0,0), new ScreenPoint(15,9), 7u, "0,0;1,1;2,1;3,2;4,2;5,3;6,4;7,4;8,5;9,5;10,6;11,7;12,7;13,8;14,8;15,9"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(15,0), new ScreenPoint(0,9), 3u, "15,0;13,1;14,1;11,2;12,2;10,3;8,4;9,4;6,5;7,5;5,6;3,7;4,7;1,8;2,8;0,9"),
new VfxLineCase(16, 10, new ScreenRect(2,2,14,8), new ScreenPoint(-5,-5), new ScreenPoint(20,12), 9u, "5,2;6,2;7,3;8,4;9,5;10,5;11,6;12,7;13,7"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(3,8), new ScreenPoint(11,1), 5u, "11,1;10,2;9,3;7,4;8,4;6,5;5,6;4,7;3,8"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(0,9), new ScreenPoint(15,0), 11u, "15,0;13,1;14,1;11,2;12,2;10,3;8,4;9,4;6,5;7,5;5,6;3,7;4,7;1,8;2,8;0,9"),
new VfxLineCase(16, 10, new ScreenRect(5,1,6,9), new ScreenPoint(5,0), new ScreenPoint(5,9), 13u, "5,1;5,2;5,3;5,4;5,5;5,6;5,7;5,8"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(7,4), new ScreenPoint(7,4), 15u, "7,4"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(0,0), new ScreenPoint(15,2), 21u, "0,0;1,0;2,0;3,0;4,1;5,1;6,1;7,1;8,1;9,1;10,1;11,1;12,2;13,2;14,2;15,2"),
new VfxLineCase(16, 10, new ScreenRect(0,0,16,10), new ScreenPoint(2,9), new ScreenPoint(14,9), 23u, "2,9;3,9;4,9;5,9;6,9;7,9;8,9;9,9;10,9;11,9;12,9;13,9;14,9"),
new VfxLineCase(16, 10, new ScreenRect(3,3,12,7), new ScreenPoint(0,5), new ScreenPoint(15,5), 31u, "3,5;4,5;5,5;6,5;7,5;8,5;9,5;10,5;11,5"),
    ];

    /// <summary>참조 구현과 같은 픽셀을 같은 색으로 찍는다.</summary>
    [Fact]
    public void MatchesReferenceImplementation()
    {
        // 경우마다 빈 화면에 선을 하나 그리고 기대 집합과 비교한다
        foreach (VfxLineCase kase in Cases)
        {
            var pixels = new byte[kase.Width * kase.Height];
            VfxLine.DrawVfxLine(pixels, kase.Width, kase.Height, kase.Clip, kase.First, kase.Last, kase.Color);
            var expected = new byte[kase.Width * kase.Height];
            foreach (string point in kase.Pixels.Split(';'))
            {
                string[] parts = point.Split(',');
                expected[int.Parse(parts[1]) * kase.Width + int.Parse(parts[0])] = (byte)kase.Color;
            }
            Assert.Equal(expected, pixels);
        }
    }

    /// <summary>표면이 작거나 거리가 signed 범위를 넘으면 쓰기 전에 거부한다.</summary>
    [Fact]
    public void RejectsBadSurfaceAndRange()
    {
        var pixels = new byte[16];
        Assert.Throws<ArgumentException>(() => VfxLine.DrawVfxLine(pixels, 0, 4, new ScreenRect(0, 0, 4, 4), new ScreenPoint(0, 0), new ScreenPoint(1, 1), 1));
        Assert.Throws<ArgumentException>(() => VfxLine.DrawVfxLine(pixels, 4, 5, new ScreenRect(0, 0, 4, 4), new ScreenPoint(0, 0), new ScreenPoint(1, 1), 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => VfxLine.DrawVfxLine(new byte[8], 8, 1, new ScreenRect(0, 0, 8, 1),
            new ScreenPoint(int.MinValue, 0), new ScreenPoint(int.MaxValue, 1), 1));
    }

    /// <summary>화면 밖 선과 빈 클립은 화면을 바꾸지 않고 조용히 돌아간다.</summary>
    [Fact]
    public void OutsideLinesLeavePixelsAlone()
    {
        var pixels = new byte[16];
        VfxLine.DrawVfxLine(pixels, 4, 4, new ScreenRect(0, 0, 4, 4), new ScreenPoint(10, 10), new ScreenPoint(20, 20), 9);
        VfxLine.DrawVfxLine(pixels, 4, 4, new ScreenRect(1, 1, 1, 1), new ScreenPoint(0, 0), new ScreenPoint(3, 3), 9);
        Assert.All(pixels, p => Assert.Equal(0, p));
    }
}
