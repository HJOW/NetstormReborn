using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 확정 입력 사건·문자 경계·걸러내기의 계약을 검사한다. cpppj/tests/DisplayTextInputTests.cpp 의
/// 입력경계 두 경우(KoreanInput_Utf8Utf16AndAtomicQueue·KoreanInput_DisplayBoundariesAndFiltering)를
/// 같은 입력과 기대값으로 옮긴 것이다. Gump 연결 경우는 TextEntry 이식 뒤에 잇는다.
/// </summary>
/// <remarks>2026-11-11 0.351에서 DisplayTextInput 이식과 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class DisplayTextInputTests
{
    /// <summary>UTF-8 기대 본문을 기존 표시 경계로만 준비한다. 입력 API의 결과를 기대값으로 재사용하지 않는다.</summary>
    /// <param name="value">UTF-16 본문이다.</param>
    /// <returns>표시 바이트다.</returns>
    private static byte[] Display(string value) => ExtendedText.EncodeDisplayText(value);

    /// <summary>미지원·비정상 입력이 사건을 일부 생성하지 않고 한글마다 단일 태그를 만드는지 확인한다.</summary>
    [Fact]
    public void Utf8Utf16AndAtomicQueue()
    {
        InputEvent[] events = DisplayTextInput.MakeCommittedTextEvents("A한글");
        Assert.Equal(3, events.Length);
        Assert.Equal((uint)'A', events[0].Code);
        Assert.True(DisplayTextInput.IsCommittedTextInput(events[1].Code));
        Assert.Equal(Display("한"), DisplayTextInput.CommittedTextBytes(events[1].Code));
        Assert.Equal(System.Text.Encoding.UTF8.GetBytes("한글"), DisplayTextInput.CommittedUtf16ToUtf8("한글"));
        Assert.Equal("한글", DisplayTextInput.DisplayTextToUtf16(Display("한글")));
        // 각 입력은 전체 검증에서 거부한다
        string[] rejected = ["A\0B", "A\nB", "A" + ((char)0x7f) + "B", "A😀"];
        foreach (string invalid in rejected)
        {
            Assert.Throws<ArgumentException>(() => DisplayTextInput.MakeCommittedTextEvents(invalid));
        }
        Assert.Throws<ArgumentException>(() => DisplayTextInput.CommittedUtf16ToUtf8("\ud800"));
        var queue = new InputQueue();
        // 실제 링에 한 칸만 남긴다
        for (uint i = 0; i < 38; i++) queue.Push((uint)'x', 0, 0);
        Assert.False(queue.TryPushBatch(DisplayTextInput.MakeCommittedTextEvents("한글")));
        Assert.Equal(1, queue.FreeSlots());
        // 실패한 본문의 일부가 들어가지 않았는지 확인한다
        for (uint i = 0; i < 38; i++) Assert.Equal((uint)'x', queue.Pop(false).Code);
        Assert.True(queue.Empty);
        Assert.True(queue.TryPushBatch(events));
        Assert.Equal((uint)'A', queue.Pop(false).Code);
        Assert.True(DisplayTextInput.IsCommittedTextInput(queue.Pop(false).Code));
        Assert.True(DisplayTextInput.IsCommittedTextInput(queue.Pop(false).Code));
        Assert.True(queue.Empty);
    }

    /// <summary>한글 중간의 모든 바이트 위치와 제외목록·길이 상한을 확인한다.</summary>
    [Fact]
    public void DisplayBoundariesAndFiltering()
    {
        byte[] text = Display("A한글B");
        Assert.Equal(10, text.Length);
        // 첫 한글 내부를 양쪽 경계로 보정한다
        for (int at = 2; at < 5; at++)
        {
            Assert.Equal(1, DisplayTextInput.DisplayTextFloor(text, at));
            Assert.Equal(5, DisplayTextInput.DisplayTextCeil(text, at));
        }
        Assert.Equal(1, DisplayTextInput.PreviousDisplayText(text, 5));
        Assert.Equal(5, DisplayTextInput.NextDisplayText(text, 1));
        Assert.Equal(Display("한"), DisplayTextInput.FilterDisplayText(text, [(byte)'A'], 7));
        Assert.Equal([(byte)'A', (byte)'B'], DisplayTextInput.FilterDisplayText(text, [0x1f], 100));
        Assert.Equal(Display("갈"), DisplayTextInput.FilterDisplayText(Display("갈"), [11], 100));
        Assert.Equal(Display("한글"), DisplayTextInput.DisplayTextLine(Display("a\n한글\nb"), 1).ToArray());
    }

    /// <summary>text 스크립트 동작은 확정 본문을 원자적으로 큐에 넣는다.</summary>
    [Fact]
    public void TextScriptOperationPushesCommittedBatch()
    {
        var screen = new Netstorm.Client.Display.Screen(8, 8);
        var input = new InputQueue();
        var runner = new Netstorm.Client.Automation.UiScriptRunner(
            Netstorm.Client.Automation.UiScript.Parse(["1\ttext\tA한"]), input, screen);
        runner.BeforeInput(1);
        Assert.Equal((uint)'A', input.Pop(false).Code);
        Assert.True(DisplayTextInput.IsCommittedTextInput(input.Pop(false).Code));
        Assert.True(input.Empty);
    }
}
