// 장기 펜스 연결선·표식 정리의 검색·방향·태그 선택 계약을 검사한다.
// cpppj/src/o/RawFenceLine.h·cpp 와 o/RawFenceMarkerClear.h·cpp, client/RawSurfaceFence.cpp 의 FenceLine/FenceMarkerClear 장기 본체를 대조한다.
// 0.589에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 정리 효과)
namespace Netstorm.Core.Tests;

/// <summary>펜스 연결선 검색점·방향 삭제와 표식 영역·태그 삭제의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceFenceClearTests
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

    /// <summary>펜스 정리용 188 타입·프레임 표를 만든다.</summary>
    private static (Netstorm.Core.Original.OriginalTypeRecord[] Types, Netstorm.Core.Original.RiftTypeFrames[] Frames) Tables()
    {
        // 188개 타입 번호 각각에 빈 타입 레코드를 채운다.
        var types = new Netstorm.Core.Original.OriginalTypeRecord[188];
        for (int index = 0; index < types.Length; index++)
        {
            types[index] = new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 };
        }

        // 188개 타입 번호 각각에 빈 프레임 표를 채운다.
        var frames = new Netstorm.Core.Original.RiftTypeFrames[188];
        for (int index = 0; index < frames.Length; index++)
        {
            frames[index] = new Netstorm.Core.Original.RiftTypeFrames([]);
        }

        return (types, frames);
    }

    /// <summary>풀·해시를 펜스 정리 검사 크기로 만든다.</summary>
    private static (Netstorm.Core.Original.RawSidPool Pool, Netstorm.Core.Original.RawSquidHash Hash) Scene()
    {
        var pool = new Netstorm.Core.Original.RawSidPool(24000, false);
        // 5번부터 64번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 64; id++)
        {
            Assert.Equal(id, pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client));
        }

        return (pool, new Netstorm.Core.Original.RawSquidHash());
    }

    /// <summary>슬롯 좌표·타입·프레임을 기록한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">기록할 번호.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    private static void Place(Netstorm.Core.Original.RawSidPool pool, int sid, byte type, float x, float y)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = type;
        raw[Netstorm.Core.Original.RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
    }

    /// <summary>표 생성 계약이 어긋나면 본체를 만들지 않는다.</summary>
    [Fact]
    public void LineConstructorRejectsBadTables()
    {
        var (pool, hash) = Scene();
        var (types, frames) = Tables();
        var mark = new Netstorm.Core.Original.UIntBox() { Value = 170u };
        // 타입 수가 모자라면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceLine(
            pool, hash, new Netstorm.Core.Original.OriginalTypeRecord[100], frames, mark, (_, _) => { }));
        // 프레임 수가 모자라면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceLine(
            pool, hash, types, new Netstorm.Core.Original.RiftTypeFrames[100], mark, (_, _) => { }));
    }

    /// <summary>검색점은 수직·수평 보정과 100회 상한을 지킨다.</summary>
    [Fact]
    public void SearchPointsKeepsCorrectionAndLimit()
    {
        // 같은 끝점은 수직 보정 뒤 한 점만 낸다.
        var single = Netstorm.Core.Original.RawFenceLine.SearchPoints([20.0f, 20.0f, 20.0f, 20.0f]);
        Assert.Single(single);
        Assert.Equal([19, 16], single[0]);
        // 수평은 양쪽으로 벌리고 2씩 전진한다.
        var horizontal = Netstorm.Core.Original.RawFenceLine.SearchPoints([20.0f, 20.0f, 30.0f, 20.0f]);
        Assert.Equal([20, 18], horizontal[0]);
        Assert.Equal([21, 18], horizontal[1]);
        Assert.Equal([28, 18], horizontal[^1]);
        // 끝점 개수가 다르면 거부한다.
        Assert.Throws<System.ArgumentException>(() => Netstorm.Core.Original.RawFenceLine.SearchPoints([0.0f, 0.0f, 0.0f]));
    }

    /// <summary>연결선은 같은 타입·방향 프레임만 가상 삭제로 보낸다.</summary>
    [Fact]
    public void LineClearDestroysOnlyMatchingDirection()
    {
        var (pool, hash) = Scene();
        var (types, frames) = Tables();
        // 170번 타입의 0번 프레임 방향을 J로 둔다.
        frames[170] = new Netstorm.Core.Original.RiftTypeFrames(
            [new Netstorm.Core.Original.RiftFrameCode((byte)'J', 0, 0, 0)]);
        var mark = new Netstorm.Core.Original.UIntBox() { Value = 170u };
        // 검색점(19,16)에 같은 타입·J 후보를 둔다.
        Place(pool, 50, 170, 19.0f, 16.0f);
        Write(pool.AllocatedBytes(50), 36, 0u, 4);
        // 다른 타입 후보도 같은 버킷 체인에 잇고 건너뛴다.
        Place(pool, 51, 171, 19.0f, 16.0f);
        Write(pool.AllocatedBytes(51), 36, 0u, 4);
        Write(pool.AllocatedBytes(50), 4, 0, 2);
        Write(pool.AllocatedBytes(51), 4, 50, 2);
        hash.Bucket(0, 19.0f, 16.0f) = 51;
        var destroyed = new System.Collections.Generic.List<int>();
        var body = new Netstorm.Core.Original.RawFenceLine(
            pool, hash, types, frames, mark, (sid, flags) =>
            {
                Assert.Equal(0x40u, flags);
                destroyed.Add(sid);
            });
        body.Clear([20.0f, 20.0f, 20.0f, 20.0f]);
        Assert.Equal([50], destroyed);
        // 프레임 범위를 벗어나면 거부한다.
        Write(pool.AllocatedBytes(50), 36, 99u, 4);
        Assert.Throws<System.ArgumentOutOfRangeException>(() => body.Clear([20.0f, 20.0f, 20.0f, 20.0f]));
        // 삭제 장치가 없으면 명시 실패한다.
        var missing = new Netstorm.Core.Original.RawFenceLine(pool, hash, types, frames, mark, null);
        Write(pool.AllocatedBytes(50), 36, 0u, 4);
        Assert.Throws<System.InvalidOperationException>(() => missing.Clear([20.0f, 20.0f, 20.0f, 20.0f]));
    }

    /// <summary>표식 영역은 들어오는 모서리만 clamp하고 천장을 올린다.</summary>
    [Fact]
    public void MarkerBoundsClampsIncomingCorner()
    {
        var (pool, hash) = Scene();
        var (types, _) = Tables();
        var mark = new Netstorm.Core.Original.UIntBox() { Value = 90u };
        // 부모(10,10)와 이웃(30,30)은 그대로 둔다.
        var body = new Netstorm.Core.Original.RawFenceMarkerClear(
            pool, hash, types, mark, (_, _) => { },
            sid => sid == 50 ? [10.0f, 10.0f] : [30.0f, 30.0f]);
        Netstorm.Core.Original.SquidSearchArea area = body.Bounds(50, 60);
        Assert.Equal(new Netstorm.Core.Original.SquidSearchArea(10, 10, 30, 30), area);
        // 보드 밖 이웃은 1..255로 clamp한다.
        var clamped = new Netstorm.Core.Original.RawFenceMarkerClear(
            pool, hash, types, mark, (_, _) => { },
            sid => sid == 50 ? [10.0f, 10.0f] : [300.0f, 300.0f]);
        Assert.Equal(new Netstorm.Core.Original.SquidSearchArea(10, 10, 255, 255), clamped.Bounds(50, 60));
        // 비유한 중심은 거부한다.
        var broken = new Netstorm.Core.Original.RawFenceMarkerClear(
            pool, hash, types, mark, (_, _) => { },
            _ => [float.NaN, 0.0f]);
        Assert.Throws<System.ArgumentOutOfRangeException>(() => broken.Bounds(50, 60));
    }

    /// <summary>표식은 현재 부모 태그가 맞는 후보만 지운다.</summary>
    [Fact]
    public void MarkerClearDestroysOnlyParentTags()
    {
        var (pool, hash) = Scene();
        var (types, _) = Tables();
        var mark = new Netstorm.Core.Original.UIntBox() { Value = 90u };
        // 부모 50의 중심(10,10)과 이웃 60의 중심(30,30)이다.
        System.Func<int, float[]> center = sid => sid == 50 ? [10.0f, 10.0f] : [30.0f, 30.0f];
        // 61번은 raw+12 WORD가 부모와 같다.
        Place(pool, 61, 90, 15.0f, 15.0f);
        Write(pool.AllocatedBytes(61), 12, 50u, 2);
        // 62번은 packed QB 태그가 부모와 같다.
        Place(pool, 62, 90, 16.0f, 16.0f);
        Write(pool.AllocatedBytes(62), 12, 7u, 2);
        Write(pool.AllocatedBytes(62), 26, 0u, 4);
        pool.AllocatedBytes(62)[30] = 50;
        // 63번은 타입이 다르다.
        Place(pool, 63, 91, 15.0f, 15.0f);
        Write(pool.AllocatedBytes(63), 12, 50u, 2);
        // 64번은 태그가 다르다.
        Place(pool, 64, 90, 15.0f, 15.0f);
        Write(pool.AllocatedBytes(64), 12, 9u, 2);
        Write(pool.AllocatedBytes(64), 26, 0u, 4);
        pool.AllocatedBytes(64)[30] = 9;
        hash.Reset();
        // 레벨 1 체인에 태그 후보들을 잇는다.
        foreach (int sid in new int[] { 64, 63, 62, 61 })
        {
            System.Span<byte> raw = pool.AllocatedBytes(sid);
            float x = BitConverter.UInt32BitsToSingle(
                (uint)(raw[14] | (raw[15] << 8) | (raw[16] << 16) | (raw[17] << 24)));
            float y = BitConverter.UInt32BitsToSingle(
                (uint)(raw[18] | (raw[19] << 8) | (raw[20] << 16) | (raw[21] << 24)));
            ref ushort head = ref hash.Bucket(1, x, y);
            Write(pool.AllocatedBytes(sid), 4, head, 2);
            head = (ushort)sid;
        }

        var destroyed = new System.Collections.Generic.List<int>();
        var body = new Netstorm.Core.Original.RawFenceMarkerClear(
            pool, hash, types, mark, (sid, flags) =>
            {
                Assert.Equal(0x40u, flags);
                destroyed.Add(sid);
            }, center);
        body.Clear(50, 60);
        destroyed.Sort();
        Assert.Equal([61, 62], destroyed);
        // 삭제 장치가 없으면 명시 실패한다.
        var missing = new Netstorm.Core.Original.RawFenceMarkerClear(pool, hash, types, mark, null, center);
        Assert.Throws<System.InvalidOperationException>(() => missing.Clear(50, 60));
    }

    /// <summary>저장소 결선은 같은 본체를 재사용하고 역할을 공유한다.</summary>
    [Fact]
    public void StorageFenceClearReusesAndSharesRoles()
    {
        // 전송 장치 없이 바인딩하면 이웃부터 거부한다.
        var bareTypes = new Netstorm.Core.Original.OriginalTypeRecord[188];
        for (int index = 0; index < bareTypes.Length; index++)
        {
            bareTypes[index] = new Netstorm.Core.Original.OriginalTypeRecord();
        }

        var bareFrames = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var bareShapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var bare = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, bareTypes, bareFrames, bareShapes);
        Assert.Throws<System.ArgumentException>(() => bare.BindFenceAlliances((_, _) => 1u));
        // 전체 결선 뒤 같은 본체를 재사용한다.
        var (types, frames) = Tables();
        var storageShapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, storageShapes);
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindFenceAlliances((_, _) => 1u);
        Assert.Same(storage.FenceLine(), storage.FenceLine());
        Assert.Same(storage.FenceMarkerClear(), storage.FenceMarkerClear());
        Assert.Same(storage.Pool, storage.FenceLine().Pool);
        Assert.Same(storage.Pool, storage.FenceMarkerClear().Pool);
        // 역할 상자는 원본 초기값을 공유한다.
        Assert.Equal(170u, storage.FenceLine().MarkType);
        Assert.Equal(90u, storage.FenceMarkerClear().MarkerType);
        Assert.Same(storage.GameAux().FenceShieldTypeBox, storage.GameAux().FenceShieldTypeBox);
    }
}
