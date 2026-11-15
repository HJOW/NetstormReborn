using System.Runtime.CompilerServices;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 현재 카메라 조회·짧은 World 대여·실제 기본 장치 복귀와 호출 수명을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj SquidDisplay/RawSurfaceStorage의 현재 표시 수명과 원문 조회 순서를 대조한다.</summary>
public sealed class SurfaceDisplayCurrentTests
{
    /// <summary>프레임16×11/핫스폿1×2를 실제 표시 모듈이 읽도록 입력한다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([new((byte)'A', (byte)'0', 0, 0)])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ =>
        {
            var shape = new SquidDisplayShape { Loaded = true, FrameCount = 1 };
            shape.Frames.Add(new() { Width = 16, Height = 11, HotspotX = 1, HotspotY = 2, CellWidth = 1, CellHeight = 1 }); return shape;
        }).ToArray();
        return new(OriginalEdition.Patch1078, types, frames, shapes);
    }
    /// <summary>같은 원문 픽셀 좌표/양수 배율/전체 viewport를 갖는 입력 뷰다.</summary>
    private static SquidDisplayView View(int x = 0) => new() { CameraX = x, Zoom = 65536, Viewport = new(0, 0, 1024, 768) };
    /// <summary>화면/Renderer 실행 없이 명시 관찰만 제공하는 기본 장치 입력이다.</summary>
    private static RawSurfaceDisplayBindings Devices(int x = 0) => new() { View = () => View(x), Suppressed = () => false, Invalidate = (_, _) => { } };

    /// <summary>기본 장치 설치는 실제 함수를 호출하지 않고 값 복사하며 미연결은 선택 위치에서 실패한다.</summary>
    [Fact]
    public void BindingIsLazyAndCopiesDefaultFunctions()
    {
        var storage = Storage(); Assert.Throws<InvalidOperationException>(() => storage.DisplayExecutor()); Assert.False(storage.Borrowed());
        int reads = 0; var input = Devices(); input.View = () => { reads++; return View(10); };
        storage.BindDisplayDevices(input); Assert.Equal(0, reads); input.View = () => throw new ApplicationException("외부 함수 재대입");
        Assert.Equal(10, storage.CurrentDisplayView().CameraX); Assert.Equal(1, reads);
        var display = storage.DisplayExecutor(); Assert.Equal(2, reads); Assert.Same(display, storage.DisplayExecutor()); Assert.Equal(2, reads);
        Assert.Throws<ArgumentException>(() => storage.BindDisplayDevices(new())); Assert.Equal(10, storage.CurrentDisplayView().CameraX); Assert.Equal(0ul, storage.Generation());
    }

    /// <summary>SetView는 원문 값 사본이며 현재 getter는 좌표/배율의 여섯 위치마다 다시 호출한다.</summary>
    [Fact]
    public void CurrentViewReadsEachProjectionAndScaleAtOriginalPosition()
    {
        var storage = Storage(); storage.BindDisplayDevices(Devices()); var display = storage.DisplayExecutor();
        var independent = View(); display.SetView(independent); independent.CameraX = 999;
        display.BindView(null); Assert.Equal(new DisplayRect(159, 218, 176, 230), display.Bounds(74, 0, 10, 20));
        int reads = 0; display.BindView(() =>
        {
            reads++; return new() { CameraX = 20, CameraY = 30, Zoom = reads switch { 2 => 131072, 5 => 32768, 6 => 196608, _ => 65536 }, Viewport = View().Viewport };
        });
        Assert.Equal(0, reads); Assert.Equal(new DisplayRect(138, 188, 147, 222), display.Bounds(74, 0, 10, 20)); Assert.Equal(6, reads);
        display.BindView(() => null!); Assert.Throws<InvalidOperationException>(() => display.Bounds(74, 0, 10, 20));
        display.BindView(null); Assert.Equal(new DisplayRect(159, 218, 176, 230), display.Bounds(74, 0, 10, 20));
    }

    /// <summary>동일 대여는 다시 허용하고 다른 살아 있는 World는 덮지 않으며 반납 뒤 기본 Client 장치를 쓴다.</summary>
    [Fact]
    public void RuntimePriorityAndReturnUseActualCurrentDevices()
    {
        var storage = Storage(); var events = new List<string>(); var defaults = Devices(1); defaults.Invalidate = (_, flags) => events.Add($"D{flags}"); storage.BindDisplayDevices(defaults);
        var current = Devices(2); current.Suppressed = () => true; current.Invalidate = (_, flags) => events.Add($"R{flags}"); current.Changed = () => events.Add("changed");
        storage.BindDisplayRuntime(current); storage.BindDisplayRuntime(current); Assert.Throws<ArgumentException>(() => storage.BindDisplayRuntime(Devices(3)));
        Assert.Equal(2, storage.CurrentDisplayView().CameraX); Assert.True(storage.DisplaySuppressed()); storage.InvalidateDisplay(new(1, 2, 3, 4), 7); storage.NoteChanged();
        storage.ReleaseDisplayRuntime(current); Assert.Equal(1, storage.CurrentDisplayView().CameraX); Assert.False(storage.DisplaySuppressed()); storage.InvalidateDisplay(new(1, 2, 3, 4), 9); storage.NoteChanged();
        Assert.Equal(new[] { "R7", "changed", "D9" }, events); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }

    /// <summary>현재 대여에 실제 메타 장치가 없으면 장기 장치를 선택하며 미선택 runtime pin은 호출하지 않는다.</summary>
    [Fact]
    public void MetaFallbackDoesNotSelectMissingRuntimeProxyOrItsPin()
    {
        var storage = Storage(); var defaults = Devices(); var events = new List<string>(); defaults.MetaRefresh = raw => events.Add($"D{raw[0]}"); storage.BindDisplayDevices(defaults);
        int pins = 0; object currentOwner = new(); var current = Devices(); current.Pin = () => { pins++; return currentOwner; };
        storage.BindDisplayRuntime(current); storage.RefreshMetaDisplay([7]); Assert.Equal(0, pins);
        current.MetaRefresh = raw => events.Add($"R{raw[0]}"); storage.RefreshMetaDisplay([8]); Assert.Equal(1, pins);
        Assert.Equal(new[] { "D7", "R8" }, events); storage.ReleaseDisplayRuntime(current);
        var missing = Storage(); missing.BindDisplayDevices(Devices()); Assert.Throws<InvalidOperationException>(() => missing.RefreshMetaDisplay([1]));
    }

    /// <summary>pin이 선택 뒤 장치를 반납해도 기본 성공으로 바꾸지 않고 실제 누락 위치에서 실패한다.</summary>
    [Fact]
    public void PinMutationKeepsSelectedMetaAndChangedFailure()
    {
        var storage = Storage(); var current = Devices(); current.MetaRefresh = _ => throw new Exception("반납 전 장치"); current.Changed = () => throw new Exception("반납 전 관찰");
        current.Pin = () => { current.MetaRefresh = null; current.Changed = null; return null; }; storage.BindDisplayRuntime(current);
        Assert.Throws<InvalidOperationException>(() => storage.RefreshMetaDisplay([1])); current.Changed = () => { };
        Assert.Throws<InvalidOperationException>(() => storage.NoteChanged()); storage.ReleaseDisplayRuntime(current);
    }

    /// <summary>같은 장기 표시가 World의 현재 뷰/dirty/실제 메타만 빌리고 반납 뒤 실제 기본 장치로 돌아간다.</summary>
    [Fact]
    public void WorldReusesDisplayAndReturnsToDefaultClientDevices()
    {
        var storage = Storage(); var defaults = Devices(99); int defaultMeta = 0, worldMeta = 0, dirty = 0; defaults.MetaRefresh = _ => defaultMeta++; storage.BindDisplayDevices(defaults);
        SquidDisplay display;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            display = first.Core().Display; Assert.Same(display, storage.DisplayExecutor()); var view = View(23);
            first.SetDisplay(view, new SurfaceDisplayHooks { Invalidate = (_, _) => dirty++ }); view.CameraX = 55;
            Assert.Equal(23, storage.CurrentDisplayView().CameraX); storage.InvalidateDisplay(new(1, 2, 3, 4), 5); Assert.Equal(1, dirty);
            storage.RefreshMetaDisplay([1]); Assert.Equal(1, defaultMeta); first.BindMetaRefresh(_ => worldMeta++); storage.RefreshMetaDisplay([2]); Assert.Equal(1, worldMeta);
        }
        Assert.Equal(99, storage.CurrentDisplayView().CameraX); storage.RefreshMetaDisplay([3]); Assert.Equal(2, defaultMeta);
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        { Assert.Same(display, next.Core().Display); Assert.Equal(0, storage.CurrentDisplayView().CameraX); }
        Assert.Equal(99, storage.CurrentDisplayView().CameraX); Assert.False(storage.Borrowed());
    }

    /// <summary>표시 대여가 먼저 거부되면 기존 대여는 유지하고 새 World의 물리 lease만 반납한다.</summary>
    [Fact]
    public void RejectedWorldDisplayBindingDoesNotLeakStorageLease()
    {
        var storage = Storage(); var current = Devices(7); storage.BindDisplayRuntime(current);
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true));
        Assert.False(storage.Borrowed()); Assert.Equal(1ul, storage.Generation()); Assert.Equal(7, storage.CurrentDisplayView().CameraX);
        storage.ReleaseDisplayRuntime(current); using var lease = storage.AcquirePrepared(); Assert.Equal(2ul, storage.Generation());
    }

    /// <summary>같은 호출 owner가 getter의 외부 반납과 정상/예외 복귀를 모두 견디며 호출 뒤에는 수집된다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void RuntimePinKeepsOwnerOnlyAcrossTheSelectedCall(bool fail)
    {
        WeakReference weak = InvokePinnedView(fail); Collect(); Assert.False(weak.IsAlive);
    }
    /// <summary>JIT 임시 참조와 실제 pin 범위를 분리해 두 수명 검사를 모두 유지한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference InvokePinnedView(bool fail)
    {
        var storage = Storage(); object? owner = new(); var weak = new WeakReference(owner); var runtime = Devices(); runtime.Pin = () => owner;
        runtime.View = () => { owner = null; runtime.Pin = null; Collect(); Assert.True(weak.IsAlive); if (fail) throw new ApplicationException("명시 현재 뷰 중단"); return View(); };
        storage.BindDisplayRuntime(runtime);
        if (fail) Assert.Throws<ApplicationException>(() => storage.CurrentDisplayView()); else storage.CurrentDisplayView();
        storage.ReleaseDisplayRuntime(runtime); return weak;
    }
    /// <summary>장기 표시 실행기가 종료된 World를 숨겨 소유하지 않는지 전체 프레임 밖에서 확인한다.</summary>
    [Fact]
    public void StoredDisplayDoesNotKeepDisposedWorldAlive()
    {
        var storage = Storage(); storage.BindDisplayDevices(Devices()); WeakReference weak = DisposeWorld(storage); Collect(); Assert.False(weak.IsAlive); GC.KeepAlive(storage);
    }
    /// <summary>종료된 World의 지역 참조를 수집 확인 프레임 밖에 둔다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DisposeWorld(RawSurfaceStorage storage)
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage); var weak = new WeakReference(world); world.Dispose(); return weak;
    }
    /// <summary>동기 호출/프레임 밖 생존 검증에만 수집기를 강제한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void Collect() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }
}
