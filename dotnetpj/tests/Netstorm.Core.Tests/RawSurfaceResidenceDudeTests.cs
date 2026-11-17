// Native 장기 거주 구역·Dude 후처리 본체의 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>거주 구역·Dude 목록 제거와 공통 Post 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceResidenceDudeTests
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

    /// <summary>후처리까지의 전체 결선이다.</summary>
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

    /// <summary>관문 비트가 없으면 50칸 목록에서 압축한다.</summary>
    [Fact]
    public void ResidenceRemovesFromListWithoutGateBits()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        int asset = AssetWithVtable(storage, 0x0050f540u);
        storage.SidLists.Residences.Append(asset);
        Assert.Equal(1u, storage.SidLists.Residences.Count);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(0u, storage.SidLists.Residences.Count);
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>관문 비트가 있으면 목록을 건드리지 않고 공통 Post만 실행한다.</summary>
    [Fact]
    public void ResidenceKeepsListWithGateBits()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        int asset = AssetWithVtable(storage, 0x0050f540u, extra: 9);
        storage.SidLists.Residences.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(1u, storage.SidLists.Residences.Count);
        // 관문 비트가 있으면 비일반이므로 사후 비용도 그대로다.
        Assert.Equal(0, storage.GameAux().TotalCost);
    }

    /// <summary>관문 비트가 없으면 30칸 population에서 압축한다.</summary>
    [Fact]
    public void DudeRemovesFromPopulationWithoutGateBits()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        int asset = AssetWithVtable(storage, 0x00508cc8u);
        storage.SidLists.Dudes.Append(asset);
        Assert.Equal(1u, storage.SidLists.Dudes.Count);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(0u, storage.SidLists.Dudes.Count);
        Assert.Equal(-100, storage.GameAux().TotalCost);
    }

    /// <summary>관문 비트가 있으면 population을 건드리지 않는다.</summary>
    [Fact]
    public void DudeKeepsPopulationWithGateBits()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.Types[74].Cost = 100f;
        int asset = AssetWithVtable(storage, 0x00508cc8u, extra: 8);
        storage.SidLists.Dudes.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Equal(1u, storage.SidLists.Dudes.Count);
    }

    /// <summary>비트3만 있어도 관문으로 인정한다.</summary>
    [Fact]
    public void ResidenceKeepsListWithBuriedBitOnly()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x0050f540u, extra: 8);
        storage.SidLists.Residences.Append(asset);
        storage.ResidencePostDestroy(asset, 0);
        Assert.Equal(1u, storage.SidLists.Residences.Count);
    }

    /// <summary>CD 판본의 Dude 후처리는 판본 오류로 실패한다.</summary>
    [Fact]
    public void DudeRejectsCdEdition()
    {
        var types = Enumerable.Range(0, 171).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = 0,
            Flags1 = 0,
            MaxHitPoints = 1,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Cd1072, types, frames, shapes);
        Assert.Throws<System.ArgumentException>(() => storage.DudePostDestroy(5, 0));
    }
}
