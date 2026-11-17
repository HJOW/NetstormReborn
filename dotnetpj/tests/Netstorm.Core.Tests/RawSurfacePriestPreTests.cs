// Native 장기 사제 보호막 조회·전처리 본체의 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>사제 보호막·전처리 본체 재사용·공유 별칭·효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfacePriestPreTests
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

    /// <summary>사제 전처리까지의 전체 결선이다.</summary>
    private static void BindAll(
        Netstorm.Core.Original.RawSurfaceStorage storage,
        System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>? aiCalls = null)
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

    /// <summary>조회기는 Native 결합 뒤에만 만든다.</summary>
    [Fact]
    public void PriestForcefieldRequiresNativeAndReuses()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.PriestForcefield());
        BindAll(storage);
        Assert.Same(storage.PriestForcefield(), storage.PriestForcefield());
        Assert.Same(storage.Pool, storage.PriestForcefield().Pool());
        Assert.Equal(167u, storage.PriestForcefield().Type());
    }

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void PriestPreReuses()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.PriestPre());
        BindAll(storage);
        Assert.Same(storage.PriestPre(), storage.PriestPre());
        Assert.Same(storage.Pool, storage.PriestPre().Pool);
    }

    /// <summary>공유 주소는 같은 GameAux 보호막 TYPE 주소를 별칭한다.</summary>
    [Fact]
    public void PriestForcefieldSharesLongTermType()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.Equal(167u, storage.PriestForcefield().Type());
        storage.GameAux().PriestForcefieldType = 168;
        Assert.Equal(168u, storage.PriestForcefield().Type());
        storage.GameAux().PriestForcefieldType = 167;
        Assert.Equal(167u, storage.PriestForcefield().Type());
    }

    /// <summary>사제 전처리는 같은 장기 본체로 분배된다.</summary>
    [Fact]
    public void PriestPreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        BindAll(storage, aiCalls: aiCalls);
        int asset = AssetWithVtable(storage, 0x0050f210u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Contains((0u, asset, false), aiCalls);
    }

    /// <summary>전처리 본체는 finder 결합 없이 만들지 않는다.</summary>
    [Fact]
    public void PriestPreRequiresDamageableDevices()
    {
        var storage = Storage(withSource: true);
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        Assert.Throws<InvalidOperationException>(() => storage.PriestPre());
    }

    /// <summary>신전 장치가 없어도 사제 분배는 Native 계약까지만 검사한다.</summary>
    [Fact]
    public void PriestForcefieldWithoutDevicesFailsNativeFirst()
    {
        var storage = Storage();
        var error = Assert.Throws<InvalidOperationException>(() => storage.PriestPre());
        Assert.Contains("Native 판본 계약", error.Message);
    }
}
