using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// Carrier/내용물의 현재 전역·진단 복귀·Native 부모 순서와 원본2880입력을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>공유 권한과 현재 커서·전체WORD/DWORD조건·사제 연결의 회귀 검사다.</summary>
public sealed class CarrierCurrentOracleTests
{
    /// <summary>원본 비교가 관측한 부모와 네 후보SID다.</summary>
    private static readonly int[] Ids = [50, 60, 61, 62, 63];
    /// <summary>정렬되지 않은 원문WORD만 쓴다.</summary>
    private static void Word(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt16LittleEndian(raw[offset..], (ushort)value);
    /// <summary>원본 직접 조회와Damageable후 조회의 반환/현재boss/전체raw/사건을 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void CarrierMatchesNativeCurrentEffects(string name, OriginalEdition edition)
    {
        string[][] rows = X86Fixture.Read("carrierpredestroy", name); Assert.Equal(960, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var state = new ContainedFinderState();
        // 원본과 같은 client 번호까지 확보하고 이후 합성free/dead 입력을 그대로 쓴다.
        for (int sid = 5; sid <= 63; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(16, row.Length); string events = ""; int validations = 0; state.Boss = row[3] != "0"; state.ContainedType = X86Fixture.UInt(row[4]);
            // 지정하지 않은 바이트도 원본 실행기의0xab로 복원한다.
            foreach (int sid in Ids) { Span<byte> raw = pool.RawSlot(sid); raw.Fill(0xab); raw[10] = 158; raw[11] = 0; BinaryPrimitives.WriteUInt32LittleEndian(raw[4..], 0); }
            pool.RawSlot(50)[11] = (byte)X86Fixture.UInt(row[2]); Word(pool.RawSlot(50), 6, X86Fixture.UInt(row[10]));
            // 각 후보의번호/타입/상태/next/kind/mask는입력만구성한다.
            foreach (string node in row[11].Split(';', StringSplitOptions.RemoveEmptyEntries)) { string[] values = node.Split(','); int sid = X86Fixture.Int(values[0]); Span<byte> raw = pool.RawSlot(sid); raw[10] = (byte)X86Fixture.UInt(values[1]); raw[11] = (byte)X86Fixture.UInt(values[2]); Word(raw, 4, X86Fixture.UInt(values[3])); Word(raw, 18, X86Fixture.UInt(values[4])); Word(raw, 20, X86Fixture.UInt(values[5])); }
            var hooks = new CarrierPreDestroyHooks { ValidateDamageable = (_, _) => validations++, DamageablePre = (sid, flags) => { events = $"D:{sid}:{flags}:{(state.Boss ? 1 : 0)}"; if (row[9] != "-") { string[] values = row[9].Split(','); state.Boss = values[0] != "0"; Word(pool.RawSlot(50), 6, X86Fixture.UInt(values[1])); Word(pool.RawSlot(63), 20, X86Fixture.UInt(values[2])); } }, DependentFound = () => events += ";G" }; var body = new RawCarrierPreDestroy(pool, state, hooks);
            if (row[1] == "Q") Assert.Equal(X86Fixture.Int(row[12]), body.FindContained(50, X86Fixture.UInt(row[5]), X86Fixture.UInt(row[6]), X86Fixture.UInt(row[7]))); else body.PreDestroy(50, X86Fixture.UInt(row[8]));
            Assert.Equal(row[13] != "0", state.Boss); Assert.Equal(row[14], events); Assert.Equal(row[1] == "P" ? 1 : 0, validations); string[] expected = row[15].Split('|'); Assert.Equal(Ids.Length, expected.Length);
            // CD의실제36BYTE와.NET의나머지14BYTE입력보존을각각확인한다.
            for (int i = 0; i < Ids.Length; i++) { byte[] native = X86Fixture.Bytes(expected[i]); Assert.Equal(native, pool.Slot(Ids[i])[..native.Length].ToArray()); if (native.Length < 50) Assert.All(pool.Slot(Ids[i])[native.Length..].ToArray(), b => Assert.Equal((byte)0xab, b)); }
            return true;
        });
    }
    /// <summary>Native는typed상태검사를앞당기지않고Damageable뒤현재권한으로잘못된head조회를생략한다.</summary>
    [Fact]
    public void NativeParentRunsBeforeCurrentBossAndFinder()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); pool.RawSlot(sid)[10] = 0; Word(pool.RawSlot(sid), 6, 0xffff); bool boss = true; var state = new ContainedFinderState { Boss = true, SharedBoss = () => boss }; int parents = 0, found = 0; var hooks = new CarrierPreDestroyHooks { ValidateDamageable = (_, _) => throw new InvalidOperationException("Native미선택typed선검사"), DamageablePre = (parent, flags) => { Assert.Equal((sid, 0xdeadbeefu), (parent, flags)); parents++; boss = false; }, DependentFound = () => found++ }; var body = new RawCarrierPreDestroy(pool, state, hooks); Assert.Same(state, body.FinderState); body.EnableNativeBody(); hooks.DamageablePre = null; body.Validate(-1, 0); body.PreDestroy(sid, 0xdeadbeef); Assert.Equal((1, 0), (parents, found)); Assert.False(boss);
    }
    /// <summary>효과뒤공유boss를다시읽으며PATCHdead진단/CD생략·정상복귀후현재head를구별한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void SharedBossAndReturningDiagnosticsUseCurrentHead(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition);
        // 실제 client 할당을 먼저 끝낸 뒤 원본의 합성 raw 입력을 쓴다.
        for (int sid = 5; sid <= 63; sid++) pool.Allocate(SidAllocation.Client);
        bool boss = true, nextBoss = false; uint checking = 0x100; var view = new RawCheckingFlag(() => checking); var state = new ContainedFinderState { Boss = true, SharedBoss = () => boss, CheckingDead = view }; view.Set(false); pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 158; Word(pool.RawSlot(50), 6, 60); pool.RawSlot(60).Clear(); pool.RawSlot(60)[10] = 6; Word(pool.RawSlot(60), 20, 1); int damages = 0, found = 0;
        var body = new RawCarrierPreDestroy(pool, state, new() { ValidateDamageable = (_, _) => { }, DamageablePre = (_, _) => { damages++; boss = nextBoss; }, DependentFound = () => found++ }); body.PreDestroy(50, 0); Assert.Equal((1, 0), (damages, found)); nextBoss = true; body.PreDestroy(50, 0); Assert.Equal((2, 1), (damages, found)); pool.RawSlot(50)[11] = 2; boss = false;
        if (edition == OriginalEdition.Patch1078) Assert.Throws<InvalidOperationException>(() => body.FindContained(50, 1, 0, 0)); else Assert.Equal(60, body.FindContained(50, 1, 0, 0));
        int diagnostics = 0; pool.RawSlot(61).Clear(); pool.RawSlot(61)[10] = 6; Word(pool.RawSlot(61), 20, 1); state.Assertion = (expression, file, line) => { diagnostics++; Assert.Equal("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy", expression); Assert.Equal(@"\Ns\O\Squidfinder.cpp", file); Assert.Equal(0x13cu, line); Word(pool.RawSlot(50), 6, 61); state.Assertion = null; };
        Assert.Equal(edition == OriginalEdition.Patch1078 ? 61 : 60, body.FindContained(50, 1, 0, 0)); Assert.Equal(edition == OriginalEdition.Patch1078 ? 1 : 0, diagnostics); checking = 0; Word(pool.RawSlot(50), 6, 60); Assert.Equal(60, body.FindContained(50, 1, 0, 0));
    }
    /// <summary>저장한next·현재TYPE와kindDWORD/전체WORDmask를같은커서/개수에적용한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentCursorAndCountKeepFullWidthConditions(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var state = new ContainedFinderState(); Word(pool.RawSlot(50), 6, 60); pool.RawSlot(50)[11] = 2;
        // free/dead/void/contained상태는종류가맞는후보를임의제외하지않는다.
        for (int sid = 60; sid <= 62; sid++) { pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 6; pool.RawSlot(sid)[11] = 15; Word(pool.RawSlot(sid), 4, (uint)(sid == 62 ? 0 : sid + 1)); Word(pool.RawSlot(sid), 18, (uint)(sid == 61 ? 7 : 0)); Word(pool.RawSlot(sid), 20, (uint)(sid == 62 ? 0x8000 : 1)); }
        var finder = new RawContainedFinder(pool, state); Assert.Equal(60, finder.Begin(50)); Word(pool.RawSlot(60), 4, 0); pool.RawSlot(61)[10] = 7; state.ContainedType = 7; Assert.Equal(61, finder.Next()); Assert.Equal(0, finder.Next()); Word(pool.RawSlot(60), 4, 61); state.ContainedType = 6; pool.RawSlot(61)[10] = 6;
        Assert.Equal(3, RawContainedFinder.CountContained(pool, state, 50, 0, 0)); Assert.Equal(1, RawContainedFinder.CountContained(pool, state, 50, 0x8000, 0)); Assert.Equal(0, RawContainedFinder.CountContained(pool, state, 50, 0x10000, 0)); Assert.Equal(0, RawContainedFinder.CountContained(pool, state, 50, 0, 0x10000)); Assert.Equal(1, RawContainedFinder.CountContained(pool, state, 50, 1, 7)); state.ContainedType = 0x10000006; Assert.Equal(0, finder.Begin(50));
    }
    /// <summary>사제어댑터는같은실제Carrier검증/부모만바꾸고보호막함수를보존한다.</summary>
    [Fact]
    public void PriestAdapterPreservesShieldAndRequiresSamePool()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); pool.RawSlot(sid)[10] = 158; int validations = 0, parents = 0; var body = new RawCarrierPreDestroy(pool, new() { Boss = false }, new() { ValidateDamageable = (_, _) => validations++, DamageablePre = (_, _) => parents++, DependentFound = () => { } }); var hooks = new PriestPreDestroyHooks { FindForcefield = _ => 0, DestroyForcefield = (_, _) => { } }; var attached = RawCarrierPreDestroy.MakeCarrierPreDestroyHooks(pool, body, hooks); Assert.Null(hooks.ValidateCarrier); Assert.Same(hooks.FindForcefield, attached.FindForcefield); Assert.Same(hooks.DestroyForcefield, attached.DestroyForcefield); attached.ValidateCarrier!(sid, 0); attached.CarrierPre!(sid, 0); Assert.Equal((2, 1), (validations, parents));
        validations = parents = 0; BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x0050f210); var priests = new PriestPostPopState(); priests.Priests.Entries = [(uint)sid, 99, (uint)sid]; priests.Priests.Count = 3; new RawPriestPreDestroy(pool, priests, attached).PreDestroy(sid, 0); Assert.Equal((2, 1), (validations, parents)); Assert.Equal(1u, priests.Priests.Count); Assert.Equal(99u, priests.Priests.Entries[0]);
        Assert.Throws<ArgumentException>(() => RawCarrierPreDestroy.MakeCarrierPreDestroyHooks(new RawSidPool(24000, false), body, hooks)); var cd = new RawCarrierPreDestroy(new RawSidPool(24000, false, edition: OriginalEdition.Cd1072), new(), new() { ValidateDamageable = (_, _) => { }, DamageablePre = (_, _) => { }, DependentFound = () => { } }); Assert.Throws<ArgumentException>(cd.EnableNativeBody);
    }
}
