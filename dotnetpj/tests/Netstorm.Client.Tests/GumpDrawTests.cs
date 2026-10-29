using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 클립 보호·복구와 실제 현재 표면 픽셀 출력을 검증한다.

/// <summary>기본 Gump Draw의 원본 전체 관측과 수집→가상 Draw→현재 클립 픽셀 계약을 대조한다.
/// 근거: cpppj/tests/GumpDrawTests.cpp. 공통 CRT 정렬/kind2 분배 연결은 R5에서 잇는다.</summary>
public sealed class GumpDrawTests
{
    /// <summary>fixture의 격리 노드 주소 기준.</summary>
    private const uint NativeNodes = 0x15000000;
    /// <summary>fixture의 각 노드 저장 간격.</summary>
    private const uint NativeStride = 128;

    /// <summary>signed 화면 경계를 저장 열에서 그대로 읽는다.</summary>
    private static ScreenRect Rect(string text)
    {
        int[] a = text.Split(',').Select(X86Fixture.Int).ToArray(); return new(a[0], a[1], a[2], a[3]);
    }

    /// <summary>실제 논리 경계 다음 inclusive VFX 경계 순서의 현재 관측.</summary>
    private static string Coordinates(GumpDrawState s) => GumpRawSupport.Coordinates(s.Clip) + "," + GumpRawSupport.Coordinates(s.VfxClip);

    /// <summary>각 판본476행의 보호 분기·현재 클립 변경·중첩 Draw·투영/캐시를 원본 기대값으로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeDraw(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpdraw", edition); Assert.Equal(476, rows.Length);
        // 입력마다 새 저장소를 쓰고 중첩 Draw만 같은 상태를 이어 쓴다.
        foreach (string[] row in rows)
        {
            Assert.Equal(17, row.Length); SortedDictionary<uint, GumpCreateNode> nodes = GumpRawSupport.Nodes(row[8]);
            var state = new GumpDrawState { Clip = Rect(row[4]), VfxClip = Rect(row[5]) }; ScreenRect? projectClip = row[7] == "-" ? null : Rect(row[7]);
            var anchors = new Dictionary<uint, ScreenPoint>(); var projected = new List<uint>();
            if (row[9] != "-")
            {
                // 투영의 정수점만 명시 경계이고 위치/경계/캐시는 실제 수집기가 계산한다.
                foreach (string item in row[9].Split(';'))
                {
                    string[] a = item.Split(','); anchors.Add(X86Fixture.UInt(a[0]), new(X86Fixture.Int(a[1]), X86Fixture.Int(a[2])));
                }
            }
            uint token = X86Fixture.UInt(row[2]); int mode = X86Fixture.Int(row[3]); ScreenRect change = Rect(row[6]);
            var collector = new GumpCollection(new() { Root = NativeNodes, Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072 }, new()
            {
                Node = key => nodes.GetValueOrDefault(key), Project = sid => { projected.Add(sid); if (projectClip.HasValue) GumpDraw.SetGumpDrawClip(state, projectClip.Value); return anchors[sid]; },
            });
            GumpDraw? draw = null; var observed = new List<string>();
            draw = new(collector, state, new() { Draw = current =>
            {
                observed.Add(current + "," + Coordinates(state));
                if (current == token && mode == 1) GumpDraw.SetGumpDrawClip(state, change);
                if (current == token && mode == 2) draw!.Draw(NativeNodes + 2 * NativeStride);
            } });
            draw.Draw(token); Assert.Equal(row[10], string.Join(';', observed)); Assert.Equal(row[11], Coordinates(state));
            Assert.Equal(row[12], string.Join(';', nodes.Select(p => p.Key + "," + GumpRawSupport.Coordinates(p.Value.Cached))));
            Assert.Equal(row[13], projected.Count == 0 ? "-" : string.Join(',', projected));
        }
    }

    /// <summary>클립/가상 Draw 계약을 같은 기본/생성 노드에 연결하는 최소 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>안정된 노드 저장소.</summary>
        public Dictionary<uint, GumpCollectionNode> Nodes { get; } = new() { [1] = new() };
        /// <summary>현재 논리/VFX 클립.</summary>
        public GumpDrawState State { get; } = new();
        /// <summary>명시 월드 투영점.</summary>
        public Dictionary<uint, ScreenPoint> Anchors { get; } = [];
        /// <summary>투영 중 적용할 현재 논리 클립.</summary>
        public ScreenRect? ProjectClip { get; set; }
        /// <summary>실제 투영 호출 순서.</summary>
        public List<uint> Projected { get; } = [];
        /// <summary>같은 노드의 현재 위치와 경계를 계산하는 수집기.</summary>
        public GumpCollection Collector { get; }
        /// <summary>필드/캐시를 복제하지 않고 같은 현재 객체를 수집기에 공급한다.</summary>
        public Scene()
        {
            Collector = new(new() { Root = 1 }, new() { Node = token => Nodes.GetValueOrDefault(token), Project = sid =>
            {
                Projected.Add(sid); if (ProjectClip.HasValue) GumpDraw.SetGumpDrawClip(State, ProjectClip.Value); return Anchors[sid];
            } });
        }
    }

    /// <summary>flagless Draw는 visible/dead/자식을 필터하지 않고 경계/캐시도 읽지 않는다. 보호 비트는 매 호출 현재 값이다.</summary>
    [Fact]
    public void CurrentFlagsAndNoUnclippedCacheWrite()
    {
        var s = new Scene(); s.Nodes[2] = new() { Flags = 0x10000, Parent = 999, Child = 999, Width = 4, Height = 4, X = 2, Y = 3, Cached = new(17, 19) };
        s.State.Clip = new(0, 0, 8, 8); s.State.VfxClip = new(91, 92, 93, 94); int calls = 0;
        var draw = new GumpDraw(s.Collector, s.State, new() { Draw = token => { Assert.Equal(2U, token); calls++; GumpDraw.SetGumpDrawClip(s.State, new(1, 2, 5, 6)); } });
        draw.Draw(2); Assert.Equal(1, calls); Assert.Equal(new ScreenPoint(17, 19), s.Nodes[2].Cached); Assert.Equal("1,2,5,6,1,2,4,5", Coordinates(s.State));
        s.Nodes[2].Parent = 1; s.Nodes[2].Flags = GumpDraw.DrawClip; GumpDraw.SetGumpDrawClip(s.State, new(0, 0, 8, 8)); draw.Draw(2);
        Assert.Equal(2, calls); Assert.Equal(new ScreenPoint(2, 3), s.Nodes[2].Cached); Assert.Equal("0,0,8,8,0,0,7,7", Coordinates(s.State));
    }

    /// <summary>가상 예외에서도 보호 클립은 복구하고 비보호 Draw가 바꾼 현재 클립은 남긴다.</summary>
    [Fact]
    public void ExceptionRestoresProtectedClip()
    {
        var s = new Scene(); s.Nodes[2] = new() { Flags = GumpDraw.DrawClip, Parent = 1, Width = 4, Height = 4, X = 2, Y = 3 };
        s.State.Clip = new(0, 0, 8, 8); s.State.VfxClip = new(91, 92, 93, 94);
        var draw = new GumpDraw(s.Collector, s.State, new() { Draw = _ => { GumpDraw.SetGumpDrawClip(s.State, new(1, 2, 5, 6)); throw new InvalidOperationException("검사 가상 Draw 실패"); } });
        Assert.Throws<InvalidOperationException>(() => draw.Draw(2)); Assert.Equal("0,0,8,8,0,0,7,7", Coordinates(s.State));
        s.Nodes[2].Flags = 0; Assert.Throws<InvalidOperationException>(() => draw.Draw(2)); Assert.Equal("1,2,5,6,1,2,4,5", Coordinates(s.State));
    }

    /// <summary>Bounds의 투영이 클립을 바꾸면 그 현재 영역을 저장하며 월드 부착 자신의 캐시는 갱신하지 않는다.</summary>
    [Fact]
    public void ProjectionChangesClipBeforeGuard()
    {
        var s = new Scene(); s.Nodes[2] = new() { Flags = GumpDraw.DrawClip | 0x2000, Parent = 1, Object = 7, Width = 10, Height = 10, X = 2, Y = 3, Cached = new(17, 19) };
        s.Anchors[7] = new(4, 5); s.ProjectClip = new(5, 7, 21, 31); GumpDraw.SetGumpDrawClip(s.State, new(0, 0, 8, 8)); string observed = "";
        new GumpDraw(s.Collector, s.State, new() { Draw = _ => observed = Coordinates(s.State) }).Draw(2);
        Assert.Equal("6,8,16,18,6,8,15,17", observed); Assert.Equal("5,7,21,31,5,7,20,30", Coordinates(s.State)); Assert.Equal(new uint[] { 7 }, s.Projected); Assert.Equal(new ScreenPoint(17, 19), s.Nodes[2].Cached);
    }

    /// <summary>실제 수집 항목의 Draw·버튼 선·투명 그림이 같은 현재 표면/클립을 사용한다. 정렬/공통 분배는 별도 R5 범위다.</summary>
    [Fact]
    public void TreeCollectionDrawPixels()
    {
        var s = new Scene(); s.Nodes[1].Child = 2; s.Nodes[2] = new() { Flags = 1 | GumpDraw.DrawClip, Parent = 1, Child = 3, Depth = 6, Width = 6, Height = 6, X = 2, Y = 2 };
        s.Nodes[3] = new() { Flags = 1 | GumpDraw.DrawClip, Parent = 2, Depth = 5, Width = 10, Height = 10, X = -2, Y = -2 };
        GumpDraw.SetGumpDrawClip(s.State, new(0, 0, 12, 12)); byte[] guarded = Enumerable.Repeat((byte)9, 146).ToArray(); Memory<byte> pixels = guarded.AsMemory(1, 144);
        var target = new GumpPixelTarget(pixels, 12, 12, s.State); var image = new IndexedImage { Width = 2, Height = 2, Indices = [55, 0, 255, 44], Opacity = [255, 0, 255, 255] }; var calls = new List<uint>();
        var draw = new GumpDraw(s.Collector, s.State, new() { Draw = token =>
        {
            calls.Add(token);
            if (token == 2) target.Lines(GumpVisual.PlanButtonDraw(new(s.Collector.Bounds(token), 0, 0, 0, 0, 0), new GumpTextContext[3], 17).Lines.ToArray());
            else target.Image(image, new(0, 0));
        } });
        var work = new RenderCollectionEntry[4]; int count = s.Collector.Collect(new(0, 0, 12, 12), work, 4); Assert.Equal(2, count);
        // 실제 수집 순서의 각 토큰을 현재 기본 Draw로 전달한다. 정렬 결과를 대신 만들지 않는다.
        for (int i = 0; i < count; i++) draw.Draw(work[i].Token);
        Assert.Equal(new uint[] { 2, 3 }, calls); Assert.Equal(17, pixels.Span[27]); Assert.Equal(55, pixels.Span[0]); Assert.Equal(9, pixels.Span[1]); Assert.Equal(255, pixels.Span[12]);
        Assert.Equal(9, guarded[0]); Assert.Equal(9, guarded[^1]); Assert.Equal("0,0,12,12,0,0,11,11", Coordinates(s.State));
    }

    /// <summary>실제 inclusive VFX 클립을 사용하고 불투명0/255도 소스 변환 표를 통과한다.</summary>
    [Fact]
    public void PixelImageTransparencyCurrentInclusiveClip()
    {
        var state = new GumpDrawState { Clip = new(0, 0, 8, 4), VfxClip = new(2, 1, 4, 2) }; byte[] pixels = Enumerable.Repeat((byte)9, 32).ToArray(); var target = new GumpPixelTarget(pixels, 8, 4, state);
        var image = new IndexedImage { Width = 4, Height = 1, Indices = [55, 0, 255, 44], Opacity = [0, 255, 255, 255] }; var colors = new byte[256];
        // 소스0/255도 변환 표의 실제 입력이다.
        for (int i = 0; i < colors.Length; i++) colors[i] = unchecked((byte)(i + 10));
        target.Image(image, new(1, 1), colors); Assert.Equal(9, pixels[9]); Assert.Equal(10, pixels[10]); Assert.Equal(9, pixels[11]); Assert.Equal(54, pixels[12]); Assert.Equal(9, pixels[13]);
        GumpDraw.SetGumpDrawClip(state, new(1, 2, 3, 3)); target.Image(image, new(1, 2)); Assert.Equal(9, pixels[17]); Assert.Equal(0, pixels[18]); Assert.Equal(9, pixels[19]);
        byte[] before = (byte[])pixels.Clone(); GumpDraw.SetGumpDrawClip(state, new(4, 2, 4, 3)); target.Image(image, new(1, 2)); Assert.Equal(before, pixels);
    }

    /// <summary>역순/끝점/현재 클립과 int 극단 inclusive 끝점의64비트 교차를 고정한다.</summary>
    [Fact]
    public void PixelLinesAndExtremeClip()
    {
        var state = new GumpDrawState(); GumpDraw.SetGumpDrawClip(state, new(2, 2, 6, 6)); byte[] pixels = Enumerable.Repeat((byte)9, 64).ToArray(); var target = new GumpPixelTarget(pixels, 8, 8, state);
        GumpLine[] lines = [new(new(7, 3), new(1, 3), 273), new(new(4, 7), new(4, 1), 18)]; target.Lines(lines);
        Assert.Equal(17, pixels[26]); Assert.Equal(17, pixels[29]); Assert.Equal(9, pixels[25]); Assert.Equal(9, pixels[30]); Assert.Equal(18, pixels[20]); Assert.Equal(18, pixels[44]); Assert.Equal(9, pixels[52]);
        state.VfxClip = new(0, 0, int.MaxValue, int.MaxValue); target.Lines([new(new(int.MinValue, 0), new(int.MaxValue, 0), 255)]); Assert.All(pixels[..8], value => Assert.Equal(255, value));
        GumpDraw.SetGumpDrawClip(state, new(0, 0, int.MinValue, int.MinValue)); Assert.Equal(int.MaxValue, state.VfxClip.Right);
        GumpDraw.SetGumpDrawClip(state, new(6, 6, 2, 2)); byte[] before = (byte[])pixels.Clone(); target.Lines(lines); Assert.Equal(before, pixels);
    }

    /// <summary>대각선도 현재 클립에서 실제 VFX 경로로 양 끝을 포함해 그린다.</summary>
    [Fact]
    public void DiagonalLinesUseCurrentClip()
    {
        var state = new GumpDrawState(); GumpDraw.SetGumpDrawClip(state, new(0, 0, 8, 8)); byte[] pixels = new byte[64]; var target = new GumpPixelTarget(pixels, 8, 8, state);
        GumpLine[] lines = [new(new(2, 2), new(3, 3), 1)]; target.Lines(lines); Assert.Equal(1, pixels[18]); Assert.Equal(1, pixels[27]); Assert.Equal(2, pixels.Count(x => x == 1));
        Array.Clear(pixels); GumpDraw.SetGumpDrawClip(state, new(0, 0, 3, 3)); target.Lines(lines); Assert.Equal(1, pixels[18]); Assert.Equal(0, pixels[27]); Assert.Equal(1, pixels.Count(x => x == 1));
    }

    /// <summary>현재 표면/가상 Draw/노드/부모/이미지 계약 밖의 입력은 실제 도달 지점에서 거부한다.</summary>
    [Fact]
    public void Contracts()
    {
        var s = new Scene(); s.Nodes[2] = new() { Flags = GumpDraw.DrawClip, Parent = 999, Width = 4, Height = 4, X = 2, Y = 3 }; GumpDraw.SetGumpDrawClip(s.State, new(0, 0, 8, 8));
        var draw = new GumpDraw(s.Collector, s.State, new() { Draw = _ => { } }); Assert.Throws<ArgumentException>(() => new GumpDraw(s.Collector, s.State, new()));
        Assert.Throws<InvalidOperationException>(() => draw.Draw(999)); Assert.Throws<InvalidOperationException>(() => draw.Draw(2)); Assert.Equal("0,0,8,8,0,0,7,7", Coordinates(s.State));
        byte[] pixels = new byte[64]; Assert.Throws<ArgumentException>(() => new GumpPixelTarget(pixels, 0, 8, s.State)); Assert.Throws<ArgumentException>(() => new GumpPixelTarget(pixels.AsMemory(0, 63), 8, 8, s.State));
        Assert.Throws<ArgumentException>(() => new GumpPixelTarget(pixels, 8, 8, s.State).Image(new() { Width = 2, Height = 2, Indices = [1], Opacity = [255] }, new(0, 0)));
    }

    /// <summary>원시 평면 주소/가려진 guard/보이는 할당 밖 장치·부분 쓰기/주소 넘침을 독립 픽셀로 검증한다.</summary>
    [Fact]
    public void RawBitmapFlatAddressesAndGuards()
    {
        var state = new GumpDrawState(); GumpDraw.SetGumpDrawClip(state, new(0, 0, 4, 2)); byte[] pixels = Enumerable.Repeat((byte)9, 8).ToArray(); var target = new GumpPixelTarget(pixels, 4, 2, state);
        var reads = new List<long>(); target.RawBitmap([10, 11, 12, 13], 2, -1, new(3, 2), new(-1, 0), new(0, 0, 4, 2), offset => { reads.Add(offset); return 77; });
        Assert.Equal(new byte[] { 10, 11, 9, 9, 12, 13, 9, 9 }, pixels); Assert.Empty(reads);
        Array.Fill(pixels, (byte)9); target.RawBitmap([10], 1, 0, new(2, 1), new(0, 0), new(0, 0, 4, 2), offset => { reads.Add(offset); return 77; });
        Assert.Equal(new long[] { 1 }, reads); Assert.Equal(10, pixels[0]); Assert.Equal(77, pixels[1]);
        Array.Fill(pixels, (byte)9); Assert.Throws<ArgumentOutOfRangeException>(() => target.RawBitmap([10], 1, 0, new(2, 1), new(0, 0), new(0, 0, 4, 2))); Assert.Equal(10, pixels[0]); Assert.Equal(9, pixels[1]);
        Assert.Throws<OverflowException>(() => target.RawBitmap([], 1, long.MaxValue, new(2, 1), new(0, 0), new(1, 0, 4, 2), _ => 0));
        Assert.Throws<ArgumentException>(() => target.RawBitmap([], 0, 0, default, default, default));
    }

    /// <summary>체크 fill은 절대 위상을 유지하고 임시 선 클립은 공유 전역을 바꾸지 않는다.</summary>
    [Fact]
    public void CheckerFillAndPrivateLineClip()
    {
        var state = new GumpDrawState(); GumpDraw.SetGumpDrawClip(state, new(0, 0, 4, 3)); byte[] pixels = Enumerable.Repeat((byte)9, 12).ToArray(); var target = new GumpPixelTarget(pixels, 4, 3, state);
        target.Fill(new(1, 0, 4, 3), 257, true); Assert.Equal(new byte[] { 9, 1, 9, 1, 9, 9, 1, 9, 9, 1, 9, 1 }, pixels);
        string before = Coordinates(state); target.LinesClipped([new(new(0, 1), new(3, 1), 17)], new(1, 0, 3, 3)); Assert.Equal(before, Coordinates(state));
        Assert.Equal(9, pixels[4]); Assert.Equal(17, pixels[5]); Assert.Equal(17, pixels[6]); Assert.Equal(9, pixels[7]);
        byte[] saved = (byte[])pixels.Clone(); target.Fill(new(3, 1, 1, 3), 99); Assert.Equal(saved, pixels);
    }
}
