using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 원본 선택 체력 막대의 현재 조회 순서를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>패치와 CD의 조회 횟수 및 진단 중 상태 변경을 검사한다.</summary>
public sealed class SpriteHitPointBarTests
{
    /// <summary>패치 색 두 번과 막대 값은 각각 별도 현재·최대 조회를 사용한다.</summary>
    [Fact]
    public void PatchReadsCurrentAndMaximumThreeTimesInOrder()
    {
        var events = new List<string>();
        int[] values = [60, 9, 7];
        int[] limits = [100, 40, 80];
        int valueIndex = 0, limitIndex = 0;
        uint[] colors = [1, 2, 3];
        SpriteHitPointResult result = SpriteHitPointBar.Read(OriginalEdition.Patch1078, colors,
            () => { events.Add("현재"); return values[valueIndex++]; },
            () => { events.Add("최대"); if (limitIndex == 1) colors[2] = 9; return limits[limitIndex++]; });
        Assert.Equal(new[] { "현재", "최대", "현재", "최대", "현재", "최대" }, events);
        Assert.Equal(new SpriteHitPointResult(7, 80, 9), result);
    }

    /// <summary>CD의 첫 양수 검증 뒤 두 번째 최대 조회가 0이 되는 경우도 그대로 보존한다.</summary>
    [Fact]
    public void CdPreservesChangedSecondMaximum()
    {
        int reads = 0;
        SpriteHitPointResult result = SpriteHitPointBar.Read(OriginalEdition.Cd1072, [1, 2, 3], () => 10, () => ++reads == 1 ? 5 : 0);
        Assert.Equal(2, reads);
        Assert.Equal(new SpriteHitPointResult(0, 0, 3), result);
    }
}
