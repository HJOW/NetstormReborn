using Netstorm.Core.Original;
using Netstorm.Core.Display;

// 실제프로세스/표시호출이명시대여만반납하고일반소유자는보존하는지확인한다.
namespace Netstorm.Core.Tests;

/// <summary>현재호출계수와효과뒤명시Runtime대여의정상/예외반납을검사한다.</summary>
public sealed class RuntimeScopedLeaseTests
{
    /// <summary>검사가발급한한호출의명시대여다.</summary>
    private sealed class Lease(Action released) : IRawRuntimeLease
    {
        /// <summary>중복반납을검출하는실제상태다.</summary>
        private bool _released;
        /// <summary>마지막효과/호출계수조건을실제반납위치에서검사한다.</summary>
        public void Dispose() { Assert.False(_released); _released = true; released(); }
    }
    /// <summary>기존Pin콜백이강하게보유할뿐인일반소유자다.</summary>
    private sealed class Owner : IDisposable
    {
        /// <summary>일반소유자의명시Dispose호출수다.</summary>
        public int Calls;
        /// <summary>Pin의반납이게임소유자를파괴하는지검출한다.</summary>
        public void Dispose() => Calls++;
    }

    /// <summary>프로세스guard는호출계수를먼저내리고각각의대여를마지막에정확히한번반납한다.</summary>
    [Fact]
    public void NestedProcessGuardsReleaseLeasesAfterCurrentCounters()
    {
        var runtime = new ProcessHostRuntimeBindings(); int released = 0;
        runtime.Pin = () => new Lease(() => { released++; Assert.Equal(released == 1 ? 1ul : 0ul, (ulong)runtime.ActiveCalls); });
        var first = new RuntimeCallGuard(runtime); var second = new RuntimeCallGuard(runtime); Assert.Equal(2ul, (ulong)runtime.ActiveCalls);
        second.Dispose(); second.Dispose(); Assert.Equal(1, released); Assert.Equal(1ul, (ulong)runtime.ActiveCalls);
        first.Dispose(); first.Dispose(); Assert.Equal(2, released); Assert.Equal(0ul, (ulong)runtime.ActiveCalls);
    }

    /// <summary>예외복귀도현재계수와대여를반납하지만일반IDisposable소유자를대신파괴하지않는다.</summary>
    [Fact]
    public void ExceptionUnwindReleasesLeaseAndKeepsOrdinaryDisposableOwner()
    {
        var runtime = new ProcessHostRuntimeBindings(); int released = 0; runtime.Pin = () => new Lease(() => released++);
        Assert.Throws<IOException>((Action)(() => { using var call = new RuntimeCallGuard(runtime); throw new IOException("선택효과실패"); }));
        Assert.Equal(1, released); Assert.Equal(0ul, (ulong)runtime.ActiveCalls);
        var owner = new Owner(); runtime.Pin = () => owner; using (var call = new RuntimeCallGuard(runtime)) Assert.Equal(0, owner.Calls);
        Assert.Equal(0, owner.Calls); Assert.Equal(0ul, (ulong)runtime.ActiveCalls);
    }

    /// <summary>표시의현재장치가callback중교체/예외를일으켜도선택한대여를마지막효과뒤반납한다.</summary>
    [Fact]
    public void DisplaySelectionKeepsLeaseThroughChangedAndExceptionalView()
    {
        var storage = new RawSurfaceStorage(); int active = 0, releases = 0, changed = 0;
        var runtime = new RawSurfaceDisplayBindings
        {
            Pin = () => { active++; return new Lease(() => { active--; releases++; }); },
            View = () => { Assert.Equal(1, active); throw new IOException("현재카메라실패"); },
            Suppressed = () => { Assert.Equal(1, active); return true; }, Invalidate = (_, _) => { },
        };
        runtime.Changed = () => { Assert.Equal(1, active); changed++; storage.ReleaseDisplayRuntime(runtime); };
        storage.BindDisplayRuntime(runtime); Assert.Throws<IOException>(() => storage.CurrentDisplayView()); Assert.Equal(0, active); Assert.Equal(1, releases);
        Assert.True(storage.DisplaySuppressed()); Assert.Equal(0, active); Assert.Equal(2, releases);
        storage.NoteChanged(); Assert.Equal(1, changed); Assert.Equal(0, active); Assert.Equal(3, releases);
        storage.NoteChanged(); Assert.Equal(3, releases);
    }

    /// <summary>실제공장의Form분리예외와현재대여반납요청뒤에도계수/marker를끝까지정리한다.</summary>
    [Fact]
    public void ActualFactoryUnpopReturnsLeaseAfterDeferredRuntimeRelease()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes); var factory = storage.FactoryExecutor();
        storage.BindDisplayDevices(new()
        {
            View = () => new() { Zoom = 65536, Viewport = new(0, 0, 1024, 768) },
            Suppressed = () => false, Invalidate = (_, _) => { },
        });
        int parent = storage.Pool.Allocate(), form = storage.Pool.Allocate(); Span<byte> raw = storage.Pool.AllocatedBytes(form); raw.Clear();
        storage.Pool.AllocatedBytes(parent).Clear(); storage.Pool.AllocatedBytes(parent)[10] = 1;
        BitConverter.TryWriteBytes(raw, 0x512098u); raw[10] = 2; raw[11] = 8; raw[16] = (byte)parent; raw[17] = (byte)(parent >> 8);
        int active = 0, released = 0; var runtime = new SquidFactoryRuntimeBindings
        {
            Unpop = storage.UnpopExecutor(), ContentType = () => 6, FormTransmit = (_, _) => { },
            Pin = () => { active++; return new Lease(() => { active--; released++; }); },
        };
        runtime.FormDetach = _ => { Assert.Equal(1, active); Assert.Equal(1u, runtime.ActiveCalls); factory.ReleaseRuntime(runtime); throw new IOException("실제분리효과실패"); };
        factory.BindRuntime(runtime); Assert.Throws<IOException>(() => factory.UnpopForm(form));
        Assert.Equal(0, active); Assert.Equal(1, released); Assert.Equal(0u, runtime.ActiveCalls);
        var next = new SquidFactoryRuntimeBindings { Unpop = storage.UnpopExecutor(), ContentType = () => 6, FormTransmit = (_, _) => { }, FormDetach = _ => { } };
        factory.BindRuntime(next); factory.ReleaseRuntime(next);
    }
}
