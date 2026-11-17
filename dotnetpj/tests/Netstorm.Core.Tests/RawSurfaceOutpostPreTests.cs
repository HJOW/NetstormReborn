// Native 장기 Outpost의 두 목록 제거·영역 갱신·Damageable 부모 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>Outpost 목록 압축·영역 통지·공통 부모 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceOutpostPreTests
{
    /// <summary>실제 최초 자산 표를 빌리는 독립 원천이다.</summary>
    private sealed class StubSource(System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> types) : Netstorm.Core.Original.IRawSurfaceAssetSource
    {
        /// <summary>실제 최초 로딩 표다.</summary>
        public System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> LoadedTypes { get; } = types;
    }

    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage(bool withSource = false)
    {
        var types = Enumerable.Range(0, 188).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = Netstorm.Core.Original.OriginalConstructors.Address(index),
            Flags1 = Netstorm.Core.Original.OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
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

    /// <summary>Outpost까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>? aiCalls = null,
        System.Collections.Generic.List<(float X, float Y)>? regions = null)
    {
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
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
            AiNotice = (owner, sid, added) => aiCalls?.Add((owner, sid, added)),
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
        storage.BindRegionLifecycle(
            (_, _) => { },
            (_, _) => { },
            (x, y) => regions?.Add((x, y)));
    }

    /// <summary>지정한 VT·타입을 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table, byte type = 74, uint extra = 0)
    {
        int sid = storage.Pool.Allocate(SidAllocation.Client);
        System.Span<byte> raw = storage.Pool.RawSlot(sid);
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        raw[10] = type;
        raw[40] = (byte)extra;
        return sid;
    }

    /// <summary>두 목록을 압축하고 영역을 통지한 뒤 공통 부모로 분배된다.</summary>
    [Fact]
    public void OutpostRemovesBothListsAndNotifiesRegion()
    {
        var storage = Storage(withSource: true);
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        var regions = new System.Collections.Generic.List<(float X, float Y)>();
        BindAll(storage, aiCalls: aiCalls, regions: regions);
        int asset = AssetWithVtable(storage, 0x00509210u);
        storage.SidLists.Outposts.Append(asset);
        storage.SidLists.Residences.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Equal(0u, storage.SidLists.Outposts.Count);
        Assert.Equal(0u, storage.SidLists.Residences.Count);
        Assert.Contains((0f, 0f), regions);
        Assert.Contains((0u, asset, false), aiCalls);
    }

    /// <summary>관문 비트가 있으면 목록·영역을 건드리지 않는다.</summary>
    [Fact]
    public void OutpostKeepsListsWithGateBits()
    {
        var storage = Storage(withSource: true);
        var regions = new System.Collections.Generic.List<(float X, float Y)>();
        BindAll(storage, regions: regions);
        int asset = AssetWithVtable(storage, 0x00509210u, extra: 9);
        storage.SidLists.Outposts.Append(asset);
        storage.SidLists.Residences.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Equal(1u, storage.SidLists.Outposts.Count);
        Assert.Equal(1u, storage.SidLists.Residences.Count);
        Assert.Empty(regions);
    }

    /// <summary>없는 쪽 목록은 건드리지 않고 있는 쪽만 압축한다.</summary>
    [Fact]
    public void OutpostRemovesOnlyListedSide()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x00509210u);
        storage.SidLists.Outposts.Append(asset);
        storage.OutpostPreDestroy(asset, 0);
        Assert.Equal(0u, storage.SidLists.Outposts.Count);
        Assert.Equal(0u, storage.SidLists.Residences.Count);
    }

    /// <summary>영역 장치 없이는 목록 압축 뒤에 실패한다.</summary>
    [Fact]
    public void OutpostWithoutRegionFailsAfterLists()
    {
        var storage = Storage(withSource: true);
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        storage.BindRewardDevices(new Netstorm.Core.Original.RawSurfaceRewardDevices
        {
            Sp = new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u),
            Editor = new Netstorm.Core.Original.UIntBox(),
            RefreshSp = () => { },
            CreditWallet = (_, _) => { },
        });
        storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        });
        storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox(),
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        });
        int asset = AssetWithVtable(storage, 0x00509210u);
        storage.SidLists.Outposts.Append(asset);
        var error = Assert.Throws<InvalidOperationException>(() => storage.OutpostPreDestroy(asset, 0));
        Assert.Contains("영역 갱신 미결합", error.Message);
        // 목록 압축은 영역 확인보다 먼저 실행된다.
        Assert.Equal(0u, storage.SidLists.Outposts.Count);
    }

    /// <summary>분배 라우팅은 Outpost 고유 전처리를 선택한다.</summary>
    [Fact]
    public void OutpostRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x00509210u);
        storage.SidLists.Outposts.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        try
        {
            hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        }
        catch (System.Exception ex)
        {
            Assert.DoesNotContain("미연결", ex.Message);
            Assert.DoesNotContain("미지 VT", ex.Message);
        }

        Assert.Equal(0u, storage.SidLists.Outposts.Count);
    }

    /// <summary>두 번째 호출도 같은 순서로 실행된다.</summary>
    [Fact]
    public void OutpostSecondCallRepeats()
    {
        var storage = Storage(withSource: true);
        var regions = new System.Collections.Generic.List<(float X, float Y)>();
        BindAll(storage, regions: regions);
        int asset = AssetWithVtable(storage, 0x00509210u);
        storage.OutpostPreDestroy(asset, 0);
        storage.OutpostPreDestroy(asset, 0);
        Assert.Equal(2, regions.Count);
    }
}
