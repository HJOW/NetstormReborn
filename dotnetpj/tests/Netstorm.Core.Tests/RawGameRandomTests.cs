// 원본 게임 난수·CRT 난수·시각 재시드 몸체의 순서·상태·경계를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 게임 난수 3종을 검증하는 테스트 클래스다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.
public sealed class RawGameRandomTests
{
    /// <summary>0 상태는 첫 호출에서 고정 시드로 바뀌고 상위 16비트 나머지를 쓴다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomReplaysFirstTwoDrawsFromZero()
    {
        var random = new GameRandom(0u);
        Assert.Equal(56u, random.Next(100u));
        Assert.Equal(0x2eb4230au, random.State());
        Assert.Equal(38u, random.Next(100u));
        Assert.Equal(0xaf266921u, random.State());
    }

    /// <summary>상한 0은 원본의 0 나눗셈이므로 거부한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomRejectsZeroLimit()
    {
        var random = new GameRandom(1u);
        Assert.Throws<ArgumentOutOfRangeException>(() => random.Next(0u));
    }

    /// <summary>상태 주입은 다음 호출에 그대로 반영된다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomSetStateFeedsNextDraw()
    {
        var random = new GameRandom();
        random.SetState(55u);
        Assert.Equal(55u, random.State());
        uint first = random.Next(65536u);
        Assert.Equal(random.State(), random.State());
        Assert.True(first < 65536u);
    }

    /// <summary>CRT 초기값 1의 첫 두 값은 41·18467이다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void CrtRandomReplaysFirstTwoDrawsFromOne()
    {
        var random = new CrtRandom(1u);
        Assert.Equal(41u, random.Next());
        Assert.Equal(18467u, random.Next());
    }

    /// <summary>빌림은 외부 상자를 직접 읽고 쓰며 별도 시드를 두지 않는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void CrtRandomBorrowSharesExternalBox()
    {
        var box = new UIntBox { Value = 1u };
        CrtRandom borrowed = CrtRandom.Borrow(box);
        Assert.Equal(41u, borrowed.Next());
        Assert.Equal(2745024u, box.Value);
        borrowed.SetState(0u);
        Assert.Equal(0u, box.Value);
        Assert.Equal(0u, borrowed.State());
    }

    /// <summary>스레드 공유 난수는 null이 아니며 호출마다 전진한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void CrtThreadRandomIsSharedAndAdvances()
    {
        CrtRandom thread = CrtRandom.Thread();
        Assert.NotNull(thread);
        uint before = thread.State();
        thread.Next();
        Assert.NotEqual(before, thread.State());
    }

    /// <summary>모드 0 초기화는 고정 시드 경로로 4114759656을 기록한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomSeedInitializesFromFixedSeedInModeZero()
    {
        var game = new GameRandom();
        var crt = new UIntBox();
        var mode = new UIntBox { Value = 0u };
        var seed = new RawGameRandomSeed(game, crt, mode, () => throw new InvalidOperationException("모드 0은 시각을 읽지 않는다."));
        Assert.Equal(4114759656u, seed.Initialize());
        Assert.Equal(4114759656u, game.State());
        Assert.Equal(0x9a0a1942u, crt.Value);
    }

    /// <summary>생성자의 null 공급자는 실제 세션 초기화 전에 거부된다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomSeedRejectsMissingProvider()
    {
        var game = new GameRandom();
        var crt = new UIntBox();
        var mode = new UIntBox();
        Assert.Throws<ArgumentNullException>(() => new RawGameRandomSeed(game, crt, mode, null!));
    }

    /// <summary>임시 재시드는 모드를 복원하고 시각 경로 시드를 기록한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomSeedReseedRestoresPreviousMode()
    {
        var game = new GameRandom();
        var crt = new UIntBox();
        var mode = new UIntBox { Value = 7u };
        var seed = new RawGameRandomSeed(game, crt, mode, () => 1234567890);
        uint value = seed.Reseed();
        Assert.Equal(7u, mode.Value);
        Assert.Equal(value, game.State());
    }

    /// <summary>공급자 예외 때도 임시 모드를 외부 상태에 남기지 않는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void GameRandomSeedReseedRestoresModeOnProviderFailure()
    {
        var game = new GameRandom();
        var crt = new UIntBox();
        var mode = new UIntBox { Value = 7u };
        var seed = new RawGameRandomSeed(game, crt, mode, () => throw new InvalidOperationException("시각 실패"));
        Assert.Throws<InvalidOperationException>(() => seed.Reseed());
        Assert.Equal(7u, mode.Value);
    }

    /// <summary>정적 CRT 한 번은 외부 ref 상태를 갱신하고 15비트를 반환한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    [Fact]
    public void NextCrtAdvancesRefState()
    {
        uint state = 1u;
        Assert.Equal(41u, RawGameRandomSeed.NextCrt(ref state));
        Assert.Equal(2745024u, state);
    }
}
