// Native 장기 자산 삭제 VT 라우팅과 영역 장치 결선의 선택 계약을 검사한다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>83개 VT 표 조회·영역 장치 설치·통지 분배·효과 분기의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceAssetDeletionTests
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

    /// <summary>최초 메뉴에서 같은 실제 장기 가상 장치만 연결한다.</summary>
    [Fact]
    public void BindRegionLifecycleRequiresNativeAndFirstOwners()
    {
        var storage = Storage();
        // 전송 장치 없이 영역 장치를 설치하면 거부한다.
        Assert.Throws<ArgumentException>(() => storage.BindRegionLifecycle((_, _) => { }, (_, _) => { }, (_, _) => { }));
        BindTransmission(storage);
        storage.BindRegionLifecycle((_, _) => { }, (_, _) => { }, (_, _) => { });
        // 두 번째 설치는 같은 장기 수명을 바꾸지 않는다.
        Assert.Throws<ArgumentException>(() => storage.BindRegionLifecycle((_, _) => { }, (_, _) => { }, (_, _) => { }));
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        var missing = Storage();
        BindTransmission(missing);
        Assert.Throws<ArgumentNullException>(() => missing.BindRegionLifecycle(null!, (_, _) => { }, (_, _) => { }));
        // 이미 빌린 물리에서는 설치하지 않는다.
        var borrowed = Storage();
        BindTransmission(borrowed);
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindRegionLifecycle((_, _) => { }, (_, _) => { }, (_, _) => { }));
    }

    /// <summary>표에 없는 Form VT 통지는 소비하지 않고 거짓으로 돌린다.</summary>
    [Fact]
    public void NotifyAssetDependencyReturnsFalseForForm()
    {
        var storage = Storage();
        BindTransmission(storage);
        // 네 내장 Form VT는 자산 83표에 없으므로 기존 Host 분배로 남긴다.
        int form = AssetWithVtable(storage, Netstorm.Core.Original.RawSquidForms.VtableFor(1));
        Assert.False(storage.NotifyAssetDependency(form, 0, 0));
    }

    /// <summary>빈 통지 77VT는 인자를 읽지 않고 조용히 넘긴다.</summary>
    [Fact]
    public void NotifyAssetDependencyEmptyNoticePassesThrough()
    {
        var storage = Storage();
        BindTransmission(storage);
        // 공통 VT의 통지는 빈 몸체이므로 전송·삭제 장치가 없어도 통과한다.
        int asset = AssetWithVtable(storage, 0x00501dd0u);
        Assert.True(storage.NotifyAssetDependency(asset, 0, 0));
    }

    /// <summary>표에 없는 VT는 효과를 실행하지 않고 미지로 실패한다.</summary>
    [Fact]
    public void UnknownVtableFailsWithoutDispatch()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        int unknown = AssetWithVtable(storage, 0xDEADBEEFu);
        // 통지 분배는 거짓으로 돌려 기존 Host가 처리하게 한다.
        Assert.False(storage.NotifyAssetDependency(unknown, 0, 0));
        // 직접 효과는 표 조회에서 미지로 실패한다.
        var hooks = storage.DefaultProcessAssetHooks();
        var error = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, unknown, 0, 0)));
        Assert.Contains("미지 VT", error.Message);
    }

    /// <summary>Pre/Post가 아닌 종속 파괴 효과는 종류 오류로 실패한다.</summary>
    [Fact]
    public void WrongEffectKindFailsBeforeBodyLookup()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        int asset = AssetWithVtable(storage, 0x00501dd0u);
        var hooks = storage.DefaultProcessAssetHooks();
        var error = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.DestroyDependent, asset, 0, 0)));
        Assert.Contains("효과 종류 오류", error.Message);
    }

    /// <summary>미연결 몸체는 원문 진입점 주소를 함께 실패한다.</summary>
    [Fact]
    public void UnconnectedBodiesCarryNativeAddresses()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
        var hooks = storage.DefaultProcessAssetHooks();
        // 공통 전처리는 다음 단위의 몸체 연결까지 주소로 실패한다.
        int common = AssetWithVtable(storage, 0x00501dd0u);
        var pre = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, common, 0, 0)));
        Assert.Contains("004B0950", pre.Message);
        // 공통 후처리도 같은 표의 다른 주소를 읽는다.
        var post = Assert.Throws<InvalidOperationException>(() => hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, common, 0, 0)));
        Assert.Contains("004B0840", post.Message);
        // 새장 통지는 부모 삭제 분기에서 주소로 실패한다.
        int aviary = AssetWithVtable(storage, 0x00509930u);
        var notice = Assert.Throws<InvalidOperationException>(() => storage.NotifyAssetDependency(aviary, common, 0));
        Assert.Contains("0045B4A0", notice.Message);
    }
}
