using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;

// 실제 공통 writer·전체 예약·현재 typed 대여와 Runtime 수명 선택을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>네트워크/게임 실행 없이 같은 Storage 전송 경계에 실제 writer 입력을 제공한다.</summary>
public sealed class SurfaceTransmissionCurrentTests
{
    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord
        { ConstructorAddress = OriginalConstructors.Address(index), Flags1 = OriginalTypeRecord.HasHitPoints, MaxHitPoints = 1001, FootX = 1, FootY = 1 }).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(), Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray());
        storage.GameAux().SquidTransmission.Compress = 0; storage.GameAux().SquidTransmission.ClientSidAllowed = 1; return storage;
    }
    /// <summary>선택되지 않은 함수도 실제 설치 계약을 만족하는 명시 검사 장치다.</summary>
    private static RawSurfaceTransmissionDevices Devices(Action<byte[]>? broadcast = null) => new()
    { Broadcast = broadcast ?? (_ => { }), Connected = _ => false, Send = (_, _) => { } };
    /// <summary>실제 독립 Pop 함수만 있는 현재 대여다. 나머지 장치는 선택 검사에서 따로 설치한다.</summary>
    private static RawSurfacePopBindings Runtime() => new() { Transmit = (_, _) => { } };

    /// <summary>실제 방송 장치 결합만 Native 정책을 선택하며 함수 값 복사/초기 설치에서 효과를 만들지 않는다.</summary>
    [Fact]
    public void NativePolicyUsesExplicitCopiedDevicesWithoutQueries()
    {
        var storage = Storage(); Assert.False(storage.UsesNativeTransmissionDevices()); int calls = 0;
        var input = Devices(_ => calls++); storage.BindTransmissionDevices(input); Assert.True(storage.UsesNativeTransmissionDevices()); Assert.Equal(0, calls);
        input.Broadcast = _ => throw new ApplicationException("바깥 함수 교체"); int sid = storage.FactoryExecutor().Create(74); storage.TransmissionExecutor().Pop(sid, 0); Assert.Equal(1, calls);
        Assert.Throws<ArgumentException>(() => storage.BindTransmissionDevices(Devices())); var missing = Storage(); Assert.Throws<ArgumentException>(() => missing.BindTransmissionDevices(new())); Assert.False(missing.UsesNativeTransmissionDevices());
        var cd = new RawSurfaceStorage(OriginalEdition.Cd1072); Assert.Throws<ArgumentException>(() => cd.BindTransmissionDevices(Devices()));
    }
    /// <summary>각 실제 초기 사용 뒤의 Native 장치 설치는 조회/생성 효과 없이 거부한다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void NativeInstallationMustPrecedeActualOwners(int selected)
    {
        var storage = Storage(); IDisposable? lease = null;
        switch (selected)
        {
            case 0: lease = storage.AcquirePrepared(); break;
            case 1: storage.FactoryExecutor(); break;
            case 2: storage.Processes(); break;
            case 3: storage.TransmissionExecutor(); break;
        }
        try { Assert.Throws<ArgumentException>(() => storage.BindTransmissionDevices(Devices())); Assert.False(storage.UsesNativeTransmissionDevices()); }
        finally { lease?.Dispose(); }
    }
    /// <summary>Native 방송/대상은 같은 전체 예약을 받고 typed 관찰이나 그 pin을 선택하지 않는다.</summary>
    [Fact]
    public void NativeDispatchRetainsWholeMutableBufferAndCurrentTargetGate()
    {
        var storage = Storage(); byte[] buffer = storage.GameAux().SquidTransmission.Buffer; Array.Fill(buffer, (byte)0xcc); var events = new List<string>();
        storage.BindTransmissionDevices(new()
        {
            Broadcast = packet => { Assert.Same(buffer, packet); Assert.Equal(2048, packet.Length); Assert.Equal(0xcc, packet[2047]); packet[2047] = 0x77; events.Add("broadcast"); },
            Connected = target => { events.Add($"connected{target}"); return true; },
            Send = (target, packet) => { Assert.Same(buffer, packet); Assert.Equal(0x77, packet[2047]); events.Add($"send{target}"); },
        });
        var runtime = Runtime(); runtime.Pin = () => throw new Exception("미선택 typed pin"); runtime.Broadcast = _ => throw new Exception("미선택 typed 방송"); storage.BindPopRuntime(runtime);
        int sid = storage.FactoryExecutor().Create(74); var writer = storage.TransmissionExecutor(); writer.Pop(sid, 0); writer.SendTo(sid, 3, 0);
        Assert.Equal(new[] { "broadcast", "connected3", "send3" }, events); storage.ReleasePopRuntime(runtime);
    }
    /// <summary>typed 관찰은 같은 버퍼의 반환35BYTE만 받고 각 선택된 대여 호출만 pin한다.</summary>
    [Fact]
    public void TypedDispatchKeepsReturnedSliceAndRechecksCurrentFunctions()
    {
        var storage = Storage(); var runtime = Runtime(); int pins = 0, broadcasts = 0, sends = 0; bool connected = false; runtime.Pin = () => { pins++; return null; };
        runtime.Broadcast = packet =>
        {
            Assert.Equal(35, packet.Length); Assert.True(MemoryMarshal.TryGetArray(packet, out var segment)); Assert.Same(storage.GameAux().SquidTransmission.Buffer, segment.Array); broadcasts++;
        };
        runtime.Connected = _ => connected; runtime.Send = (_, packet) => { Assert.Equal(35, packet.Length); sends++; }; storage.BindPopRuntime(runtime); storage.BindPopRuntime(runtime);
        Assert.Throws<ArgumentException>(() => storage.BindPopRuntime(Runtime())); int sid = storage.FactoryExecutor().Create(74); var writer = storage.TransmissionExecutor(); writer.Pop(sid, 0); Assert.Equal(1, pins);
        writer.SendTo(sid, 2, 0); Assert.Equal(2, pins); Assert.Equal(0, sends); connected = true; writer.SendTo(sid, 2, 0); Assert.Equal(4, pins); Assert.Equal(1, sends);
        runtime.Broadcast = packet => { Assert.Equal(35, packet.Length); broadcasts += 10; }; writer.Pop(sid, 0); Assert.Equal(11, broadcasts);
        storage.ReleasePopRuntime(runtime); Assert.Throws<InvalidOperationException>(() => writer.Pop(sid, 0)); Assert.False(storage.Borrowed());
    }
    /// <summary>ProcessForm은 실제 현재 외부 writer만 선택하며 반납 뒤에는 장기 Host의 실제 PID 누락을 보고한다.</summary>
    [Fact]
    public void ProcessPacketUsesCurrentWriterThenActualLongHost()
    {
        var storage = Storage(); int broadcasts = 0; storage.BindTransmissionDevices(Devices(packet => { Assert.Equal(0x6a, packet[1]); broadcasts++; }));
        int form = new RawSquidForms(storage.Pool).Create(2); var runtime = Runtime(); int pins = 0; runtime.Pin = () => { pins++; return null; };
        runtime.ProcessPacket = (sid, packet, argument) => { Assert.Equal(form, sid); Assert.Equal(77u, argument); packet[1] = 0x6a; return 5; }; storage.BindPopRuntime(runtime);
        storage.TransmissionExecutor().Transmit(form, -1, -1, 77, 0); Assert.Equal(1, broadcasts); Assert.Equal(1, pins);
        runtime.ProcessPacket = (_, _, _) => -1; storage.TransmissionExecutor().Pop(form, 0); Assert.Equal(1, broadcasts);
        storage.ReleasePopRuntime(runtime); var error = Assert.Throws<InvalidOperationException>(() => storage.TransmissionExecutor().Pop(form, 0)); Assert.Contains("실제 PID 객체 누락", error.Message);
    }
    /// <summary>pin이 선택 뒤 함수를 반납해도 다른 장치/Host 또는 빈 성공으로 바꾸지 않는다.</summary>
    [Fact]
    public void PinMutationPreservesSelectedMissingFunctionFailures()
    {
        var storage = Storage(); var runtime = Runtime(); runtime.Pin = () => { runtime.Transmit = null; return null; }; storage.BindPopRuntime(runtime);
        Assert.Throws<InvalidOperationException>(() => storage.TransmitPop(99999, 0));
        runtime.Broadcast = _ => { }; runtime.Pin = () => { runtime.Broadcast = null; return null; }; int sid = storage.FactoryExecutor().Create(74); Assert.Throws<InvalidOperationException>(() => storage.TransmissionExecutor().Pop(sid, 0));
        runtime.ProcessPacket = (_, _, _) => 0; runtime.Pin = () => { runtime.ProcessPacket = null; return null; }; int form = new RawSquidForms(storage.Pool).Create(2);
        Assert.Throws<InvalidOperationException>(() => storage.TransmissionExecutor().Pop(form, 0)); storage.ReleasePopRuntime(runtime);
    }
    /// <summary>ProcessForm 방송의 반환0도 실제 Native 장치를 요구하고 양수만 전체 예약을 보낸다.</summary>
    [Fact]
    public void ProcessFormBroadcastKeepsSignedBoundaryAndActualNativeDevice()
    {
        var storage = Storage(); Assert.Throws<InvalidOperationException>(() => storage.BroadcastProcessForm(0)); int calls = 0; storage.BindTransmissionDevices(Devices(packet => { Assert.Same(storage.GameAux().SquidTransmission.Buffer, packet); calls++; }));
        storage.BroadcastProcessForm(0); Assert.Equal(0, calls); storage.BroadcastProcessForm(2048); Assert.Equal(1, calls);
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.BroadcastProcessForm(-1)); Assert.Throws<ArgumentOutOfRangeException>(() => storage.BroadcastProcessForm(2049)); Assert.Equal(1, calls);
    }
    /// <summary>현재 Runtime owner는 선택 호출 정상/예외 복귀까지 유지되고 그 뒤 수집된다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void CurrentRuntimePinSurvivesOnlyTheSelectedCall(bool fail)
    {
        WeakReference weak = InvokePinned(fail); Collect(); Assert.False(weak.IsAlive);
    }
    /// <summary>JIT 임시 참조와 실제 pin 소유 범위를 분리한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference InvokePinned(bool fail)
    {
        var storage = Storage(); object? owner = new(); var weak = new WeakReference(owner); var runtime = Runtime(); runtime.Pin = () => owner;
        runtime.Transmit = (_, _) => { owner = null; runtime.Pin = null; Collect(); Assert.True(weak.IsAlive); if (fail) throw new ApplicationException("명시 송신 중단"); };
        storage.BindPopRuntime(runtime); if (fail) Assert.Throws<ApplicationException>(() => storage.TransmitPop(0, 0)); else storage.TransmitPop(0, 0); storage.ReleasePopRuntime(runtime); return weak;
    }
    /// <summary>같은 writer/공통 버퍼가 두 World에서 유지되고 반납 후 죽은 typed World를 호출하지 않는다.</summary>
    [Fact]
    public void WorldsReuseWriterAndReleaseTypedRuntime()
    {
        var storage = Storage(); RawSquidTransmission writer; int sid;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        { writer = storage.TransmissionExecutor(); sid = first.Core().Factory.Create(74); writer.SameOwnerMode(1); writer.Pop(sid, 0); }
        Assert.Throws<InvalidOperationException>(() => writer.Pop(sid, 0)); byte[] buffer = storage.GameAux().SquidTransmission.Buffer;
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        { Assert.Same(writer, storage.TransmissionExecutor()); Assert.Same(buffer, storage.GameAux().SquidTransmission.Buffer); Assert.Equal(1u, storage.GameAux().SquidTransmission.SameOwnerEnabled); writer.Pop(sid, 0); }
        Assert.Throws<InvalidOperationException>(() => writer.Pop(sid, 0));
    }
    /// <summary>다른 현재 Pop 대여 거부 뒤 기존 writer 대여와 새 World lease 반납을 검증한다.</summary>
    [Fact]
    public void RejectedPopBindingPreservesOtherRuntimeAndReturnsWorldLease()
    {
        var storage = Storage(); var runtime = Runtime(); int calls = 0; runtime.Transmit = (_, _) => calls++; storage.BindPopRuntime(runtime);
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true)); Assert.False(storage.Borrowed()); storage.TransmitPop(0, 0); Assert.Equal(1, calls);
        storage.ReleasePopRuntime(runtime); using var lease = storage.AcquirePrepared(); Assert.True(storage.Borrowed());
    }
    /// <summary>Native 공장 Form 부착도 실제 같은 장기 writer/버퍼와 공유 깊이를 사용한다.</summary>
    [Fact]
    public void NativeFactoryFormAttachUsesActualLongTransmission()
    {
        var storage = Storage(); int calls = 0; storage.BindTransmissionDevices(Devices(packet => { Assert.Same(storage.GameAux().SquidTransmission.Buffer, packet); calls++; })); var factory = storage.FactoryExecutor();
        using (var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage)) Assert.Same(factory, world.Core().Factory);
        int parent = factory.Create(74); int form = new RawSquidForms(storage.Pool).Create(1); storage.PostPopState.Depth = 9; factory.AttachForm(form, parent, 0);
        Assert.Equal(1, calls); Assert.Equal(9u, storage.PostPopState.Depth); Assert.Equal(parent, RawContainedLink.CommonContainedParent(storage.Pool, form));
    }
    /// <summary>장기 writer는 명시 종료된 World를 소유하지 않는다.</summary>
    [Fact]
    public void LongWriterDoesNotKeepDisposedWorldAlive()
    {
        var storage = Storage(); WeakReference weak = DisposeWorld(storage); Collect(); Assert.False(weak.IsAlive); GC.KeepAlive(storage);
    }
    /// <summary>종료 World 지역 참조를 수집 검사 프레임 밖에 둔다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DisposeWorld(RawSurfaceStorage storage)
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage); storage.TransmissionExecutor(); var weak = new WeakReference(world); world.Dispose(); return weak;
    }
    /// <summary>동기 호출과 호출 밖의 실제 소유만 검사한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void Collect() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }
}
