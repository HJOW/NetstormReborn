using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 1~3단계에서 각 SID의 본체/그림자를 별도로 판정한다.
// 2026-11-26 1.065에서 R5-2c용으로 추가했다.
public sealed class RawObjectCollectionTests
{
    private sealed class Scene
    {
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Hash = new RawSquidHash();
            Pool = new RawSidPool(24000, true, null, edition);
            Types = [new OriginalTypeRecord(), new OriginalTypeRecord { Flags1 = 0, Flags2 = 0, FrameCount = 1 }];
            Shapes = [new SquidDisplayShape(), new SquidDisplayShape { Loaded = true, FrameCount = 1 }];
            Shapes[1].Frames.Add(new SquidDisplayFrame { CellWidth = 4, CellHeight = 4, CellHotspotX = 0, CellHotspotY = 0 });
            Collector = new RawObjectCollection(Hash, Pool, Types, Shapes);
        }

        public RawSquidHash Hash { get; }
        public RawSidPool Pool { get; }
        public List<OriginalTypeRecord> Types { get; }
        public List<SquidDisplayShape> Shapes { get; }
        public RawObjectCollection Collector { get; }
    }

    /// <summary>표 길이·토큰 범위를 먼저 검사한다.</summary>
    [Fact]
    public void ConstructorValidatesTables()
    {
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        Assert.Throws<ArgumentException>(() => new RawObjectCollection(hash, pool, [], [new SquidDisplayShape()]));
        Assert.Throws<ArgumentException>(() => new RawObjectCollection(hash, pool, [new OriginalTypeRecord()], []));
        Assert.Throws<ArgumentException>(() => new RawObjectCollection(hash, pool,
            [new OriginalTypeRecord()], [new SquidDisplayShape()], 0xFFFFFFFFU));
    }

    private static void Put(Scene scene, int sid, float x, float y, int frame = 0, sbyte depth = 7)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid);
        raw.Clear();
        raw[10] = 1;
        BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        raw[35] = unchecked((byte)depth);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0);
    }

    private static void Place(Scene scene, int sid, float x, float y)
    {
        scene.Hash.Select(1);
        scene.Hash.At(RawObjectCollection.Cell(x, 1), RawObjectCollection.Cell(y, 1)) = unchecked((ushort)sid);
    }

    /// <summary>빈 해시는 0을 돌리고 2단계를 선택한다.</summary>
    [Fact]
    public void EmptyHashCollectsNothing()
    {
        var scene = new Scene();
        var output = new RenderCollectionEntry[8];
        Assert.Equal(0, scene.Collector.Collect(new(0, 0, 32, 32), output, 8));
        Assert.Equal(2, scene.Hash.Selected());
    }

    /// <summary>한 SID를 본체로 제출하고 토큰을 되돌린다.</summary>
    [Fact]
    public void SingleHeadSubmitsBody()
    {
        var scene = new Scene();
        Put(scene, 9, 10.5f, 20.5f);
        Place(scene, 9, 10.5f, 20.5f);
        var output = new RenderCollectionEntry[8];
        Assert.Equal(1, scene.Collector.Collect(new(0, 0, 32, 32), output, 8));
        Assert.Equal(10.5f, output[0].X);
        Assert.Equal(20.5f, output[0].Y);
        Assert.Equal(9u * 50u, output[0].Token);
        Assert.Equal(7, output[0].Depth);
        Assert.Equal(0, output[0].Kind);
        Assert.Equal(0, output[0].Tail);
        Assert.Equal(9, scene.Collector.Object(output[0].Token));
    }

    /// <summary>walker는 y에 0.5를 더하고 그림자는 0.1을 빼 tail 4로 둔다.</summary>
    [Fact]
    public void WalkerAndShadowAdjustY()
    {
        var scene = new Scene();
        scene.Types[1].Flags2 = 0x10000;
        scene.Types[1].Flags1 = 0x40000;
        scene.Shapes[1].Frames.Add(new SquidDisplayFrame { CellWidth = 4, CellHeight = 4, CellHotspotX = 0, CellHotspotY = 0 });
        Put(scene, 11, 10.5f, 20.5f);
        Place(scene, 11, 10.5f, 20.5f);
        var output = new RenderCollectionEntry[8];
        Assert.Equal(2, scene.Collector.Collect(new(0, 0, 32, 32), output, 8));
        Assert.Equal(21f, output[0].Y);
        Assert.Equal(0, output[0].Tail);
        Assert.Equal(20.4f, output[1].Y, 4);
        Assert.Equal(4, output[1].Tail);
    }

    /// <summary>같은 체인의 순환·0타입·작업 넘침·limit을 거부한다.</summary>
    [Fact]
    public void BadChainsThrow()
    {
        var scene = new Scene();
        Put(scene, 12, 10.5f, 20.5f);
        Span<byte> raw = scene.Pool.RawSlot(12);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 12);
        Place(scene, 12, 10.5f, 20.5f);
        Assert.Throws<InvalidOperationException>(() => scene.Collector.Collect(new(0, 0, 32, 32), new RenderCollectionEntry[8], 8));

        var typed = new Scene();
        Put(typed, 13, 10.5f, 20.5f);
        typed.Pool.RawSlot(13)[10] = 0;
        Place(typed, 13, 10.5f, 20.5f);
        Assert.Throws<ArgumentOutOfRangeException>(() => typed.Collector.Collect(new(0, 0, 32, 32), new RenderCollectionEntry[8], 8));

        var small = new Scene();
        Put(small, 14, 10.5f, 20.5f);
        Place(small, 14, 10.5f, 20.5f);
        Assert.Throws<InvalidOperationException>(() => small.Collector.Collect(new(0, 0, 32, 32), [], 8));
        Assert.Throws<InvalidOperationException>(() => small.Collector.Collect(new(0, 0, 32, 32), new RenderCollectionEntry[8], 1));
    }

    /// <summary>범위 밖 토큰을 거부한다.</summary>
    [Fact]
    public void BadTokensThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Collector.Object(1));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Collector.Object(3));
    }

    /// <summary>CD판은 실제 SHP 프레임·깊이 바이트를 쓴다.</summary>
    [Fact]
    public void CdEditionUsesPhysicalBytes()
    {
        var scene = new Scene(OriginalEdition.Cd1072);
        Span<byte> raw = scene.Pool.RawSlot(15);
        raw.Clear();
        raw[10] = 1;
        raw[34] = 0;
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 10.5f);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 20.5f);
        raw[33] = unchecked((byte)-3);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0);
        Place(scene, 15, 10.5f, 20.5f);
        var output = new RenderCollectionEntry[8];
        Assert.Equal(1, scene.Collector.Collect(new(0, 0, 32, 32), output, 8));
        Assert.Equal(-3, output[0].Depth);
        Assert.Equal(0, output[0].Tail);
    }
}
