using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

/// <summary>
/// 한글 입력의 커서·선택·삭제·상한·붙여넣기·자동완성 문자 경계를 검사한다.
/// cpppj/tests/DisplayTextInputTests.cpp 의 Gump 네 경우(EditCursorSelectionAndDeletion·ByteLimitAndNumericExclusion·
/// PasteWholeCharacters·ConsoleCompletionWordBoundary·AsciiPasteIntoExtendedBodyHonorsFinalByteLimit)를 같은 입력과
/// 기대값으로 옮긴 것이다. 그리기·스크롤 픽셀은 글꼴 표시(R5) 뒤에 잇는다.
/// </summary>
/// <remarks>2026-11-11 0.352에서 TextEntry 확장 경계 이식과 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class KoreanTextEntryTests
{
    /// <summary>UTF-16 본문을 NUL 종료 표시 바이트로만 준비한다.</summary>
    /// <param name="value">본문이다.</param>
    /// <returns>NUL 종료 표시 바이트다.</returns>
    private static byte[] DisplayTerminated(string value) => [.. ExtendedText.EncodeDisplayText(value), (byte)0];

    /// <summary>현재 버퍼 본문을 표시 경계에서 UTF-16으로 읽는다.</summary>
    /// <param name="scene">입력 장면이다.</param>
    /// <returns>본문이다.</returns>
    private static string Text(TextEntryWidgetScene scene) => ExtendedText.DecodeDisplayText(scene.Buffer.Text());

    /// <summary>현재 버퍼 본문을 표시 경계에서 UTF-16으로 읽는다.</summary>
    /// <param name="buffer">편집 버퍼다.</param>
    /// <returns>본문이다.</returns>
    private static string BufferText(TextEntryBuffer buffer) => ExtendedText.DecodeDisplayText(buffer.Text());

    /// <summary>확정 본문을 같은 Handle 경로로 입력한다.</summary>
    /// <param name="scene">입력 장면이다.</param><param name="value">UTF-16 본문이다.</param>
    private static void Type(TextEntryWidgetScene scene, string value)
    {
        foreach (InputEvent item in DisplayTextInput.MakeCommittedTextEvents(value))
        {
            scene.Handle(item.Code);
        }
    }

    /// <summary>실제 Gump에서 왼쪽·오른쪽·선택·삭제가 네 바이트 중간에 멈추지 않는지 확인한다.</summary>
    [Fact]
    public void EditCursorSelectionAndDeletion()
    {
        var scene = new TextEntryWidgetScene();
        scene.Construct();
        scene.Handle((uint)'A');
        Type(scene, "한글");
        scene.Handle((uint)'B');
        Assert.Equal("A한글B", Text(scene));
        Assert.Equal(10, scene.Buffer.State.SelectionBegin);
        scene.Handle(0x250000);
        Assert.Equal(9, scene.Buffer.State.SelectionBegin);
        scene.Handle(0x250000);
        Assert.Equal(5, scene.Buffer.State.SelectionBegin);
        scene.Handle(8);
        Assert.Equal("A글B", Text(scene));
        Assert.Equal(1, scene.Buffer.State.SelectionBegin);
        scene.Handle(0x2e0000);
        Assert.Equal("AB", Text(scene));
        scene.Buffer.Select(1, 1);
        Type(scene, "한글");
        Assert.Equal("A한글B", Text(scene));
        scene.Handle(0x20000000 | 0x250000);
        Assert.Equal(5, scene.Buffer.State.SelectionBegin);
        Assert.Equal(9, scene.Buffer.State.SelectionEnd);
        Type(scene, "가");
        Assert.Equal("A한가B", Text(scene));
        // 손상된 외부 선택도 한 문자 전체로 확장한다
        scene.Buffer.Select(2, 3);
        scene.Handle(8);
        Assert.Equal("A가B", Text(scene));
    }

    /// <summary>기존 물리 바이트 상한을 유지하면서 선택 교체·숫자 전용 제외목록을 확인한다.</summary>
    [Fact]
    public void ByteLimitAndNumericExclusion()
    {
        var scene = new TextEntryWidgetScene();
        scene.Construct();
        scene.Node.MaxLength = 9;
        Type(scene, "한글");
        Assert.Equal("한글", Text(scene));
        Type(scene, "가");
        Assert.Equal("한글", Text(scene));
        scene.Buffer.Select(0, 4);
        Type(scene, "나");
        Assert.Equal("나글", Text(scene));
        scene.Buffer.Select(0, 4);
        scene.Handle((uint)'A');
        Assert.Equal("A글", Text(scene));
        uint excluded = scene.Storage.Allocate(2);
        var bytes = scene.Storage.Bytes(excluded);
        bytes.Span[0] = 0x1f;
        bytes.Span[1] = 0;
        scene.Node.ExcludedChars = excluded;
        Type(scene, "한");
        Assert.Equal("A글", Text(scene));
    }

    /// <summary>붙여넣기의 제외문자·상한이 한글 내부 제어 숫자를 지우지 않는지 확인한다.</summary>
    [Fact]
    public void PasteWholeCharacters()
    {
        byte[] clipboard = DisplayTerminated("갈한글");
        var storage = new TextEntryStorage(target => clipboard.CopyTo(target), _ => { });
        TextEntryBuffer buffer = storage.Create(1);
        buffer.Construct();
        buffer.Select(0, 0);
        buffer.Paste([], 10);
        Assert.Equal("갈한", BufferText(buffer));
        Assert.Equal(8, buffer.State.Length);
        buffer.Select(0, 4);
        clipboard = DisplayTerminated("가나");
        buffer.Paste([], 10);
        Assert.Equal("가한", BufferText(buffer));
    }

    /// <summary>기존 본문이 한글일 때 긴 ASCII도 물리 상한을 넘기지 않는다.</summary>
    [Fact]
    public void AsciiPasteIntoExtendedBodyHonorsFinalByteLimit()
    {
        byte[] clipboard = [.. Enumerable.Repeat((byte)'A', 255), (byte)0];
        var storage = new TextEntryStorage(target => clipboard.CopyTo(target), _ => { });
        TextEntryBuffer buffer = storage.Create(1);
        buffer.Construct();
        buffer.Reset(DisplayTerminated("한"));
        buffer.Select(4, 4);
        buffer.Paste([], 10);
        Assert.Equal("한AAAAA", BufferText(buffer));
        Assert.Equal(9, buffer.State.Length);
        // 남은 상한 0이면 본문·선택을 자르지 않는다
        buffer.Paste([], 10);
        Assert.Equal("한AAAAA", BufferText(buffer));
        Assert.Equal((9, 9), (buffer.State.SelectionBegin, buffer.State.SelectionEnd));
        buffer.Reset(DisplayTerminated("한"));
        buffer.Select(0, 0);
        buffer.Paste([], 5);
        Assert.Equal("한", BufferText(buffer));
    }

    /// <summary>Console 자동완성이 한글 토큰 내부의 공백과 같은 숫자 바이트를 단어 경계로 오해하지 않는지 확인한다.</summary>
    [Fact]
    public void ConsoleCompletionWordBoundary()
    {
        var scene = new TextEntryWidgetScene();
        scene.Construct();
        scene.Buffer.Reset(DisplayTerminated("prefix 갈"));
        scene.Buffer.Select(scene.Buffer.State.Length, scene.Buffer.State.Length);
        scene.Node.Autocomplete = 1;
        scene.Node.ResetWord = 1;
        scene.Handle(9);
        Assert.Equal(DisplayTerminated("갈")[..^1], scene.CompletionWord);
        Assert.Equal(7u, scene.Node.WordStart);
        Assert.Equal(4u, scene.Node.WordLength);
        Assert.Equal("prefix AAAA", Text(scene));
    }
}
