// 복원된 섬·다리·noIsland 삭제 분배의 생성 검증·공통 유지·가상 표 분기를 검증한다.
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawSurfaceLifecycle의 PATCH 본체를 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
public sealed class RawSurfaceLifecycleTests
{
    /// <summary>10.78 다리 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint BridgeVtable = 0x005034c8u;

    /// <summary>10.78 섬 받침 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint IslandVtable = 0x00506d98u;

    /// <summary>10.78 무인도 표면 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint NoIslandVtable = 0x00506f08u;

    /// <summary>섬·다리 추가 비트의 추상·매립 마스크다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const byte AbstractOrBuried = 9;

    /// <summary>삭제 분배 순서를 관찰하는 장면이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private sealed class LifecycleScene
    {
        /// <summary>같은 풀을 공유하는 원시 풀이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public RawSidPool Pool = new(32768, false);

        /// <summary>이웃 탐색기가 읽는 공간 해시다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public RawSquidHash Hash = new();

        /// <summary>공통 삭제 훅이 받은 사건이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public List<SquidDestroyEvent> CommonLog = [];

        /// <summary>외부 효과 호출 순서다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public List<string> Order = [];

        /// <summary>다리 효과가 받은 사건이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public List<BridgeLifecycleEvent> BridgeLog = [];

        /// <summary>같은 풀·타입의 이웃 탐색기다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public RawSquidNeighbors Neighbors;

        /// <summary>같은 풀의 공통 삭제 훅이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public SquidDestroyHooks Common;

        /// <summary>외부 효과 묶음이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public SurfaceLifecycleHooks Hooks;

        /// <summary>장면을 만든다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        public LifecycleScene()
        {
            var scene = this;
            var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            for (int index = 0; index < types.Length; ++index)
            {
                types[index] = new OriginalTypeRecord();
            }

            var frames = new List<List<FrameCode>>(types.Length);
            // 이웃 탐색기 구성이 요구하는 빈 프레임 표를 채우는 반복문이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            for (int index = 0; index < types.Length; ++index)
            {
                frames.Add([]);
            }

            Neighbors = new RawSquidNeighbors(Pool, Hash, new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells], types, frames);
            Common = new SquidDestroyHooks
            {
                Emit = @event => scene.CommonLog.Add(@event),
                Selected = () => 0,
            };
            Hooks = new SurfaceLifecycleHooks
            {
                ScheduleFall = (sid, x, y) => scene.Order.Add($"fall:{sid}:{x}:{y}"),
                Destroy = (sid, flags) => scene.Order.Add($"destroy:{sid}:{flags}"),
                FallWalker = sid => scene.Order.Add($"walker:{sid}"),
                BridgeEffect = @event => scene.BridgeLog.Add(@event),
            };
        }

        /// <summary>연결 타입 155의 분배기를 만든다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        /// <returns>분배기다.</returns>
        public RawSurfaceLifecycle Build() => new(Pool, Neighbors, 155u, Common, Hooks);

        /// <summary>번호를 확보하고 가상 표를 쓴다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
        /// <param name="vtable">쓸 가상 표 값이다.</param>
        /// <returns>확보한 번호다.</returns>
        public int Take(uint vtable)
        {
            int sid = Pool.Allocate(SidAllocation.Client);
            Span<byte> raw = Pool.AllocatedBytes(sid);
            // 낮은 바이트부터 합치는 반복문이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                raw[index] = (byte)(vtable >> (index * 8));
            }

            return sid;
        }
    }

    /// <summary>연결 타입 범위와 공통/외부 훅이 아니면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void ConstructorRejectsBadConnectorAndMissingHooks()
    {
        var scene = new LifecycleScene();
        Assert.NotNull(scene.Build());
        // 자산 첫 번호보다 작으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 69u, scene.Common, scene.Hooks));
        // 타입 표 개수와 같으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 188u, scene.Common, scene.Hooks));
        // 공통 emit이 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, new SquidDestroyHooks { Selected = () => 0 }, scene.Hooks));
        // 공통 선택 조회가 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, new SquidDestroyHooks { Emit = _ => { } }, scene.Hooks));
        // 외부 낙하 예약이 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        var hooks = scene.Hooks.Clone();
        hooks.ScheduleFall = null;
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, scene.Common, hooks));
        // 외부 재귀 삭제가 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        hooks = scene.Hooks.Clone();
        hooks.Destroy = null;
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, scene.Common, hooks));
        // 외부 walker 낙하가 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        hooks = scene.Hooks.Clone();
        hooks.FallWalker = null;
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, scene.Common, hooks));
        // 외부 다리 효과가 없으면 거절한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        hooks = scene.Hooks.Clone();
        hooks.BridgeEffect = null;
        Assert.Throws<ArgumentException>(() => new RawSurfaceLifecycle(scene.Pool, scene.Neighbors, 155u, scene.Common, hooks));
    }

    /// <summary>공통 선택·Form 해제를 유지하고 emit만 분배기로 바꾼다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksPreservesSelectedAndReplacesEmit()
    {
        var scene = new LifecycleScene();
        int selected = 7;
        Action<int> form = _ => { };
        scene.Common.Selected = () => selected;
        scene.Common.UnpopForm = form;
        var lifecycle = scene.Build();
        SquidDestroyHooks supplied = lifecycle.Hooks();
        Assert.Equal(selected, supplied.Selected!());
        Assert.Same(form, supplied.UnpopForm);
        var transmitted = new SquidDestroyEvent(SquidDestroyEffect.Transmit, 5, 0, 8u);
        supplied.Emit!(transmitted);
        Assert.Equal([transmitted], scene.CommonLog);
    }

    /// <summary>사전·사후가 아닌 효과는 그대로 공통에 전달한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitForwardsOtherEffects()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        var first = new SquidDestroyEvent(SquidDestroyEffect.Transmit, 5, 0, 8u);
        var second = new SquidDestroyEvent(SquidDestroyEffect.ReleaseDependent, 6, 5, 0u);
        lifecycle.Emit(first);
        lifecycle.Emit(second);
        Assert.Equal([first, second], scene.CommonLog);
        Assert.Empty(scene.Order);
    }

    /// <summary>모르는 가상 표는 사전·사후 모두 공통에 전달한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitForwardsUnknownVtable()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        int sid = scene.Take(0u);
        var pre = new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 0, 1u);
        var post = new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 0, 2u);
        lifecycle.Emit(pre);
        lifecycle.Emit(post);
        Assert.Equal([pre, post], scene.CommonLog);
    }

    /// <summary>섬 가상 표의 사전은 공통 사전으로 정확히 한 번 닫는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitRoutesIslandPreToBasePre()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        int sid = scene.Take(IslandVtable);
        // 추상 비트를 세워 이웃 순회를 건너뛰고 공통 사전만 관찰한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        scene.Pool.AllocatedBytes(sid)[0x28] |= AbstractOrBuried;
        lifecycle.Emit(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 9, 3u));
        Assert.Equal([new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 0, 3u)], scene.CommonLog);
        Assert.Empty(scene.Order);
    }

    /// <summary>무인도 가상 표의 사후는 공통 사후로 정확히 한 번 닫는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitRoutesNoIslandPostToBasePost()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        int sid = scene.Take(NoIslandVtable);
        // 추상 비트를 세워 칸 탐색을 건너뛰고 공통 사후만 관찰한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        scene.Pool.AllocatedBytes(sid)[0x28] |= AbstractOrBuried;
        lifecycle.Emit(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 9, 4u));
        Assert.Equal([new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 0, 4u)], scene.CommonLog);
        Assert.Empty(scene.Order);
    }

    /// <summary>다리 가상 표의 사전은 다리 몸체를 거쳐 공통 사전으로 닫는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitRoutesBridgePreThroughBridgeBody()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        int sid = scene.Take(BridgeVtable);
        // 추상 비트를 세워 연결 탐색을 건너뛰고 공통 사전만 관찰한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        scene.Pool.AllocatedBytes(sid)[40] |= AbstractOrBuried;
        lifecycle.Emit(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 9, 5u));
        Assert.Equal([new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 0, 5u)], scene.CommonLog);
    }

    /// <summary>다리 가상 표의 사후는 다리 몸체를 거쳐 공통 사후로 닫는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitRoutesBridgePostThroughBridgeBody()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        int sid = scene.Take(BridgeVtable);
        // 추상 비트를 세워 탐색·소리를 건너뛰고 공통 사후만 관찰한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        scene.Pool.AllocatedBytes(sid)[40] |= AbstractOrBuried;
        lifecycle.Emit(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 9, 6u));
        Assert.Equal([new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 0, 6u)], scene.CommonLog);
        Assert.Empty(scene.BridgeLog);
    }

    /// <summary>연결 객체 삭제는 외부 재귀 삭제로 연결한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitBridgeForwardsDestroyLink()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        lifecycle.EmitBridge(new BridgeLifecycleEvent(BridgeLifecycleEffect.DestroyLink, 11, 7u));
        Assert.Equal(["destroy:11:7"], scene.Order);
        Assert.Empty(scene.CommonLog);
    }

    /// <summary>다리 Base 앞뒤는 공통 사전·사후로 연결한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitBridgeForwardsBasePreAndPost()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        lifecycle.EmitBridge(new BridgeLifecycleEvent(BridgeLifecycleEffect.BasePreDestroy, 12, 8u));
        lifecycle.EmitBridge(new BridgeLifecycleEvent(BridgeLifecycleEffect.BasePostDestroy, 13, 9u));
        Assert.Equal(
        [
            new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, 12, 0, 8u),
            new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, 13, 0, 9u),
        ], scene.CommonLog);
    }

    /// <summary>walker 낙하는 외부 walker 훅으로 연결한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitBridgeForwardsFallWalker()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        lifecycle.EmitBridge(new BridgeLifecycleEvent(BridgeLifecycleEffect.FallWalker, 14, 0u));
        Assert.Equal(["walker:14"], scene.Order);
        Assert.Empty(scene.CommonLog);
    }

    /// <summary>그 밖의 다리 효과는 파편·소리 외부 효과로 연결한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmitBridgeForwardsRemainderToBridgeEffect()
    {
        var scene = new LifecycleScene();
        var lifecycle = scene.Build();
        var removal = new BridgeLifecycleEvent(BridgeLifecycleEffect.NotifyRemoval, 15, 0u);
        var sound = new BridgeLifecycleEvent(BridgeLifecycleEffect.FallSound, 16, 0u, 1.5f, 2.5f);
        lifecycle.EmitBridge(removal);
        lifecycle.EmitBridge(sound);
        Assert.Equal([removal, sound], scene.BridgeLog);
        Assert.Empty(scene.CommonLog);
        Assert.Empty(scene.Order);
    }
}
