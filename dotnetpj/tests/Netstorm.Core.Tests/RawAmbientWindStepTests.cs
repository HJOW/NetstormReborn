// 공유 바람 진행의 감쇠·잡음·각도 저장을 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 바람 진행을 검증하는 테스트 클래스다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.
public sealed class RawAmbientWindStepTests
{
    /// <summary>잡음에 들지 않으면 감쇠 두 저장만 하고 각도와 두 번째 난수를 보존한다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    [Fact]
    public void StepWithoutNoiseOnlyDampsAndPreservesAngle()
    {
        var state = new AmbientWindState { X = 1.0f, Y = 2.0f, Angle = 3.0f };
        var random = new GameRandom(0u);
        RawAmbientWind.Step(state, random, OriginalEdition.Patch1078);
        Assert.Equal((float)(1.0 * 0.9), state.X);
        Assert.Equal((float)(2.0 * 0.9), state.Y);
        Assert.Equal(3.0f, state.Angle);
        Assert.Equal(0x2eb4230au, random.State());
    }

    /// <summary>null 바람·난수는 실제 호출 전에 거부한다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    [Fact]
    public void StepRejectsMissingStateAndRandom()
    {
        var random = new GameRandom(1u);
        var state = new AmbientWindState();
        Assert.Throws<ArgumentNullException>(() => RawAmbientWind.Step(null!, random, OriginalEdition.Patch1078));
        Assert.Throws<ArgumentNullException>(() => RawAmbientWind.Step(state, null!, OriginalEdition.Patch1078));
    }

    /// <summary>양판본·현재값·0재시드·장기 감쇠·난수 소비 1920행을 같은 물리 비트로 비교한다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    [Fact]
    public void AmbientWindReplaysNativeCpuSequencesAndSharedRandom()
    {
        string[][] rows = X86Fixture.ReadFile("ambient-wind-hardware-x86.tsv");
        int checkedRows = 0;
        // 각 행은 독립 초기 입력이며 행 안에서는 같은 바람·난수를 이어 소비하는 반복문이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(14, row.Length);
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            var state = new AmbientWindState
            {
                X = BitConverter.UInt32BitsToSingle(uint.Parse(row[4], CultureInfo.InvariantCulture)),
                Y = BitConverter.UInt32BitsToSingle(uint.Parse(row[5], CultureInfo.InvariantCulture)),
                Angle = BitConverter.UInt32BitsToSingle(uint.Parse(row[6], CultureInfo.InvariantCulture)),
            };
            var random = new GameRandom(uint.Parse(row[2], CultureInfo.InvariantCulture));
            uint checking = uint.Parse(row[7], CultureInfo.InvariantCulture);
            string events = string.Empty;
            var diagnostics = new BoardAngleDiagnostics
            {
                Checking = new RawCheckingFlag(() => checking),
                Assertion = (expression, file, line) =>
                {
                    if (events.Length > 0)
                    {
                        events += "|";
                    }

                    events += line.ToString(CultureInfo.InvariantCulture) + ":" + expression + ":" + file;
                },
            };
            uint steps = uint.Parse(row[3], CultureInfo.InvariantCulture);
            // 매 프레임 원본처럼 감쇠·같은 난수·선택 각도를 진행하며 시각을 따로 적분하지 않는 반복문이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.
            for (uint step = 0u; step < steps; ++step)
            {
                RawAmbientWind.Step(state, random, edition, diagnostics);
            }

            Assert.Equal(uint.Parse(row[8], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(state.X));
            Assert.Equal(uint.Parse(row[9], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(state.Y));
            Assert.Equal(uint.Parse(row[10], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(state.Angle));
            Assert.Equal(uint.Parse(row[11], CultureInfo.InvariantCulture), random.State());
            Assert.Equal(uint.Parse(row[12], CultureInfo.InvariantCulture), checking);
            Assert.Equal(row[13], events.Length == 0 ? "-" : events);
            ++checkedRows;
        }

        Assert.Equal(1920, checkedRows);
    }
}
