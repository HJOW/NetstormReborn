using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // Gump 트리 좌표/수집의 원본 관측과 저장소 공유 계약을 검증한다.

/// <summary>원본 PE의 위치·경계·수집 결과/전체 출력/캐시/투영 순서를 대조한다.
/// 근거: cpppj/tests/GumpCollectionTests.cpp. 공통 Draw 분배 통합은 R5에서 잇는다.</summary>
public sealed class GumpCollectionTests
{
    /// <summary>판본마다389행을 재생하며 top-level next 보관과 child next 재조회도 함께 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeCollection(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpcollection", edition); Assert.Equal(389, rows.Length);
        // 관측마다 독립 노드/응답을 쓰며 캐시와 링크 부작용을 함께 비교한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(12, row.Length); string[] config = row[3].Split(',');
            SortedDictionary<uint, GumpCreateNode> nodes = GumpRawSupport.Nodes(row[4]);
            var state = new GumpCollectionState { Root = X86Fixture.UInt(config[0]), OverrideHead = X86Fixture.UInt(config[1]), Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072 };
            var anchors = new Dictionary<uint, long[]>();
            if (row[5] != "-")
            {
                // 응답의 SID 다음 네 값은 정수 위치와 변경할next다.
                foreach (string item in row[5].Split(';'))
                {
                    long[] values = item.Split(',').Select(v => long.Parse(v, System.Globalization.CultureInfo.InvariantCulture)).ToArray(); anchors.Add((uint)values[0], values[1..]);
                }
            }
            var projected = new List<uint>();
            var collector = new GumpCollection(state, new()
            {
                Node = token => nodes.GetValueOrDefault(token),
                Project = sid =>
                {
                    projected.Add(sid); long[] a = anchors[sid];
                    if (a[2] != 0) nodes[(uint)a[2]].Next = (uint)a[3];
                    return new((int)a[0], (int)a[1]);
                },
            });
            uint target = X86Fixture.UInt(config[2]); int limit = X86Fixture.Int(config[3]);
            var view = new ScreenRect(X86Fixture.Int(config[4]), X86Fixture.Int(config[5]), X86Fixture.Int(config[6]), X86Fixture.Int(config[7]));
            var guarded = new RenderCollectionEntry[34];
            Array.Fill(guarded, new(BitConverter.UInt32BitsToSingle(0xa5a5a5a5), BitConverter.UInt32BitsToSingle(0xa5a5a5a5), 0xa5a5a5a5, unchecked((short)0xa5a5), 0xa5, 0xa5));
            Span<RenderCollectionEntry> work = guarded.AsSpan(1, 32);
            string result = row[1] switch
            {
                "Position" => GumpRawSupport.Coordinates(collector.Position(target)),
                "Bounds" => GumpRawSupport.Coordinates(collector.Bounds(target)),
                "Collect" => collector.Collect(view, work, limit).ToString(System.Globalization.CultureInfo.InvariantCulture),
                _ => collector.CollectNode(target, view, work, limit).ToString(System.Globalization.CultureInfo.InvariantCulture),
            };
            Assert.Equal(row[6], result); Assert.Equal(row[7], GumpRawSupport.EncodeEntries(work));
            Assert.Equal(row[8], string.Join(';', nodes.Select(pair => $"{pair.Key},{GumpRawSupport.Coordinates(pair.Value.Cached)}")));
            Assert.Equal(row[9], projected.Count == 0 ? "-" : string.Join(',', projected));
            Assert.Equal(row[11][..64], GumpRawSupport.EncodeEntries(guarded.AsSpan(0, 1)) + GumpRawSupport.EncodeEntries(guarded.AsSpan(33, 1)));
        }
    }

    /// <summary>최상위별 제한을 전체 제한으로 바꾸지 않고 실제 출력 용량 초과만 거부한다.</summary>
    [Fact]
    public void PerRootLimitAndSafeBuffer()
    {
        var nodes = new Dictionary<uint, GumpCollectionNode>
        {
            [1] = new() { Child = 2 }, [2] = new() { Flags = 1, Next = 3, Parent = 1, Depth = 4, Width = 10, Height = 10, X = 1, Y = 1 },
            [3] = new() { Flags = 1, Parent = 1, Depth = 5, Width = 10, Height = 10, X = 20, Y = 20 },
        };
        var collector = new GumpCollection(new() { Root = 1 }, new() { Node = token => nodes.GetValueOrDefault(token) });
        var work = new RenderCollectionEntry[2]; Assert.Equal(2, collector.Collect(new(0, 0, 100, 100), work, 1));
        Assert.Equal(2U, work[0].Token); Assert.Equal(3U, work[1].Token);
        Assert.Throws<ArgumentException>(() => collector.Collect(new(0, 0, 100, 100), work.AsSpan(0, 1), 1));
        Assert.Equal(0, collector.Collect(new(0, 0, 100, 100), [], 0)); Assert.Equal(0, collector.CollectNode(999, default, [], 0));
    }

    /// <summary>순환·누락·잘못된 월드 부모/투영을 유효 포인터 계약 밖으로 명시한다.</summary>
    [Fact]
    public void RejectsDamagedLinksAndMissingProjection()
    {
        var nodes = new Dictionary<uint, GumpCollectionNode> { [1] = new() { Child = 2 }, [2] = new() { Flags = 1, Parent = 1, Width = 10, Height = 10, X = 1, Y = 1 } };
        var collector = new GumpCollection(new() { Root = 1 }, new() { Node = token => nodes.GetValueOrDefault(token) });
        var work = new RenderCollectionEntry[8];
        Assert.Throws<ArgumentException>(() => new GumpCollection(new(), new())); Assert.Throws<InvalidOperationException>(() => collector.Position(999));
        nodes[2].Parent = 2; Assert.Throws<InvalidOperationException>(() => collector.Position(2)); nodes[2].Parent = 1;
        nodes[2].Next = 2; Assert.Throws<InvalidOperationException>(() => collector.Collect(new(0, 0, 100, 100), work, 8)); nodes[2].Next = 0;
        nodes[2].Child = 2; Assert.Throws<InvalidOperationException>(() => collector.Collect(new(0, 0, 100, 100), work, 8)); nodes[2].Child = 0;
        nodes[2].Flags = 0x2001; nodes[2].Parent = 999; Assert.Throws<InvalidOperationException>(() => collector.Position(2)); nodes[2].Parent = 1;
        Assert.Throws<InvalidOperationException>(() => collector.Position(2));
    }

    /// <summary>생성 노드와 수집 노드가 같은 인스턴스이며 현재 크기/위치/플래그/캐시를 공유한다.</summary>
    [Fact]
    public void CreationAndCollectionShareStorage()
    {
        var nodes = new Dictionary<uint, GumpCreateNode> { [1] = new(), [2] = new() };
        var state = new GumpCreateState { ScreenWidth = 16, ScreenHeight = 16 };
        var create = new GumpCreate(state, new() { Node = token => nodes.GetValueOrDefault(token), Allocate = _ => 1 });
        create.InitializeRoot(); create.Construct(2, 27, 4, 4, 1);
        nodes[1].Child = 2; nodes[2].Parent = 1; nodes[2].X = 2; nodes[2].Y = 3;
        var collector = new GumpCollection(state.Collection, create.CollectionHooks()); var work = new RenderCollectionEntry[4];
        Assert.Equal(1, collector.Collect(new(0, 0, 16, 16), work, 4)); Assert.Equal(2U, work[0].Token); Assert.Equal(-40, work[0].Depth); Assert.Equal(2, work[0].Kind);
        Assert.Equal(new ScreenPoint(2, 3), nodes[2].Cached); Assert.Equal(2U, state.Count); Assert.Equal(2U, state.Entries[1]);
        nodes[2].Flags = 0; Assert.Equal(0U, collector.Flags(2)); Assert.Equal(0, collector.Collect(new(0, 0, 16, 16), work, 4));
    }

    /// <summary>로컬 변환은 root 위치를 생략하고 월드 부모를 만난 뒤 일반 부모 위치를 더 빼지 않으며 캐시는 보존한다.</summary>
    [Fact]
    public void ScreenToLocalUsesCurrentParentsWithoutUpdatingCache()
    {
        var nodes = new Dictionary<uint, GumpCollectionNode>
        {
            [1] = new() { X = 7, Y = 11 }, [2] = new() { Parent = 1, X = 4.25F, Y = 3.5F },
            [3] = new() { Parent = 2, X = 1.5F, Y = 2.25F, Cached = new(17, -19) },
        };
        var collector = new GumpCollection(new() { Root = 1 }, new() { Node = token => nodes.GetValueOrDefault(token), Project = _ => new(100, 200) });
        Assert.Equal((14.25F, 24.25F), collector.ScreenToLocal(3, new(20, 30))); Assert.Equal(new ScreenPoint(17, -19), nodes[3].Cached);
        nodes[2].Flags = 0x2000;
        Assert.Equal((16F, 27F), collector.ScreenToLocal(3, new(120, 230))); Assert.Equal(new ScreenPoint(17, -19), nodes[3].Cached);
        nodes[1].Parent = 3; Assert.Throws<InvalidOperationException>(() => collector.ScreenToLocal(3, default));
    }
}
