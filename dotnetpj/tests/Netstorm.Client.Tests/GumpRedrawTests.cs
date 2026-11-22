using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 원본 기본 갱신 요청·캐시와 현재 지도 조회 순서를 검증한다.

/// <summary>독립 PE1474관측을 같은 실제 위치/갱신 본체로 대조한다.</summary>
public sealed class GumpRedrawTests
{
    /// <summary>원본 DWORD 입력이다.</summary>
    private static uint N(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>원시 DWORD를 signed 좌표로 읽는다.</summary>
    private static int S(string text) => unchecked((int)N(text));
    /// <summary>원본 출력과 같은 비트 순서의 숫자열이다.</summary>
    private static string Packed(params uint[] values) => string.Join(',', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    /// <summary>현재LTRB의 원시 입력만 복원한다.</summary>
    private static ScreenRect Rectangle(string text) { var values = text.Split(','); return new(S(values[0]), S(values[1]), S(values[2]), S(values[3])); }
    /// <summary>Whole728·Map728·Clamp18의 요청/모든 캐시/투영 순서를 대조한다.</summary>
    [Fact]
    public void Native1474RequestsCachesAndMapClamp()
    {
        int count = 0, whole = 0, map = 0, clamp = 0;
        // 원본의 입력 노드/현재 지도만 제품에 제공한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "gump-redraw-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; var row = line.Split('\t'); Assert.Equal(11, row.Length); var bounds = Rectangle(row[4]); var requests = new List<string>(); string caches = "-", projected = "-";
            if (row[0] == "Clamp") { var result = GumpRedraw.ClampRectangle(Rectangle(row[10]), bounds); requests.Add(Packed(unchecked((uint)result.Left), unchecked((uint)result.Top), unchecked((uint)result.Right), unchecked((uint)result.Bottom))); clamp++; }
            else
            {
                var nodes = GumpRawSupport.Nodes(row[5]); var anchors = new Dictionary<uint, ScreenPoint>(); var projection = new List<uint>();
                if (row[6] != "-")
                {
                    // 원본 투영 장치의 명시 응답을 그대로 사용한다.
                    foreach (string item in row[6].Split(';')) { var values = item.Split(','); anchors.Add(N(values[0]), new(S(values[1]), S(values[2]))); }
                }
                var collection = new GumpCollection(new() { Root = nodes.First().Key, Edition = OriginalEdition.Patch1078 }, new() { Node = token => nodes.GetValueOrDefault(token), Project = sid => { projection.Add(sid); return anchors[sid]; } });
                var redraw = new GumpRedraw(collection, new() { Node = token => nodes.GetValueOrDefault(token), Map = () => bounds, Invalidate = (rect, flags) => requests.Add(Packed(unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), flags)) });
                if (row[0] == "Whole") { redraw.Whole(N(row[3]), N(row[2])); whole++; } else { Assert.Equal("Map", row[0]); redraw.Map(N(row[3]), N(row[2])); map++; }
                caches = string.Join(';', nodes.Select(pair => Packed(pair.Key, unchecked((uint)pair.Value.Cached.X), unchecked((uint)pair.Value.Cached.Y)))); if (projection.Count != 0) projected = string.Join(',', projection);
            }
            Assert.Equal(row[7], requests.Count == 0 ? "-" : string.Join(';', requests)); Assert.Equal(row[8], caches); Assert.Equal(row[9], projected); count++;
        }
        Assert.Equal(1474, count); Assert.Equal(728, whole); Assert.Equal(728, map); Assert.Equal(18, clamp);
    }
    /// <summary>실제 투영 반환 뒤의 현재 크기/지도를 읽으며 숨은 조상에서는 위치를 건드리지 않는다.</summary>
    [Fact]
    public void UsesCurrentProjectionSizeThenMapAndRejectsParentCycles()
    {
        var root = new GumpCreateNode { Flags = 1 }; var node = new GumpCreateNode { Flags = 0x2001, Parent = 1, Object = 7, Width = 12.75F, Height = 9.5F }; ScreenRect map = new(0, 0, 10, 10); string order = ""; var rectangles = new List<ScreenRect>();
        // 같은 두 현재 노드의 실제 주소만 조회한다.
        GumpCreateNode? Node(uint token) => token == 1 ? root : token == 2 ? node : null;
        var collection = new GumpCollection(new() { Root = 1 }, new() { Node = Node, Project = sid => { Assert.Equal(7U, sid); order += "P"; map = new(100, 200, 300, 400); node.Width = 20.75F; return new(111, 211); } });
        var redraw = new GumpRedraw(collection, new() { Node = Node, Map = () => { order += "M"; return map; }, Invalidate = (rect, flags) => { order += "I"; Assert.Equal(0U, flags); rectangles.Add(rect); } });
        redraw.Map(2, uint.MaxValue); Assert.Equal("PMI", order); Assert.Equal(new ScreenRect(111, 211, 131, 220), Assert.Single(rectangles));
        order = ""; rectangles.Clear(); root.Flags = 0; redraw.Whole(2); Assert.Empty(rectangles); Assert.Equal("", order);
        root.Flags = 1; node.Flags = 1; root.Parent = 2; Assert.Throws<InvalidOperationException>(() => redraw.Whole(2)); Assert.Throws<ArgumentOutOfRangeException>(() => redraw.Map(99));
    }
}
