// Native 장기 아이스 타워의 전역·본체 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>아이스 역할 공유·본체 재사용·효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceIceTowerTests
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

    /// <summary>아이스 타워까지의 전체 결선이다.</summary>
    private static void BindAll(Netstorm.Core.Original.RawSurfaceStorage storage)
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
            Destroy = (_, _) => true,
            GeyserRegistration = (_, _) => { },
        });
        storage.BindCarrierReplenishment(() => { });
    }

    /// <summary>지정한 VT·타입을 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table, byte type)
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

    /// <summary>현재 역할의 주소만 설치한다.</summary>
    [Fact]
    public void IceTowerGlobalsRequireNativeAndShareBoxes()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.IceTowerGlobals());
        BindAll(storage);
        Netstorm.Core.Original.IceTowerTypes globals = storage.IceTowerGlobals();
        Assert.Same(storage.GameAux().RainBlockerTypeBox, globals.SharedIce);
        Assert.Same(storage.GameAux().GrowingRainBlockerTypeBox, globals.SharedGrowing);
        Assert.Same(storage.Links.NoIslandTypeBox, globals.SharedNoIsland);
        Assert.Equal(147u, globals.ReadIce());
        Assert.Equal(150u, globals.ReadGrowing());
    }

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void IceTowerReuses()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.Same(storage.IceTower(), storage.IceTower());
        Assert.Same(storage.Pool, storage.IceTower().Pool);
    }

    /// <summary>완성형 파괴는 성장형 생성 뒤 공통 후처리로 분배된다.</summary>
    [Fact]
    public void CompletePostDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[147].Cost = 100f;
        storage.LoadingDepth = 5;
        int asset = AssetWithVtable(storage, 0x0050e940u, 147);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>성장형 후처리는 공통 후처리로 분배된다.</summary>
    [Fact]
    public void GrowingPostDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[150].Cost = 50f;
        int asset = AssetWithVtable(storage, 0x0050e888u, 150);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0x8200000u));
        Assert.Equal(-50, storage.GameAux().TotalCost);
    }

    /// <summary>로딩 관문이 닫히면 성장을 건너뛰고 공통으로 간다.</summary>
    [Fact]
    public void PostDestroySkipsGrowthWhenLoading()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[147].Cost = 100f;
        storage.LoadingDepth = 5;
        int asset = AssetWithVtable(storage, 0x0050e940u, 147);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>성장 예약은 같은 Host에 사건을 부착한다.</summary>
    [Fact]
    public void PostPopAttachesGrowthRegular()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Frames[150] = new Netstorm.Core.Original.RiftTypeFrames([new Netstorm.Core.Original.RiftFrameCode(65, 65, 0, 0)]);
        int asset = AssetWithVtable(storage, 0x0050e888u, 150);
        storage.IceTower().PostPop(asset, 1);
        Assert.NotNull(storage.Processes().Host().FindEvent(asset, 0x3a6u));
    }

    /// <summary>서버 관문이 닫히면 성장을 건너뛰고 공통으로 간다.</summary>
    [Fact]
    public void PostDestroySkipsGrowthWithoutServer()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[147].Cost = 100f;
        storage.LoadingDepth = 0;
        storage.Authority().Server = false;
        int asset = AssetWithVtable(storage, 0x0050e940u, 147);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(-100, storage.GameAux().TotalCost);
        storage.Authority().Server = true;
    }
}
