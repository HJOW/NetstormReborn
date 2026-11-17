// 펜스 수명 본체와 장기 저장소 결선·삭제 분배 선택을 검사한다.
// cpppj/src/o/RawFenceLifetime.cpp 와 client/RawSurfaceFence.cpp 의 FenceLifetime 장기 본체를 대조한다.
// 0.587에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 우선)
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>펜스 이웃 스냅샷·정리·재생성·삭제 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceFenceLifetimeTests
{
    /// <summary>실제 최초 자산 표를 빌리는 독립 원천이다.</summary>
    private sealed class StubSource(System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> types) : Netstorm.Core.Original.IRawSurfaceAssetSource
    {
        /// <summary>실제 최초 로딩 표다.</summary>
        public System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> LoadedTypes { get; } = types;
    }

    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage(bool withSource = true)
    {
        // 독립 검사마다 같은 188 TYPE 표를 만든다.
        var types = Enumerable.Range(0, 188).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = Netstorm.Core.Original.OriginalConstructors.Address(index),
            Flags1 = Netstorm.Core.Original.OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        // 프레임과 셰이프도 같은 길이로 맞춘다.
        var frames = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        // 원천이 있으면 실제 최초 로딩 표를 빌린다.
        Netstorm.Core.Original.IRawSurfaceAssetSource? source = withSource ? new StubSource(types) : null;
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes, assetSource: source);
        storage.GameAux().SquidTransmission.Compress = 0;
        storage.GameAux().SquidTransmission.ClientSidAllowed = 1;
        storage.BindDisplayDevices(new Netstorm.Core.Original.RawSurfaceDisplayBindings
        {
            View = () => new Netstorm.Core.Original.SquidDisplayView { Zoom = 65536, Viewport = new Netstorm.Core.Display.DisplayRect(0, 0, 1024, 768) },
            Suppressed = () => false,
            Invalidate = (_, _) => { },
        });
        storage.BindDisplayDiagnostics(new Netstorm.Core.Original.StringBox(), (_, _, _) => { });
        return storage;
    }

    /// <summary>펜스까지의 전체 결선이다.</summary>
    private static void BindAll(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        // Native 전송 장치를 먼저 연결한다.
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        // 삭제 분배의 현재 선택 장치를 연결한다.
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        // 내용물·보상·후처리·삭제·직렬화·피해·자원 장치를 순서대로 연결한다.
        storage.BindContentDevices(new Netstorm.Core.Original.RawSurfaceContentDevices
        {
            Cursor = new Netstorm.Core.Original.UIntBox() { Value = 0 },
            LocalOwner = new Netstorm.Core.Original.UIntBox() { Value = 1 },
            ContentType = new Netstorm.Core.Original.UIntBox() { Value = 6 },
            Effects = new Netstorm.Core.Original.ContentLifetimeHooks
            {
                CancelCursor = () => { },
                LocalNotice = _ => { },
                RemoteNotice = _ => { },
                Sound = (_, _) => { },
            },
        });
        storage.BindRewardDevices(new Netstorm.Core.Original.RawSurfaceRewardDevices
        {
            Sp = new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u),
            Editor = new Netstorm.Core.Original.UIntBox(),
            RefreshSp = () => { },
            CreditWallet = (_, _) => { },
        });
        storage.BindPostPopDevices(new Netstorm.Core.Original.SquidPostPopDevices
        {
            AiNotice = (_, _, _) => { },
            LocalOwner = () => 1u,
            RecheckSelection = () => { },
        });
        storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        });
        storage.BindSerializationGlobals(new Netstorm.Core.Original.FortSerializationGlobals());
        storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox() { Value = 0x40480002u },
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        });
        storage.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (_, _) => true,
            GeyserRegistration = (_, _) => { },
        });
    }

    /// <summary>펜스 이웃·정리·재생성 장치를 기록용으로 연결한다.</summary>
    private static System.Collections.Generic.List<(int Sid, uint Direction, bool Allied)> BindFence(
        Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        // 이웃 조회 호출 순서를 기록한다.
        var neighbors = new System.Collections.Generic.List<(int Sid, uint Direction, bool Allied)>();
        storage.BindFenceLifetimeDevices(new Netstorm.Core.Original.RawSurfaceFenceLifetimeDevices
        {
            Neighbor = (sid, direction, allied) =>
            {
                neighbors.Add((sid, direction, allied));
                return 0;
            },
            ClearMarkers = (_, _) => { },
            ClearLine = (_, _, _, _, _) => { },
            Rebuild = _ => { },
        });
        return neighbors;
    }

    /// <summary>지정한 VT·타입·상태를 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        uint table,
        byte type = 74,
        byte extra = 0,
        byte state = 0)
    {
        // 새 Client 슬롯을 할당한다.
        int sid = storage.Pool.Allocate(SidAllocation.Client);
        System.Span<byte> raw = storage.Pool.RawSlot(sid);
        // 가상 표 DWORD를 낮은 바이트부터 기록한다.
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        // 타입·상태·추가입력을 기록한다.
        raw[10] = type;
        raw[11] = state;
        raw[40] = extra;
        // 좌표와 소유자를 고정값으로 기록한다.
        WriteSingle(raw, 14, 10.0f);
        WriteSingle(raw, 18, 20.0f);
        raw[34] = 1;
        return sid;
    }

    /// <summary>슬롯의 비정렬 float를 낮은 바이트부터 기록한다.</summary>
    private static void WriteSingle(System.Span<byte> raw, int offset, float value)
    {
        // 비트를 그대로 복사한다.
        byte[] bits = BitConverter.GetBytes(value);
        // 네 바이트를 순서대로 쓴다.
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = bits[index];
        }
    }

    /// <summary>모든 효과가 있어야 본체를 만든다.</summary>
    [Fact]
    public void FenceLifetimeRequiresAllHooks()
    {
        // 독립 풀을 빌린다.
        var storage = Storage();
        var pool = storage.Pool;
        // 이웃이 빠진 묶음은 거부한다.
        var missing = new Netstorm.Core.Original.FenceLifetimeHooks
        {
            ClearMarkers = (_, _) => { },
            ClearLine = (_, _, _, _, _) => { },
            Unpop = (_, _) => { },
            Repop = (_, _) => { },
            Rebuild = _ => { },
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => { },
            Damageable = (_, _) => { },
        };
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Core.Original.RawFenceLifetime(pool, missing));
    }

    /// <summary>차단 비트가 있으면 이웃을 찾지 않는다.</summary>
    [Fact]
    public void ClearSkipsWhenBlocked()
    {
        // 독립 풀과 기록기를 준비한다.
        var storage = Storage();
        int parent = AssetWithVtable(storage, 0x00509470u, extra: 9);
        int calls = 0;
        var body = new Netstorm.Core.Original.RawFenceLifetime(storage.Pool, new Netstorm.Core.Original.FenceLifetimeHooks
        {
            Neighbor = (_, _, _) => { calls++; return 0; },
            ClearMarkers = (_, _) => Assert.Fail("차단 시 표식을 지우지 않는다."),
            ClearLine = (_, _, _, _, _) => Assert.Fail("차단 시 연결선을 지우지 않는다."),
            Unpop = (_, _) => { },
            Repop = (_, _) => { },
            Rebuild = _ => Assert.Fail("차단 시 재생성하지 않는다."),
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => Assert.Fail("차단 시 Regular를 붙이지 않는다."),
            Damageable = (_, _) => { },
        });
        body.Clear(parent);
        Assert.Equal(0, calls);
    }

    /// <summary>동맹 이웃 네 방향을 순서대로 정리한다.</summary>
    [Fact]
    public void ClearVisitsAlliedNeighborsInOrder()
    {
        // 부모와 네 이웃을 만든다.
        var storage = Storage();
        int parent = AssetWithVtable(storage, 0x00509470u);
        int[] others = new int[4];
        // 네 방향의 이웃 슬롯을 미리 만든다.
        for (int index = 0; index < 4; index++)
        {
            others[index] = AssetWithVtable(storage, 0x00509470u);
            WriteSingle(storage.Pool.RawSlot(others[index]), 14, 30.0f + index);
            WriteSingle(storage.Pool.RawSlot(others[index]), 18, 40.0f + index);
            storage.Pool.RawSlot(others[index])[34] = (byte)(2 + index);
        }
        // 호출 순서를 기록한다.
        var seen = new System.Collections.Generic.List<uint>();
        var marks = new System.Collections.Generic.List<(int Parent, int Other)>();
        var lines = new System.Collections.Generic.List<(uint First, uint Second)>();
        var body = new Netstorm.Core.Original.RawFenceLifetime(storage.Pool, new Netstorm.Core.Original.FenceLifetimeHooks
        {
            Neighbor = (_, direction, allied) =>
            {
                Assert.True(allied);
                seen.Add(direction);
                return others[direction / 2];
            },
            ClearMarkers = (sid, other) => marks.Add((sid, other)),
            ClearLine = (points, first, second, sid, other) =>
            {
                Assert.Equal(4, points.Length);
                lines.Add((first, second));
            },
            Unpop = (_, _) => { },
            Repop = (_, _) => { },
            Rebuild = _ => { },
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => { },
            Damageable = (_, _) => { },
        });
        body.Clear(parent);
        // 0/2/4/6 순서로 찾는다.
        Assert.Equal(new uint[] { 0, 2, 4, 6 }, seen);
        Assert.Equal(4, marks.Count);
        Assert.Equal(4, lines.Count);
        // 소유자는 호출 시점의 현재 슬롯에서 읽는다.
        Assert.Equal((1u, 2u), (lines[0].First, lines[0].Second));
    }

    /// <summary>죽은 상태는 Unpop/Repop 없이 이웃만 스냅샷한다.</summary>
    [Fact]
    public void RefreshSkipsUnpopRepopWhenDead()
    {
        // 죽은 부모를 만든다.
        var storage = Storage();
        int parent = AssetWithVtable(storage, 0x00509470u, state: 2);
        int unpop = 0;
        int repop = 0;
        int rebuild = 0;
        // 이웃 조회 횟수를 센다.
        int neighborCalls = 0;
        var body = new Netstorm.Core.Original.RawFenceLifetime(storage.Pool, new Netstorm.Core.Original.FenceLifetimeHooks
        {
            Neighbor = (_, _, _) => { neighborCalls++; return 0; },
            ClearMarkers = (_, _) => Assert.Fail("이웃이 없으면 표식을 지우지 않는다."),
            ClearLine = (_, _, _, _, _) => Assert.Fail("이웃이 없으면 연결선을 지우지 않는다."),
            Unpop = (_, _) => unpop++,
            Repop = (_, _) => repop++,
            Rebuild = _ => rebuild++,
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => Assert.Fail("이웃이 없으면 Regular를 붙이지 않는다."),
            Damageable = (_, _) => { },
        });
        body.Refresh(parent);
        // 자기 정리 전 네 방향을 먼저 저장하고 정리에서 네 방향을 다시 찾는다.
        Assert.Equal(8, neighborCalls);
        Assert.Equal(0, unpop);
        Assert.Equal(0, repop);
        Assert.Equal(0, rebuild);
    }

    /// <summary>삭제 전처리는 앞 두 이웃에만 Regular를 붙인다.</summary>
    [Fact]
    public void PreDestroyAttachesRegularOnlyToFirstTwo()
    {
        // 차단·죽음으로 정리·Unpop/Repop을 건너뛴다.
        var storage = Storage();
        int parent = AssetWithVtable(storage, 0x00509470u, extra: 9, state: 2);
        int n1 = AssetWithVtable(storage, 0x00509470u);
        int n2 = AssetWithVtable(storage, 0x00509470u);
        int n3 = AssetWithVtable(storage, 0x00509470u);
        int n4 = AssetWithVtable(storage, 0x00509470u);
        int[] general = [n1, n2, n3, n4];
        // Regular와 Damageable 호출을 기록한다.
        var added = new System.Collections.Generic.List<int>();
        int damageable = 0;
        uint damageFlags = 0;
        var body = new Netstorm.Core.Original.RawFenceLifetime(storage.Pool, new Netstorm.Core.Original.FenceLifetimeHooks
        {
            Neighbor = (_, direction, allied) => allied ? 0 : general[direction / 2],
            ClearMarkers = (_, _) => { },
            ClearLine = (_, _, _, _, _) => { },
            Unpop = (_, _) => Assert.Fail("죽은 상태는 Unpop하지 않는다."),
            Repop = (_, _) => Assert.Fail("죽은 상태는 Repop하지 않는다."),
            Rebuild = _ => { },
            ReserveRegular = () => true,
            AddRegular = (sid, @event, payload) =>
            {
                Assert.Equal(2u, @event);
                Assert.Equal(0.10000000149011612f, payload);
                added.Add(sid);
            },
            Damageable = (_, flags) => { damageable++; damageFlags = flags; },
        });
        body.PreDestroy(parent, 7);
        // 원문처럼 첫 두 칸에만 붙인다.
        Assert.Equal(new int[] { n1, n2 }, added);
        Assert.Equal(1, damageable);
        Assert.Equal(7u, damageFlags);
    }

    /// <summary>장기 저장소는 펜스 삭제를 같은 본체로 분배한다.</summary>
    [Fact]
    public void StorageFenceLifetimeEmitRoutes()
    {
        // 전체 결선 뒤 펜스 장치를 연결한다.
        var storage = Storage();
        BindAll(storage);
        var neighbors = BindFence(storage);
        // 같은 본체를 재사용하고 같은 풀을 공유한다.
        Assert.Same(storage.FenceLifetime(), storage.FenceLifetime());
        Assert.Same(storage.Pool, storage.FenceLifetime().Pool);
        // 중복 연결은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindFenceLifetimeDevices(new Netstorm.Core.Original.RawSurfaceFenceLifetimeDevices
        {
            Neighbor = (_, _, _) => 0,
            ClearMarkers = (_, _) => { },
            ClearLine = (_, _, _, _, _) => { },
            Rebuild = _ => { },
        }));
        // 차단·죽음으로 Unpop/Repop 없이 Damageable까지 잇는다.
        int asset = AssetWithVtable(storage, 0x00509470u, extra: 9, state: 2);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        // 자기 정리 전후로 동맹·일반 이웃을 각각 네 번씩 찾는다.
        Assert.Equal(8, neighbors.Count);
    }
}
