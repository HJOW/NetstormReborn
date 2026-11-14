using Netstorm.Core.Display;
using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 Take 부모와 실제 공장의 현재 해제/생성자/대여를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>반환 객체·저장한 VT·진단 복귀와 실제 장치의 부분 효과를 검사한다.</summary>
public sealed class SquidTakeParentCurrentTests
{
    /// <summary>원본 장치가 사용하는 실제 객체 토큰의 기준값이다. SID로 재해석하지 않는다.</summary>
    private const uint ObjectBase = 0x13000000;
    /// <summary>명시 가상 장치의 VT 토큰이다.</summary>
    private const uint Vtable = 0x14000000;
    /// <summary>명시 cdecl 생성자 장치의 주소 토큰이다.</summary>
    private const uint Constructor = 0x10001000;

    /// <summary>720개 양판본 원본 관측의 전체 읽기/쓰기·진단·반환 객체와 두 payload를 비교한다.</summary>
    [Fact]
    public void ReturnedObjectsAndCachedVtableOrderMatchIndependentPe()
    {
        int count = 0; string[][] rows = X86Fixture.Read("squid-take-parent-current"); Assert.Equal(720, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length); uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(5, input.Length);
            bool patch = row[0] == "originals", useBase = input[4] != 0; Assert.True(patch || row[0] == "originalCD");
            int width = patch ? 50 : 36; uint type = input[0], sid = input[1], profile = input[3], ctor = useBase ? 0 : Constructor;
            uint[] objects = [ObjectBase + (uint)(5 * width), ObjectBase + (uint)(9 * width)]; byte[][] slots = [new byte[width], new byte[width]]; var trace = new List<string>();
            // 원본 두 객체의 전체 payload와 명시 입력만 공급한다.
            for (int index = 0; index < slots.Length; index++)
            {
                // 실제 판본 stride와 같은 입력 패턴이다.
                for (int offset = 0; offset < width; offset++) slots[index][offset] = unchecked((byte)(((index == 0 ? 5 : 9) * width + offset) * 37 + 19));
                BinaryPrimitives.WriteUInt32LittleEndian(slots[index], Vtable); slots[index][10] = 74; slots[index][11] = (byte)input[2];
            }
            // 토큰 전체를 실제 두 배열에만 연결한다.
            byte[] Raw(uint token) { Assert.Contains(token, objects); return slots[token == objects[0] ? 0 : 1]; }
            var hooks = new RawSquidTakeHooks
            {
                Capacity = () => { trace.Add("Q32768"); return 32768; }, Slot = _ => objects[0],
                State = token => { byte value = Raw(token)[11]; trace.Add($"R{token}:{value}"); return value; },
                Type = token => { byte value = Raw(token)[10]; trace.Add($"T{token}:{value}"); return value; },
                Vtable = token => { uint value = BinaryPrimitives.ReadUInt32LittleEndian(Raw(token)); trace.Add($"V{token}:{value}"); return value; },
                Detail = (server, client) => { trace.Add($"F{server}:{client}"); trace.Add("L"); },
                Assertion = (expression, file, line) => trace.Add($"D{expression}|{file}|{line}"),
                Unpop = (token, table) => { trace.Add($"U{token}:{table}"); if (profile == 1) { Raw(token)[11] = 0x8f; ctor = useBase ? 0 : Constructor + 0x10; } },
                StoreState = (token, value) => { trace.Add($"W{token}:{value}"); Raw(token)[11] = value; },
                StoreType = (token, value) => { trace.Add($"Y{token}:{value}"); Raw(token)[10] = value; if (profile == 2) { BinaryPrimitives.WriteUInt32LittleEndian(Raw(token), Vtable + 0x100); Raw(token)[11] = 0x80; } },
                Constructor = number => { trace.Add($"C{number}:{ctor}"); return ctor; },
                Construct = (address, number) =>
                {
                    if (address == 0) { uint table = patch ? 0x501dd0u : 0x5058d8u; trace.Add($"B{objects[0]}:{table}"); BinaryPrimitives.WriteUInt32LittleEndian(Raw(objects[0]), table); return objects[0]; }
                    trace.Add($"A{address}:{number}"); uint token = objects[profile == 1 ? 1 : 0]; BinaryPrimitives.WriteUInt32LittleEndian(Raw(token), Vtable); return token;
                },
                PostTake = (token, table) => trace.Add($"P{token}:{table}"),
            };
            uint returned = RawSquidTake.Take(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, type, sid, hooks);
            Assert.Equal(X86Fixture.UInt(row[2]), returned); Assert.Equal(row[3], string.Join(';', trace));
            Assert.Equal(row[4], Convert.ToHexStringLower(slots[0]) + ";" + Convert.ToHexStringLower(slots[1])); count++; return true;
        });
        Assert.Equal(720, count);
    }

    /// <summary>하한 단락 평가와 선택 실패 앞의 CD 두 상태 저장을 보존한다.</summary>
    [Fact]
    public void SelectedMissingConstructorLeavesBothCdStateStores()
    {
        byte state = 3; var writes = new List<byte>();
        var hooks = new RawSquidTakeHooks { Assertion = (_, _, _) => { }, Slot = _ => 5, State = _ => state, StoreState = (_, value) => { state = value; writes.Add(value); } };
        Assert.Throws<InvalidOperationException>(() => RawSquidTake.Take(OriginalEdition.Cd1072, 74, 5, hooks));
        Assert.Equal(new byte[] { 2, 0 }, writes); Assert.Equal(0, state);
    }

    /// <summary>실제 공간 해제의 선택 인자와 재진입 효과를 관측한다.</summary>
    private sealed class UnpopSpy : IRawUnpop
    {
        /// <summary>같은 실제 객체 풀이다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>선택한 실제 객체/현재 TYPE를 전달하는 외부 입력이다.</summary>
        public Action<int, OriginalTypeRecord>? Effect { get; set; }
        /// <summary>실제 호출 수다.</summary>
        public int Calls { get; private set; }
        /// <summary>장기 풀을 빌린다.</summary>
        public UnpopSpy(RawSidPool pool) => Pool = pool;
        /// <summary>실제 소비된 선택 인자를 전달한다. 기본 입력은 현재 void 비트만 기록한다.</summary>
        void IRawUnpop.Unpop(int sid, OriginalTypeRecord type) { Calls++; if (Effect is { } effect) effect(sid, type); else Pool.RawSlot(sid)[11] |= 4; }
    }

    /// <summary>현재 물리 TYPE 표를 만든다.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

    /// <summary>void 여부에 관계없이 현재 VT+48을 호출하고 해제 뒤 현재 생성자/후처리를 선택한다.</summary>
    [Fact]
    public void FactoryVoidTakeCallsActualUnpopThenCurrentConstructorAndPostTake()
    {
        var pool = new RawSidPool(32768, true); var types = Types(); var unpop = new UnpopSpy(pool); var factory = new RawSquidFactory(pool, types, unpop: unpop);
        int sid = factory.Create(74); uint free = pool.FreeCount; pool.RawSlot(sid)[26] = 0x78; pool.RawSlot(sid)[27] = 0x56; pool.RawSlot(sid)[34] = 9;
        Assert.Equal(4, pool.Slot(sid)[11]);
        unpop.Effect = (number, record) => { Assert.Equal(sid, number); Assert.Same(types[74], record); types[74].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 161); types[74].ZOrder = 0x144; };
        Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, unpop.Calls); Assert.Equal(0, pool.Slot(sid)[26]); Assert.Equal(0, pool.Slot(sid)[27]);
        Assert.Equal(9, pool.Slot(sid)[34]); Assert.Equal(0x44, pool.Slot(sid)[35]); Assert.Equal(free, pool.FreeCount);
    }

    /// <summary>TYPE 불일치의 두 이름은 첫NUL 앞을 복사하고 진단 복귀 뒤 현재 클래스가 달라도 실제 해제를 실행한다.</summary>
    [Fact]
    public void FactoryReturningDiagnosticCopiesNamesThenUsesCurrentVtable()
    {
        var pool = new RawSidPool(32768, true); var types = Types(); types[74].Name = "SERVER\0tail"; types[75].Name = "CLIENT\0tail";
        var unpop = new UnpopSpy(pool); var factory = new RawSquidFactory(pool, types, unpop: unpop); int sid = factory.Create(74); pool.RawSlot(sid)[10] = 75; string? detail = null; int assertions = 0;
        factory.BindDiagnostics(new() { Types = types, Detail = text => detail = text, Assertion = (expression, file, line) =>
        {
            Assert.Equal("0", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x1bau, line); Assert.Equal("Invalid take. Server type SERVER, client type CLIENT", detail);
            assertions++; types[74].ZOrder = 0x123; types[75].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 161);
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), OriginalConstructors.Recipe(pool.Edition, types[75].ConstructorAddress)!.Vtable);
        } });
        Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, assertions); Assert.Equal(1, unpop.Calls); Assert.Equal(0x23, pool.Slot(sid)[35]);
    }

    /// <summary>미지원 생성자는 앞선 실제 상태 저장을 취소하지 않고 null 반환 객체를 SID로 위조하지 않는다.</summary>
    [Fact]
    public void FactorySelectedConstructorFailureKeepsPriorStateWrites()
    {
        var pool = new RawSidPool(32768, true); var types = Types(); var factory = new RawSquidFactory(pool, types); int sid = 16000;
        pool.RawSlot(sid)[11] = 0x8f; types[74].ConstructorAddress = 0xdeadbeef;
        Assert.Throws<InvalidOperationException>(() => factory.Take(74, sid)); Assert.Equal(0x8c, pool.Slot(sid)[11]);
        pool.RawSlot(sid)[11] |= 1; types[74].ConstructorAddress = 0x4429e0; Assert.Contains("void NULL", Assert.Throws<InvalidOperationException>(() => factory.Take(74, sid)).Message); Assert.Equal(0x8c, pool.Slot(sid)[11]);
    }

    /// <summary>같은 Pool의 현재 runtime 입력을 외부 효과 장치와 함께 만든다.</summary>
    private static SquidFactoryRuntimeBindings Runtime(IRawUnpop unpop, Action<int>? detach = null) => new()
    {
        Unpop = unpop, FormDetach = detach ?? (_ => throw new InvalidOperationException("선택되지 않은 분리")),
        ContentType = () => 6, FormTransmit = (_, _) => throw new InvalidOperationException("선택되지 않은 전송"),
    };

    /// <summary>선택된 현재 대여가 독립 장기 getter를 앞서며 반납 뒤 같은 장기 실제 해제로 돌아간다.</summary>
    [Fact]
    public void FactoryRuntimeOverridesDefaultAndDefersReleaseUntilActualCallReturns()
    {
        var pool = new RawSidPool(32768, true); var types = Types(); var fallback = new UnpopSpy(pool); var current = new UnpopSpy(pool);
        var factory = new RawSquidFactory(pool, types); int getterCalls = 0, pins = 0; object owner = new();
        factory.BindUnpop(() => { getterCalls++; return fallback; }); int sid = factory.Create(74);
        SquidFactoryRuntimeBindings runtime = Runtime(current); runtime.Pin = () => { pins++; return owner; };
        current.Effect = (number, _) =>
        {
            Assert.Equal(sid, number); Assert.Equal(1u, runtime.ActiveCalls); factory.ReleaseRuntime(runtime);
            Assert.Throws<ArgumentException>(() => factory.BindRuntime(Runtime(fallback)));
        };
        factory.BindRuntime(runtime); factory.BindRuntime(runtime);
        Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, current.Calls); Assert.Equal(0, getterCalls); Assert.Equal(1, pins); Assert.Equal(0u, runtime.ActiveCalls);
        Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, fallback.Calls); Assert.Equal(1, getterCalls);
        var otherPool = new RawSidPool(32768, true); Assert.Throws<ArgumentException>(() => factory.BindRuntime(Runtime(new UnpopSpy(otherPool))));
        Assert.Throws<ArgumentException>(() => factory.BindRuntime(new() { Unpop = current })); GC.KeepAlive(owner);
    }

    /// <summary>제공자가 보관하지 않는 runtime은 장기 공장 때문에 살아남지 않는다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference<SquidFactoryRuntimeBindings> BorrowOnly(RawSquidFactory factory, IRawUnpop unpop)
    {
        SquidFactoryRuntimeBindings runtime = Runtime(unpop); factory.BindRuntime(runtime); return new(runtime);
    }

    /// <summary>호출 밖의 약한 대여가 만료되면 원래 실제 공간 장치로 돌아간다.</summary>
    [Fact]
    public void FactoryDoesNotOwnPreviousRuntimeOutsideCall()
    {
        var pool = new RawSidPool(32768, true); var unpop = new UnpopSpy(pool); var factory = new RawSquidFactory(pool, Types(), unpop: unpop);
        int sid = factory.Create(74); WeakReference<SquidFactoryRuntimeBindings> weak = BorrowOnly(factory, new UnpopSpy(pool));
        GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.False(weak.TryGetTarget(out _));
        Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, unpop.Calls);
    }

    /// <summary>호출 외부에 owner 강한 참조를 남기지 않는 실제 pin 입력 장면이다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (RawSquidFactory Factory, UnpopSpy Unpop, SquidFactoryRuntimeBindings Runtime, WeakReference<object> Owner, int Sid) OwnerScene()
    {
        var pool = new RawSidPool(32768, true); var unpop = new UnpopSpy(pool); var factory = new RawSquidFactory(pool, Types()); object owner = new();
        SquidFactoryRuntimeBindings runtime = Runtime(unpop); runtime.Pin = () => owner; factory.BindRuntime(runtime);
        return (factory, unpop, runtime, new(owner), factory.Create(74));
    }

    /// <summary>장치를 반납해도 실제 외곽 owner를 Take 전체 호출 동안 유지하고 pin 실패는 상태 쓰기를 앞선다.</summary>
    [Fact]
    public void FactoryCallPinsActualOuterOwnerAndPinFailureLeavesSlotsUntouched()
    {
        var s = OwnerScene(); s.Unpop.Effect = (_, _) =>
        {
            s.Runtime.Pin = null; GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.True(s.Owner.TryGetTarget(out _));
        };
        Assert.Equal(s.Sid, s.Factory.Take(74, s.Sid)); Assert.Equal(0u, s.Runtime.ActiveCalls);
        s.Runtime.Pin = () => throw new InvalidOperationException("실제 owner pin 실패");
        byte[] before = s.Unpop.Pool.Slot(s.Sid).ToArray(); Assert.Throws<InvalidOperationException>(() => s.Factory.Take(74, s.Sid));
        Assert.Equal(before, s.Unpop.Pool.Slot(s.Sid).ToArray()); Assert.Equal(0u, s.Runtime.ActiveCalls);
    }

    /// <summary>명시 부착 입력을 넣되 정렬/Pop 자체를 대신 검증했다고 주장하지 않는다.</summary>
    private static void Attachment(RawSidPool pool, int form, int parent)
    {
        pool.RawSlot(form)[11] = 8; BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[4..], 0); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[14..], 0);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[16..], (ushort)parent); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent)[6..], (ushort)form);
    }

    /// <summary>실제 World 공장 Take가 현재 Session 분리/TYPE7 생성자 변경을 소비하고 반납 뒤 World 기본 종단으로 돌아간다.</summary>
    [Fact]
    public void WorldFactoryUsesCurrentSessionDetachAndCurrentConstructorThenDefault()
    {
        var types = Types().ToList(); types[6].ConstructorAddress = OriginalConstructors.Address(6); types[7].ConstructorAddress = OriginalConstructors.Address(3); types[7].ZOrder = 0x44;
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToList(); var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToList();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes); var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames); RawWorldCore core = world.Core();
        int parent = core.Factory.Create(74), form = core.Factory.CreateContained(0); core.Pool.RawSlot(form)[10] = 7; core.Pool.RawSlot(form)[35] = 0x77; Attachment(core.Pool, form, parent);
        BinaryPrimitives.WriteUInt32LittleEndian(core.Pool.RawSlot(form)[18..], 0x12345678); int notices = 0;
        var state = new SquidContainedUnpopState { ContentFormType = new() { Value = 7 }, GumpFormType = new() { Value = 3 }, UiEvent = new() };
        var session = new RawSquidContainedUnpop(core.Pool, state, new()
        {
            Parent = sid => RawContainedLink.CommonContainedParent(core.Pool, sid), Previous = sid => RawContainedLink.CommonContainedPrevious(core.Pool, sid),
            SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(core.Pool, sid, previous),
            ParentNotice = (owner, sid, reason) => { Assert.Equal(parent, owner); Assert.Equal(form, sid); Assert.Equal(2u, reason); Assert.Equal(4, core.Pool.Slot(form)[11]); notices++; types[7].ConstructorAddress = OriginalConstructors.Address(1); },
        });
        world.BindContainedUnpop(session); uint free = core.Pool.FreeCount; Assert.Equal(form, core.Factory.Take(7, form));
        Assert.Equal(1, notices); Assert.Equal(0x512110u, BinaryPrimitives.ReadUInt32LittleEndian(core.Pool.Slot(form))); Assert.Equal(7, core.Pool.Slot(form)[10]); Assert.Equal(0x77, core.Pool.Slot(form)[35]);
        Assert.Equal(0x12345678u, BinaryPrimitives.ReadUInt32LittleEndian(core.Pool.Slot(form)[18..])); Assert.Equal(free, core.Pool.FreeCount);
        world.BindContainedUnpop(null); Attachment(core.Pool, form, parent); core.Factory.UnpopForm(form);
        Assert.Equal(1, notices); Assert.Equal(4, core.Pool.Slot(form)[11]); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(core.Pool.Slot(parent)[6..]));
    }

    /// <summary>실제 공장도 두 판본의 네 Form VT를 읽고 자산 VT를 가로막지 않는다. CD 관리풀 전체 복원을 뜻하지 않는다.</summary>
    [Fact]
    public void ActualFactoryTakeUsesEditionFormTablesAndActualAssetUnpop()
    {
        // 같은 실제 공장의 선택 분배를 판본별 기록값과 비교한다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            var pool = new RawSidPool(32768, edition: edition); var types = Types(); var unpop = new UnpopSpy(pool); var factory = new RawSquidFactory(pool, types, unpop: unpop);
            var forms = new RawSquidForms(pool); int sid = 16000, z = edition == OriginalEdition.Patch1078 ? 35 : 33;
            // TYPE7 별칭도 실제 네 Form VT의 해제/빈후처리를 선택한다.
            foreach (uint formType in new uint[] { 1, 2, 3, 6 })
            {
                forms.ConstructVtableAt(formType, sid); pool.RawSlot(sid)[10] = 7; pool.RawSlot(sid)[11] = 0; pool.RawSlot(sid)[z] = 0x77;
                BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid)[16..], 9); types[7].ConstructorAddress = OriginalConstructors.Address(edition, (int)formType); types[7].ZOrder = 0x44;
                uint table = BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)); Assert.Equal(sid, factory.Take(7, sid));
                Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid))); Assert.Equal(0x77, pool.Slot(sid)[z]); Assert.Equal(4, pool.Slot(sid)[11]);
            }
            pool.RawSlot(sid)[10] = 74; pool.RawSlot(sid)[11] = 4; types[74].ZOrder = 0x55;
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), OriginalConstructors.BaseVtableFor(edition));
            Assert.Equal(sid, factory.Take(74, sid)); Assert.Equal(1, unpop.Calls); Assert.Equal(0x55, pool.Slot(sid)[z]);
        }
    }

}
