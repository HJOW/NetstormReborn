using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 Kernel 인계 전후의 소유권을 C++ unique_ptr 경계와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>SquidProcess.cpp167~196의 등록/부분 효과/가상 소멸 계약을 검사한다.</summary>
public sealed class RawProcessAttachOwnershipTests
{
    /// <summary>실제 Host가 부착하고 Kernel이 소유하는 검사 프로세스다.</summary>
    private sealed class OwnedProcess : SquidProcess
    {
        /// <summary>가상 소멸 횟수이며 이중 호출을 드러낸다.</summary>
        public int Disposes { get; private set; }
        /// <summary>실제 마지막 부착 통지 횟수다.</summary>
        public int AttachedCalls { get; private set; }
        /// <summary>현재 부착 통지에서 예외를 요청한다.</summary>
        public bool FailAttached { get; set; }
        /// <summary>검사에서 같은 기반 클래스의 현재 PID를 읽는다.</summary>
        public uint CurrentPid => Pid();
        /// <summary>부착의 마지막 경계에서만 선택한 실패를 만든다.</summary>
        public override void OnAttached() { AttachedCalls++; if (FailAttached) throw new InvalidOperationException("attached-failure"); }
        /// <summary>실제 Kernel 반납이 부르는 소멸 횟수를 기록한다.</summary>
        public override void Dispose() { Disposes++; base.Dispose(); }
    }
    /// <summary>복원된 실제 ProcessForm2 생성자/풀/Host를 만들고 살아 있는 부모를 확보한다.</summary>
    private static (RawSidPool Pool, Kernel Kernel, SquidProcessState State, SquidProcessHost Host, int Parent) Scene(uint capacity = 4, bool serverParent = false)
    {
        var pool = new RawSidPool(32768); var kernel = new Kernel(capacity); var state = new SquidProcessState { Assertion = (_, _, _) => throw new InvalidOperationException("host-assertion") };
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToList(); types[2].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, 2);
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); var host = new SquidProcessHost(pool, types, kernel, destroy, state);
        int parent = pool.Allocate(serverParent ? SidAllocation.Server : SidAllocation.Client); pool.RawSlot(parent).Clear(); pool.RawSlot(parent)[10] = 70;
        return (pool, kernel, state, host, parent);
    }
    /// <summary>미지원 실제 Form 생성자는 앞선 Kernel 인계/할당을 남기고 객체를 소멸시키지 않는다.</summary>
    [Fact]
    public void ConstructorFailureKeepsKernelOwnerAndEarlierAllocation()
    {
        var (pool, kernel, state, host, parent) = Scene(); host.Type(2).ConstructorAddress = 0xfffffff0; uint free = pool.FreeCount; var process = new OwnedProcess();
        Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50));
        Assert.Equal(0, process.Disposes); Assert.Same(process, kernel.Get(process.CurrentPid)); Assert.Equal(1, kernel.Size()); Assert.Equal(free - 1, pool.FreeCount); Assert.Equal(1u, state.TypeRotation);
        Assert.Equal(parent, process.Parent()); Assert.Equal(0, process.Form()); kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes);
    }
    /// <summary>실제 Form과 부모 체인 부착 뒤 통지 예외도 Kernel의 같은 객체를 보존한다.</summary>
    [Fact]
    public void OnAttachedFailureKeepsActualFormParentAndKernelObject()
    {
        var (pool, kernel, _, host, parent) = Scene(); var process = new OwnedProcess { FailAttached = true };
        Assert.Equal("attached-failure", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50)).Message);
        Assert.Equal(0, process.Disposes); Assert.Equal(1, process.AttachedCalls); Assert.Same(process, host.FromForm(process.Form())); Assert.Same(process, kernel.Get(process.CurrentPid));
        Assert.Equal(parent, RawContainedLink.CommonContainedParent(pool, process.Form())); Assert.Equal(process.Form(), BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent)[6..]));
        kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes); Assert.Equal(0, kernel.Size());
    }
    /// <summary>Kernel.Add가 실패한 소유 인자를 이미 소멸했으면 Host에서 다시 소멸시키지 않는다.</summary>
    [Fact]
    public void KernelDiagnosticFailureDisposesOwnedArgumentExactlyOnce()
    {
        var (pool, kernel, _, host, parent) = Scene(1); kernel.Add(new BaseProcess()); uint free = pool.FreeCount;
        kernel.BindDiagnostics(new() { Summary = () => throw new InvalidOperationException("kernel-summary") }); var process = new OwnedProcess();
        Assert.Equal("kernel-summary", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50)).Message);
        Assert.Equal(1, process.Disposes); Assert.Equal(1, kernel.Size()); Assert.Equal(free, pool.FreeCount); Assert.Equal(0, process.AttachedCalls); kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes);
    }
    /// <summary>등록 전 비활성 부모의 자연 반환은 여전히 local 소유 객체를 한 번 소멸시킨다.</summary>
    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    public void InactiveParentDisposesBeforeKernelTransfer(int flag)
    {
        var (pool, kernel, state, host, parent) = Scene(); pool.RawSlot(parent)[11] = (byte)flag; uint free = pool.FreeCount; var process = new OwnedProcess();
        Assert.Null(host.Attach(process, 46, parent, 0x50)); Assert.Equal(1, process.Disposes); Assert.Equal(0, kernel.Size()); Assert.Equal(free, pool.FreeCount); Assert.Equal(0u, state.TypeRotation);
    }
    /// <summary>타입 진단이 등록 전에 던지면 원문 부모 필드도 쓰지 않고 local 소유만 끝낸다.</summary>
    [Fact]
    public void PreRegistrationAssertionDisposesBeforeAnyKernelOrParentWrite()
    {
        var (_, kernel, state, host, parent) = Scene(); var process = new OwnedProcess();
        Assert.Equal("host-assertion", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 5, parent, 0x50)).Message);
        Assert.Equal(1, process.Disposes); Assert.Equal(0, kernel.Size()); Assert.Equal(0u, state.TypeRotation); Assert.Equal(0, process.Parent());
    }
    /// <summary>같은 기존 Kernel 객체의 재부착 실패는 신규 소유권을 만들거나 소멸시키지 않는다.</summary>
    [Fact]
    public void ReattachFailurePreservesExistingKernelOwnership()
    {
        var (_, kernel, _, host, parent) = Scene(); var process = new OwnedProcess(); Assert.Same(process, host.Attach(process, 46, parent, 0x50)); uint pid = process.CurrentPid; int form = process.Form(); process.FailAttached = true;
        Assert.Equal("attached-failure", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50, form)).Message);
        Assert.Equal(0, process.Disposes); Assert.Equal(2, process.AttachedCalls); Assert.Equal(1, kernel.Size()); Assert.Equal(pid, process.CurrentPid); Assert.Same(process, kernel.Get(pid)); kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes);
    }
    /// <summary>Find가 제외하는 실제slot0도 Add에 인계된 소유권이므로 예외에서 local 소멸하지 않는다.</summary>
    [Fact]
    public void SlotZeroRemainsKernelOwnedAfterCreateFailure()
    {
        var (pool, kernel, state, host, parent) = Scene(1); kernel.Add(new BaseProcess()); var events = new List<string>();
        kernel.BindDiagnostics(new() { Summary = () => events.Add("summary"), Detail = _ => events.Add("detail"), Assertion = (_, _, _) => events.Add("assert") }); host.Type(2).ConstructorAddress = 0xfffffff0;
        var process = new OwnedProcess(); uint free = pool.FreeCount; Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50));
        Assert.Equal(new[] { "summary", "detail", "assert" }, events); Assert.Equal(0, process.Disposes); Assert.Equal(0u, kernel.Find(process)); Assert.Same(process, kernel.Get(0)); Assert.Equal(2, kernel.Size()); Assert.Equal(free - 1, pool.FreeCount); Assert.Equal(1u, state.TypeRotation);
        kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes); Assert.Equal(0, kernel.Size());
    }
    /// <summary>실제 서버 ProcessForm writer 실패는 Form 필드/Kernel을 남기며 뒤 가상부착은 실행하지 않는다.</summary>
    [Fact]
    public void TransmissionFailurePreservesKernelAndUnattachedForm()
    {
        var (pool, kernel, _, host, parent) = Scene(serverParent: true); host.SetFormTransmit(new() { WriteData = (_, _) => throw new InvalidOperationException("writer-failure") }); var process = new OwnedProcess();
        Assert.Equal("writer-failure", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0)).Message);
        Assert.Equal(0, process.Disposes); Assert.Equal(0, process.AttachedCalls); Assert.Same(process, kernel.Get(process.CurrentPid)); Assert.True(process.Form() != 0); Assert.Equal(4, pool.Slot(process.Form())[11] & 4); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent)[6..]));
        kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes);
    }
    /// <summary>독립 공장 생성이 선택한 실제 원문 진단은 같은 Host 상태 장치에 전달한다.</summary>
    [Fact]
    public void IndependentFactoryUsesCurrentHostAssertionAfterKernelTransfer()
    {
        var (pool, kernel, state, host, parent) = Scene(); state.CurrentProcessFormType = new UIntBox { Value = 162 }; var calls = new List<uint>();
        state.Assertion = (text, file, line) => { Assert.Equal("type != fakeThreeByThreeSurfaceType", text); Assert.Equal(@"\Ns\O\Squid.cpp", file); calls.Add(line); throw new InvalidOperationException("factory-assertion"); };
        var process = new OwnedProcess(); uint free = pool.FreeCount;
        Assert.Equal("factory-assertion", Assert.Throws<InvalidOperationException>(() => host.Attach(process, 46, parent, 0x50)).Message);
        Assert.Equal(new uint[] { 0x189 }, calls); Assert.Equal(0, process.Disposes); Assert.Same(process, kernel.Get(process.CurrentPid)); Assert.Equal(free, pool.FreeCount); kernel.DrainOwnedForShutdown(); Assert.Equal(1, process.Disposes);
    }
}
