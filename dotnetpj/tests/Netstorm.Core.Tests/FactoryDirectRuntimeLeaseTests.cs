using Netstorm.Core.Original;

// 기존공간/Form장치를유지하는직접공장대여와Native묶음의선택순서를검사한다.
namespace Netstorm.Core.Tests;

/// <summary>실제Form분리본문의중첩/예외/반납과직접owner선택을대조한다.</summary>
public sealed class FactoryDirectRuntimeLeaseTests
{
    /// <summary>한호출의marker대여이며중복반납을검출한다.</summary>
    private sealed class Lease(Action released) : IRawRuntimeLease
    {
        /// <summary>이미반납했는지다.</summary>
        private bool _disposed;
        /// <summary>호출계수반납뒤실제대여를정확히한번반납한다.</summary>
        public void Dispose() { Assert.False(_disposed); _disposed = true; released(); }
    }
    /// <summary>기존보유전용Pin의일반소유자를검사한다.</summary>
    private sealed class Owner : IDisposable
    {
        /// <summary>불필요한명시Dispose호출수다.</summary>
        public int Calls;
        /// <summary>일반소유자의파괴시도를기록한다.</summary>
        public void Dispose() => Calls++;
    }
    /// <summary>원문Form상태/부모입력과실제같은저장소공장을준비한다.</summary>
    private static (RawSurfaceStorage Storage, RawSquidFactory Factory, int Form) Scene()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536, Viewport = new(0, 0, 1024, 768) }, Suppressed = () => false, Invalidate = (_, _) => { } });
        int parent = storage.Pool.Allocate(), form = storage.Pool.Allocate(); storage.Pool.AllocatedBytes(parent).Clear(); storage.Pool.AllocatedBytes(parent)[10] = 1;
        Span<byte> raw = storage.Pool.AllocatedBytes(form); raw.Clear(); BitConverter.TryWriteBytes(raw, 0x512098u); raw[10] = 2; raw[11] = 8; raw[16] = (byte)parent; raw[17] = (byte)(parent >> 8);
        return (storage, storage.FactoryExecutor(), form);
    }

    /// <summary>직접분리예외도기존효과를호출하고marker만반납하며일반소유자는파괴하지않는다.</summary>
    [Fact]
    public void DirectDetachFailureReturnsLeaseWithoutReplacingDevices()
    {
        var (storage, factory, form) = Scene(); int active = 0, releases = 0, effects = 0, expectedMarkerCalls = 1; var failure = new IOException("실제직접분리실패");
        factory.BindDirectRuntimePin(() => { active++; return new Lease(() => { active--; releases++; Assert.Equal(0ul, factory.ActiveRuntimeCalls); }); });
        factory.BindFormDetach(_ => { effects++; Assert.Equal(expectedMarkerCalls, active); Assert.Equal(1ul, factory.ActiveRuntimeCalls); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => factory.UnpopForm(form))); Assert.Equal(0, active); Assert.Equal(1, releases); Assert.Equal(1, effects); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
        Assert.NotEqual(0, storage.Pool.AllocatedBytes(form)[11] & RawSidPool.VoidBit); storage.Pool.AllocatedBytes(form)[11] = 8;
        expectedMarkerCalls = 0; var owner = new Owner(); factory.BindDirectRuntimePin(() => owner); Assert.Same(failure, Assert.Throws<IOException>(() => factory.UnpopForm(form))); Assert.Equal(2, effects); Assert.Equal(0, owner.Calls); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
    }

    /// <summary>현재Native묶음이있으면명시NULL pin도직접pin보다우선하며반납뒤직접장치로복귀한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void NativeRuntimePresenceControlsPinSelection(bool nativePin)
    {
        var (storage, factory, form) = Scene(); int directPins = 0, nativePins = 0, released = 0, directEffects = 0, nativeEffects = 0;
        factory.BindDirectRuntimePin(() => { directPins++; return new Lease(() => released++); }); factory.BindFormDetach(_ => directEffects++);
        var runtime = new SquidFactoryRuntimeBindings { Unpop = storage.UnpopExecutor(), ContentType = () => 6, FormTransmit = (_, _) => { }, FormDetach = _ => { nativeEffects++; Assert.Equal(1ul, factory.ActiveRuntimeCalls); } };
        if (nativePin) runtime.Pin = () => { nativePins++; return new Lease(() => released++); };
        factory.BindRuntime(runtime); factory.UnpopForm(form); Assert.Equal(nativePin ? 1 : 0, nativePins); Assert.Equal(0, directPins); Assert.Equal(1, nativeEffects); Assert.Equal(0, directEffects);
        factory.ReleaseRuntime(runtime); storage.Pool.AllocatedBytes(form)[11] = 8; factory.UnpopForm(form); Assert.Equal(1, directPins); Assert.Equal(1, directEffects); Assert.Equal(nativePin ? 2 : 1, released); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
    }

    /// <summary>같은기존분리효과의중첩호출은각대여와실행수를안쪽부터정확히반납한다.</summary>
    [Fact]
    public void NestedDirectCallsReleaseInOrderAfterCounters()
    {
        var (storage, factory, form) = Scene(); var counts = new List<ulong>(); bool nested = false;
        int inner = storage.Pool.Allocate(); storage.Pool.AllocatedBytes(form).CopyTo(storage.Pool.AllocatedBytes(inner));
        factory.BindDirectRuntimePin(() => new Lease(() => counts.Add(factory.ActiveRuntimeCalls)));
        factory.BindFormDetach(_ => { if (!nested) { nested = true; Assert.Equal(1ul, factory.ActiveRuntimeCalls); factory.UnpopForm(inner); } else Assert.Equal(2ul, factory.ActiveRuntimeCalls); });
        factory.UnpopForm(form); Assert.Equal(new ulong[] { 1, 0 }, counts); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
    }

    /// <summary>실제pin실패는물리입력검사전전달되며실행수와Form필드를쓰지않는다.</summary>
    [Fact]
    public void PinFailureDoesNotPublishAnActiveCall()
    {
        var (_, factory, _) = Scene(); var failure = new IOException("실제공장대여실패"); factory.BindDirectRuntimePin(() => throw failure);
        Assert.Same(failure, Assert.Throws<IOException>(() => factory.UnpopForm(0))); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
    }
}
