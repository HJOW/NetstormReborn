// Native 장기 소용돌이 소유자 분배와 벌룬 본체 고착 동작의 선택 계약을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>소용돌이 소유자 라우팅과 벌룬 정기 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceVortexBalloonTests
{
    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage()
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
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
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

    /// <summary>소용돌이까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(int Sid, uint Owner)>? vortexOwners = null)
    {
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
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
        storage.BindSerializationGlobals(new Netstorm.Core.Original.FortSerializationGlobals());
        storage.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (_, _) => true,
            GeyserRegistration = (_, _) => { },
        });
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
            (sid, owner) => vortexOwners?.Add((sid, owner)),
            (_, _) => { },
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

    /// <summary>신전 가상 표는 같은 장기 전체 부모를 선택한다.</summary>
    [Fact]
    public void VortexOwnerRoutesViaSetAssetOwner()
    {
        var storage = Storage();
        var vortexOwners = new System.Collections.Generic.List<(int Sid, uint Owner)>();
        BindAll(storage, vortexOwners: vortexOwners);
        int asset = AssetWithVtable(storage, 0x005196f0u);
        storage.SetAssetOwner(asset, 3);
        Assert.Contains((asset, 3u), vortexOwners);
        // 공통 Owner 경로는 타지 않으므로 슬롯 소유자는 그대로다.
        Assert.Equal(0, storage.Pool.Slot(asset)[34]);
    }

    /// <summary>장치 없는 신전 분배는 명시 실패한다.</summary>
    [Fact]
    public void VortexOwnerWithoutDevicesFails()
    {
        var storage = Storage();
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        int asset = AssetWithVtable(storage, 0x005197a8u);
        var error = Assert.Throws<InvalidOperationException>(() => storage.SetAssetOwner(asset, 3));
        Assert.Contains("미결합", error.Message);
    }

    /// <summary>일반 자산은 공통 Owner 분배를 그대로 쓴다.</summary>
    [Fact]
    public void CommonOwnerStillUsedForOrdinaryAsset()
    {
        var storage = Storage();
        var vortexOwners = new System.Collections.Generic.List<(int Sid, uint Owner)>();
        BindAll(storage, vortexOwners: vortexOwners);
        int asset = AssetWithVtable(storage, 0x0050da88u);
        storage.SetAssetOwner(asset, 4);
        Assert.Empty(vortexOwners);
        Assert.Equal(4, storage.Pool.Slot(asset)[34]);
    }

    /// <summary>구름 사건3은 프레임만 진행하고 입력 페이로드를 돌린다.</summary>
    [Fact]
    public void CloudRegularEvent3Advances()
    {
        var storage = Storage();
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x00504198u);
        var advanced = new System.Collections.Generic.List<(int Sid, int Delta, uint Flags)>();
        float result = storage.BalloonBody().CloudRegular(asset, 3, 9, 0.5f, (sid, delta, flags) => advanced.Add((sid, delta, flags)), null);
        Assert.Equal(0.5f, result);
        Assert.Contains((asset, 1, 0u), advanced);
    }

    /// <summary>다른 사건은 기본 Carrier 정기 경계로 넘긴다.</summary>
    [Fact]
    public void CloudRegularOthersUseBase()
    {
        var storage = Storage();
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x00504198u);
        bool based = false;
        float result = storage.BalloonBody().CloudRegular(asset, 7, 2, 0.25f, null, (_, _, _, _) => { based = true; return 1.5f; });
        Assert.True(based);
        Assert.Equal(1.5f, result);
    }

    /// <summary>다른 가상 표의 구름 정기는 거부한다.</summary>
    [Fact]
    public void CloudRegularRejectsOtherVtable()
    {
        var storage = Storage();
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x005040c0u);
        Assert.Throws<System.ArgumentException>(() => storage.BalloonBody().CloudRegular(asset, 3, 0, 0f, (_, _, _) => { }, null));
    }
}
