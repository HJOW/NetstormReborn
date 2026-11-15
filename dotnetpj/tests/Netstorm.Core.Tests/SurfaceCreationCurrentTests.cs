using System.Runtime.CompilerServices;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 원문 건물 옆면 진단과 장기 공장/해제 부모의 실제 재사용·현재 종단을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>실제 표시/슬롯 모듈을 사용하되 게임 창을 열지 않는 현재 조립 검사다.</summary>
public sealed class SurfaceCreationCurrentTests
{
    /// <summary>실제 188타입 생성자 표와 독립 셰이프를 같은 저장소에 입력한다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord
        { ConstructorAddress = OriginalConstructors.Address(index), MaxHitPoints = 1001, FootX = 1, FootY = 1 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        return new(OriginalEdition.Patch1078, types, frames, shapes);
    }
    /// <summary>장기 Client 뷰와 실제 관찰만 공급하고 World를 만들지 않는다.</summary>
    private static void DisplayDevices(RawSurfaceStorage storage) => storage.BindDisplayDevices(new()
    { View = () => new() { Zoom = 65536, Viewport = new(0, 0, 1024, 768) }, Suppressed = () => false, Invalidate = (_, _) => { } });
    /// <summary>직접 표면 단어를 입력해 현재 다리/진단 소비 위치만 선택한다.</summary>
    private static int Bridge(RawSurfaceStorage storage)
    {
        int sid = storage.Pool.Allocate(); Span<byte> raw = storage.Pool.AllocatedBytes(sid); raw[10] = 82; raw[12] = 0x7e; raw[13] = 0xa5; raw[40] = 0; return sid;
    }

    /// <summary>현재 섬 TYPE는 지면 조회 성공 뒤만 읽으며 값 폭을 BYTE로 자르지 않는다.</summary>
    [Fact]
    public void BuildingReadsCurrentIslandOnlyAtSelectedComparison()
    {
        var storage = Storage(); storage.BuildingSpaceState.IslandType = 97; var building = storage.BuildingExecutor(); int sid = Bridge(storage); storage.Pool.RawSlot(sid)[10] = 97;
        storage.Hash.Bucket(0, 0, 2) = (ushort)sid; uint current = 97; int reads = 0;
        building.BindCurrentIslandType(() => { reads++; return current; }); Assert.Equal(0, reads);
        Assert.True(building.HasIsland(-0.25f, 2.5f)); Assert.Equal(1, reads); current = 353; Assert.False(building.HasIsland(0, 2));
        Assert.False(building.HasIsland(-1, 2)); Assert.Equal(2, reads);
        Assert.Throws<ArgumentOutOfRangeException>(() => building.HasIsland(float.NaN, 2)); Assert.Equal(2, reads);
    }
    /// <summary>nibble을 checking보다 먼저 고정하고 전체 DWORD 비영 진단의 정상 복귀를 유지한다.</summary>
    [Fact]
    public void BuildingDiagnosticReturnsWithOriginalFieldsAndOrder()
    {
        var storage = Storage(); int sid = Bridge(storage); var building = storage.BuildingExecutor(); int calls = 0;
        building.BindNotifyDiagnostics(new(() => { storage.Pool.RawSlot(sid)[12] = 0; return 0x80000000u; }), (expression, file, line) =>
        {
            Assert.Equal("num >= 0 && num <= 7", expression); Assert.Equal("\\Ns\\O\\Bridge.cpp", file); Assert.Equal(0xffu, line); calls++;
            building.BindNotifyDiagnostics(new(true), (_, _, _) => throw new ApplicationException("다음 호출의 장치"));
        });
        building.NotifySurface(sid); Assert.Equal(1, calls); Assert.Equal(0xa5, storage.Pool.Slot(sid)[13]);
        storage.Pool.RawSlot(sid)[12] = 0x40; Assert.Throws<ApplicationException>(() => building.NotifySurface(sid));
    }
    /// <summary>현재 다리 TYPE/매립 관문은 checking 조회보다 앞서며 진단 함수 누락은 선택 위치에서만 실패한다.</summary>
    [Fact]
    public void BuildingUsesCurrentBridgeAndDoesNotPreflightDiagnostic()
    {
        var storage = Storage(); int sid = Bridge(storage); var building = storage.BuildingExecutor(); int reads = 0;
        building.BindNotifyDiagnostics(new(() => { reads++; return uint.MaxValue; }), null);
        storage.Links.BridgeType = 81; building.NotifySurface(sid); Assert.Equal(0, reads);
        storage.Links.BridgeType = 82; storage.Pool.RawSlot(sid)[40] = 8; building.NotifySurface(sid); Assert.Equal(0, reads);
        storage.Pool.RawSlot(sid)[40] = 0; Assert.Throws<InvalidOperationException>(() => building.NotifySurface(sid)); Assert.Equal(1, reads);
        building.BindChecking(() => 0); building.NotifySurface(sid); Assert.Equal(1, reads);
    }
    /// <summary>CD의 NotifySurface는 버려지는 포인터 계산뿐이므로 raw SID/TYPE/checking을 읽지 않는다.</summary>
    [Fact]
    public void CdNotifyHasNoPatchReadsAndAcceptsItsActualTypeCount()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072); var hash = new RawSquidHash(); var types = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray();
        var building = new RawBuildingSpace(pool, hash, types, new() { IslandType = 82, BridgeType = 81, Debug = true });
        building.BindCurrentBridgeType(() => throw new Exception("CD TYPE 조회 금지")); building.BindNotifyDiagnostics(new RawCheckingFlag((Func<uint>)(() => throw new Exception("CD checking 조회 금지"))), null);
        building.NotifySurface(int.MaxValue); Assert.Throws<ArgumentException>(() => new RawBuildingSpace(pool, hash, Storage().Types, new()));
    }
    /// <summary>저장소의 공장 getter는 공간/표시/World를 만들지 않으며 현재 HP owner와 깊이 주소를 빌린다.</summary>
    [Fact]
    public void FactoryIsLazyAndUsesCurrentSharedHitPoints()
    {
        var storage = Storage(); var factory = storage.FactoryExecutor(); Assert.Same(factory, storage.FactoryExecutor()); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
        storage.Types[158].Flags1 = OriginalTypeRecord.HasHitPoints;
        storage.GameAux().PriestQuarterHpBox.Value = true; int sid = factory.Create(158); Assert.Equal(250, BitConverter.ToInt32(storage.Pool.Slot(sid).Slice(26, 4)));
        storage.GameAux().PriestQuarterHpBox.Value = false; sid = factory.Create(158); Assert.Equal(1001, BitConverter.ToInt32(storage.Pool.Slot(sid).Slice(26, 4)));
        Assert.Equal(0ul, storage.Generation()); Assert.Throws<InvalidOperationException>(() => storage.UnpopExecutor()); Assert.False(storage.Borrowed());
    }
    /// <summary>같은 장기 공간/공장 실행기를 두 World와 일반 fresh가 빌리며 현재 checking 교체도 즉시 보인다.</summary>
    [Fact]
    public void WorldsReuseFactoryBuildingAndUnpopWithoutOldCallbacks()
    {
        var storage = Storage(); RawSquidFactory factory; RawBuildingSpace building; RawUnpop unpop;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            var core = first.Core(); factory = core.Factory; building = core.Building; unpop = core.Unpop;
            Assert.Same(factory, storage.FactoryExecutor()); Assert.Same(building, storage.BuildingExecutor()); Assert.Same(unpop, storage.UnpopExecutor());
        }
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        { var core = next.Core(); Assert.Same(factory, core.Factory); Assert.Same(building, core.Building); Assert.Same(unpop, core.Unpop); }
        using (var fresh = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            Assert.Same(factory, fresh.Core().Factory); int sid = Bridge(storage); int assertions = 0; var detail = new StringBox();
            storage.BindDisplayDiagnostics(detail, (_, _, _) => assertions++); storage.BindChecking(new(uint.MaxValue)); building.NotifySurface(sid); Assert.Equal(1, assertions);
        }
        Assert.False(storage.Borrowed()); Assert.Equal(3ul, storage.Generation());
    }
    /// <summary>기존 실제 공장 대여 거부 뒤 다른 대여/표시는 유지하고 새 World lease만 반납한다.</summary>
    [Fact]
    public void RejectedFactoryBindingReturnsWorldLeaseWithoutReleasingOtherRuntime()
    {
        var storage = Storage(); DisplayDevices(storage); var factory = storage.FactoryExecutor(); int detaches = 0;
        var runtime = new SquidFactoryRuntimeBindings { Unpop = storage.UnpopExecutor(), FormDetach = _ => detaches++, ContentType = () => 6, FormTransmit = (_, _) => { } };
        factory.BindRuntime(runtime); Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true));
        Assert.False(storage.Borrowed()); factory.BindRuntime(runtime); factory.ReleaseRuntime(runtime); Assert.Equal(0, detaches); Assert.Equal(65536, storage.CurrentDisplayView().Zoom);
        using var lease = storage.AcquirePrepared(); Assert.True(storage.Borrowed());
    }
    /// <summary>같은 풀만 현재 종단으로 빌리고 독립 기본 내용물 번호는 현재 DWORD를 읽는다.</summary>
    [Fact]
    public void ContainedTerminalKeepsActualIdentityAndCurrentGlobals()
    {
        var storage = Storage(); var terminal = storage.ContainedUnpopExecutor(); Assert.Same(terminal, storage.ContainedUnpopExecutor());
        Assert.Equal(6u, terminal.ContentFormType()); storage.ContainedType.Value = 0xfedcba98; Assert.Equal(0xfedcba98u, terminal.ContentFormType());
        Assert.Throws<ArgumentException>(() => storage.BindContainedUnpop(Storage().ContainedUnpopExecutor())); storage.BindContainedUnpop(terminal); storage.BindContainedUnpop(null);
        Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }
    /// <summary>같은 실제 Form 체인을 현재 가상 링크 위치에 입력한다.</summary>
    private static void FormChain(RawSurfaceStorage storage, int parent, int form)
    {
        Span<byte> head = storage.Pool.RawSlot(parent); head.Clear(); head[10] = 1; head[6] = (byte)form; head[7] = (byte)(form >> 8);
        Span<byte> raw = storage.Pool.RawSlot(form); raw.Clear(); BitConverter.TryWriteBytes(raw, 0x512098u); raw[10] = 2; raw[11] = 8; raw[16] = (byte)parent; raw[17] = (byte)(parent >> 8);
    }
    /// <summary>공장 Form 해제가 실제 현재 종단을 고르고 반납 후에는 같은 장기 공통 종단으로 돌아간다.</summary>
    [Fact]
    public void FactoryDetachSelectsCurrentTerminalThenActualDefault()
    {
        var storage = Storage(); var factory = storage.FactoryExecutor(); int parent = storage.Pool.Allocate(); int form = storage.Pool.Allocate(); int selected = 0;
        var terminal = new RawSquidContainedUnpop(storage.Pool, new() { ContentFormType = storage.ContainedType, GumpFormType = storage.ContainedGumpType, UiEvent = storage.ContainedUiEvent },
            new() { Parent = _ => { selected++; return parent; }, Previous = _ => 0, SetPrevious = (_, _) => { } });
        FormChain(storage, parent, form); storage.BindContainedUnpop(terminal); factory.UnpopForm(form); Assert.Equal(1, selected); Assert.Equal(4, storage.Pool.Slot(form)[11]); Assert.Equal(0, storage.Pool.Slot(parent)[6]);
        FormChain(storage, parent, form); storage.BindContainedUnpop(null); factory.UnpopForm(form); Assert.Equal(1, selected); Assert.Equal(4, storage.Pool.Slot(form)[11]); Assert.Equal(0, storage.Pool.Slot(parent)[6]);
    }
    /// <summary>실제 UI Form이 선택됐을 때만 현재 명령 함수와 전체 명령 DWORD를 소비한다.</summary>
    [Fact]
    public void ContainedUiUsesActualCommandOnlyAtSelectedGump()
    {
        var storage = Storage(); int parent = storage.Pool.Allocate(); int form = storage.Pool.Allocate(); FormChain(storage, parent, form);
        var terminal = storage.ContainedUnpopExecutor(); terminal.NotifyUi(parent); storage.Pool.RawSlot(form)[10] = 3;
        Assert.Throws<InvalidOperationException>(() => terminal.NotifyUi(parent)); var commands = new List<(uint, uint, uint)>(); storage.BindContentCommand((code, x, y) => commands.Add((code, x, y)));
        storage.ContainedUiEvent.Value = 0xabcdef01; terminal.NotifyUi(parent); Assert.Equal(new[] { (0xabcdef01u, (uint)parent, 0u) }, commands);
    }
    /// <summary>장기 공장/해제/표시는 명시 종료된 World를 소유하지 않는다.</summary>
    [Fact]
    public void LongCreationExecutorsDoNotKeepDisposedWorldAlive()
    {
        var storage = Storage(); WeakReference weak = DisposeWorld(storage); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.False(weak.IsAlive); GC.KeepAlive(storage);
    }
    /// <summary>JIT 지역 참조와 실제 장기 소유를 분리한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DisposeWorld(RawSurfaceStorage storage)
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage); var weak = new WeakReference(world); world.Dispose(); return weak;
    }
}
