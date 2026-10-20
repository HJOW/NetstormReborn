using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 미니맵 갱신 큐(<see cref="RawMetadisplayDirty"/>)의 경계와 Pop·Unpop 연결을 확인한다.
/// 원본 기대값 재생(X86RawMetadisplayDirtyTests)이 520행의 결과를 대조하고, 이 검사들은 cpppj/tests/MetadisplayDirtyTests.cpp 의
/// 같은 풀·현재 슬롯 좌표, 비활성 표시의 bitmap 확보, 필수 효과 거부, 실제 Pop/Unpop 통지 시점과 겹침 순서를 10.78 로 옮긴다.
/// 연결 지점 순서는 cpppj 구현의 계약이며 원본 행으로 증명된 것이 아니다 (W02 열두째 조각, 2026-10-20).
/// </summary>
public sealed class RawMetadisplayDirtyTests
{
    /// <summary>검사에 쓰는 타입 번호. 생성자 주소가 없어 기본 가상 표를 쓰고 섬 비트(2)를 켤 수 있다.</summary>
    private const uint TestType = 74;

    /// <summary>10.78 슬롯의 x 좌표(float) 위치.</summary>
    private const int XOffset = 14;

    /// <summary>10.78 슬롯의 y 좌표(float) 위치.</summary>
    private const int YOffset = 18;

    /// <summary>10.78 슬롯의 추가 비트 바이트 위치(매몰 비트 8 포함).</summary>
    private const int ExtraOffset = 40;

    /// <summary>검사 장면의 표시 상태를 채운 슬롯 풀을 만든다. 클라이언트 번호 5 를 확보한다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        Assert.Equal(RawSidPool.FirstOrdinary, pool.Allocate(Simulation.SidAllocation.Client));
        return pool;
    }

    /// <summary>표시가 켜진 빈 상태를 만든다.</summary>
    private static MetadisplayDirtyState ActiveState() => new() { DisplayPresent = true, DisplayFlags = 1 };

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>생성자 주소 표를 채운 10.78 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, OriginalConstructors.TypeCount)
        .Select(number => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(number) })
        .ToArray();

    /// <summary>
    /// 같은 풀의 통지 함수는 현재 슬롯 좌표를 읽고, 다른 풀·유한하지 않은 좌표는 거부한다.
    /// 좌표는 4로 나눈 정수 칸이며 -7.99 는 -1 이다(몫 절삭).
    /// </summary>
    [Fact]
    public void HookReadsLiveSlotAndRejectsForeignPool()
    {
        RawSidPool pool = Pool();
        MetadisplayDirtyState state = ActiveState();
        var dirty = new RawMetadisplayDirty(pool, state);
        Action<float, float> hook = RawMetadisplayDirty.MakeHook(pool, dirty);
        Span<byte> raw = pool.AllocatedBytes(RawSidPool.FirstOrdinary);
        Put(raw, XOffset, BitConverter.SingleToUInt32Bits(7.99f));
        Put(raw, YOffset, BitConverter.SingleToUInt32Bits(-7.99f));
        dirty.Enqueue(RawSidPool.FirstOrdinary);
        Assert.Equal(1, state.Count);
        Assert.Equal(new MetadisplayDirtyCell(1, -1), state.Cells[0]);
        // 같은 슬롯의 좌표를 바꾸면 새 칸으로 등록한다
        Put(raw, XOffset, BitConverter.SingleToUInt32Bits(8f));
        dirty.Enqueue(RawSidPool.FirstOrdinary);
        hook(8.25f, -4f);
        Assert.Equal(2, state.Count);
        Assert.Equal(new MetadisplayDirtyCell(2, -1), state.Cells[1]);
        // 다른 풀의 통지 함수는 만들지 않고, 무한 좌표는 등록하지 않는다
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => RawMetadisplayDirty.MakeHook(other, dirty));
        Assert.Throws<ArgumentOutOfRangeException>(() => dirty.Enqueue(float.PositiveInfinity, 1));
    }

    /// <summary>
    /// 비활성 표시는 좌표 변환·큐 변경 없이 돌려주지만, Refresh 는 bitmap 을 먼저 확보하고 전체 갱신을 요구한다.
    /// 활성 표시에서 효과가 없으면 rebuild·셀 그리기를 요구하며 예외로 거부한다.
    /// </summary>
    [Fact]
    public void InactiveDisplayAllocatesBitmapAndEffectsAreRequired()
    {
        RawSidPool pool = Pool();
        var state = new MetadisplayDirtyState { Count = 1 };
        state.Cells[0] = new MetadisplayDirtyCell(3, 4);
        var dirty = new RawMetadisplayDirty(pool, state);
        dirty.Enqueue(float.PositiveInfinity, 1);
        Assert.Equal(1, state.Count);
        dirty.Refresh(default);
        Assert.Equal(MetadisplayDirtyState.BitmapSide * MetadisplayDirtyState.BitmapSide, state.Bitmap.Length);
        Assert.Equal(MetadisplayDirtyState.BitmapSide, state.BitmapWidth);
        Assert.Equal(1U, state.FullRefresh);
        Assert.Equal(1, state.Count);
        Assert.Equal(0U, state.Frames);
        // 활성 표시에서 rebuild 효과가 없으면 그리기 전에 거부한다
        state.DisplayPresent = true;
        state.DisplayFlags = 1;
        Assert.Throws<InvalidOperationException>(() => dirty.Refresh(default));
        var rebuildOnly = new MetadisplayDirtyHooks { Rebuild = () => { } };
        var missingCell = new RawMetadisplayDirty(pool, state, rebuildOnly);
        Assert.Throws<InvalidOperationException>(() => missingCell.Refresh(default));
    }

    /// <summary>
    /// 실제 Pop·Unpop 통지는 등록·해제의 원본 시점에 일어난다: 겹침 Pop 도 좌표를 쓴 뒤 큐에 넣고,
    /// 같은 칸의 재등록은 중복을 만들지 않으며, 매몰·비섬 타입과 void 재해제는 통지하지 않는다.
    /// </summary>
    [Fact]
    public void PopAndUnpopNotifyAtOriginalPoints()
    {
        var pool = new RawSidPool(24000, false);
        var hash = new RawSquidHash();
        byte[] spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 2;
        var factory = new RawSquidFactory(pool, types);
        var unpop = new RawUnpop(pool, hash, spots);
        var pop = new RawPop(pool, hash, spots);
        MetadisplayDirtyState state = ActiveState();
        var dirty = new RawMetadisplayDirty(pool, state);
        pop.SetMetadisplay(dirty);
        unpop.SetMinimapQueue(dirty);
        int sid = factory.Create(TestType, Simulation.SidAllocation.Client);
        // 겹침 반환도 좌표를 쓴 뒤 큐에 넣고, spot·체인·void 는 그대로 둔다
        spots[40 * RawSquidHash.WorldCells + 30] = 2;
        Assert.Equal(RawPopResult.Overlap, pop.Pop(sid, types[TestType], 1, 1, 30, 40, 8));
        Assert.Equal(1, state.Count);
        Assert.Equal(new MetadisplayDirtyCell(7, 10), state.Cells[0]);
        Assert.Equal(30f, BitConverter.UInt32BitsToSingle(BitConverter.ToUInt32(pool.Slot(sid)[XOffset..])));
        Assert.Equal(RawSidPool.VoidBit, pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal(0, hash.Bucket(0, 30, 40));
        spots[40 * RawSquidHash.WorldCells + 30] = 0;
        // 같은 칸의 다시 등록은 중복을 만들지 않는다
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40, 8));
        Assert.Equal(1, state.Count);
        Assert.Equal(2, spots[40 * RawSquidHash.WorldCells + 30]);
        // 해제는 void 전환·spot 해제 전에 이전 위치를 알리고, void 재해제는 알리지 않는다
        state.Count = 0;
        unpop.Unpop(sid, types[TestType], 8);
        Assert.Equal(1, state.Count);
        Assert.Equal(new MetadisplayDirtyCell(7, 10), state.Cells[0]);
        Assert.Equal(RawSidPool.VoidBit, pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal(0, spots[40 * RawSquidHash.WorldCells + 30]);
        Assert.Equal(0, hash.Bucket(0, 30, 40));
        unpop.Unpop(sid, types[TestType], 8);
        Assert.Equal(1, state.Count);
        // 매몰 객체와 섬·다리가 아닌 타입도 통지하지 않는다
        int buried = factory.Create(TestType, Simulation.SidAllocation.Client);
        pool.AllocatedBytes(buried)[ExtraOffset] |= 8;
        Assert.Equal(RawPopResult.Registered, pop.Pop(buried, types[TestType], 1, 1, 40, 60, 8));
        Assert.Equal(1, state.Count);
        unpop.Unpop(buried, types[TestType], 8);
        Assert.Equal(1, state.Count);
        var plain = new OriginalTypeRecord
        {
            ConstructorAddress = types[TestType].ConstructorAddress,
            FootX = 1,
            FootY = 1,
            MaxHitPoints = types[TestType].MaxHitPoints,
            ZOrder = types[TestType].ZOrder,
        };
        int nonSurface = factory.Create(TestType, Simulation.SidAllocation.Client);
        Assert.Equal(RawPopResult.Registered, pop.Pop(nonSurface, plain, 1, 1, 80, 100, 8));
        Assert.Equal(1, state.Count);
        unpop.Unpop(nonSurface, plain, 8);
        Assert.Equal(1, state.Count);
        // 다른 풀을 쓰는 큐는 Pop·Unpop 에 연결할 수 없다
        var foreign = new RawMetadisplayDirty(new RawSidPool(24000, false), new MetadisplayDirtyState());
        Assert.Throws<ArgumentException>(() => pop.SetMetadisplay(foreign));
        Assert.Throws<ArgumentException>(() => unpop.SetMinimapQueue(foreign));
    }
}
