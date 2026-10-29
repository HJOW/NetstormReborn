using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 커서 선택/사건 전달과 실제 수명 연결을 검증하는 영역이다.

/// <summary>GumpEvents의 전체 노드/전역/콜백 순서를 원본 세 PE 관측과 대조한다.
/// 근거: cpppj/tests/GumpEventsTests.cpp.</summary>
public sealed class GumpEventsTests
{
    /// <summary>각 판본257행의 기본 필터·직계/전체 선택·재귀/부모 전달을 같은 입력으로 재생한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeEvents(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpevents", edition); Assert.Equal(257, rows.Length);
        // 한 행의 연속 연산만 현재 노드/전역을 공유한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(14, row.Length); var scene = new GumpEventFixture(edition, row, input: false);
            var collection = new GumpCollection(scene.Create.Collection, scene.CollectionHooks());
            var events = new GumpEvents(scene.Create, scene.Lifetime, scene.Event, collection, scene.EventHooks(row[5] == "1")); var results = new List<uint>();
            // 원본의 완성 최상위 입력만 전달하고 예상 선택/처리는 계산하지 않는다.
            foreach (string item in row[4].Split(';'))
            {
                string[] p = item.Split(','); uint[] a = p[1..].Select(GumpEventFixture.Word).ToArray();
                results.Add(p[0] switch
                {
                    "Base" => events.BaseFilter(a[0], a[1], a[2], a[3]) ? 1U : 0,
                    "Top" => events.PickTopLevel(new(unchecked((int)a[1]), unchecked((int)a[2]))),
                    "Pick" => events.Pick(new(unchecked((int)a[1]), unchecked((int)a[2])), a[3]),
                    "Dispatch" => events.Dispatch(a[0], a[1], a[2], a[3], a[4] != 0), "Bubble" => events.Bubble(a[0], a[1], a[2], a[3]),
                    _ => throw new InvalidDataException("이벤트 연산 입력 오류"),
                });
            }
            scene.Check(row, results);
        }
    }

    /// <summary>현재 위치/깊이와 실제 지연 삭제가 후속 선택/기본 dialog 필터에 즉시 반영된다.</summary>
    [Fact]
    public void CreateTreeLifetimeIntegration()
    {
        var scene = new GumpEventRig(); var handled = new List<uint>();
        GumpEvents events = scene.Events(new() { Handle = (token, _, _, _) => { handled.Add(token); return 7; } });
        scene.Rig.Nodes[3].Depth = 4; scene.Rig.Nodes[2].Depth = 5; scene.Rig.Nodes[4].Depth = 3;
        Assert.Equal(3U, events.PickTopLevel(new(12, 12))); Assert.Equal(4U, events.Pick(new(12, 12), 0)); Assert.Equal(new ScreenPoint(11, 11), scene.Rig.Nodes[4].Cached);
        scene.Rig.Nodes[3].Flags |= 0x400; Assert.Equal(3U, events.Dispatch(1, 1, 2, 3)); Assert.Equal(new uint[] { 3 }, handled);
        scene.Rig.LifeState.Dialogs[0] = 4; scene.Rig.LifeState.DialogCount = 1; scene.Rig.LifeState.CurrentDialog = 4;
        Assert.True(events.BaseFilter(4, uint.MaxValue, 999, 999)); scene.Rig.Lifetime.Kill(2);
        Assert.Equal(3U, events.Pick(new(12, 12), 0)); Assert.Equal(0U, scene.Rig.LifeState.CurrentDialog); Assert.Equal(2U, scene.Rig.LifeState.KillCount);
    }

    /// <summary>전달은 visible만 제한하고 선택은 dead/void를 제외한다. Dispatch 성공 토큰과 Bubble raw 반환을 구별한다.</summary>
    [Fact]
    public void DispatchUsesItsOwnFlagsAndReturnContract()
    {
        var scene = new GumpEventRig(); GumpEventHooks hooks = new() { Filter = (_, _, _, _) => true, Handle = (token, _, _, _) => token == 3 ? 0x80000007U : 0 };
        GumpEvents events = scene.Events(hooks); scene.Rig.Nodes[3].Flags = 0x10009;
        Assert.Equal(3U, events.Dispatch(1, 1, 2, 3)); Assert.NotEqual(3U, events.Pick(new(12, 12), 0));
        scene.Rig.Nodes[3].Flags = 0; Assert.Equal(0U, events.Dispatch(1, 1, 2, 3)); Assert.Equal(3U, events.Dispatch(1, 1, 2, 3, true));
        hooks.Handle = (token, _, _, _) => token == 2 ? 0x80000007U : 0;
        Assert.Equal(0x80000007U, scene.Events(hooks).Bubble(4, 1, 2, 3));
    }

    /// <summary>help 사건은 화면 밖에서 다른 입력 플래그로 통과하지 않고 오른쪽/아래 끝은 제외한다.</summary>
    [Fact]
    public void BaseFilterDialogAndHalfOpenBounds()
    {
        var scene = new GumpEventRig(); GumpEvents events = scene.Events(); scene.Rig.Nodes[2].Flags |= 0xe00;
        Assert.True(events.BaseFilter(2, 0x4010000, 10, 10)); Assert.False(events.BaseFilter(2, 0x10000, 30, 10));
        Assert.False(events.BaseFilter(2, 0x20000, 10, 26)); Assert.False(events.BaseFilter(2, 0x10000, 9, 10));
        Assert.True(events.BaseFilter(2, 1, 0, 0)); Assert.True(events.BaseFilter(2, 0x2000001, 999, 999));
        scene.Rig.LifeState.CurrentDialog = 2; Assert.True(events.BaseFilter(2, 0x10000, 999, 999));
    }

    /// <summary>잘못된 토큰·빠진 처리기·부모/형제/자식 순환은 원본 포인터 계약 밖이므로 거부한다.</summary>
    [Fact]
    public void ContractsAndCycles()
    {
        var scene = new GumpEventRig(); GumpEvents events = scene.Events(new() { Handle = (_, _, _, _) => 0 });
        Assert.Throws<ArgumentException>(() => new GumpEvents(scene.Rig.State, scene.Rig.LifeState, scene.State, scene.Collection, new()));
        Assert.Throws<InvalidOperationException>(() => events.Dispatch(99, 1, 2, 3)); Assert.Throws<ArgumentException>(() => scene.Events().Bubble(2, 1, 2, 3));
        scene.Rig.Nodes[2].Parent = 2; Assert.Throws<InvalidOperationException>(() => events.Bubble(2, 1, 2, 3)); scene.Rig.Nodes[2].Parent = 1;
        scene.Rig.Nodes[3].Next = 3; Assert.Throws<InvalidOperationException>(() => events.PickTopLevel(new(12, 12))); scene.Rig.Nodes[3].Next = 2;
        scene.Rig.Nodes[4].Child = 2; Assert.Throws<InvalidOperationException>(() => events.Dispatch(2, 1, 2, 3));
    }

    /// <summary>필터 예외 전에 기록한 현재 노드/전역 쓰기를 원복하지 않는다.</summary>
    [Fact]
    public void CallbackExceptionKeepsCurrentWrites()
    {
        var scene = new GumpEventRig(); GumpEvents events = scene.Events(new()
        {
            Filter = (token, _, _, _) => { scene.Rig.Nodes[token].Command = 999; scene.Rig.LifeState.CurrentDialog = token; throw new InvalidOperationException("필터 실패"); },
            Handle = (_, _, _, _) => 0,
        });
        Assert.Throws<InvalidOperationException>(() => events.Dispatch(3, 1, 2, 3)); Assert.Equal(999U, scene.Rig.Nodes[3].Command); Assert.Equal(3U, scene.Rig.LifeState.CurrentDialog);
    }
}
