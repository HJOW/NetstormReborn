using Netstorm.Assets;

namespace Netstorm.Assets.Tests;

/// <summary>
/// 제자리 애니메이션 프레임 규칙과 기준점 이동, 글 사이 그림 표기를 검사한다.
/// 기대값은 2026-10-03 TEST01 녹화를 원본 스프라이트와 맞춰 본 결과다 (docs/videos/test01-visuals-20261003.md).
/// </summary>
public class StructureAnimationTests
{
    /// <summary>가이저처럼 증기 프레임(A00~A19)과 baseframe 기본 프레임(B00)을 가진 시험용 타입.</summary>
    private static readonly TypeDefinition Geyser = TypeDefinition.Parse(
        "typename geyser\ntypeflags geyser shadow;\n{\n\thotFootRatioX = 0.22;\n\thotFootRatioY = 0.22;\n}\n"
        + string.Concat(Enumerable.Range(0, 20).Select(i => $"A{i:00} : : \"g.gif\" #{i} ;\n"))
        + "B00 : default baseframe : \"g.gif\" #20 ;\nP00 : help : \"b.gif\" #0 ;\n");

    /// <summary>Rain 워크샵: 레벨마다 본체 01 과 겹침 조각 02·03.</summary>
    private static readonly TypeDefinition RainFactory = TypeDefinition.Parse(
        "typename rainFactory\ntypeflags factory shadow;\n{\n\ttheme = \"rain\";\n}\n"
        + "A01 : default baseframe : \"f.gif\" #0 ;\nA02 : : \"f.gif\" #1 ;\nA03 : : \"f.gif\" #2 ;\n"
        + "B01 : : \"f.gif\" #3 ;\nB02 : : \"f.gif\" #4 ;\nB03 : : \"f.gif\" #5 ;\n"
        + "C01 : : \"f.gif\" #6 ;\nC02 : : \"f.gif\" #7 ;\nC03 : : \"f.gif\" #8 ;\nP00 : help : \"b.gif\" #0 ;\n");

    /// <summary>Sun 워크샵: 겹침 조각이 있어도 돌리지 않는다.</summary>
    private static readonly TypeDefinition SunFactory = TypeDefinition.Parse(
        "typename sunFactory\ntypeflags factory shadow;\n{\n\ttheme = \"sun\";\n}\n"
        + "A01 : default baseframe : \"f.gif\" #0 ;\nA02 : : \"f.gif\" #1 ;\nB01 : : \"f.gif\" #2 ;\nC01 : : \"f.gif\" #3 ;\nC02 : : \"f.gif\" #4 ;\n");

    /// <summary>경과 시간이 프레임 길이만큼 지날 때마다 순번이 하나씩 오르고 끝에서 처음으로 돌아간다.</summary>
    [Theory]
    [InlineData(0.0, 0)]
    [InlineData(0.99, 0)]
    [InlineData(1.0, 1)]
    [InlineData(2.5, 2)]
    [InlineData(3.0, 0)]
    [InlineData(-5.0, 0)]
    public void LoopIndex_WrapsByFrameLength(double seconds, int expected)
    {
        Assert.Equal(expected, StructureAnimation.LoopIndex(seconds, 1.0, 3));
    }

    /// <summary>프레임 수나 길이가 0 이하이면 0번 프레임이다 (0으로 나누지 않는다).</summary>
    [Fact]
    public void LoopIndex_EmptyOrZeroLength_IsZero()
    {
        Assert.Equal(0, StructureAnimation.LoopIndex(10, 1.0, 0));
        Assert.Equal(0, StructureAnimation.LoopIndex(10, 0, 5));
    }

    /// <summary>가이저는 A00~A16 의 17장만 약 24Hz 로 되풀이한다 (A17 이후와 기본 B00 은 쓰지 않는다).</summary>
    [Fact]
    public void Geyser_LoopsFirstSeventeenSteamFrames()
    {
        double frame = StructureAnimation.SteamFrameSeconds;
        Assert.Equal(0, StructureAnimation.GeyserFrame(Geyser, 0));
        Assert.Equal(1, StructureAnimation.GeyserFrame(Geyser, frame * 1.5));
        Assert.Equal(16, StructureAnimation.GeyserFrame(Geyser, frame * 16.5));
        Assert.Equal(0, StructureAnimation.GeyserFrame(Geyser, frame * 17.5));
    }

    /// <summary>증기 그림이 없는 타입은 기본 프레임을 쓴다.</summary>
    [Fact]
    public void Geyser_WithoutSteamFrames_UsesDefault()
    {
        TypeDefinition plain = TypeDefinition.Parse("typename g\n{\n}\nB00 : : \"g.gif\" #0 ;\nB01 : default : \"g.gif\" #1 ;\n");
        Assert.Equal(1, StructureAnimation.GeyserFrame(plain, 3));
    }

    /// <summary>워크샵 본체는 레벨 1·2·3 → A01·B01·C01 이고 범위 밖 레벨은 가까운 값으로 맞춘다.</summary>
    [Theory]
    [InlineData(1, 0)]
    [InlineData(2, 3)]
    [InlineData(3, 6)]
    [InlineData(0, 0)]
    [InlineData(9, 6)]
    public void Factory_BodyFollowsLevel(int level, int expectedBody)
    {
        Assert.Equal(expectedBody, StructureAnimation.FactoryFrames(RainFactory, level, 0).Body);
    }

    /// <summary>Rain 워크샵은 "조각 없음 → 02 → 03" 을 0.5초마다 되풀이한다 (1.5초 주기).</summary>
    [Fact]
    public void RainFactory_CyclesOverlayEveryHalfSecond()
    {
        Assert.Null(StructureAnimation.FactoryFrames(RainFactory, 2, 0.1).Overlay);
        Assert.Equal(4, StructureAnimation.FactoryFrames(RainFactory, 2, 0.6).Overlay);
        Assert.Equal(5, StructureAnimation.FactoryFrames(RainFactory, 2, 1.1).Overlay);
        Assert.Null(StructureAnimation.FactoryFrames(RainFactory, 2, 1.6).Overlay);
        Assert.Equal(3, StructureAnimation.FactoryFrames(RainFactory, 2, 1.1).Body);
    }

    /// <summary>Sun 워크샵은 겹침 조각을 그리지 않는다 (녹화 내내 창 조각이 보이지 않았다).</summary>
    [Fact]
    public void SunFactory_NeverShowsOverlay()
    {
        // 여러 시각에서 본체만 나오는지 본다
        for (double seconds = 0; seconds < 3; seconds += 0.25)
        {
            StructureFrames frames = StructureAnimation.FactoryFrames(SunFactory, 3, seconds);
            Assert.Equal(3, frames.Body);
            Assert.Null(frames.Overlay);
        }
    }

    /// <summary>기본 클러스터에 baseframe 이 있는 신전은 본체 위에 다른 글자 묶음을 겹쳐 돌린다 (Wind: B01 + A00~).</summary>
    [Fact]
    public void Vortex_WithBaseFrame_OverlaysOtherSide()
    {
        TypeDefinition wind = TypeDefinition.Parse("typename windVortex\ntypeflags vortex;\n{\n}\n"
            + "A00 : : \"v.gif\" #0 ;\nA01 : : \"v.gif\" #1 ;\nA02 : : \"v.gif\" #2 ;\nB01 : default baseframe : \"v.gif\" #3 ;\nC00 : : \"v.gif\" #4 ;\n");
        double frame = StructureAnimation.SwirlFrameSeconds;
        Assert.Equal(new StructureFrames(3, 0), StructureAnimation.VortexFrames(wind, 0));
        Assert.Equal(new StructureFrames(3, 2), StructureAnimation.VortexFrames(wind, frame * 2.5));
        Assert.Equal(new StructureFrames(3, 0), StructureAnimation.VortexFrames(wind, frame * 3.5));
    }

    /// <summary>baseframe 이 없는 신전(Thunder A00)은 본체 자체를 B 묶음으로 바꿔 돌린다.</summary>
    [Fact]
    public void Vortex_WithoutBaseFrame_ReplacesBody()
    {
        TypeDefinition thunder = TypeDefinition.Parse("typename thunderVortex\ntypeflags vortex;\n{\n}\n"
            + "A00 : default : \"v.gif\" #0 ;\nB00 : : \"v.gif\" #1 ;\nB01 : : \"v.gif\" #2 ;\nC00 : : \"v.gif\" #3 ;\n");
        double frame = StructureAnimation.SwirlFrameSeconds;
        Assert.Equal(new StructureFrames(1), StructureAnimation.VortexFrames(thunder, 0));
        Assert.Equal(new StructureFrames(2), StructureAnimation.VortexFrames(thunder, frame * 1.5));
        Assert.Equal(new StructureFrames(1), StructureAnimation.VortexFrames(thunder, frame * 2.5));
    }

    /// <summary>풍선은 A 묶음이 여러 장일 때만 돌리고, 한 장뿐이면 호출한 쪽이 기본 프레임을 쓰도록 null 이다.</summary>
    [Fact]
    public void Balloon_LoopsOnlyWhenSeveralFrames()
    {
        TypeDefinition floater = TypeDefinition.Parse("typename rainBalloon\ntypeflags balloon flyershadow;\n{\n}\n"
            + "A00 : default : \"b.gif\" #0 ;\nA01 : : \"b.gif\" #1 ;\nA02 : : \"b.gif\" #2 ;\nP00 : help : \"h.gif\" #0 ;\n");
        TypeDefinition single = TypeDefinition.Parse("typename sunBalloon\ntypeflags balloon flyershadow;\n{\n}\n"
            + "A00 : gumpframe default : \"b.gif\" #0 ;\nP00 : help : \"h.gif\" #0 ;\n");
        Assert.Equal(2, StructureAnimation.BalloonFrame(floater, StructureAnimation.SwirlFrameSeconds * 2.5));
        Assert.Null(StructureAnimation.BalloonFrame(single, 1));
    }

    /// <summary>
    /// 같은 이름 클러스터를 방향별 4장씩 둔 수송 유닛: 움직인 적이 없으면 0번, 서쪽은 4번 묶음, 이동 중에는 묶음 안을 돈다.
    /// 걷기 묶음(A~H)이 있는 타입에는 적용하지 않는다.
    /// </summary>
    [Fact]
    public void Sail_UsesHeadingGroups()
    {
        TypeDefinition skater = TypeDefinition.Parse("typename windwalker\ntypeflags walker shadow;\n{\n}\n"
            + string.Concat(Enumerable.Range(0, 20).Select(i => $"A01 : {(i == 5 ? "default" : "")} : \"w.gif\" #{i} ;\n")));
        Assert.Equal(0, StructureAnimation.SailFrame(skater, -1, false, 5));
        Assert.Equal(4, StructureAnimation.SailFrame(skater, 6, false, 5));
        Assert.Equal(8, StructureAnimation.SailFrame(skater, 4, false, 5));
        Assert.Equal(12 + 2, StructureAnimation.SailFrame(skater, 2, true, StructureAnimation.SwirlFrameSeconds * 2.5));
        Assert.Null(StructureAnimation.SailFrame(Geyser, 0, false, 0));
    }

    /// <summary>hotFootRatio × 한 칸 크기만큼 왼쪽·위로 옮기고 소수점은 버린다 (녹화의 네 타입).</summary>
    [Theory]
    [InlineData(0.22, 0.22, -3, -2)]
    [InlineData(0.5, 0.4, -8, -4)]
    [InlineData(0.5, 0.2, -8, -2)]
    [InlineData(1.0, 0.7, -16, -7)]
    public void HotFootShift_MovesLeftAndUp(double ratioX, double ratioY, int expectedX, int expectedY)
    {
        string x = ratioX.ToString(System.Globalization.CultureInfo.InvariantCulture);
        string y = ratioY.ToString(System.Globalization.CultureInfo.InvariantCulture);
        TypeDefinition type = TypeDefinition.Parse($"typename t\n{{\n\thotFootRatioX = {x};\n\thotFootRatioY = {y};\n}}\nA00 : : \"a\" #0 ;\n");
        Assert.Equal((expectedX, expectedY), StructureAnimation.HotFootShift(type, 16, 11));
    }

    /// <summary>비율이 없는 타입(건물)은 옮기지 않는다.</summary>
    [Fact]
    public void HotFootShift_WithoutRatio_IsZero()
    {
        Assert.Equal((0, 0), StructureAnimation.HotFootShift(SunFactory, 16, 11));
    }

    /// <summary>글 사이 그림의 프레임 표기: 숫자는 클러스터 번호, 글자 + 숫자는 클러스터 이름, 못 찾으면 기본 프레임.</summary>
    [Theory]
    [InlineData("3", 3)]
    [InlineData("B00", 20)]
    [InlineData("a5", 5)]
    [InlineData("a99", 20)]
    [InlineData("*", 20)]
    [InlineData("500", 20)]
    public void InlinePicture_ResolvesFrame(string frameName, int expected)
    {
        Assert.Equal(expected, InlinePicture.Frame(Geyser, frameName));
    }
}
