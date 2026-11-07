// 일반 지형 분기·요청·농지 후처리 진단의 기본값·보관·호출을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 일반 지형 요청형을 검증하는 테스트 클래스다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.
public sealed class RawNormalTerrainRequestTests
{
    /// <summary>기본 요청은 미복원 부모 분기다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultRequestIsUnrestoredParent()
    {
        NormalTerrainRequest request = NormalTerrainRequest.Unrestored;
        Assert.Equal(0u, request.Owner);
        Assert.Equal(127, request.SelectedRegion);
        Assert.Equal(NormalTerrainBranch.UnrestoredParent, request.Branch);
    }

    /// <summary>지정한 소유자·범위·지역·분기를 그대로 보관한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void RequestKeepsFourValues()
    {
        var bounds = new ChunkRect(1, 2, 15, 15);
        var request = new NormalTerrainRequest(3u, bounds, 5, NormalTerrainBranch.Normal);
        Assert.Equal(3u, request.Owner);
        Assert.Equal(bounds, request.Bounds);
        Assert.Equal(5, request.SelectedRegion);
        Assert.Equal(NormalTerrainBranch.Normal, request.Branch);
    }

    /// <summary>분기 세 값은 서로 다르다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void BranchesAreDistinct()
    {
        Assert.NotEqual(NormalTerrainBranch.Normal, NormalTerrainBranch.Archipelago);
        Assert.NotEqual(NormalTerrainBranch.Normal, NormalTerrainBranch.UnrestoredParent);
        Assert.NotEqual(NormalTerrainBranch.Archipelago, NormalTerrainBranch.UnrestoredParent);
    }

    /// <summary>진단 묶음의 기본값은 모두 비연결이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void DiagnosticsDefaultToUnbound()
    {
        var diagnostics = new NormalTerrainDiagnostics();
        Assert.Null(diagnostics.Context);
        Assert.Null(diagnostics.Stamp);
        Assert.Null(diagnostics.Region);
        Assert.Null(diagnostics.Farm);
        Assert.Null(diagnostics.Postpass);
        Assert.Null(diagnostics.DecoderDiagnostics);
    }

    /// <summary>다섯 진단을 바깥에 전달한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void DiagnosticsForwardFiveAssertions()
    {
        var seen = new List<string>();
        var diagnostics = new NormalTerrainDiagnostics
        {
            Context = assertion => seen.Add($"context:{assertion}"),
            Stamp = assertion => seen.Add($"stamp:{assertion}"),            Region = assertion => seen.Add($"region:{assertion}"),
            Farm = assertion => seen.Add($"farm:{assertion}"),
            Postpass = assertion => seen.Add($"postpass:{assertion}"),
        };
        diagnostics.Context!(TerrainContextAssertion.RightChunkLimit);
        diagnostics.Stamp!(TerrainStampAssertion.MissingRegion);
        diagnostics.Region!(SquidRegionAssert.UnsupportedGenus);
        diagnostics.Farm!(FarmClusterAssertion.MissingIsland);
        diagnostics.Postpass!(TerrainFarmPostpassAssertion.BoundsNotFound);
        Assert.Equal(
        [
            $"context:{TerrainContextAssertion.RightChunkLimit}",
            $"stamp:{TerrainStampAssertion.MissingRegion}",
            $"region:{SquidRegionAssert.UnsupportedGenus}",
            $"farm:{FarmClusterAssertion.MissingIsland}",
            $"postpass:{TerrainFarmPostpassAssertion.BoundsNotFound}",
        ], seen);
    }

    /// <summary>복사는 같은 콜백을 공유하며 원본과 별개 묶음이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    [Fact]
    public void CloneSharesCallbacksWithoutAliasingBundle()
    {
        Action<TerrainFarmPostpassAssertion> postpass = _ => { };
        var diagnostics = new NormalTerrainDiagnostics { Postpass = postpass };
        NormalTerrainDiagnostics copy = diagnostics.Clone();
        Assert.Same(postpass, copy.Postpass);
        Assert.Null(copy.Context);
        copy.Context = _ => { };
        Assert.Null(diagnostics.Context);
    }
}
