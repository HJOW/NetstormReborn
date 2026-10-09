using Netstorm.Core.Display;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 오브젝트 화면 경계(계획 5-2)를 원본 x86 기대값에 대조한다.
/// 기대값은 display-x86.tsv 의 Bounds 행이며 열 구성은 cpppj/tests/DisplayTests.cpp 와 같다:
/// Bounds | 프레임 | x비트 | y비트 | 왼쪽 | 위 | 오른쪽 | 아래.
/// 직전 Type 행의 프레임 표(폭·높이·hotspot 부호 있는 16비트 묶음)와 View 행의 카메라·Q16 확대율을 문맥으로 쓴다.
/// 선택 확장·그림자는 Bounds 계산에 들어가지 않으므로(원본 Update 경로) 이 검사 범위 밖이다.
/// </summary>
public sealed class X86DisplayBoundsTests
{
    /// <summary>화면 경계 368개: 소수·음수 좌표와 부호 있는 hotspot·Q16 네 배율을 포함한다.</summary>
    [Fact]
    public void Bounds_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("display");
        var failures = new List<string>();
        // 직전 Type 행의 프레임 표와 로딩 여부다.
        var frames = new List<(short Width, short Height, short HotspotX, short HotspotY)>();
        bool loaded = false;
        // 직전 View 행의 카메라·Q16 확대율이다 (Bounds 계산에는 표시 영역 자르기를 쓰지 않는다).
        int cameraX = 0;
        int cameraY = 0;
        int zoom = 0;
        bool hasView = false;
        int bounds = 0;
        // 모든 행을 순서대로 훑어 문맥을 따라간다.
        foreach (string[] row in rows)
        {
            try
            {
                if (row[0] == "Type")
                {
                    // "폭,높이,hotX,hotY;…" 묶음을 부호 있는 16비트로 읽는다.
                    frames.Clear();
                    foreach (string group in row[5].Split(';', StringSplitOptions.RemoveEmptyEntries))
                    {
                        string[] values = group.Split(',');
                        frames.Add(((short)X86Fixture.Int(values[0]), (short)X86Fixture.Int(values[1]),
                            (short)X86Fixture.Int(values[2]), (short)X86Fixture.Int(values[3])));
                    }
                    loaded = row[3] != "0";
                }
                else if (row[0] == "View")
                {
                    cameraX = X86Fixture.Int(row[3]);
                    cameraY = X86Fixture.Int(row[4]);
                    zoom = X86Fixture.Int(row[5]);
                    hasView = true;
                }
                else if (row[0] == "Bounds")
                {
                    if (!hasView)
                    {
                        throw new InvalidOperationException("Bounds 행 앞에 View 행이 없습니다");
                    }
                    int frame = X86Fixture.Int(row[1]);
                    float x = X86Fixture.FloatBits(row[2]);
                    float y = X86Fixture.FloatBits(row[3]);
                    DisplayRect actual = loaded
                        ? DisplayBounds.Bounds(frames[frame].Width, frames[frame].Height,
                            frames[frame].HotspotX, frames[frame].HotspotY, x, y, cameraX, cameraY, zoom)
                        : new DisplayRect(0, 0, 0, 0);
                    if (actual.Left != X86Fixture.Int(row[4]) || actual.Top != X86Fixture.Int(row[5])
                        || actual.Right != X86Fixture.Int(row[6]) || actual.Bottom != X86Fixture.Int(row[7]))
                    {
                        failures.Add($"Bounds {bounds + 1}: 기대 ({row[4]},{row[5]},{row[6]},{row[7]}) 실제 ({actual.Left},{actual.Top},{actual.Right},{actual.Bottom})");
                    }
                    bounds++;
                }
            }
            catch (Exception error)
            {
                failures.Add($"Bounds {bounds + 1}: {error.GetType().Name}: {error.Message}");
            }
        }
        Assert.True(failures.Count == 0, $"원본 입력 {bounds}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(8)));
        Assert.Equal(368, bounds);
    }

    /// <summary>투영은 0 쪽으로 자르고 Q16 확대는 산술 이동한다. 선택 확장과 그림자 번호도 원본대로다.</summary>
    [Fact]
    public void ProjectionScaleSelectionAndShadow_FollowOriginalRules()
    {
        // trunc(0.25 × 16 + 0.5) = 4 에서 카메라 7을 빼면 −3 이다.
        Assert.Equal(-3, DisplayBounds.Project(0.25f, DisplayBounds.ScreenScaleX, 7));
        // 음수도 0 쪽으로 자른다: trunc(−4.5) = −4.
        Assert.Equal(-4, DisplayBounds.Project(-0.3125f, DisplayBounds.ScreenScaleX, 0));
        // Q16 0.5배(32768)에서 −3 은 산술 이동으로 −2 가 된다.
        Assert.Equal(-2, DisplayBounds.Scale(-3, 32768));
        // 1.0배(65536)에서는 값이 그대로다.
        Assert.Equal(17, DisplayBounds.Scale(17, DisplayBounds.ZoomOne));
        // 유한하지 않거나 32비트에 못 들어가는 좌표는 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => DisplayBounds.Project(float.NaN, DisplayBounds.ScreenScaleX, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => DisplayBounds.Project(float.PositiveInfinity, DisplayBounds.ScreenScaleX, 0));
        // 선택 확장: 폭 9 미만이면 양쪽 9, 항상 양쪽 3, 위쪽은 판본별 15·9다. 다시 자르지 않는다.
        Assert.Equal(new DisplayRect(-13, -18, 21, 1),
            DisplayBounds.ExpandForSelection(new DisplayRect(-1, -3, 9, 1), DisplayEdition.Patch1078));
        Assert.Equal(new DisplayRect(-13, -12, 21, 1),
            DisplayBounds.ExpandForSelection(new DisplayRect(-1, -3, 9, 1), DisplayEdition.CD1072));
        // 폭 19 이상이면 양쪽 9를 더하지 않는다.
        Assert.Equal(new DisplayRect(-3, -18, 23, 1),
            DisplayBounds.ExpandForSelection(new DisplayRect(0, -3, 20, 1), DisplayEdition.Patch1078));
        // 표시 영역 자르기는 각 모서리를 독립적으로 고정한다.
        Assert.Equal(new DisplayRect(12, 9, 20, 30),
            DisplayBounds.Clip(new DisplayRect(0, -3, 20, 30), new DisplayRect(12, 9, 620, 450)));
        // 그림자 타입은 프레임 수 + 현재 프레임이다.
        Assert.Equal(7, DisplayBounds.ShadowFrameIndex(DisplayBounds.ShadowFlag1, 5, 2));
        Assert.Equal(2, DisplayBounds.ShadowFrameIndex(0, 5, 2));
    }
}
