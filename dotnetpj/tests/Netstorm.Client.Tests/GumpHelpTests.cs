using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 기본 우클릭 도움말의 콜백 뒤 현재 값/판본별 인자를 검사한다.

/// <summary>cpppj/tests/GumpTextTests.cpp의 GumpHelp 계약과 미도달 경계다.</summary>
public sealed class GumpHelpTests
{
    /// <summary>시작/제목/항목 콜백이 바꾼 문자열/본문/사건/주제/루트를 다음 호출에서 다시 읽는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentCallbacksAndEditionArguments(OriginalEdition edition)
    {
        var create = new GumpCreateState(); create.Collection.Edition = edition; create.Entries[0] = 91; var node = new GumpCreateNode { AlternateTextPointer = 5 }; var state = new GumpHelpState { Body = 11, Event = 12 }; var calls = new List<string>();
        var h = new GumpHelpHooks { Node = _ => node, Text = t => { Assert.Equal(5U, t); return "priest\0ignored"; }, BeginMenu = a => { Assert.Equal(0U, a); calls.Add("begin"); node.TextPointer = 7; }, Title = title => { Assert.Equal(7U, title); calls.Add("title"); state.Body = 21; state.Event = 22; state.Topic = "changedHelp"; }, Entry = (body, code, topic, a, b, extra) => { Assert.Equal((21U, 22U, "changedHelp", 1U, 1U), (body, code, topic, a, b)); Assert.Equal(edition == OriginalEdition.Patch1078 ? 0U : (uint?)null, extra); calls.Add("entry"); create.Entries[0] = 93; }, AttachMenu = (x, y, parent) => { Assert.Equal((-7, 13, 93U), (x, y, parent)); calls.Add("attach"); }, Sound = name => { Assert.Equal("opengump.wav", name); calls.Add("sound"); } };
        var help = new GumpHelp(create, state, h); var buttons = new ButtonGumpHooks(); GumpHelp.BindGumpHelp(buttons, help); Assert.Equal(1U, buttons.BaseEvent!(1, InputCode.RightButton, 0xfffffff9, 13)); Assert.Equal(new[] { "begin", "title", "entry", "attach", "sound" }, calls);
        Assert.Equal(0U, help.Handle(1, InputCode.RightButton | InputCode.Release, 0, 0)); node.AlternateTextPointer = 0; Assert.Equal(0U, help.Handle(1, InputCode.RightButton, 0, 0)); Assert.Equal(5, calls.Count);
    }
}
