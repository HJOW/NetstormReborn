using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원문 Create 부모와 실제 공장의 현재 생성자/장기 진단을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>전체 DWORD 토큰/실제 반환 객체·저장VT와 앞선 할당 효과를 비교한다.</summary>
public sealed class SquidCreationParentTests
{
    /// <summary>독립 원본 장치의 실제 객체 버퍼 토큰이다.</summary>
    private const uint ObjectBase = 0x13000000;
    /// <summary>독립 장치의 가상 표 토큰이다.</summary>
    private const uint Table = 0x14000000;
    /// <summary>명시 cdecl 장치의 주소 토큰이다.</summary>
    private const uint Constructor = 0x10001000;

    /// <summary>588개 양판본 원문 진단/읽기/쓰기·반환 객체와 전체 payload를 대조한다.</summary>
    [Fact]
    public void CreationOrderAndReturnedObjectsMatchIndependentPe()
    {
        string[][] rows = X86Fixture.Read("squid-creation-parent"); Assert.Equal(588, rows.Length); int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length); uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(5, input.Length);
            bool patch = row[0] == "originals"; Assert.True(patch || row[0] == "originalCD"); int width = patch ? 50 : 36;
            uint type = input[0], flags = input[1], sid = input[2], profile = input[3], constructor = input[4] != 0 ? 0 : Constructor;
            byte[] bytes = new byte[128 * width]; var trace = new List<string>();
            // 기대값과 독립된 원문 입력 패턴을 전체 실제 stride로 공급한다.
            for (int index = 0; index < bytes.Length; index++) bytes[index] = unchecked((byte)(index * 37 + 19));
            // 호스트 포인터나 SID로 좁히지 않고 외부 버퍼의 물리 토큰만 해석한다.
            int Offset(uint token) { Assert.InRange(token, ObjectBase, ObjectBase + (uint)bytes.Length - (uint)width); return checked((int)(token - ObjectBase)); }
            var hooks = new RawSquidCreationHooks
            {
                FakeSurfaceType = () => 162,
                Assertion = (expression, file, line) => trace.Add($"D{expression}|{file}|{line}"),
                Allocate = value => { trace.Add($"A{value}"); if (profile == 1) constructor = Constructor + 0x10; return sid; },
                Constructor = number => { trace.Add($"T{number}:{constructor}"); return constructor; },
                Construct = (address, number) =>
                {
                    if (address == 0)
                    {
                        uint returned = unchecked(ObjectBase + number * (uint)width), table = patch ? 0x501dd0u : 0x5058d8u;
                        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(Offset(returned)), table); trace.Add($"B{returned}:{table}"); return returned;
                    }
                    trace.Add($"C{address}:{number}"); uint token = ObjectBase + (uint)((profile == 1 ? 9 : 5) * width);
                    BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(Offset(token)), Table); return token;
                },
                Vtable = token => { uint table = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(Offset(token))); trace.Add($"V{token}:{table}"); return table; },
                StoreType = (token, value) => { trace.Add($"W{token}:{value}"); bytes[Offset(token) + 10] = value; if (profile == 2) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(Offset(token)), Table + 0x100); },
                PostCreate = (token, table) => trace.Add($"P{token}:{table}"),
            };
            uint returnedObject = RawSquidCreation.Create(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, type, flags, hooks);
            Assert.Equal(X86Fixture.UInt(row[2]), returnedObject); Assert.Equal(row[3], string.Join(';', trace)); Assert.Equal(row[4], Convert.ToHexStringLower(bytes.AsSpan(Offset(returnedObject), width))); count++; return true;
        });
        Assert.Equal(588, count);
    }

    /// <summary>실제 공장이 빌릴 현재 물리 TYPE 표다.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

    /// <summary>미연결 생성자/반환 객체 접근은 앞선 할당을 취소하거나 TYPE 쓰기를 앞당기지 않는다.</summary>
    [Fact]
    public void SelectedMissingDevicesPreserveEarlierAllocationAndVtableRead()
    {
        string trace = ""; var hooks = new RawSquidCreationHooks
        {
            FakeSurfaceType = () => 162, Allocate = _ => { trace += "A"; return 5; }, Constructor = _ => { trace += "T"; return 123; },
        };
        Assert.Throws<InvalidOperationException>(() => RawSquidCreation.Create(OriginalEdition.Patch1078, 74, 0, hooks)); Assert.Equal("AT", trace);
        hooks.Construct = (_, _) => { trace += "C"; return 0; }; hooks.Vtable = token => { Assert.Equal(0u, token); trace += "V"; throw new InvalidOperationException("명시 null 객체 입력"); };
        hooks.StoreType = (_, _) => trace += "W";
        // 양판본 모두 TYPE 저장 전에 실제 반환 객체의 VT를 읽는다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            trace = ""; Assert.Throws<InvalidOperationException>(() => RawSquidCreation.Create(edition, 74, 0, hooks)); Assert.Equal("ATCV", trace);
        }
    }

    /// <summary>fake 진단은 할당 전에 복귀하고 그 뒤 현재 생성자를 다시 선택한다.</summary>
    [Fact]
    public void FactoryReturningFakeDiagnosisChangesCurrentConstructorBeforeAllocation()
    {
        var pool = new RawSidPool(32768); var types = Types(); var factory = new RawSquidFactory(pool, types); var fake = new UIntBox { Value = 74 }; factory.BindFakeSurfaceType(fake); uint free = pool.FreeCount; int assertions = 0;
        factory.BindDiagnostics(new() { Types = types, Assertion = (expression, file, line) =>
        {
            Assert.Equal("type != fakeThreeByThreeSurfaceType", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x189u, line); Assert.Equal(free, pool.FreeCount);
            assertions++; types[74].ConstructorAddress = OriginalConstructors.Address(3);
        } });
        int sid = factory.Create(74); Assert.Equal(1, assertions); Assert.Equal(free - 1, pool.FreeCount); Assert.Equal(0x512188u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid))); Assert.Equal(74, pool.Slot(sid)[10]);
        fake.Value = 75; Assert.Equal(sid + 1, factory.Create(74)); Assert.Equal(1, assertions);
    }

    /// <summary>할당 장치의 실제 현재 효과 뒤 생성자를 조회하고 잘못된 생성자/TYPE는 이미 할당한 SID를 되돌리지 않는다.</summary>
    [Fact]
    public void FactoryLooksUpConstructorAfterActualAllocationAndRetainsFailedAllocation()
    {
        var types = Types(); bool armed = false;
        var pool = new RawSidPool(32768, sharedServer: () => { if (armed) types[74].ConstructorAddress = OriginalConstructors.Address(6); return true; });
        var factory = new RawSquidFactory(pool, types); armed = true; int sid = factory.Create(74); Assert.Equal(0x512208u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid))); armed = false;
        uint free = pool.FreeCount; types[76].ConstructorAddress = 0x12345678;
        Assert.Throws<InvalidOperationException>(() => factory.Create(76)); Assert.Equal(free - 1, pool.FreeCount); Assert.Equal(0, pool.Slot(sid + 1)[10]); Assert.Equal(4, pool.Slot(sid + 1)[11]);
        Assert.Throws<ArgumentOutOfRangeException>(() => factory.Create(188)); Assert.Equal(free - 2, pool.FreeCount); Assert.Equal(0, pool.Slot(sid + 2)[10]);
    }

    /// <summary>네Form/자산별칭 및 Contained/Gump도 현재TYPE 생성자와 실제VT의 빈 초기화를 사용한다.</summary>
    [Fact]
    public void ActualFormsAndConvenienceCreationUseCurrentConstructorAndRealVtable()
    {
        // 원문 양판본 가상표를 같은 관리풀의 실제 물리 prefix에서 비교한다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            var pool = new RawSidPool(32768, edition: edition); var types = Types(); var factory = new RawSquidFactory(pool, types); int z = edition == OriginalEdition.Patch1078 ? 35 : 33;
            // 현재 생성자가 선택한 네 실제Form VT는 자산HP/zOrder를 쓰지 않는다.
            foreach (uint number in new uint[] { 1, 2, 3, 6, 74 })
            {
                uint form = number == 74 ? 6 : number; types[number].ConstructorAddress = OriginalConstructors.Address(edition, (int)form); types[number].ZOrder = 79; types[number].Flags1 = 16; types[number].MaxHitPoints = 999;
                // CD 관리풀의 서버영역 차이를 선택하지 않는 실제 client 할당 입력이다.
                int sid = factory.Create(number, SidAllocation.Client); Assert.Equal((byte)number, pool.Slot(sid)[10]); Assert.Equal(0, pool.Slot(sid)[z]); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)[26..]));
            }
            types[6].ConstructorAddress = OriginalConstructors.Address(edition, 3); types[3].ConstructorAddress = OriginalConstructors.Address(edition, 1);
            int contained = factory.CreateContained(2), gump = factory.CreateGumpForm(2); var forms = new RawSquidForms(pool);
            forms.ConstructVtableAt(3, 16000); Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(16000)), BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(contained)));
            forms.ConstructVtableAt(1, 16000); Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(16000)), BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(gump)));
            Assert.Equal(6, pool.Slot(contained)[10]); Assert.Equal(3, pool.Slot(gump)[10]);
        }
    }

    /// <summary>같은 장기 GameAux의 fake TYPE 주소를 실제World 공장이 빌리며 일반 준비 뒤에도 보존한다.</summary>
    [Fact]
    public void ActualWorldFactoryUsesLongLivedFakeTypeAcrossWorldOwners()
    {
        var aux = new RawGameAuxState(); aux.FakeSurfaceTypeBox.Value = 74; int assertions = 0;
        // 장기GameAux는 공유하지만 현재 관리구조의 World별Storage/Factory임을 구별한다.
        for (int generation = 0; generation < 2; generation++)
        {
            var types = Types(); var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToList(); var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToList();
            var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes, gameAux: aux);
            storage.BindDisplayDiagnostics(new StringBox(), (expression, file, line) => { Assert.Equal("type != fakeThreeByThreeSurfaceType", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x189u, line); assertions++; });
            var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames); world.Core().Factory.Create(74);
            Assert.Equal(generation + 1, assertions); aux.ResetForGeneralWorld(); Assert.Equal(74u, aux.FakeSurfaceTypeBox.Value);
        }
    }
}
