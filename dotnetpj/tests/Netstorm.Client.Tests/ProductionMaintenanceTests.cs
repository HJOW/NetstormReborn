using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 같은 실제 내용물/패널/장기 시계와 원본 유지 결과를 비교한다.

/// <summary>독립86행의 전체 필드와 개발용 패턴 덮어쓰기 경계를 검사한다.</summary>
public sealed class ProductionMaintenanceTests
{
    /// <summary>현재 double의 실제8바이트를 기록한다.</summary>
    private static string DoubleHex(double value) { Span<byte> raw = stackalloc byte[8]; BinaryPrimitives.WriteUInt64LittleEndian(raw, BitConverter.DoubleToUInt64Bits(value)); return Convert.ToHexStringLower(raw); }
    /// <summary>명시 입력의 같은 물리 슬롯에 비정렬WORD를 쓴다.</summary>
    private static void Word(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt16LittleEndian(raw[offset..], unchecked((ushort)value));
    /// <summary>기대값은 재계산하지 않고 원본의 전체150바이트/188시계/패널/효과를 비교한다.</summary>
    [Fact]
    public void NativeAll86ContentsClocksPanelAndEffects()
    {
        int count = 0;
        // 명시 입력 슬롯과 double 비트만 복원한다.
        foreach (var row in Rows("production-idle"))
        {
            Assert.Equal(6, row.Length); ulong[] values = row[0].Split(',').Select(value => value.StartsWith('-') ? unchecked((ulong)long.Parse(value, CultureInfo.InvariantCulture)) : ulong.Parse(value, CultureInfo.InvariantCulture)).ToArray(); Assert.Equal(27, values.Length);
            uint Number(int index) => unchecked((uint)values[index]); // 원문 DWORD의 하위 비트다.
            double Real(int index) => BitConverter.UInt64BitsToDouble(values[index]); // 원문 double 비트다.
            var pool = new RawSidPool(65535, true);
            // 독립 관측이 공급한 SID5~8의 물리 입력을 복원하며 제품 쓰기 관문은 유지한다.
            for (int sid = 5; sid <= 8; sid++) Raw(pool, sid).Clear();
            var parent = Raw(pool, 5); Word(parent, 6, 6); parent[10] = 152; parent[34] = 1; uint[] kinds = [82, 128, 133];
            // 실제 원문 type/next/kind/flags/패턴/수량만 준비한다.
            for (int index = 0; index < 3; index++) { var raw = Raw(pool, 6 + index); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x1240f000); Word(raw, 4, index == 2 ? 0u : (uint)(7 + index)); raw[10] = 6; Word(raw, 18, kinds[index]); Word(raw, 20, index == 0 ? Number(20) : 8); Word(raw, 12, index == 0 ? Number(22) : 0); raw[30] = unchecked((byte)Number(index == 0 ? 19 : 23 + index)); }
            BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool, 6)[31..], Number(21)); var globals = new ProductionMaintenanceGlobals { StockEnabled = Number(11), StockLimit = unchecked((int)Number(12)), QuickBridges = Number(14), ForcedPattern = Number(15), Sequence = Number(16) }; globals.StockUntil.AddRange(new double[188]); globals.StockUntil[128] = globals.StockUntil[133] = Real(26);
            var panel = new ProductionMaintenancePanel { BridgeUntil = Real(10), OldMoney = unchecked((int)Number(17)), Slots = unchecked((int)Number(5)), Temple = Number(4) };
            var reads = new ProductionMaintenanceReads { Active = () => Number(0) is 1 or 2, Technology = () => Number(2), TemplateTechnology = () => Number(2), Temple = () => Number(3), Editor = () => Number(1), GeneratorTimer = () => Number(7), StockRate = () => Number(13), Artificial = _ => Number(6) != 0 }; List<string> effects = [];
            var hooks = new ProductionMaintenanceHooks { WallSeconds = () => Real(8), GameSeconds = () => Real(9), Money = () => F(Number(18)), TouchDeck = () => { }, Pattern = () => { effects.Add("R:10000"); return (uint)CanonDecoder.SelectBridgePattern(1234); }, Create = (sid, kind, argument, flags, attach, quality) => effects.Add($"C:{sid},{kind},{argument},{flags},{attach},{quality}"), Destroy = sid => effects.Add("D:" + sid), Allowed = kind => ((Number(23) >> (kind == 133 ? 1 : 0)) & 1) != 0, Cost = kind => kind == 128 ? 400 : 200, Refresh = mask => effects.Add("U:" + mask) };
            new RawProductionMaintenance(pool, 188, new(), globals, panel, reads, hooks).Run();
            Assert.Equal(row[1], DoubleHex(panel.BridgeUntil) + ',' + unchecked((uint)panel.OldMoney) + ',' + panel.Temple); Assert.Equal(row[2], Convert.ToHexStringLower(pool.Slot(6)) + Convert.ToHexStringLower(pool.Slot(7)) + Convert.ToHexStringLower(pool.Slot(8))); Assert.Equal(row[3], string.Concat(globals.StockUntil.Select(DoubleHex))); Assert.Equal(row[4], globals.Sequence.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[5], effects.Count == 0 ? "-" : string.Join(';', effects)); count++;
        }
        Assert.Equal(86, count);
    }
    /// <summary>개발용 지정 순서는 원본 난수/순서 소비 뒤 적용하고 잘못된 설정은 원자적으로 거부한다.</summary>
    [Fact]
    public void TestPatternsAreAtomicAndOverrideAfterOriginalRules()
    {
        var globals = new ProductionMaintenanceGlobals(); Assert.Null(globals.NextTestBridgePattern()); globals.ConfigureTestBridgePatterns("3, 0,25"); Assert.Equal(new uint?[] { 3, 0, 25, 3 }, Enumerable.Range(0, 4).Select(_ => globals.NextTestBridgePattern()).ToArray());
        // 잘못된 값은 현재 순서와 소비 횟수를 변경하지 않는다.
        foreach (string invalid in new[] { "26", "-1", "1,", "x", "1,,2" }) { Assert.Throws<ArgumentException>(() => globals.ConfigureTestBridgePatterns(invalid)); Assert.Equal(4ul, globals.TestBridgeDraws); Assert.Equal(new uint[] { 3, 0, 25 }, globals.TestBridgePatterns); }
        globals.ConfigureTestBridgePatterns("off"); Assert.Empty(globals.TestBridgePatterns); Assert.Equal(0ul, globals.TestBridgeDraws); Assert.Null(globals.NextTestBridgePattern());
        var pool = new RawSidPool(65535, true); Raw(pool, 5).Clear(); Raw(pool, 5)[10] = 152; Raw(pool, 5)[34] = 1; globals.ForcedPattern = 12; globals.ConfigureTestBridgePatterns("3,0"); var panel = new ProductionMaintenancePanel { Slots = 1 }; List<uint> supplied = []; int random = 0;
        var reads = new ProductionMaintenanceReads { Active = () => true, Technology = () => 5, TemplateTechnology = () => 5, Temple = () => 1, Editor = () => 0, GeneratorTimer = () => 0, StockRate = () => 0, Artificial = _ => false };
        var hooks = new ProductionMaintenanceHooks { WallSeconds = () => 100, GameSeconds = () => 100, Money = () => 100, TouchDeck = () => { }, Pattern = () => { random++; return 7; }, Create = (_, _, pattern, _, _, _) => supplied.Add(pattern), Destroy = _ => { }, Allowed = _ => true, Cost = _ => 0, Refresh = _ => { } };
        var maintenance = new RawProductionMaintenance(pool, 188, new(), globals, panel, reads, hooks); maintenance.Run(); maintenance.Run(); Assert.Equal(new uint[] { 3, 0 }, supplied); Assert.Equal(2, random); Assert.Equal(2u, globals.Sequence); panel.Slots = 0; maintenance.Run(); Assert.Equal(2ul, globals.TestBridgeDraws); Assert.Equal(2, random); globals.ConfigureTestBridgePatterns("off"); panel.Slots = 1; maintenance.Run(); Assert.Equal(12u, supplied[^1]); Assert.Equal(3, random); Assert.Equal(3u, globals.Sequence);
    }
}
