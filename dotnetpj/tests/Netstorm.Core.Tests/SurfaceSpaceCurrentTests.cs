using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.CompilerServices;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 소유자128행과 같은 장기 공간 실행기의 실제 조립/재사용을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>파생 소유자 접두와 장기 PostPop/Pop·프레임·연결/표면을 게임 창 없이 확인한다.</summary>
public sealed class SurfaceSpaceCurrentTests
{
    /// <summary>실제 생성자와 두 물리 프레임을 갖는 독립 장기 자산이다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index), MaxHitPoints = 1001, FootX = 1, FootY = 1 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([new((byte)'A', (byte)'0', 0, 0), new((byte)'B', (byte)'0', 0, 0)])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ =>
        {
            var shape = new SquidDisplayShape { Loaded = true, FrameCount = 2 };
            // 두 프레임 모두 원문 표시/공간 크기를 실제 헤더에서 읽는다.
            for (int index = 0; index < 2; index++) shape.Frames.Add(new() { Width = 16, Height = 11, CellWidth = 1, CellHeight = 1 });
            return shape;
        }).ToArray();
        return new(OriginalEdition.Patch1078, types, frames, shapes);
    }
    /// <summary>기본 표시를 명시 공급해 getter가 World를 합성하지 않도록 한다.</summary>
    private static void Devices(RawSurfaceStorage storage) => storage.BindDisplayDevices(new()
    { View = () => new() { Zoom = 65536, Viewport = new(0, 0, 1024, 768) }, Suppressed = () => false, Invalidate = (_, _) => { } });

    /// <summary>두 원본의 실제 생산 접두/고정 중립 body128행을 전체 원문 payload와 직접 대조한다.</summary>
    [Fact]
    public void AssetOwnerMatchesAllNative128Inputs()
    {
        string[][] rows = X86Fixture.Read("asset-owner-current-local"); int count = 0;
        // 원본 입력만 만들고 기대 생산 카운터/이벤트/바이트를 C# 결과로 계산하지 않는다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length); bool patch = row[0] == "originals"; string[] args = row[1].Split(','); Assert.Equal(5, args.Length);
            uint kind = X86Fixture.UInt(args[0]), extra = X86Fixture.UInt(args[1]), owner = X86Fixture.UInt(args[2]); var dirty = new UIntBox { Value = X86Fixture.UInt(args[3]) }; bool stop = X86Fixture.UInt(args[4]) != 0;
            int width = row[5].Length / 2; Assert.Equal(patch ? 50 : 36, width); var pool = new RawSidPool(32768, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            // CD의 논리36BYTE만 비교하며 PATCH 전용 이웃 바이트를 입력/출력으로 해석하지 않는다.
            for (int index = 0; index < width; index++) pool.RawSlot(5)[index] = unchecked((byte)(index * 29 + 7));
            pool.RawSlot(5)[patch ? 34 : 32] = 3; pool.RawSlot(5)[patch ? 40 : 35] = unchecked((byte)extra);
            uint originalTable = BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(5)); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5), kind != 0 ? (patch ? 0x50a8e8u : 0x500b10u) : (patch ? 0x509148u : 0x503298u));
            string events = "-", cause = "returned";
            Action<int, uint> dispatch = RawAssetOwner.MakeAssetOwnerDispatch(pool, new(), dirty, (sid, player) =>
            {
                Assert.Equal(5, sid); events = "B" + player.ToString(CultureInfo.InvariantCulture) + ',' + dirty.Value.ToString(CultureInfo.InvariantCulture);
                if (stop) throw new ApplicationException("base"); pool.RawSlot(sid)[patch ? 34 : 32] = unchecked((byte)player);
            });
            try { dispatch(5, owner); } catch (ApplicationException error) { cause = error.Message; }
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5), originalTable);
            Assert.Equal(row[2], cause); Assert.Equal(row[3], events); Assert.Equal(row[4], dirty.Value.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[5], Convert.ToHexStringLower(pool.Slot(5)[..width])); count++;
        }
        Assert.Equal(128, count);
    }
    /// <summary>동일 분배기가 현재 VT/extra를 읽고 부모 중단 뒤에도 앞선 생산 증가를 유지한다.</summary>
    [Fact]
    public void AssetOwnerKeepsLiveSelectionAndPrefixOnParentFailure()
    {
        var storage = Storage(); int sid = storage.Pool.Allocate(); var dirty = storage.PostPopState.ProductionDirtyBox; dirty.Value = 7; int calls = 0;
        Action<int, uint> dispatch = RawAssetOwner.MakeAssetOwnerDispatch(storage.Pool, storage.Links, dirty, (_, _) => { calls++; throw new ApplicationException("base"); });
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(sid), 0x509148); storage.Pool.RawSlot(sid)[40] = 0;
        Assert.Throws<ApplicationException>(() => dispatch(sid, 2)); Assert.Equal(8u, storage.PostPopState.ProductionDirty); Assert.Equal(1, calls);
        storage.Pool.RawSlot(sid)[40] = 8; Assert.Throws<ApplicationException>(() => dispatch(sid, 2)); Assert.Equal(8u, dirty.Value); Assert.Equal(2, calls);
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(sid), 0x50a8e8); storage.Pool.RawSlot(sid)[34] = 3; dispatch(sid, uint.MaxValue); Assert.Equal(0, storage.Pool.Slot(sid)[34]); Assert.Equal(2, calls);
        ref uint address = ref storage.PostPopState.ProductionDirtyValue; address = uint.MaxValue; dirty.Value = unchecked(dirty.Value + 1); Assert.Equal(0u, address);
    }
    /// <summary>보호 생성자의 두 공장/젬/받침/종유석을 실제 World의 동일 분배기에 연결한다.</summary>
    [Fact]
    public void ActualWorldConstructorsUseDerivedOwnerAndSharedProductionStorage()
    {
        var storage = Storage(); using var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage); world.ConfigureInstallModes(false, false, false, false);
        storage.Links.Battle = true; storage.Links.Mission = false; storage.Links.OwnerColors[2] = 6;
        // 원문 다섯 보호 생성자의 실제 VT에 따라 분배하고 공중 유닛/통신/포획은 실행하지 않는다.
        foreach (uint type in new uint[] { 85, 126, 121, 94, 95 })
        {
            int sid = world.Core().Factory.Create(type); uint table = BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(sid)); storage.PostPopState.ProductionDirty = uint.MaxValue;
            world.SetAssetOwner(sid, 2); Assert.Equal(type == 121 ? 0 : 2, storage.Pool.Slot(sid)[34]); Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(sid)));
            Assert.Equal(type is 85 or 126 ? 0u : uint.MaxValue, storage.PostPopState.ProductionDirty);
            if (type is 94 or 95) Assert.Equal(5u, BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(sid).Slice(36)));
        }
    }
    /// <summary>모든 getter는 같은 실행기를 재사용하고 물리/게임 값이나 대여를 초기화하지 않는다.</summary>
    [Fact]
    public void LongSpaceGettersReuseActualPhysicalAndStateWithoutWorld()
    {
        var storage = Storage(); Devices(storage); storage.Graph.Records[252].Reserved = 0x1234; storage.Spots[15] = 0x88; storage.PostPopState.ProductionDirty = 77;
        Assert.Same(storage.PostPopExecutor(), storage.PostPopExecutor()); Assert.Same(storage.PopExecutor(), storage.PopExecutor()); Assert.Same(storage.FrameExecutor(), storage.FrameExecutor()); Assert.Same(storage.ConnectionsExecutor(), storage.ConnectionsExecutor()); Assert.Same(storage.SurfaceExecutor(), storage.SurfaceExecutor());
        Assert.Equal(0x1234, storage.Graph.Records[252].Reserved); Assert.Equal(0x88, storage.Spots[15]); Assert.Equal(77u, storage.PostPopState.ProductionDirty); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }
    /// <summary>World 간 준비/일반 재대여도 같은 다섯 공간 실행기를 빌리고 실제 Pop/프레임 변경을 처리한다.</summary>
    [Fact]
    public void WorldsReuseSpaceExecutorsAndCurrentFrameChange()
    {
        var storage = Storage(); RawPostPop post; RawPop pop; SquidFrame frame; RawBridgeConnect connect; RawIslandPostPop surface;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            var core = first.Core(); post = core.PostPop; pop = core.Pop; frame = core.Frame; connect = first.BridgeConnections(); surface = storage.SurfaceExecutor();
            Assert.Same(post, storage.PostPopExecutor()); Assert.Same(pop, storage.PopExecutor()); Assert.Same(frame, storage.FrameExecutor()); Assert.Same(connect, storage.ConnectionsExecutor());
            int sid = core.Factory.Create(74); first.Pop(sid, 20, 30, 0x18); frame.Set(sid, 1, 0); Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(sid).Slice(36))); Assert.Equal(0u, post.ActivationDepth);
        }
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        { var core = next.Core(); Assert.Same(post, core.PostPop); Assert.Same(pop, core.Pop); Assert.Same(frame, core.Frame); Assert.Same(connect, next.BridgeConnections()); Assert.Same(surface, storage.SurfaceExecutor()); }
        using (var fresh = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage)) Assert.Same(post, fresh.Core().PostPop);
        Assert.False(storage.Borrowed());
    }
    /// <summary>표면 진단 문맥은 일반 새 World 준비로 지우지 않는 같은 현재 DWORD다.</summary>
    [Fact]
    public void CurrentSurfaceDiagnosticContextSurvivesGeneralPreparation()
    {
        var storage = Storage(); storage.GameAux().SupportTypeWas = 0xfedcba98; using (storage.Acquire()) Assert.Equal(0xfedcba98u, storage.GameAux().SupportTypeWas);
        Assert.Equal(0xfedcba98u, storage.GameAux().SupportTypeWas);
    }
    /// <summary>장기 접두·프레임·표면 콜백이 종료된 World를 보유하지 않는다.</summary>
    [Fact]
    public void LongSpaceExecutorsDoNotKeepDisposedWorldAlive()
    {
        var storage = Storage(); WeakReference weak = DisposeWorld(storage); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.False(weak.IsAlive); GC.KeepAlive(storage);
    }
    /// <summary>종료 World의 지역 참조와 실제 저장소 소유를 다른 프레임에 둔다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DisposeWorld(RawSurfaceStorage storage)
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage); var weak = new WeakReference(world); world.Dispose(); return weak;
    }
}
