using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 캡처/dialog/overlay의 실제 하위 선택/수명 연결을 검증하는 영역이다.

/// <summary>GumpInput 전체 상태/두 표/원본 반환/가상 호출 당시 상태를 검증한다.
/// 근거: cpppj/tests/GumpInputTests.cpp.</summary>
public sealed class GumpInputTests
{
    /// <summary>각 판본232행을 같은 현재 노드/캡처/dialog/overlay/선택 모듈로 재생한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeInput(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpinput", edition); Assert.Equal(232, rows.Length);
        // 각 행의 연속 명령만 현재 상태를 공유한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(16, row.Length); var scene = new GumpEventFixture(edition, row, input: true);
            var collection = new GumpCollection(scene.Create.Collection, scene.CollectionHooks());
            var events = new GumpEvents(scene.Create, scene.Lifetime, scene.Event, collection, scene.EventHooks(row[6] == "1"));
            var input = new GumpInput(scene.Create, scene.Lifetime, scene.Input, collection, events, scene.InputHooks());
            var lifetime = new GumpLifetime(scene.Create, scene.Lifetime, new() { Node = token => scene.Nodes.GetValueOrDefault(token) }); var results = new List<string>();
            // void의 불확정 반환은 비교하지 않고 최상위 공개 연산만 실제 모듈로 호출한다.
            foreach (string item in row[5].Split(';'))
            {
                string[] p = item.Split(','); uint[] a = p[1..].Select(GumpEventFixture.Word).ToArray(); string result = "-";
                switch (p[0])
                {
                    case "Capture": input.Capture(a[0]); break; case "Override": input.SetOverride(a[0], a[1] != 0); break;
                    case "Dialog": result = input.AddDialog(a[1]) ? "1" : "0"; break; case "Remove": result = lifetime.RemoveDialog(a[1]) ? "1" : "0"; break;
                    case "Route": result = input.Route(a[1], a[2], a[3], a[4] != 0).ToString(CultureInfo.InvariantCulture); break;
                    case "Overlay": result = input.Overlay(a[1], new(unchecked((int)a[2]), unchecked((int)a[3]))).ToString(CultureInfo.InvariantCulture); break;
                    default: throw new InvalidDataException("입력 연산 이름 오류");
                }
                results.Add(result);
            }
            scene.CheckInput(row, results);
        }
    }

    /// <summary>실제 등록/캡처/Push/삭제 요청과 전체 커서 선택이 같은 생성 저장소를 사용한다.</summary>
    [Fact]
    public void CreateCaptureDialogLifetimeIntegration()
    {
        var scene = new GumpEventRig(); scene.Rig.Nodes[2].Flags |= 0x400; scene.Rig.Nodes[3].Flags |= 0x400;
        GumpEvents events = scene.Events(new() { Handle = (_, _, _, _) => 0x80000007 }); GumpInput input = scene.Input(events, new() { Handle = (_, _, _, _) => 0x80000007 });
        Assert.Equal(3U, input.Route(1, 2, 3)); input.Capture(4); input.Capture(2); Assert.Equal(4U, scene.Rig.LifeState.Focus); Assert.Equal(4U, input.Route(1, 2, 3)); Assert.Equal(0U, scene.Rig.LifeState.Focus);
        Assert.True(input.AddDialog(2)); Assert.Equal(2U, input.Route(1, 2, 3)); Assert.True(input.AddDialog(3)); Assert.False(input.AddDialog(2)); Assert.Equal(3U, scene.Rig.LifeState.CurrentDialog);
        Assert.True(scene.Rig.Lifetime.RemoveDialog(3)); Assert.Equal(2U, scene.Rig.LifeState.CurrentDialog); input.Capture(4); scene.Rig.Lifetime.Kill(2);
        Assert.Equal(0U, scene.Rig.LifeState.Focus); Assert.Equal(0U, scene.Rig.LifeState.CurrentDialog); Assert.Equal(2U, scene.Rig.LifeState.KillCount); Assert.Equal(0U, scene.Rig.Nodes[3].Next);
        Assert.Equal(3U, input.Route(1, 2, 3)); Assert.Equal(3U, events.Pick(new(12, 12), 0)); scene.Rig.Lifetime.SetOverlay(3, true); scene.InputState.OverlayEvent = 123;
        Assert.Equal(0x80000007U, input.Overlay(999, new(12, 12))); input.SetOverride(3, true); Assert.Equal(3U, scene.Rig.State.Collection.OverrideHead);
        input.SetOverride(2, true); Assert.Equal(0U, scene.Rig.State.Collection.OverrideHead); scene.Rig.Lifetime.SetOverlay(3, false);
    }

    /// <summary>캡처 실패 뒤 재전달이 없고 성공 뒤에는 콜백이 바꾼 현재 캡처를 반환한 후 지운다.</summary>
    [Fact]
    public void CaptureCurrentValueAndNoFallback()
    {
        var scene = new GumpEventRig(); var handled = new List<uint>(); int filters = 0; bool success = false;
        GumpEvents events = scene.Events(new() { Filter = (_, _, _, _) => { filters++; return true; }, Handle = (_, _, _, _) => 0 });
        GumpInput input = scene.Input(events, new() { Handle = (token, _, _, _) => { handled.Add(token); scene.Rig.LifeState.Focus = success ? 3U : 2U; return success ? 7U : 0; } });
        input.Capture(4); Assert.Equal(0U, input.Route(1, 2, 3)); Assert.Equal(2U, scene.Rig.LifeState.Focus); Assert.Equal(new uint[] { 4 }, handled); Assert.Equal(0, filters);
        success = true; Assert.Equal(3U, input.Route(1, 2, 3)); Assert.Equal(0U, scene.Rig.LifeState.Focus); Assert.Equal(new uint[] { 4, 2 }, handled); Assert.Equal(0, filters);
    }

    /// <summary>overlay는 부모 visible/실제 Pick/별도 사건 전역을 사용하며 첫 일치의0 반환에서도 끝낸다.</summary>
    [Fact]
    public void OverlayActualPickParentsAndZeroReturn()
    {
        var scene = new GumpEventRig(); scene.Rig.Nodes[2].Depth = 1; scene.Rig.Nodes[3].Depth = 5; scene.Rig.Nodes[4].Depth = 2;
        scene.Rig.LifeState.Overlay[0] = 2; scene.InputState.OverlayEvent = 123; int calls = 0; uint received = 0; uint value = 0x80000007;
        GumpInput input = scene.Input(scene.Events(), new() { Handle = (_, e, _, _) => { calls++; received = e; return value; } });
        Assert.Equal(value, input.Overlay(999, new(12, 12))); Assert.Equal(1, calls); Assert.Equal(123U, received);
        scene.Rig.Nodes[1].Flags = 0; scene.Rig.Nodes[2].Cached = new(99, 99); Assert.Equal(0U, input.Overlay(999, new(12, 12))); Assert.Equal(99, scene.Rig.Nodes[2].Cached.X); Assert.Equal(1, calls);
        scene.Rig.Nodes[1].Flags = 0x10001; Assert.Equal(value, input.Overlay(999, new(12, 12))); Assert.Equal(2, calls);
        scene.Rig.Nodes[2].Flags = 9; Assert.Equal(0U, input.Overlay(999, new(12, 12))); Assert.Equal(2, calls);
        scene.Rig.Nodes[2].Flags = 1; scene.Rig.Nodes[2].Depth = 10; Assert.Equal(0U, input.Overlay(999, new(12, 12))); Assert.Equal(4U, scene.State.Selected); Assert.Equal(2, calls);
        scene.Rig.LifeState.Overlay[99] = 4; Assert.Equal(value, input.Overlay(999, new(12, 12))); Assert.Equal(3, calls); Assert.Equal(0U, input.Overlay(999, new(19, 12)));
        scene.Rig.Nodes[2].Depth = 1; scene.Rig.LifeState.Overlay[99] = 2; value = 0; int before = calls;
        Assert.Equal(0U, input.Overlay(999, new(12, 12))); Assert.Equal(before + 1, calls);
    }

    /// <summary>가득 찬 표는 중복 검색 전에 로그를 호출하며 비활성 마지막 칸은 중복으로 세지 않는다.</summary>
    [Fact]
    public void DialogCapacityBeforeDuplicate()
    {
        var scene = new GumpEventRig(); int overflows = 0;
        GumpInput input = scene.Input(scene.Events(), new() { Overflow = code => { Assert.Equal(19U, code); overflows++; scene.Rig.LifeState.CurrentDialog = 4; } });
        Array.Fill(scene.Rig.LifeState.Dialogs, 3U); scene.Rig.LifeState.DialogCount = 16; Assert.False(input.AddDialog(3));
        Assert.Equal(1, overflows); Assert.Equal(16U, scene.Rig.LifeState.DialogCount); Assert.Equal(4U, scene.Rig.LifeState.CurrentDialog);
        scene.Rig.LifeState.DialogCount = 15; scene.Rig.LifeState.Dialogs[15] = 2; Assert.True(input.AddDialog(2)); Assert.Equal(16U, scene.Rig.LifeState.DialogCount); Assert.Equal(2U, scene.Rig.LifeState.CurrentDialog);
    }

    /// <summary>없는 토큰/부모 순환/누락 경계/음수 dialog 개수는 명시 오류로 거부한다.</summary>
    [Fact]
    public void Contracts()
    {
        var scene = new GumpEventRig(); GumpEvents events = scene.Events(); GumpInput input = scene.Input(events);
        Assert.Throws<ArgumentException>(() => new GumpInput(scene.Rig.State, scene.Rig.LifeState, scene.InputState, scene.Collection, events, new()));
        input.Capture(99); Assert.Throws<InvalidOperationException>(() => input.Route(1, 2, 3)); scene.Rig.LifeState.Focus = 0;
        scene.Rig.LifeState.Overlay[0] = 99; Assert.Throws<InvalidOperationException>(() => input.Overlay(1, new(12, 12))); scene.Rig.LifeState.Overlay[0] = 2;
        scene.Rig.Nodes[2].Parent = 2; Assert.Throws<InvalidOperationException>(() => input.Overlay(1, new(12, 12))); scene.Rig.Nodes[2].Parent = 1;
        scene.Rig.LifeState.DialogCount = uint.MaxValue; Assert.Throws<ArgumentOutOfRangeException>(() => input.AddDialog(2)); scene.Rig.LifeState.Focus = 2;
        Assert.Throws<ArgumentException>(() => input.Route(1, 2, 3)); scene.Rig.LifeState.DialogCount = 16; Assert.Throws<ArgumentException>(() => input.AddDialog(2));
    }

    /// <summary>처리기 예외 이전의 캡처/노드 쓰기는 그대로 유지한다.</summary>
    [Fact]
    public void CallbackExceptionKeepsWrites()
    {
        var scene = new GumpEventRig(); GumpInput input = scene.Input(scene.Events(), new()
        {
            Handle = (token, _, _, _) => { scene.Rig.Nodes[token].Command = 999; scene.Rig.LifeState.Focus = 3; throw new InvalidOperationException("입력 처리기 실패"); },
        });
        input.Capture(2); Assert.Throws<InvalidOperationException>(() => input.Route(1, 2, 3)); Assert.Equal(999U, scene.Rig.Nodes[2].Command); Assert.Equal(3U, scene.Rig.LifeState.Focus);
    }
}
