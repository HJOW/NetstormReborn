using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Client 표·현재 Runtime pin·같은 Pool의 가상 삭제 분배 연결을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>미선택 기록에는 세션을 요구하지 않고 실제 선택에서는 명시 삭제와 부분 효과를 확인한다.</summary>
public sealed class ClientMetaZoneCleanupTests
{
    /// <summary>실제 FormPreDestroy가 선택한 가상 통지만 관찰하는 프로세스다.</summary>
    private sealed class ObservedProcess(Action<uint> destroyed) : SquidProcess
    {
        /// <summary>실제 가상 소멸 통지의 관측 장치이며 삭제 결과를 공급하지 않는다.</summary>
        private readonly Action<uint> _destroyed = destroyed;
        /// <summary>Kernel/풀 삭제는 실제 Host에 맡기고 원문 가상 통지 시점만 기록한다.</summary>
        public override void OnFormDestroy(uint flags) => _destroyed(flags);
    }

    /// <summary>같은 장기 물리/Player/프로세스와 실제 가상 Form 분배기를 보유한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>현재 Client의 실제 장기 물리/게임/시계 소유자다.</summary>
        public ClientSurfaceGlobals Surface { get; }
        /// <summary>같은 실제 Player 고정뷰와 장기 Host 대여를 가진 제품 세션이다.</summary>
        public HostSession Session { get; }
        /// <summary>Gameplay·Surface를 같은 호출 끝까지 보유하는 실제 외곽 수명이다.</summary>
        public HostMissionRuntime Runtime { get; }
        /// <summary>실제 프로세스 Form을 같은 Host로 삭제하는 기존 분배기다.</summary>
        public RawSquidFormDispatch Dispatch { get; }
        /// <summary>같은 실제 장기 기록표/flag만 사용하는 정리 장치다.</summary>
        public ClientStateStorageCleanup Cleanup { get; }
        /// <summary>실자산과 같은 실제 Host를 구성하되 Meta 전용 자산 동작을 대체하지 않는다.</summary>
        public Scene(bool bind = true)
        {
            var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local);
            Surface = new(game, clock, new(), () => 1000);
            Session = new(Surface.Kernel, new(17), new(), pool: Surface.Storage.Pool, sharedGameplay: game, sharedLocal: local,
                sharedPlayerRecords: game.PlayerRecords, sharedPlayerProcesses: Surface.Processes, sharedProcessTime: clock.GameTime);
            _ = HostPostPopRuntimeTests.Build(Session, Surface.Storage); _ = Session.BuildPlayerWorldHooks();
            var host = Session.PlayerProcesses; Dispatch = new(Session.Pool, host.Destroy(), host.Hooks()); Dispatch.Bind(host);
            if (bind) Session.BindStateDeletion(Dispatch);
            Runtime = HostMissionRuntime.Create(Session, Surface.Storage); Cleanup = new(Surface, local, new());
            local.MetaFirstReset.Value = 0;
        }
        /// <summary>선택 marker에 들어갈 실제 Kernel 프로세스 Form을 실제 공장/Host로 만든다.</summary>
        public (ObservedProcess Process, int Sid, uint Pid) Form(Action<uint> destroyed)
        {
            int parent = Session.BuildPlayerWorldHooks().Create!(152, 2);
            var process = new ObservedProcess(destroyed);
            Assert.Same(process, Session.PlayerProcesses.Attach(process, 10, parent, 0x50));
            return (process, process.Form(), Surface.Kernel.Find(process));
        }
        /// <summary>실제 메타 기록의 상태와+4c DWORD만 명시 입력으로 쓴다.</summary>
        public void Marker(uint sid)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(Session.LocalState.MetaZoneRecords.Span, 1);
            BinaryPrimitives.WriteUInt32LittleEndian(Session.LocalState.MetaZoneRecords.Span[76..], sid);
        }
        /// <summary>각 소비자를 먼저 반납하고 실제 외곽 Gameplay/Surface 대여를 끝낸다.</summary>
        public void Dispose() { Session.ReleasePlayerRecordViews(); Runtime.Dispose(); }
    }

    /// <summary>최초 전체 초기화와 비양수/빈 기록은 현재 Runtime 없이 실제 같은 표만 지운다.</summary>
    [Fact]
    public void FirstResetAndUnselectedMarkersNeedNoRuntimeAndKeepOtherLocalState()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 1000);
        var cleanup = new ClientStateStorageCleanup(surface, local, new()); local.MetaZoneRecords.Span.Fill(0xa5); local.GameTicks.Value = 73;
        cleanup.ClearMetaZones(); Assert.Equal(0u, local.MetaFirstReset.Value); Assert.All(local.MetaZoneRecords.ToArray(), value => Assert.Equal(0, value));
        BinaryPrimitives.WriteUInt32LittleEndian(local.MetaZoneRecords.Span, uint.MaxValue);
        BinaryPrimitives.WriteUInt32LittleEndian(local.MetaZoneRecords.Span[76..], uint.MaxValue);
        cleanup.ClearMetaZones(); Assert.Equal(73u, local.GameTicks.Value); Assert.False(game.Borrowed()); Assert.Equal(0, surface.Kernel.Size());
        BinaryPrimitives.WriteUInt32LittleEndian(local.MetaZoneRecords.Span, 1);
        BinaryPrimitives.WriteUInt32LittleEndian(local.MetaZoneRecords.Span[76..], 5);
        byte[] before = local.MetaZoneRecords.ToArray(); Assert.Throws<InvalidOperationException>(cleanup.ClearMetaZones); Assert.Equal(before, local.MetaZoneRecords.ToArray());
    }

    /// <summary>선택된 실제 Form 가상 삭제가 전체 Runtime을 pin하고 Kernel/부모/풀을 정상 반납한 뒤 표를 지운다.</summary>
    [Fact]
    public void ActualProcessFormDeletionPinsRuntimeAndReturnsKernelAndPool()
    {
        using var scene = new Scene(); int calls = 0;
        var form = scene.Form(_ =>
        {
            calls++; Assert.True(scene.Surface.Gameplay.RuntimeCallActive(scene.Session));
            Assert.Throws<InvalidOperationException>(scene.Session.ReleasePlayerRecordViews);
        });
        scene.Marker((uint)form.Sid); scene.Cleanup.ClearMetaZones();
        Assert.Equal(1, calls); Assert.Null(scene.Surface.Kernel.Get(form.Pid)); Assert.NotEqual(0, scene.Session.Pool.Slot(form.Sid)[11] & 1);
        Assert.All(scene.Session.LocalState.MetaZoneRecords.ToArray(), value => Assert.Equal(0, value));
        Assert.False(scene.Surface.Gameplay.RuntimeCallActive(scene.Session)); Assert.True(scene.Surface.Gameplay.Borrowed());
    }

    /// <summary>미설치 실제 분배와 DWORD SID 범위는 같은 필드를 보존하고 Runtime pin을 정상 반납한다.</summary>
    [Fact]
    public void MissingDispatchAndWideSidFailBeforeMarkerZeroStore()
    {
        using var scene = new Scene(false); var form = scene.Form(_ => Assert.Fail("미설치 분배의 가상 통지"));
        scene.Marker((uint)form.Sid); byte[] before = scene.Session.LocalState.MetaZoneRecords.ToArray();
        Assert.Throws<InvalidOperationException>(scene.Cleanup.ClearMetaZones); Assert.Equal(before, scene.Session.LocalState.MetaZoneRecords.ToArray()); Assert.Same(form.Process, scene.Surface.Kernel.Get(form.Pid));
        Assert.False(scene.Surface.Gameplay.RuntimeCallActive(scene.Session));
        scene.Marker(0x10005); Assert.Throws<ArgumentOutOfRangeException>(scene.Cleanup.ClearMetaZones);
        Assert.Equal(0x10005u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Session.LocalState.MetaZoneRecords.Span[76..]));
        Assert.False(scene.Surface.Gameplay.RuntimeCallActive(scene.Session));
    }

    /// <summary>실제 가상 삭제가 실패하면 앞선 dead 쓰기는 남고 메타 SID0 쓰기는 실행하지 않는다.</summary>
    [Fact]
    public void ActualVirtualFailureKeepsPoolPrefixAndMarkerAndReleasesPin()
    {
        using var scene = new Scene(); var failure = new IOException("실제 Form 가상 통지 실패");
        var form = scene.Form(_ => throw failure); scene.Marker((uint)form.Sid);
        Assert.Same(failure, Assert.Throws<IOException>(scene.Cleanup.ClearMetaZones));
        Assert.NotEqual(0, scene.Session.Pool.Slot(form.Sid)[11] & 2); Assert.Same(form.Process, scene.Surface.Kernel.Get(form.Pid));
        Assert.Equal((uint)form.Sid, BinaryPrimitives.ReadUInt32LittleEndian(scene.Session.LocalState.MetaZoneRecords.Span[76..]));
        Assert.False(scene.Surface.Gameplay.RuntimeCallActive(scene.Session)); Assert.True(scene.Surface.Storage.Borrowed());
    }

    /// <summary>다른 실제 Pool/현재 Client를 묵인하지 않고 같은 분배 재확인과 반납 뒤 설치를 구별한다.</summary>
    [Fact]
    public void DispatchAndSelectedWorldRequireSameActualPoolAndCurrentLocalOwner()
    {
        using var scene = new Scene(); using var other = new Scene();
        scene.Session.BindStateDeletion(scene.Dispatch); Assert.Throws<ArgumentException>(() => scene.Session.BindStateDeletion(other.Dispatch));
        var differentSurface = new ClientSurfaceGlobals(scene.Surface.Gameplay, scene.Surface.Clock, scene.Surface.Kernel, () => 1000);
        var cleanup = new ClientStateStorageCleanup(differentSurface, scene.Session.LocalState, new()); scene.Marker(5);
        Assert.Throws<InvalidOperationException>(cleanup.ClearMetaZones); Assert.False(scene.Surface.Gameplay.RuntimeCallActive(scene.Session));
        scene.Session.ReleasePlayerRecordViews(); Assert.Throws<InvalidOperationException>(() => scene.Session.BindStateDeletion(scene.Dispatch));
    }
}
