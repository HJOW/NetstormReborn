using Netstorm.Core.Original;

// 현재 월드 자산의 값 사본과 실제 최초 로딩 비용 owner의 정체성/소비 시점을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>RawSurfaceStorage vector 복사와 PostPop 최초 자산 비용 연결을 확인한다.</summary>
public sealed class SurfaceAssetsCurrentTests
{
    /// <summary>실제 자산 owner가 제공하는 최초 표와 지연 조회 횟수를 관찰한다.</summary>
    private sealed class Source : IRawSurfaceAssetSource
    {
        /// <summary>실제 최초 표의 입력이며 누락 검사는 의도적으로 null도 사용한다.</summary>
        public IReadOnlyList<OriginalTypeRecord>? Types { get; set; }
        /// <summary>최초 비용 owner가 실제로 소비된 횟수다.</summary>
        public int Reads { get; private set; }
        /// <summary>실제 최초 TYPE 표만 반환하며 현재 Storage 사본을 합성하지 않는다.</summary>
        public IReadOnlyList<OriginalTypeRecord> LoadedTypes { get { Reads++; return Types!; } }
        /// <summary>명시 실제 표 owner를 만들되 표를 조회/변경하지 않는다.</summary>
        public Source(IReadOnlyList<OriginalTypeRecord>? types) => Types = types;
    }
    /// <summary>실제 생성자/한 프레임과 서로 다른 비용을 갖는 최초 입력을 만든다.</summary>
    private static (OriginalTypeRecord[] Types, RiftTypeFrames[] Frames, SquidDisplayShape[] Shapes) Inputs()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index), MaxHitPoints = 1001, FootX = 1, FootY = 1, Cost = 17 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([new((byte)'A', (byte)'0', 0, 0)])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ =>
        {
            var shape = new SquidDisplayShape { Loaded = true, FrameCount = 1 }; shape.Frames.Add(new() { Width = 16, Height = 11, CellWidth = 1, CellHeight = 1 }); return shape;
        }).ToArray();
        return (types, frames, shapes);
    }
    /// <summary>별도 값을 지정하지 않은 독립 typed 저장소를 같은 입력에서 만든다.</summary>
    private static RawSurfaceStorage Storage() { var input = Inputs(); return new(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes); }

    /// <summary>TYPE와 중첩 usage/프레임/셰이프 목록/헤더를 최초 자산 입력과 독립 소유한다.</summary>
    [Fact]
    public void ConstructorOwnsIndependentAssetValuesAndNestedHeaders()
    {
        var input = Inputs(); input.Types[74].MinUsage[0] = 7; input.Types[74].MaxUsage[0] = 9; var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes);
        Assert.NotSame(input.Types[74], storage.Types[74]); Assert.NotSame(input.Types[74].MinUsage, storage.Types[74].MinUsage); Assert.NotSame(input.Types[74].MaxUsage, storage.Types[74].MaxUsage);
        Assert.NotSame(input.Frames[74], storage.Frames[74]); Assert.NotSame(input.Frames[74].Codes, storage.Frames[74].Codes); Assert.NotSame(input.Shapes[74], storage.Shapes[74]); Assert.NotSame(input.Shapes[74].Frames[0], storage.Shapes[74].Frames[0]);
        input.Types[74].Cost = 999; input.Types[74].MinUsage[0] = 88; input.Shapes[74].Frames[0].Width = 777; input.Shapes[74].Loaded = false;
        Assert.Equal(17, storage.Types[74].Cost); Assert.Equal(7, storage.Types[74].MinUsage[0]); Assert.Equal(16, storage.Shapes[74].Frames[0].Width); Assert.True(storage.Shapes[74].Loaded);
        storage.Types[74].MaxUsage[0] = 33; storage.Shapes[74].Frames[0].Height = 44; Assert.Equal(9, input.Types[74].MaxUsage[0]); Assert.Equal(11, input.Shapes[74].Frames[0].Height);
        Assert.Equal(storage.Frames[74].Codes, storage.Codes[74]); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }
    /// <summary>모든 표시 헤더 필드와 NaN payload/부호0를 계산 없이 그대로 복사한다.</summary>
    [Fact]
    public void ShapeValueCopyPreservesEveryHeaderAndFloatBitPattern()
    {
        var frame = new SquidDisplayFrame
        {
            Width = -123, Height = 321, HotspotX = short.MinValue, HotspotY = short.MaxValue, MuzzleX = -27, MuzzleY = 29,
            CellWidth = BitConverter.UInt32BitsToSingle(0x7fc01234), CellHeight = BitConverter.UInt32BitsToSingle(0x80000000), CellHotspotX = -17, CellHotspotY = 19, WalkStepX = -21, WalkStepY = 23,
        };
        var shape = new SquidDisplayShape { FrameCount = -3, Loaded = false }; shape.Frames.Add(frame); var copy = shape.Clone(); var actual = Assert.Single(copy.Frames);
        Assert.False(copy.Loaded); Assert.Equal(-3, copy.FrameCount); Assert.NotSame(frame, actual);
        Assert.Equal(-123, actual.Width); Assert.Equal(321, actual.Height); Assert.Equal(short.MinValue, actual.HotspotX); Assert.Equal(short.MaxValue, actual.HotspotY); Assert.Equal(-27, actual.MuzzleX); Assert.Equal(29, actual.MuzzleY);
        Assert.Equal(0x7fc01234u, BitConverter.SingleToUInt32Bits(actual.CellWidth)); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(actual.CellHeight)); Assert.Equal(-17, actual.CellHotspotX); Assert.Equal(19, actual.CellHotspotY); Assert.Equal(-21, actual.WalkStepX); Assert.Equal(23, actual.WalkStepY);
        frame.WalkStepX = 99; shape.Frames.Clear(); Assert.Equal(-21, actual.WalkStepX); Assert.Single(copy.Frames);
    }
    /// <summary>명시 최초 자산 정체성만 보관하고 생성/일반 getter/Source 조회에서 비용 표를 읽거나 Native를 추정하지 않는다.</summary>
    [Fact]
    public void SourceIdentityIsExplicitLazyAndIndependentOfNativePolicy()
    {
        var input = Inputs(); var source = new Source(input.Types); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes, assetSource: source);
        Assert.Same(source, storage.Source()); Assert.Equal(0, source.Reads); Assert.False(storage.UsesNativeTransmissionDevices()); storage.FactoryExecutor(); Assert.Equal(0, source.Reads); Assert.False(storage.Borrowed());
        Assert.Null(Storage().Source());
    }
    /// <summary>현재 TYPE 비용은 수정해도 최초 자산 표의 로딩 비용만 실제 PostPop에 공급한다.</summary>
    [Fact]
    public void PostPopReadsActualLoadedCostsRatherThanMutableCurrentTypes()
    {
        var input = Inputs(); var loaded = input.Types.Select(type => type.Clone()).ToArray(); var source = new Source(loaded);
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes, assetSource: source); storage.Types[74].Cost = 999;
        RawPostPop post = storage.PostPopExecutor(); Assert.Equal(1, source.Reads); Assert.Same(post, storage.PostPopExecutor()); Assert.Equal(1, source.Reads);
        int sid = storage.FactoryExecutor().Create(74); storage.PostPopState.Depth = 1; post.PostPopBase(sid, 1); Assert.Equal(17, storage.GameAux().TotalCost); Assert.Equal(0u, post.ActivationDepth);
        storage.Types[74].Cost = -333; sid = storage.FactoryExecutor().Create(74); storage.PostPopState.Depth = 1; post.PostPopBase(sid, 1); Assert.Equal(34, storage.GameAux().TotalCost); Assert.Equal(17, loaded[74].Cost); Assert.Equal(1, source.Reads);
    }
    /// <summary>독립 typed 경로는 실제 최초 자산 owner를 합성하지 않고 기존 실행기 구성 시점 비용 사본을 사용한다.</summary>
    [Fact]
    public void NoSourceKeepsExistingTypedPostPopSnapshotContract()
    {
        var input = Inputs(); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes); input.Types[74].Cost = 999; storage.Types[74].Cost = 33;
        RawPostPop post = storage.PostPopExecutor(); storage.Types[74].Cost = 777; int sid = storage.FactoryExecutor().Create(74); storage.PostPopState.Depth = 1; post.PostPopBase(sid, 1);
        Assert.Null(storage.Source()); Assert.Equal(33, storage.GameAux().TotalCost);
    }
    /// <summary>최초 자산 표가 실제 선택됐을 때만 범위를 거부하고 실패한 PostPop을 공개하지 않는다.</summary>
    [Fact]
    public void InvalidLoadedTableFailsOnlyWhenSelectedAndDoesNotPublishExecutor()
    {
        var input = Inputs(); var source = new Source([new()]); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes, assetSource: source);
        Assert.Equal(0, source.Reads); Assert.Throws<ArgumentException>(() => storage.PostPopExecutor()); Assert.Equal(1, source.Reads); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
        source.Types = null; Assert.Throws<InvalidOperationException>(() => storage.PostPopExecutor()); Assert.Equal(2, source.Reads);
        source.Types = input.Types; Assert.Same(storage.PostPopExecutor(), storage.PostPopExecutor()); Assert.Equal(3, source.Reads);
    }
    /// <summary>일반 fresh/준비 재대여가 현재 자산 값을 원본 입력으로 덮거나 owner/헤더 주소를 바꾸지 않는다.</summary>
    [Fact]
    public void PreparedAndGeneralWorldsKeepOwnedCurrentAssets()
    {
        var input = Inputs(); var source = new Source(input.Types); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, input.Types, input.Frames, input.Shapes, assetSource: source);
        OriginalTypeRecord type = storage.Types[74]; SquidDisplayFrame frame = storage.Shapes[74].Frames[0]; type.Cost = 123; frame.Width = 27;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, input.Types, 97, storage)) { Assert.Same(type, first.Types()[74]); Assert.Equal(123, first.Types()[74].Cost); }
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, input.Types, 97, storage, prepared: true)) Assert.Same(type, next.Types()[74]);
        using (var fresh = new RawSurfaceWorld(OriginalEdition.Patch1078, input.Types, 97, storage)) { Assert.Same(frame, storage.Shapes[74].Frames[0]); Assert.Equal(27, frame.Width); }
        Assert.Equal(17, input.Types[74].Cost); Assert.Equal(16, input.Shapes[74].Frames[0].Width); Assert.Same(source, storage.Source()); Assert.Equal(1, source.Reads);
    }
}
