using System.Globalization;
using Netstorm.Client.Input;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 원문 사건 처리 중 단축키에 도달한 구간만 실제 큐와 대조한다.

/// <summary>독립8040행의 도달 경계와4280행의 전체 예약/활성 큐를 검사한다.</summary>
public sealed class ProductionShortcutsTests
{
    /// <summary>부호 있는 좌표를 원문 DWORD로 기록한다.</summary>
    private static string Bits(int value) => unchecked((uint)value).ToString(CultureInfo.InvariantCulture);
    /// <summary>명시 입력 사건열을 실제 제품 사건으로 읽는다.</summary>
    private static InputEvent[] Events(string text) => text.Split(';').Select(item => { var values = item.Split(':'); Assert.Equal(3, values.Length); return new InputEvent(U(values[0]), unchecked((int)U(values[1])), unchecked((int)U(values[2]))); }).ToArray();
    /// <summary>기대 예약은 키 매핑을 다시 계산하지 않고 CPU 관측에서 읽는다.</summary>
    [Fact]
    public void Native4280ReachableCasesAnd8040BoundaryClassification()
    {
        int native = 0, compared = 0, gated = 0;
        // 같은 큐의 공개 API만 사용하고 앞선 Guide/Gump 관문은 본체 비교에서 제외한다.
        foreach (var row in Rows("production-shortcuts"))
        {
            Assert.Equal(17, row.Length); native++; Assert.Equal("1", row[9]); Assert.True(U(row[16]) > 0); Assert.Equal(40 * 12 * 2, row[13].Length);
            var events = Events(row[1]); List<int> counts = []; List<string> expected = []; int pausedCalls = 0;
            // 실제 CALL 인자와 사건 경계만 관측한다.
            foreach (string item in row[14].Split(';')) { if (item.StartsWith("guide:", StringComparison.Ordinal)) counts.Add(0); else if (item.StartsWith("paused:", StringComparison.Ordinal)) pausedCalls++; else if (item.StartsWith("queue:", StringComparison.Ordinal)) { Assert.NotEmpty(counts); counts[^1]++; expected.Add(item); } }
            Assert.Equal(events.Length, counts.Count); bool reachable = U(row[2]) == 0 && U(row[3]) == 0 && U(row[5]) == 0 && events.All(item => (item.Code & InputCode.Release) == 0); Assert.Equal(reachable, pausedCalls == events.Length);
            if (!reachable) { gated++; continue; }
            compared++; var queue = new InputQueue(); uint read = U(row[7]), filled = U(row[8]); Assert.True(read < 40 && filled < 40);
            // 빈 큐의 두 커서를 명시 입력 위치까지 함께 전진시킨다.
            for (uint index = 0; index < read; index++) { queue.Push(0x02001000 + index, 0, 0); queue.Pop(false); }
            // 원문이 공급한 기존 활성 슬롯 표식을 같은 공개 API로 준비한다.
            for (uint index = 0; index < filled; index++) { uint physical = (read + index) % 40; queue.Push(0x02000300 + physical, (int)(100 + physical), (int)(200 + physical)); }
            List<string> observed = []; int reads = 0; var hooks = new ProductionShortcutHooks { Command = () => { reads++; return U(row[6]); }, Queue = (command, slot, zero) => { observed.Add("queue:" + command + ':' + Bits(slot) + ':' + Bits(zero)); queue.PushCommand(command, slot, zero); } };
            // 사건 사이 큐를 비우지 않고 원문과 같은 입력 순서를 유지한다.
            for (int index = 0; index < events.Length; index++) Assert.Equal(counts[index] != 0, ProductionShortcuts.Queue(events[index], unchecked((int)U(row[4])) > 0, hooks));
            Assert.Equal(expected, observed); Assert.Equal(counts.Sum(), reads); List<string> active = [];
            // 공개 Pop으로 최대39개 실제 활성 사건만 관측한다.
            while (!queue.Empty) { Assert.True(active.Count < 39); var item = queue.Pop(false); active.Add(item.Code + ":" + Bits(item.X) + ':' + Bits(item.Y)); }
            Assert.Equal(row[12], active.Count == 0 ? "-" : string.Join(';', active));
        }
        Assert.Equal(8040, native); Assert.Equal(4280, compared); Assert.Equal(3760, gated);
    }
    /// <summary>비선택 관문과 매 예약 현재 명령·부분 실패·별도 반복 명령을 확인한다.</summary>
    [Fact]
    public void ReleasePauseLazyDevicesAndCurrentCommands()
    {
        var missing = new ProductionShortcutHooks();
        // 뗌/정지 입력은 명령 조회나 예약 장치를 요구하지 않는다.
        foreach (char key in "qWaSzXyEd") { Assert.False(ProductionShortcuts.Queue(new((uint)key | InputCode.Release, 0, 0), false, missing)); Assert.False(ProductionShortcuts.Queue(new(key, 0, 0), true, missing)); }
        Assert.False(ProductionShortcuts.Queue(new('?', 0, 0), false, missing)); uint command = 0x020000f4; int reads = 0, calls = 0; List<uint> seen = [];
        var partial = new ProductionShortcutHooks { Command = () => { reads++; return command; }, Queue = (code, slot, zero) => { Assert.Equal(calls, slot); Assert.Equal(0, zero); calls++; if (calls == 3) throw new InvalidOperationException("명시 세 번째 예약 중단"); seen.Add(code); command++; } };
        Assert.Throws<InvalidOperationException>(() => ProductionShortcuts.Queue(new('E', 0, 0), false, partial)); Assert.Equal(3, reads); Assert.Equal(3, calls); Assert.Equal(new uint[] { 0x020000f4, 0x020000f5 }, seen);
        uint repeat = 0x02000107; List<uint> repeated = []; var current = new ProductionShortcutHooks { RepeatCommand = () => repeat, Queue = (code, slot, zero) => { Assert.Equal(0, slot); Assert.Equal(0, zero); repeated.Add(code); } }; ProductionShortcuts.Queue(new('D', 0, 0), false, current); repeat++; ProductionShortcuts.Queue(new('d', 0, 0), false, current); Assert.Equal(new uint[] { 0x02000107, 0x02000108 }, repeated);
    }
}
