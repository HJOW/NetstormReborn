using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 원본 버튼 전수 관측과 실제 캡처/프레임/수명 통합을 검사한다.

/// <summary>cpppj/tests/ButtonGumpTests.cpp의 세 PE 관측과 독립 계약이다.</summary>
public sealed class ButtonGumpTests
{
    /// <summary>원본 생비트의 signed/unsigned DWORD 입력.</summary>
    private static uint W(string value) => ButtonGumpFixture.Word(value);

    /// <summary>세 판본742행의 모든 노드/표/콜백 당시 상태/시각 QWORD를 기대값 그대로 대조한다.</summary>
    [Theory]
    [InlineData("originals", 252)]
    [InlineData("originalCD", 245)]
    [InlineData("original1037", 245)]
    public void NativeBodies(string edition, int expectedCount)
    {
        string[][] rows = X86Fixture.Read("buttongump", edition); Assert.Equal(expectedCount, rows.Length);
        // 각 행만 독립 상태이며 같은 행 안의 연속 action은 상태를 공유한다.
        foreach (string[] r in rows)
        {
            Assert.Equal(13, r.Length); var s = new ButtonGumpFixture(); s.Load(edition, r); var results = new List<string>();
            // 실제 공개 함수에 입력 전체 인자를 전달하고 예상 선택/기한/전파는 계산하지 않는다.
            foreach (string action in r[2].Split(';'))
            {
                string[] parts = action.Split(','); string op = parts[0]; uint[] a = parts.Skip(1).Select(W).ToArray(); uint t = a[0]; string value = "-";
                switch (op)
                {
                    case "Construct": value = s.Button.ConstructBase(t, unchecked((int)a[1]), unchecked((int)a[2]), a[3]).ToString(); break;
                    case "Value": s.Button.SetValue(t, a[1]); break; case "Pressed": s.Button.SetPressed(t, a[1]); break;
                    case "Enabled": s.Button.SetEnabled(t, a[1] != 0); break; case "Pulse": s.Button.SetPulse(t, a[1] != 0); break; case "Blink": s.Button.SetBlink(t, a[1]); break;
                    case "Handle": value = s.Button.Handle(t, a[1], a[2], a[3]).ToString(); break;
                    case "Broadcast": s.Button.Broadcast(t, a[1], a[2], a[3]); break; case "Idle": s.Button.Idle(t); break;
                    case "Contains": value = s.Button.Contains(t, (BitConverter.UInt32BitsToSingle(a[1]), BitConverter.UInt32BitsToSingle(a[2]))) ? "1" : "0"; break;
                    case "Local": var p = s.Collection.ScreenToLocal(t, new(unchecked((int)a[2]), unchecked((int)a[3]))); value = BitConverter.SingleToUInt32Bits(p.X) + "," + BitConverter.SingleToUInt32Bits(p.Y); break;
                    default: throw new ArgumentException("버튼 fixture action 오류");
                }
                results.Add(op + ':' + value);
            }
            Assert.Equal(r[7], string.Join(';', results)); Assert.Equal(r[8], s.Nodes()); Assert.Equal(r[9], s.Globals()); Assert.Equal(r[10], s.Calls.Count == 0 ? "-" : string.Join('|', s.Calls));
        }
    }

    /// <summary>트리 전달→캡처→프레임 통지→뗌/해제와 실제 배경 눌림 상태가 같은 저장소를 사용한다.</summary>
    [Fact]
    public void CaptureFrameAndVisualState()
    {
        var s = new ButtonGumpFixture(); s.Basic(); uint token = ButtonGumpFixture.Tokens[1]; ButtonGumpNode n = s.Records[token].Node;
        Assert.Equal(token, s.Input.Route(InputCode.LeftButton, 1, 1)); Assert.Equal(token, s.LifeState.Focus); Assert.Equal(1U, n.Pressed); Assert.Equal(0x180U, n.Flags & 0x180); Assert.NotEqual(0U, s.Records[ButtonGumpFixture.Tokens[5]].BackgroundFlags & GumpBackgroundFlags.Pressed);
        ButtonVisualState visual = s.Button.VisualState(token, 37, 14); Assert.Equal(1, visual.Pressed); Assert.Equal(0, visual.Value); Assert.Equal(0, visual.Rect.Left);
        s.Frame.Broadcast(0, 100, 100); Assert.Equal(0U, n.Pressed); Assert.Equal(token, s.LifeState.Focus); s.Frame.Broadcast(0, 1, 1); Assert.Equal(1U, n.Pressed);
        Assert.Equal(token, s.Input.Route(InputCode.Release | InputCode.LeftButton, 1, 1)); Assert.Equal(0U, s.LifeState.Focus); Assert.Equal(0U, n.Pressed); Assert.Equal(0U, n.Flags & 0x180); Assert.Equal(0U, s.Records[ButtonGumpFixture.Tokens[5]].BackgroundFlags & GumpBackgroundFlags.Pressed);
        int calls = s.Calls.Count; s.Frame.Idle(); Assert.Equal(calls, s.Calls.Count);
    }

    /// <summary>pulse 해제가 현재idle 표를 지워도 이미 실행 중인 반복 통지 후반은 계속한다.</summary>
    [Fact]
    public void PulseAndRepeatReadCurrentState()
    {
        var s = new ButtonGumpFixture(); s.Basic(); uint t = ButtonGumpFixture.Tokens[1]; ButtonGumpNode n = s.Records[t].Node; n.Pressed = 1; s.Button.SetPulse(t, true); s.Frame.Idle();
        Assert.Equal(0U, n.ButtonFlags & 0x10); Assert.Equal(0U, n.Flags & 0x80); Assert.Contains(s.Calls, c => c.StartsWith("Parent,")); Assert.Equal(1.05, n.RepeatDeadline);
        s.Calls.Clear(); n.ButtonFlags = 0; n.Pressed = 1; n.FirstDeadline = n.RepeatDeadline = 0; s.Button.Idle(t); Assert.Contains(s.Calls, c => c.StartsWith("Parent,")); Assert.Equal(1.05, n.RepeatDeadline);
    }

    /// <summary>팝업은 visible/void와 무관하게 같은 지연 삭제 큐에 들어가고 캡처는 실제 버튼으로 이동한다.</summary>
    [Fact]
    public void PopupKillUsesSharedStorage()
    {
        var s = new ButtonGumpFixture(); s.Basic(); uint t = ButtonGumpFixture.Tokens[1], popup = ButtonGumpFixture.Tokens[2]; s.Records[t].Node.Next = popup; s.Records[popup].Node.Flags = 0x4008; s.LifeState.Focus = popup;
        s.Button.Handle(t, InputCode.LeftButton, 1, 1); Assert.NotEqual(0U, s.Records[popup].Node.Flags & 0x10000); Assert.Equal(1U, s.LifeState.KillCount); Assert.Equal(popup, s.LifeState.Kill[0]); Assert.Equal(t, s.LifeState.Focus); Assert.Equal(1U, s.Records[t].Node.Pressed);
    }

    /// <summary>로컬 좌표는 루트 위치를 빼지 않고 월드 부모의 기준점 뒤 일반 위치도 생략한다.</summary>
    [Fact]
    public void LocalRootAndWorldParent()
    {
        var s = new ButtonGumpFixture(); s.Basic(); uint t = ButtonGumpFixture.Tokens[1], parent = ButtonGumpFixture.Tokens[2]; s.Records[ButtonGumpFixture.Tokens[0]].Node.X = 100; s.Records[t].Node.X = 10.25f;
        Assert.Equal((.75f, 1f), s.Collection.ScreenToLocal(t, new(11, 1))); ButtonGumpNode p = s.Records[parent].Node; p.Flags |= 0x2000; p.Object = 2; p.X = -.75f; p.Y = .25f; s.Records[t].Node.Parent = parent; s.Anchors[2] = new(17, -19); Assert.Equal((1f, 1f), s.Collection.ScreenToLocal(t, new(18, -18)));
    }

    /// <summary>순환/없는토큰/공유저장소 오류와 콜백 예외 전 쓰기를 확인한다.</summary>
    [Fact]
    public void ContractsAndCallbackExceptions()
    {
        var s = new ButtonGumpFixture(); s.Basic(); uint t = ButtonGumpFixture.Tokens[1]; ButtonGumpHooks h = s.Hooks(); h.Redraw = (token, _) => { s.Records[token].Node.Value = 7; throw new InvalidOperationException("의도한 콜백 예외"); };
        var throwing = new ButtonGump(s.Create, s.State, s.Factory, s.Collection, s.Events, s.Input, s.Lifetime, h); Assert.Throws<InvalidOperationException>(() => throwing.SetValue(t, 1)); Assert.Equal(7U, s.Records[t].Node.Value);
        s.Records[t].Node.Parent = t; Assert.Throws<InvalidOperationException>(() => s.Collection.ScreenToLocal(t, new(1, 1))); Assert.Throws<InvalidOperationException>(() => s.Button.Handle(0, InputCode.LeftButton, 1, 1));
        h.BaseNode = _ => new(); var mismatch = new ButtonGump(s.Create, s.State, s.Factory, s.Collection, s.Events, s.Input, s.Lifetime, h); Assert.Throws<InvalidOperationException>(() => mismatch.SetValue(t, 1));
    }

    /// <summary>버튼 외 입력/프레임 대상은 각 층의 기존 처리기와 전체 반환DWORD를 유지한다.</summary>
    [Fact]
    public void BindPreservesEachFallback()
    {
        var calls = new List<string>(); var e = new GumpEventHooks { Handle = (_, _, _, _) => { calls.Add("event"); return 0x81234567; } }; var i = new GumpInputHooks { Handle = (_, _, _, _) => { calls.Add("input"); return 0xfedcba98; } }; var f = new GumpFrameHooks { Idle = _ => calls.Add("idle"), Broadcast = (_, _, _, _) => calls.Add("broadcast") };
        ButtonGump.BindButtonGump(e, i, f, _ => null); Assert.Equal(0x81234567U, e.Handle!(9, 1, 2, 3)); Assert.Equal(0xfedcba98U, i.Handle!(9, 1, 2, 3)); f.Idle!(9); f.Broadcast!(9, 1, 2, 3); Assert.Equal(new[] { "event", "input", "idle", "broadcast" }, calls);
    }
}
