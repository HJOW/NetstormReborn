using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 TYPE HP의 원본 반환·진단 복귀·물리 표 재대여를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원본1160관찰과 실제 보상 부모를 같은 조회 순서로 검증한다.</summary>
public sealed class RiftHitPointCurrentTests
{
    /// <summary>양판본 원본을 재생해 반환·번호 횟수·진단·효과 뒤 현재 상태를 비교한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void OriginalRowsKeepSelectedRecordAndReadCurrentGlobals(string editionName)
    {
        string[][] rows = X86Fixture.Read("rifttype-hit-points", editionName);
        Assert.Equal(580, rows.Length);
        // 같은 물리 기록과 원문이 지정한 현재 표 이동을 행마다 독립 구성한다.
        foreach (string[] row in rows)
        {
            string[] input = row[1].Split(','); Assert.Equal(9, input.Length);
            var physical = Enumerable.Range(0, 190).Select(_ => new OriginalTypeRecord()).ToArray();
            int number = X86Fixture.Int(input[0]); OriginalTypeRecord selected = physical[1 + number];
            selected.MaxHitPoints = X86Fixture.Int(input[1]); selected.Flags1 = X86Fixture.UInt(input[2]);
            uint checking = X86Fixture.UInt(input[3]), quarter = X86Fixture.UInt(input[4]);
            uint priest = X86Fixture.UInt(input[5]), dais = X86Fixture.UInt(input[6]);
            int shift = X86Fixture.Int(input[7]), profile = X86Fixture.Int(input[8]), calls = 0;
            var diagnostics = new List<string>(); var origin = new ArraySegment<OriginalTypeRecord>(physical, 1, 188);
            int result = RawRiftHitPoints.Read(editionName == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072,
                selected, new()
                {
                    Checking = new RawCheckingFlag(() => checking), Quarter = new RawCheckingFlag(() => quarter),
                    Priest = () => priest, Dais = () => dais,
                }, new()
                {
                    Number = () => { calls++; return RawRiftHitPoints.CurrentRiftTypeNumber(number, origin, new(physical, 1 + shift, 188)); },
                    Assertion = (expression, file, line) =>
                    {
                        diagnostics.Add($"{expression}|{file}|{line}");
                        if (profile != 0)
                        {
                            checking = 0; quarter = 0x80000000;
                            selected.MaxHitPoints = new[] { -13, -37, 41 }[profile - 1];
                            shift = new[] { 0, 1, -1 }[profile - 1]; priest = unchecked((uint)(74 - shift));
                        }
                    },
                });
            Assert.Equal(X86Fixture.Int(row[2]), result);
            Assert.Equal(row[3], diagnostics.Count == 0 ? "-" : string.Join(';', diagnostics));
            Assert.Equal(X86Fixture.Int(row[4]), calls);
            Assert.Equal(row[5], $"{selected.MaxHitPoints},{checking},{quarter},{priest},{dais},{shift}");
        }
    }

    /// <summary>현재 표 뷰를 옮겨도 같은 기록 인덱스를 쓰며 독립 할당과 비중첩 연결을 거부한다.</summary>
    [Fact]
    public void PhysicalViewsKeepNegativeDwordAndRejectOtherAllocations()
    {
        var records = new OriginalTypeRecord[190]; var origin = new ArraySegment<OriginalTypeRecord>(records, 1, 188);
        Assert.Equal(uint.MaxValue, RawRiftHitPoints.CurrentRiftTypeNumber(0, origin, new(records, 2, 188)));
        Assert.Equal(75u, RawRiftHitPoints.CurrentRiftTypeNumber(74, origin, new(records, 0, 188)));
        Assert.Throws<InvalidOperationException>(() => RawRiftHitPoints.CurrentRiftTypeNumber(0, origin, new(new OriginalTypeRecord[190], 1, 188)));
        Assert.Throws<InvalidOperationException>(() => RawRiftHitPoints.CurrentRiftTypeNumber(0, new(records, 0, 1), new(records, 1, 1)));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawRiftHitPoints.CurrentRiftTypeNumber(188, origin, origin));
    }

    /// <summary>CD와 미선택 분기는 번호/진단을 요구하지 않으며 선택된 누락은 그 지점에서 실패한다.</summary>
    [Fact]
    public void OnlySelectedNumberAndDiagnosticAreRequired()
    {
        var type = new OriginalTypeRecord { MaxHitPoints = -7 };
        var globals = new RiftHitPointGlobals { Checking = new(true), Priest = () => 74, Dais = () => 74 };
        Assert.Equal(-7, RawRiftHitPoints.Read(OriginalEdition.Cd1072, type, globals, new()));
        Assert.Throws<InvalidOperationException>(() => RawRiftHitPoints.Read(OriginalEdition.Patch1078, type, globals, new()));
        Assert.Equal(-7, RawRiftHitPoints.Read(OriginalEdition.Patch1078, type, globals, new() { Number = () => 74 }));
        Assert.Throws<InvalidOperationException>(() => RawRiftHitPoints.Read(OriginalEdition.Patch1078, type, globals, new() { Number = () => 75 }));
    }

    /// <summary>실제 보상 HP가 진단 뒤 바뀐 전역·현재 base를 읽고 선택된 물리 this를 보존한다.</summary>
    [Fact]
    public void RewardHpKeepsSelectedPhysicalRecordAcrossResumedDiagnostic()
    {
        var physical = Enumerable.Range(0, 189).Select(_ => new OriginalTypeRecord()).ToArray();
        physical[74].MaxHitPoints = 29; physical[75].MaxHitPoints = 999;
        var pool = new RawSidPool(24000, true); var random = new GameRandom(1);
        var quarter = new BoolBox(); var priest = new UIntBox { Value = 158 }; var dais = new UIntBox { Value = 165 };
        var state = new SquidRewardState { DebugAsserts = true, SharedPriestQuarterHp = quarter, SharedPriestType = priest, SharedDaisType = dais };
        var reward = new RawSquidReward(pool, new ArraySegment<OriginalTypeRecord>(physical, 0, 188), new(), state, new RawSpStore(random, () => 1));
        reward.BindCurrentTypes(new ArraySegment<OriginalTypeRecord>(physical, 0, 188)); int calls = 0;
        reward.BindHitPointDiagnostics((expression, file, line) =>
        {
            Assert.Equal(RawRiftHitPoints.Expression, expression); Assert.Equal(RawRiftHitPoints.Source, file); Assert.Equal(0x1e3u, line);
            calls++; quarter.Value = true; priest.Value = 73; physical[74].MaxHitPoints = -37;
            reward.BindCurrentTypes(new ArraySegment<OriginalTypeRecord>(physical, 1, 188));
        });
        Assert.Equal(-9, reward.TypeHitPoints(74)); Assert.Equal(1, calls);
        Assert.Throws<ArgumentException>(() => reward.BindCurrentTypes(new OriginalTypeRecord[3]));
        Assert.Throws<ArgumentException>(() => reward.BindCurrentTypes(physical.Take(188).ToList()));
        state.DebugAsserts = false; quarter.Value = false;
        Assert.Equal(-37, reward.TypeHitPoints(73));
        reward.BindCurrentTypes(null); Assert.Equal(999, reward.TypeHitPoints(75));
    }

    /// <summary>전송의 기존 공장 HP 진입도 같은 물리 기준 이동과 레코드 칸 교체를 처리한다.</summary>
    [Fact]
    public void FactorySelectedHpSharesPhysicalViewAndCurrentRecordReplacement()
    {
        var physical = Enumerable.Range(0, 189).Select(_ => new OriginalTypeRecord()).ToArray();
        physical[74].MaxHitPoints = 29; var origin = new ArraySegment<OriginalTypeRecord>(physical, 0, 188);
        var factory = new RawSquidFactory(new RawSidPool(24000, true), origin);
        var quarter = new BoolBox(); var priest = new UIntBox { Value = 158 }; var dais = new UIntBox { Value = 165 };
        factory.BindWeakenedMana(quarter); factory.BindHitPointTypes(priest, dais);
        factory.BindDiagnostics(new()
        {
            Types = origin, Checking = new(true), Assertion = (_, _, _) =>
            {
                quarter.Value = true; priest.Value = 73;
                physical[74] = new OriginalTypeRecord { MaxHitPoints = -37 };
                factory.BindDiagnostics(new() { Types = new ArraySegment<OriginalTypeRecord>(physical, 1, 188) });
            },
        });
        Assert.Equal(-9, factory.ReadSelectedHitPoints(physical[74], origin));
    }
}
