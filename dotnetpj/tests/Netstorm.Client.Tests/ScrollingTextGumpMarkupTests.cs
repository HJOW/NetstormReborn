using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 구간별 실제 추가/현재 태그·문맥·제한과 원본 픽셀 경계를 검사한다.

/// <summary>cpppj 다중 줄 파서의 정적 계약과 실제 버퍼/글꼴/SHP 출력 통합.독립 원본 실행 자료는 아니다.</summary>
public sealed class ScrollingTextGumpMarkupTests
{
    /// <summary>각 일반/리터럴 구간만 실제 버퍼 통지/재그리기를 수행하며 태그는 호출 간 유지된다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void SegmentNotificationsLiteralAndPersistentTags(OriginalEdition edition)
    {
        var s = new StyleMarkupScene(edition); int notices = 0; s.Scene.State.OnAppend = _ => notices++; s.Parser.Parse(2, "A~[T 7]A\n~~?"u8); Assert.Equal(4, notices); Assert.Equal(4, s.Scene.Events.Count(e => e == "redraw:2,False")); Assert.Equal(new[] { "A", "A" }, s.Scene.Texts(s.Scene.State.HardLines[0])); Assert.Equal(new[] { "~", "?" }, s.Scene.Texts(s.Scene.State.HardLines[1])); Assert.Equal(7U, s.Scene.Node.Metadata); uint first = s.Scene.State.HardLines[0]; Assert.Equal((0U, 7U), (s.Scene.Fragments.Fragments[first].Metadata, s.Scene.Fragments.Fragments[s.Scene.Fragments.Fragments[first].Next].Metadata)); s.Parser.Parse(2, "A\n\0ignored"u8); Assert.Equal(2, s.Scene.State.LineCount); Assert.Equal(new[] { "~", "?", "A" }, s.Scene.Texts(s.Scene.State.HardLines[1]));
    }
    /// <summary>알 수 없는 명령은~만 소비하고 미닫힌 대괄호는~[만 소비한다.끝~도 마지막 빈 입력을 통지한다.</summary>
    [Fact]
    public void UnknownUnclosedAndTrailingCommands()
    {
        var s = new StyleMarkupScene(); s.Parser.Parse(2, "a~?b~[open"u8); Assert.Equal(new[] { "a", "?b", "open" }, s.Scene.Texts(s.Scene.State.HardLines[0])); int notices = 0; s.Scene.State.OnAppend = bytes => { notices++; Assert.Equal(new byte[] { 0 }, bytes.ToArray()); }; s.Parser.Parse(2, "~"u8); Assert.Equal(1, notices); Assert.Equal(1U, s.Scene.Node.TextFlags & 1); Assert.Equal(1U, s.Scene.State.Dirty);
    }
    /// <summary>추가 통지가 바꾼 현재 태그는 다음 구간에,현재 기본 문맥은~. 복귀 뒤에 적용한다.</summary>
    [Fact]
    public void CurrentMetadataAndDefaultContextAfterNotice()
    {
        var s = new StyleMarkupScene(); s.Scene.Node.Metadata = 1; s.Scene.State.OnAppend = _ => { s.Scene.Node.Metadata = 9; s.Scene.Node.TextContext[2] = 77; }; s.Parser.Parse(2, "A~.A\n"u8); uint first = s.Scene.State.HardLines[0], second = s.Scene.Fragments.Fragments[first].Next; Assert.Equal((1U, 171U), (s.Scene.Fragments.Fragments[first].Metadata, s.Scene.Fragments.Fragments[first].Context[2])); Assert.Equal((9U, 77U), (s.Scene.Fragments.Fragments[second].Metadata, s.Scene.Fragments.Fragments[second].Context[2]));
    }
    /// <summary>소리 콜백 뒤 현재 태그와~.의 현재 문맥 재조회가 실제 후속 조각에 반영된다.</summary>
    [Fact]
    public void SoundCallbacksUpdateFollowingSegments()
    {
        var s = new StyleMarkupScene(); s.Hooks.Sound = _ => { s.Scene.Node.Metadata = 19; s.Scene.Node.TextContext[2] = 66; }; s.Parser.Parse(2, "~[S . bell]A~.A\n"u8); uint[] chain = StyleTextTestSupport.Chain(s.Scene.Fragments, s.Scene.State.HardLines[0]); Assert.Equal(new uint[] { 19, 19 }, chain.Select(t => s.Scene.Fragments.Fragments[t].Metadata)); Assert.Equal(new uint[] { 171, 66 }, chain.Select(t => s.Scene.Fragments.Fragments[t].Context[2]));
    }
    /// <summary>그림 선택 콜백 뒤 제한 필드/태그를 읽고 마지막 빈 구간도 통지한다.</summary>
    [Fact]
    public void RestrictedPictureReadsCurrentCallbackAndMetadata()
    {
        var s = new StyleMarkupScene(); s.Hooks.Type = _ => { s.Scene.Node.Callback = 1; s.Scene.Node.Metadata = 44; return s.Fields; }; s.Hooks.FrameHeight = (_, _) => 17; int notices = 0; s.Scene.State.OnAppend = _ => notices++; s.Parser.Parse(2, "~[I 2.1]"u8); Assert.Empty(s.Pictures.Pictures); Assert.Equal(44U, s.Scene.Node.Metadata); Assert.Equal(1, notices); s.Hooks.FrameHeight = (_, _) => 12; s.Parser.Parse(2, "~[I 2.1]"u8); uint p = s.Scene.State.HardLines[0]; Assert.Equal(.6f, s.Pictures.Pictures[p].Scale); Assert.Equal(44U, s.Scene.Fragments.Fragments[p].Metadata); Assert.Equal(2, notices);
    }
    /// <summary>입력은 콜백 이전 복사본이며 현재 기본/파생이 다른 저장소면 거부한다.</summary>
    [Fact]
    public void InputCopyAndSharedStorageContracts()
    {
        var s = new StyleMarkupScene(); byte[] input = "A~.A\n"u8.ToArray(); s.Scene.State.OnAppend = _ => input.AsSpan().Fill(90); s.Parser.Parse(2, input); Assert.Equal(new[] { "A", "A" }, s.Scene.Texts(s.Scene.State.HardLines[0])); var invalid = new ScrollingTextGumpMarkup(s.Scene.Scroll, s.Scene.Style, s.Picture, s.Markup, new() { Node = _ => s.Scene.Node, Base = _ => new ScrollingTextGumpNode() }); Assert.Throws<InvalidOperationException>(() => invalid.Parse(2, [])); Assert.Throws<InvalidOperationException>(() => s.Parser.Parse(99, []));
    }
    /// <summary>실제 대괄호 그림과 일반 글자를 같은 버퍼/색표/클립의 실제 SHP·글리프 픽셀로 출력한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ParsedPictureAndTextUseActualPixels(OriginalEdition edition)
    {
        var s = new StyleBufferScene(edition); var assets = new MarkupAssets(GumpBackgroundTestAssets.Shapes(), 90, [new(82, 0, 2, 0, 0)]); var h = new MarkupPixelTarget(new(s.Pixels, 32, 40, s.Clip), assets, s.Colors.VfxColors, new byte[256]).Hooks(); var state = new MarkupState(); MarkupAssets.BindMarkupAssets(state, h, assets); var pictures = new Markup(s.Fragments, state, h, edition); Markup.BindMarkup(s.StyleHooks, pictures); StyleTextBuffer.BindStyleTextBufferMarkup(s.BufferHooks, pictures);
        var markup = new StyleTextMarkup(s.Fragments, pictures, new(), new() { FindType = n => n == "Unit" ? 82 : -1, Type = _ => new() { FrameCount = 2 }, TypeCount = () => 90 }, edition); markup.Hooks(s.StyleHooks); var parser = new ScrollingTextGumpMarkup(s.Scroll, s.Style, pictures, markup, new() { Node = _ => s.Node, Base = _ => s.Node }); parser.Parse(2, "A~[T 9]~[I Unit.0]A\n"u8); s.Draw.Draw(2); uint[] chain = StyleTextTestSupport.Chain(s.Fragments, s.Rows()[0].Head); Assert.Equal(new uint[] { 0, 9, 9 }, chain.Select(t => s.Fragments.Fragments[t].Metadata)); Assert.Equal(171, s.Pixels.Span[34]); Assert.Equal(171, s.Pixels.Span[39]); Assert.Equal(new byte[] { 1, 2, 1 }, s.Pixels.Span.Slice(868, 3).ToArray()); Assert.Equal(4, s.Pixels.Span[901]); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]);
    }
}
