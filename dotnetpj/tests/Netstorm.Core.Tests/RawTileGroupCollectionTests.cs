using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 같은 0단계 머리에서 행별 묶음·토큰·셀 환산을 수집한다.
// 2026-11-26 1.064에서 R5-2b용으로 추가했다.
public sealed class RawTileGroupCollectionTests
{
    /// <summary>화면 높이와 토큰 영역을 먼저 검사한다.</summary>
    [Fact]
    public void ConstructorValidatesRanges()
    {
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawTileGroupCollection(hash, pool, new RawTileGroupState(ScreenHeight: -1)));
        Assert.Throws<ArgumentException>(() => new RawTileGroupCollection(hash, pool, new RawTileGroupState(ObjectTokenBase: 0x80000000U)));
    }

    /// <summary>셀 환산은 +0.5 절삭 뒤 상한만 자른다.</summary>
    [Fact]
    public void CellKeepsLowerOverflowForExpand()
    {
        Assert.Equal(0, RawTileGroupCollection.Cell(float.NaN));
        Assert.Equal(RawSquidHash.WorldCells - 1, RawTileGroupCollection.Cell(300));
        // 음수 절삭은 확장 단계에서 클램프하므로 여기서는 음수를 보존한다
        Assert.Equal(-1, RawTileGroupCollection.Cell(-1.5f));
    }

    private static (RawTileGroupCollection Collector, RawSquidHash Hash, RawSidPool Pool) Scene()
    {
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        return (new RawTileGroupCollection(hash, pool, new RawTileGroupState(ScreenHeight: 768)), hash, pool);
    }

    /// <summary>한 칸 머리를 읽어 토큰·깊이·kind를 제출한다.</summary>
    [Fact]
    public void CollectSingleHeadSubmitsEntry()
    {
        var (collector, hash, pool) = Scene();
        hash.Select(0);
        hash.At(3, 4) = 7;
        Span<byte> raw = pool.RawSlot(7);
        raw.Clear();
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 3.5f);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 4.25f);
        raw[35] = 0xFE;
        var view = new RenderWorldRect(2.6f, 3.6f, 3.4f, 4.4f);
        var groups = collector.Collect(view, 10, out bool covered);
        Assert.False(covered);
        Assert.Single(groups);
        Assert.Equal(7 * 50u, groups[0].Entries[0].Token);
        Assert.Equal(3.5f, groups[0].Entries[0].X);
        Assert.Equal(4.25f, groups[0].Entries[0].Y);
        Assert.Equal(-2, groups[0].Entries[0].Depth);
        Assert.Equal(1, groups[0].Entries[0].Kind);
        // 객체 토큰을 SID로 되돌린다
        Assert.Equal(7, collector.Object(groups[0].Entries[0].Token));
        // 묶음 토큰으로 같은 보기를 찾는다
        Assert.Same(groups[0], collector.Group(groups[0].Token));
    }

    /// <summary>빈 칸이 하나라도 있으면 배경을 다시 그린다.</summary>
    [Fact]
    public void EmptyCellClearsCovered()
    {
        var (collector, hash, _) = Scene();
        hash.Select(0);
        hash.At(0, 0) = 5;
        var groups = collector.Collect(new RenderWorldRect(-0.4f, -0.4f, 1.4f, 1.4f), 10, out bool covered);
        Assert.False(covered);
        Assert.Single(groups);
    }

    /// <summary>행 제한에 닿으면 수집 중단을 명시한다.</summary>
    [Fact]
    public void RowLimitThrows()
    {
        var (collector, hash, _) = Scene();
        hash.Select(0);
        hash.At(0, 0) = 5;
        hash.At(1, 0) = 6;
        Assert.Throws<InvalidOperationException>(() => collector.Collect(new RenderWorldRect(-0.4f, -0.4f, 1.4f, 1.4f), 1, out _));
    }

    /// <summary>범위 밖 묶음·객체 토큰을 거부한다.</summary>
    [Fact]
    public void BadTokensThrow()
    {
        var (collector, _, _) = Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => collector.Group(0x80000001U));
        Assert.Throws<ArgumentOutOfRangeException>(() => collector.Object(1));
        Assert.Throws<ArgumentOutOfRangeException>(() => collector.Object(3));
    }
}
