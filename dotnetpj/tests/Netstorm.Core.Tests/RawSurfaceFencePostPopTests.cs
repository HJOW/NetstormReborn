// 펜스 후처리·정기 처리 본체와 저장소 결선의 순서·분배 계약을 검사한다.
// cpppj/src/o/RawFencePostPop.h·cpp·o/RawFenceRegular.h·cpp 와 client/RawSurfaceFence.cpp 의 FencePostPop/FenceRegular 장기 본체를 대조한다.
// 0.592에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 후처리·정기 처리)
namespace Netstorm.Core.Tests;

/// <summary>후처리의 접두 순서와 정기 처리의 사건 분기를 확인한다.</summary>
public sealed class RawSurfaceFencePostPopTests
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

    /// <summary>후처리 검사 크기의 풀을 만든다.</summary>
    private static Netstorm.Core.Original.RawSidPool Scene()
    {
        var pool = new Netstorm.Core.Original.RawSidPool(24000, false);
        // 5번부터 120번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 120; id++)
        {
            Assert.Equal(id, pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client));
        }

        return pool;
    }

    /// <summary>가상 표·타입·좌표·owner·extra를 기록한다.</summary>
    private static void Place(Netstorm.Core.Original.RawSidPool pool, int sid, byte type, float x, float y, byte owner = 1, byte extra = 0)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        Write(raw, 0, 0x00509470u);
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = type;
        raw[Netstorm.Core.Original.RawSidPool.StateOffset] = 0;
        raw[34] = owner;
        raw[40] = extra;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y));
    }

    /// <summary>후처리·정기 처리용 188 타입·프레임 표를 만든다.</summary>
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

    /// <summary>판본이 어긋나면 후처리 본체를 만들지 않는다.</summary>
    [Fact]
    public void PostPopConstructorRejectsBadPool()
    {
        var pool = Scene();
        var hooks = new Netstorm.Core.Original.FencePostPopHooks
        {
            Theme = _ => 0,
            Neighbor = (_, _) => 0,
            ReserveRegular = () => false,
            AddRegular = (_, _, _) => { },
            ClearLine = _ => { },
            Connections = _ => 0,
            Damageable = (_, _) => { },
        };
        var cd = new Netstorm.Core.Original.RawSidPool(24000, false, edition: Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFencePostPop(cd, hooks));
        Assert.Same(pool, new Netstorm.Core.Original.RawFencePostPop(pool, hooks).Pool);
    }

    /// <summary>가상 표가 00509470일 때만 맡는다.</summary>
    [Fact]
    public void PostPopHandlesChecksVtable()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f);
        var body = new Netstorm.Core.Original.RawFencePostPop(pool, new Netstorm.Core.Original.FencePostPopHooks());
        Assert.True(body.Handles(50));
        pool.AllocatedBytes(50)[0] = 1;
        Assert.False(body.Handles(50));
    }

    /// <summary>다른 가상 표는 Validate에서 거부하고 투영은 그대로 전달한다.</summary>
    [Fact]
    public void PostPopValidateRejectsForeignTable()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f);
        var seen = new System.Collections.Generic.List<(int Sid, uint Flags)>();
        var body = new Netstorm.Core.Original.RawFencePostPop(pool, new Netstorm.Core.Original.FencePostPopHooks
        {
            ValidateBase = (sid, flags, _) => seen.Add((sid, flags)),
        });
        var projection = new Netstorm.Core.Original.PopProjection(1.0f, 2.0f, 0, new Netstorm.Core.Original.OriginalTypeRecord());
        body.Validate(50, 1, projection);
        Assert.Equal([(50, 1u)], seen);
        pool.AllocatedBytes(50)[0] = 1;
        Assert.Throws<System.ArgumentException>(() => body.Validate(50, 1, projection));
    }

    /// <summary>flags 0이면 접두를 건너뛰고 같은 Damageable로 끝난다.</summary>
    [Fact]
    public void PostPopFlagsZeroSkipsPrefix()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f);
        var calls = new System.Collections.Generic.List<string>();
        var body = new Netstorm.Core.Original.RawFencePostPop(pool, new Netstorm.Core.Original.FencePostPopHooks
        {
            Theme = _ => { calls.Add("theme"); return 3; },
            Neighbor = (_, _) => { calls.Add("neighbor"); return 0; },
            ReserveRegular = () => { calls.Add("reserve"); return true; },
            AddRegular = (_, _, _) => calls.Add("add"),
            ClearLine = _ => calls.Add("clear"),
            Connections = _ => { calls.Add("conn"); return 0; },
            Damageable = (_, flags) => calls.Add($"dmg:{flags}"),
        });
        body.PostPop(50, 0);
        Assert.Equal(["dmg:0"], calls);
        // 빠진 Damageable은 선택 위치에서 명시 실패한다.
        var missing = new Netstorm.Core.Original.RawFencePostPop(pool, new Netstorm.Core.Original.FencePostPopHooks());
        Assert.Throws<System.InvalidOperationException>(() => missing.PostPop(50, 0));
    }

    /// <summary>flags 1이면 주제·이웃·캐시·쌍 정리·연결 수·예약 순서대로 실행한다.</summary>
    [Fact]
    public void PostPopFlagsOneRunsPrefixInOrder()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f, owner: 3);
        Write(pool.AllocatedBytes(50), 46, 0u);
        var events = new System.Collections.Generic.List<string>();
        var lines = new System.Collections.Generic.List<float[]>();
        var body = new Netstorm.Core.Original.RawFencePostPop(pool, new Netstorm.Core.Original.FencePostPopHooks
        {
            Theme = sid => { events.Add($"theme:{sid}"); return 3; },
            Neighbor = (_, direction) => { events.Add($"n{direction}"); return (int)(70 + direction / 2); },
            ReserveRegular = () => { events.Add("reserve"); return true; },
            AddRegular = (sid, @event, payload) => events.Add($"add:{sid}:{@event}:{payload}"),
            ClearLine = points => { events.Add("clear"); lines.Add(points); },
            Connections = sid => { events.Add($"conn:{sid}"); return 5; },
            Damageable = (sid, flags) => events.Add($"dmg:{sid}:{flags}"),
        });
        body.PostPop(50, 1);
        Assert.Equal([
            "theme:50", "reserve", "add:50:0:0.1",
            "n0", "n2", "n4", "n6",
            "reserve", "add:70:3:0.1", "reserve", "add:72:3:0.1", "clear",
            "reserve", "add:71:3:0.1", "reserve", "add:73:3:0.1", "clear",
            "conn:50", "reserve", "add:50:2:0.1", "dmg:50:1",
        ], events);
        // owner 캐시(+2a)와 연결 수(+2e)를 기록한다.
        Assert.Equal(3u, (uint)(pool.Slot(50)[42] | (pool.Slot(50)[43] << 8) | (pool.Slot(50)[44] << 16) | (pool.Slot(50)[45] << 24)));
        Assert.Equal(5u, (uint)(pool.Slot(50)[46] | (pool.Slot(50)[47] << 8) | (pool.Slot(50)[48] << 16) | (pool.Slot(50)[49] << 24)));
        Assert.Equal(2, lines.Count);
    }

    /// <summary>판본·프레임·지정기 계약이 어긋나면 정기 처리 본체를 만들지 않는다.</summary>
    [Fact]
    public void RegularConstructorRejectsBadContracts()
    {
        var pool = Scene();
        var (types, frames) = Tables();
        var hooks = new Netstorm.Core.Original.SquidFrameHooks
        {
            FrameSize = (_, _) => [8.0f, 8.0f],
            Update = (_, _) => { },
            Unpop = (_, _) => { },
            Pop = (_, _, _, _) => { },
        };
        var setter = new Netstorm.Core.Original.SquidFrame(pool, types, hooks);
        var full = new Netstorm.Core.Original.FenceRegularHooks
        {
            Neighbor = (_, _) => 0,
            ClearMarkers = (_, _) => { },
            ClearLine = _ => { },
            Connections = _ => 0,
            ReserveRegular = () => false,
            AddRegular = (_, _, _) => { },
            RefreshNeighbors = _ => { },
            Rebuild = _ => { },
            Damageable = (_, _, _, _) => 0.0f,
        };
        var cd = new Netstorm.Core.Original.RawSidPool(24000, false, edition: Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceRegular(cd, frames, setter, full));
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceRegular(
            pool, new Netstorm.Core.Original.RiftTypeFrames[100], setter, full));
        // 다른 풀의 지정기는 거부한다.
        var other = new Netstorm.Core.Original.SquidFrame(Scene(), types, hooks);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceRegular(pool, frames, other, full));
        Assert.Same(pool, new Netstorm.Core.Original.RawFenceRegular(pool, frames, setter, full).Pool);
    }

    /// <summary>다른 가상 표와 bit20·사건 분기를 확인한다.</summary>
    [Fact]
    public void RegularHandleDispatchesEvents()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f);
        var (types, frames) = Tables();
        var setter = new Netstorm.Core.Original.SquidFrame(pool, types, new Netstorm.Core.Original.SquidFrameHooks
        {
            FrameSize = (_, _) => [8.0f, 8.0f],
            Update = (_, _) => { },
            Unpop = (_, _) => { },
            Pop = (_, _, _, _) => { },
        });
        var calls = new System.Collections.Generic.List<string>();
        var body = new Netstorm.Core.Original.RawFenceRegular(pool, frames, setter, new Netstorm.Core.Original.FenceRegularHooks
        {
            Neighbor = (_, _) => 0,
            ClearMarkers = (_, _) => calls.Add("markers"),
            ClearLine = _ => calls.Add("line"),
            Connections = _ => 0,
            ReserveRegular = () => { calls.Add("reserve"); return false; },
            AddRegular = (_, _, _) => calls.Add("add"),
            RefreshNeighbors = sid => calls.Add($"refresh:{sid}"),
            Rebuild = sid => calls.Add($"rebuild:{sid}"),
            Damageable = (sid, @event, count, payload) => { calls.Add($"dmg:{sid}:{@event}:{count}:{payload}"); return -1.0f; },
        });
        // 다른 가상 표는 거부한다.
        pool.AllocatedBytes(50)[0] = 1;
        Assert.Throws<System.ArgumentException>(() => body.Handle(50, 0, 0, 0.0f));
        Assert.False(body.Handles(50));
        pool.AllocatedBytes(50)[0] = 0x70;
        Assert.True(body.Handles(50));
        // bit20이 서 있으면 Damageable에 그대로 넘긴다.
        pool.AllocatedBytes(50)[40] = 0x20;
        Assert.Equal(-1.0f, body.Handle(50, 7, 3, 0.5f));
        Assert.Equal(["dmg:50:7:3:0.5"], calls);
        pool.AllocatedBytes(50)[40] = 0;
        // 사건 1·2는 갱신·재생성을 부르고 0을 돌려준다.
        Assert.Equal(0.0f, body.Handle(50, 1, 0, 0.0f));
        Assert.Equal(0.0f, body.Handle(50, 2, 0, 0.0f));
        // 사건 3은 캐시를 갱신하지 않고 빈 x87 NaN을 돌려준다.
        Write(pool.AllocatedBytes(50), 42, 9u);
        Assert.Equal(0xffc00000u, BitConverter.SingleToUInt32Bits(body.Handle(50, 3, 0, 0.0f)));
        Assert.Equal(9u, (uint)(pool.Slot(50)[42] | (pool.Slot(50)[43] << 8)));
        // 그 밖의 사건은 Damageable에 넘긴다.
        Assert.Equal(-1.0f, body.Handle(50, 9, 0, 0.0f));
        Assert.Contains("refresh:50", calls);
        Assert.Contains("rebuild:50", calls);
    }

    /// <summary>사건 0은 프레임을 전진시키고 owner·연결 수 변경 때만 정리한다.</summary>
    [Fact]
    public void RegularHandleEventZeroAdvancesFrame()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f, owner: 3);
        var (types, frames) = Tables();
        // 12번 타입 0번 프레임 방향을 B로 둔다.
        frames[12] = new Netstorm.Core.Original.RiftTypeFrames(
            [new Netstorm.Core.Original.RiftFrameCode((byte)'B', 0, 0, 0), new Netstorm.Core.Original.RiftFrameCode((byte)'B', 1, 1, 0)]);
        var setter = new Netstorm.Core.Original.SquidFrame(pool, types, new Netstorm.Core.Original.SquidFrameHooks
        {
            FrameSize = (_, _) => [8.0f, 8.0f],
            Update = (_, _) => { },
            Unpop = (_, _) => { },
            Pop = (_, _, _, _) => { },
        });
        var connections = 0;
        var cleared = 0;
        var added = new System.Collections.Generic.List<(int Sid, uint Event)>();
        var body = new Netstorm.Core.Original.RawFenceRegular(pool, frames, setter, new Netstorm.Core.Original.FenceRegularHooks
        {
            Neighbor = (_, direction) => direction == 0 ? 60 : 0,
            ClearMarkers = (_, _) => { },
            ClearLine = _ => cleared++,
            Connections = _ => { connections++; return 7; },
            ReserveRegular = () => true,
            AddRegular = (sid, @event, _) => added.Add((sid, @event)),
            RefreshNeighbors = _ => Assert.Fail("사건 0은 갱신하지 않는다."),
            Rebuild = _ => Assert.Fail("사건 0은 재생성하지 않는다."),
            Damageable = (_, _, _, _) => { Assert.Fail("사건 0은 Damageable에 넘기지 않는다."); return 0.0f; },
        });
        // owner 캐시가 다르면 정리하고 연결 수를 기록한다.
        Write(pool.AllocatedBytes(50), 42, 9u);
        Write(pool.AllocatedBytes(50), 46, 0u);
        Write(pool.AllocatedBytes(50), 36, 0u);
        Assert.Equal(0.1f, body.Handle(50, 0, 0, 0.0f));
        Assert.Equal(3u, (uint)(pool.Slot(50)[42] | (pool.Slot(50)[43] << 8)));
        Assert.Equal(7u, (uint)(pool.Slot(50)[46] | (pool.Slot(50)[47] << 8)));
        Assert.Equal(2, connections);
        Assert.True(cleared >= 1);
        Assert.Contains((50, 2u), added);
        // 프레임 표를 벗어난 TYPE은 거부한다.
        pool.AllocatedBytes(50)[Netstorm.Core.Original.RawSidPool.TypeOffset] = 200;
        Assert.Throws<System.ArgumentOutOfRangeException>(() => body.Handle(50, 0, 0, 0.0f));
    }

    /// <summary>fallback 분배는 맡는 표만 처리한다.</summary>
    [Fact]
    public void RegularHandlerFallsBack()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f);
        var (types, frames) = Tables();
        var setter = new Netstorm.Core.Original.SquidFrame(pool, types, new Netstorm.Core.Original.SquidFrameHooks
        {
            FrameSize = (_, _) => [8.0f, 8.0f],
            Update = (_, _) => { },
            Unpop = (_, _) => { },
            Pop = (_, _, _, _) => { },
        });
        var body = new Netstorm.Core.Original.RawFenceRegular(pool, frames, setter, new Netstorm.Core.Original.FenceRegularHooks
        {
            Neighbor = (_, _) => 0,
            ClearMarkers = (_, _) => { },
            ClearLine = _ => { },
            Connections = _ => 0,
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => { },
            RefreshNeighbors = _ => { },
            Rebuild = _ => { },
            Damageable = (_, _, _, _) => -2.0f,
        });
        // bit20 표식은 Damageable 값으로 끝난다.
        pool.AllocatedBytes(50)[40] = 0x20;
        Assert.Equal(-2.0f, body.Handler()(50, 9, 0, 0.0f));
        pool.AllocatedBytes(50)[40] = 0;
        pool.AllocatedBytes(50)[0] = 1;
        // 맡지 않는 표는 fallback이 없으면 명시 실패한다.
        Assert.Throws<System.InvalidOperationException>(() => body.Handler()(50, 9, 0, 0.0f));
        Assert.Equal(9.0f, body.Handler((_, _, _, _) => 9.0f)(50, 9, 0, 0.0f));
    }

    /// <summary>저장소 결선은 같은 본체를 재사용하고 실제 이웃·장치에 잇는다.</summary>
    [Fact]
    public void StoragePostPopAndRegularReuseRealBodies()
    {
        var (types, frames) = Tables();
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
        storage.BindDisplayDevices(new Netstorm.Core.Original.RawSurfaceDisplayBindings
        {
            View = () => new Netstorm.Core.Original.SquidDisplayView { Zoom = 65536, Viewport = new Netstorm.Core.Display.DisplayRect(0, 0, 1024, 768) },
            Suppressed = () => false,
            Invalidate = (_, _) => { },
        });
        storage.BindDisplayDiagnostics(new Netstorm.Core.Original.StringBox(), (_, _, _) => { });
        // 정기 처리 장치가 없으면 본체를 만들지 않는다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRegularDevices(new Netstorm.Core.Original.RawSurfaceFenceRegularDevices()));
        Assert.Same(storage.FencePostPop(), storage.FencePostPop());
        Assert.Same(storage.Pool, storage.FencePostPop().Pool);
        Assert.Throws<System.InvalidOperationException>(() => storage.FenceRegular());
        storage.BindFenceRegularDevices(new Netstorm.Core.Original.RawSurfaceFenceRegularDevices
        {
            Damageable = (_, _, _, _) => 0.0f,
        });
        Assert.Same(storage.FenceRegular(), storage.FenceRegular());
        Assert.Same(storage.Pool, storage.FenceRegular().Pool);
        // 중복 결선은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRegularDevices(new Netstorm.Core.Original.RawSurfaceFenceRegularDevices
        {
            Damageable = (_, _, _, _) => 0.0f,
        }));
    }
}
