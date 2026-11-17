// Native 장기 표면 수명의 장치·본체 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>표면 수명 장치 계약·본체 재사용·효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceSurfaceLifetimeTests
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

    /// <summary>표면 수명까지의 전체 결선이다.</summary>
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

    /// <summary>짧은 World의 실제 효과 소유권만 연결한다.</summary>
    [Fact]
    public void BindSurfaceLifetimeRuntimeRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        Assert.Throws<ArgumentException>(() => storage.BindSurfaceLifetimeRuntime(new Netstorm.Core.Original.RawSurfaceLifetimeBindings
        {
            FallWalker = _ => { },
            BridgeEffect = _ => { },
        }));
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        var first = new Netstorm.Core.Original.RawSurfaceLifetimeBindings
        {
            FallWalker = _ => { },
            BridgeEffect = _ => { },
            Pin = () => new object(),
        };
        storage.BindSurfaceLifetimeRuntime(first);
        Assert.Throws<ArgumentException>(() => storage.BindSurfaceLifetimeRuntime(new Netstorm.Core.Original.RawSurfaceLifetimeBindings
        {
            FallWalker = _ => { },
            BridgeEffect = _ => { },
        }));
        System.GC.KeepAlive(first);
        Assert.Throws<ArgumentException>(() => storage.BindSurfaceLifetimeRuntime(new Netstorm.Core.Original.RawSurfaceLifetimeBindings
        {
            BridgeEffect = _ => { },
        }));
        // 준비 대여 중의 바인딩은 World가 실제 효과를 연결하는 정상 경로이므로 거부하지 않는다(원문 계약).
        var borrowed = Storage();
        borrowed.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        using var lease = borrowed.AcquirePrepared();
        borrowed.BindSurfaceLifetimeRuntime(new Netstorm.Core.Original.RawSurfaceLifetimeBindings
        {
            FallWalker = _ => { },
            BridgeEffect = _ => { },
            Pin = () => new object(),
        });
    }

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void SurfaceLifetimeReuses()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.SurfaceLifetime());
        BindAll(storage);
        Netstorm.Core.Original.RawSurfaceLifecycle first = storage.SurfaceLifetime();
        Assert.Same(first, storage.SurfaceLifetime());
    }

    /// <summary>다리 전처리는 표면 수명으로 분배된다.</summary>
    [Fact]
    public void BridgePreRoutesViaSurface()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int bridge = AssetWithVtable(storage, 0x005034c8u, 82);
        var hooks = storage.DefaultProcessAssetHooks();
        try
        {
            hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, bridge, 0, 0));
        }
        catch (InvalidOperationException ex)
        {
            Assert.DoesNotContain("미연결", ex.Message);
            Assert.DoesNotContain("미지 VT", ex.Message);
        }
    }

    /// <summary>섬 전처리는 표면 수명으로 분배된다.</summary>
    [Fact]
    public void IslandPreRoutesViaSurface()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int island = AssetWithVtable(storage, 0x00506d98u, 94);
        var hooks = storage.DefaultProcessAssetHooks();
        try
        {
            hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, island, 0, 0));
        }
        catch (System.Exception ex)
        {
            Assert.DoesNotContain("미연결", ex.Message);
            Assert.DoesNotContain("미지 VT", ex.Message);
        }
    }

    /// <summary>무인도 후처리는 표면 수명으로 분배된다.</summary>
    [Fact]
    public void NoIslandPostRoutesViaSurface()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        int surface = AssetWithVtable(storage, 0x00506f08u, 157);
        var hooks = storage.DefaultProcessAssetHooks();
        try
        {
            hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, surface, 0, 0));
        }
        catch (InvalidOperationException ex)
        {
            Assert.DoesNotContain("미연결", ex.Message);
            Assert.DoesNotContain("미지 VT", ex.Message);
        }
    }

    /// <summary>표면 수명 본체는 Native 결합 뒤에만 만든다.</summary>
    [Fact]
    public void SurfaceLifetimeRequiresNative()
    {
        var storage = Storage();
        Assert.Throws<InvalidOperationException>(() => storage.SurfaceLifetime());
    }
}
