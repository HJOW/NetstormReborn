using Netstorm.Core.Original;

// 전송이 선택한 TYPE HP의 현재 전역·진단 복귀·물리 표 경계를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 RawRiftHitPoints/SquidFactory의 현재 최대HP 조회 계약이다.</summary>
public sealed class RawSquidFactoryHitPointsTests
{
    /// <summary>진단 복귀 뒤 현재 약화·특별 번호와 처음 선택한 TYPE의 현재HP를 소비한다.</summary>
    [Fact]
    public void DiagnosticReturnReadsCurrentGlobalsAndSelectedHp()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[74].MaxHitPoints = 200; var pool = new RawSidPool(32768, true);
        var factory = new RawSquidFactory(pool, types); var quarter = new BoolBox(); var priest = new UIntBox { Value = 158 }; var dais = new UIntBox { Value = 165 };
        factory.BindWeakenedMana(quarter); factory.BindHitPointTypes(priest, dais);
        int diagnostics = 0;
        factory.BindDiagnostics(new SquidFactoryDiagnostics
        {
            Types = types, Checking = new RawCheckingFlag(0x80000000u),
            Assertion = (expression, file, line) =>
            {
                Assert.Equal("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType", expression);
                Assert.Equal(@"\Ns\O\Rifttype.cpp", file); Assert.Equal(0x1e3u, line); diagnostics++;
                quarter.Value = true; priest.Value = 74; types[74] = new OriginalTypeRecord { MaxHitPoints = -11 };
            },
        });
        Assert.Equal(-2, factory.ReadSelectedHitPoints(types[74], types)); Assert.Equal(1, diagnostics);
        dais.Value = 74; Assert.Equal(-2, factory.ReadSelectedHitPoints(types[74], types)); Assert.Equal(1, diagnostics);
        types[74].Flags1 = OriginalTypeRecord.HasHitPoints; dais.Value = 0;
        Assert.Equal(-2, factory.ReadSelectedHitPoints(types[74], types)); Assert.Equal(1, diagnostics);
    }

    /// <summary>CD에서는 checking·HP플래그 진단 없이 현재 특별 타입만 약화한다.</summary>
    [Fact]
    public void CdUsesSignedQuarterWithoutPatchAssertion()
    {
        // 기존 공장 API의188개 padded CD 표를 유지하며 HP getter 자체의 판본 분기를 검사한다.
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { MaxHitPoints = -15 }).ToArray();
        var factory = new RawSquidFactory(new RawSidPool(32768, false, edition: OriginalEdition.Cd1072), types, true);
        factory.BindDiagnostics(new SquidFactoryDiagnostics { Types = types, Checking = new RawCheckingFlag(true) });
        Assert.Equal(-3, factory.ReadSelectedHitPoints(types[158], types)); Assert.Equal(-15, factory.ReadSelectedHitPoints(types[74], types));
    }

    /// <summary>원문 getNum을 실제 선택할 때만 다른 물리 표를 거부하며 번호로 TYPE를 다시 고르지 않는다.</summary>
    [Fact]
    public void ReboundTableFailsOnlyAtSelectedNumberGetter()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { MaxHitPoints = 77 }).ToArray();
        var factory = new RawSquidFactory(new RawSidPool(32768, true), types);
        var other = types.Select(type => type.Clone()).ToArray();
        factory.BindDiagnostics(new SquidFactoryDiagnostics { Types = other });
        Assert.Equal(77, factory.ReadSelectedHitPoints(types[74], types));
        factory.BindWeakenedMana(new BoolBox { Value = true });
        Assert.Throws<InvalidOperationException>(() => factory.ReadSelectedHitPoints(types[74], types));
    }
}
