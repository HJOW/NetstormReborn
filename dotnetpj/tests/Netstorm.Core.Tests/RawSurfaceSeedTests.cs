// 서피스 시드·스냅샷·표시훅의 기본값·값 의미·훅 호출을 검증한다.
using Netstorm.Core.Display;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 서피스 월드 자료형을 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.
public sealed class RawSurfaceSeedTests
{
    /// <summary>기본 시드는 모두 0이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultSeedIsAllZeros()
    {
        var seed = default(SurfaceSeed);
        Assert.Equal(0u, seed.Type);
        Assert.Equal(0u, seed.Owner);
        Assert.Equal(0, seed.Frame);
        Assert.Equal(0.0f, seed.X);
        Assert.Equal(0.0f, seed.Y);
    }

    /// <summary>지정한 다섯 값을 그대로 보관한다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void SeedKeepsFiveValues()
    {
        var seed = new SurfaceSeed(82u, 3u, 5, 1.5f, -2.25f);
        Assert.Equal(82u, seed.Type);
        Assert.Equal(3u, seed.Owner);
        Assert.Equal(5, seed.Frame);
        Assert.Equal(1.5f, seed.X);
        Assert.Equal(-2.25f, seed.Y);
    }

    /// <summary>스냅샷은 번호와 객체 자료를 함께 나른다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void SnapshotCarriesSidAndObject()
    {
        var seed = new SurfaceSeed(94u, 1u, 0, 10.0f, 20.0f);
        var snapshot = new SurfaceSnapshot(7, seed);
        Assert.Equal(7, snapshot.Sid);
        Assert.Equal(seed, snapshot.Object);
        Assert.Equal(94u, snapshot.Object.Type);
    }

    /// <summary>훅 묶음의 기본값은 모두 비연결이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksDefaultToUnbound()
    {
        var hooks = new SurfaceDisplayHooks();
        Assert.Null(hooks.Suppressed);
        Assert.Null(hooks.Invalidate);
    }

    /// <summary>억제 조회와 영역 무효화를 바깥에 전달한다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksForwardSuppressedAndInvalidate()
    {
        bool suppressed = true;
        DisplayRect area = default;
        uint flags = 0u;
        var hooks = new SurfaceDisplayHooks
        {
            Suppressed = () => suppressed,
            Invalidate = (rect, effect) =>
            {
                area = rect;
                flags = effect;
            },
        };
        Assert.True(hooks.Suppressed!());
        var rect = new DisplayRect(1, 2, 30, 40);
        hooks.Invalidate!(rect, 9u);
        Assert.Equal(rect, area);
        Assert.Equal(9u, flags);
    }

    /// <summary>복사는 같은 콜백을 공유하며 원본과 별개 묶음이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void CloneSharesCallbacksWithoutAliasingBundle()
    {
        Func<bool> suppressed = () => false;
        var hooks = new SurfaceDisplayHooks { Suppressed = suppressed };
        SurfaceDisplayHooks copy = hooks.Clone();
        Assert.Same(suppressed, copy.Suppressed);
        Assert.Null(copy.Invalidate);
        copy.Invalidate = (_, _) => { };
        Assert.Null(hooks.Invalidate);
    }
}
