using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션이 섬 런타임 부모를 공유 저장소로 만드는지 검사한다.
/// 근거: RawIslandRuntime의 동일 풀·기록·모드·목록·해시 공유 계약. 시작 흐름 연결은 다음 단위다. (2026-11-12 0.452에서 추가했다)</summary>
public sealed class HostIslandsBuildTests
{
    /// <summary>세션 저장소를 공유하는 섬 런타임을 만들고 빈 목록을 확인한다.</summary>
    [Fact]
    public void BuildIslandsSharesSessionStoresWithEmptyList()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        RawIslandRuntime islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => throw new InvalidOperationException("조립 검사는 Residence owner를 요청하지 않는다"),
            Assertion = _ => throw new InvalidOperationException("조립 검사의 Runtime 진단은 원본 분류와 불일치한다"),
        });
        Assert.Same(session.Pool, islands.Pool());
        Assert.Equal(0u, islands.Count());
    }

    /// <summary>효과 묶음이 없으면 섬 부모 만들기를 거절하는지 확인한다.</summary>
    [Fact]
    public void BuildIslandsRejectsMissingHooks()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<ArgumentNullException>(() => session.BuildIslands(null!));
    }
}
