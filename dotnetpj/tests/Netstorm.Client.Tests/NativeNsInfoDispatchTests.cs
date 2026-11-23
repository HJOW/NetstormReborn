// 세 NSInfo 파생 입력은 같은 실제 저장소와 각 전체 원문 반환/관문을 사용한다.
// cpppj/src/client/RawMenuControls.cpp 의 NsHandle 분배 뼈대를 이식한다.
// 0.919에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 소유 판정과 파생 처리 호출의 계약이다.</summary>
public sealed class NativeNsInfoDispatchTests
{
    /// <summary>같은 실제 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 파생 처리 호출이다.</summary>
        public List<(string Kind, uint Event, uint First, uint Second)> Calls { get; } = [];

        /// <summary>실제 기본 처리 호출이다.</summary>
        public List<(uint Token, uint Event, uint First, uint Second)> Based { get; } = [];

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativeNsInfoDispatchHooks Hooks() => new()
        {
            IsRoot = token => token == 1,
            IsPlayer = token => token is >= 10 and < 20,
            IsBattle = token => token == 30,
            Root = (@event, first, second) => { Calls.Add(("root", @event, first, second)); return 11u; },
            Player = (@event, first, second) => { Calls.Add(("player", @event, first, second)); return 22u; },
            Battle = (@event, first, second) => { Calls.Add(("battle", @event, first, second)); return 33u; },
            Base = (token, @event, first, second) => { Based.Add((token, @event, first, second)); return 44u; },
        };
    }

    /// <summary>소유 순서대로 파생 처리에 보낸다. 0.919에서 추가됐다.</summary>
    [Fact]
    public void RoutesByOwnershipInOrder()
    {
        var scene = new Scene();
        Assert.Equal(11u, NativeNsInfoDispatch.Dispatch(1, 7, 8, 9, scene.Hooks()));
        Assert.Equal(22u, NativeNsInfoDispatch.Dispatch(15, 7, 8, 9, scene.Hooks()));
        Assert.Equal(33u, NativeNsInfoDispatch.Dispatch(30, 7, 8, 9, scene.Hooks()));
        Assert.Equal([("root", 7u, 8u, 9u)], scene.Calls.Where(call => call.Kind == "root"));
        Assert.Equal([("player", 7u, 8u, 9u)], scene.Calls.Where(call => call.Kind == "player"));
        Assert.Equal([("battle", 7u, 8u, 9u)], scene.Calls.Where(call => call.Kind == "battle"));
        Assert.Empty(scene.Based);
    }

    /// <summary>어느 파생에도 속하지 않으면 같은 기본 처리에 전달한다. 0.919에서 추가됐다.</summary>
    [Fact]
    public void FallsThroughToBase()
    {
        var scene = new Scene();
        Assert.Equal(44u, NativeNsInfoDispatch.Dispatch(99, 7, 8, 9, scene.Hooks()));
        Assert.Empty(scene.Calls);
        Assert.Equal([(99u, 7u, 8u, 9u)], scene.Based);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.919에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => NativeNsInfoDispatch.Dispatch(1, 7, 8, 9, new NativeNsInfoDispatchHooks()));
        var hooks = scene.Hooks();
        hooks.Root = null;
        Assert.Throws<InvalidOperationException>(() => NativeNsInfoDispatch.Dispatch(1, 7, 8, 9, hooks));
        var baseHooks = scene.Hooks();
        baseHooks.Base = null;
        Assert.Throws<InvalidOperationException>(() => NativeNsInfoDispatch.Dispatch(99, 7, 8, 9, baseHooks));
        Assert.Throws<ArgumentNullException>(() => NativeNsInfoDispatch.Dispatch(1, 7, 8, 9, null!));
    }
}
