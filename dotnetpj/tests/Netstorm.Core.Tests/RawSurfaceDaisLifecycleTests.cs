// 다이스·제단·Player·비행장 홈의 상위 분배와 호스트 장치 결선을 검사한다.
// cpppj/src/client/AssetDeletionVtables.inc·RawSurfaceAssetDeletion.cpp·RawSurfaceAssetNotice.cpp 의 분배를 대조한다.
// 0.593에서 추가됐다. (R4-1 잔여 중 피해/해방 상위 결선 장치 경계)
namespace Netstorm.Core.Tests;

/// <summary>미연결 VT 분배의 장치 경계와 Emit 라우팅을 확인한다.</summary>
public sealed class RawSurfaceDaisLifecycleTests
{
    /// <summary>분배 검사 크기의 저장소를 만든다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage()
    {
        // 188개 타입 번호 각각에 빈 타입 레코드·프레임·셰이프를 채운다.
        var types = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        var frames = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
        return storage;
    }

    /// <summary>가상 표만 기록한 자산을 만든다.</summary>
    /// <param name="storage">자를 풀을 빌리는 저장소.</param>
    /// <param name="table">기록할 가상 표.</param>
    private static int AssetWithVtable(Netstorm.Core.Original.RawSurfaceStorage storage, uint table)
    {
        int sid = storage.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        System.Span<byte> raw = storage.Pool.AllocatedBytes(sid);
        raw.Clear();
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        return sid;
    }

    /// <summary>전송 장치 전과 중복·누락은 결선을 거부한다.</summary>
    [Fact]
    public void BindRejectsMissingOrDuplicateDevices()
    {
        var bareTypes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.OriginalTypeRecord()).ToArray();
        var bareFrames = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var bareShapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var bare = new Netstorm.Core.Original.RawSurfaceStorage(
            Netstorm.Core.Original.OriginalEdition.Patch1078, bareTypes, bareFrames, bareShapes);
        var dais = new Netstorm.Core.Original.RawSurfaceStorage.RawSurfaceDaisLifecycle
        {
            AltarPre = (_, _) => { },
            DaisPre = (_, _) => { },
            DaisPost = (_, _) => { },
        };
        // 전송 장치 전에는 모두 거부한다.
        Assert.Throws<System.ArgumentException>(() => bare.BindDaisLifecycle(dais));
        Assert.Throws<System.ArgumentException>(() => bare.BindPlayerPre((_, _) => { }));
        Assert.Throws<System.ArgumentException>(() => bare.BindAviaryHome((_, _) => { }));
        // 묶음 하나라도 빠지면 거부한다.
        var storage = Storage();
        Assert.Throws<System.ArgumentException>(() => storage.BindDaisLifecycle(new Netstorm.Core.Original.RawSurfaceStorage.RawSurfaceDaisLifecycle()));
        storage.BindDaisLifecycle(dais);
        storage.BindPlayerPre((_, _) => { });
        storage.BindAviaryHome((_, _) => { });
        // 중복 결선은 거부한다.
        Assert.Throws<System.ArgumentException>(() => storage.BindDaisLifecycle(dais));
        Assert.Throws<System.ArgumentException>(() => storage.BindPlayerPre((_, _) => { }));
        Assert.Throws<System.ArgumentException>(() => storage.BindAviaryHome((_, _) => { }));
    }

    /// <summary>다이스·제단 전처리/후처리는 같은 호스트 장치를 선택한다.</summary>
    [Fact]
    public void DaisDevicesForwardSidAndFlags()
    {
        var storage = Storage();
        var calls = new System.Collections.Generic.List<(string Name, int Sid, uint Flags)>();
        storage.BindDaisLifecycle(new Netstorm.Core.Original.RawSurfaceStorage.RawSurfaceDaisLifecycle
        {
            AltarPre = (sid, flags) => calls.Add(("altar", sid, flags)),
            DaisPre = (sid, flags) => calls.Add(("pre", sid, flags)),
            DaisPost = (sid, flags) => calls.Add(("post", sid, flags)),
        });
        storage.DaisAltarPre(50, 1);
        storage.DaisPreDestroy(51, 2);
        storage.DaisPostDestroy(52, 3);
        Assert.Equal([("altar", 50, 1u), ("pre", 51, 2u), ("post", 52, 3u)], calls);
        // 장치 없는 저장소는 명시 실패한다.
        var bare = Storage();
        Assert.Throws<System.InvalidOperationException>(() => bare.DaisAltarPre(50, 1));
        Assert.Throws<System.InvalidOperationException>(() => bare.DaisPreDestroy(50, 1));
        Assert.Throws<System.InvalidOperationException>(() => bare.DaisPostDestroy(50, 1));
    }

    /// <summary>Player 전처리와 비행장 홈 통지는 같은 호스트 장치를 선택한다.</summary>
    [Fact]
    public void PlayerAndAviaryDevicesForwardSidAndFlags()
    {
        var storage = Storage();
        var calls = new System.Collections.Generic.List<(string Name, int Sid, uint Flags)>();
        storage.BindPlayerPre((sid, flags) => calls.Add(("player", sid, flags)));
        storage.BindAviaryHome((sid, method) => calls.Add(("aviary", sid, method)));
        storage.PlayerPreDestroy(50, 1);
        storage.NotifyAviaryHome(51, 0x0045b4a0u);
        storage.NotifyAviaryHome(52, 0x0045b5b0u);
        storage.NotifyAviaryHome(53, 0x0045b680u);
        Assert.Equal([("player", 50, 1u), ("aviary", 51, 0x0045b4a0u), ("aviary", 52, 0x0045b5b0u), ("aviary", 53, 0x0045b680u)], calls);
        // 장치 없는 저장소는 명시 실패한다.
        var bare = Storage();
        Assert.Throws<System.InvalidOperationException>(() => bare.PlayerPreDestroy(50, 1));
        Assert.Throws<System.InvalidOperationException>(() => bare.NotifyAviaryHome(50, 0x0045b4a0u));
    }

    /// <summary>83 VT표의 다이스·Player 항목은 Emit에서 같은 장치에 닿는다.</summary>
    [Fact]
    public void EmitRoutesDaisAndPlayerTables()
    {
        var storage = Storage();
        var calls = new System.Collections.Generic.List<(string Name, int Sid, uint Flags)>();
        storage.BindDaisLifecycle(new Netstorm.Core.Original.RawSurfaceStorage.RawSurfaceDaisLifecycle
        {
            AltarPre = (sid, flags) => calls.Add(("altar", sid, flags)),
            DaisPre = (sid, flags) => calls.Add(("pre", sid, flags)),
            DaisPost = (sid, flags) => calls.Add(("post", sid, flags)),
        });
        storage.BindPlayerPre((sid, flags) => calls.Add(("player", sid, flags)));
        var hooks = storage.DefaultProcessAssetHooks();
        int altar = AssetWithVtable(storage, 0x005076b8u);
        int dais = AssetWithVtable(storage, 0x00507950u);
        int player = AssetWithVtable(storage, 0x0050ee08u);
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, altar, 0, 7));
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, dais, 0, 8));
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PostDestroy, dais, 0, 9));
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.PreDestroy, player, 0, 10));
        Assert.Equal([("altar", altar, 7u), ("pre", dais, 8u), ("post", dais, 9u), ("player", player, 10u)], calls);
    }

    /// <summary>세 비행장 VT의 통지는 같은 장치에 주소를 함께 전달한다.</summary>
    [Fact]
    public void EmitRoutesAviaryNotices()
    {
        var storage = Storage();
        var calls = new System.Collections.Generic.List<(int Sid, uint Method)>();
        storage.BindAviaryHome((sid, method) => calls.Add((sid, method)));
        var hooks = storage.DefaultProcessAssetHooks();
        int sun = AssetWithVtable(storage, 0x00509930u);
        int rain = AssetWithVtable(storage, 0x005099e8u);
        int wind = AssetWithVtable(storage, 0x00509aa0u);
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.ReleaseDependent, sun, 0, 0));
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.ReleaseDependent, rain, 0, 0));
        hooks.Emit!(new Netstorm.Core.Original.SquidDestroyEvent(Netstorm.Core.Original.SquidDestroyEffect.ReleaseDependent, wind, 0, 0));
        Assert.Equal([(sun, 0x0045b4a0u), (rain, 0x0045b5b0u), (wind, 0x0045b680u)], calls);
    }
}
