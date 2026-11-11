using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 분위(quartile) 범위가 비는 드문 타일 코드의 NB23 교정 계약을 검사한다
/// (cpppj <c>o/RawFrameChoose.cpp</c>의 HEAD 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.345에서 10차 NB23 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 L1 그대로다.</remarks>
public sealed class RawFrameChooseTests
{
    /// <summary>같은 방향·변형 코드를 지정 수만큼 만든다.</summary>
    /// <param name="count">만들 코드 수다.</param>
    /// <returns>후보 검색용 프레임 표다.</returns>
    private static RiftTypeFrames Frames(int count)
    {
        List<RiftFrameCode> codes = [];
        // 후보는 저장 순서대로 인덱스가 매겨진다
        for (int index = 0; index < count; index++)
        {
            codes.Add(new RiftFrameCode((byte)'A', (byte)'P', (byte)index, 0));
        }
        return new RiftTypeFrames(codes);
    }

    /// <summary>후보 5개의 빈 분위 범위는 예외 대신 전체 후보에서 고른다.</summary>
    /// <param name="theme">사분위 번호(0..3)다.</param>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void EmptyQuartileFallsBackToWholeCandidates(int theme)
    {
        RiftTypeFrames frames = Frames(5);
        var code = new RiftFrameCode((byte)'A', (byte)'P', 0, 0);
        // 후보 5개면 quarter=1이라 모든 theme의 분위 범위가 빈다
        int picked = RawFrameChoose.ChooseTerrainFrame(frames, code, theme, new NetstormRandom(1));
        Assert.InRange(picked, 0, 4);
    }

    /// <summary>후보 1개의 빈 분위 범위도 전체 후보(유일 후보)를 돌려준다.</summary>
    [Fact]
    public void SingleCandidateFallsBackToWholeCandidates()
    {
        RiftTypeFrames frames = Frames(1);
        var code = new RiftFrameCode((byte)'A', (byte)'P', 0, 0);
        // quarter=0이라 low=1·high=0으로 범위가 빈다
        Assert.Equal(0, RawFrameChoose.ChooseTerrainFrame(frames, code, 0, new NetstormRandom(1)));
    }

    /// <summary>정상 분위 범위는 기존처럼 해당 구간에서만 고른다.</summary>
    [Fact]
    public void NormalQuartileRangeStillApplies()
    {
        RiftTypeFrames frames = Frames(12);
        var code = new RiftFrameCode((byte)'A', (byte)'P', 0, 0);
        // 후보 12개·theme=1이면 low=4·high=6이다
        int picked = RawFrameChoose.ChooseTerrainFrame(frames, code, 1, new NetstormRandom(1));
        Assert.True(picked is 4 or 5, $"기대 구간 4..5, 실제 {picked}");
    }

    /// <summary>theme=-1은 전체 후보에서 고른다.</summary>
    [Fact]
    public void WholeThemePicksFromAllCandidates()
    {
        RiftTypeFrames frames = Frames(5);
        var code = new RiftFrameCode((byte)'A', (byte)'P', 0, 0);
        int picked = RawFrameChoose.ChooseTerrainFrame(frames, code, -1, new NetstormRandom(1));
        Assert.InRange(picked, 0, 4);
    }
}
