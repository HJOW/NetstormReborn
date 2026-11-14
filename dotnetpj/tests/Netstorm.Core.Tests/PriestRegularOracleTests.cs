using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 사제 정기384원본과 선택 경계·실제 커널 예약/삭제 재진입을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>사제 정기 분배의 원문 비트와 현재 호스트 수명 검사다.</summary>
public sealed class PriestRegularOracleTests
{
    /// <summary>부모 호출 인자를 원문 little endian 바이트 그대로 기록한다.</summary>
    private static string Bytes(params uint[] words)
    {
        byte[] bytes = new byte[words.Length * 4];
        // 각 DWORD의 원문 저장 폭을 보존한다.
        for (int i = 0; i < words.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(i * 4), words[i]);
        return Convert.ToHexStringLower(bytes);
    }
    /// <summary>고유/부모 후단은 실제 원본 실행기에 넣은 대체 경계이며 기대 비트/사건은 기록에서 읽는다.</summary>
    [Fact]
    public void DispatchMatches384NativeInputs()
    {
        string[][] rows = X86Fixture.Read("priest-regular-dispatch"); Assert.Equal(384, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(6, row.Length); uint ev = X86Fixture.UInt(row[0]), count = X86Fixture.UInt(row[1]), payload = X86Fixture.UInt(row[2]), external = X86Fixture.UInt(row[3]); string trace = "";
            var hooks = new PriestRegularHooks { Destroy = (sid, flags) => { Assert.Equal(60, sid); trace = "destroy:" + Bytes(flags); }, Unique = (sid, code, number, value) => { Assert.Equal((60, ev, count, payload), (sid, code, number, BitConverter.SingleToUInt32Bits(value))); trace = code == 603 ? "fall:" : "ritual:"; return BitConverter.UInt32BitsToSingle(external); }, Carrier = (sid, code, number, value) => { Assert.Equal(60, sid); trace = "carrier:" + Bytes(code, number, BitConverter.SingleToUInt32Bits(value)); return BitConverter.UInt32BitsToSingle(external); } };
            Assert.Equal(X86Fixture.UInt(row[4]), BitConverter.SingleToUInt32Bits(RawPriestRegular.Run(60, ev, count, BitConverter.UInt32BitsToSingle(payload), hooks))); Assert.Equal(row[5], trace); return true;
        });
    }
    /// <summary>601은count의비영만 보고 선택한 경계만 요구하며 모든 인자/NaN payload를 손실 없이 전달한다.</summary>
    [Fact]
    public void OnlySelectedBoundaryIsRequiredAndCountsRemainUnsigned()
    {
        var empty = new PriestRegularHooks(); Assert.Equal(3f, RawPriestRegular.Run(0, 601, 0, float.NaN, empty)); Assert.Throws<InvalidOperationException>(() => RawPriestRegular.Run(60, 601, 0x80000000, 0, empty)); Assert.Throws<InvalidOperationException>(() => RawPriestRegular.Run(60, 602, 0, 0, empty)); Assert.Throws<InvalidOperationException>(() => RawPriestRegular.Run(60, 606, 0, 0, empty));
        uint seen = 0; Assert.Equal(-1f, RawPriestRegular.Run(60, 601, uint.MaxValue, 0, new() { Destroy = (sid, flags) => { Assert.Equal(60, sid); seen = flags; }, Unique = (_, _, _, _) => throw new InvalidOperationException("601미선택고유"), Carrier = (_, _, _, _) => throw new InvalidOperationException("601미선택부모") })); Assert.Equal(0x100000u, seen);
        // 네 고유 사건은count의부호 해석 없이 같은 후단과 원문 payload 비트를 받는다.
        for (uint ev = 602; ev <= 605; ev++)
        {
            uint selected = ev; float result = RawPriestRegular.Run(60, ev, uint.MaxValue, BitConverter.UInt32BitsToSingle(0xffc12345), new() { Unique = (sid, code, count, value) => { Assert.Equal((60, selected, uint.MaxValue, 0xffc12345u), (sid, code, count, BitConverter.SingleToUInt32Bits(value))); return BitConverter.UInt32BitsToSingle(0x80000000); } }); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(result));
        }
    }
    /// <summary>실제 Kernel/Host가601을3초 후 재호출하고 삭제 경계가 현재 Regular를 지우면 후속쓰기를 생략한다.</summary>
    [Fact]
    public void ActualKernelReschedules601AndPreservesDeletionReentry()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var kernel = new Kernel(8); var state = new SquidProcessState { Now = 10 }; var hash = new RawSquidHash(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, hash, new byte[65536]), types); var hooks = new PriestRegularHooks(); var calls = new List<uint>(); var host = new SquidProcessHost(pool, types, kernel, destroy, state, (sid, ev, count, value) => { calls.Add(count); return RawPriestRegular.Run(sid, ev, count, value, hooks); });
        types[2].ConstructorAddress = OriginalConstructors.Address(2); host.BindFormFactory(new RawSquidFactory(pool, types)); int parent = pool.Allocate(); pool.RawSlot(parent).Clear(); pool.RawSlot(parent)[10] = 158; RegularProcess process = host.AddRegular(parent, 601, 0.25f)!; Assert.NotNull(process); uint pid = process.KernelId; int form = process.Form(); Assert.NotEqual(0, form); hooks.Destroy = (sid, flags) => { Assert.Equal((parent, 0x100000u), (sid, flags)); host.Kill(process, flags); };
        kernel.RunFrame(); Assert.Equal([0u], calls); Assert.Equal((1u, 3f, 13d), (process.Count(), process.Payload(), process.Time())); Assert.Same(process, kernel.Get(pid)); state.Now = 12.999; kernel.RunFrame(); Assert.Single(calls); state.Now = 13; kernel.RunFrame(); Assert.Equal([0u, 1u], calls); Assert.Null(kernel.Get(pid)); Assert.Equal((1u, 3f, 13d), (process.Count(), process.Payload(), process.Time())); Assert.Equal(0, kernel.Size()); Assert.NotEqual(0, pool.Slot(form)[11] & RawSidPool.FreeBit); Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent)[6..]));
    }
}
