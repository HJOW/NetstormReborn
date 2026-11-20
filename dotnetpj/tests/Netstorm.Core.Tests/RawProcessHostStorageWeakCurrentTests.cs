using System.Runtime.CompilerServices;
using Netstorm.Core.Original;

// 장기 Host와 정리 증빙이 짧은 Runtime을 역소유하지 않는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 weak_ptr와 실제 장치 재확인/legacy 교체 관문을 검증한다.</summary>
public sealed class RawProcessHostStorageWeakCurrentTests
{
    /// <summary>같은 풀/188TYPE와 Kernel/Frame을 가진 실제 장기 장치를 구성한다.</summary>
    private static (RawProcessHostStorage Storage, RawSidPool Pool, OriginalTypeRecord[] Types, Kernel Kernel, FrameTimeBox Frame) Scene(bool bindDevices = true)
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity); OriginalTypeRecord[] types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        var kernel = new Kernel(); var frame = new FrameTimeBox(); var storage = new RawProcessHostStorage(pool, types, new RawAuthorityState());
        if (bindDevices) storage.BindDevices(kernel, frame); return (storage, pool, types, kernel, frame);
    }
    /// <summary>같은 실제 풀의 삭제 본체만 갖는 유효한 대여를 만든다.</summary>
    private static ProcessHostRuntimeBindings Binding(RawSidPool pool, OriginalTypeRecord[] types) =>
        new() { Destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types) };
    /// <summary>Use/호출 snapshot이 사라진 뒤 장기 저장소만 남기는 별도 JIT 수명 경계다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DropBorrow(RawProcessHostStorage storage, RawSidPool pool, OriginalTypeRecord[] types)
    {
        ProcessHostRuntimeBindings runtime = Binding(pool, types); _ = storage.Bind(runtime); return new WeakReference(runtime);
    }
    /// <summary>현재 Use를 반납하되 정상 Clear의 증빙만 남기는 별도 수명 경계다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (RawProcessHostStorage.ClearPass Pass, WeakReference Runtime) DropBorrowKeepingPass(RawProcessHostStorage storage, RawSidPool pool, OriginalTypeRecord[] types)
    {
        ProcessHostRuntimeBindings runtime = Binding(pool, types); var use = storage.Bind(runtime); RawProcessHostStorage.ClearPass pass = use.ClearForState(); use.Dispose(); return (pass, new WeakReference(runtime));
    }
    /// <summary>강한 관측 참조를 만들지 않고 종료된 별도 호출 프레임의 객체를 수집한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void CollectReleased() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }

    /// <summary>장기 저장소만 남으면 현재 대여가 만료되고 같은 Host에 다음 Runtime을 빌릴 수 있다.</summary>
    [Fact]
    public void LongStorageDoesNotOwnDroppedCurrentRuntime()
    {
        var (storage, pool, types, _, _) = Scene(); WeakReference weak = DropBorrow(storage, pool, types); SquidProcessHost host = storage.Host(); CollectReleased();
        Assert.False(weak.IsAlive); Assert.Throws<InvalidOperationException>(() => host.Runtime());
        var next = Binding(pool, types); using var use = storage.Bind(next); Assert.Same(host, storage.Host()); Assert.Same(next, host.Runtime());
    }
    /// <summary>발급 Use가 끝난 뒤 남은 정상 증빙은 이전 Runtime을 살아 있게 만들지 않는다.</summary>
    [Fact]
    public void ClearPassDoesNotOwnReleasedRuntime()
    {
        var (storage, pool, types, _, _) = Scene(); var (pass, weak) = DropBorrowKeepingPass(storage, pool, types); CollectReleased();
        Assert.False(weak.IsAlive); Assert.Throws<InvalidOperationException>(() => storage.Host().Runtime()); GC.KeepAlive(pass);
    }
    /// <summary>원문의 동일주소 early return은 현재 대여 중에도 같은 시각 상자와 값을 보존한다.</summary>
    [Fact]
    public void SameDeviceConfirmationPreservesBorrowedClockAndCurrentRuntime()
    {
        var (storage, pool, types, kernel, frame) = Scene(); var runtime = Binding(pool, types); using var use = storage.Bind(runtime); DoubleBox same = storage.State().SharedNow!; same.Value = 21.5;
        storage.BindDevices(kernel, frame); Assert.Same(same, storage.State().SharedNow); Assert.Equal(21.5, frame.Value.Game); Assert.Same(runtime, storage.Host().Runtime());
        Assert.Throws<ArgumentException>(() => storage.BindDevices(kernel, new FrameTimeBox())); Assert.Same(same, storage.State().SharedNow);
    }
    /// <summary>최초 외부 장치 결합도 살아 있는 legacy 회수 대여를 바꾸지 못한다.</summary>
    [Fact]
    public void LegacyReclaimRejectsFirstExternalDeviceBinding()
    {
        var (storage, _, _, kernel, frame) = Scene(false); Action<int> reclaim = _ => { }; _ = storage.BindLegacyAllocationReclaim(reclaim);
        Assert.Throws<ArgumentException>(() => storage.BindDevices(kernel, frame)); Assert.False(storage.HasDevices()); GC.KeepAlive(reclaim);
    }
}
