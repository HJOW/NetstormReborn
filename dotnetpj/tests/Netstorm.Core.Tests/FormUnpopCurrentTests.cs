using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// Form 공통 부모의 실제 관측과 현재 종속 분리를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>진단 복귀/현재 VT/void와 실제 공장·종속 해제의 부분 효과를 검사한다.</summary>
public sealed class FormUnpopCurrentTests
{
    /// <summary>PATCH/CD의 실제 원본 관측을 같은 입력과 외부 진단/getter/Detach 장치로 재생한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, "originals")]
    [InlineData(OriginalEdition.Cd1072, "originalCD")]
    public void CommonParentMatchesIndependentPeObservations(OriginalEdition edition, string name)
    {
        int count = 0;
        // 이전 행 결과를 재사용하지 않고 전체 물리 입력을 매번 초기화한다.
        X86Fixture.CheckRows(X86Fixture.Read("form-unpop-current"), row =>
        {
            if (row[0] != name) return true;
            Assert.Equal(4, row.Length); uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(5, input.Length);
            int sid = (int)input[0]; uint profile = input[4]; var pool = new RawSidPool(32768, true, null, edition);
            int nativeBytes = edition == OriginalEdition.Patch1078 ? 50 : 36;
            // 원본 장치와 같은16개 슬롯의 각 payload를 공급한다.
            for (int index = 0; index < 16; index++)
            {
                Span<byte> raw = pool.RawSlot(index); raw.Fill(0xa5);
                // 관리 풀의 CD 패딩과 원본36BYTE를 구분하고 PE의 실제 stride로 입력을 만든다.
                for (int offset = 0; offset < nativeBytes; offset++) raw[offset] = unchecked((byte)((index * nativeBytes + offset) * 37 + 19));
            }
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x14000000); pool.RawSlot(sid)[10] = (byte)input[1]; pool.RawSlot(sid)[11] = (byte)input[2]; pool.RawSlot(9)[11] = (byte)input[3];
            var events = new List<string>(); var hooks = new FormUnpopHooks
            {
                Assertion = (expression, file, line) =>
                {
                    events.Add($"D{expression}|{file}|{line}");
                    if (profile == 1) { pool.RawSlot(sid)[10] = 6; pool.RawSlot(sid)[11] = 8; BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x14000100); }
                },
                Parent = (current, table) => { Assert.Equal(sid, current); events.Add($"P{table}"); if (profile == 2) pool.RawSlot(9)[11] ^= 1; return 9; },
                Detach = current => { Assert.Equal(sid, current); events.Add($"U{pool.Slot(current)[11]}"); },
            };
            new RawSquidForms(pool, unpop: hooks).Unpop(sid);
            Assert.Equal(row[2], string.Join(';', events)); Assert.Equal(row[3], Convert.ToHexStringLower(pool.Slot(sid)[..nativeBytes]) + ";" + Convert.ToHexStringLower(pool.Slot(9)[..nativeBytes]));
            // CD 관리 어댑터의 원문 밖14BYTE도 입력값 그대로 남아야 한다.
            for (int index = 0; index < 16; index++) Assert.All(pool.Slot(index)[nativeBytes..].ToArray(), value => Assert.Equal(0xa5, value));
            count++;
            return true;
        });
        Assert.Equal(864, count);
    }

    /// <summary>실제 서버 부모와 부착된 Form의 물리 입력을 공급한다.</summary>
    private static (RawSidPool Pool, RawSquidForms Forms, int Parent, int Form) Scene(uint kind = 6)
    {
        var pool = new RawSidPool(32768, true); int parent = pool.Allocate(SidAllocation.Server); pool.RawSlot(parent).Clear(); pool.RawSlot(parent)[10] = 74;
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(parent), OriginalConstructors.BaseVtable);
        var forms = new RawSquidForms(pool); int form = forms.Create(kind, SidAllocation.Client); Attachment(pool, form, parent);
        return (pool, forms, parent, form);
    }

    /// <summary>정렬 부착을 대신 주장하지 않고 이미 부착된 단일 Form의 명시 입력만 만든다.</summary>
    private static void Attachment(RawSidPool pool, int form, int parent)
    {
        pool.RawSlot(form)[11] = 8; BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[4..], 0);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[14..], 0); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[16..], (ushort)parent);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent)[6..], (ushort)form);
    }

    /// <summary>부모 알림 누락은 앞서 완료한 void·링크 제거를 취소하지 않는다.</summary>
    [Fact]
    public void MissingNoticeKeepsCompletedLinkWritesAndPayload()
    {
        var s = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(s.Form)[18..], 0xabcdef01); uint free = s.Pool.FreeCount;
        Assert.Throws<InvalidOperationException>(() => s.Forms.Unpop(s.Form));
        Assert.Equal(4, s.Pool.Slot(s.Form)[11]); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(s.Pool.Slot(s.Parent)[6..]));
        Assert.Equal(0xabcdef01u, BinaryPrimitives.ReadUInt32LittleEndian(s.Pool.Slot(s.Form)[18..])); Assert.Equal(free, s.Pool.FreeCount);
    }

    /// <summary>TYPE 진단 복귀가 고친 현재 VT를 실제 공통 getter와 종속 분리가 사용한다.</summary>
    [Fact]
    public void ReturningDiagnosticUsesRepairedVtableWithoutTypeAliasGuard()
    {
        var s = Scene(3); s.Pool.RawSlot(s.Form)[10] = 70; BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(s.Form), 0xdeadbeef); int diagnostics = 0;
        s.Forms.BindUnpop(new() { Assertion = (expression, file, line) =>
        {
            Assert.Equal("isForm()", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0xa59u, line); diagnostics++;
            s.Pool.RawSlot(s.Form)[10] = 69; s.Forms.ConstructVtableAt(3, s.Form);
        } });
        s.Forms.Unpop(s.Form); Assert.Equal(1, diagnostics); Assert.Equal(4, s.Pool.Slot(s.Form)[11]); Assert.Equal(s.Parent, s.Forms.Parent(s.Form));
    }

    /// <summary>동일 호출은 원문 hooks 값 사본을 유지하고 다음 호출만 재연결된 장치를 선택한다.</summary>
    [Fact]
    public void ReturningAssertionCannotReplaceCurrentParentAndDetachSnapshots()
    {
        var s = Scene(3); int old = 0, next = 0; s.Pool.RawSlot(s.Form)[10] = 70;
        s.Forms.BindUnpop(new()
        {
            Assertion = (_, _, _) => { s.Pool.RawSlot(s.Form)[10] = 3; s.Forms.BindUnpop(new() { Parent = (_, _) => s.Parent, Detach = _ => next++ }); },
            Parent = (sid, table) => { Assert.Equal(s.Form, sid); Assert.Equal(0x512188u, table); Assert.Equal(12, s.Pool.Slot(sid)[11]); return s.Parent; },
            Detach = _ => old++,
        });
        s.Forms.Unpop(s.Form); Assert.Equal(1, old); Assert.Equal(0, next);
        s.Pool.RawSlot(s.Form)[11] = 8; s.Forms.Unpop(s.Form); Assert.Equal(1, next);
    }

    /// <summary>생성자는 두 판본에서 지정 physical SID의 VT DWORD만 쓴다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x512188u)]
    [InlineData(OriginalEdition.Cd1072, 0x505a80u)]
    public void ConstructorWritesOnlyActualVtableAtReservedPhysicalSlot(OriginalEdition edition, uint table)
    {
        var pool = new RawSidPool(32768, true, null, edition); pool.RawSlot(0).Fill(0xa5); byte[] expected = pool.Slot(0).ToArray(); BinaryPrimitives.WriteUInt32LittleEndian(expected, table);
        new RawSquidForms(pool).ConstructVtableAt(3, 0); Assert.Equal(expected, pool.Slot(0).ToArray());
    }

    /// <summary>실제 Factory의 분리 장치와 진단을 소비하고 반납 뒤 독립 기본 종단으로 돌아간다.</summary>
    [Fact]
    public void FactoryUsesCurrentDetachAndReturnsToDefaultWithoutOldNotice()
    {
        var s = Scene(); s.Pool.RawSlot(s.Form)[10] = 7;
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var factory = new RawSquidFactory(s.Pool, types);
        int notices = 0; var state = new SquidContainedUnpopState { ContentFormType = new() { Value = 7 }, GumpFormType = new() { Value = 3 }, UiEvent = new() };
        var detach = new RawSquidContainedUnpop(s.Pool, state, new()
        {
            Parent = sid => RawContainedLink.CommonContainedParent(s.Pool, sid), Previous = sid => RawContainedLink.CommonContainedPrevious(s.Pool, sid),
            SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(s.Pool, sid, previous),
            ParentNotice = (parent, form, reason) => { Assert.Equal(s.Parent, parent); Assert.Equal(s.Form, form); Assert.Equal(2u, reason); Assert.Equal(4, s.Pool.Slot(form)[11]); notices++; },
        });
        factory.BindFormDetach(detach.Detach); factory.UnpopForm(s.Form); Assert.Equal(1, notices);
        factory.BindFormDetach(null); Attachment(s.Pool, s.Form, s.Parent); factory.UnpopForm(s.Form); Assert.Equal(1, notices); Assert.Equal(4, s.Pool.Slot(s.Form)[11]);
    }

    /// <summary>복귀 진단 뒤 free/예약 슬롯도 원문 LinkRaw처럼 contained BYTE만 쓸 수 있다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(5)]
    public void ContainedDetachKeepsPhysicalFreeSlotWrites(int sid)
    {
        var pool = new RawSidPool(32768, true); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 7; pool.RawSlot(sid)[11] = 9; pool.RawSlot(9).Clear();
        var state = new SquidContainedUnpopState { ContentFormType = new() { Value = 7 }, GumpFormType = new() { Value = 3 }, UiEvent = new() }; int notices = 0;
        new RawSquidContainedUnpop(pool, state, new() { Parent = _ => 9, Previous = _ => 0, ParentNotice = (_, _, _) => notices++ }).Detach(sid);
        Assert.Equal(1, pool.Slot(sid)[11]); Assert.Equal(1, notices);
    }
}
