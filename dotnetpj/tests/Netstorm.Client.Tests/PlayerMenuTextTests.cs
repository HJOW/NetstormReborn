using Netstorm.Client.Gump;

// 실제 메뉴 공통 서식의 인자·문자·버퍼 경계를 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>고정 서식의 결과와 원본 계약 오류를 검증한다.</summary>
public sealed class PlayerMenuTextTests
{
    /// <summary>문자열·정수와 리터럴 퍼센트를 순서대로 기록한다.</summary>
    [Fact]
    public void FormatsOrderedArgumentsWithoutReinterpretingInput()
    {
        PriestMenuArgument[] values = ["Alpha %d", -2147483648, "é"];
        Assert.Equal("Alpha %d:-2147483648:%:é", PlayerMenuText.Format("%s:%d:%%:%s", values));
    }

    /// <summary>원문 버퍼의 종료 NUL을 고려한 길이 경계를 지킨다.</summary>
    [Fact]
    public void RejectsOutputThatDoesNotFitOriginalBuffer()
    {
        Assert.Equal(new string('a', 255), PlayerMenuText.Format("%s", [new string('a', 255)]));
        Assert.Throws<ArgumentOutOfRangeException>(() => PlayerMenuText.Format("%s", [new string('a', 256)]));
    }

    /// <summary>부족·남는 인자와 미지원 서식은 실제 오류로 남긴다.</summary>
    [Fact]
    public void RejectsMissingExtraWrongAndUnsupportedArguments()
    {
        Assert.Throws<InvalidOperationException>(() => PlayerMenuText.Format("%", []));
        Assert.Throws<InvalidOperationException>(() => PlayerMenuText.Format("%s", []));
        Assert.Throws<InvalidOperationException>(() => PlayerMenuText.Format("%x", [1]));
        Assert.Throws<InvalidOperationException>(() => PlayerMenuText.Format("%s", [1]));
        Assert.Throws<ArgumentOutOfRangeException>(() => PlayerMenuText.Format("plain", [1]));
    }
}
