using Netstorm.Client.Input;

namespace Netstorm.Tests;

/// <summary>
/// 원본 입력 사건 큐(00452f90·00452f00·00452fe0)의 순서·넘침·거르기·명령 코드를 검사한다. cpppj/tests/ScreenTests.cpp 의 InputQueue 검사 세 개를
/// 같은 입력과 기대값으로 옮긴 것이다. (2026-10-28 R1에서 이식)
/// </summary>
public sealed class InputQueueTests
{
    /// <summary>넣은 순서로 나오고, 빈 큐는 0으로 채운 사건을 준다.</summary>
    [Fact]
    public void PushAndPopKeepOrder()
    {
        var queue = new InputQueue();
        Assert.True(queue.Empty);
        Assert.Equal(default, queue.Pop(false));
        queue.Push(InputCode.LeftButton, 10, 20);
        queue.Push(InputCode.RightButton | InputCode.Shift, 30, 40);
        Assert.False(queue.Empty);
        Assert.Equal(new InputEvent(InputCode.LeftButton, 10, 20), queue.Pop(false));
        Assert.Equal(new InputEvent(InputCode.RightButton | InputCode.Shift, 30, 40), queue.Pop(false));
        Assert.True(queue.Empty);
    }

    /// <summary>40칸 고리에는 39개까지 담긴다. 가득 차면 새로 넣은 것을 버리고, 고리를 한 바퀴 돈 뒤에도 순서가 유지된다.</summary>
    [Fact]
    public void OverflowDropsNewest()
    {
        var queue = new InputQueue();
        // 칸 수보다 많이 넣는다.
        for (uint i = 1; i <= 50; i++) queue.Push(i, (int)i, 0);
        // 앞의 39개가 순서대로 남는다.
        for (uint i = 1; i <= InputQueue.Capacity - 1; i++) Assert.Equal(i, queue.Pop(false).Code);
        Assert.True(queue.Empty);
        queue.Push(100, 0, 0);
        queue.Push(101, 0, 0);
        Assert.True(queue.Pop(false).Code == 100 && queue.Pop(false).Code == 101 && queue.Empty);
    }

    /// <summary>뗌 사건 거르기, DEL → Ctrl+Backspace, 명령 사건의 코드 다듬기.</summary>
    [Fact]
    public void ReleaseFilterDeleteAndCommandMatchOriginal()
    {
        var queue = new InputQueue();
        // 걸러진 뗌 사건도 큐에서는 빠지고, 돌려주는 값은 0이다.
        queue.Push(InputCode.LeftButton | InputCode.Release, 1, 2);
        queue.Push(InputCode.LeftButton, 3, 4);
        Assert.Equal(default, queue.Pop(true));
        Assert.Equal(new InputEvent(InputCode.LeftButton, 3, 4), queue.Pop(true));
        // 거르지 않으면 뗌 사건이 그대로 나온다.
        queue.Push(InputCode.RightButton | InputCode.Release, 5, 6);
        Assert.Equal(InputCode.RightButton | InputCode.Release, queue.Pop(false).Code);
        // 글자 0x7f(DEL)는 Ctrl+Backspace(8)가 된다. 보조 키 가운데 Ctrl 자리만 새로 켜진다.
        queue.Push(0x7f, 0, 0);
        Assert.Equal(InputCode.Control | 8u, queue.Pop(false).Code);
        queue.Push(InputCode.Release | 0x7f, 0, 0);
        Assert.Equal(InputCode.Release | InputCode.Control | 8u, queue.Pop(false).Code);
        // 가상 키 0x7f(F16)는 글자가 아니므로 바뀌지 않는다.
        queue.Push(0x7fu << InputCode.VirtualKeyShift, 0, 0);
        Assert.Equal(0x7fu << InputCode.VirtualKeyShift, queue.Pop(false).Code);
        // 명령 사건: 0x0100ffff만 남기고 명령 표시를 붙인다.
        queue.PushCommand(0xffffffffu, 7, 8);
        Assert.Equal(new InputEvent(0x0100ffffu | InputCode.Command, 7, 8), queue.Pop(false));
    }
}
