using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 한 줄 텍스트의 생성/소유/클립/픽셀/수명 통합 검사다.

/// <summary>cpppj/src/client/TextGump.cpp의 한 줄 클래스 계약을 같은 실제 처리기로 검사한다. 독립 원본 실행/L1 기대값은 없다.</summary>
public sealed class TextGumpTests
{
    /// <summary>기본 루트1/텍스트2와 실제 글꼴·조각·표면·수명을 공유하는 장면.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 등록/판본/화면.</summary>
        public GumpCreateState Create { get; } = new() { ScreenWidth = 20, ScreenHeight = 8 };
        /// <summary>기본 루트 저장소.</summary>
        public GumpCreateNode Root { get; } = new();
        /// <summary>실제 텍스트 파생 저장소.</summary>
        public TextGumpNode Node { get; } = new();
        /// <summary>실제 조각 소유 표.</summary>
        public StyleTextState Fragments { get; } = new();
        /// <summary>현재 조각 가상 경계. 실제 예외/변경 입력으로 바꿀 수 있다.</summary>
        public StyleTextHooks StyleHooks { get; } = new();
        /// <summary>같은 현재 논리/VFX 클립.</summary>
        public GumpDrawState Clip { get; } = new();
        /// <summary>보호 앞/뒤 픽셀을 포함한 실제 출력 버퍼.</summary>
        public byte[] Guarded { get; } = Enumerable.Repeat((byte)7, 162).ToArray();
        /// <summary>실제 출력 영역.</summary>
        public Memory<byte> Pixels => Guarded.AsMemory(1, 160);
        /// <summary>현재 글꼴/VFX 색표.</summary>
        public GumpGlyphColorState Colors { get; } = new();
        /// <summary>실제 반납 순서.</summary>
        public List<string> Released { get; } = [];
        /// <summary>실제 기본 생성/등록.</summary>
        public GumpCreate Factory { get; }
        /// <summary>실제 트리/캐시/수집.</summary>
        public GumpCollection Collection { get; }
        /// <summary>실제 부모/형제 부착.</summary>
        public GumpTree Tree { get; }
        /// <summary>실제 지연 삭제/기본 소멸자.</summary>
        public GumpLifetime Lifetime { get; }
        /// <summary>실제 같은 조각 처리기.</summary>
        public StyleText Style { get; }
        /// <summary>실제 텍스트 처리기.</summary>
        public TextGump Text { get; }
        /// <summary>공통 Draw 보호/가상 분배.</summary>
        public GumpDraw Draw { get; }
        /// <summary>같은 실제 기본/파생을 조회한다.</summary>
        private GumpCreateNode? Find(uint token) => token == 1 ? Root : token == 2 ? Node : null;
        /// <summary>투영 콜백과 텍스트가 같은 노드/클립/조각을 사용한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078, Func<uint, ScreenPoint>? project = null)
        {
            Create.Collection.Edition = edition; Factory = new(Create, new() { Node = Find, Allocate = _ => 1 }); Factory.InitializeRoot(); Collection = new(Create.Collection, Factory.CollectionHooks(project));
            var branch = new GumpTreeHooks { Node = Find, Redraw = (_, _) => { }, Push = t => Tree!.Push(t), Attach = (t, x, y, p, m) => Tree!.Attach(t, x, y, p, m), Kill = t => Lifetime!.Kill(t) }; Tree = new(Create, new() { MapView = new(0, 0, 20, 8) }, branch);
            var life = new GumpLifetimeHooks { Node = Find, Push = branch.Push, DeletingDestructor = (t, f) => Lifetime!.DeleteScalar(t, f), Release = t => Released.Add("node:" + t) }; TextGump.BindTextGumpLifetime(life, t => t == 2 ? Text : null); Lifetime = new(Create, new(), life);
            GumpDraw.SetGumpDrawClip(Clip, new(0, 0, 20, 8)); FontStore fonts = StyleTextTestSupport.PixelFonts(); var glyph = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Pixels, 20, 8, Clip), Colors), edition); StyleText.BindStyleTextFonts(StyleHooks, new(fonts, edition), glyph); StyleHooks.Release = t => Released.Add("fragment:" + t); Style = new(Fragments, StyleHooks, edition);
            Text = new(Create, new() { Alignment = -7 }, Factory, Lifetime, Collection, Clip, Style, new() { Node = t => t == 2 ? Node : null, BaseNode = Find }); var draw = new GumpDrawHooks { Draw = _ => Assert.Fail("없는 텍스트 가상Draw") }; TextGump.BindTextGumpDraw(draw, t => t == 2 ? Text : null); Draw = new(Collection, Clip, draw);
        }
    }

    /// <summary>정수 생성/전체 문맥/실제 글꼴 맞춤·수집/픽셀·지연 삭제를 두 판본 같은 저장소에 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x5144d4U)]
    [InlineData(OriginalEdition.Cd1072, 0x505dc0U)]
    public void ConstructFitPixelsCollectAndDestroy(OriginalEdition edition, uint vtable)
    {
        var s = new Scene(edition); Assert.Equal(2U, s.Text.Construct(2, 8, 4, 0, 171)); Assert.Equal(vtable, s.Node.Vtable); Assert.Equal(0xfffffff9U, s.Node.Command); Assert.Equal(2U, s.Create.Count); Assert.Equal(new uint[] { 0, 0, 171, 0, 0, 0, 1, 1 }, s.Node.TextContext); s.Text.AppendText(2, "A A\0"u8, s.Node.TextContext); s.Text.Fit(2, 1, 2); Assert.Equal((8f, 16f), (s.Node.BaseWidth, s.Node.BaseHeight));
        s.Node.Height = 4; s.Tree.Move(2, 2, 1, 1); var work = new RenderCollectionEntry[4]; Assert.Equal(1, s.Collection.Collect(new(0, 0, 20, 8), work, 4)); Assert.Equal(2U, work[0].Token); s.Draw.Draw(2);
        Assert.Equal(171, s.Pixels.Span[22]); Assert.Equal(171, s.Pixels.Span[27]); Assert.Equal(2, s.Pixels.Span.ToArray().Count(p => p == 171)); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]); Assert.Equal(new ScreenRect(0, 0, 20, 8), s.Clip.Clip); Assert.Equal(19, s.Clip.VfxClip.Right);
        s.Lifetime.Kill(2); s.Lifetime.Cleanup(); Assert.Empty(s.Fragments.Fragments); Assert.Equal(1U, s.Create.Count); Assert.Equal(new[] { "fragment:1", "node:2" }, s.Released); Assert.Equal(0, s.Collection.Collect(new(0, 0, 20, 8), work, 4));
    }

    /// <summary>소유head가 비었으면 경계를 조회하지 않고0길이 추가는 잘못된 토큰도 조회하지 않는다. 재생성은 옛head를 반납하지 않는다.</summary>
    [Fact]
    public void EmptyAppendRangesAndReconstruction()
    {
        var s = new Scene(); s.Text.Construct(2, 8, 4, 0, 171); s.Node.Parent = 999; s.Text.Draw(2); s.Text.AppendText(999, [0], new uint[8]); s.Node.Parent = 0;
        Assert.Throws<ArgumentException>(() => s.Text.AppendText(2, [65], new uint[8])); Assert.Throws<ArgumentException>(() => s.Text.AppendText(2, [65], new uint[8], -2)); Assert.Throws<ArgumentOutOfRangeException>(() => s.Text.AppendText(2, [65], new uint[8], 2));
        s.Text.AppendText(2, [65, 0, 66], s.Node.TextContext, 3); uint old = s.Node.Head; Assert.Equal(new byte[] { 65, 0, 66, 0 }, s.Fragments.Fragments[old].Text); s.Text.Construct(2, 2, 3, 1, 9); Assert.Equal(0U, s.Node.Head); Assert.True(s.Fragments.Fragments.ContainsKey(old)); Assert.Empty(s.Released);
    }

    /// <summary>공통Draw 보호 비트가 없어도 내부 클립이 적용되고 출력 예외에도 복구된다.</summary>
    [Fact]
    public void InternalClipAndFailureRestoration()
    {
        var s = new Scene(); s.Text.Construct(2, 1, 1, 0, 171); s.Text.AppendText(2, "AA\0"u8, s.Node.TextContext); s.Tree.Move(2, 2, 1, 1); s.Node.Flags &= ~GumpDraw.DrawClip; s.Draw.Draw(2); Assert.Equal(171, s.Pixels.Span[22]); Assert.Equal(7, s.Pixels.Span[24]); Assert.Equal(new ScreenRect(0, 0, 20, 8), s.Clip.Clip);
        s.StyleHooks.TextDraw = (_, _, _) => { Assert.Equal(new ScreenRect(2, 1, 3, 2), s.Clip.Clip); GumpDraw.SetGumpDrawClip(s.Clip, new(9, 9, 10, 10)); throw new InvalidOperationException("출력 경계 실패"); }; Assert.Throws<InvalidOperationException>(() => s.Draw.Draw(2)); Assert.Equal(new ScreenRect(0, 0, 20, 8), s.Clip.Clip); Assert.Equal(19, s.Clip.VfxClip.Right);
    }

    /// <summary>측정 콜백이 바꾼 현재scale와 정수 여백 감김을 실제Fit에 반영한다.</summary>
    [Fact]
    public void FitUsesCurrentScaleAndWrappedPadding()
    {
        var s = new Scene(); s.Text.Construct(2, 1, 1, 0, 9); s.Text.AppendText(2, "A\0"u8, s.Node.TextContext); s.StyleHooks.TextMeasure = (ReadOnlySpan<byte> text, uint[] context, ref int w, ref int h) => { w = int.MaxValue; h = 8; s.Node.Scale = .5f; };
        s.Text.Fit(2, 2, 3); Assert.Equal((float)unchecked(int.MaxValue + 2), s.Node.BaseWidth); Assert.Equal((float)((double)unchecked(int.MaxValue + 2) * .5f), s.Node.Width); Assert.Equal((11f, 5.5f), (s.Node.BaseHeight, s.Node.Height));
    }

    /// <summary>두 번째Position 투영이head를 바꾸면 현재 새head를 출력한다. 내부클립은첫Bounds 뒤 저장한다.</summary>
    [Fact]
    public void ProjectionReadsCurrentHeadAfterBounds()
    {
        Scene? s = null; int calls = 0; uint replacement = 0; s = new Scene(project: _ => { if (++calls == 2) s!.Node.Head = replacement; return new(2, 1); }); s.Text.Construct(2, 8, 4, 0, 171); s.Text.AppendText(2, "A\0"u8, s.Node.TextContext); uint old = s.Node.Head;
        uint[] other = (uint[])s.Node.TextContext.Clone(); other[2] = 77; replacement = s.Style.NewText("A\0"u8, other); s.Node.Flags = (s.Node.Flags & ~GumpDraw.DrawClip) | 0x2000; s.Node.Parent = 1; s.Node.Object = 50; s.Draw.Draw(2); Assert.Equal(2, calls); Assert.Equal(replacement, s.Node.Head); Assert.Equal(77, s.Pixels.Span[22]); Assert.True(s.Fragments.Fragments.ContainsKey(old));
    }
}
