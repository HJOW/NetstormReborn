// Native 장기 피해 전처리·해방·폭탄 본체의 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>피해 전처리·해방·폭탄 본체 재사용과 효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceDamageablePreTests
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

    /// <summary>전처리까지의 전체 결선이다.</summary>
    private static void BindAll(Netstorm.Core.Original.RawSurfaceStorage storage, System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>? aiCalls = null, System.Collections.Generic.List<int>? collapsed = null, System.Collections.Generic.List<int>? exploded = null)
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
        storage.EnablePatchDestroyDiagnostics();
        storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = new Netstorm.Core.Original.UIntBox() { Value = 0x40480002u },
            Collapse = sid => collapsed?.Add(sid),
            Explosion = sid => exploded?.Add(sid),
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

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void DamageBodiesReuse()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        Assert.Same(storage.DamageableRelease(), storage.DamageableRelease());
        Assert.Same(storage.DamageablePre(), storage.DamageablePre());
        Assert.Same(storage.BombPre(), storage.BombPre());
        Assert.Same(storage.Pool, storage.DamageableRelease().Pool);
        Assert.Same(storage.Pool, storage.DamageablePre().Pool);
        Assert.Same(storage.Pool, storage.BombPre().Pool);
    }

    /// <summary>공유 주소는 같은 GameAux·장치 주소를 별칭한다.</summary>
    [Fact]
    public void DamageableGlobalsShareLongTermBoxes()
    {
        var storage = Storage();
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
        var genus = new Netstorm.Core.Original.UIntBox() { Value = 9u };
        storage.BindDamageableDevices(new Netstorm.Core.Original.RawSurfaceDamageableDevices
        {
            Finder = new Netstorm.Core.Original.ContainedFinderState(),
            AllowedGenus = genus,
            Collapse = _ => { },
            Explosion = _ => { },
            Sound = _ => { },
            SpawnPriest = (_, _, _, _) => { },
            DropDude = (_, _, _) => { },
            DropIsland = (_, _, _, _) => { },
        });
        Assert.Same(storage.GameAux().PriestTypeBox, storage.DamageablePreGlobals().SharedPriestType);
        Assert.Same(genus, storage.DamageableReleaseGlobals().SharedAllowedGenus);
    }

    /// <summary>피해 전처리는 장기 본체로 분배된다.</summary>
    [Fact]
    public void DamageablePreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var collapsed = new System.Collections.Generic.List<int>();
        var exploded = new System.Collections.Generic.List<int>();
        BindAll(storage, collapsed: collapsed, exploded: exploded);
        int asset = AssetWithVtable(storage, 0x0050b6e8u);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0x300000u));
        // 붕괴·폭발 효과가 같은 장치 호출을 통과한다.
        Assert.Contains(asset, collapsed);
        Assert.Contains(asset, exploded);
    }

    /// <summary>너겟 단일 점프도 같은 전처리 본체를 공유한다.</summary>
    [Fact]
    public void NuggetThunkSharesDamageablePre()
    {
        var storage = Storage(withSource: true);
        var collapsed = new System.Collections.Generic.List<int>();
        var exploded = new System.Collections.Generic.List<int>();
        BindAll(storage, collapsed: collapsed, exploded: exploded);
        int asset = AssetWithVtable(storage, 0x0050dc30u, 154);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0x300000u));
        Assert.Contains(asset, collapsed);
        Assert.Contains(asset, exploded);
    }

    /// <summary>폭탄 자연 관문은 장기 본체로 분배된다.</summary>
    [Fact]
    public void BombPreRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        var exploded = new System.Collections.Generic.List<int>();
        BindAll(storage, exploded: exploded);
        int asset = AssetWithVtable(storage, 0x00502370u, 25);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0x100000u));
        Assert.Contains(asset, exploded);
    }

    /// <summary>매립 단일 점프는 현재 공통Pre와 동일한 인수로 분배된다.</summary>
    [Fact]
    public void BuriedJumpSharesCommonPre()
    {
        var storage = Storage(withSource: true);
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        BindAll(storage, aiCalls: aiCalls);
        int asset = AssetWithVtable(storage, 0x00502c38u);
        storage.SetAssetOwner(asset, 1);
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        Assert.Contains((1u, asset, false), aiCalls);
    }
}
