// 펜스 표식·포스 필드 생성 본체와 재생성 실제 생성 결선을 검사한다.
// cpppj/src/o/RawFenceCreate.h·cpp 와 client/RawSurfaceFence.cpp 의 FenceMarkers/FenceShields 장기 본체를 대조한다.
// 0.591에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 생성)
namespace Netstorm.Core.Tests;

/// <summary>표식 태그·포스 필드 프레임과 저장소 실제 생성 결선의 계약을 확인한다.</summary>
public sealed class RawSurfaceFenceCreateTests
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

    /// <summary>생성 검사 크기의 풀을 만든다.</summary>
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

    /// <summary>슬롯 좌표·타입·owner를 기록한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">기록할 번호.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    /// <param name="owner">소유자 바이트.</param>
    private static void Place(Netstorm.Core.Original.RawSidPool pool, int sid, byte type, float x, float y, byte owner = 1)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = type;
        raw[Netstorm.Core.Original.RawSidPool.StateOffset] = 0;
        raw[34] = owner;
        raw[40] = 0;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y));
    }

    /// <summary>펜스 생성용 188 타입·프레임 표를 만든다.</summary>
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

    /// <summary>판본·표·효과 계약이 어긋나면 본체를 만들지 않는다.</summary>
    [Fact]
    public void CreateConstructorsRejectBadContracts()
    {
        var pool = Scene();
        var (types, frames) = Tables();
        var markerType = new Netstorm.Core.Original.UIntBox() { Value = 90u };
        var shieldType = new Netstorm.Core.Original.UIntBox() { Value = 170u };
        var full = new Netstorm.Core.Original.FenceCreateHooks
        {
            Create = (_, _) => 0,
            NotifySurface = _ => { },
            SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { },
        };
        // CD 판본 풀은 거부한다.
        var cd = new Netstorm.Core.Original.RawSidPool(24000, false, edition: Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceMarkerCreate(cd, markerType, full, _ => [0.0f, 0.0f]));
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceShieldCreate(cd, frames, shieldType, full));
        // 프레임 표가 188개가 아니면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceShieldCreate(
            pool, new Netstorm.Core.Original.RiftTypeFrames[100], shieldType, full));
        // 하위 효과가 빠지면 거부한다.
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceMarkerCreate(
            pool, markerType, new Netstorm.Core.Original.FenceCreateHooks(), _ => [0.0f, 0.0f]));
        Assert.Same(pool, new Netstorm.Core.Original.RawFenceMarkerCreate(pool, markerType, full, _ => [0.0f, 0.0f]).Pool);
    }

    /// <summary>표식 지점은 방향·inclusive 경계·4칸 구간 변경을 지킨다.</summary>
    [Fact]
    public void MarkerPointsKeepsDirectionAndCellChanges()
    {
        // 아래쪽 수직 이동은 Y만 한 칸씩 전진한다.
        var vertical = Netstorm.Core.Original.RawFenceMarkerCreate.Points([10.0f, 10.0f, 10.0f, 30.0f]);
        Assert.NotEmpty(vertical);
        Assert.All(vertical, point => Assert.Equal(10.0f, point[0]));
        Assert.Equal(vertical.OrderBy(point => point[1]).ToArray(), vertical.ToArray());
        // 오른쪽 수평 이동은 X만 한 칸씩 전진한다.
        var horizontal = Netstorm.Core.Original.RawFenceMarkerCreate.Points([10.0f, 10.0f, 30.0f, 10.0f]);
        Assert.NotEmpty(horizontal);
        Assert.All(horizontal, point => Assert.Equal(10.0f, point[1]));
        // 동률은 세로를 우선한다.
        var tie = Netstorm.Core.Original.RawFenceMarkerCreate.Points([10.0f, 10.0f, 30.0f, 30.0f]);
        Assert.NotEmpty(tie);
        Assert.Equal(10.0f, tie[0][0]);
        // 비유한 좌표와 잘못된 개수는 거부한다.
        Assert.Throws<System.ArgumentOutOfRangeException>(() => Netstorm.Core.Original.RawFenceMarkerCreate.Points([float.NaN, 0.0f, 0.0f, 0.0f]));
        Assert.Throws<System.ArgumentException>(() => Netstorm.Core.Original.RawFenceMarkerCreate.Points([0.0f, 0.0f, 0.0f]));
    }

    /// <summary>표식 생성은 QA·통지·태그·owner·Pop 순서를 지킨다.</summary>
    [Fact]
    public void MarkerCreateWritesTagsAndPopsInOrder()
    {
        var pool = Scene();
        Place(pool, 50, 12, 10.0f, 10.0f, owner: 3);
        Place(pool, 60, 12, 30.0f, 30.0f, owner: 4);
        var expected = Netstorm.Core.Original.RawFenceMarkerCreate.Points([10.0f, 10.0f, 30.0f, 30.0f]);
        Assert.NotEmpty(expected);
        int next = 61;
        var created = new System.Collections.Generic.List<int>();
        var notified = new System.Collections.Generic.List<int>();
        var owners = new System.Collections.Generic.List<(int Sid, uint Owner)>();
        var pops = new System.Collections.Generic.List<(int Sid, float X, float Y, uint Flags)>();
        var body = new Netstorm.Core.Original.RawFenceMarkerCreate(
            pool,
            new Netstorm.Core.Original.UIntBox() { Value = 90u },
            new Netstorm.Core.Original.FenceCreateHooks
            {
                Create = (type, flags) =>
                {
                    Assert.Equal(90u, type);
                    Assert.Equal(2u, flags);
                    int sid = next++;
                    pool.AllocatedBytes(sid)[Netstorm.Core.Original.RawSidPool.TypeOffset] = 90;
                    created.Add(sid);
                    return sid;
                },
                NotifySurface = notified.Add,
                SetOwner = (sid, owner) => owners.Add((sid, owner)),
                Pop = (sid, x, y, flags) => pops.Add((sid, x, y, flags)),
            },
            sid => sid == 50 ? [10.0f, 10.0f] : [30.0f, 30.0f]);
        body.Create(50, 60);
        Assert.Equal(expected.Count, created.Count);
        // QA WORD는 부모, packed 태그는 이웃을 가리킨다.
        foreach (int sid in created)
        {
            System.ReadOnlySpan<byte> raw = pool.Slot(sid);
            Assert.Equal(50u, (uint)(raw[12] | (raw[13] << 8)));
            Assert.Equal(60u, (uint)((raw[26] << 8) | raw[30]));
        }

        // QA를 쓴 뒤 통지하고 현재 owner로 지정한다.
        Assert.Equal(created, notified);
        Assert.All(owners, entry => Assert.Equal(3u, entry.Owner));
        Assert.Equal(expected.Count, pops.Count);
        // Pop 좌표는 계산된 지점과 순서대로 같고 flags는 0x40이다.
        for (int index = 0; index < expected.Count; index++)
        {
            Assert.Equal(created[index], pops[index].Sid);
            Assert.Equal(expected[index][0], pops[index].X);
            Assert.Equal(expected[index][1], pops[index].Y);
            Assert.Equal(0x40u, pops[index].Flags);
        }

        // 새 SID가 없으면 명시 실패한다.
        var missing = new Netstorm.Core.Original.RawFenceMarkerCreate(
            pool,
            new Netstorm.Core.Original.UIntBox() { Value = 90u },
            new Netstorm.Core.Original.FenceCreateHooks
            {
                Create = (_, _) => 0,
                NotifySurface = _ => { },
                SetOwner = (_, _) => { },
                Pop = (_, _, _, _) => { },
            },
            sid => sid == 50 ? [10.0f, 10.0f] : [30.0f, 30.0f]);
        Assert.Throws<System.InvalidOperationException>(() => missing.Create(50, 60));
    }

    /// <summary>포스 필드 지점은 mode0과 같은 보정으로 100개를 넘지 않는다.</summary>
    [Fact]
    public void ShieldPointsKeepsCorrectionAndLimit()
    {
        // 같은 끝점 수직 보정 뒤 한 점만 낸다.
        var single = Netstorm.Core.Original.RawFenceShieldCreate.Points([20.0f, 20.0f, 20.0f, 20.0f]);
        Assert.Single(single);
        // 긴 수평선도 100개를 넘지 않는다.
        var capped = Netstorm.Core.Original.RawFenceShieldCreate.Points([10.0f, 10.0f, 300.0f, 10.0f]);
        Assert.True(capped.Count <= 100);
        Assert.True(capped.Count > 1);
        // 비유한 좌표와 잘못된 개수는 거부한다.
        Assert.Throws<System.ArgumentOutOfRangeException>(() => Netstorm.Core.Original.RawFenceShieldCreate.Points([float.NaN, 0.0f, 0.0f, 0.0f]));
        Assert.Throws<System.ArgumentException>(() => Netstorm.Core.Original.RawFenceShieldCreate.Points([0.0f, 0.0f, 0.0f]));
    }

    /// <summary>포스 필드 생성은 J/K 첫 프레임·owner 선택·bit20 종료를 지킨다.</summary>
    [Fact]
    public void ShieldCreateWritesFirstFrameAndSelectsOwner()
    {
        var pool = Scene();
        var (_, frames) = Tables();
        // 12번 타입의 첫 프레임 방향을 J로 둔다.
        frames[12] = new Netstorm.Core.Original.RiftTypeFrames(
            [new Netstorm.Core.Original.RiftFrameCode((byte)'J', 0, 0, 0), new Netstorm.Core.Original.RiftFrameCode((byte)'J', 1, 1, 0)]);
        int next = 61;
        var owners = new System.Collections.Generic.List<uint>();
        var pops = new System.Collections.Generic.List<(float X, float Y)>();
        var body = new Netstorm.Core.Original.RawFenceShieldCreate(
            pool,
            frames,
            new Netstorm.Core.Original.UIntBox() { Value = 170u },
            new Netstorm.Core.Original.FenceCreateHooks
            {
                Create = (type, flags) =>
                {
                    Assert.Equal(170u, type);
                    Assert.Equal(2u, flags);
                    int sid = next++;
                    System.Span<byte> raw = pool.AllocatedBytes(sid);
                    raw.Clear();
                    raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = 12;
                    return sid;
                },
                SetOwner = (_, owner) => owners.Add(owner),
                Pop = (_, x, y, _) => pops.Add((x, y)),
            });
        // 수직 끝점은 J 프레임을 쓰고 owner가 다르면 둘째 SID가 작지 않을 때 둘째 owner를 쓴다.
        body.Create([10.0f, 10.0f, 10.0f, 12.0f], 3u, 4u, 40, 50);
        Assert.NotEmpty(owners);
        Assert.All(owners, owner => Assert.Equal(4u, owner));
        Assert.Equal((uint)frames[12].Run((byte)'J').First, (uint)(pool.Slot(61)[36] | (pool.Slot(61)[37] << 8) | (pool.Slot(61)[38] << 16) | (pool.Slot(61)[39] << 24)));
        Assert.Equal(pops.Count, owners.Count);
        // 생성 직후 bit20이 서 있으면 즉시 종료한다.
        int before = owners.Count;
        var stopped = new Netstorm.Core.Original.RawFenceShieldCreate(
            pool,
            frames,
            new Netstorm.Core.Original.UIntBox() { Value = 170u },
            new Netstorm.Core.Original.FenceCreateHooks
            {
                Create = (_, _) =>
                {
                    System.Span<byte> raw = pool.AllocatedBytes(next);
                    raw.Clear();
                    raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = 12;
                    raw[40] = 0x20;
                    return next++;
                },
                SetOwner = (_, owner) => owners.Add(owner),
                Pop = (_, _, _, _) => { },
            });
        stopped.Create([10.0f, 10.0f, 10.0f, 12.0f], 3u, 3u, 50, 60);
        Assert.Equal(before, owners.Count);
        // 현재 TYPE이 표를 벗어나면 거부한다.
        var wild = new Netstorm.Core.Original.RawFenceShieldCreate(
            pool,
            frames,
            new Netstorm.Core.Original.UIntBox() { Value = 170u },
            new Netstorm.Core.Original.FenceCreateHooks
            {
                Create = (_, _) =>
                {
                    System.Span<byte> raw = pool.AllocatedBytes(next);
                    raw.Clear();
                    raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = 200;
                    return next++;
                },
                SetOwner = (_, _) => { },
                Pop = (_, _, _, _) => { },
            });
        Assert.Throws<System.ArgumentOutOfRangeException>(() => wild.Create([10.0f, 10.0f, 10.0f, 12.0f], 3u, 3u, 50, 60));
    }

    /// <summary>저장소 결선은 같은 생성 본체를 재사용하고 역할을 공유한다.</summary>
    [Fact]
    public void StorageCreateReusesAndSharesRoles()
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
        Assert.Same(storage.FenceMarkers(), storage.FenceMarkers());
        Assert.Same(storage.FenceShields(), storage.FenceShields());
        Assert.Same(storage.Pool, storage.FenceMarkers().Pool);
        Assert.Same(storage.Pool, storage.FenceShields().Pool);
        // 역할 상자는 원본 초기값을 공유한다.
        Assert.Equal(90u, storage.FenceMarkers().MarkerType);
        Assert.Same(storage.GameAux().FenceMarkerTypeBox, storage.GameAux().FenceMarkerTypeBox);
    }

    /// <summary>재생성 실제 생성 결선은 순서 강제 뒤 표식·포스 필드 본체에 잇는다.</summary>
    [Fact]
    public void StorageRebuildActualCreationsBindsRealBodies()
    {
        var (types, frames) = Tables();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
        // 전송 장치 전에는 실제 결선을 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRebuildActualCreations());
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindFenceRebuildActualCreations();
        Assert.Same(storage.FenceMarkers().Pool, storage.FenceRebuild().Pool);
        // 중복 결선은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceRebuildActualCreations());
    }
}
