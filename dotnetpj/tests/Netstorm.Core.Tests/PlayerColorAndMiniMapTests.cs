using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>
/// 플레이어 색 번호 규칙, 선택 괄호·체력 막대 모양, 미니맵 좌표 규칙을 검사한다.
/// 근거: exe 의 색 이름 목록·기본 색 표(FUN_0043b540)와 2026-10-03 TEST01 녹화, 1-1 시작 캡처.
/// </summary>
public sealed class PlayerColorAndMiniMapTests
{
    /// <summary>색 이름은 원본 목록의 순번이 색 번호이고 대소문자를 가리지 않는다. 목록에 없는 이름은 0이다.</summary>
    [Theory]
    [InlineData("blue", 1)]
    [InlineData("Red", 2)]
    [InlineData("WHITE", 3)]
    [InlineData("green", 4)]
    [InlineData("purple", 5)]
    [InlineData("yellow", 6)]
    [InlineData("lightblue", 7)]
    [InlineData(" orange ", 8)]
    [InlineData("none", 0)]
    [InlineData("cyan", 0)]
    [InlineData("", 0)]
    [InlineData(null, 0)]
    public void Parse_UsesOriginalNameOrder(string? name, int expected)
    {
        Assert.Equal(expected, PlayerColors.Parse(name));
    }

    /// <summary>덮어쓰지 않으면 소유자 번호가 곧 색 번호다 (사람 1 파랑, 2 빨강, 3 흰색).</summary>
    [Fact]
    public void Table_DefaultIsOwnerNumber()
    {
        IReadOnlyDictionary<int, int> table = PlayerColors.Table();
        // 소유자 1~8 이 모두 자기 번호를 색으로 가진다
        for (int owner = 1; owner <= PlayerColors.Maximum; owner++)
        {
            Assert.Equal(owner, table[owner]);
        }
        Assert.False(table.ContainsKey(0));
        Assert.Equal(0, PlayerColors.Default(0));
        Assert.Equal(0, PlayerColors.Default(9));
    }

    /// <summary>유효한 덮어쓰기만 반영하고 범위 밖 값은 기본색으로 둔다.</summary>
    [Fact]
    public void Table_AppliesValidOverridesOnly()
    {
        IReadOnlyDictionary<int, int> table = PlayerColors.Table(new Dictionary<int, int> { [2] = 8, [3] = 0, [4] = 12 });
        Assert.Equal(8, table[2]);
        Assert.Equal(3, table[3]);
        Assert.Equal(4, table[4]);
    }

    /// <summary>미션 머리 값 aiNColor 를 색 번호로 읽는다 (따옴표·대소문자 무관, 모르는 이름은 빼 둔다).</summary>
    [Fact]
    public void MissionStart_ReadsAiColors()
    {
        var header = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["ai2Color"] = "orange",
            ["ai3Color"] = "\"red\"",
            ["ai7Color"] = "cyan",
        };
        MissionStart start = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        Assert.Equal(8, start.AiColors[2]);
        Assert.Equal(2, start.AiColors[3]);
        Assert.False(start.AiColors.ContainsKey(7));
        Assert.Equal(8, PlayerColors.Table(start.AiColors)[2]);
    }

    /// <summary>괄호 색은 색 번호별 팔레트 번호 표(exe 0x5319c4)를 따른다. 파랑(1)과 중립(0)은 84번이다.</summary>
    [Theory]
    [InlineData(0, 84)]
    [InlineData(1, 84)]
    [InlineData(2, 249)]
    [InlineData(3, 184)]
    [InlineData(8, 222)]
    [InlineData(99, 84)]
    public void BracketPaletteIndex_FollowsColorTable(int color, int expected)
    {
        Assert.Equal(expected, SelectionMarks.BracketPaletteIndex(color));
    }

    /// <summary>괄호 길이는 폭 ÷ 4, (높이 − 1) ÷ 4 의 정수 나눗셈이다 (Cloud Floater 그림 상자 63×52 → 15, 12).</summary>
    [Fact]
    public void BracketArms_AreQuarterOfFrameBox()
    {
        Assert.Equal((15, 12), SelectionMarks.BracketArms(63, 52));
        Assert.Equal((5, 6), SelectionMarks.BracketArms(22, 25));
    }

    /// <summary>체력 색: 절반 초과 초록, 절반 이하 노랑, 4분의 1 이하 빨강 (정수 나눗셈 경계 포함).</summary>
    [Theory]
    [InlineData(200, 200, 0x5f, 0xbc, 0x5c)]
    [InlineData(101, 200, 0x5f, 0xbc, 0x5c)]
    [InlineData(100, 200, 0xff, 0xff, 0x16)]
    [InlineData(51, 200, 0xff, 0xff, 0x16)]
    [InlineData(50, 200, 0xff, 0x16, 0x16)]
    [InlineData(0, 200, 0xff, 0x16, 0x16)]
    public void HealthColor_UsesHalfAndQuarterThresholds(int hitPoints, int maximum, int r, int g, int b)
    {
        Assert.Equal(((byte)r, (byte)g, (byte)b), SelectionMarks.HealthColor(hitPoints, maximum));
    }

    /// <summary>막대는 그림 상자 위 끝 바로 위에 놓이고, 추가 막대는 3픽셀씩 위로 쌓인다. 채움 길이는 값 × 폭 ÷ 최대다.</summary>
    [Fact]
    public void Bars_SitAboveFrameBox()
    {
        Assert.Equal(new PixelRect(99, 45, 42, 4), SelectionMarks.BarBackground(100, 50, 40));
        Assert.Equal(new PixelRect(100, 46, 40, 2), SelectionMarks.BarFill(100, 50, 40, 200, 200));
        Assert.Equal(new PixelRect(100, 46, 10, 2), SelectionMarks.BarFill(100, 50, 40, 50, 200));
        Assert.Equal(new PixelRect(100, 46, 40, 2), SelectionMarks.BarFill(100, 50, 40, 999, 200));
        Assert.Equal(new PixelRect(100, 46, 0, 2), SelectionMarks.BarFill(100, 50, 40, -5, 200));
        Assert.Equal(new PixelRect(99, 42, 42, 4), SelectionMarks.BarBackground(100, 50, 40, 1));
        Assert.Equal(new PixelRect(100, 43, 20, 2), SelectionMarks.BarFill(100, 50, 40, 1, 2, 1));
    }

    /// <summary>
    /// 미니맵 창은 화면 중심을 가운데 두고 월드 끝에서만 멈춘다. TEST01 녹화 37.5초: 화면 중심 (172.7, 201.2),
    /// 상자 78×74 → 가로 원점 약 94.7, 세로는 256 − 148 = 108 에 걸린다.
    /// </summary>
    [Fact]
    public void Origin_FollowsViewCenterAndClampsToWorld()
    {
        Assert.Equal(94.7, MiniMapLayout.Origin(172.7, 78, 256), 3);
        Assert.Equal(108, MiniMapLayout.Origin(201.2, 74, 256), 3);
        Assert.Equal(0, MiniMapLayout.Origin(10, 78, 256), 3);
        Assert.Equal(100, MiniMapLayout.Origin(250, 78, 256), 3);
    }

    /// <summary>창이 월드보다 크면 원점은 0이다.</summary>
    [Fact]
    public void Origin_WindowLargerThanWorld_IsZero()
    {
        Assert.Equal(0, MiniMapLayout.Origin(50, 200, 256), 3);
    }

    /// <summary>1픽셀 = 2칸으로 서로 바꾸며, 상자 밖 픽셀은 상자 끝 값으로 당긴다.</summary>
    [Fact]
    public void PixelAndCell_ConvertAtTwoCellsPerPixel()
    {
        Assert.Equal(2, MiniMapLayout.CellsPerPixel);
        Assert.Equal(34, MiniMapLayout.ToPixel(162, 94), 3);
        Assert.Equal(162, MiniMapLayout.ToCell(34, 94, 78), 3);
        Assert.Equal(94, MiniMapLayout.ToCell(-20, 94, 78), 3);
        Assert.Equal(94 + 156, MiniMapLayout.ToCell(500, 94, 78), 3);
    }

    /// <summary>화면 중심이 창 가운데에 올 때 중심 칸은 상자 한가운데 픽셀이 된다 (흰 사각형이 상자 중심에 놓인다).</summary>
    [Fact]
    public void ViewCenter_MapsToBoxCenter_WhenNotClamped()
    {
        double origin = MiniMapLayout.Origin(128, 78, 256);
        Assert.Equal(39, MiniMapLayout.ToPixel(128, origin), 3);
    }
}
