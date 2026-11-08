// 표시 수집의 계약을 확인한다.
using Netstorm.Core.Display;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 표시 수집 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.
public sealed class SquidDisplayTests
{
    /// <summary>기록용 출력 장치다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private sealed class TestSink : SquidDisplaySink
    {
        public bool Suppress;
        public List<(DisplayRect Rect, uint Flags)> Invalidated = [];
        public List<byte[]> MetaRefreshed = [];

        public override bool Suppressed() => Suppress;

        public override void Invalidate(DisplayRect rect, uint flags) => Invalidated.Add((rect, flags));

        public override void MetaRefresh(ReadOnlySpan<byte> raw) => MetaRefreshed.Add(raw.ToArray());
    }

    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToList();
        types[80].Name = "T80";
        types[80].DefaultFrame = 2;
        return types;
    }

    /// <summary>세 프레임짜리 셰이프 표를 만든다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private static List<SquidDisplayShape> Shapes()
    {
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        shapes[80].FrameCount = 3;
        shapes[80].Loaded = true;
        shapes[80].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        shapes[80].Frames.Add(new SquidDisplayFrame { Width = 4, Height = 4, CellWidth = 4f, CellHeight = 4f });
        shapes[80].Frames.Add(new SquidDisplayFrame { Width = 200, Height = 200, CellWidth = 16f, CellHeight = 16f });
        return shapes;
    }

    /// <summary>카메라·화면을 만든다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private static SquidDisplayView View() => new() { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 2000, 2000) };

    /// <summary>50바이트 슬롯을 만든다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private static byte[] Slot(uint table, uint type, int frame, float x, float y, ushort word = 0, byte extra = 0)
    {
        var raw = new byte[50];
        Write(raw, 0, table, 4);
        raw[RawSidPool.TypeOffset] = (byte)type;
        Write(raw, 36, unchecked((uint)frame), 4);
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        Write(raw, 12, word, 2);
        raw[40] = extra;
        return raw;
    }

    /// <summary>목록 크기가 맞지 않으면 거부하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorRejectsMismatchedCounts()
    {
        Assert.Throws<ArgumentException>(() => new SquidDisplay(OriginalEdition.Patch1078, Types().GetRange(0, 2), Shapes().GetRange(0, 3), new TestSink(), View()));
    }

    /// <summary>잘못된 카메라를 거부하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void SetViewRejectsBadView()
    {
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), new TestSink(), View());
        Assert.Throws<ArgumentException>(() => display.SetView(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 0, Viewport = new DisplayRect(0, 0, 10, 10) }));
        Assert.Throws<ArgumentException>(() => display.SetView(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(10, 0, 0, 10) }));
    }

    /// <summary>프레임 연결 판정을 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void ValidateFrameBindingChecksContents()
    {
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), new TestSink(), View());
        display.ValidateFrameBinding(OriginalEdition.Patch1078, Types());
        var changed = Types();
        changed[80].Flags1 = 1u;
        Assert.Throws<ArgumentException>(() => display.ValidateFrameBinding(OriginalEdition.Patch1078, changed));
        Assert.Throws<ArgumentException>(() => display.ValidateFrameBinding(OriginalEdition.Cd1072, Types()));
    }

    /// <summary>패치 SHP 셀 크기를 읽는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameSizePatchReadsCells()
    {
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), new TestSink(), View());
        Assert.Equal([8f, 6f], display.FrameSize(OriginalEdition.Patch1078, Slot(0x00501DD0u, 80u, 0, 10f, 20f), 0));
        Assert.Throws<ArgumentException>(() => display.FrameSize(OriginalEdition.Cd1072, Slot(0x00501DD0u, 80u, 0, 10f, 20f), 0));
    }

    /// <summary>CD SHP 셀 크기를 읽는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameSizeCdReadsCells()
    {
        var display = new SquidDisplay(OriginalEdition.Cd1072, Types(), Shapes(), new TestSink(), View());
        Assert.Equal([8f, 6f], display.FrameSize(OriginalEdition.Cd1072, Slot(0x00501DD0u, 80u, 0, 10f, 20f), 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => display.FrameSize(OriginalEdition.Cd1072, Slot(0x00501DD0u, 80u, 5, 10f, 20f), 5));
    }

    /// <summary>화면 사각형을 계산하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundsComputesRect()
    {
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), new TestSink(), View());
        Assert.Equal(new DisplayRect(1598, 547, 1609, 560), display.Bounds(80, 0, 100f, 50f));
        Assert.Throws<ArgumentOutOfRangeException>(() => display.Bounds(80, 9, 100f, 50f));
    }

    /// <summary>일반 갱신이 본그림만 등록하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateMarksMain()
    {
        var sink = new TestSink();
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        display.Update(Slot(0x00501DD0u, 80u, 0, 100f, 50f));
        Assert.Equal([(new DisplayRect(1598, 547, 1609, 560), 0u)], sink.Invalidated);
    }

    /// <summary>그림자 표식이 함께 등록되는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateMarksShadow()
    {
        var sink = new TestSink();
        var types = Types();
        types[80].Flags1 = 0x40000u;
        var shapes = Shapes();
        shapes[80].FrameCount = 1;
        var display = new SquidDisplay(OriginalEdition.Patch1078, types, shapes, sink, View());
        display.Update(Slot(0x00501DD0u, 80u, 0, 100f, 50f));
        Assert.Equal([(new DisplayRect(1598, 547, 1609, 560), 0u), (new DisplayRect(1600, 550, 1605, 555), 0u)], sink.Invalidated);
    }

    /// <summary>선택 표식이 범위를 넓히는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateMarksMarker()
    {
        var sink = new TestSink();
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        display.Update(Slot(0x00501DD0u, 80u, 0, 100f, 50f, 0, 4));
        Assert.Equal([(new DisplayRect(1586, 532, 1621, 560), 16u)], sink.Invalidated);
    }

    /// <summary>억제 중에는 등록하지 않는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateSkipsWhenSuppressed()
    {
        var sink = new TestSink { Suppress = true };
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        display.Update(Slot(0x00501DD0u, 80u, 0, 100f, 50f));
        Assert.Empty(sink.Invalidated);
    }

    /// <summary>지원하지 않는 가상 표를 거부하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateRejectsUnsupportedVtable()
    {
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), new TestSink(), View());
        Assert.Throws<InvalidOperationException>(() => display.Update(Slot(0x12345678u, 80u, 0, 100f, 50f)));
    }

    /// <summary>메타 갱신이 장치에 그대로 전달되는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateForwardsMetaRefresh()
    {
        var sink = new TestSink();
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        byte[] slot = Slot(0x50c9f0u, 80u, 0, 100f, 50f);
        display.Update(slot);
        Assert.Equal([slot], sink.MetaRefreshed);
    }

    /// <summary>간헐천 고정 외형이 여백을 적용하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateGeyserAppliesInset()
    {
        var sink = new TestSink();
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        display.Update(Slot(0x50da88u, 80u, 0, 100f, 50f, 0, 0), 0x2000u);
        Assert.Equal([(new DisplayRect(1600, 600, 1801, 701), 0u)], sink.Invalidated);
    }

    /// <summary>마나 갱신이 10픽셀을 더하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateManaExpandsTen()
    {
        var sink = new TestSink();
        var display = new SquidDisplay(OriginalEdition.Patch1078, Types(), Shapes(), sink, View());
        display.Update(Slot(0x50c3b0u, 80u, 0, 100f, 50f, 8));
        display.Update(Slot(0x509810u, 80u, 0, 100f, 50f));
        Assert.Equal([(new DisplayRect(1598, 547, 1619, 570), 0u), (new DisplayRect(1598, 547, 1619, 570), 0u)], sink.Invalidated);
    }

    /// <summary>신전 부분 갱신이 inset을 적용하는지 검증한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateVortexAppliesInsets()
    {
        var sink = new TestSink();
        var types = Types();
        var shapes = Shapes();
        shapes[80].Frames[0] = new SquidDisplayFrame { Width = 100, Height = 100, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f };
        var display = new SquidDisplay(OriginalEdition.Patch1078, types, shapes, sink, View());
        display.Update(Slot(0x5196f0u, 80u, 0, 100f, 50f), 0x2000u);
        Assert.Equal([(new DisplayRect(1610, 576, 1687, 625), 0u)], sink.Invalidated);
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private static void Write(Span<byte> raw, int offset, uint value, int width)
    {
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
