using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// contained Form 생성기(RawContainedCreate)의 계약을 확인한다.
/// cpppj 에는 이 몸체만 따로 관찰한 x86 fixture 가 없으므로(ReturnPiece·FormLifecycle 등이 함께 쓴다)
/// 원본 소스의 쓰기 순서·기본값·경계를 합성 입력으로 검사하고, 실제 팩토리 연결까지 확인한다.
/// 기준 판본은 10.78 이다 (W03 내용물 생성 조각, 2026-10-23).
/// </summary>
public sealed class RawContainedCreateTests
{
    /// <summary>장면 풀의 슬롯 수.</summary>
    private const int PoolCapacity = 32768;

    /// <summary>10.78 contained Form(타입 6)의 가상 표 값.</summary>
    private const uint ContainedVtable = 0x00512208;

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static ushort Word(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>슬롯의 32비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static uint Dword(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
    }

    /// <summary>
    /// 생성 경계를 실제 내장 Form 생성기로 채운 장면. 부착·표면 통지 호출을 순서대로 기록한다.
    /// </summary>
    private sealed class Scene
    {
        /// <summary>내용물 슬롯을 가진 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>타입 번호 순서의 타입 레코드.</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>내장 Form 생성기. 실제 생성 경로를 쓴다.</summary>
        public readonly RawSquidForms Forms;

        /// <summary>생성 시 받은 생성 영역 플래그.</summary>
        public readonly List<uint> FactoryFlags = [];

        /// <summary>표면 통지로 받은 번호.</summary>
        public readonly List<int> Notified = [];

        /// <summary>부착 호출 기록(Form, 부모, 플래그).</summary>
        public readonly List<(int Form, int Parent, uint Flags)> Attached = [];

        /// <summary>검증 대상 생성기.</summary>
        public readonly RawContainedCreate Contained;

        /// <summary>장면을 만든다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(PoolCapacity, true);
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 표는 값 객체 레코드로 채운다
            for (int index = 0; index < Types.Length; index++)
            {
                Types[index] = new OriginalTypeRecord();
            }
            Forms = new RawSquidForms(Pool);
            var state = new ContainedCreateState();
            var hooks = new ContainedCreateHooks
            {
                Create = (type, flags) =>
                {
                    Assert.Equal(6u, type);
                    FactoryFlags.Add(flags);
                    SidAllocation allocation = flags == 1 ? SidAllocation.Predictable
                        : flags == 2 ? SidAllocation.Client : SidAllocation.Server;
                    return Forms.Create(6, allocation);
                },
                NotifySurface = sid => Notified.Add(sid),
                Attach = (form, parent, flags) => Attached.Add((form, parent, flags)),
            };
            Contained = new RawContainedCreate(Pool, state, hooks);
        }

        /// <summary>서버 영역에 부모 자산을 하나 만든다.</summary>
        public int NewParent() => Pool.Allocate();
    }

    /// <summary>
    /// flags 0 은 0x20 으로 바뀌고, 저장 타입(+18)·인자(+12)·품질(+30)·플래그(+20)가 원본 순서대로 쓰이며
    /// 부착 경계가 마지막에 한 번 불린다.
    /// </summary>
    [Fact]
    public void WritesStoredFieldsAndCallsAttach()
    {
        var scene = new Scene();
        int parent = scene.NewParent();
        int sid = scene.Contained.Create(parent, 100, 0x0182, 0, 0x10, 2);
        Assert.Equal(6, scene.Pool.Slot(sid)[10]);
        Assert.Equal(100, Word(scene.Pool, sid, 18));
        Assert.Equal(0x0182, Word(scene.Pool, sid, 12));
        Assert.Equal(0x20, Word(scene.Pool, sid, 20));
        Assert.Equal(2, scene.Pool.Slot(sid)[30]);
        Assert.Equal(new List<uint> { 2 }, scene.FactoryFlags);
        Assert.Empty(scene.Notified);
        Assert.Equal(new List<(int, int, uint)> { (sid, parent, 0x10) }, scene.Attached);
    }

    /// <summary>부착 플래그 0x20 은 0x10 보다 우선해 예측 영역 생성을 요구한다.</summary>
    [Fact]
    public void PredictableAttachWinsOverClient()
    {
        var scene = new Scene();
        int parent = scene.NewParent();
        int sid = scene.Contained.Create(parent, 100, 0, 0x108, 0x30, 0);
        Assert.Equal(new List<uint> { 1 }, scene.FactoryFlags);
        Assert.Equal(RawSidPool.PredictableFirst + 1, sid);
        // flags 는 하위 WORD 만 쓰고 품질 BYTE 는 0 으로 남는다
        Assert.Equal(0x0108, Word(scene.Pool, sid, 20));
        Assert.Equal(0, scene.Pool.Slot(sid)[30]);
    }

    /// <summary>생성된 슬롯이 다리 타입(82)이면 표면 통지를 한 번 부른다.</summary>
    [Fact]
    public void NotifiesSurfaceForBridgeType()
    {
        var scene = new Scene();
        int parent = scene.NewParent();
        // 실제 다리 생성처럼 생성 콜백이 타입 바이트를 82 로 바꾼다
        var state = new ContainedCreateState();
        var hooks = new ContainedCreateHooks
        {
            Create = (type, flags) =>
            {
                int sid = scene.Forms.Create(6, SidAllocation.Server);
                scene.Pool.RawSlot(sid)[10] = 82;
                return sid;
            },
            NotifySurface = sid => scene.Notified.Add(sid),
            Attach = (form, objectParent, flags) => scene.Attached.Add((form, objectParent, flags)),
        };
        var contained = new RawContainedCreate(scene.Pool, state, hooks);
        int created = contained.Create(parent, 100, 0, 0x20, 0x10, 0);
        Assert.Equal(new List<int> { created }, scene.Notified);
        Assert.Equal(82, scene.Pool.Slot(created)[10]);
    }

    /// <summary>부모·생성 번호·경계 누락은 슬롯을 바꾸기 전에 거부한다.</summary>
    [Fact]
    public void RejectsInvalidContracts()
    {
        var scene = new Scene();
        int parent = scene.NewParent();
        Assert.Throws<ArgumentException>(() => new RawContainedCreate(scene.Pool, new ContainedCreateState(), new ContainedCreateHooks()));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Contained.Create(0, 100, 0, 0, 0x10, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Contained.Create(scene.Pool.Capacity, 100, 0, 0, 0x10, 0));
        var empty = new ContainedCreateHooks
        {
            Create = (_, _) => 0,
            NotifySurface = _ => { },
            Attach = (_, _, _) => { },
        };
        var invalid = new RawContainedCreate(scene.Pool, new ContainedCreateState(), empty);
        Assert.Throws<ArgumentOutOfRangeException>(() => invalid.Create(parent, 100, 0, 0, 0x10, 0));
        Assert.Empty(scene.Attached);
    }

    /// <summary>
    /// BindCreate 는 기존 contained 생성기의 부착 경계만 ContentForm 정렬 부착기로 바꾼다.
    /// 생성은 실제 팩토리의 contained 생성자를 쓰고, 부착은 부모 알림·목록 순서를 따르며 기존 Attach 콜백은 부르지 않는다.
    /// 추가: W03 삭제·수명 열다섯째 조각(2026-10-23).
    /// </summary>
    [Fact]
    public void BindCreateReplacesOnlyAttachBoundary()
    {
        var scene = new Scene();
        var factory = new RawSquidFactory(scene.Pool, scene.Types);
        var reasons = new List<(int Parent, int Form, uint Reason)>();
        var forms = new RawContentFormHost(scene.Pool, scene.Types,
            (parent, form, reason) => reasons.Add((parent, form, reason)));
        int parent = factory.Create(90, SidAllocation.Client);
        var hooks = new ContainedCreateHooks
        {
            NotifySurface = sid => scene.Notified.Add(sid),
            // BindCreate 가 이 부착 콜백을 ContentForm 정렬 부착기로 바꿔야 한다
            Attach = (_, _, _) => throw new InvalidOperationException("기존 부착 경계가 호출됐습니다"),
        };
        ContainedCreateHooks bound = forms.BindCreate(factory, hooks);
        var contained = new RawContainedCreate(scene.Pool, new ContainedCreateState(), bound);
        int sid = contained.Create(parent, 100, 0x0182, 0, 0x10, 2);
        // 생성은 실제 팩토리의 contained 전용 생성자를 그대로 쓴다
        Assert.Equal(ContainedVtable, Dword(scene.Pool, sid, 0));
        Assert.Equal(6, scene.Pool.Slot(sid)[10]);
        Assert.Equal(100, Word(scene.Pool, sid, 18));
        // 정렬 부착기가 부모 머리와 contained 비트를 세우고 void 를 끈다
        Assert.Equal(sid, Word(scene.Pool, parent, 6));
        Assert.Equal(8, scene.Pool.Slot(sid)[11] & 0x0c);
        Assert.True(forms.Linked(sid));
        // 부모 부착 알림은 한 번만 나가며 표면 통지(다리 아님)는 없다
        Assert.Equal(new List<(int, int, uint)> { (parent, sid, 1u) }, reasons);
        Assert.Empty(scene.Notified);
        // 다른 풀의 팩토리는 부착 경계를 바꾸기 전에 거부한다
        var other = new RawSidPool(PoolCapacity, true);
        var otherFactory = new RawSquidFactory(other, scene.Types);
        Assert.Throws<ArgumentException>(() => forms.BindCreate(otherFactory, new ContainedCreateHooks
        {
            NotifySurface = _ => { },
            Attach = (_, _, _) => { },
        }));
    }

    /// <summary>
    /// 실제 팩토리 연결은 같은 풀만 받고, contained 전용 타입 6 만 만들어 내장 Form 가상 표를 쓴다.
    /// 다른 풀의 팩토리와 다른 타입 요청은 쓰기 전에 거부한다.
    /// </summary>
    [Fact]
    public void ConnectsFactoryWithSamePoolOnly()
    {
        var scene = new Scene();
        int parent = scene.NewParent();
        var factory = new RawSquidFactory(scene.Pool, scene.Types);
        var hooks = RawContainedCreate.ConnectFactory(scene.Pool, factory, new ContainedCreateHooks
        {
            NotifySurface = sid => scene.Notified.Add(sid),
            Attach = (form, objectParent, flags) => scene.Attached.Add((form, objectParent, flags)),
        });
        var contained = new RawContainedCreate(scene.Pool, new ContainedCreateState(), hooks);
        int sid = contained.Create(parent, 100, 0x0182, 0, 0x10, 2);
        Assert.Equal(ContainedVtable, Dword(scene.Pool, sid, 0));
        Assert.Equal(6, scene.Pool.Slot(sid)[10]);
        Assert.Equal(100, Word(scene.Pool, sid, 18));
        // 다른 풀의 팩토리는 연결 시점에 거부한다
        var other = new RawSidPool(PoolCapacity, true);
        var otherFactory = new RawSquidFactory(other, scene.Types);
        Assert.Throws<ArgumentException>(() => RawContainedCreate.ConnectFactory(scene.Pool, otherFactory,
            new ContainedCreateHooks { NotifySurface = _ => { }, Attach = (_, _, _) => { } }));
        // 연결된 생성 경계는 타입 6 이 아니면 거부한다
        Assert.Throws<ArgumentException>(() => hooks.Create!(7, 0));
    }
}
