using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// ColorSort primary의 보호 계약을 확인한다. 원본 기대값 재생(X86OwnerHslColorMapsTests)이 두 경로의 표를 대조하고,
/// 이 검사들은 생성·명도 읽기의 계약만 옮긴다.
/// 근거: cpppj/src/o/RawColorSortPrimary.h/.cpp. (R2c, 2026-10-28)
/// </summary>
public sealed class RawColorSortPrimaryTests
{
    /// <summary>초기 바이트가 0x3800이 아니면 거부한다.</summary>
    [Fact]
    public void ConstructorRejectsBadAllocation()
    {
        Assert.Throws<ArgumentException>(() => new RawColorSortPrimary(new byte[100]));
    }

    /// <summary>생성 전 명도 읽기와 범위 밖 번호를 거부한다. 생성 뒤에는 읽힌다.</summary>
    [Fact]
    public void LightnessNeedsConstruction()
    {
        var primary = new RawColorSortPrimary(new byte[RawColorSortPrimary.TotalBytes]);
        Assert.False(primary.Constructed);
        Assert.Throws<InvalidOperationException>(() => primary.Lightness(0));
        Assert.Throws<ArgumentException>(() => primary.Construct(new byte[100]));
        primary.Construct(new byte[1024]);
        Assert.True(primary.Constructed);
        Assert.Equal(0.0, primary.Lightness(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => primary.Lightness(256));
    }

    /// <summary>관측 경계는 진입 (0,0,0) 뒤 각 store의 (누적 수,offset,size)를 받는다.</summary>
    [Fact]
    public void CheckpointObservesStores()
    {
        var primary = new RawColorSortPrimary(new byte[RawColorSortPrimary.TotalBytes]);
        var calls = new List<(uint, uint, uint)>();
        primary.Construct(new byte[1024], (writes, offset, size) => calls.Add((writes, offset, size)));
        // 진입 1회 + 기록당 7회(R·G·번호·B·H·L·S) × 256
        Assert.Equal(1 + 7 * 256, calls.Count);
        Assert.Equal((0u, 0u, 0u), calls[0]);
        Assert.Equal((1u, 24u, 8u), calls[1]);
    }
}
