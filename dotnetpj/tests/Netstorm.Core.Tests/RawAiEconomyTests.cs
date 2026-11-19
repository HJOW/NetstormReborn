// AI 경제(지갑·SP 정리)의 분기·순서를 실제 메모리·SP 저장소로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 경제를 검사하는 클래스다. 2026-11-19 0.658에서 R4-5 AI 경제용으로 추가했다.
// 근거: cpppj o/RawAiEconomy.cpp의 Reconcile(원문 004138c0)이다.
public sealed class RawAiEconomyTests
{
    /// <summary>본체 필드를 읽는다. 0.658에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.658에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>경제 장면을 만든다. 0.658에서 추가했다.</summary>
    private static (RawAiBrainMemory Memory, uint Token, RawSpStore Store) BuildScene(double now = 10.0)
    {
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        var store = new RawSpStore(new GameRandom(1u), () => 0x12345678u);
        return (memory, token, store);
    }

    /// <summary>기본 갈고리를 만든다. 0.658에서 추가했다.</summary>
    private static AiEconomyHooks BaseHooks(bool demo = false, Func<uint, bool>? captured = null) => new()
    {
        Demo = () => demo,
        Captured = captured ?? (_ => false),
    };

    /// <summary>기한이 되면 지갑을 올리고 SP를 쓴다. 0.658에서 추가했다.</summary>
    [Fact]
    public void ReconcileSupportsWhenDue()
    {
        (RawAiBrainMemory memory, uint token, RawSpStore store) = BuildScene();
        var body = new RawAiEconomy(memory, store, () => 10.0, BaseHooks());
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x264, 2u);
        Set(memory, token, 0x290, 100u);
        Set(memory, token, 0x24c, 500u);
        Set(memory, token, 0x2d8, 0u);
        body.Reconcile(token);
        Assert.Equal(500u, Get(memory, token, 0x2d8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(500.0f), store.Get(1));
        Assert.Equal(BitConverter.DoubleToUInt64Bits(11.0), Get(memory, token, 0x5d8) | ((ulong)Get(memory, token, 0x5dc) << 32));
    }

    /// <summary>기한 전이면 지갑을 건드리지 않고 포획 예산·SP 쓰기만 한다. 0.658에서 추가했다.</summary>
    [Fact]
    public void ReconcileSkipsSupportWhenNotDue()
    {
        (RawAiBrainMemory memory, uint token, RawSpStore store) = BuildScene(now: 5.0);
        var body = new RawAiEconomy(memory, store, () => 5.0, BaseHooks());
        ulong due = BitConverter.DoubleToUInt64Bits(1e15);
        Set(memory, token, 0x5d8, unchecked((uint)due));
        Set(memory, token, 0x5dc, unchecked((uint)(due >> 32)));
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x2d8, 7000u);
        body.Reconcile(token);
        Assert.Equal(7000u, Get(memory, token, 0x2d8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(7000.0f), store.Get(1));
    }

    /// <summary>포획이면 예산까지 보정한다. 0.658에서 추가했다.</summary>
    [Fact]
    public void ReconcileBudgetsCapturedOwner()
    {
        (RawAiBrainMemory memory, uint token, RawSpStore store) = BuildScene();
        var body = new RawAiEconomy(memory, store, () => 10.0, BaseHooks(captured: owner => owner == 2));
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x264, 2u);
        Set(memory, token, 0x290, 100u);
        Set(memory, token, 0x2d8, 0u);
        body.Reconcile(token);
        Assert.Equal(2300u, Get(memory, token, 0x2d8));
    }

    /// <summary>데모에서는 하한 아래로 내리지 않는다. 0.658에서 추가했다.</summary>
    [Fact]
    public void DemoFloorKeepsMinimum()
    {
        (RawAiBrainMemory memory, uint token, RawSpStore store) = BuildScene();
        var body = new RawAiEconomy(memory, store, () => 10.0, BaseHooks(demo: true));
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x264, 2u);
        Set(memory, token, 0x290, 0u);
        Set(memory, token, 0x24c, 0u);
        Set(memory, token, 0x2d8, 0u);
        body.Reconcile(token);
        Assert.Equal(1190u, Get(memory, token, 0x2d8));
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.658에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawAiBrainMemory memory, uint token, RawSpStore store) = BuildScene();
        Assert.Throws<ArgumentNullException>(() => new RawAiEconomy(memory, store, () => 10.0, null!));
        var body = new RawAiEconomy(memory, store, () => 10.0, new AiEconomyHooks());
        Assert.Throws<InvalidOperationException>(() => body.Reconcile(token));
        uint small = memory.Allocate(100);
        var strict = new RawAiEconomy(memory, store, () => 10.0, BaseHooks());
        Assert.Throws<ArgumentException>(() => strict.Reconcile(small));
    }
}
