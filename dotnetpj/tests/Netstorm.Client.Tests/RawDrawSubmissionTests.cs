using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 한 유효 dirty 안에서 UI→건설→묶음→객체→미리보기 순차 병합과 kind 0~4 분배를 대조한다.
// 2026-11-26 1.065에서 R5-2c용으로 추가했다.
public sealed class RawDrawSubmissionTests
{
    private static RawDrawCollectionRequest Request(uint flags = 0) => new(
        new(0, 0, 64, 64), new(0, 0, 16, 16), new(0, 0, 32, 32), flags, 70);

    private static RenderCollectionEntry Entry(float x, byte kind, uint token) => new(x, 1, token, 3, kind, 0);

    /// <summary>연결된 수집기를 원문 순서로 병합하고 묶음은 kind 1로 둔다.</summary>
    [Fact]
    public void CollectMergesInDocumentedOrder()
    {
        bool covered = true;
        var groups = new List<RawTileGroupView>
        {
            new(0x80000000U, [new RenderCollectionEntry(5, 6, 7, 2, 0, 9)]),
        };
        var hooks = new RawDrawCollectionHooks
        {
            Ui = (_, _) => [Entry(1, 0, 10)],
            Construction = (_, _) => [Entry(2, 0, 11)],
            Groups = (RenderWorldRect view, int limit, ref bool cover) => { cover = covered; return groups; },
            Objects = (_, _) => [Entry(3, 0, 12)],
            Preview = (_, _) => [Entry(4, 0, 13)],
        };
        RawDrawCollectionResult result = RawDrawSubmission.CollectDrawEntries(Request(), hooks);
        Assert.Equal(5, result.Count);
        Assert.True(result.BackgroundCovered);
        Assert.Equal([10u, 11u, 0x80000000U, 12u, 13u], result.Entries.Select(entry => entry.Token).ToList());
        Assert.Equal(1, result.Entries[2].Kind);
        Assert.Equal(5, result.Entries[2].X);
    }

    /// <summary>bit 2는 UI만 수집하고 배경 덮음을 거짓으로 둔다.</summary>
    [Fact]
    public void DirtyBitTwoCollectsUiOnly()
    {
        var hooks = new RawDrawCollectionHooks
        {
            Ui = (_, _) => [Entry(1, 0, 10)],
            Objects = (_, _) => [Entry(3, 0, 12)],
        };
        RawDrawCollectionResult result = RawDrawSubmission.CollectDrawEntries(Request(4), hooks);
        Assert.Equal([10u], result.Entries.Select(entry => entry.Token).ToList());
        Assert.False(result.BackgroundCovered);
    }

    /// <summary>넘치는 반환·빈 묶음을 거부한다.</summary>
    [Fact]
    public void BadCollectionsThrow()
    {
        var over = new RawDrawCollectionHooks
        {
            Ui = (screen, limit) => Enumerable.Range(0, RawDrawSubmission.Capacity + 1)
                .Select(i => Entry(1, 0, (uint)i)).ToList(),
        };
        Assert.Throws<InvalidOperationException>(() => RawDrawSubmission.CollectDrawEntries(Request(), over));
        var empty = new RawDrawCollectionHooks { Groups = (RenderWorldRect view, int limit, ref bool cover) => [new RawTileGroupView(1, [])] };
        Assert.Throws<ArgumentException>(() => RawDrawSubmission.CollectDrawEntries(Request(), empty));
    }

    /// <summary>kind 0~4를 분배하고 묶음 숨김·후속·누락을 지킨다.</summary>
    [Fact]
    public void DispatchRoutesKinds()
    {
        var calls = new List<string>();
        var groups = new Dictionary<uint, RawTileGroupView>
        {
            [77] = new(77, [new RenderCollectionEntry(1, 1, 5, 0, 0, 0), new RenderCollectionEntry(2, 2, 6, 0, 0, 0)]),
        };
        var hooks = new RawDrawDispatchHooks
        {
            Sprite = entry => calls.Add($"S{entry.Token}"),
            Group = token => groups[token],
            Ui = token => calls.Add($"U{token}"),
            Construction = token => calls.Add($"C{token}"),
            Preview = entry => calls.Add($"P{entry.Token}"),
            AfterEach = () => calls.Add("A"),
        };
        RawDrawSubmission.DispatchDrawEntries(
            [Entry(0, 0, 1), Entry(0, 1, 77), Entry(0, 2, 2), Entry(0, 3, 3), Entry(0, 4, 4), Entry(0, 9, 9)],
            false, true, hooks);
        Assert.Equal(["S1", "A", "S5", "S6", "A", "U2", "A", "C3", "A", "P4", "A", "A"], calls);
        calls.Clear();
        RawDrawSubmission.DispatchDrawEntries([Entry(0, 1, 77)], true, false, hooks);
        Assert.Empty(calls);
        Assert.Throws<InvalidOperationException>(() => RawDrawSubmission.DispatchDrawEntries(
            [Entry(0, 0, 1)], false, false, new RawDrawDispatchHooks()));
    }
}
