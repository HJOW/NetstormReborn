using System.Runtime.CompilerServices;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 조각의 소유·참조 겹침·현재 조회 순서와 실제 출력 계약을 검사한다.

/// <summary>cpppj/src/client/StyleText.cpp의 정적 계약과 독립 입력 검사다. 원본 실행 기대값/L1 검사는 아니다.</summary>
public sealed class StyleTextTests
{
    /// <summary>복사/클론의+8 길이·미사용문맥·기본 소멸 뒤 동작과 실제 반납 순서.</summary>
    [Fact]
    public void CopiesCloneLengthAndDestructionOrder()
    {
        var state = new StyleTextState(); var released = new List<uint>(); var h = new StyleTextHooks { Release = t => { Assert.Equal(StyleTextFragmentKind.Base, state.Fragments[t].Kind); Assert.Empty(state.Fragments[t].Text); Assert.Equal(0U, state.Fragments[t].Next); released.Add(t); } }; var style = new StyleText(state, h);
        byte[] bytes = [65, 0, 66]; uint[] context = [2, 5, 7, 4, 9, 11, 1, 1]; uint original = style.NewText(bytes, context, 3, 2); bytes[0] = 90; context[2] = 99;
        Assert.Equal(new byte[] { 65, 0, 66, 0 }, state.Fragments[original].Text); Assert.Equal(7U, state.Fragments[original].Context[2]); uint clone = style.CloneText(original); Assert.Equal(new byte[] { 65, 0, 0 }, state.Fragments[clone].Text); Assert.Equal(0U, state.Fragments[clone].Metadata);
        uint empty = style.CloneText(clone); Assert.Equal(new byte[] { 0 }, state.Fragments[empty].Text); state.Fragments[original].Metadata = 5; Assert.Throws<ArgumentOutOfRangeException>(() => style.CloneText(original));
        style.Destroy(original, 0); Assert.Equal((0, 0), style.Baseline(original)); Assert.Equal((0, 0), style.MeasureFragment(original)); style.DrawFragment(original, new()); Assert.Empty(released); style.Destroy(original, 1); Assert.Equal(new[] { original }, released); Assert.False(state.Fragments.ContainsKey(original));
        Assert.Throws<ArgumentException>(() => style.NewText([65], new uint[8])); Assert.Throws<ArgumentOutOfRangeException>(() => style.NewText([65], new uint[8], 2));
    }

    /// <summary>자동 토큰은 틈/소진을 처리하고 체인 이전은source의next를 보존하며 같은 소유 주소는 거부한다.</summary>
    [Fact]
    public void TokensAppendTransferAndOwnershipFields()
    {
        var state = new StyleTextState(); var style = new StyleText(state, new()); uint head = 0, source = 0; uint a = style.NewText("a\0"u8, new uint[8]), b = style.NewText("b\0"u8, new uint[8]), c = style.NewText("c\0"u8, new uint[8]), d = style.NewText("d\0"u8, new uint[8]);
        style.Append(ref head, a); style.Append(ref head, b); style.Append(ref source, c); style.Append(ref source, d); style.Transfer(ref head, ref source); Assert.Equal(0U, source); Assert.Equal(new[] { a, b, c, d }, StyleTextTestSupport.Chain(state, head));
        Assert.Throws<ArgumentException>(() => style.Transfer(ref head, ref head)); state.NextToken = 1; uint e = style.NewText("e\0"u8, new uint[8]); Assert.Equal(5U, e); style.Clear(ref head); Assert.Equal(0U, head); Assert.Single(state.Fragments); Assert.True(state.Fragments.ContainsKey(e));
        var last = new StyleTextState { NextToken = uint.MaxValue }; var exhausted = new StyleText(last, new()); Assert.Equal(uint.MaxValue, exhausted.NewText([0], new uint[8])); Assert.Equal(0U, last.NextToken); Assert.Throws<InvalidOperationException>(() => exhausted.NewText([0], new uint[8]));
    }

    /// <summary>Clear는 삭제 전에next를 저장하고 마지막head를0으로 쓴다. 예외 전 문자열 소멸과 잘못된 순환도 관측한다.</summary>
    [Fact]
    public void ClearCurrentHeadAndPartialFailure()
    {
        var state = new StyleTextState(); var h = new StyleTextHooks(); var style = new StyleText(state, h); uint head = 0; uint a = style.NewText("a\0"u8, new uint[8]), b = style.NewText("b\0"u8, new uint[8]), orphan = style.NewText("c\0"u8, new uint[8]); style.Append(ref head, a); style.Append(ref head, b); var calls = new List<uint>();
        h.Release = t => { calls.Add(t); head = orphan; }; style.Clear(ref head); Assert.Equal(new[] { a, b }, calls); Assert.Equal(0U, head); Assert.True(state.Fragments.ContainsKey(orphan));
        head = orphan; h.Release = _ => throw new InvalidOperationException("의도한 반납 예외"); Assert.Throws<InvalidOperationException>(() => style.Clear(ref head)); Assert.Equal(orphan, head); Assert.Equal(StyleTextFragmentKind.Base, state.Fragments[orphan].Kind); Assert.Empty(state.Fragments[orphan].Text);
        h.Release = null; uint cycle = style.NewText([0], new uint[8]); state.Fragments[cycle].Next = cycle; head = cycle; Assert.Throws<InvalidOperationException>(() => style.Clear(ref head)); Assert.False(state.Fragments.ContainsKey(cycle)); Assert.Equal(cycle, head);
    }

    /// <summary>모든 한 바이트 명령의 판본별 유효 집합·제한 슬롯·부분 문맥 변경을 독립 표로 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ShortCommandSetAndRestrictedSlots(OriginalEdition edition)
    {
        var style = new StyleText(new(), new() { Color = c => (uint)(1000 + c) }, edition); string valid = "*.013456BINEbgijklmnoqrtwxy" + (edition == OriginalEdition.Patch1078 ? "SUadpsu" : ""); uint[] original = [6, 5, 17, 1, 123, 7, 4, 8];
        // 모든 바이트를 검사하되 기대 유효 집합은 원본 정적case 목록을 독립 문자열로 고정했다.
        for (int code = 0; code < 256; code++) { uint[] context = (uint[])original.Clone(); Assert.Equal(valid.Contains((char)code), style.ApplyShortCommand((byte)code, context, original)); Assert.Equal(123U, context[4]); }
        uint[] restricted = (uint[])original.Clone(); Assert.True(style.ApplyShortCommand((byte)'3', restricted, original, true)); Assert.Equal(edition == OriginalEdition.Patch1078 ? 5U : 3U, restricted[1]);
        style.ApplyShortCommand((byte)'E', restricted, original); Assert.Equal(2U, restricted[0] & 2); Assert.Equal(1107U, restricted[5]); Assert.Equal((1U, 1U), (restricted[6], restricted[7])); style.ApplyShortCommand((byte)'N', restricted, original); Assert.Equal(0U, restricted[0] & 2); Assert.Equal(0U, restricted[3]); style.ApplyShortCommand((byte)'.', restricted, original); Assert.Equal(original, restricted);
    }

    /// <summary>기존head 뒤 구간/리터럴~~/미인식명령/끝 빈 조각과NUL 절단을 보존한다.</summary>
    [Fact]
    public void ParseFragmentsAndContextSnapshots()
    {
        var state = new StyleTextState(); var style = new StyleText(state, new() { Color = c => (uint)(100 + c) }); uint[] initial = [2, 5, 17, 1, 8, 9, 1, 1]; uint head = style.NewText("Z\0"u8, initial);
        style.Parse(ref head, "a~rB~.C~~D~?E~\0ignored"u8, initial); uint[] chain = StyleTextTestSupport.Chain(state, head); Assert.Equal(new[] { "Z", "a", "B", "C", "~", "D", "?E", "" }, chain.Select(t => StyleTextTestSupport.Text(state.Fragments[t])));
        Assert.Equal(214U, state.Fragments[chain[2]].Context[2]); Assert.Equal(initial, state.Fragments[chain[3]].Context); initial[2] = 88; Assert.Equal(17U, state.Fragments[chain[3]].Context[2]);
    }

    /// <summary>닫힌 대괄호의head/문맥/T태그 변경은 이후 모든 조각과 마지막 빈 조각에 남는다. ~.는 태그를 지우지 않는다.</summary>
    [Fact]
    public void BracketMetadataAndMissingBoundary()
    {
        var state = new StyleTextState(); var h = new StyleTextHooks(); var style = new StyleText(state, h); uint head = 0; uint[] initial = new uint[8];
        h.Bracket = (ReadOnlySpan<byte> command, uint[] context, ref uint owner, ref uint metadata) => { Assert.Equal("T 42", System.Text.Encoding.Latin1.GetString(command)); context[2] = 77; metadata = 42; uint picture = StyleText.RegisterStyleTextFragment(state, new() { Kind = StyleTextFragmentKind.External, Metadata = metadata }); style.Append(ref owner, picture); };
        style.Parse(ref head, "a~[T 42]b~."u8, initial); uint[] chain = StyleTextTestSupport.Chain(state, head); Assert.Equal(4, chain.Length); Assert.Equal(StyleTextFragmentKind.External, state.Fragments[chain[1]].Kind); Assert.Equal(42U, state.Fragments[chain[2]].Metadata); Assert.Equal(42U, state.Fragments[chain[3]].Metadata); Assert.Equal(77U, state.Fragments[chain[2]].Context[2]); Assert.Equal(initial, state.Fragments[chain[3]].Context);
        h.Bracket = null; uint partial = 0; Assert.Throws<ArgumentException>(() => style.Parse(ref partial, "a~[x]"u8, initial)); Assert.Equal("a", StyleTextTestSupport.Text(state.Fragments[partial])); uint open = 0; style.Parse(ref open, "~[unfinished"u8, initial); Assert.Equal("unfinished", StyleTextTestSupport.Text(state.Fragments[open]));
    }

    /// <summary>Measure는 두 번째에 현재head를 다시 읽고Draw는 시작head를 보존한다. 각가상호출 뒤 현재next를 읽는다.</summary>
    [Fact]
    public void MeasureAndDrawUseDifferentHeadSnapshots()
    {
        var state = new StyleTextState(); uint head = 1; int firstCalls = 0; var calls = new List<string>();
        // 세 안정된 외부 조각과 독립 가상 크기를 공급한다.
        for (uint t = 1; t <= 3; t++) StyleText.RegisterStyleTextFragment(state, new() { Kind = StyleTextFragmentKind.External });
        state.Fragments[1].Next = 2; var h = new StyleTextHooks { ExternalMetrics = t => { calls.Add("M" + t); if (t == 1 && ++firstCalls == 1) head = 3; return t == 1 ? (10, 2) : t == 2 ? (12, 6) : (20, 4); }, ExternalMeasure = t => { calls.Add("S" + t); return t == 3 ? (7, 9) : (3, 8); }, ExternalDraw = (t, at, _) => calls.Add($"D{t}:{at.X},{at.Y}") }; var style = new StyleText(state, h); int width = 0, height = 0;
        style.Measure(in head, ref width, ref height); Assert.Equal((7, 11), (width, height)); Assert.Equal(new[] { "M1", "M2", "M3", "S3" }, calls);
        head = 1; firstCalls = 0; calls.Clear(); Assert.Equal(12, style.Draw(head, new(10, 20))); Assert.Equal(3U, head); Assert.Equal(new[] { "M1", "M2", "M1", "S1", "D1:10,24", "M2", "S2", "D2:13,20" }, calls);
    }

    /// <summary>출력 alias에서도 높이→폭 쓰기를 유지하고CD만 출력 전에next를 저장한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0)]
    [InlineData(OriginalEdition.Cd1072, 5)]
    public void OutputAliasAndEditionNextTiming(OriginalEdition edition, int expectedNext)
    {
        var state = new StyleTextState(); StyleText.RegisterStyleTextFragment(state, new() { Kind = StyleTextFragmentKind.External }); StyleText.RegisterStyleTextFragment(state, new() { Kind = StyleTextFragmentKind.External }); int m1 = 0;
        var h = new StyleTextHooks { ExternalMetrics = t => { if (t == 1 && ++m1 == 1) state.Fragments[1].Next = 2; return (0, 0); }, ExternalMeasure = t => t == 1 ? (-2, 0) : (5, 0) }; var style = new StyleText(state, h, edition); uint head = 1; int height = 0;
        style.Measure(in head, ref Unsafe.As<uint, int>(ref state.Fragments[1].Next), ref height); Assert.Equal(unchecked((uint)expectedNext), state.Fragments[1].Next);
        uint empty = 0; int alias = 19; style.Measure(in empty, ref alias, ref alias); Assert.Equal(0, alias);
    }

    /// <summary>head와 출력 주소가 겹치면CD만 초기화 전head로 첫 순회를 수행한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0)]
    [InlineData(OriginalEdition.Cd1072, 1)]
    public void HeadAliasBeforeOutputInitialization(OriginalEdition edition, int expectedCalls)
    {
        var state = new StyleTextState(); StyleText.RegisterStyleTextFragment(state, new() { Kind = StyleTextFragmentKind.External }); int calls = 0; var style = new StyleText(state, new() { ExternalMetrics = _ => { calls++; return (3, 1); } }, edition); uint head = 1; int height = 19;
        style.Measure(in head, ref Unsafe.As<uint, int>(ref head), ref height); Assert.Equal(expectedCalls, calls); Assert.Equal(0U, head); Assert.Equal(0, height);
    }

    /// <summary>실제 글꼴/현재 색/출력override의font 보존을8비트 픽셀로 확인한다.</summary>
    [Fact]
    public void FontPaletteAndPixelBindings()
    {
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 10, 4)); byte[] pixels = Enumerable.Repeat((byte)7, 40).ToArray(); var colors = new GumpGlyphColorState(); FontStore fonts = StyleTextTestSupport.PixelFonts(); var draw = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(pixels, 10, 4, clip), colors)); var h = new StyleTextHooks(); StyleText.BindStyleTextFonts(h, new(fonts), draw);
        var palette = new PaletteColorTable(); palette.Basic[0] = 9; palette.Basic[1] = 171; palette.Basic[7] = 77; palette.Named.Add(new(249, 233, 145, 81)); StyleText.BindStyleTextPalette(h, palette); var state = new StyleTextState(); var style = new StyleText(state, h); uint head = 0; uint[] context = [0, 0, 55, 0, 0, 0, 1, 1]; style.Parse(ref head, "~rA~wA"u8, context); int width = 0, height = 0; style.Measure(in head, ref width, ref height); Assert.Equal((4, 14), (width, height)); Assert.Equal(31, style.Draw(head, new(1, 1))); Assert.Equal(171, pixels[11]); Assert.Equal(77, pixels[13]); Assert.Equal(77, colors.VfxColors[100]);
        uint[] selected = [0, 2, 66, 0, 0, 0, 1, 1]; pixels.AsSpan().Fill(7); style.DrawFragment(head, new(2, 2), selected); Assert.Equal(66, pixels[22]); Assert.Equal(2U, selected[1]); Assert.Equal(81U, h.Color!((byte)'i')); Assert.Throws<ArgumentException>(() => h.Color((byte)'a')); Assert.Throws<ArgumentException>(() => h.Color((byte)'?'));
    }
}
