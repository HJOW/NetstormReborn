using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 감시 생성 부모의 현재 입력과 실제 공장 연결을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj/tests/DependCreateCurrentTests.cpp의 최초 PE 관측과 현재 생성자 계약을 옮긴다.</summary>
public sealed class DependCreateCurrentTests
{
    /// <summary>원본 관측의 명시 가상 장치가 반환하는 토큰 시작값이다.</summary>
    private const uint DependVtable = 0x14000000;

    /// <summary>PATCH810·CD810개의 생성·WORD·저장/현재 가상표·부착 사건과 두 슬롯 전체를 원본에 대조한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, "originals")]
    [InlineData(OriginalEdition.Cd1072, "originalCD")]
    public void X86IndependentParentFixture1620(OriginalEdition edition, string name)
    {
        int count = 0;
        // 각 최초 관측에는 같은 명시 생성자/진단/부착 장치 입력을 독립 공급한다.
        X86Fixture.CheckRows(X86Fixture.Read("depend-create-parent-current"), row =>
        {
            if (row[0] != name) return true;
            uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray();
            uint profile = input[3], number = 7, creates = 0;
            bool server = input[2] != 0;
            int stride = edition == OriginalEdition.Patch1078 ? 50 : 36;
            byte[][] slots = Enumerable.Range(0, 16).Select(_ => new byte[stride]).ToArray();
            // 원본 PE에 공급한 작은 물리 입력만 재현하며 실제 게임 풀이나 생성자로 주장하지 않는다.
            for (int slot = 0; slot < slots.Length; slot++)
                // 원본 슬롯 전체의 초기 payload를 같은 stride로 만든다.
                for (int offset = 0; offset < stride; offset++) slots[slot][offset] = unchecked((byte)((slot * stride + offset) * 37 + 19));
            var events = new List<string>();
            var hooks = new DependCreateHooks
            {
                Server = () => server, FormType = () => number, Sid = token => token,
                Create = (type, flags) =>
                {
                    uint sid = creates == 0 ? 5u : 9u;
                    events.Add($"C{type},{flags},{sid}");
                    BinaryPrimitives.WriteUInt32LittleEndian(slots[sid], DependVtable + (sid == 5 ? 0u : 0x100u));
                    slots[sid][10] = unchecked((byte)type); slots[sid][11] = 4;
                    if (profile == 1 && creates == 0) number = 0x12345678;
                    creates++; return sid;
                },
                Word = (sid, offset, value) =>
                {
                    events.Add($"W{sid},{offset},{value}");
                    BinaryPrimitives.WriteUInt16LittleEndian(slots[sid].AsSpan(offset), value);
                    if (profile == 3 && sid == 9 && offset == 18) BinaryPrimitives.WriteUInt32LittleEndian(slots[5], DependVtable + 0x200);
                },
                Vtable = sid => BinaryPrimitives.ReadUInt32LittleEndian(slots[sid]),
                Attach = (sid, table, parent, flags) =>
                {
                    events.Add($"A{sid},{table},{parent},{flags}");
                    if (profile == 4 && sid == 5) BinaryPrimitives.WriteUInt32LittleEndian(slots[9], DependVtable + 0x200);
                },
                Assertion = (expression, file, line) =>
                {
                    events.Add($"D{expression}|{file}|{line}");
                    if (profile == 2) { number = uint.MaxValue; server = true; }
                },
            };
            Assert.Equal(new DependCreateObjects(9, 5), RawDependCreate.Create(edition, input[0], input[1], hooks));
            Assert.Equal(row[2], number.ToString()); Assert.Equal(row[3] == "1", server);
            Assert.Equal(row[4], string.Join(';', events));
            Assert.Equal(row[5], Convert.ToHexStringLower(slots[5]) + ";" + Convert.ToHexStringLower(slots[9]));
            count++; return true;
        });
        Assert.Equal(810, count);
    }

    /// <summary>독립 검사에도 실제 입력 생성자 주소를 공급한다. 원본 관측 기대값은 변경하지 않는다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition)
    {
        // 현재 관리 공장의 188칸 하네스와 CD의 알려진 생성자 주소만 사용한다. CD 게임 전체 적재 지원과 구분한다.
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[1].ConstructorAddress = OriginalConstructors.Address(edition, 1);
        types[7].ConstructorAddress = OriginalConstructors.Address(edition, 2);
        types[74].ConstructorAddress = OriginalConstructors.Address(edition, 74);
        return types;
    }

    /// <summary>선택된 Process 접두 부재는 두 할당·상대 WORD를 보존하고 부모에 붙기 전에 실패한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void BindingAndSelectedAttachFailureKeepLinks(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition);
        var other = new RawSidPool(32768, edition: edition);
        OriginalTypeRecord[] types = Types(edition);
        var factory = new RawSquidFactory(pool, types);
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new(), new byte[65536]), types);
        var host = new RawDependFormHost(pool, destroy);
        var number = new UIntBox { Value = 7 };
        Assert.Throws<ArgumentException>(() => host.BindCreation(new RawSquidFactory(other, types), number));
        host.BindCreation(factory, number);
        int watched = factory.Create(74, SidAllocation.Client), watcher = factory.Create(74, SidAllocation.Client);
        uint free = pool.FreeCount;
        Assert.Throws<InvalidOperationException>(() => host.Create(watched, watcher));
        Assert.Equal(free - 2, pool.FreeCount);
        Assert.Equal(7, pool.Slot(7)[10]); Assert.Equal(7, pool.Slot(8)[10]);
        Assert.Equal(8, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(7)[20..]));
        Assert.Equal(7, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(8)[18..]));
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(watched)[6..]));
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(watcher)[6..]));
    }

    /// <summary>독립 공장도 TYPE1의 현재 Gump 생성자를 사용하고 감시 가상표를 덮어쓰지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void IndependentHostUsesCurrentInputConstructor(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition);
        OriginalTypeRecord[] types = Types(edition);
        types[1].ConstructorAddress = OriginalConstructors.Address(edition, 3);
        var factory = new RawSquidFactory(pool, types);
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new(), new byte[65536]), types);
        var host = new RawDependFormHost(pool, destroy);
        int watched = factory.Create(74, SidAllocation.Client), watcher = factory.Create(74, SidAllocation.Client);
        DependFormPair pair = host.Create(watched, watcher);
        uint table = new RawSquidForms(pool).Vtable(3);
        Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(pair.WatcherForm)));
        Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(pair.WatchedForm)));
        Assert.False(host.IsDependForm(pair.WatcherForm));
        Assert.Equal(watcher, RawContainedLink.CommonContainedParent(pool, pair.WatcherForm));
        Assert.Equal(watched, RawContainedLink.CommonContainedParent(pool, pair.WatchedForm));
    }

    /// <summary>실제 World 공장의 첫 진단이 타입 번호를 바꾸면 두 번째 생성과 이전 클래스 삭제가 현재 값을 사용한다.</summary>
    [Fact]
    public void WorldReadsNumberTwiceAndKeepsPreviousClassRoute()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        types[7].ConstructorAddress = 0;
        types[8].ConstructorAddress = OriginalConstructors.Address(1);
        types[70].ConstructorAddress = 0;
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var aux = new RawGameAuxState();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes, gameAux: aux);
        int diagnostics = 0;
        storage.BindDisplayDiagnostics(new(), (expression, _, line) =>
        {
            Assert.Equal("!isForm()", expression); Assert.Equal(0x7d4u, line);
            Assert.Equal(7, storage.Pool.Slot(7)[10]);
            BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(7), 0x512110);
            aux.DependFormType = 8; diagnostics++;
        });
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames);
        RawWorldCore core = world.Core();
        int watched = core.Factory.Create(70, SidAllocation.Client), watcher = core.Factory.Create(70, SidAllocation.Client);
        aux.DependFormType = 7; uint free = storage.Pool.FreeCount;
        DependFormPair pair = core.Dependencies.Create(watched, watcher);
        Assert.Equal(1, diagnostics); Assert.Equal(8u, aux.DependFormType);
        Assert.Equal(7, storage.Pool.Slot(pair.WatcherForm)[10]); Assert.Equal(8, storage.Pool.Slot(pair.WatchedForm)[10]);
        Assert.True(core.Dependencies.IsDependForm(pair.WatcherForm)); Assert.True(core.Dependencies.IsDependForm(pair.WatchedForm));
        Assert.True(core.Dependencies.Has(watched, watcher));
        core.Dependencies.Remove(watcher, watched, 0x10);
        Assert.False(core.Dependencies.Has(watched, watcher)); Assert.Equal(free, storage.Pool.FreeCount);
        Assert.Equal(0u, core.Destroy.PreDepth); Assert.Equal(0u, core.Destroy.PostDepth);
        aux.DependFormType = uint.MaxValue;
        free = storage.Pool.FreeCount;
        Assert.Throws<ArgumentOutOfRangeException>(() => core.Dependencies.Create(watched, watcher));
        Assert.Equal(free - 1, storage.Pool.FreeCount);
        aux.ResetForGeneralWorld(); Assert.Equal(uint.MaxValue, aux.DependFormType);
    }

    /// <summary>Native 검색·가상 삭제·공장 Unpop 바인딩이 같은 풀의 현재 감시 클래스와 깊이를 사용한다.</summary>
    [Fact]
    public void NativeBindingsUseCurrentFinderAndActualCommonDeletion()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types(OriginalEdition.Patch1078);
        types[7].ConstructorAddress = OriginalConstructors.Address(1);
        var factory = new RawSquidFactory(pool, types);
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new(), new byte[65536]), types);
        var host = new RawDependFormHost(pool, destroy);
        host.BindCreation(factory, new() { Value = 7 }, (_, _, _) => throw new InvalidOperationException("예상하지 않은 원문 진단"));
        host.BindUnpop(factory.UnpopForm);
        int watched = factory.Create(74, SidAllocation.Client), watcher = factory.Create(74, SidAllocation.Client);
        int searches = 0, deletions = 0;
        host.BindVirtualDestroy((sid, flags) => { deletions++; destroy.Destroy(sid, flags, host.Hooks()); }, parent => { Assert.Equal(watched, parent); searches++; });
        uint free = pool.FreeCount;
        DependFormPair pair = host.Create(watched, watcher);
        Assert.True(host.Has(watched, watcher));
        Assert.True(host.IsDependForm(pair.WatcherForm));
        host.Remove(watcher, watched, 0x10);
        Assert.False(host.Has(watched, watcher));
        Assert.Equal(3, searches); Assert.True(deletions >= 2);
        Assert.Equal(free, pool.FreeCount); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
    }
}
