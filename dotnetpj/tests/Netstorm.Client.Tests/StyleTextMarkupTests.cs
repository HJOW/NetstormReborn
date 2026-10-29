using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 정규식 검색/판본/현재 타입/옵션 계약을 고정 입력에 대조한다.

/// <summary>cpppj/src/client/StyleTextMarkup.cpp의 정적 계약 검사.원본 실행 대조/L1로 표시하지 않는다.</summary>
public sealed class StyleTextMarkupTests
{
    /// <summary>실제 한 줄 StyleText 파서의 기존 글꼴/파생 훅을 유지하며 태그·그림·끝 빈 조각까지 연결한다.</summary>
    [Fact]
    public void ActualSingleLineParserKeepsHooksAndFragmentTags()
    {
        var s = new StyleMarkupScene(); Assert.NotNull(s.Scene.StyleHooks.TextMeasure); Assert.NotNull(s.Scene.StyleHooks.ExternalDraw); s.Scene.Style.Parse(ref s.Head, "A~[T 42]B~[I Unit.1]C~[E -9.2]"u8, s.Scene.Context); uint[] chain = StyleTextTestSupport.Chain(s.Scene.Fragments, s.Head); Assert.Equal(5, chain.Length); Assert.Equal(new uint[] { 0, 42, 42, 42, 42 }, chain.Select(t => s.Scene.Fragments.Fragments[t].Metadata)); Assert.Equal(1U, s.Pictures.Pictures[chain[2]].Frame); Assert.Equal(new[] { "A", "B", "C", "" }, chain.Where(t => t != chain[2]).Select(t => StyleTextTestSupport.Text(s.Scene.Fragments.Fragments[t]))); var last = s.Scene.Fragments.Fragments[chain[4]]; Assert.Equal((2U, 9U, unchecked((uint)-9), 2U), (last.Context[0], last.Context[5], last.Context[6], last.Context[7]));
    }
    /// <summary>앵커 없는 부분 검색과 임의 한BYTE 구분자,greedy 캡처/되돌림을 고정 문자열로 검사한다.</summary>
    [Theory]
    [InlineData("prefix S . thunder suffix", "thunder")]
    [InlineData("S thunder", "hunder")]
    [InlineData("S   .   thunder", "thunder")]
    [InlineData("S\nthunder", "thunder")]
    [InlineData("S . a\tb", "a\tb")]
    public void PartialSearchWildcardAndSpaceOnly(string command, string expected)
    { var s = new StyleMarkupScene(); s.Apply(command); Assert.Equal(new[] { expected }, s.Sounds); }

    /// <summary>높은 우선순위의 무효 그림/비활성 소리도 일치하면 다음 태그 명령을 소비하지 않는다.</summary>
    [Fact]
    public void MatchPriorityConsumesDisabledOrInvalidCommands()
    {
        var s = new StyleMarkupScene(); s.Metadata = 11; s.Apply("S . thunder T 42 I Missing.0"); Assert.Equal(0U, s.Head); Assert.Empty(s.Sounds); Assert.Equal(11U, s.Metadata); s.State.AllowSounds = 0; s.Apply("T 42 S . thunder"); Assert.Empty(s.Sounds); Assert.Equal(11U, s.Metadata); s.Apply("prefixT4294967297suffix"); Assert.Equal(1U, s.Metadata); s.Apply("T -7"); Assert.Equal(1U, s.Metadata); s.Apply("T 9\0T 77"); Assert.Equal(9U, s.Metadata);
    }
    /// <summary>CD 한 줄은 I/T만 처리하며 이름/숫자 입력의 비ASCII BYTE도 유지한다.</summary>
    [Fact]
    public void CdCommandsAndByteNameCapture()
    {
        var cd = new StyleMarkupScene(OriginalEdition.Cd1072); cd.Apply("S . thunder C . 7 T 33"); Assert.Empty(cd.Sounds); Assert.Equal(33U, cd.Metadata); Assert.Equal(171U, cd.Scene.Context[2]); var s = new StyleMarkupScene(); string? name = null; s.Hooks.FindType = n => { name = n; return -1; }; s.Markup.Apply([73, 32, 233, 85, 46, 48], s.Scene.Context, ref s.Head, ref s.Metadata); Assert.Equal("éU", name);
    }
    /// <summary>80바이트 캡처 한도와 프레임 지역 배열의 NUL 경계를 구분한다.</summary>
    [Fact]
    public void CaptureLimitAndFrameLocalBoundary()
    {
        var s = new StyleMarkupScene(); s.Apply("S . " + new string('x', 100)); Assert.Equal(80, Assert.Single(s.Sounds).Length); string? name = null; s.Hooks.FindType = n => { name = n; return -1; }; s.Apply("I " + new string('x', 100) + ".0"); Assert.Equal(80, name!.Length); Assert.Throws<ArgumentException>(() => s.Apply("I 2." + new string('1', 80))); Assert.Empty(s.Pictures.Pictures); s.Apply("I 2." + new string('0', 79)); Assert.Equal(0U, s.Pictures.Pictures[s.Head].Frame);
    }
    /// <summary>원본 타입/글자/특수 프레임과 count까지의 클램프를 고정 결과로 검사한다.</summary>
    [Theory]
    [InlineData("I Unit.*", 2U, false, false)]
    [InlineData("I 2.#5", 2U, true, false)]
    [InlineData("I 2.&", 2U, true, true)]
    [InlineData("I 2.A1", 4U, false, false)]
    [InlineData("I 2.B-1", 5U, false, false)]
    [InlineData("I 2.C9", 0U, false, false)]
    [InlineData("I 2.-9", 0U, false, false)]
    [InlineData("I 2.99", 8U, false, false)]
    [InlineData("I 2.A2147483647", 0U, false, false)]
    [InlineData("I 2A1", 1U, false, false)]
    public void PictureSelectionFramesAndFit(string command, uint frame, bool fit, bool mapped)
    {
        var s = new StyleMarkupScene(); s.State.FitWidth = 15; s.State.FitHeight = 10; s.Metadata = 91; s.Apply(command); var p = s.Pictures.Pictures[s.Head]; Assert.Equal((2U, frame, fit ? .5f : 1f, mapped ? (byte)1 : (byte)0), (p.Type, p.Frame, p.Scale, p.Mapped)); Assert.Equal(91U, s.Scene.Fragments.Fragments[s.Head].Metadata); Assert.Equal(0U, s.Scene.Fragments.Fragments[s.Head].Next);
    }
    /// <summary>패치는 gump 선택 필드를 다시 읽고 CD는 첫 값 사본을 사용한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 4U, 4)]
    [InlineData(OriginalEdition.Cd1072, 2U, 2)]
    public void EditionCurrentTypeReads(OriginalEdition edition, uint expected, int expectedCalls)
    {
        var s = new StyleMarkupScene(edition); int calls = 0; s.Hooks.Type = _ => new() { GumpFrame = ++calls == 1 ? 2 : 4, FrameCount = 20 }; s.Apply("I 2.*"); Assert.Equal(expected, s.Pictures.Pictures[s.Head].Frame); Assert.Equal(expectedCalls, calls);
    }
    /// <summary>전경/배경/그림자 허용과 한 축만 허용 범위인 E,현재 검정색/기본 옵션 유지 규칙.</summary>
    [Fact]
    public void OptionsPaletteOffsetAndConfigReadOrder()
    {
        var s = new StyleMarkupScene(); var palette = new PaletteColorTable(); palette.Basic[0] = 17; StyleTextMarkup.BindStyleTextMarkupPalette(s.Hooks, palette); s.Apply("C . -7"); s.Apply("B . 4294967297"); s.Apply("E -9.2"); Assert.Equal(unchecked((uint)-7), s.Scene.Context[2]); Assert.Equal(17U, s.Scene.Context[5]); Assert.Equal((2U, unchecked((uint)-9), 2U), (s.Scene.Context[0], s.Scene.Context[6], s.Scene.Context[7])); palette.Basic[0] = 23; s.Apply("E 3\n4"); Assert.Equal((23U, 3U, 4U), (s.Scene.Context[5], s.Scene.Context[6], s.Scene.Context[7])); s.Apply("E 99.99"); Assert.Equal(23U, s.Scene.Context[5]);
        var keys = new List<string>(); StyleTextMarkup.LoadStyleTextMarkupOptions(s.State, key => { keys.Add(key); return key == "allowzonesounds" ? "-2" : key == "zonefgcolor" ? "0" : null; }); Assert.Equal(new[] { "allowzonesounds", "zonebgcolor", "zonefgcolor", "zoneoffset" }, keys); Assert.Equal((-2, 1, 0, 1), (s.State.AllowSounds, s.State.AllowBackgroundColor, s.State.AllowForegroundColor, s.State.AllowOffset)); s.Apply("C . 88"); Assert.Equal(unchecked((uint)-7), s.Scene.Context[2]); var args = new List<(string, int, int, int, int, int)>(); StyleTextMarkup.BindStyleTextMarkupSound(s.Hooks, (n, l, v, p, r, m) => args.Add((n, l, v, p, r, m))); s.Apply("S . thunder"); Assert.Equal(("thunder", 0, 0, 0, 0, 0), Assert.Single(args));
    }
    /// <summary>다중 줄은 접두를 소비한 뒤 남은#만 맞춤으로 읽으며 &는 특수 문자가 아니다.</summary>
    [Theory]
    [InlineData("I 2.#", 2U, 0)]
    [InlineData("I 2.##", 2U, 65)]
    [InlineData("I 2.*#", 2U, 65)]
    [InlineData("I 2.A#", 3U, 65)]
    [InlineData("I 2.&", 0U, 0)]
    [InlineData("I 2.99", 8U, 0)]
    public void ScrollingSuffixRules(string command, uint frame, int width)
    { var s = new StyleMarkupScene(); s.ApplyScrolling(command); Assert.Equal((2U, frame, width, width == 0 ? 0 : 60), Assert.Single(s.Requests)); }
    /// <summary>높이 문턱은 패치16/CD12이며 null은 무자산,음수WORD는 허용된다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 12, true)]
    [InlineData(OriginalEdition.Patch1078, 16, true)]
    [InlineData(OriginalEdition.Patch1078, 17, false)]
    [InlineData(OriginalEdition.Cd1072, 12, true)]
    [InlineData(OriginalEdition.Cd1072, 13, false)]
    [InlineData(OriginalEdition.Cd1072, -1, true)]
    [InlineData(OriginalEdition.Patch1078, -32769, false)]
    public void RestrictedHeightThreshold(OriginalEdition edition, int height, bool allowed)
    { var s = new StyleMarkupScene(edition); s.Hooks.FrameHeight = (_, _) => height == -32769 ? null : (short)height; s.ApplyScrolling("I 2.1", true); if (allowed) Assert.Equal((2U, 1U, 99999, 12), Assert.Single(s.Requests)); else Assert.Empty(s.Requests); }
    /// <summary>패치의 숫자 타입 상한은 이름 조회 결과와 구별하며 제한 프레임은 count를 거부한다.</summary>
    [Fact]
    public void ScrollingNumericLimitAndRestrictedCount()
    {
        var s = new StyleMarkupScene(); s.Hooks.TypeCount = () => 2; s.ApplyScrolling("I 2.0"); Assert.Empty(s.Requests); s.ApplyScrolling("I Unit.0"); Assert.Single(s.Requests); s.Requests.Clear(); s.Hooks.TypeCount = () => 100; s.ApplyScrolling("I 2.99", true); Assert.Empty(s.Requests); var cd = new StyleMarkupScene(OriginalEdition.Cd1072); cd.Hooks.TypeCount = () => 2; cd.ApplyScrolling("I 2.0"); Assert.Single(cd.Requests);
    }
    /// <summary>CD 다중 줄 W만 허용 전역을 무시하고 첫 캡처 소리를 재생한다.패치 W는 소비하지 않는다.</summary>
    [Fact]
    public void CdScrollingSoundAndUnavailableBoundaries()
    {
        var cd = new StyleMarkupScene(OriginalEdition.Cd1072); cd.State.AllowSounds = 0; cd.ApplyScrolling("W thunder.ignored T 99"); Assert.Equal(new[] { "thunder" }, cd.Sounds); Assert.Equal(0U, cd.Metadata); var s = new StyleMarkupScene(); s.ApplyScrolling("W thunder.ignored T 99"); Assert.Empty(s.Sounds); Assert.Equal(99U, s.Metadata); s.Hooks.Sound = null; Assert.Throws<ArgumentException>(() => s.Apply("S . thunder")); s.State.AllowSounds = 0; s.Apply("S . thunder"); s.Hooks.Black = null; s.Apply("E 99.99"); Assert.Throws<ArgumentException>(() => s.Apply("E 1.1")); Assert.Equal(2U, s.Scene.Context[0] & 2);
    }
}
