// 현재 런타임 묶음의 기본값·전달·복사를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 호스트 묶음을 검증하는 테스트 클래스다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.
public sealed class RawProcessHostBindingsTests
{
    /// <summary>전송 묶음은 비어 시작한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void TransmissionDefaultsNull()
    {
        var transmission = new ProcessFormTransmission();
        Assert.Null(transmission.WriteData);
        Assert.Null(transmission.Transmit);
        Assert.Null(transmission.Packet);
        Assert.Null(transmission.WriteFullData);
    }

    /// <summary>네 콜백이 인자·반환을 그대로 나른다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void TransmissionForwardsCalls()
    {
        var process = new SquidProcess();
        int sent = 0;
        var transmission = new ProcessFormTransmission
        {
            WriteData = (target, changed) =>
            {
                Assert.Same(process, target);
                Assert.True(changed);
                return 12;
            },
            Transmit = length => sent = length,
            Packet = length =>
            {
                Assert.Equal(12, length);
                return [(byte)0x7a, (byte)0x01];
            },
            WriteFullData = (target, argument) =>
            {
                Assert.Same(process, target);
                Assert.Equal(54u, argument);
                return 20;
            },
        };
        Assert.Equal(12, transmission.WriteData!(process, true));
        transmission.Transmit!(12);
        Assert.Equal(12, sent);
        Assert.Equal([(byte)0x7a, (byte)0x01], transmission.Packet!(12));
        Assert.Equal(20, transmission.WriteFullData!(process, 54u));
    }

    /// <summary>런타임 묶음은 비어 시작한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingsDefaultsNull()
    {
        var bindings = new ProcessHostRuntimeBindings();
        Assert.Null(bindings.Destroy);
        Assert.Null(bindings.Regular);
        Assert.Null(bindings.Asset);
        Assert.Null(bindings.Transmit);
        Assert.Equal(0ul, bindings.ActiveCalls);
        Assert.Null(bindings.PostPopDepth);
        Assert.Null(bindings.Assertion);
        Assert.Null(bindings.Factory);
    }

    /// <summary>현재 월드의 참조·대리자·개수를 그대로 pin한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingsPinCurrentWorld()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var factory = new RawSquidFactory(pool, types);
        var hooks = new SquidDestroyHooks();
        var transmission = new ProcessFormTransmission();
        string seen = string.Empty;
        var bindings = new ProcessHostRuntimeBindings
        {
            Regular = (parent, ev, count, payload) =>
            {
                Assert.Equal(5, parent);
                Assert.Equal(7u, ev);
                return payload + 1f;
            },
            Asset = hooks,
            Transmit = transmission,
            ActiveCalls = 2ul,
            PostPopDepth = () => 3u,
            Assertion = (expression, file, line) => seen = line + ":" + expression + ":" + file,
            Factory = factory,
        };
        Assert.Equal(2.5f, bindings.Regular!(5, 7u, 0u, 1.5f));
        Assert.Same(hooks, bindings.Asset);
        Assert.Same(transmission, bindings.Transmit);
        Assert.Equal(2ul, bindings.ActiveCalls);
        Assert.Equal(3u, bindings.PostPopDepth!());
        bindings.Assertion!("e", "f", 9u);
        Assert.Equal("9:e:f", seen);
        Assert.Same(factory, bindings.Factory);
        Assert.Null(bindings.Destroy);
    }

    /// <summary>복사는 개수 값을 복사하고 참조·대리자는 공유한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void CloneSharesDelegatesAndCopiesCount()
    {
        var transmission = new ProcessFormTransmission { Transmit = _ => { } };
        ProcessFormTransmission transmissionCopy = transmission.Clone();
        Assert.Same(transmission.Transmit, transmissionCopy.Transmit);
        var bindings = new ProcessHostRuntimeBindings
        {
            Regular = (_, _, _, payload) => payload,
            Transmit = transmission,
            ActiveCalls = 4ul,
        };
        ProcessHostRuntimeBindings copy = bindings.Clone();
        Assert.Same(bindings.Regular, copy.Regular);
        Assert.Same(transmission, copy.Transmit);
        Assert.Equal(4ul, copy.ActiveCalls);
        copy.ActiveCalls = 9ul;
        Assert.Equal(4ul, bindings.ActiveCalls);
    }
}
