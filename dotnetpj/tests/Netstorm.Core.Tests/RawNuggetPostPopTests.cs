using Netstorm.Core.Original;
using Netstorm.Tests;

// 너겟 최초 부모의 실제 조회/두 확보/난수/생성/부모를 원문288행과 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 원문 기대값과 현재 도달 위치의 누락/실패 부분 효과를 검사한다.</summary>
public sealed class RawNuggetPostPopTests
{
    /// <summary>원문에 공급한 네 쌍의 명시 난수 반환 비트다. 분기를 계산하지 않는다.</summary>
    private static readonly uint[][] RandomInputs = [[0x3dcccccd, 0x3e4ccccd], [0, 0], [0x3e99999a, 0x3f000000], [0x7fc01234, 0x3e800000]];
    /// <summary>호출 인자의 원문 DWORD를 순서대로 기록한다.</summary>
    private static void Event(List<string> trace, string name, params uint[] values) => trace.Add(name + ":" + string.Join(':', values));
    /// <summary>288행 전체를 같은 실제 PATCH 모듈로 실행하고 기계어 두 기대 trace를 그대로 대조한다.</summary>
    [Fact]
    public void NativeAllocationsRandomAndFallback288()
    {
        var pool = new RawSidPool(24000); int count = 0;
        // 원문 flags/기존예약/두할당/난수 입력만 읽고 결과는 fixture에서 비교한다.
        foreach (string[] fields in X86Fixture.Read("nugget-postpop"))
        {
            Assert.Equal(3, fields.Length); uint[] row = fields[0].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(4, row.Length);
            uint allocations = 0, randoms = 0; var trace = new List<string>();
            var body = new RawNuggetPostPop(pool, new()
            {
                HasRegular = (sid, value) => { Event(trace, "H", (uint)sid, value); return row[1] != 0; },
                Allocate = () => { Event(trace, "A", 40); return ((row[2] >> (int)allocations++) & 1) != 0; },
                Random = (low, high) => { Event(trace, "R", BitConverter.SingleToUInt32Bits(low), BitConverter.SingleToUInt32Bits(high)); return BitConverter.UInt32BitsToSingle(RandomInputs[row[3]][randoms++]); },
                Construct = (sid, value, payload) => Event(trace, "C", value, (uint)sid, BitConverter.SingleToUInt32Bits(payload)),
                Damageable = (sid, flags) => Event(trace, "D", (uint)sid, flags),
            });
            body.PostPop(50, row[0]); Assert.Equal(fields[1], string.Join(';', trace)); Assert.Equal(fields[1], fields[2]); count++;
        }
        Assert.Equal(288, count);
    }
    /// <summary>미최초는 모든 예약/난수/생성 장치와 SID 물리를 읽지 않는다.</summary>
    [Fact]
    public void NoFirstSkipsUnboundFrontAndSlot()
    {
        var pool = new RawSidPool(24000); var events = new List<string>();
        var body = new RawNuggetPostPop(pool, new() { Damageable = (sid, flags) => { Assert.Equal(50, sid); Assert.Equal(4u, flags); events.Add("D"); } });
        body.PostPop(50, 4); Assert.Equal(new[] { "D" }, events);
    }
    /// <summary>두 확보가 실패하면 난수/생성자 없이 직접 부모를 계속한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void IndependentFailedAllocationsSkipRandomAndConstruct(bool existing)
    {
        var events = new List<string>(); var body = new RawNuggetPostPop(new(24000), new()
        {
            HasRegular = (_, value) => { Assert.Equal(1u, value); events.Add("H"); return existing; }, Allocate = () => { events.Add("A"); return false; }, Damageable = (_, _) => events.Add("D"),
        });
        body.PostPop(50, 1); Assert.Equal(existing ? new[] { "H", "A", "D" } : new[] { "H", "A", "A", "D" }, events);
    }
    /// <summary>선택한 난수 또는 생성자 누락은 앞선 조회/확보/난수 효과를 유지한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void MissingRandomOrConstructorKeepsPrefix(bool random)
    {
        var events = new List<string>(); var hooks = new NuggetPostPopHooks { HasRegular = (_, _) => { events.Add("H"); return false; }, Allocate = () => { events.Add("A"); return true; } };
        if (random) hooks.Random = (_, _) => { events.Add("R"); return 0.25f; };
        var body = new RawNuggetPostPop(new(24000), hooks); Assert.Throws<InvalidOperationException>(() => body.PostPop(50, 1)); Assert.Equal(random ? new[] { "H", "A", "R" } : new[] { "H", "A" }, events);
    }
    /// <summary>외부 함수 묶음 변경은 이미 구성한 모듈의 원문 함수 값 사본을 바꾸지 않는다.</summary>
    [Fact]
    public void HookFunctionsAreCopiedByValue()
    {
        int calls = 0; var hooks = new NuggetPostPopHooks { Damageable = (_, _) => calls++ }; var body = new RawNuggetPostPop(new(24000), hooks); hooks.Damageable = null; body.PostPop(50, 0); Assert.Equal(1, calls);
    }
}
