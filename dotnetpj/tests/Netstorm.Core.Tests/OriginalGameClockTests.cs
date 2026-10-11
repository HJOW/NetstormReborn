using System.Globalization;
using Netstorm.Core.Display;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>원본 PE의 실제 시각 비트와 연속 중첩 정지 상태를 기본 x87 모드에서 대조한다.</summary>
public sealed class OriginalGameClockTests
{
    /// <summary>TSV의 double 비트/큰 DWORD를 문화권과 무관하게 읽는다.</summary>
    private static ulong UInt64(string value) => ulong.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>세 독립 판본의 연속 호출을 재생한다. 몫/나머지·정지 기대값을 C#에서 생성하지 않는다.</summary>
    [Theory]
    [InlineData("originals", 1584)]
    [InlineData("originalCD", 1440)]
    [InlineData("original1037", 1440)]
    public void NativeClock_MatchesAllTimeAndPauseBits(string edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("clockkernel").Where(row => row[0] == edition && row[2] == "639").ToArray();
        Assert.Equal(expectedRows, rows.Length);
        var clocks = new Dictionary<string, OriginalGameClock>();
        // 같은 입력 번호의 정지 상태를 유지하며 실제 관찰 순서대로 모든 반환을 대조한다.
        bool Matches(string[] row)
        {
            if (!clocks.TryGetValue(row[1], out OriginalGameClock? clock))
            {
                clock = new OriginalGameClock(X86Fixture.UInt(row[3]), X86Fixture.UInt(row[4]), X86Fixture.UInt(row[5]));
                clocks.Add(row[1], clock);
            }
            uint now = X86Fixture.UInt(row[7]);
            string value;
            switch (row[6])
            {
                case "Wall": value = BitConverter.DoubleToUInt64Bits(clock.WallSeconds(now)).ToString(CultureInfo.InvariantCulture); break;
                case "Game": value = BitConverter.DoubleToUInt64Bits(clock.GameSeconds(now)).ToString(CultureInfo.InvariantCulture); break;
                case "Pause": clock.Pause(now); value = "-"; break;
                case "Resume": clock.Resume(now); value = "-"; break;
                case "IsPaused": value = clock.IsPaused ? "1" : "0"; break;
                default: throw new InvalidDataException("알 수 없는 원본 시계 연산입니다.");
            }
            return value == row[8] && clock.PauseDepth == X86Fixture.Int(row[9])
                && BitConverter.DoubleToUInt64Bits(clock.HeldGameSeconds) == UInt64(row[10])
                && BitConverter.DoubleToUInt64Bits(clock.PausedTotalSeconds) == UInt64(row[11]);
        }
        X86Fixture.CheckRows(rows, Matches);
    }

    /// <summary>첫 정지의 저장 시각을 중첩 정지에서 보존하고 마지막 재개에서만 누계를 반영한다.</summary>
    [Fact]
    public void NestedPause_PreservesFirstHeldTimeAndResumesOnce()
    {
        var clock = new OriginalGameClock(1000);
        clock.Resume(1000);
        clock.Pause(2000);
        clock.Pause(3000);
        Assert.Equal(2, clock.PauseDepth);
        Assert.Equal(1, clock.GameSeconds(4000));
        Assert.Equal(3, clock.WallSeconds(4000));
        clock.Resume(4000);
        Assert.True(clock.IsPaused);
        Assert.Equal(0, clock.PausedTotalSeconds);
        clock.Resume(5000);
        Assert.False(clock.IsPaused);
        Assert.Equal(3, clock.PausedTotalSeconds);
        Assert.Equal(2, clock.GameSeconds(6000));
        clock.Resume(7000);
        Assert.Equal(3, clock.PausedTotalSeconds);
    }

    /// <summary>ゲーム 보정값은 벽시계에 적용하지 않고 timeGetTime 감김은 DWORD 뺄셈으로 통과한다.</summary>
    [Fact]
    public void AdjustmentAndWrap_KeepClocksSeparate()
    {
        var clock = new OriginalGameClock(0xffffff00, gameAdjustmentMilliseconds: 100);
        Assert.Equal(0.512, clock.WallSeconds(256));
        Assert.Equal(0.412, clock.GameSeconds(256), 12);
        Assert.Equal(0x32c85780u, OriginalGameClock.EpochReferenceSeconds);
    }
}
