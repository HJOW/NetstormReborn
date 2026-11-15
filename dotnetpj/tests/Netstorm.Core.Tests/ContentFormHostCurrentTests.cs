using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 Content 정렬·필드·재부착의 실제 순서를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj ContentAttachCurrentTests·ContentLifetimeTests·FormTakeTests의 현재 몸체 검사를 옮긴다.</summary>
public sealed class ContentFormHostCurrentTests
{
    /// <summary>해당 판본의 독립 TYPE 입력이다. 정렬이 필요한 genus만 각 검사에서 지정한다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();

    /// <summary>부모·목록·상대 번호의 실제 WORD만 쓴다.</summary>
    private static void Put(RawSidPool pool, int sid, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid)[offset..], value);

    /// <summary>지정 WORD 전체를 읽어 상위BYTE를 보존한다.</summary>
    private static ushort Word(RawSidPool pool, int sid, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(sid)[offset..]);

    /// <summary>독립 사본과 실제 TYPE 대여를 구분하고 통지 뒤 현재 부모 TYPE 오류가 battle0 판정보다 먼저 드러난다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void ActualHostReadsBorrowedTypesAfterNotice(bool current)
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types(OriginalEdition.Patch1078);
        int parent = pool.Allocate(SidAllocation.Client); pool.RawSlot(parent)[10] = 90; pool.RawSlot(parent)[11] = 0;
        int form = new RawSquidForms(pool).Create(6, SidAllocation.Client); Put(pool, form, 18, 85);
        int notices = 0, captured = 0;
        var state = new ContentFormState();
        var host = new RawContentFormHost(pool, types, (_, _, _) => { notices++; types[85].Flags2 = 0x200000; pool.RawSlot(parent)[10] = 255; }, state: state, captured: (_, _) => captured++);
        if (current) host.BindCurrentTypes(types);
        host.BindPop((sid, _) => pool.RawSlot(sid)[11] &= unchecked((byte)~4));
        Exception? failure = Record.Exception(() => host.Attach(form, parent, 0x10));
        if (current) Assert.IsType<ArgumentOutOfRangeException>(failure); else Assert.Null(failure);
        Assert.Equal(1, notices); Assert.Equal(0, captured); Assert.False(state.InBattle); Assert.Equal(form, Word(pool, parent, 6));
    }

    /// <summary>양판본 네 실제 Form getter에서 TYPE/상태 별칭96개를 허용하고 자기 뒤 범위 실패까지 검사한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void FieldAliasesStatesAndWholeChain(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition);
        var forms = new RawSquidForms(pool);
        var host = new RawContentFormHost(pool, Types(edition), (_, _, _) => { });
        int parent = forms.Create(6, SidAllocation.Client), count = 0;
        // 네 생성자 가상표와 여섯 TYPE 별칭·두 상태를 독립 공급한다.
        foreach (uint formType in new uint[] { 1, 2, 3, 6 })
        {
            int form = forms.Create(formType, SidAllocation.Client);
            Put(pool, form, 16, (ushort)parent); Put(pool, form, 20, 0x20); Put(pool, parent, 6, (ushort)form);
            // 직접 필드 몸체가 읽지 않는 TYPE를 바꾼다.
            foreach (byte type in new byte[] { 0, 6, 7, 63, 170, 255 })
                // free/dead/void 필터가 추가되지 않았는지 확인한다.
                foreach (byte state in new byte[] { 4, 255 })
                {
                    pool.RawSlot(form)[10] = type; pool.RawSlot(form)[11] = state; Put(pool, form, 4, 0);
                    Assert.True(host.Robust(form)); Assert.True(host.Linked(form));
                    Put(pool, form, 4, 32768); Assert.False(host.Linked(form)); count++;
                }
        }
        Assert.Equal(48, count);
    }

    /// <summary>WORLD 진단은 현재 함수 사본으로 복귀하고 재확보된 풀의 현재 flags를 읽는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, false)]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, false)]
    [InlineData(OriginalEdition.Cd1072, true)]
    public void RobustReadsCurrentFlagsAfterReturningDiagnostic(OriginalEdition edition, bool keepWorld)
    {
        var pool = new RawSidPool(32768, edition: edition);
        int form = new RawSquidForms(pool).Create(6, SidAllocation.Client); Put(pool, form, 20, 0x80);
        var state = new ContentFormState(); int diagnostics = 0;
        state.Assertion = (expression, file, line) =>
        {
            bool patch = edition == OriginalEdition.Patch1078;
            Assert.Equal("!(getListFlags() & CFF_WORLD)", expression); Assert.Equal(patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", file); Assert.Equal(patch ? 0xbb1u : 0xb68u, line);
            diagnostics++; state.Assertion = null; pool.Reset(32770);
            pool.RawSlot(form)[10] = 255; pool.RawSlot(form)[11] = 255; Put(pool, form, 20, keepWorld ? (ushort)0xa0 : (ushort)0);
        };
        var host = new RawContentFormHost(pool, Types(edition), (_, _, _) => { }, state: state);
        Assert.Equal(keepWorld, host.Robust(form)); Assert.Equal(1, diagnostics); Assert.Equal(32770, pool.Capacity);
    }

    /// <summary>원문 Regular의 count0/음수는 범위 밖 this도 읽지 않으며 전달 float 비트를 그대로 반환한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ZeroNegativeRegularSkipsObjectFields(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition);
        var host = new RawContentFormHost(pool, Types(edition), (_, _, _) => { });
        // 원문 조기 반환 경로의 SID·count·음수 및 음수0 입력 조합이다.
        foreach (int form in new[] { 0, 65535 })
            // count가 양수가 아닌 모든 대표 경계를 공급한다.
            foreach (int count in new[] { 0, -1, int.MinValue })
                // float 비트 재해석으로 음수0까지 보존한다.
                foreach (uint bits in new uint[] { 0x80000000, 0xc0600000 })
                    Assert.Equal(bits, BitConverter.SingleToUInt32Bits(host.Damage(form, count, BitConverter.UInt32BitsToSingle(bits))));
    }

    /// <summary>앞단 진단이 현재 dead 상태와 전역을 고치면 실제 링크·Pop·부모 통지까지 계속한다.</summary>
    [Fact]
    public void DeadDiagnosticsResumeAtCurrentSlotsAndKeepDwordViews()
    {
        var pool = new RawSidPool(32768); var forms = new RawSquidForms(pool);
        int parent = forms.Create(6, SidAllocation.Client), form = forms.Create(6, SidAllocation.Client);
        Put(pool, form, 18, 85); pool.RawSlot(parent)[11] |= 2; pool.RawSlot(form)[11] |= 2;
        uint checking = 0; bool battle = false; var events = new List<uint>(); var state = new ContentFormState();
        state.CheckingLinksFlag.Bind(() => checking); state.InBattleFlag.Bind(() => battle);
        state.Assertion = (expression, file, line) =>
        {
            Assert.Equal(@"\Ns\O\Squid.cpp", file); events.Add(line);
            if (line == 0xbe6) { Assert.Equal("!into.isDead() && !into.isFree()", expression); pool.RawSlot(parent)[11] &= unchecked((byte)~2); }
            else { Assert.Equal(0xbe7u, line); Assert.Equal("!isDead() && !isFree()", expression); pool.RawSlot(form)[11] &= unchecked((byte)~2); checking = 0x80000000; battle = true; }
        };
        var host = new RawContentFormHost(pool, Types(OriginalEdition.Patch1078), (owner, child, reason) => { Assert.Equal(parent, owner); Assert.Equal(form, child); events.Add(reason); }, state: state);
        host.Attach(form, parent, 0x10);
        Assert.Equal(new uint[] { 0xbe6, 0xbe7, 1 }, events); Assert.True(host.Linked(form));
        Assert.Equal(0x80000000u, state.CheckingLinksFlag.Read()); Assert.True(state.InBattle);
        checking = 0; battle = false; Assert.False(state.CheckingLinks); Assert.False(state.InBattle);
    }

    /// <summary>진단 복귀에도 dead가 남으면 링크·순서·payload를 쓰지 않으며 CD는 진단 없이 반환한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void UnrepairedDeadReturnsWithoutLinkWrites(OriginalEdition edition)
    {
        // 부모·자신·둘 다 dead인 세 가지 원문 입력을 공급한다.
        foreach (int bits in new[] { 1, 2, 3 })
        {
            var pool = new RawSidPool(32768, edition: edition); var forms = new RawSquidForms(pool);
            int parent = forms.Create(6, SidAllocation.Client), form = forms.Create(6, SidAllocation.Client);
            Put(pool, form, 18, 85); if ((bits & 1) != 0) pool.RawSlot(parent)[11] |= 2; if ((bits & 2) != 0) pool.RawSlot(form)[11] |= 2;
            byte[] before = pool.Bytes.ToArray(); int assertions = 0;
            var state = new ContentFormState { Assertion = (_, _, _) => assertions++ };
            var host = new RawContentFormHost(pool, Types(edition), (_, _, _) => throw new InvalidOperationException("dead 앞단에서 부모 통지"), state: state);
            host.Attach(form, parent, 0x10);
            Assert.Equal(edition == OriginalEdition.Patch1078 ? bits == 3 ? 2 : 1 : 0, assertions);
            Assert.True(pool.Bytes.SequenceEqual(before));
        }
    }

    /// <summary>Unpop이 바꾼 부모 대신 선저장 부모를 사용하고 현재 가상 부착과 flags 쓰기의 부분 효과를 보존한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ReattachUsesCurrentClassCachedParentAndWritePrefix(OriginalEdition edition)
    {
        // Content 재부착·Gump 전환·선택 장치 실패·미연결 가상표를 각각 검사한다.
        foreach (int profile in new[] { 0, 1, 2, 3 })
        {
            var pool = new RawSidPool(32768, edition: edition); var forms = new RawSquidForms(pool, (_, _) => { });
            int parent = forms.Create(6, SidAllocation.Client), other = forms.Create(6, SidAllocation.Client), form = forms.Create(6, SidAllocation.Client);
            var types = Types(edition); var host = new RawContentFormHost(pool, types, (_, _, _) => { });
            host.BindPop(forms.Pop); Put(pool, form, 18, 75); Put(pool, form, 20, 0x120); host.Attach(form, parent, 0x10); pool.RawSlot(form)[10] = 7;
            host.BindUnpop(sid => { forms.Unpop(sid); Put(pool, form, 16, (ushort)other); Put(pool, form, 20, 0xab34); if (profile is 1 or 3) forms.ConstructVtableAt(3, form); });
            int reattached = 0;
            if (profile != 3) host.BindReattach((sid, into, flags) =>
            {
                Assert.Equal(form, sid); Assert.Equal(parent, into); Assert.Equal(0x10u, flags); Assert.Equal(0xaa34, Word(pool, form, 20)); reattached++;
                if (profile == 2) throw new InvalidOperationException("명시 재부착 장치 실패");
                if (profile == 1) { forms.BindAttach(new() { Genus = type => types[(int)type].Flags2, ContentType = () => 6 }); forms.Attach(sid, into, flags); }
                else host.Attach(sid, into, flags);
            });
            Exception? failure = Record.Exception(() => Assert.Equal(0f, host.Damage(form, 1, 9f)));
            Assert.Equal(profile >= 2, failure is not null); Assert.Equal(profile == 3 ? 0 : 1, reattached);
            Assert.Equal(0xaa34, Word(pool, form, 20)); Assert.Equal(profile >= 2 ? other : parent, Word(pool, form, 16));
            Assert.Equal(profile >= 2 ? 0 : form, Word(pool, parent, 6)); Assert.Equal(profile >= 2 ? 4 : 0, pool.Slot(form)[11] & 4);
        }
    }

    /// <summary>contained 생성 중의 표면 통지가 바꾼 현재 재부착 장치를 마지막 위치에서 선택하고 앞선 필드 쓰기를 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void BindCreateSelectsCurrentReattachAfterCreationEffects(bool withBinding)
    {
        var pool = new RawSidPool(32768); OriginalTypeRecord[] types = Types(OriginalEdition.Patch1078);
        types[6].ConstructorAddress = OriginalConstructors.Address(6);
        var factory = new RawSquidFactory(pool, types); factory.BindFormPopDepth(new() { Value = 17 });
        var forms = new RawSquidForms(pool); int parent = forms.Create(1, SidAllocation.Client);
        var host = new RawContentFormHost(pool, types, (_, _, _) => throw new InvalidOperationException("변환된 Gump를 Content로 부착함"));
        int created = 0, reattached = 0;
        ContainedCreateHooks hooks = host.BindCreate(factory, new()
        {
            NotifySurface = sid =>
            {
                created = sid; forms.ConstructVtableAt(3, sid); pool.RawSlot(sid)[10] = 7;
                if (withBinding) host.BindReattach((current, into, flags) => { Assert.Equal(sid, current); Assert.Equal(parent, into); reattached++; factory.AttachForm(current, into, flags); });
            },
        });
        // 명시 입력에서만 bridge 역할을6으로 두어 생성 뒤 표면 통지 경계를 선택한다.
        var creator = new RawContainedCreate(pool, new() { BridgeType = 6 }, hooks);
        Exception? failure = Record.Exception(() => creator.Create(parent, 100, 0xabcd, 0x20, 0x10, 2));
        if (withBinding) Assert.Null(failure); else Assert.IsType<InvalidOperationException>(failure);
        Assert.NotEqual(0, created); Assert.Equal(withBinding ? 1 : 0, reattached);
        Assert.Equal(100, Word(pool, created, 18)); Assert.Equal(0x20, Word(pool, created, 20)); Assert.Equal(2, pool.Slot(created)[30]);
        Assert.Equal(withBinding ? created : 0, Word(pool, parent, 6)); Assert.Equal(withBinding ? 8 : 4, pool.Slot(created)[11] & 12);
    }
}
