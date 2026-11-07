// 게임 시계의 공개 호출열을 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 게임 시계를 검증하는 테스트 클래스다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.
public sealed class RawGameClockTests
{
    /// <summary>실제 double 반환 비트를 Native little endian 출력과 같은 16자리 hex로 표현하는 정적 보조 함수다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="value">변환할 배정밀도 값이다.</param>
    private static string DoubleHex(double value) =>
        BitConverter.ToString(BitConverter.GetBytes(value)).Replace("-", string.Empty).ToLowerInvariant();

    /// <summary>독립 Native 공개 114행을 C#과 비교한다. raw 24행은 C++ 객체 배치 증거라 개수만 센다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    [Fact]
    public void GameClockReplaysNativePublicSequence()
    {
        string[][] rows = X86Fixture.ReadFile("game-clock-reset-x86.tsv");
        int counted = 0;
        int rawRows = 0;
        int publicRows = 0;
        int resets = 0;
        string key = string.Empty;
        GameClock? clock = null;
        // 두 정밀도의 모든 원문행을 빠짐없이 분류하는 반복문이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length);
            Assert.True(row[0] == "639" || row[0] == "895");
            if (row[1].StartsWith("raw", StringComparison.Ordinal))
            {
                Assert.Equal("Reset", row[6]);
                Assert.Equal("0", row[11]);
                ++rawRows;
                ++counted;
                continue;
            }

            string current = row[0] + ":" + row[1];
            if (current != key)
            {
                key = current;
                clock = new GameClock(uint.Parse(row[2], CultureInfo.InvariantCulture));
                Assert.Equal("0", row[5]);
            }

            Assert.NotNull(clock);
            uint now = uint.Parse(row[7], CultureInfo.InvariantCulture);
            string result = "-";
            if (row[6] == "Game")
            {
                result = DoubleHex(clock.GameSeconds(now));
            }
            else if (row[6] == "Wall")
            {
                result = DoubleHex(clock.WallSeconds(now));
            }
            else if (row[6] == "IsPaused")
            {
                result = clock.IsPaused() ? "1" : "0";
            }
            else if (row[6] == "Pause")
            {
                clock.Pause(now);
            }
            else if (row[6] == "Resume")
            {
                clock.Resume(now);
            }
            else if (row[6] == "Reset")
            {
                clock.Reset();
                ++resets;
                Assert.Equal("0", row[11]);
            }
            else
            {
                throw new InvalidOperationException("게임 시계 미해석 명령: " + row[6]);
            }

            Assert.Equal(row[8], result);
            ++publicRows;
            ++counted;
        }

        Assert.Equal(138, counted);
        Assert.Equal(24, rawRows);
        Assert.Equal(114, publicRows);
        Assert.Equal(22, resets);
    }

    /// <summary>누적 정지만 지워 현재 프레임의 번호·이전 게임 기준과 원점을 유지하며 즉시 캡처하지 않는다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    [Fact]
    public void ResetPreservesOriginAndCapturedFrameHistory()
    {
        var clock = new GameClock(1000u);
        FrameTime first = clock.Capture(2000u);
        Assert.Equal(1.0, first.Game);
        Assert.Equal(1.0, first.Wall);
        Assert.Equal(1.0, first.Delta);
        Assert.Equal(1ul, first.Number);
        clock.Pause(2000u);
        clock.Resume(5000u);
        FrameTime previous = clock.Capture(6000u);
        Assert.Equal(2.0, previous.Game);
        Assert.Equal(5.0, previous.Wall);
        Assert.Equal(1.0, previous.Delta);
        Assert.Equal(2ul, previous.Number);
        clock.Reset();
        Assert.False(clock.IsPaused());
        Assert.Equal(5.0, clock.GameSeconds(6000u));
        Assert.Equal(5.0, clock.WallSeconds(6000u));
        FrameTime next = clock.Capture(6000u);
        Assert.Equal(5.0, next.Game);
        Assert.Equal(5.0, next.Wall);
        Assert.Equal(3.0, next.Delta);
        Assert.Equal(3ul, next.Number);
        clock.Reset();
        FrameTime repeated = clock.Capture(6000u);
        Assert.Equal(5.0, repeated.Game);
        Assert.Equal(5.0, repeated.Wall);
        Assert.Equal(0.0, repeated.Delta);
        Assert.Equal(4ul, repeated.Number);
    }

    /// <summary>중첩 정지와 저장 게임 시각은 유지하며 마지막 재개만 원래 계산을 수행한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    [Fact]
    public void ResetKeepsNestedPauseAndHeldGameTime()
    {
        var clock = new GameClock(1000u);
        clock.Pause(2000u);
        clock.Pause(2500u);
        clock.Reset();
        Assert.True(clock.IsPaused());
        Assert.Equal(1.0, clock.GameSeconds(8000u));
        clock.Resume(9000u);
        Assert.True(clock.IsPaused());
        Assert.Equal(1.0, clock.GameSeconds(10000u));
        clock.Reset();
        clock.Resume(11000u);
        Assert.False(clock.IsPaused());
        Assert.Equal(2.0, clock.GameSeconds(12000u));
        Assert.Equal(11.0, clock.WallSeconds(12000u));
        FrameTime first = clock.Capture(12000u);
        Assert.Equal(2.0, first.Game);
        Assert.Equal(2.0, first.Delta);
        Assert.Equal(1ul, first.Number);
        clock.Reset();
        FrameTime next = clock.Capture(12000u);
        Assert.Equal(11.0, next.Game);
        Assert.Equal(9.0, next.Delta);
        Assert.Equal(2ul, next.Number);
    }

    /// <summary>밀리초 DWORD가 감겨도 Reset은 같은 원점과 마지막 캡처 기준을 유지하고 누적 정지만 지운다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    [Fact]
    public void ResetPreservesWrappedOriginAndNonzeroFrameHistory()
    {
        const uint origin = 0xfffffe00u;
        var clock = new GameClock(origin);
        uint At(uint elapsed) => unchecked(origin + elapsed);
        FrameTime first = clock.Capture(At(1000u));
        Assert.Equal(1.0, first.Game);
        Assert.Equal(1.0, first.Wall);
        Assert.Equal(1.0, first.Delta);
        Assert.Equal(1ul, first.Number);
        clock.Pause(At(1000u));
        clock.Pause(At(1500u));
        clock.Resume(At(2000u));
        Assert.True(clock.IsPaused());
        clock.Resume(At(3000u));
        FrameTime previous = clock.Capture(At(4000u));
        Assert.Equal(2.0, previous.Game);
        Assert.Equal(4.0, previous.Wall);
        Assert.Equal(1.0, previous.Delta);
        Assert.Equal(2ul, previous.Number);
        clock.Reset();
        Assert.False(clock.IsPaused());
        Assert.Equal(4.0, clock.GameSeconds(At(4000u)));
        Assert.Equal(4.0, clock.WallSeconds(At(4000u)));
        FrameTime next = clock.Capture(At(4000u));
        Assert.Equal(4.0, next.Game);
        Assert.Equal(4.0, next.Wall);
        Assert.Equal(2.0, next.Delta);
        Assert.Equal(3ul, next.Number);
    }

    /// <summary>묶은 보정 DWORD를 매 게임 시각 조회에 직접 반영한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundSynchronizationTicksShiftGameSeconds()
    {
        var clock = new GameClock(1000u);
        Assert.Equal(1.0, clock.GameSeconds(2000u));
        uint ticks = 500u;
        clock.BindSynchronizationTicks(() => ticks);
        Assert.Equal(0.5, clock.GameSeconds(2000u));
        ticks = 0u;
        Assert.Equal(1.0, clock.GameSeconds(2000u));
        Assert.Throws<ArgumentNullException>(() => clock.BindSynchronizationTicks(null!));
    }
}
