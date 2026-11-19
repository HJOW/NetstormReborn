// AI 전체 프레임(전체행동·개별틱·방문·소멸)의 순서·수명을 실제 메모리로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 전체 프레임을 검사하는 클래스다. 2026-11-19 0.647에서 R4-5 AI 런타임 결선용으로 추가했다.
// 근거: cpppj o/RawAiRuntime.cpp의 Actions·Tick·Frame(원문 00417c70·00417d30·00417de0)이다.
public sealed class RawAiRuntimeTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.647에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>본체 필드를 읽는다. 0.647에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.647에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>런타임 장면을 만든다. 0.647에서 추가했다.</summary>
    private static (RawAiBrainMemory Memory, uint Token, byte[] Players) BuildScene()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        return (memory, token, new byte[9 * 180]);
    }

    /// <summary>기본 전역을 만든다. 0.647에서 추가했다.</summary>
    private static AiRuntimeGlobals BaseGlobals(bool battle = true, uint timing = 0u, uint dryLog = 29u) => new()
    {
        Battle = () => battle,
        Timing = () => timing,
        DryLog = () => dryLog,
    };

    /// <summary>호출 순서를 기록하는 기본 갈고리를 만든다. 0.647에서 추가했다.</summary>
    private static AiRuntimeHooks StrictHooks(List<string> calls) => new()
    {
        Initialize = _ => calls.Add("initialize"),
        Prepare = _ => calls.Add("prepare"),
        SelectGoal = _ => calls.Add("selectGoal"),
        Collectors = _ => calls.Add("collectors"),
        Factories = _ => calls.Add("factories"),
        DryScan = _ => 0u,
        DryNotice = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Priest = (_, _) => calls.Add("priest"),
        Drops = _ => calls.Add("drops"),
        Bridges = _ => calls.Add("bridges"),
        Carriers = _ => calls.Add("carriers"),
        Economy = _ => calls.Add("economy"),
        Money = _ => 0.0f,
        Wall = () => 0.0,
        Took = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Assertion = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Destruct = _ => throw new InvalidOperationException("호출되면 안 된다"),
        Free = _ => throw new InvalidOperationException("호출되면 안 된다"),
    };

    /// <summary>전투가 아니면 아무 brain도 방문하지 않는다. 0.647에서 추가했다.</summary>
    [Fact]
    public void FrameSkipsWhenNotBattle()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var calls = new List<string>();
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(battle: false), StrictHooks(calls));
        BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(1 * 180 + 0xb0, 4), token);
        runtime.Frame();
        Assert.Empty(calls);
    }

    /// <summary>brain이 없으면 아무 행동도 하지 않는다. 0.647에서 추가했다.</summary>
    [Fact]
    public void FrameSkipsEmptyOwners()
    {
        (RawAiBrainMemory memory, _, byte[] players) = BuildScene();
        var calls = new List<string>();
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), StrictHooks(calls));
        runtime.Frame();
        Assert.Empty(calls);
    }

    /// <summary>254가 서 있으면 소멸 대상이며 brain 기록은 지운다. 0.647에서 추가했다.</summary>
    [Fact]
    public void FrameDestructsFinishedBrain()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var calls = new List<string>();
        var hooks = StrictHooks(calls);
        var destructed = new List<uint>();
        hooks.Destruct = brain => destructed.Add(brain);
        hooks.Free = brain => destructed.Add(brain + 1);
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), hooks);
        Set(memory, token, 0x254, 1u);
        BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(2 * 180 + 0xb0, 4), token);
        runtime.Frame();
        Assert.Equal([token, token + 1], destructed);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(players.AsSpan(2 * 180 + 0xb0, 4)));
        Assert.Empty(calls);
    }

    /// <summary>258이 없으면 초기화만 하고 본문을 유지한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void TickInitializesOnlyWhenIncomplete()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var calls = new List<string>();
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), StrictHooks(calls));
        Assert.True(runtime.Tick(token));
        Assert.Equal(["initialize"], calls);
    }

    /// <summary>전체 틱은 준비→목표→행동 순서로 실행한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void TickRunsPrepareGoalAndActions()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var calls = new List<string>();
        var hooks = StrictHooks(calls);
        hooks.Money = _ => 100.0f;
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), hooks);
        Set(memory, token, 0x258, 1u);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x2d8, 100u);
        Set(memory, token, 0x29c, 7u);
        Assert.True(runtime.Tick(token));
        Assert.Equal(["prepare", "selectGoal", "collectors", "factories", "priest", "drops", "bridges", "carriers", "economy"], calls);
    }

    /// <summary>제거가 있으면 로그 종류와 fake 판정을 통지한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void ActionsNotifiesDryRemoval()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var calls = new List<string>();
        var hooks = StrictHooks(calls);
        var noticed = new List<(uint Kind, bool Fake)>();
        hooks.DryScan = _ => 5u;
        hooks.DryNotice = (_, kind, fake) => noticed.Add((kind, fake));
        hooks.Money = _ => 0.0f;
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(dryLog: 29u), hooks);
        Set(memory, token, 0x258, 1u);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x2d8, 0u);
        Set(memory, token, 0x280, 5u);
        runtime.Actions(token);
        Assert.Equal([(29u, true)], noticed);
    }

    /// <summary>지갑이 다르면 진단을 남기고 계속한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void ActionsAssertsWalletMismatch()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var asserted = new List<string>();
        var hooks = StrictHooks([]);
        hooks.Money = _ => 50.0f;
        hooks.Assertion = (text, line) => asserted.Add(text + ":" + line);
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x2d8, 100u);
        runtime.Actions(token);
        Assert.Equal(["money == getMoney( playerId ):4168"], asserted);
    }

    /// <summary>느린 전체 틱은 실행시간을 통지한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void TickLogsSlowFrame()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        var took = new List<double>();
        var hooks = StrictHooks([]);
        hooks.Money = _ => 0.0f;
        double now = 100.0;
        hooks.Wall = () => now += 0.2;
        hooks.Took = (_, elapsed) => took.Add(elapsed);
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(timing: 1u), hooks);
        Set(memory, token, 0x258, 1u);
        Set(memory, token, 0x260, 1u);
        Assert.True(runtime.Tick(token));
        Assert.Single(took);
        Assert.True(took[0] > 0.1);
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.647에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawAiBrainMemory memory, uint token, byte[] players) = BuildScene();
        Assert.Throws<ArgumentException>(() => new RawAiRuntime(memory, new byte[10], BaseGlobals(), StrictHooks([])));
        Assert.Throws<ArgumentNullException>(() => new RawAiRuntime(memory, players, BaseGlobals(), null!));
        var runtime = new RawAiRuntime(memory, players, BaseGlobals(), new AiRuntimeHooks());
        Assert.Throws<InvalidOperationException>(() => runtime.Actions(token));
        var noBattleGlobals = BaseGlobals();
        noBattleGlobals.Battle = null;
        var noBattleRuntime = new RawAiRuntime(memory, players, noBattleGlobals, StrictHooks([]));
        Assert.Throws<InvalidOperationException>(() => noBattleRuntime.Frame());
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => runtime.Tick(small));
    }
}
