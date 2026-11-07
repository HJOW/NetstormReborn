// 월드 런타임 훅의 기본값·전달·복사를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 월드 런타임 훅을 검증하는 테스트 클래스다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.
public sealed class RawWorldRuntimeHooksTests
{
    /// <summary>런타임 묶음은 비어 시작한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksDefaultsNull()
    {
        var hooks = new RawWorldRuntimeHooks();
        Assert.Null(hooks.Assets);
        Assert.Null(hooks.Destroy);
        Assert.Null(hooks.Regular);
        Assert.Null(hooks.Transmit);
        Assert.Null(hooks.WalkerFall);
        Assert.Null(hooks.LoadingDepth);
        Assert.Null(hooks.FormTransmit);
        Assert.Null(hooks.SquidPacket);
        Assert.Null(hooks.SquidConnected);
        Assert.Null(hooks.SquidSend);
        Assert.Null(hooks.DependencyNotice);
        Assert.Null(hooks.BridgeEffect);
    }

    /// <summary>열두 경계가 인자·반환을 그대로 나른다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksForwardCalls()
    {
        string seen = string.Empty;
        var hooks = new RawWorldRuntimeHooks
        {
            Assets = new SquidDestroyHooks(),
            Destroy = (sid, flags, _) =>
            {
                Assert.Equal(5, sid);
                Assert.Equal(6u, flags);
                return true;
            },
            Regular = (parent, ev, count, payload) => payload + 1f,
            Transmit = new ProcessFormTransmission(),
            WalkerFall = sid =>
            {
                Assert.Equal(7, sid);
                return true;
            },
            LoadingDepth = depth => Assert.Equal(2u, depth),
            FormTransmit = (form, flags) =>
            {
                Assert.Equal(8, form);
                Assert.Equal(9u, flags);
            },
            SquidPacket = packet => Assert.Equal([(byte)0x7a], packet),
            SquidConnected = target =>
            {
                Assert.Equal(3, target);
                return true;
            },
            SquidSend = (target, packet) =>
            {
                Assert.Equal(4, target);
                Assert.Equal([(byte)0x01], packet);
            },
            DependencyNotice = (watcher, watched, form) => seen = $"{watcher}:{watched}:{form}",
            BridgeEffect = @event => seen = @event.Effect.ToString(),
        };
        Assert.True(hooks.Destroy!(5, 6u, hooks.Assets!));
        Assert.Equal(2f, hooks.Regular!(0, 0u, 0u, 1f));
        Assert.True(hooks.WalkerFall!(7));
        hooks.LoadingDepth!(2u);
        hooks.FormTransmit!(8, 9u);
        hooks.SquidPacket!([(byte)0x7a]);
        Assert.True(hooks.SquidConnected!(3));
        hooks.SquidSend!(4, [(byte)0x01]);
        hooks.DependencyNotice!(1, 2, 3);
        Assert.Equal("1:2:3", seen);
        hooks.BridgeEffect!(new BridgeLifecycleEvent(BridgeLifecycleEffect.DestroyLink, 0, 0u));
        Assert.Equal("DestroyLink", seen);
    }

    /// <summary>복사는 참조·대리자를 공유한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    [Fact]
    public void CloneSharesReferences()
    {
        var hooks = new RawWorldRuntimeHooks
        {
            Regular = (_, _, _, payload) => payload,
            WalkerFall = _ => true,
            SquidPacket = _ => { },
        };
        RawWorldRuntimeHooks copy = hooks.Clone();
        Assert.Same(hooks.Regular, copy.Regular);
        Assert.Same(hooks.WalkerFall, copy.WalkerFall);
        Assert.Same(hooks.SquidPacket, copy.SquidPacket);
        hooks.Regular = (_, _, _, _) => 0f;
        Assert.NotSame(hooks.Regular, copy.Regular);
    }
}
