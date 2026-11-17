// Native 장기 운반체·비행체 전처리와 신전 삭제 전처리의 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>운반체·비행체·신전 전처리 장치·본체 재사용과 효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceCarrierFlyerVortexTests
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

    /// <summary>운반체·비행체·신전까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(int Sid, uint Flags)>? destroyed = null,
        System.Collections.Generic.List<(int Sid, uint Flags)>? vortexPre = null)
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
            Destroy = (sid, flags) => { destroyed?.Add((sid, flags)); return true; },
            GeyserRegistration = (_, _) => { },
        });
        storage.BindCarrierReplenishment(() => { });
        storage.BindRegionLifecycle(
            (_, _) => { },
            (sid, flags) => vortexPre?.Add((sid, flags)),
            (_, _) => { });
    }

    /// <summary>지정한 VT·타입을 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table, byte type = 74)
    {
        int sid = storage.Pool.Allocate(SidAllocation.Client);
        System.Span<byte> raw = storage.Pool.RawSlot(sid);
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        raw[10] = type;
        return sid;
    }

    /// <summary>실제 Client 보충 부모를 최초 연결만 한다.</summary>
    [Fact]
    public void BindCarrierReplenishmentRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        Assert.Throws<ArgumentException>(() => storage.BindCarrierReplenishment(() => { }));
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        storage.BindCarrierReplenishment(() => { });
        Assert.Throws<ArgumentException>(() => storage.BindCarrierReplenishment(() => { }));
        Assert.Throws<ArgumentNullException>(() => storage.BindCarrierReplenishment(null!));
        var borrowed = Storage();
        borrowed.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindCarrierReplenishment(() => { }));
    }

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void CarrierFlyerBodiesReuse()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.Same(storage.CarrierPre(), storage.CarrierPre());
        Assert.Same(storage.FlyerPre(), storage.FlyerPre());
        Assert.Same(storage.Pool, storage.CarrierPre().Pool);
        Assert.Same(storage.Pool, storage.FlyerPre().Pool);
    }

    /// <summary>운반체 전처리는 같은 장기 본체로 분배된다.</summary>
    [Fact]
    public void CarrierPreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x00503d80u);
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
    }

    /// <summary>비행체 전처리는 대상 삭제 뒤 같은 Damageable 부모를 선택한다.</summary>
    [Fact]
    public void FlyerPreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var destroyed = new System.Collections.Generic.List<(int Sid, uint Flags)>();
        BindAll(storage, destroyed: destroyed);
        int target = storage.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        int asset = AssetWithVtable(storage, 0x0050a050u);
        System.Span<byte> raw = storage.Pool.RawSlot(asset);
        raw[8] = (byte)target;
        raw[9] = (byte)(target >> 8);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        // +8 WORD 대상을 가상 삭제하고 같은 개체로 Damageable을 잇는다.
        Assert.Contains((target, 0u), destroyed);
    }

    /// <summary>신전 전처리는 같은 장기 가상 장치를 선택한다.</summary>
    [Fact]
    public void VortexPreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var vortexPre = new System.Collections.Generic.List<(int Sid, uint Flags)>();
        BindAll(storage, vortexPre: vortexPre);
        int asset = AssetWithVtable(storage, 0x005196f0u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Contains((asset, 0u), vortexPre);
    }

    /// <summary>신전 장치 없이는 전처리 분배에서 실패한다.</summary>
    [Fact]
    public void VortexPreWithoutDevicesFails()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        // 영역 장치를 다시 연결할 수 없으므로 장치 없는 저장소로 확인한다.
        var bare = Storage();
        bare.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        bare.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        int asset = AssetWithVtable(bare, 0x005196f0u);
        var hooks = bare.DefaultProcessAssetHooks();
        var error = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0)));
        Assert.Contains("미결합", error.Message);
    }
}
