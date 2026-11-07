// 원문 CRT 나머지·현재 검사·판본별 끝각 반환을 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 보드 각도를 검증하는 테스트 클래스다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.
public sealed class RawBoardAngleTests
{
    /// <summary>0 입력 꼬리는 부호와 무관하게 +0을 반환한다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    [Fact]
    public void ZeroAngleReturnsPositiveZero()
    {
        Assert.Equal(0.0, RawBoardAngle.Normalize(0.0f, OriginalEdition.Patch1078));
        Assert.Equal(0.0, RawBoardAngle.Normalize(-0.0f, OriginalEdition.Cd1072));
    }

    /// <summary>실제 기계어 1088행을 판본·나머지·검사·진단까지 그대로 재생한다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    [Fact]
    public void BoardAngleReplaysNativeRealFmodAndCurrentGate()
    {
        string[][] rows = X86Fixture.ReadFile("board-angle-hardware-x86.tsv");
        int checkedRows = 0;
        // 각 행의 독립 입력으로 원문 몸체만 실행하고 기대값과 대조하는 반복문이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(8, row.Length);
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            float angle = BitConverter.UInt32BitsToSingle(uint.Parse(row[1], CultureInfo.InvariantCulture));
            double modulus = double.Parse(row[2], CultureInfo.InvariantCulture);
            uint checking = uint.Parse(row[3], CultureInfo.InvariantCulture);
            uint action = uint.Parse(row[4], CultureInfo.InvariantCulture);
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
                    if (action != 0u)
                    {
                        checking = 0u;
                    }
                },
            };
            double result = RawBoardAngle.Normalize(angle, edition, diagnostics, modulus);
            string actual = BitConverter.ToString(BitConverter.GetBytes(result)).Replace("-", string.Empty).ToLowerInvariant();
            Assert.Equal(row[5], actual);
            Assert.Equal(uint.Parse(row[6], CultureInfo.InvariantCulture), checking);
            Assert.Equal(row[7], events.Length == 0 ? "-" : events);
            ++checkedRows;
        }

        Assert.Equal(1088, checkedRows);
    }

    /// <summary>실제 저장소가 만든 같은 현재 뷰는 생성 뒤 검사 변경과 진단 복귀를 그대로 전달한다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentViewUsesLiveCheckingAndOriginalDiagnostic()
    {
        int checking = 0;
        int assertions = 0;
        var diagnostics = new BoardAngleDiagnostics
        {
            Checking = new RawCheckingFlag(() => unchecked((uint)checking)),
            Assertion = (expression, file, line) =>
            {
                Assert.Equal("angle < 361.0f", expression);
                Assert.Equal("\\Ns\\O\\Boardcoord.cpp", file);
                Assert.Equal(0xb3u, line);
                ++assertions;
                checking = 0;
            },
        };
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, diagnostics, 720.0));
        Assert.Equal(0, assertions);
        checking = unchecked((int)0x80000000u);
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, diagnostics, 720.0));
        Assert.Equal(1, assertions);
        Assert.Equal(0, checking);
        checking = 1;
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Cd1072, diagnostics, 720.0));
        Assert.Equal(1, assertions);
        Assert.Equal(1, checking);
    }

    /// <summary>장치 누락은 원문 진단이 선택될 때만 실패하며 CD는 장치를 요구하지 않는다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingSelectedServiceHasExactBoundary()
    {
        var diagnostics = new BoardAngleDiagnostics { Checking = new RawCheckingFlag(1u) };
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(360.5f, OriginalEdition.Patch1078, diagnostics, 720.0));
        Assert.Throws<InvalidOperationException>(() => RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, diagnostics, 720.0));
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Cd1072, diagnostics, 720.0));
    }
}
