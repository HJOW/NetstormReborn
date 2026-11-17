// 장기 펜스 이웃 검색·연결 개수의 선택 계약을 검사한다.
// cpppj/src/o/RawFenceNeighbors.h·cpp 와 client/RawSurfaceFence.cpp 의 FenceNeighbors 장기 본체를 대조한다.
// 0.588에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 이웃)
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>펜스 방향 검색·최근접 선택·동맹·연결 개수의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceFenceNeighborsTests
{
    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Write(System.Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>펜스 이웃용 188 타입 표를 만든다.</summary>
    private static Netstorm.Core.Original.OriginalTypeRecord[] Types()
    {
        // 188개 타입 번호 각각에 빈 타입 레코드를 채운다.
        var types = new Netstorm.Core.Original.OriginalTypeRecord[188];
        for (int index = 0; index < types.Length; index++)
        {
            types[index] = new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 };
        }

        return types;
    }

    /// <summary>풀·해시·타입을 펜스 이웃 검사 크기로 만든다.</summary>
    private static (Netstorm.Core.Original.RawSidPool Pool, Netstorm.Core.Original.RawSquidHash Hash, Netstorm.Core.Original.OriginalTypeRecord[] Types) Scene()
    {
        var pool = new Netstorm.Core.Original.RawSidPool(24000, false);
        // 5번부터 64번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 64; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }

        return (pool, new Netstorm.Core.Original.RawSquidHash(), Types());
    }

    /// <summary>슬롯 좌표·타입·상태를 기록한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">기록할 번호.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="state">상태 바이트.</param>
    /// <param name="extra">추가입력 바이트.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    /// <param name="owner">소유자 바이트.</param>
    private static void Place(
        Netstorm.Core.Original.RawSidPool pool, int sid, byte type, byte state, byte extra, float x, float y, byte owner)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = type;
        raw[Netstorm.Core.Original.RawSidPool.StateOffset] = state;
        raw[40] = extra;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        raw[34] = owner;
    }

    /// <summary>후보를 레벨 1 해시 체인에 잇는다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="sid">이을 번호.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    private static void LinkLevel1(Netstorm.Core.Original.RawSidPool pool, Netstorm.Core.Original.RawSquidHash hash, int sid, float x, float y)
    {
        // 표면을 건너뛰는 이웃 검색은 1단계 해시를 쓴다.
        ref ushort head = ref hash.Bucket(1, x, y);
        Write(pool.AllocatedBytes(sid), 4, head, 2);
        head = (ushort)sid;
    }

    /// <summary>표·getter 계약이 어긋나면 본체를 만들지 않는다.</summary>
    [Fact]
    public void ConstructorRejectsBadContract()
    {
        var (pool, hash, types) = Scene();
        var ranges = new float[188];
        var alliances = new uint[81];
        // 타입 수가 모자라면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, new Netstorm.Core.Original.OriginalTypeRecord[100], ranges, alliances));
        // 사거리 수가 모자라면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, new float[100], alliances));
        // 동맹 수가 모자라면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, ranges, new uint[80]));
        // getter가 빠지면 거부한다.
        Assert.Throws<System.ArgumentNullException>(() => new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, (Func<uint, float>)null!, (_, _) => 1u));
        Assert.Throws<System.ArgumentNullException>(() => new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 1.0f, (Func<uint, uint, uint>)null!));
    }

    /// <summary>중심은 발자국 절반을 뺀 위치다.</summary>
    [Fact]
    public void CenterUsesFootprint()
    {
        var (pool, hash, types) = Scene();
        types[74].FootX = 4;
        types[74].FootY = 2;
        types[74].Range = 20.0f;
        Place(pool, 50, 74, 0, 0, 20.75f, 21.9f, 1);
        var body = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 20.0f, (_, _) => 1u);
        float[] center = body.Center(50);
        Assert.Equal(20.75f - 2.0f, center[0], 4);
        Assert.Equal(21.9f - 1.0f, center[1], 4);
    }

    /// <summary>검색 경계는 방향 벡터와 발자국을 반영한다.</summary>
    [Fact]
    public void SearchBoundsDirectionAndRange()
    {
        var (pool, hash, types) = Scene();
        types[74].FootX = 1;
        types[74].FootY = 1;
        types[74].Range = 10.0f;
        Place(pool, 50, 74, 0, 0, 20.0f, 20.0f, 1);
        var body = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 10.0f, (_, _) => 1u);
        // 남쪽(방향 4)은 같은 x에서 y만 늘어난다.
        float[] south = body.SearchBounds(50, 4);
        Assert.Equal(new float[] { 20.0f, 20.0f, 20.0f, 30.0f }, south);
        // 동쪽(방향 2)은 같은 y에서 x만 늘어난다.
        float[] east = body.SearchBounds(50, 2);
        Assert.Equal(new float[] { 20.0f, 20.0f, 30.0f, 20.0f }, east);
        // 범위를 벗어난 방향은 거부한다.
        Assert.Throws<System.ArgumentOutOfRangeException>(() => body.SearchBounds(50, 8));
    }

    /// <summary>선택은 축 공유·생존·타입 일치 후보에서 가장 가까운 하나를 남긴다.</summary>
    [Fact]
    public void SelectFiltersAndKeepsClosest()
    {
        var (pool, hash, types) = Scene();
        types[74].FootX = 1;
        types[74].FootY = 1;
        types[74].Range = 100.0f;
        types[75].FootX = 1;
        types[75].FootY = 1;
        // 기준은 (20,20)이다.
        Place(pool, 50, 74, 0, 0, 20.0f, 20.0f, 1);
        // 같은 x의 가까운 후보(20,25)와 먼 후보(20,40)다.
        Place(pool, 60, 74, 0, 0, 20.0f, 25.0f, 1);
        Place(pool, 61, 74, 0, 0, 20.0f, 40.0f, 1);
        // 타입이 다른 후보·자기 자신·죽은 후보·차단 후보·축을 공유하지 않는 후보는 제외한다.
        Place(pool, 62, 75, 0, 0, 20.0f, 22.0f, 1);
        Place(pool, 63, 74, 2, 0, 20.0f, 21.0f, 1);
        Place(pool, 64, 74, 0, 9, 20.0f, 21.5f, 1);
        Place(pool, 51, 74, 0, 0, 25.0f, 25.0f, 1);
        var body = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 100.0f, (_, _) => 1u);
        // finder 순서를 그대로 주고 동점 첫 후보를 유지한다.
        Assert.Equal(60, body.Select(50, [60, 61], false));
        Assert.Equal(60, body.Select(50, [61, 60], false));
        // 제외 후보만 주면 0이다.
        Assert.Equal(0, body.Select(50, [62, 63, 64, 51, 50], false));
        // 동맹 검사는 가장 가까운 후보를 고른 뒤에 한다.
        Assert.Equal(60, body.Select(50, [60], true));
        var hostile = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 100.0f, (_, _) => 0u);
        // 소유자가 다르고 동맹값이 0이면 버린다.
        Place(pool, 60, 74, 0, 0, 20.0f, 25.0f, 2);
        Assert.Equal(0, hostile.Select(50, [60], true));
        // 소유자 범위를 벗어나면 거부한다.
        Place(pool, 60, 74, 0, 0, 20.0f, 25.0f, 9);
        Assert.Throws<System.ArgumentOutOfRangeException>(() => hostile.Select(50, [60], true));
    }

    /// <summary>축 거리 절삭과 사거리 포함을 판정한다.</summary>
    [Fact]
    public void WithinRangeUsesAxisDistance()
    {
        var (pool, hash, types) = Scene();
        types[74].Range = 10.0f;
        Place(pool, 50, 74, 0, 0, 0.0f, 0.0f, 1);
        var body = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 10.0f, (_, _) => 1u);
        // 수직이면 Y차로 판정한다. 절삭 뒤 비교이므로 10.9도 포함이다.
        Assert.True(body.WithinRange(50, [20.0f, 20.0f, 20.0f, 29.9f]));
        Assert.True(body.WithinRange(50, [20.0f, 20.0f, 20.0f, 30.0f]));
        Assert.False(body.WithinRange(50, [20.0f, 20.0f, 20.0f, 31.0f]));
        // 수직이 아니면 X차로 판정한다.
        Assert.True(body.WithinRange(50, [20.0f, 20.0f, 29.9f, 30.0f]));
        Assert.False(body.WithinRange(50, [20.0f, 20.0f, 31.0f, 30.0f]));
        // 끝점 개수가 다르면 거부한다.
        Assert.Throws<System.ArgumentException>(() => body.WithinRange(50, [0.0f, 0.0f, 0.0f]));
    }

    /// <summary>finder 순서로 남쪽 이웃을 찾아 연결 개수를 센다.</summary>
    [Fact]
    public void NeighborFindsSouthAndConnectionsCountsOne()
    {
        var (pool, hash, types) = Scene();
        types[74].FootX = 1;
        types[74].FootY = 1;
        types[74].Range = 20.0f;
        // 부모(20,20)와 남쪽 후보(20,30)는 x를 공유한다.
        Place(pool, 50, 74, 0, 0, 20.0f, 20.0f, 1);
        Place(pool, 60, 74, 0, 0, 20.0f, 30.0f, 1);
        hash.Reset();
        LinkLevel1(pool, hash, 60, 20.0f, 30.0f);
        var body = new Netstorm.Core.Original.RawFenceNeighbors(
            pool, hash, types, _ => 20.0f, (_, _) => 1u);
        // 남쪽(방향 4)에서 찾고 다른 방향에서는 찾지 못한다.
        Assert.Equal(60, body.Neighbor(50, 4, true));
        Assert.Equal(0, body.Neighbor(50, 0, true));
        Assert.Equal(0, body.Neighbor(50, 2, true));
        Assert.Equal(0, body.Neighbor(50, 6, true));
        // 비동맹 조회도 같은 후보를 찾는다.
        Assert.Equal(60, body.Neighbor(50, 4, false));
        // 연결 개수는 남쪽 하나다.
        Assert.Equal(1u, body.Connections(50));
        // 차단된 부모의 연결 개수는 0이다.
        Place(pool, 50, 74, 0, 9, 20.0f, 20.0f, 1);
        Assert.Equal(0u, body.Connections(50));
    }

    /// <summary>저장소 결선은 최초 한 번만 하고 같은 본체를 재사용한다.</summary>
    [Fact]
    public void StorageBindFenceNeighborsReuses()
    {
        // 전송 장치 없이 바인딩하면 거부한다.
        var bareTypes = Types();
        var bareFrames = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var bareShapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var bare = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, bareTypes, bareFrames, bareShapes);
        Assert.Throws<System.ArgumentException>(() => bare.BindFenceAlliances((_, _) => 1u));
        // 전체 결선 뒤 한 번만 연결한다.
        var types = Types();
        var frames = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindFenceAlliances((_, _) => 1u);
        Assert.Same(storage.FenceNeighbors(), storage.FenceNeighbors());
        Assert.Same(storage.Pool, storage.FenceNeighbors().Pool);
        // 중복 연결은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceAlliances((_, _) => 1u));
    }
}
