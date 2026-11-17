// Native 장기 간헐천 고유 수명의 전역·본체 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>간헐천 프레임·목록·AI·공통 부모 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceGeyserLifetimeTests
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

    /// <summary>간헐천까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>? aiCalls = null,
        System.Collections.Generic.List<(int Sid, uint Operation)>? aiUnregisters = null,
        System.Collections.Generic.List<(float X, float Y, uint Flags, uint Owner)>? islandDrops = null,
        Netstorm.Core.Original.UIntBox? editor = null)
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
            Editor = editor ?? new Netstorm.Core.Original.UIntBox(),
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
            DropIsland = (x, y, flags, owner) => islandDrops?.Add((x, y, flags, owner)),
        });
        storage.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (_, _) => true,
            GeyserRegistration = (sid, operation) => aiUnregisters?.Add((sid, operation)),
        });
    }

    /// <summary>지정한 VT·타입을 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table, byte type = 122, uint extra = 0)
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

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void GeyserLifetimeReuses()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.GeyserLifetime());
        BindAll(storage);
        Assert.Same(storage.GeyserLifetime(), storage.GeyserLifetime());
        Assert.Same(storage.Pool, storage.GeyserLifetime().Pool);
    }

    /// <summary>가상 표만 허용하고 다른 파생으로 확장하지 않는다.</summary>
    [Fact]
    public void HandlesOnlyGeyserVtable()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.True(storage.GeyserLifetime().Handles(AssetWithVtable(storage, 0x0050da88u)));
        Assert.False(storage.GeyserLifetime().Handles(AssetWithVtable(storage, 0x00501dd0u)));
    }

    /// <summary>전처리는 프레임 쓰기·AI 해제·목록 압축 뒤 공통 전처리로 분배된다.</summary>
    [Fact]
    public void PreDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        var aiUnregisters = new System.Collections.Generic.List<(int Sid, uint Operation)>();
        BindAll(storage, aiCalls: aiCalls, aiUnregisters: aiUnregisters);
        storage.Types[122].DefaultFrame = 0x12345678;
        int asset = AssetWithVtable(storage, 0x0050da88u);
        storage.SidLists.Geysers.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        // 기본 프레임 DWORD가 raw+36에 기록된다.
        System.ReadOnlySpan<byte> raw = storage.Pool.Slot(asset);
        uint frame = (uint)(raw[36] | (raw[37] << 8) | (raw[38] << 16) | (raw[39] << 24));
        Assert.Equal(0x12345678u, frame);
        // AI 해제와 목록 압축과 공통 통지를 통과한다.
        Assert.Contains((asset, 0u), aiUnregisters);
        Assert.Equal(0u, storage.SidLists.Geysers.Count);
        Assert.Contains((0u, asset, false), aiCalls);
    }

    /// <summary>관문 비트가 있으면 AI·목록을 건드리지 않는다.</summary>
    [Fact]
    public void PreDestroySkipsAiAndListWithGateBits()
    {
        var storage = Storage(withSource: true);
        var aiUnregisters = new System.Collections.Generic.List<(int Sid, uint Operation)>();
        BindAll(storage, aiUnregisters: aiUnregisters);
        int asset = AssetWithVtable(storage, 0x0050da88u, extra: 9);
        storage.SidLists.Geysers.Append(asset);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Empty(aiUnregisters);
        Assert.Equal(1u, storage.SidLists.Geysers.Count);
    }

    /// <summary>편집 모드 후처리는 받침 정리를 호출한다.</summary>
    [Fact]
    public void PostDestroyDropsIslandInEditorMode()
    {
        var storage = Storage(withSource: true);
        var islandDrops = new System.Collections.Generic.List<(float X, float Y, uint Flags, uint Owner)>();
        BindAll(storage, islandDrops: islandDrops, editor: new Netstorm.Core.Original.UIntBox() { Value = 1 });
        int asset = AssetWithVtable(storage, 0x0050da88u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 100));
        Assert.Contains((0f, 0f, 100u, 0u), islandDrops);
    }

    /// <summary>비편집 모드 후처리는 받침 정리를 건너뛴다.</summary>
    [Fact]
    public void PostDestroySkipsIslandDropOutsideEditor()
    {
        var storage = Storage(withSource: true);
        var islandDrops = new System.Collections.Generic.List<(float X, float Y, uint Flags, uint Owner)>();
        BindAll(storage, islandDrops: islandDrops);
        int asset = AssetWithVtable(storage, 0x0050da88u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        Assert.Empty(islandDrops);
    }
}
