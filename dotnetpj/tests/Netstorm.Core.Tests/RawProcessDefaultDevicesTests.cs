using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 같은 장기 Host의 현재 대여/기본 장치와 실제 Kernel 수명을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>기본 장치는 실제 선택 위치에서만 읽고 현재 World의 부분 연결을 가짜 성공으로 바꾸지 않는지 검사한다.</summary>
public sealed class RawProcessDefaultDevicesTests
{
    /// <summary>실제 풀/커널/공장과 선택 가능한 현재 대여를 소유하는 입력 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 슬롯 저장소다.</summary>
        public RawSidPool Pool { get; } = new(32768, true);
        /// <summary>같은 현재 TYPE 배열이다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>실제 등록/삭제되는 Kernel이다.</summary>
        public Kernel Kernel { get; } = new(16);
        /// <summary>장기 현재 시각/권한/진단이다.</summary>
        public SquidProcessState State { get; } = new() { Boss = true, Server = true };
        /// <summary>현재 World가 없으면 null인 실제 대여다.</summary>
        public ProcessHostRuntimeBindings? Current { get; set; }
        /// <summary>같은 풀의 실제 공통 삭제다.</summary>
        public RawSquidDestroy Destroy { get; }
        /// <summary>같은 풀의 실제 생성/부착/분리 공장이다.</summary>
        public RawSquidFactory Factory { get; }
        /// <summary>안정된 장기 Host다.</summary>
        public SquidProcessHost Host { get; }
        /// <summary>실제로 할당한 부모 자산 번호다.</summary>
        public int Parent { get; }
        /// <summary>기본 자산/현재 선택 장치다.</summary>
        public SquidDestroyHooks Assets { get; } = new() { Selected = () => 0, SelectedRaw = () => 0 };
        /// <summary>장기 기본 Pop 깊이의 현재 입력이다.</summary>
        public uint Depth { get; set; }
        /// <summary>원문 복귀 진단을 기록한다.</summary>
        public List<(string Expression, string File, uint Line)> Assertions { get; } = [];
        /// <summary>장기 기본 Regular의 선택 효과다.</summary>
        public RegularHandler Handler { get; set; } = (_, _, _, payload) => payload;
        /// <summary>실제 서버 부모를 확보하고 현재 장치만 준비한다. 기본 결합은 별도다.</summary>
        public Scene()
        {
            // 실제 TYPE의 ProcessForm 생성자 주소를 사용한다. 0은 별도 기본 Squid 생성자다.
            Types[2].ConstructorAddress = OriginalConstructors.Address(Pool.Edition, 2);
            Parent = Pool.Allocate(SidAllocation.Server); Span<byte> raw = Pool.RawSlot(Parent); raw.Clear(); raw[10] = 74;
            BinaryPrimitives.WriteUInt32LittleEndian(raw, OriginalConstructors.BaseVtable);
            Destroy = new(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Destroy.BindPatchDiagnostics(new() { Assertion = (e, f, l) => Assertions.Add((e, f, l)), Trace = (_, _) => { } });
            Factory = new(Pool, Types); Factory.BindDiagnostics(new() { Types = Types, Assertion = (e, f, l) => Assertions.Add((e, f, l)) });
            Host = new(Pool, Types, Kernel, State, () => Current);
            Assets.Emit = effect =>
            {
                if (effect.Effect == SquidDestroyEffect.DestroyDependent) Destroy.Destroy(effect.Sid, effect.Flags, Host.Hooks());
                else if (effect.Effect == SquidDestroyEffect.PreDestroy) Destroy.CompletePreDestroy();
                else if (effect.Effect == SquidDestroyEffect.PostDestroy) Destroy.CompletePostDestroy();
            };
            State.Assertion = (e, f, l) => Assertions.Add((e, f, l));
        }
        /// <summary>장기 실제 저장소와 기본 장치를 최초 한 번 연결한다.</summary>
        public void Bind()
        {
            Host.BindDefaultLifecycle(() => Destroy, (parent, ev, count, payload) => Handler(parent, ev, count, payload), () => Assets);
            Host.BindDefaultFormFactory(() => Factory, () => Depth);
        }
    }

    /// <summary>기본 결합은 getter를 미리 실행하지 않고 중복/직접 Host 결합은 거부한다.</summary>
    [Fact]
    public void LifecycleBindingIsLazyAndSingleWithoutInventingRuntime()
    {
        var s = new Scene(); int reads = 0;
        Assert.Throws<InvalidOperationException>(() => s.Host.Destroy()); Assert.Throws<InvalidOperationException>(() => s.Host.Regular());
        s.Host.BindDefaultLifecycle(() => { reads++; return s.Destroy; }, (_, _, _, value) => value + 3, () => { reads++; return s.Assets; });
        Assert.Equal(0, reads); Assert.Equal(0, s.Kernel.Size()); Assert.Null(s.Current);
        Assert.Same(s.Destroy, s.Host.Destroy()); Assert.Equal(5f, s.Host.Regular()!(0, 0, 0, 2)); Assert.Same(s.Assets, s.Host.Asset()); Assert.Equal(2, reads);
        Assert.Throws<ArgumentException>(() => s.Host.BindDefaultLifecycle(() => s.Destroy, s.Handler, () => s.Assets));
        Assert.Throws<InvalidOperationException>(() => s.Host.Runtime());
        var direct = new SquidProcessHost(s.Pool, s.Types, s.Kernel, s.Destroy, s.State);
        Assert.Throws<ArgumentException>(() => direct.BindDefaultLifecycle(() => s.Destroy, s.Handler, () => s.Assets));
        Assert.Throws<ArgumentException>(() => direct.BindDefaultFormFactory(() => s.Factory, () => 0));
    }

    /// <summary>현재 World가 있으면 그 부분 연결을 그대로 선택하고 기본 장치로 빈 항목을 대체하지 않는다.</summary>
    [Fact]
    public void CurrentRuntimeWinsWithoutUnrelatedDestroyPrecheck()
    {
        var s = new Scene(); s.Bind();
        s.Current = new() { Regular = (_, _, _, _) => 7, Asset = new() { SelectedRaw = () => 0x80000005 } };
        Assert.Equal(7f, s.Host.Regular()!(0, 0, 0, 0)); Assert.Equal(0x80000005u, s.Host.Hooks().SelectedRaw!()); Assert.Equal(0ul, s.Current.ActiveCalls);
        Assert.Throws<InvalidOperationException>(() => s.Host.Destroy()); Assert.Throws<InvalidOperationException>(() => s.Host.CreateOrTakeForm(0x10, 0));
        s.Current.Regular = null; s.Current.Asset = null; Assert.Null(s.Host.Regular()); Assert.Null(s.Host.Asset());
        s.Current = null; Assert.Same(s.Destroy, s.Host.Destroy()); Assert.Equal(1f, s.Host.Regular()!(0, 0, 0, 1));
    }

    /// <summary>기본 공장 Pool은 결합 시 읽지 않고 실제 생성 위치에서 검사한다.</summary>
    [Fact]
    public void DefaultFactoryIsLazyAndRequiresSamePhysicalPoolAtUse()
    {
        var s = new Scene(); int reads = 0;
        var otherPool = new RawSidPool(32768, true); var other = new RawSquidFactory(otherPool, s.Types);
        RawSquidFactory selected = other;
        s.Host.BindDefaultFormFactory(() => { reads++; return selected; }, () => 0);
        Assert.Equal(0, reads); Assert.Throws<ArgumentException>(() => s.Host.CreateOrTakeForm(0x10, 0));
        selected = s.Factory; int form = s.Host.CreateOrTakeForm(0x10, 0); Assert.Equal(2, reads); Assert.Equal(2, s.Pool.Slot(form)[10]);
        Assert.Equal(SquidProcessHost.PatchFormVtable, BinaryPrimitives.ReadUInt32LittleEndian(s.Pool.Slot(form)));
        Assert.Throws<ArgumentException>(() => s.Host.BindDefaultFormFactory(() => s.Factory, () => 0));
    }

    /// <summary>실제 장기 Host는 World 없이 공장 부착/Regular 틱/Native 분배 삭제와 같은 FIFO 반납을 완료한다.</summary>
    [Fact]
    public void NoWorldRegularUsesActualFactoryKernelAndNativeDeletion()
    {
        var s = new Scene(); s.Bind(); int calls = 0; s.Handler = (_, _, _, _) => { calls++; return 0; };
        RegularProcess process = Assert.IsType<RegularProcess>(s.Host.AddRegular(s.Parent, 0x900, 1, 0x50));
        int form = process.Form(); uint pid = s.Kernel.Find(process); Assert.True(s.Host.IsRegistered(pid, process)); Assert.Null(s.Current);
        Assert.Equal(s.Parent, RawContainedLink.CommonContainedParent(s.Pool, form));
        s.Kernel.RunFrame(); Assert.Equal(1, calls); Assert.False(s.Host.IsRegistered(pid, process));
        Assert.Equal(1, s.Pool.Slot(form)[11] & 1); Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth);
        Assert.Equal((uint)form, s.Pool.Deletions(true)[0].Sid); Assert.Equal(46u, s.Pool.Deletions(true)[0].Type);
        // 삭제 이력은 고정20칸 순환 물리이며 아직 쓰지 않은19칸도 보존한다.
        Assert.All(s.Pool.Deletions(true).Skip(1), entry => Assert.Equal(default, entry)); Assert.Null(s.Current);
    }

    /// <summary>권한 거부가 장치를 앞서며 Native Kill 진단은 반환 후 처음 저장한 Form을 실제 분배기에 전달한다.</summary>
    [Fact]
    public void NativeKillPreservesAuthorityAndReturningDiagnosticsBeforeDispatch()
    {
        var s = new Scene(); s.Bind(); var process = new SquidProcess(); process.SetIdentity(0, s.Parent, 0);
        s.State.Boss = false; s.Host.Kill(process, 0); Assert.Empty(s.Assertions);
        s.State.Boss = true; int dispatched = -1; s.Pool.RawSlot(0)[11] = 2;
        s.Assets.Emit = effect => { Assert.Equal(SquidDestroyEffect.DestroyDependent, effect.Effect); dispatched = effect.Sid; };
        s.State.Assertion = (e, f, l) => { s.Assertions.Add((e, f, l)); process.SetIdentity(5, s.Parent, 0); };
        s.Host.Kill(process, 0x10); Assert.Equal(0, dispatched);
        Assert.Equal(new[] { ("s.isValid()", @"\Ns\O\Baseprocess.cpp", 0x98u), ("!s.isDead()", @"\Ns\O\Baseprocess.cpp", 0x99u) }, s.Assertions);
    }

    /// <summary>현재 선택 전체 DWORD를 훅 생성 후에도 재조회하고 누락을 0으로 대신하지 않는다.</summary>
    [Fact]
    public void NativeWholeSelectionUsesCurrentDeviceAndPinsActualRuntime()
    {
        var s = new Scene(); s.Bind(); SquidDestroyHooks hooks = s.Host.Hooks(); Assert.Equal(0u, hooks.SelectedRaw!());
        int pins = 0; object owner = new(); s.Current = new()
        {
            Pin = () => { pins++; return owner; },
            Asset = new() { SelectedRaw = () => { Assert.Equal(1ul, s.Current!.ActiveCalls); return 0xffff0005; } },
        };
        Assert.Equal(0xffff0005u, hooks.SelectedRaw()); Assert.Equal(1, pins); Assert.Equal(0ul, s.Current.ActiveCalls);
        s.Current.Asset.SelectedRaw = null; Assert.Throws<InvalidOperationException>(() => hooks.SelectedRaw()); Assert.Equal(0ul, s.Current.ActiveCalls);
        s.Current = null; Assert.Equal(0u, hooks.SelectedRaw());
    }

    /// <summary>기본 직렬화는 값 복사하고 빈 현재 장치만 기본으로 돌아가며 부분 현재 장치는 그대로 선택한다.</summary>
    [Fact]
    public void DefaultTransmissionCopiesAndSelectsWholeCurrentDevice()
    {
        var s = new Scene(); int writes = 0;
        var input = new ProcessFormTransmission { WriteData = (_, _) => ++writes, WriteFullData = (_, _) => 3, Packet = _ => [1, 2, 3], Transmit = _ => { } };
        s.Host.BindDefaultFormTransmission(input, (e, f, l) => s.Assertions.Add((e, f, l)));
        input.WriteData = (_, _) => 99; Assert.Equal(1, s.Host.Transmission()!.WriteData!(new SquidProcess(), false));
        s.Current = new() { Transmit = new() }; Assert.Equal(2, s.Host.Transmission()!.WriteData!(new SquidProcess(), false));
        s.Current.Transmit.Packet = _ => [9]; Assert.Null(s.Host.Transmission()!.WriteData); Assert.Equal(new byte[] { 9 }, s.Host.Transmission()!.Packet!(1));
        Assert.Throws<ArgumentException>(() => s.Host.BindDefaultFormTransmission(input, (_, _, _) => { }));
        var empty = new Scene(); Assert.Throws<ArgumentException>(() => empty.Host.BindDefaultFormTransmission(new(), (_, _, _) => { }));
        Assert.Null(empty.Host.Transmission());
    }

    /// <summary>파생 writer가 현재 장치를 교체해도 같은 writer 호출이 고른 packet 함수 사본을 유지한다.</summary>
    [Fact]
    public void FullWriterKeepsSelectedPacketDeviceWhenCallbackRebindsIt()
    {
        var s = new Scene(); int form = new RawSquidForms(s.Pool).Create(2, SidAllocation.Client);
        var process = new SquidProcess(); uint pid = s.Kernel.Add(process);
        BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(form)[18..], pid);
        BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(form)[16..], (ushort)s.Parent);
        var device = new ProcessFormTransmission { Packet = _ => [1, 2] };
        device.WriteFullData = (selected, argument) => { Assert.Same(process, selected); Assert.Equal(7u, argument); device.Packet = _ => [9, 9]; return 2; };
        s.Current = new() { Transmit = device }; byte[] packet = [0x7a, 0x7a, 0x7a];
        Assert.Equal(2, s.Host.WriteFormPacket(form, 7, packet, new RawSquidTransmissionState()));
        Assert.Equal(new byte[] { 1, 2, 0x7a }, packet); Assert.Equal(new byte[] { 9, 9 }, device.Packet!(2));
    }

    /// <summary>현재 World가 없으면 공유 예약의 같은 장기 깊이를 먼저 읽고 복귀 진단 뒤 실제 타입/시각으로 부착한다.</summary>
    [Fact]
    public void SharedRegularUsesDefaultDepthAndReturningAssertion()
    {
        var s = new Scene(); s.Bind(); s.Depth = 4; s.State.Now = 12.5;
        s.State.Assertion = (e, f, l) => { Assert.Equal("basePostPop == 0", e); Assert.Equal(@"\Ns\O\Regular.cpp", f); Assert.Equal(0x70u, l); s.Depth = 0; };
        RegularProcess process = Assert.IsType<RegularProcess>(s.Host.AddSharedRegular(s.Parent, 9, 2));
        Assert.Equal(12.5, process.Time()); Assert.Equal(61, s.Pool.Slot(process.Form())[10]); Assert.True(process.Shared());
        s.Host.DrainRuntimeBindings(null); Assert.Equal(0, s.Kernel.Size());
    }

    /// <summary>실제 외곽 owner는 호출 동안만 보유하고 참조 반납을 소유자 Dispose로 바꾸지 않는다.</summary>
    private sealed class Owner : IDisposable
    {
        /// <summary>호출한 명시 Dispose 횟수다.</summary>
        public int Disposals { get; private set; }
        /// <summary>명시적인 소유자 종료만 센다.</summary>
        public void Dispose() => Disposals++;
    }

    /// <summary>원문 pin은 activeCalls 증가보다 먼저 실행되고 예외이면 호출 개수를 바꾸지 않는다.</summary>
    [Fact]
    public void RuntimePinPreservesOrderExceptionAndOwnerLifetime()
    {
        var runtime = new ProcessHostRuntimeBindings(); var owner = new Owner(); int pins = 0;
        runtime.Pin = () => { Assert.Equal(0ul, runtime.ActiveCalls); pins++; return owner; };
        var guard = new RuntimeCallGuard(runtime); Assert.Equal(1ul, runtime.ActiveCalls); Assert.Equal(1, pins); Assert.Same(runtime.Pin, runtime.Clone().Pin);
        runtime.Pin = null; Assert.Equal(0, owner.Disposals);
        guard.Dispose(); guard.Dispose(); Assert.Equal(0ul, runtime.ActiveCalls); Assert.Equal(0, owner.Disposals);
        runtime.Pin = () => throw new InvalidOperationException("실제 pin 실패"); Assert.Throws<InvalidOperationException>(() => new RuntimeCallGuard(runtime)); Assert.Equal(0ul, runtime.ActiveCalls);
    }

    /// <summary>테스트 스택/콜백이 원본 owner를 계속 쥐지 않게 독립 호출에서 실제 pin만 남긴다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (RuntimeCallGuard Guard, WeakReference<Owner> Owner) PinnedOwner()
    {
        var owner = new Owner(); var runtime = new ProcessHostRuntimeBindings { Pin = () => owner };
        var weak = new WeakReference<Owner>(owner); var guard = new RuntimeCallGuard(runtime); runtime.Pin = null;
        return (guard, weak);
    }

    /// <summary>콜백과 외부 참조를 놓은 뒤에도 실제 owner가 선택 호출 동안 수거되지 않는다.</summary>
    [Fact]
    public void RuntimeGuardHoldsActualOuterOwnerAfterCallbackIsDetached()
    {
        var value = PinnedOwner(); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
        Assert.True(value.Owner.TryGetTarget(out Owner? owner)); Assert.Equal(0, owner!.Disposals);
        value.Guard.Dispose(); GC.KeepAlive(owner);
    }


    /// <summary>잔류 프로세스가 이전 대여를 소유하지 않는지 실제 약한 참조로 관측한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (SquidProcess Process, WeakReference<ProcessHostRuntimeBindings> Runtime) ReleasedRuntime()
    {
        var runtime = new ProcessHostRuntimeBindings(); var process = new SquidProcess { RuntimeLink = runtime };
        return (process, new(runtime));
    }

    /// <summary>호출 밖의 잔류는 C++ weak_ptr처럼 이전 월드 대여의 수명을 늘리지 않는다.</summary>
    [Fact]
    public void ResidualProcessDoesNotOwnPreviousWorldRuntime()
    {
        var value = ReleasedRuntime(); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
        Assert.False(value.Runtime.TryGetTarget(out _)); Assert.Null(value.Process.RuntimeLink); GC.KeepAlive(value.Process);
    }

}
