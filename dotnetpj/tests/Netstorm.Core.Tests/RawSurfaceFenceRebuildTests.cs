// 펜스 재생성 본체와 수명 실제 본체 결선의 동맹·사거리·생성 순서를 검사한다.
// cpppj/src/o/RawFenceRebuild.h·cpp 와 client/RawSurfaceFence.cpp 의 FenceRebuild 장기 본체를 대조한다.
// 0.590에서 추가됐다. (R4-1 잔여 중 펜스 수명 실제 이웃 결선)
namespace Netstorm.Core.Tests;

/// <summary>재생성의 방향 순서·extra 검사·효과 순서와 저장소 실제 결선을 확인한다.</summary>
public sealed class RawSurfaceFenceRebuildTests
{
    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Write(System.Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 4바이트를 쓴다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>재생성 검사 크기의 풀을 만든다.</summary>
    private static Netstorm.Core.Original.RawSidPool Scene()
    {
        var pool = new Netstorm.Core.Original.RawSidPool(24000, false);
        // 5번부터 64번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 64; id++)
        {
            Assert.Equal(id, pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client));
        }

        return pool;
    }

    /// <summary>슬롯 좌표·타입·owner·extra를 기록한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">기록할 번호.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    /// <param name="owner">소유자 바이트.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    private static void Place(Netstorm.Core.Original.RawSidPool pool, int sid, float x, float y, byte owner = 1, byte extra = 0)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = 12;
        raw[Netstorm.Core.Original.RawSidPool.StateOffset] = 0;
        raw[34] = owner;
        raw[40] = extra;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y));
    }

    /// <summary>네 방향을 모두 기록하는 이웃 훅을 만든다.</summary>
    /// <param name="calls">호출 기록.</param>
    /// <param name="other">0번 방향에 돌려줄 이웃 번호.</param>
    private static System.Func<int, uint, int> NeighborHook(System.Collections.Generic.List<uint> calls, int other) =>
        (_, direction) =>
        {
            calls.Add(direction);
            return direction == 0 ? other : 0;
        };

    /// <summary>판본·효과 계약이 어긋나면 본체를 만들지 않는다.</summary>
    [Fact]
    public void RebuildConstructorRejectsBadPoolOrHooks()
    {
        var pool = Scene();
        var hooks = new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = (_, _) => 0,
            WithinRange = (_, _) => true,
            Markers = (_, _) => { },
            Shields = (_, _, _, _, _) => { },
        };
        // CD 판본 풀은 거부한다.
        var cd = new Netstorm.Core.Original.RawSidPool(24000, false, edition: Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceRebuild(cd, hooks));
        // 하위 효과가 하나라도 빠지면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceRebuild(pool, new Netstorm.Core.Original.FenceRebuildHooks()));
        Assert.Same(pool, new Netstorm.Core.Original.RawFenceRebuild(pool, hooks).Pool);
    }

    /// <summary>차단된 부모(extra 9)는 이웃을 조회하지 않는다.</summary>
    [Fact]
    public void RebuildSkipsBlockedParent()
    {
        var pool = Scene();
        Place(pool, 50, 10.0f, 10.0f, extra: 9);
        var directions = new System.Collections.Generic.List<uint>();
        var body = new Netstorm.Core.Original.RawFenceRebuild(pool, new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = NeighborHook(directions, 60),
            WithinRange = (_, _) => true,
            Markers = (_, _) => Assert.Fail("차단된 부모는 표식을 만들지 않는다."),
            Shields = (_, _, _, _, _) => Assert.Fail("차단된 부모는 포스 필드를 만들지 않는다."),
        });
        body.Rebuild(50);
        Assert.Empty(directions);
    }

    /// <summary>동맹·사거리·대상 extra를 통과하면 표식 뒤에 현재 owner로 포스 필드를 만든다.</summary>
    [Fact]
    public void RebuildCreatesMarkersThenShieldsWithCurrentOwners()
    {
        var pool = Scene();
        Place(pool, 50, 10.0f, 10.0f, owner: 3);
        Place(pool, 60, 30.0f, 30.0f, owner: 4);
        var directions = new System.Collections.Generic.List<uint>();
        var markers = new System.Collections.Generic.List<(int Parent, int Other)>();
        var shields = new System.Collections.Generic.List<(float[] Points, uint First, uint Second, int Parent, int Other)>();
        var body = new Netstorm.Core.Original.RawFenceRebuild(pool, new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = NeighborHook(directions, 60),
            WithinRange = (_, _) => true,
            Markers = (parent, other) => markers.Add((parent, other)),
            Shields = (points, first, second, parent, other) => shields.Add((points, first, second, parent, other)),
        });
        body.Rebuild(50);
        // 네 방향을 순서대로 조회하고 0번 방향에서만 생성한다.
        Assert.Equal([0u, 2u, 4u, 6u], directions);
        Assert.Equal([(50, 60)], markers);
        Assert.Single(shields);
        Assert.Equal([10.0f, 10.0f, 30.0f, 30.0f], shields[0].Points);
        Assert.Equal(3u, shields[0].First);
        Assert.Equal(4u, shields[0].Second);
    }

    /// <summary>사거리 밖과 차단된 대상(extra 0x21)은 건너뛴다.</summary>
    [Fact]
    public void RebuildSkipsOutOfRangeAndBlockedTarget()
    {
        var pool = Scene();
        Place(pool, 50, 10.0f, 10.0f);
        Place(pool, 60, 30.0f, 30.0f);
        Place(pool, 61, 30.0f, 30.0f, extra: 0x21);
        int created = 0;
        var body = new Netstorm.Core.Original.RawFenceRebuild(pool, new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = NeighborHook(new System.Collections.Generic.List<uint>(), 60),
            WithinRange = (_, _) => false,
            Markers = (_, _) => created++,
            Shields = (_, _, _, _, _) => created++,
        });
        body.Rebuild(50);
        Assert.Equal(0, created);
        // 사거리는 통과하되 대상이 차단되면 역시 만들지 않는다.
        var blocked = new Netstorm.Core.Original.RawFenceRebuild(pool, new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = NeighborHook(new System.Collections.Generic.List<uint>(), 61),
            WithinRange = (_, _) => true,
            Markers = (_, _) => created++,
            Shields = (_, _, _, _, _) => created++,
        });
        blocked.Rebuild(50);
        Assert.Equal(0, created);
    }

    /// <summary>호출 때 빠진 하위 효과는 빈 처리로 넘어가지 않는다.</summary>
    [Fact]
    public void RebuildRequiresConnectedHooksAtCall()
    {
        var pool = Scene();
        Place(pool, 50, 10.0f, 10.0f);
        Place(pool, 60, 30.0f, 30.0f);
        var hooks = new Netstorm.Core.Original.FenceRebuildHooks
        {
            Neighbor = NeighborHook(new System.Collections.Generic.List<uint>(), 60),
            WithinRange = (_, _) => true,
            Markers = (_, _) => { },
            Shields = (_, _, _, _, _) => { },
        };
        var body = new Netstorm.Core.Original.RawFenceRebuild(pool, hooks);
        // 생성 뒤에 빠진 표식 효과는 호출 때 명시 실패한다.
        hooks.Markers = null;
        Assert.Throws<System.InvalidOperationException>(() => body.Rebuild(50));
    }

    /// <summary>저장소 결선은 같은 재생성 본체를 재사용하고 실제 이웃·사거리를 쓴다.</summary>
    [Fact]
    public void StorageFenceRebuildReusesAndUsesRealNeighbors()
    {
        var types = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
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
        // 생성 장치가 없으면 재생성 본체를 만들지 않는다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRebuildDevices(new Netstorm.Core.Original.RawSurfaceFenceRebuildDevices()));
        int created = 0;
        storage.BindFenceRebuildDevices(new Netstorm.Core.Original.RawSurfaceFenceRebuildDevices
        {
            Markers = (_, _) => created++,
            Shields = (_, _, _, _, _) => created++,
        });
        // 같은 본체를 재사용하고 같은 풀을 공유한다.
        Assert.Same(storage.FenceRebuild(), storage.FenceRebuild());
        Assert.Same(storage.Pool, storage.FenceRebuild().Pool);
        // 동맹 읽기가 없으면 실제 이웃 결선에서 거부한다.
        Assert.Throws<System.InvalidOperationException>(() => storage.FenceRebuild().Rebuild(5));
        // 중복 결선은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRebuildDevices(new Netstorm.Core.Original.RawSurfaceFenceRebuildDevices
        {
            Markers = (_, _) => { },
            Shields = (_, _, _, _, _) => { },
        }));
        Assert.Equal(0, created);
    }

    /// <summary>수명 실제 결선은 같은 저장소의 이웃·정리·재생성 본체에 잇는다.</summary>
    [Fact]
    public void StorageLifetimeActualsBindRealBodies()
    {
        var types = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
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
        // 동맹·재생성 생성 장치가 없으면 실제 결선을 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceLifetimeActuals());
        storage.BindFenceAlliances((_, _) => 1u);
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceLifetimeActuals());
        storage.BindFenceRebuildDevices(new Netstorm.Core.Original.RawSurfaceFenceRebuildDevices
        {
            Markers = (_, _) => { },
            Shields = (_, _, _, _, _) => { },
        });
        storage.BindFenceLifetimeActuals();
        // 같은 수명 본체를 만들고 실제 이웃 본체와 같은 풀을 공유한다.
        Assert.Same(storage.FenceNeighbors().Pool, storage.FenceLifetime().Pool);
        // 중복 실제 결선은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceLifetimeActuals());
    }
}
