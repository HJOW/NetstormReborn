// 공유 바람 세 float의 초기값·독립성·쓰기를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AmbientWindState를 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.
public sealed class RawAmbientWindTests
{
    /// <summary>실제 PE 최초값(0·1·0)으로 시작한다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultsAreZeroOneZero()
    {
        var state = new AmbientWindState();
        Assert.Equal(0.0f, state.X);
        Assert.Equal(1.0f, state.Y);
        Assert.Equal(0.0f, state.Angle);
    }

    /// <summary>인스턴스마다 독립 전역이다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    [Fact]
    public void InstancesAreIndependent()
    {
        var first = new AmbientWindState();
        var second = new AmbientWindState();
        first.X = 2.5f;
        first.Y = 3.5f;
        first.Angle = 4.5f;
        Assert.Equal(0.0f, second.X);
        Assert.Equal(1.0f, second.Y);
        Assert.Equal(0.0f, second.Angle);
    }

    /// <summary>세 값을 읽고 쓴다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    [Fact]
    public void ValuesAreReadableAndWritable()
    {
        var state = new AmbientWindState
        {
            X = -1.25f,
            Y = 0.5f,
            Angle = 90.0f,
        };
        Assert.Equal(-1.25f, state.X);
        Assert.Equal(0.5f, state.Y);
        Assert.Equal(90.0f, state.Angle);
    }
}
