namespace Netstorm.Assets.Tests;

/// <summary>생산 창(덱) 항목의 어둡게·빨갛게 색 변환표 테스트 (원본 FUN_0043c010 규칙)</summary>
public sealed class ProductionTintRemapTests
{
    /// <summary>시험 팔레트: 0 검정, 1 기준색, 2 기준색 × 0.6, 3 기준색의 R 만 키운 색, 나머지는 흰색</summary>
    private static Palette TestPalette()
    {
        var colors = new Rgb[Palette.ColorCount];
        // 쓰지 않는 칸은 어느 목표와도 멀도록 흰색으로 채운다
        for (int i = 0; i < colors.Length; i++) colors[i] = new Rgb(255, 255, 255);
        colors[0] = new Rgb(0, 0, 0);
        colors[1] = new Rgb(200, 100, 50);
        colors[2] = new Rgb(120, 60, 30);
        colors[3] = new Rgb(255, 100, 50);
        return new Palette(colors);
    }

    /// <summary>어둡게 표는 RGB 0.6배 색, 빨갛게 표는 R 2.1배(최대 1) 색에 가장 가까운 팔레트 인덱스를 고른다</summary>
    [Fact]
    public void Remaps_PickNearestTransformedColor()
    {
        Palette palette = TestPalette();
        byte[] dark = ProductionTintRemap.Darkened(palette);
        byte[] red = ProductionTintRemap.Reddened(palette);
        Assert.Equal(2, dark[1]);
        Assert.Equal(3, red[1]);
        // 검정은 그대로 검정이고, 이미 R 이 최대인 색은 빨갛게 해도 자기 자신이다
        Assert.Equal(0, dark[0]);
        Assert.Equal(0, red[0]);
        Assert.Equal(3, red[3]);
    }

    /// <summary>거리가 같은 후보가 여럿이면 원본처럼 앞선 인덱스를 고른다 (흰색 칸은 4번이 처음)</summary>
    [Fact]
    public void Remaps_KeepFirstIndexOnTies()
    {
        byte[] red = ProductionTintRemap.Reddened(TestPalette());
        Assert.Equal(4, red[255]);
    }

    /// <summary>원본 게임 팔레트에서도 256칸 표를 만들고, 빨갛게 표는 R 을 줄이지 않는다</summary>
    [Fact]
    public void Remaps_OriginalPalette()
    {
        Palette palette = Palette.Load(OriginalData.RequireFile("d/" + Palette.GameCol));
        byte[] dark = ProductionTintRemap.Darkened(palette);
        byte[] red = ProductionTintRemap.Reddened(palette);
        Assert.Equal(Palette.ColorCount, dark.Length);
        Assert.Equal(Palette.ColorCount, red.Length);
        // 대부분의 색은 어둡게 하면 밝기가 줄고, 빨갛게 하면 R 이 줄지 않는다
        int darker = Enumerable.Range(0, Palette.ColorCount).Count(i => Sum(palette[dark[i]]) <= Sum(palette[i]));
        Assert.True(darker >= 250, $"어두워진 색 {darker}개");
        int redder = Enumerable.Range(0, Palette.ColorCount).Count(i => palette[red[i]].R + 16 >= palette[i].R);
        Assert.True(redder >= 250, $"R 이 유지·증가한 색 {redder}개");
    }

    /// <summary>RGB 합 (밝기 비교용)</summary>
    private static int Sum(Rgb color) => color.R + color.G + color.B;
}
