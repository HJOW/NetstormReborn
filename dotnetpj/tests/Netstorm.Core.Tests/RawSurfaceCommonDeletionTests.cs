// Native 장기 공통 삭제의 장치·공유 주소·본체 결선과 분배 선택을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>공통 삭제 장치 계약·공유 별칭·본체 재사용·효과 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceCommonDeletionTests
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
        // PATCH 진단 단언은 장기 상세 owner에 기록하고 통과시킨다.
        storage.BindDisplayDiagnostics(new Netstorm.Core.Original.StringBox(), (_, _, _) => { });
        return storage;
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

    /// <summary>실제 장치 묶음을 갖춘 전체 결선이다.</summary>
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
    }

    /// <summary>지정한 VT를 슬롯에 기록한 독립 자산 번호다.</summary>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table)
    {
        int sid = storage.Pool.Allocate(SidAllocation.Client);
        System.Span<byte> raw = storage.Pool.RawSlot(sid);
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        return sid;
    }

    /// <summary>첫 공통 본체 전에 실제 UI/소리 장치만 연결한다.</summary>
    [Fact]
    public void BindDeletionDevicesRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        Assert.Throws<ArgumentException>(() => storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        }));
        BindTransmission(storage);
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
        Assert.Throws<ArgumentException>(() => storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        }));
        Assert.Throws<ArgumentNullException>(() => storage.BindDeletionDevices(null!));
        var borrowed = Storage();
        BindTransmission(borrowed);
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = _ => { },
        }));
    }

    /// <summary>공통 후처리 기본 장치는 완전한 세 콜백 뒤에만 설치한다.</summary>
    [Fact]
    public void BindPostPopDevicesRequiresPatchAndCallbacks()
    {
        var storage = Storage();
        BindTransmission(storage);
        Assert.Throws<ArgumentException>(() => storage.BindPostPopDevices(new Netstorm.Core.Original.SquidPostPopDevices()));
        storage.BindPostPopDevices(new Netstorm.Core.Original.SquidPostPopDevices
        {
            AiNotice = (_, _, _) => { },
            LocalOwner = () => 1u,
            RecheckSelection = () => { },
        });
        Assert.True(storage.PostPopState.GraphsEnabled);
        // 두 번째 설치는 같은 장기 수명을 교체한다(원문 parity).
        storage.BindPostPopDevices(new Netstorm.Core.Original.SquidPostPopDevices
        {
            AiNotice = (_, _, _) => { },
            LocalOwner = () => 7u,
            RecheckSelection = () => { },
        });
        Assert.True(storage.PostPopState.GraphsEnabled);
    }

    /// <summary>복원 뒤 삭제 상태가 같은 장기 원문 주소를 별칭한다.</summary>
    [Fact]
    public void RestoreGlobalsAliasesLongTermBoxes()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindSerializationGlobals(new Netstorm.Core.Original.FortSerializationGlobals());
        storage.RestoreDeletionGlobals();
        Assert.Same(storage.GameAux().LostXBox, storage.DeletionState.SharedLostX);
        Assert.Same(storage.GameAux().RainBlockerTypeBox, storage.DeletionState.SharedSilentType);
        Assert.Same(storage.Links.NoIslandTypeBox, storage.DeletionState.SharedSpecialSurfaceType);
        Assert.Same(storage.GameAux().FakeSurfaceTypeBox, storage.DeletionState.SharedReplacementSurfaceType);
        storage.OwnerMode.Fort = true;
        Assert.True(storage.DeletionState.FortSuppressed);
        storage.Links.NoIslandType = 93;
        Assert.Equal(93u, storage.DeletionState.CurrentSpecialSurfaceType);
    }

    /// <summary>같은 장기 본체를 World 반납 뒤에도 재사용한다.</summary>
    [Fact]
    public void CommonDeletionReuses()
    {
        var storage = Storage(withSource: true);
        Assert.Throws<InvalidOperationException>(() => storage.CommonDeletion());
        BindAll(storage);
        storage.EnablePatchDestroyDiagnostics();
        Netstorm.Core.Original.RawSquidDestroyLifecycle first = storage.CommonDeletion();
        Assert.Same(first, storage.CommonDeletion());
        Assert.Same(storage.Pool, first.Pool);
    }

    /// <summary>공통 전처리는 분배 본체를 거쳐 실제 장부와 통지를 갱신한다.</summary>
    [Fact]
    public void CommonPreDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
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
        var aiCalls = new System.Collections.Generic.List<(uint Owner, int Sid, bool Added)>();
        var sounds = new System.Collections.Generic.List<int>();
        storage.BindPostPopDevices(new Netstorm.Core.Original.SquidPostPopDevices
        {
            AiNotice = (owner, sid, added) => aiCalls.Add((owner, sid, added)),
            LocalOwner = () => 1u,
            RecheckSelection = () => { },
        });
        storage.BindDeletionDevices(new Netstorm.Core.Original.RawSurfaceDeletionDevices
        {
            RememberPlacement = (_, _, _) => { },
            UnitLost = sid => sounds.Add(sid),
        });
        storage.BindSerializationGlobals(new Netstorm.Core.Original.FortSerializationGlobals());
        storage.EnablePatchDestroyDiagnostics();
        // 일반 자산의 전처리는 공통 몸체로 분배된다.
        int asset = AssetWithVtable(storage, 0x00501dd0u);
        storage.Pool.RawSlot(asset)[10] = 74;
        storage.Types[74].Group = 0;
        storage.SetAssetOwner(asset, 1);
        storage.LoadingDepth = 0;
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, asset, 0, 0));
        // 같은 호출에서 소리 통지와 AI 통지가 실제 장치를 통과한다.
        Assert.Contains(asset, sounds);
        Assert.Contains((1u, asset, false), aiCalls);
    }

    /// <summary>공통 후처리는 분배 본체를 거쳐 같은 깊이로 닫는다.</summary>
    [Fact]
    public void CommonPostDestroyRoutesViaEmit()
    {
        var storage = Storage(withSource: true);
        BindAll(storage);
        storage.EnablePatchDestroyDiagnostics();
        int asset = AssetWithVtable(storage, 0x00501dd0u);
        storage.Pool.RawSlot(asset)[10] = 74;
        storage.Types[74].Group = 0;
        storage.Types[74].Cost = 100f;
        storage.LoadingDepth = 0;
        int before = storage.GameAux().TotalCost;
        var hooks = storage.DefaultProcessAssetHooks();
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, asset, 0, 0));
        // 사후 비용 차감(원문 x87 뺄셈·0 방향 절삭)이 같은 장부에 남는다.
        Assert.NotEqual(before, storage.GameAux().TotalCost);
    }
}
