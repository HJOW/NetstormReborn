using Netstorm.Core.Original;

// 같은시계객체에생성자값을대입하는Client시작prefix를검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원점/보정참조/중첩정지/내부프레임의생성자대입을대조한다.</summary>
public sealed class GameClockInitializationTests
{
    /// <summary>같은시계참조를유지하며이전보정과정지/프레임을새생성자값으로대입한다.</summary>
    [Fact]
    public void InitializeAssignsConstructorStateInPlace()
    {
        var ticks = new UIntBox { Value = 10 }; var clock = new GameClock(100); GameClock alias = clock;
        clock.BindSynchronizationTicks(() => ticks.Value, ticks); _ = clock.Capture(500); clock.Pause(600); clock.Pause(700); Assert.True(clock.IsPaused());
        clock.Initialize(1000); Assert.Same(alias, clock); Assert.False(clock.IsPaused()); ticks.Value = 777;
        FrameTime frame = clock.Capture(1125); Assert.Equal(0.125, frame.Game); Assert.Equal(0.125, frame.Wall); Assert.Equal(0.125, frame.Delta); Assert.Equal(1ul, frame.Number);
    }
    /// <summary>원문unsigned32원점감김을대입후에도유지하고Reset은생성자대입과구별한다.</summary>
    [Fact]
    public void InitializeKeepsUnsignedOriginArithmeticAndResetContract()
    {
        var clock = new GameClock(); clock.Initialize(uint.MaxValue - 10); FrameTime first = clock.Capture(5); Assert.Equal(0.016, first.Game); Assert.Equal(1ul, first.Number);
        clock.Pause(10); clock.Reset(); Assert.True(clock.IsPaused()); FrameTime second = clock.Capture(20); Assert.Equal(0.021, second.Game); Assert.Equal(2ul, second.Number);
    }
}
