using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 일반 Pop·실제 표시/공간·원문 프레임 진행의 결선을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 부모를 실제 RawPop 어댑터와 같은 풀의 표시/공장/프레임에 조합한다.</summary>
public sealed class RawPopCurrentTests
{
    /// <summary>현재 raw 프레임과 실제 표시 변경 사각형을 함께 보관하는 무창 장치다.</summary>
    private sealed class Sink(RawSidPool pool) : SquidDisplaySink
    {
        /// <summary>관측할 현재 실제 SID다.</summary>
        public int Target;
        /// <summary>실제 변경 영역 계산 시점의 프레임이다.</summary>
        public readonly List<uint> Frames = [];
        /// <summary>실제 SHP로 계산한 변경 사각형이다.</summary>
        public readonly List<DisplayRect> Rectangles = [];
        /// <summary>프레임 상세 진단이 선택한 현재 타입·프레임·abstract 입력이다.</summary>
        public readonly List<(uint Type, int Frame, uint Abstract)> Errors = [];
        /// <summary>이 독립 장면은 원문 억제 플래그를 켜지 않는다.</summary>
        public override bool Suppressed() => false;
        /// <summary>실제 표시 계산 결과와 호출 시점의 raw 프레임을 수정 없이 보관한다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags)
        {
            Frames.Add(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(Target)[36..])); Rectangles.Add(rect);
        }
        /// <summary>오류 상세의 실제 선택을 기록하고 명시적으로 중단해 앞선 공간 변경을 관찰한다.</summary>
        public override void FrameError(uint type, int frame, uint abstractValue)
        {
            Errors.Add((type, frame, abstractValue)); throw new InvalidOperationException("명시 프레임 오류 상세 중단");
        }
        /// <summary>다음 진행의 old/new 두 효과만 관측하도록 이전 기록을 비운다.</summary>
        public void Clear() { Frames.Clear(); Rectangles.Clear(); }
    }

    /// <summary>같은 풀의 미니맵 경계에서 명시한 현재 변경만 공급한다.</summary>
    private sealed class MiniMap(RawSidPool pool, Action action) : IRawPopMetadisplay
    {
        /// <summary>실제 Pop과 공유하는 같은 풀이다.</summary>
        public RawSidPool Pool => pool;
        /// <summary>좌표 저장 뒤 실행되는 명시 외부 입력이다.</summary>
        public void Enqueue(int sid) => action();
    }

    /// <summary>같은 판본의 실제 생성자 주소와 기본 발자국을 공급한다.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, 188).Select(number => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(number) }).ToArray();

    /// <summary>독립 원문 입력 SID50까지 client 영역에 실제로 확보한다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원문 장면과 같은 client 번호를 확보해 물리 입력을 준비한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        Span<byte> raw = pool.RawSlot(50); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501dd0); raw[10] = 74; raw[11] = 4; return pool;
    }

    /// <summary>실제 어댑터의 전송중 진단은 공간/최초 Pop/void 해제 뒤 선택되고 정상 복귀한다.</summary>
    [Fact]
    public void ActualAdapterTransmittingDiagnosticFollowsSpatialAndVoidWrites()
    {
        var pool = Pool(); var hash = new RawSquidHash(); var spots = new byte[65536]; OriginalTypeRecord[] types = Types(); var pop = new RawPop(pool, hash, spots);
        pool.RawSlot(50)[11] = 0x14; var lines = new List<uint>();
        pop.BindDiagnostics(new SquidPopDiagnostics
        {
            Types = types, FrameSize = _ => [1, 1],
            Assertion = (expression, file, line) =>
            {
                Assert.Equal("!xmitJustCreatedFlag", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x35fu, line);
                Assert.Equal(0x10, pool.Slot(50)[11]); Assert.Equal((ushort)50, hash.Bucket(1, 30, 40)); Assert.Equal(0x80, pool.Slot(50)[40]);
                lines.Add(line); pool.RawSlot(50)[11] = 0;
            },
        });
        Assert.Equal(RawPopResult.Registered, pop.PopCurrent(50, 30, 40)); Assert.Equal(new uint[] { 0x35f }, lines); Assert.Equal(0, pool.Slot(50)[11]);
    }

    /// <summary>초기 TYPE 참조는 기존 표에 남고 다음 TYPE/SHP 효과는 교체된 현재 장치를 읽는다.</summary>
    [Fact]
    public void RebindingDiagnosticsDuringMiniMapKeepsInitialTypeAndCurrentEffects()
    {
        var pool = Pool(); var hash = new RawSquidHash(); var spots = new byte[65536]; OriginalTypeRecord[] initial = Types(), current = Types();
        initial[74].Flags2 = 2; current[75].Flags2 = 1; var pop = new RawPop(pool, hash, spots); int frameReads = 0;
        pop.BindDiagnostics(new SquidPopDiagnostics { Types = initial, FrameSize = _ => throw new InvalidOperationException("교체 전 SHP 선택 금지") });
        pop.SetMetadisplay(new MiniMap(pool, () =>
        {
            initial[74] = new OriginalTypeRecord { Flags2 = 2, FootX = 2 }; pool.RawSlot(50)[10] = 75;
            pop.BindDiagnostics(new SquidPopDiagnostics
            {
                Types = current,
                FrameSize = _ => { frameReads++; Assert.Equal(1, spots[40 * 256 + 29]); Assert.Equal(1, spots[40 * 256 + 30]); return [3, 1]; },
            });
        }));
        Assert.Equal(RawPopResult.Registered, pop.PopCurrent(50, 30, 40)); Assert.Equal(1, frameReads); Assert.Equal((ushort)50, hash.Bucket(2, 30, 40));
        Assert.Throws<ArgumentException>(() => pop.BindDiagnostics(new SquidPopDiagnostics { Types = initial[..187] }));
    }

    /// <summary>실제 표시기의 SHP 실패는 앞선 좌표/spot을 보존하고 해시/가상 후반을 실행하지 않는다.</summary>
    [Fact]
    public void ActualDisplayFrameFailureKeepsEarlierCoordinatesAndSpots()
    {
        var pool = Pool(); var hash = new RawSquidHash(); var spots = new byte[65536]; OriginalTypeRecord[] types = Types(); types[74].Flags2 = 1;
        SquidDisplayShape[] shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); shapes[74].Loaded = true; shapes[74].FrameCount = 1;
        shapes[74].Frames.Add(new SquidDisplayFrame { CellWidth = 1, CellHeight = 1 }); var sink = new Sink(pool) { Target = 50 };
        var display = new SquidDisplay(pool.Edition, types, shapes, sink, new SquidDisplayView()); var pop = new RawPop(pool, hash, spots, display);
        pop.BindDiagnostics(new SquidPopDiagnostics { Types = types }); BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(50)[36..], -1);
        Assert.Throws<InvalidOperationException>(() => pop.PopCurrent(50, 30, 40, 0x40)); Assert.Equal(new[] { (74u, -1, 0u) }, sink.Errors);
        Assert.Equal(1, spots[40 * 256 + 30]); Assert.Equal(30, BinaryPrimitives.ReadSingleLittleEndian(pool.Slot(50)[14..]));
        Assert.Equal(4, pool.Slot(50)[11]); Assert.Equal(0, pool.Slot(50)[40]); Assert.Equal((ushort)0, hash.Bucket(1, 30, 40)); Assert.Empty(sink.Frames);
    }

    /// <summary>실제 공장·SHP 표시·writer·Pop/Unpop·진행기를 같은 공간에 연결해 한 호출 늦은 단계 이동을 확인한다.</summary>
    [Fact]
    public void ActualFrameBindingMovesHashAndInvalidatesOldAndNewShapes()
    {
        var pool = new RawSidPool(32768, true); OriginalTypeRecord[] types = Types(); types[155].Flags1 = 0x28000002; types[155].Flags2 = 0x2000000;
        SquidDisplayShape[] shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); shapes[155].Loaded = true; shapes[155].FrameCount = 4;
        shapes[155].Frames.AddRange([
            new SquidDisplayFrame { Width = 11, Height = 17, HotspotX = 12, HotspotY = 16, CellWidth = 1, CellHeight = 1 },
            new SquidDisplayFrame { Width = 26, Height = 10, HotspotX = 5, HotspotY = 2, CellWidth = 2, CellHeight = 1.5f },
            new SquidDisplayFrame { Width = 8, Height = 4, HotspotX = 2, HotspotY = 1, CellWidth = 5, CellHeight = 5 },
            new SquidDisplayFrame { Width = 6, Height = 3, HotspotX = 1, CellWidth = 6, CellHeight = 6 },
        ]);
        var sink = new Sink(pool); var display = new SquidDisplay(pool.Edition, types, shapes, sink, new SquidDisplayView { Viewport = new DisplayRect(0, 0, 640, 480) });
        var hash = new RawSquidHash(); var spots = new byte[65536]; var unpop = new RawUnpop(pool, hash, spots, display); var pop = new RawPop(pool, hash, spots, display);
        pop.BindDiagnostics(new SquidPopDiagnostics { Types = types }); RawSquidTransmissionState wire = TestSquidTransmission.Bind(pop, pool, types)!;
        var factory = new RawSquidFactory(pool, types, unpop: unpop); sink.Target = factory.Create(155);
        Assert.Equal(RawPopResult.Registered, pop.PopCurrent(sink.Target, 20, 20)); Assert.Equal(1u, wire.CompressedCount);
        var setter = new SquidFrame(pool, types, SquidFrameBinding.MakeSquidFrameHooks(pool, types, display, unpop, pop)); setter.BindCurrentTypes(types);
        RiftTypeFrames[] codes = types.Select(_ => new RiftTypeFrames([])).ToArray(); codes[155] = new RiftTypeFrames(Enumerable.Range(0, 4).Select(index => new RiftFrameCode(65, 65, (byte)index, 0)));
        var advance = new RawFrameAdvance(pool, codes, setter);
        sink.Clear(); Assert.False(advance.Advance(sink.Target, 1)); Assert.Equal(new uint[] { 0, 1 }, sink.Frames); Assert.NotEqual(sink.Rectangles[0].Left, sink.Rectangles[1].Left);
        sink.Clear(); Assert.False(advance.Advance(sink.Target, 1)); Assert.Equal(new uint[] { 1, 2 }, sink.Frames); Assert.Equal((ushort)sink.Target, hash.Bucket(1, 20, 20));
        sink.Clear(); Assert.False(advance.Advance(sink.Target, 1)); Assert.Equal(new uint[] { 2, 3 }, sink.Frames); Assert.Equal((ushort)0, hash.Bucket(1, 20, 20)); Assert.Equal((ushort)sink.Target, hash.Bucket(3, 20, 20));
        sink.Clear(); Assert.True(advance.Advance(sink.Target, 1)); Assert.Equal(new uint[] { 3, 0 }, sink.Frames); Assert.Equal((ushort)sink.Target, hash.Bucket(3, 20, 20));
        sink.Clear(); Assert.False(advance.Advance(sink.Target, 1)); Assert.Equal(new uint[] { 0, 1 }, sink.Frames); Assert.Equal((ushort)sink.Target, hash.Bucket(1, 20, 20)); Assert.Equal((ushort)0, hash.Bucket(3, 20, 20));
        Assert.Equal(1u, wire.CompressedCount);
        Assert.Throws<ArgumentException>(() => SquidFrameBinding.MakeSquidFrameHooks(pool, types, display, new RawUnpop(pool, new RawSquidHash(), spots, display), pop));
        Assert.Throws<ArgumentException>(() => SquidFrameBinding.MakeSquidFrameHooks(pool, types, display, unpop, new RawPop(pool, hash, spots)));
        Assert.False(pop.ReleaseMetadisplay(null));
        var replacement = new MiniMap(pool, () => { }); pop.SetMetadisplay(replacement); Assert.False(pop.ReleaseMetadisplay(new MiniMap(pool, () => { }))); Assert.True(pop.ReleaseMetadisplay(replacement)); Assert.False(pop.ReleaseMetadisplay(null));
    }
}
