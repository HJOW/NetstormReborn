using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 판본(<see cref="OriginalEdition"/>) 배선을 고정한다.
/// 근거: cpppj/src/o/SidPool.h(판본은 생성 때 필수). 이 클론은 기존 호출의 동작을 유지하려고 기본 10.78로 둔다. (R2d-2, 2026-10-29)
/// 기존 L1(const sid-x86 전체)이 기본 판본에서 그대로 통과함으로 회귀 없음을 본다.
/// </summary>
public sealed class RawSidPoolEditionTests
{
    /// <summary>기본 판본은 10.78이다.</summary>
    [Fact]
    public void DefaultIsPatch1078()
    {
        var pool = new RawSidPool(32768);
        Assert.Equal(OriginalEdition.Patch1078, pool.Edition);
    }

    /// <summary>명시한 CD 판본을 유지한다.</summary>
    [Fact]
    public void ExplicitCdPreserved()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072);
        Assert.Equal(OriginalEdition.Cd1072, pool.Edition);
    }

    /// <summary>Reset 뒤에도 판본을 유지한다(원본처럼 판본은 생성 때만 정한다).</summary>
    [Fact]
    public void ResetKeepsEdition()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072);
        pool.Reset();
        Assert.Equal(OriginalEdition.Cd1072, pool.Edition);
    }
}
