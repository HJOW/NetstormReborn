using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 공통 표시 owner와 실제 Host/Kernel 수명을 같은 객체로 합성한다.
namespace Netstorm.Core.Tests;

/// <summary>생성 중 공개·출력 중 변경·현재 배율·실제 Form 반납을 검증한다.</summary>
public sealed class RenderProcessSystemTests
{
    /// <summary>실제 원문 풀/타입/Host/Kernel과 공통 등록기를 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>동일한 raw 저장소다.</summary>
        public RawSidPool Pool { get; } = new(24000, false);
        /// <summary>실제 폼을 포함한188TYPE다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>동일한 실행/소유 커널이다.</summary>
        public Kernel Kernel { get; } = new(8);
        /// <summary>폼의 원문 부착/삭제 호스트다.</summary>
        public SquidProcessHost Host { get; }
        /// <summary>실제 공통 등록/수집 owner다.</summary>
        public RenderProcessSystem Rendering { get; }
        /// <summary>같은 배율 owner다.</summary>
        public ConstructionVisualState State { get; } = new();
        /// <summary>실제 부착될 부모다.</summary>
        public int Parent { get; }
        /// <summary>현재 소유 상태를 원문 부모에 연결한다.</summary>
        public Scene()
        {
            Types[80].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, 2);
            var destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Host = new(Pool, Types, Kernel, destroy, new SquidProcessState { CurrentProcessFormType = new() { Value = 80 } });
            Parent = Pool.Allocate(SidAllocation.Client); Pool.RawSlot(Parent).Clear(); Pool.RawSlot(Parent)[10] = 71;
            BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Parent)[14..], 3.5f); BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Parent)[18..], 4.5f);
            Rendering = new(Pool, Host, State, new() { Reserve = _ => true, ReleaseArray = () => { } });
        }
    }

    /// <summary>공통 표시 등록을 소유하는 실제 파생 프로세스다.</summary>
    private sealed class Probe : SquidProcess
    {
        /// <summary>같은 장기 공통 등록기다.</summary>
        public required RenderProcessSystem Rendering { get; init; }
        /// <summary>이 파생이 소유하는 원문 필드다.</summary>
        public RenderProcessFields Fields { get; } = new();
        /// <summary>실제 Kernel 실행 횟수다.</summary>
        public int Frames { get; private set; }
        /// <summary>단독 소유권 반납 횟수다.</summary>
        public int Disposals { get; private set; }
        /// <summary>실제 커널이 같은 파생을 실행했음을 관찰한다.</summary>
        public override void RunFrame() => Frames++;
        /// <summary>버퍼 수명이 끝날 때 등록/base를 정리하고 같은 토큰 조회를 없앤다.</summary>
        public override void Dispose()
        {
            Disposals++; Rendering.Registry.Destroy(Fields); Rendering.Forget(Fields.Token);
        }
    }

    /// <summary>완료 전 미공개, 실제Form부착/Kernel실행/삭제와 오래된토큰 무효화를 연결한다.</summary>
    [Fact]
    public void ActualHostKernelFormSharesRegistrationLifetime()
    {
        var s = new Scene(); var process = new Probe { Rendering = s.Rendering }; int draws = 0;
        uint token = s.Rendering.Track(process.Fields, (fields, type, parent, flags) =>
        {
            Assert.Null(s.Rendering.Lookup(fields.Token)); Assert.Same(process, s.Host.Attach(process, type, parent, flags));
            fields.Form = process.Form(); fields.Parent = process.Parent();
        }, () => draws++);
        Assert.Throws<InvalidOperationException>(() => s.Rendering.Ready(token));
        s.Rendering.Draw(); Assert.Equal(0, draws);
        s.Rendering.Registry.Construct(process.Fields, token, s.Parent, 14); Assert.Equal(1, s.Kernel.Size());
        Assert.Same(process, s.Host.FromForm(process.Form())); Assert.Null(s.Rendering.Lookup(token));
        process.Fields.Right = 16; process.Fields.Bottom = 11;
        s.Rendering.Ready(token); Assert.Same(process.Fields, s.Rendering.Lookup(token));
        var output = new RenderCollectionEntry[1]; Assert.Equal(1, s.Rendering.Collect(new(-1, -1, 2, 2), output, 1));
        Assert.Equal(token, output[0].Token); Assert.Equal(3.5f, output[0].X);
        s.Kernel.RunFrame(); Assert.Equal(1, process.Frames); s.Rendering.Draw(); Assert.Equal(1, draws);
        int form = process.Form(); s.Host.Kill(process, 0x10);
        Assert.Equal(1, process.Disposals); Assert.Equal(0, s.Kernel.Size()); Assert.Equal(0, s.Rendering.Size);
        Assert.Null(s.Host.FromForm(form)); Assert.Null(s.Rendering.Lookup(token)); Assert.Equal(0u, s.Rendering.Slots[0]);
        s.Rendering.DrawToken(token); Assert.Equal(1, draws); Assert.Equal(-12345, process.Fields.Parent);
    }

    /// <summary>출력 중 자기삭제 뒤40칸을 넘겨 확장해도 현재 목록순서와 함수 수명을 유지한다.</summary>
    [Fact]
    public void DrawingRereadsCurrentExpandedSlotsAndCopiesSelfDeletingCallback()
    {
        var s = new Scene(); var first = new RenderProcessFields(); var events = new List<int>();
        uint token = s.Rendering.Track(first, (_, _, _, _) => { }, () =>
        {
            events.Add(0); s.Rendering.Registry.Destroy(first); s.Rendering.Forget(first.Token);
            // 같은0번 구멍과 확장 뒤40번을 포함해41개의 새 효과를 현재 등록한다.
            for (int i = 1; i <= 41; i++)
            {
                int number = i; var next = new RenderProcessFields();
                uint nextToken = s.Rendering.Track(next, (_, _, _, _) => { }, () => events.Add(number));
                s.Rendering.Registry.Construct(next, nextToken, 0, 14); s.Rendering.Ready(nextToken);
            }
        });
        s.Rendering.Registry.Construct(first, token, 0, 14); s.Rendering.Ready(token); s.Rendering.Draw();
        Assert.Equal(new[] { 0 }.Concat(Enumerable.Range(2, 40)), events); Assert.Equal(80, s.Rendering.Slots.Length); Assert.Null(s.Rendering.Lookup(token));
        events.Clear(); s.Rendering.Draw(); Assert.Equal(Enumerable.Range(1, 41), events);
    }

    /// <summary>수집은 현재배율·미완성null·원문head만 읽고 limit뒤 불완전부모에 접근하지 않는다.</summary>
    [Fact]
    public void CollectionUsesCurrentScaleAndStopsBeforeNextInvalidParent()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(50).Clear(); pool.RawSlot(50)[6] = 7;
        var state = new ConstructionVisualState(); var fields = new RenderProcessFields { Parent = 50, Right = 16, Bottom = 11 };
        var bad = new RenderProcessFields { Parent = -1 };
        var registry = new RawRenderRegistry(pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (_, _, _, _) => { } }, new uint[] { 1, 2 });
        var collection = new RawRenderCollection(pool, state, registry, token => token == 1 ? fields : bad);
        var output = new[] { new RenderCollectionEntry(0, 0, 0, 0, 0, 0xab) }; var view = new RenderWorldRect(-1, -1, 2, 2);
        Assert.Equal(new(0, 0, 1, 1), collection.Bounds(fields)); state.ScaleX = 8; Assert.Equal(new(0, 0, 2, 1), collection.Bounds(fields));
        Assert.Equal(1, collection.Collect(view, output, 1)); Assert.Equal(0xab, output[0].Tail);
        Assert.Throws<ArgumentException>(() => collection.Collect(view, output, 2));
        state.ScaleX = 0; Assert.Equal(0, collection.Collect(view, [], 0)); Assert.Equal(0, collection.Collect(view, [], -1));
        Assert.Throws<ArgumentException>(() => collection.Bounds(fields));
        var absent = new RawRenderCollection(pool, state, registry, _ => null); Assert.Equal(0, absent.Collect(view, output, 1));
    }

    /// <summary>확보 실패/중복/불일치 삭제가 기존 목록을 손상시키지 않고 함수 사본을 보존한다.</summary>
    [Fact]
    public void RegistryCopiesFunctionsAndKeepsPriorSlotsOnFailure()
    {
        var pool = new RawSidPool(24000, false); var fields = new RenderProcessFields(); int attachments = 0;
        var hooks = new RenderRegistryHooks { Reserve = _ => false, ReleaseArray = () => Assert.Fail("실패 확보 반납"), Attach = (_, _, _, _) => attachments++ };
        var registry = new RawRenderRegistry(pool, hooks, new uint[] { 99 }); hooks.Reserve = _ => true;
        Assert.Throws<OutOfMemoryException>(() => registry.Construct(fields, 1, 50, 14)); Assert.Equal(new uint[] { 99 }, registry.Slots.ToArray()); Assert.Equal(0, attachments);
        var live = new RawRenderRegistry(pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (_, _, _, _) => attachments++ });
        live.Construct(fields, 1, 50, 14); uint[] before = live.Slots.ToArray();
        Assert.Throws<ArgumentException>(() => live.Construct(fields, 1, 50, 14)); Assert.Throws<InvalidOperationException>(() => live.Construct(fields, 2, 50, 14));
        var wrong = new RenderProcessFields { Token = 2, Slot = 0 }; Assert.Throws<InvalidOperationException>(() => live.Destroy(wrong)); Assert.Equal(before, live.Slots.ToArray());
        live.Destroy(fields); Assert.Throws<InvalidOperationException>(() => live.Destroy(fields));
    }

    /// <summary>건설 생성 어댑터는 실제 부착 뒤의 부모/영역과 진입부모깊이를 돌려주며 입력은 보존한다.</summary>
    [Fact]
    public void ConstructionRegistryAdapterUsesAttachedCurrentFields()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(50)[35] = 255; var registration = new RenderProcessFields();
        var registry = new RawRenderRegistry(pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (fields, type, parent, flags) =>
        { Assert.Equal((17u, 50, 16u), (type, parent, flags)); fields.Parent = 51; fields.Form = 7; fields.Left = 3; fields.Bottom = 8; pool.RawSlot(parent)[35] = 128; } });
        var input = new ConstructionVisualHooks { Reserve = _ => true }; var bound = RawConstructionLifetime.MakeRegistryHooks(registry, registration, 1, input);
        Assert.Null(input.Attach); Assert.Same(input.Reserve, bound.Reserve); var visual = new ConstructionVisualFields { Parent = 50 };
        bound.Attach!(visual, 17); Assert.Equal(51, visual.Parent); Assert.Equal(-128, visual.ZOrder); Assert.Equal(3, visual.Left); Assert.Equal(8, visual.Bottom);
        Assert.Throws<ArgumentException>(() => RawConstructionLifetime.MakeRegistryHooks(registry, registration, 0, input));
    }
}
