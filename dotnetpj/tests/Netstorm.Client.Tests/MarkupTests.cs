using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 그림 소유/배율/현재 헤더와 실제8비트 출력 계약을 검사한다.

/// <summary>cpppj Markup 정적 계약과 독립 SHP 입력의 픽셀 통합. 독립 원본 실행 기대값은 아니다.</summary>
public sealed class MarkupTests
{
    /// <summary>파생 복제는 타입/프레임/태그만 복사하며 scalar 반납 직전의 필드는 관측 가능하다.</summary>
    [Fact]
    public void CloneResetsScaleMappedAndScalarReleaseOrder()
    {
        var fragments = new StyleTextState(); var state = new MarkupState(); var markup = new Markup(fragments, state, new()); var events = new List<uint>(); var h = new StyleTextHooks { Release = t => { Assert.True(markup.Contains(t)); Assert.Equal(StyleTextFragmentKind.Base, fragments.Fragments[t].Kind); events.Add(t); } }; Markup.BindMarkup(h, markup); var style = new StyleText(fragments, h);
        uint original = markup.NewPicture(3, 4, 77); state.Pictures[original].Scale = .25f; markup.SetMapped(original, 0x1234); uint clone = markup.ClonePicture(original); Assert.Equal((3U, 4U, 1f, (byte)0), (state.Pictures[clone].Type, state.Pictures[clone].Frame, state.Pictures[clone].Scale, state.Pictures[clone].Mapped)); Assert.Equal((byte)0x34, state.Pictures[original].Mapped); Assert.Equal(77U, fragments.Fragments[clone].Metadata);
        style.Destroy(original, 0); Assert.True(markup.Contains(original)); Assert.Equal((0, 0), style.MeasureFragment(original)); Assert.Throws<ArgumentOutOfRangeException>(() => markup.ClonePicture(original)); style.Destroy(original, 1); Assert.Equal(new[] { original }, events); Assert.False(markup.Contains(original)); Assert.False(fragments.Fragments.ContainsKey(original)); style.Destroy(clone, 1); Assert.Empty(state.Pictures);
    }
    /// <summary>signed 타입/프레임을 검사하고 헤더 콜백 이후 현재 기준점의 하위WORD를 뺀다.</summary>
    [Fact]
    public void CurrentLookupAndWrappedHotspot()
    {
        var state = new MarkupState { TypeCount = 5 }; var fragments = new StyleTextState(); var type = new MarkupType { Token = 91, Shape = 55, FrameCount = 2 }; int calls = 0;
        var markup = new Markup(fragments, state, new() { Type = t => { Assert.Equal(3U, t); return type; }, Frame = (t, a, f) => { Assert.Equal((91U, 0U, 1U), (t, a, f)); calls++; type.HotspotX = 65538; type.HotspotY = -65537; return new(12, 20, 10, -4, -1, -2); } }); uint p = markup.NewPicture(3, 1); Assert.Equal(new MarkupFrameView(3, 55, new(12, 20, 8, -3, -1, -2)), markup.Lookup(p)); Assert.Equal(1, calls);
        state.Pictures[p].Frame = uint.MaxValue; Assert.Equal(default, markup.Lookup(p)); state.Pictures[p].Type = uint.MaxValue; Assert.Equal(default, markup.Lookup(p)); state.Pictures[p].Type = 5; Assert.Equal(default, markup.Lookup(p)); Assert.Equal(1, calls);
    }
    /// <summary>무자산0/0의 NaN 축소는 패치에서 저장하지 않고 CD에서 저장한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, false)]
    [InlineData(OriginalEdition.Cd1072, true)]
    public void EditionNanAndTruncation(OriginalEdition edition, bool nan)
    {
        var f = new StyleTextState(); var s = new MarkupState { TypeCount = 3 }; var h = new MarkupHooks(); var m = new Markup(f, s, h, edition); uint p = m.NewPicture(0, 0); m.Fit(p, 0, 0); Assert.Equal(nan, float.IsNaN(s.Pictures[p].Scale)); h.Type = _ => new() { Shape = 2, FrameCount = 1 }; h.Frame = (_, _, _) => new(7, 11, -3, -5, 0, 0); s.Pictures[p].Type = 2; s.Pictures[p].Scale = .5f; Assert.Equal((3, 5), m.Measure(p)); Assert.Equal((5, -2), m.Baseline(p)); m.Fit(p, 100, 100); Assert.Equal(.5f, s.Pictures[p].Scale); s.Pictures[p].Scale = float.PositiveInfinity; Assert.Equal((0, 0), m.Measure(p));
    }
    /// <summary>색표 설치로 바뀐 현재 프레임만 다시 읽고 저장한 SHP/위치/배율/치환 여부는 유지한다.</summary>
    [Fact]
    public void DrawReadsFrameAfterColorButKeepsComputedRequest()
    {
        var f = new StyleTextState(); var s = new MarkupState { TypeCount = 3 }; var requests = new List<MarkupShapeRequest>(); uint p = 0;
        var m = new Markup(f, s, new() { Type = _ => new() { Shape = 50, FrameCount = 1 }, Frame = (_, _, _) => new(3, 4, 2, -2, 0, 0), Color = () => { s.Pictures[p].Type = 0; s.Pictures[p].Frame = 1; s.Pictures[p].Scale = .25f; s.Pictures[p].Mapped = 0; }, Shape = r => requests.Add(r) }); p = m.NewPicture(2, 0); s.Pictures[p].Scale = .5f; m.SetMapped(p, 2); m.Draw(p, new(10, 10)); Assert.Equal(new MarkupShapeRequest(2, 50, 1, new(11, 9), true, true, 0, 32768, 32768), Assert.Single(requests));
    }
    /// <summary>그림 외 파생 가상 동작은 기존 경계를 보존하며 미연결 경계는 실제 도달 시 거부한다.</summary>
    [Fact]
    public void OtherExternalFallbacksArePreserved()
    {
        var f = new StyleTextState(); var m = new Markup(f, new(), new()); uint other = StyleText.RegisterStyleTextFragment(f, new() { Kind = StyleTextFragmentKind.External }); var events = new List<string>(); var h = new StyleTextHooks { ExternalMetrics = _ => (6, 3), ExternalMeasure = _ => (4, 5), ExternalDraw = (t, p, c) => events.Add("draw"), ExternalDestroy = (_, _) => events.Add("destroy"), Release = _ => events.Add("release") }; Markup.BindMarkup(h, m); var style = new StyleText(f, h); Assert.Equal((6, 3), style.Baseline(other)); Assert.Equal((4, 5), style.MeasureFragment(other)); style.DrawFragment(other, new()); style.Destroy(other, 1); Assert.Equal(new[] { "draw", "destroy", "release" }, events);
        var missing = new StyleTextHooks(); Markup.BindMarkup(missing, m); Assert.Throws<ArgumentException>(() => missing.ExternalMeasure!(99)); Assert.Throws<ArgumentException>(() => missing.ExternalDraw!(99, new(), null)); var buffer = new StyleTextBufferHooks { CloneExternal = _ => 123 }; StyleTextBuffer.BindStyleTextBufferMarkup(buffer, m); Assert.Equal(123U, buffer.CloneExternal!(other));
    }
    /// <summary>실제 SHP 논리 메타/물리 프레임을 구별하고 색 설치 뒤 추가 물리 프레임의 투명 픽셀을 합성한다.</summary>
    [Fact]
    public void RealShapeMetadataPhysicalFrameAndSharedColorPixels()
    {
        var assets = new MarkupAssets(GumpBackgroundTestAssets.Shapes(), 90, [new(82, 0, 1, 0, 0)]); Assert.Null(assets.Type(81)); Assert.Equal(new MarkupFrame(3, 3, 0, 0, 0, 0), assets.Frame(82, 0, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Frame(82, 0, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Frame(82, 1, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Shape(82, 2)); Assert.Equal(0, assets.Shape(82, 1).Block);
        byte[] pixels = Enumerable.Repeat((byte)7, 64).ToArray(), colors = new byte[256], pictureColors = Enumerable.Range(0, 256).Select(i => unchecked((byte)(i + 40))).ToArray(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); var h = new MarkupPixelTarget(new(pixels, 8, 8, clip), assets, colors, pictureColors).Hooks(); var f = new StyleTextState(); var state = new MarkupState(); MarkupAssets.BindMarkupAssets(state, h, assets); var m = new Markup(f, state, h); uint p = m.NewPicture(82, 0); m.Draw(p, new(2, 1)); Assert.Equal(new byte[] { 1, 2, 1 }, pixels.AsSpan(10, 3).ToArray()); pixels.AsSpan().Fill(7); m.SetMapped(p, 1); var install = h.Color; h.Color = () => { install!(); state.Pictures[p].Frame = 1; }; m.Draw(p, new(2, 1)); Assert.Equal(new byte[] { 45, 46, 45 }, pixels.AsSpan(10, 3).ToArray()); Assert.Equal(7, pixels[19]); Assert.Equal(44, colors[4]); Assert.Equal(7, pixels[0]); Assert.Equal(7, pixels[^1]);
    }
}
