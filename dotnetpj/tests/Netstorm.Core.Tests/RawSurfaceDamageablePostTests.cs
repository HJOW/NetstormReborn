// Native 장기 피해 후처리의 장치·공유 주소·본체 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>피해 후처리 장치 계약·공유 별칭·본체 재사용·효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceDamageablePostTests
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

    /// <summary>실제 SP 저장소와 editor 상자를 묶은 보상 장치다.</summary>
    private static void BindReward(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindRewardDevices(new Netstorm.Core.Original.RawSurfaceRewardDevices
        {
            Sp = new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u),
            Editor = new Netstorm.Core.Original.UIntBox(),
            RefreshSp = () => { },
            CreditWallet = (_, _) => { },
        });
    }

    /// <summary>실제 방송 장치만 있는 Native 전송 묶음이다.</summary>
    private static void BindTransmission(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
    }

    /// <summary>피해 후처리까지의 전체 결선이다.</summary>
    private static void BindAll(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        BindTransmission(storage);
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

    /// <summary>첫 실제 모듈 전 현재 finder/허용 genus owner와 현재 장치를 연결한다.</summary>
    [Fact]
    public void BindDamageableDevicesRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        Assert.Throws<ArgumentException>(() => storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox(),
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        }));
        BindTransmission(storage);
        // 공통 삭제 장치가 먼저 있어야 한다.
        Assert.Throws<ArgumentException>(() => storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox(),
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        }));
        BindReward(storage);
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
        Assert.Throws<ArgumentException>(() => storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox(),
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        }));
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        var missing = Storage();
        BindTransmission(missing);
        Assert.Throws<ArgumentException>(() => missing.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices()));
    }

    /// <summary>전역 조회는 실제 finder 결합 뒤에 허용한다.</summary>
    [Fact]
    public void DamageableGlobalsRequireFinder()
    {
        var storage = Storage();
        BindTransmission(storage);
        Assert.Throws<InvalidOperationException>(() => storage.DamageablePostGlobals());
        Assert.Throws<InvalidOperationException>(() => storage.DamageablePreGlobals());
        Assert.Throws<InvalidOperationException>(() => storage.DamageableReleaseGlobals());
        BindReward(storage);
        storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        });
        storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox() { Value = 7u },
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        });
        Assert.Same(storage.DamageablePostGlobals(), storage.DamageablePostGlobals());
    }

    /// <summary>공유 주소는 같은 GameAux·권한·장부 주소를 별칭한다.</summary>
    [Fact]
    public void DamageablePostSharesLongTermAddresses()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Netstorm.Core.Original.RawDamageablePostDestroy first = storage.DamageablePost();
        Assert.Same(first, storage.DamageablePost());
        Assert.Same(storage.Pool, first.Pool);
        Netstorm.Core.Original.DamageablePostDestroyState state = storage.DamageablePostGlobals();
        Assert.Same(storage.GameAux().TempleProductTypeBox, state.SharedExcludedDropTypes[0]);
        Assert.Same(storage.GameAux().RainFlyerTypeBox, state.SharedExcludedDropTypes[2]);
        Assert.Same(storage.GameAux().WindFlyerTypeBox, state.SharedExcludedDropTypes[3]);
        Assert.Same(storage.GameAux().BulfTypeBox, state.SharedSpecialDropType);
        Assert.Same(storage.SidLists.Dudes, state.SharedDudePopulation);
        Assert.Same(storage.GameAux().DamageRegularEventBox, state.SharedDamageEvent);
        Assert.Equal(6699u, state.ReadDamageEvent());
    }

    /// <summary>피해 후처리 0x0044b9e0은 장기 본체로 분배된다.</summary>
    [Fact]
    public void DamageablePostDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        // 0x0050b6e8의 후처리는 0x0044b9e0이다.
        int asset = AssetWithVtable(storage, 0x0050b6e8u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        // 사후 비용 차감이 같은 장기 장부에 남는다.
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>제단 꼬리 0x00448b60은 같은 장기 본체를 공유한다.</summary>
    [Fact]
    public void AltarTailSharesDamageablePost()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        // 0x005076b8의 후처리는 제단 꼬리 0x00448b60이다.
        int asset = AssetWithVtable(storage, 0x005076b8u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>피해 전처리는 다음 단위의 몸체 연결까지 주소로 실패한다.</summary>
    [Fact]
    public void DamageablePreStillNeedsBody()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x0050b6e8u);
        var hooks = storage.DefaultProcessAssetHooks();
        var error = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0)));
        Assert.Contains("0044B4B0", error.Message);
    }
}
