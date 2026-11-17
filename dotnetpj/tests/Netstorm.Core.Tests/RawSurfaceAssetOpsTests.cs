// Native 장기 자산 생성·소유자·삭제 상위 결선의 선택 계약을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>자산 상위 3종(Create·SetOwner·Destroy)의 라우팅과 장치 계약을 확인한다.</summary>
public sealed class RawSurfaceAssetOpsTests
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

    /// <summary>실제 현재 선택/해제 부모만 있는 기본 선택 묶음이다.</summary>
    private static void BindProcess(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
    }

    /// <summary>실제 별칭 주소와 네 효과를 묶은 Content 장치다.</summary>
    private static void BindContent(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
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

    /// <summary>같은 장기 공장으로 실제 생성한다.</summary>
    [Fact]
    public void CreateAssetCreatesTypedSlot()
    {
        var storage = Storage();
        int sid = storage.CreateAsset(74, 0);
        Assert.True(sid >= Netstorm.Core.Original.RawSidPool.FirstOrdinary);
        Assert.Equal(74, storage.Pool.Slot(sid)[Netstorm.Core.Original.RawSidPool.TypeOffset]);
    }

    /// <summary>같은 실제가상 owner 분배로 소유자 바이트를 쓴다.</summary>
    [Fact]
    public void SetAssetOwnerWritesOwnerByte()
    {
        var storage = Storage();
        int sid = storage.CreateAsset(74, 0);
        storage.SetAssetOwner(sid, 3);
        Assert.Equal(3, storage.Pool.Slot(sid)[34]);
    }

    /// <summary>장치 없이 삭제하면 현재 가상 장치를 요구한다.</summary>
    [Fact]
    public void DestroyAssetRequiresDevices()
    {
        var storage = Storage();
        var error = Assert.Throws<InvalidOperationException>(() => storage.DestroyAsset(5, 0));
        Assert.Contains("현재 가상 삭제 장치", error.Message);
    }

    /// <summary>독립 typed 장치는 스텁 반환을 그대로 돌린다.</summary>
    [Fact]
    public void DestroyAssetUsesResourceDevice()
    {
        var storage = Storage();
        BindTransmission(storage);
        bool? seen = null;
        storage.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (sid, flags) => { seen = true; Assert.Equal(5, sid); Assert.Equal(0u, flags); return true; },
            GeyserRegistration = (_, _) => { },
        });
        Assert.True(storage.DestroyAsset(5, 0));
        Assert.True(seen);
        // 거짓 반환도 그대로 돌린다.
        var denied = Storage();
        BindTransmission(denied);
        denied.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (_, _) => false,
            GeyserRegistration = (_, _) => { },
        });
        Assert.False(denied.DestroyAsset(5, 0));
    }

    /// <summary>PATCH 삭제 모드는 분배 본문을 선택한다.</summary>
    [Fact]
    public void DestroyAssetPatchPathUsesDispatch()
    {
        var storage = Storage(withSource: true);
        BindTransmission(storage);
        BindProcess(storage);
        BindContent(storage);
        BindReward(storage);
        storage.EnablePatchDestroyDiagnostics();
        // 분배 본문은 미지 VT에서 호스트 실행을 거부한다. 자원 스텁 경로가 아님을 보장한다.
        int unknown = AssetWithVtable(storage, 0xDEADBEEFu);
        var error = Assert.Throws<InvalidOperationException>(() => storage.DestroyAsset(unknown, 0));
        Assert.Contains("미지 VT", error.Message);
    }

    /// <summary>가상10 통지는 같은 상위 결선으로 flags0 삭제를 보낸다.</summary>
    [Fact]
    public void DestroyNoticeRoutesViaDestroyAsset()
    {
        var storage = Storage();
        BindTransmission(storage);
        int seenSid = -1;
        uint seenFlags = 99;
        storage.BindResourceDevices(new Netstorm.Core.Original.RawSurfaceResourceDevices
        {
            Random = () => new Netstorm.Core.Original.GameRandom(1),
            CarrierCheck = _ => { },
            DamageableRegular = (_, _, _, payload) => payload,
            StartConstruction = _ => { },
            Destroy = (sid, flags) => { seenSid = sid; seenFlags = flags; return true; },
            GeyserRegistration = (_, _) => { },
        });
        // 세 VT 중 하나의 통지로 flags0 삭제를 확인한다.
        int asset = AssetWithVtable(storage, 0x00509148u);
        Assert.True(storage.NotifyAssetDependency(asset, 0, 7));
        Assert.Equal(asset, seenSid);
        Assert.Equal(0u, seenFlags);
    }
}
