using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 메뉴의 실제 출력/현재 필드/클립 복구와 원본 J02 자산 대조 검사다.

/// <summary>정적 cpppj 계약·실제 자산/픽셀 조립 검사다.독립 PE Draw 관찰/L1이나 전체 화면 캡처는 아니다.</summary>
public sealed class MenuGumpDrawTests
{
    /// <summary>plain disabled는 임시 색,styled disabled/색 변경 항목은 실제 저장 문맥으로 출력한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualPlainAndStyledContextsPixels(OriginalEdition edition)
    {
        var s = new MenuDrawScene(edition); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0x200); s.Ui.Control.Controls.AddText("A"u8, enabled: false); s.Ui.Control.Controls.AddText("A"u8); s.Ui.Menu.Node.Items[2].Flags |= 0x200; s.Attach(); s.Ui.Scene.Draw.Draw(40); Assert.Equal(77, s.Ui.Scene.Pixels.Span[4 * 64 + 17]); Assert.Equal(81, s.Ui.Scene.Pixels.Span[20 * 64 + 17]); Assert.Equal(edition == OriginalEdition.Patch1078 ? 123 : 171, s.Ui.Scene.Pixels.Span[36 * 64 + 17]); Assert.Equal(new byte[] { 77 }, s.Colors); Assert.Equal(171U, s.Ui.Menu.Node.TextContext[2]); Assert.Equal(new ScreenRect(0, 0, 64, 128), s.Ui.Scene.Clip.Clip); Assert.Equal(7, s.Ui.Scene.Guarded[0]); Assert.Equal(7, s.Ui.Scene.Guarded[^1]);
    }
    /// <summary>선택 강조의 compact/세로/가로 로컬 float rect와 개별 클립을 확인한다.</summary>
    [Theory]
    [InlineData(0x1000000U, 4, 5, 30, 41)]
    [InlineData(0x80000U, 4, 8, 30, 14)]
    [InlineData(0x40000U, 4, 5, 15, 41)]
    public void ActualHighlightClips(uint flags, int left, int top, int right, int bottom)
    {
        var s = new MenuDrawScene(); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 8); s.Attach(); var n = s.Ui.Menu.Node; n.MenuFlags = flags; n.Width = 30; n.Height = 40; n.ItemWidth = 15; n.Offsets[0] = 3; n.Heights[0] = 10; n.BackgroundFlags = 0; s.Draw.Draw(40); Assert.NotEmpty(s.Shapes); Assert.All(s.Shapes, q => Assert.Equal(new ScreenRect(left, top, right, bottom), q.Clip)); Assert.Equal(new ScreenRect(0, 0, 64, 128), s.Ui.Scene.Clip.Clip);
    }
    /// <summary>실제 월드 투영의 큰 정수Y는 패치 bottom에서만 float32로 좁힌다.CD 분기의 조건식 공통형 좁힘을 막는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 16777224)]
    [InlineData(OriginalEdition.Cd1072, 16777225)]
    public void ProjectedBottomPreservesEditionNarrowing(OriginalEdition edition, int bottom)
    {
        var s = new MenuDrawScene(edition); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 8); s.Attach(); var n = s.Ui.Menu.Node; n.Flags |= 0x2000; n.X = n.Y = 0; n.Width = n.TargetWidth = n.Height = n.TargetHeight = n.Heights[0] = 10; n.Offsets[0] = 0; n.BackgroundFlags = 0; var collection = new GumpCollection(s.Ui.Scene.Create.Collection, new() { Node = t => s.Ui.Scene.Nodes.GetValueOrDefault(t), Project = _ => new(0, 16777217) }); GumpDraw.SetGumpDrawClip(s.Ui.Scene.Clip, new(0, 16777200, 64, 16777300)); var backgroundHooks = s.Ui.Decoration.BackgroundHooks; backgroundHooks.Shape = r => s.Shapes.Add((r, s.Ui.Scene.Clip.Clip)); var background = new GumpBackgroundDraw(collection, s.Ui.Scene.Clip, new(), backgroundHooks); var draw = new MenuGumpDraw(s.Ui.Scene.Create, s.Ui.Scene.Clip, s.State, collection, background, new(s.Fonts, s.GlyphHooks, edition), s.Ui.Menu.Style, s.Ui.Menu.Layout, s.Hooks); draw.Draw(40); Assert.NotEmpty(s.Shapes); Assert.All(s.Shapes, q => Assert.Equal(new ScreenRect(2, 16777219, 8, bottom), q.Clip)); Assert.Equal(new ScreenRect(0, 16777200, 64, 16777300), s.Ui.Scene.Clip.Clip);
    }
    /// <summary>구분선의 명암1픽셀 두 pass와 첫 pass 뒤 현재 배경 타입 재조회를 실제 출력에서 관측한다.</summary>
    [Fact]
    public void SeparatorPassesUseCurrentGlobalsAndIndividualClip()
    {
        var s = new MenuDrawScene(); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Ui.Control.Controls.AddSeparator(); s.Attach(); var n = s.Ui.Menu.Node; n.Items[0].Flags = 0x10; n.ItemCount = 1; n.Width = 40; n.Height = n.TargetHeight = 16; n.Heights[0] = 16; n.Offsets[0] = 0; n.BackgroundFlags = 0; var shade = s.Ui.Decoration.BackgroundHooks.Shade; s.Ui.Decoration.BackgroundHooks.Shade = c => { shade!(c); s.State.BackgroundType = 83; }; s.Draw.Draw(40); Assert.Equal(new uint[] { 82, 83 }, s.Shapes.Select(q => q.Request.Type).Distinct()); Assert.All(s.Shapes.Where(q => q.Request.Type == 82), q => Assert.Equal(new ScreenRect(4, 11, 40, 12), q.Clip)); Assert.All(s.Shapes.Where(q => q.Request.Type == 83), q => Assert.Equal(new ScreenRect(4, 12, 40, 13), q.Clip)); Assert.Empty(s.Glyphs); Assert.Empty(s.Markers);
    }
    /// <summary>일반 항목의 임시 italic/disabled 문맥은 기본 문맥을 변경하지 않으며 비활성은 그림자 패스를 생략한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void PlainBoldAndDisabledTemporaryContext(bool disabled)
    {
        var s = new MenuDrawScene(); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0x40U | (disabled ? 0x200U : 0)); s.Attach(); uint[] context = (uint[])s.Ui.Menu.Node.TextContext.Clone(); s.Draw.Draw(40); Assert.Equal(context, s.Ui.Menu.Node.TextContext); Assert.Equal(disabled ? new byte[] { 77 } : [9, 171], s.Colors); Assert.All(s.Glyphs, q => Assert.Same(s.Fonts.Get(0, FontStyle.Italic), q.Font));
    }
    /// <summary>체크 콜백 뒤 바뀐 항목 flags/색/개수로 스타일 출력과 다음 행을 선택하되 앞서 저장한 위치는 유지한다.</summary>
    [Fact]
    public void MarkerCallbacksUseCurrentItemAndCount()
    {
        var s = new MenuDrawScene(); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 4); s.Ui.Control.Controls.AddText("A"u8); s.Attach(); uint styled = s.Ui.Menu.Node.Items[1].Styled; s.Ui.Menu.Node.Items[1].Styled = 0; s.Ui.Menu.Node.Items[1].Flags = 0; s.Ui.Menu.Layout.SetText(40, 1, "A\0"u8.ToArray(), s.Ui.Menu.Node.TextContext); s.Ui.Menu.Node.ItemCount = 1; var marker = s.Hooks.Marker; s.Hooks.Marker = (t, p) => { marker!(t, p); s.Ui.Menu.Node.Items[0].Styled = styled; s.Ui.Menu.Node.Items[0].Flags = 0x400; s.Ui.Menu.Node.ItemCount = 2; s.Ui.Menu.Node.X = 7; s.State.Foreground = 66; }; s.Draw.Draw(40); Assert.Equal((82U, new ScreenPoint(10, 11)), (Assert.Single(s.Markers).Type, s.Markers[0].Point)); Assert.Equal(123, s.Ui.Scene.Pixels.Span[4 * 64 + 17]); Assert.Equal(new byte[] { 9, 66 }, s.Colors); Assert.Equal(new ScreenPoint(17, 20), s.Glyphs[^1].Position);
    }
    /// <summary>하위 표시는 현재 실제 항목 측정 뒤 DrawGump의 현재 Position/저장 임시 문맥을 사용한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualChildMarkerAndHorizontalBranch(OriginalEdition edition)
    {
        var s = new MenuDrawScene(edition); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0x800); s.Attach(); s.Draw.Draw(40); Assert.Contains(s.Glyphs, q => q.Code == 32 && q.Position == new ScreenPoint(20, 4)); Assert.Empty(s.Markers); var h = new MenuDrawScene(edition); h.Begin(true); h.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 4 | 0x800); h.Attach(); h.Ui.Menu.Node.Items[0].Flags |= 0x10; h.Draw.Draw(40); Assert.DoesNotContain(h.Glyphs, q => q.Code == 32); Assert.Empty(h.Markers); Assert.Empty(h.Shapes);
    }
    /// <summary>클립은 실제 marker 오류/40칸 범위 오류에서도 복구하고 이미 쓴 배경 픽셀은 보존한다.</summary>
    [Fact]
    public void ExceptionRestoresClipAndKeepsEarlierPixels()
    {
        var s = new MenuDrawScene(); s.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 4); s.Attach(); GumpDraw.SetGumpDrawClip(s.Ui.Scene.Clip, new(1, 2, 50, 60)); s.Hooks.Marker = (_, _) => throw new InvalidOperationException("명시 입력 오류"); Assert.Throws<InvalidOperationException>(() => s.Draw.Draw(40)); Assert.Equal(new ScreenRect(1, 2, 50, 60), s.Ui.Scene.Clip.Clip); Assert.Equal(new ScreenRect(1, 2, 49, 59), s.Ui.Scene.Clip.VfxClip); Assert.Equal(9, s.Ui.Scene.Pixels.Span[3 * 64 + 2]); s.Ui.Menu.Node.ItemCount = uint.MaxValue; s.Hooks.Marker = null; s.Draw.Draw(40); s.Ui.Menu.Node.ItemCount = 41; s.Ui.Menu.Node.Items[0].Flags = 0; Assert.Throws<ArgumentOutOfRangeException>(() => s.Draw.Draw(40)); Assert.Equal(new ScreenRect(1, 2, 50, 60), s.Ui.Scene.Clip.Clip);
    }
    /// <summary>보호 원본 폴더가 있는 저장소 최상위를 찾는다.자료는 읽기만 한다.</summary>
    private static string Root()
    {
        var dir = new DirectoryInfo(AppContext.BaseDirectory);
        // 실제 원본이 있는 최상위를 찾을 때까지 부모만 방문한다.
        while (dir is not null) { if (File.Exists(Path.Combine(dir.FullName, "originals", "Netstorm.exe"))) return dir.FullName; dir = dir.Parent; } throw new InvalidOperationException("보호 원본 저장소 경로 없음");
    }
    /// <summary>두 보호 SHP의 고정 J02=60을 독립 직접 합성해 실제 바인딩/상대 RECT/clip/padding과 전 바이트를 대조한다.</summary>
    [Theory]
    [InlineData("originals", "d")]
    [InlineData("originalCD", "D")]
    public void OriginalFortGumpJ02PixelsAndClip(string directory, string data)
    {
        string root = Root(); var files = Netstorm.Assets.GameFileSystem.Open(Path.Combine(root, "originals")); var types = new Netstorm.Assets.TypeCatalog(files); var art = types.Find("fortGump")!; Assert.Equal(22, art.LoadIndex); Assert.Equal(58, art.Definition.Frames.Find('J', Netstorm.Assets.TypeFrameTable.DefaultVariant, 0)); var shapes = new ShapeDatabase(File.ReadAllBytes(Path.Combine(root, directory, data, "_shapes.shp"))); var frame = shapes.Blocks()[22].Frames[60]; var image = shapes.Decode(22, 60); Assert.Equal((-6, -6, 12, 12), (frame.Rect.Left, frame.Rect.Top, image.Width, image.Height)); var assets = new GumpDecorationAssets(shapes, [new(92, 22, 54)]);
        // 두 실제 파일의 같은 고정60번 픽셀로 정상/부분/빈 클립을 각각 독립 합성한다.
        foreach (var point in new ScreenPoint[] { new(35, 24), new(10, 8), new(80, 56) })
        {
            byte[] actual = Enumerable.Repeat((byte)0xa5, 83 * 59).ToArray(), expected = (byte[])actual.Clone(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(9, 7, 72, 51)); var pixel = new GumpDecorationPixelTarget(new(actual, 83, 59, clip), assets); var hooks = new MenuGumpDrawHooks(); MenuGumpDraw.BindMenuGumpDrawAssets(hooks, type => type == 92 ? 58 : throw new ArgumentOutOfRangeException(nameof(type)), pixel); hooks.Marker!(92, point);
            // 기대 그림은 제품 선택식이 아닌 고정 물리60번에서 읽는다.각 행 padding도 같은 배열 대조에 포함한다.
            for (int y = 0; y < 12; y++)
            {
                // 원문 clip/투명도만 적용하며 제품의 Shape/Image 함수는 호출하지 않는다.
                for (int x = 0; x < 12; x++) { int dx = point.X - 6 + x, dy = point.Y - 6 + y, source = y * 12 + x; if (dx >= 9 && dy >= 7 && dx < 72 && dy < 51 && image.Opacity[source] != 0) expected[dy * 83 + dx] = image.Indices[source]; }
            }
            Assert.Equal(expected, actual); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Marker!(69, point));
        }
    }
}
