// Native 장기 벌룬 본체의 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>벌룬 그림자 본체 재사용·표 판정·삭제 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceBalloonBodyTests
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

    /// <summary>벌룬까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>? aiCalls = null,
        System.Collections.Generic.List<(int Sid, uint Flags)>? destroyed = null)
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

    /// <summary>장치 결합 뒤에만 벌룬 본체를 만든다.</summary>
    [Fact]
    public void BalloonBodyRequiresDamageableDevices()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.BalloonBody());
        BindAll(storage);
        Assert.Same(storage.BalloonBody(), storage.BalloonBody());
        Assert.Same(storage.Pool, storage.BalloonBody().Pool);
    }

    /// <summary>가상 표만 확인하고 다른 파생으로 확장하지 않는다.</summary>
    [Fact]
    public void BalloonBodyHandlesOnlyBalloonVtables()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.True(storage.BalloonBody().Handles(AssetWithVtable(storage, 0x005040c0u)));
        Assert.True(storage.BalloonBody().Handles(AssetWithVtable(storage, 0x00504198u)));
        Assert.False(storage.BalloonBody().Handles(AssetWithVtable(storage, 0x00501dd0u)));
    }

    /// <summary>그림자 삭제를 먼저 하고 같은 Carrier 기본 전처리를 실행한다.</summary>
    [Fact]
    public void BalloonPreDestroysShadowFirst()
    {
        var storage = Storage(withSource: true);
        var destroyed = new System.Collections.Generic.List<(int Sid, uint Flags)>();
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        BindAll(storage, aiCalls: aiCalls, destroyed: destroyed);
        int shadow = AssetWithVtable(storage, 0x00501dd0u);
        int asset = AssetWithVtable(storage, 0x005040c0u);
        System.Span<byte> raw = storage.Pool.RawSlot(asset);
        raw[8] = (byte)shadow;
        raw[9] = (byte)(shadow >> 8);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Contains((shadow, 0u), destroyed);
        Assert.Contains((0u, asset, false), aiCalls);
    }

    /// <summary>그림자가 없으면 삭제 없이 같은 개체로 잇는다.</summary>
    [Fact]
    public void BalloonPreWithoutShadowSkipsDestroy()
    {
        var storage = Storage(withSource: true);
        var destroyed = new System.Collections.Generic.List<(int Sid, uint Flags)>();
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        BindAll(storage, aiCalls: aiCalls, destroyed: destroyed);
        int asset = AssetWithVtable(storage, 0x005040c0u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Empty(destroyed);
        Assert.Contains((0u, asset, false), aiCalls);
    }

    /// <summary>다른 가상 표의 Validate는 거부한다.</summary>
    [Fact]
    public void BalloonValidateRejectsOtherVtable()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int other = AssetWithVtable(storage, 0x00501dd0u);
        Assert.Throws<InvalidOperationException>(() => storage.BalloonBody().Validate(other, 0));
    }

    /// <summary>Robust는 보관 프레임과 현재 TYPE 프레임 수를 비교한다.</summary>
    [Fact]
    public void BalloonRobustComparesStoredFrame()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int asset = AssetWithVtable(storage, 0x005040c0u);
        System.Span<byte> raw = storage.Pool.RawSlot(asset);
        raw[36] = 0xFF;
        raw[37] = 0xFF;
        raw[38] = 0xFF;
        raw[39] = 0xFF;
        Assert.True(storage.BalloonBody().Robust(asset));
    }
}
