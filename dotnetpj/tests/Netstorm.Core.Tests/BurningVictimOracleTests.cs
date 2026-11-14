using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원문 두SID진단과 세연소앞단의 현재조회/선택슬롯/복귀를 저장관찰로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>상세SID1012입력과 연소대상504입력의 원문 대조다.</summary>
public sealed class BurningVictimOracleTests
{
    /// <summary>진단 장치가 중단하는 명시 입력이며 제품의 예외를 성공으로 바꾸지 않는다.</summary>
    private sealed class AssertionFailure : Exception { }
    /// <summary>원문 두DWORD의 현재 비트를 그대로 문자열로 보존한다.</summary>
    private static string State(RawSidRobustDiagnostics state) => $"{state.LastRobust}:{state.Type}";
    /// <summary>같은 원문 번호/주소 부모를 모든 입력과 진단 전후 trace로 비교한다.</summary>
    [Fact]
    public void SidDiagnosticsMatchAllOriginalInputsAndCurrentReads()
    {
        string[][] rows = X86Fixture.Read("sid-robust-parent"); Assert.Equal(1012, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            long input = X86Fixture.Long(row[1]); int live = X86Fixture.Int(row[2]);
            uint checking = X86Fixture.UInt(row[3]), start = X86Fixture.UInt(row[4]), capacity = X86Fixture.UInt(row[5]);
            uint pointer = row[0] == "object" ? unchecked((uint)input) : unchecked(start + (uint)input * 50);
            byte[] raw = new byte[50]; BinaryPrimitives.WriteUInt32LittleEndian(raw, X86Fixture.UInt(row[6])); raw[11] = (byte)X86Fixture.UInt(row[7]); raw[10] = (byte)X86Fixture.UInt(row[8]);
            uint mode = X86Fixture.UInt(row[9]); var diagnostics = new RawSidRobustDiagnostics { LastRobust = 0xaaaa5555, Type = 0x55bbdd66 }; var events = new List<string>();
            var hooks = new SidRobustHooks
            {
                PoolBase = () => { events.Add($"P:{start}:{State(diagnostics)}"); return start; },
                Capacity = () => { events.Add($"C:{capacity}:{State(diagnostics)}"); return capacity; },
                Checking = () => { events.Add($"K:{checking}:{State(diagnostics)}"); return checking; },
                Memory = (address, width) =>
                {
                    events.Add($"M:{address}:{width}:{State(diagnostics)}");
                    if (address < pointer || address - pointer > (ulong)raw.Length || (ulong)width > (ulong)raw.Length - (address - pointer)) throw new ArgumentOutOfRangeException(nameof(address));
                    return raw.AsMemory((int)(address - pointer), width);
                },
                Assertion = (expression, line) =>
                {
                    Assert.Equal("resolve( sid ).isRobust()", expression); Assert.Equal(0x218u, line); events.Add($"A:{line}:{State(diagnostics)}");
                    if (mode == 1) throw new AssertionFailure();
                    if (mode == 2) { diagnostics.LastRobust = 0x11223344; diagnostics.Type = 0x55667788; checking = 0; start = 0x14010000 + 50; capacity = 7; }
                },
            };
            bool result = false; string outcome = "ok";
            try { result = row[0] == "object" ? RawSidRobust.SquidIsRobust(pointer, live, diagnostics, hooks) : RawSidRobust.IsSidRobust(unchecked((int)input), live, diagnostics, hooks); }
            catch (AssertionFailure) { outcome = "assertion"; }
            Assert.Equal(row[10], outcome); Assert.Equal(row[11] != "0", result); Assert.Equal(X86Fixture.UInt(row[12]), diagnostics.LastRobust); Assert.Equal(X86Fixture.UInt(row[13]), diagnostics.Type);
            Assert.Equal(X86Fixture.UInt(row[14]), checking); Assert.Equal(X86Fixture.UInt(row[15]), start); Assert.Equal(X86Fixture.UInt(row[16]), capacity);
            Assert.Equal(row[17], events.Count == 0 ? "-" : string.Join(';', events)); return true;
        });
    }
    /// <summary>원문 식/파일의 ASCII바이트를 fixture표기로 보존한다.</summary>
    private static string Hex(string value) => Convert.ToHexStringLower(Encoding.ASCII.GetBytes(value));
    /// <summary>현재SID가 진단 중 바뀌어도 두번째robust뒤 선택한 슬롯은 보존한다.</summary>
    [Fact]
    public void VictimParentsMatchOriginalCurrentSidAndSelectedSlot()
    {
        string[][] rows = X86Fixture.Read("burning-victim-parent"); Assert.Equal(504, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] p = row[0].Split(',').Select(X86Fixture.UInt).ToArray(); int current = (ushort)p[1]; uint checking = p[2], calls = 0;
            var events = new List<string>(); var hooks = new BurningVictimHooks
            {
                Robust = (sid, live) => { Assert.True(live); bool result = ((p[3] >> (int)Math.Min(calls, 1u)) & 1) != 0; events.Add($"R:{sid}:{(result ? 1 : 0)}"); calls++; if (p[7] == 1 && calls == 1) { current = 1; checking = 0; } return result; },
                Vtable = sid => { events.Add($"V:{sid}:{p[4]}"); return p[4] != 0; },
                HasHitPoints = sid => { events.Add($"H:{sid}:{p[5]}"); if (p[7] == 2) { current = 1; checking = 0; } return p[5] != 0; },
                Assertion = (expression, file, line) => { events.Add($"D:{Hex(expression)}:{Hex(file)}:{line}"); if (p[6] == 0) throw new AssertionFailure(); },
            };
            string outcome = "empty", selected = "-";
            try { int? victim = RawBurningVictim.Run(() => current, (BurningVictimEntry)p[0], new RawCheckingFlag(() => checking), hooks); if (victim is not null) { outcome = "body"; selected = victim.Value.ToString(); } }
            catch (AssertionFailure) { outcome = "assertion"; }
            Assert.Equal(row[1], outcome); Assert.Equal(X86Fixture.UInt(row[2]), checking); Assert.Equal(X86Fixture.Int(row[3]), current); Assert.Equal(row[4], selected); Assert.Equal(row[5], string.Join(';', events)); return true;
        });
    }
    /// <summary>번호/live/checking 미선택은 물리장치와 상세DWORD 초기화를 요구하지 않는다.</summary>
    [Fact]
    public void UnselectedSidChecksPreserveDiagnosticsAndNeverDemandMemory()
    {
        var d = new RawSidRobustDiagnostics { LastRobust = 11, Type = 22 }; var hooks = new SidRobustHooks();
        Assert.False(RawSidRobust.IsSidRobust(-1, 1, d, hooks)); Assert.False(RawSidRobust.IsSidRobust(65535, 0, d, hooks));
        Assert.True(RawSidRobust.IsSidRobust(65534, 0, d, hooks)); hooks.Checking = () => 0; Assert.True(RawSidRobust.IsSidRobust(65534, 1, d, hooks));
        Assert.Equal((11u, 22u), (d.LastRobust, d.Type));
    }
    /// <summary>64비트 호스트주소와 선행TYPE 물리접근을32비트로 좁히지 않는다.</summary>
    [Fact]
    public void HostPointerModeKeepsHighAddressAndTypeReadPrecedesRange()
    {
        const ulong start = 0x123456780000; byte[] raw = new byte[50]; BinaryPrimitives.WriteUInt32LittleEndian(raw, 1); raw[10] = 7;
        var d = new RawSidRobustDiagnostics(); var hooks = new SidRobustHooks { AddressMode = SidRobustAddressMode.HostPointer, PoolBase = () => start, Capacity = () => 64,
            Memory = (address, width) => { Assert.InRange(address, start + 50, start + 99); return raw.AsMemory((int)(address - start - 50), width); } };
        Assert.True(RawSidRobust.SquidIsRobust(start + 50, 1, d, hooks)); Assert.Equal(7u, d.Type);
        hooks.Memory = (_, _) => throw new ArgumentOutOfRangeException("선택 물리 없음");
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSidRobust.SquidIsRobust(0, 1, d, hooks)); Assert.Equal(0u, d.LastRobust); Assert.Equal(7u, d.Type);
    }
}
