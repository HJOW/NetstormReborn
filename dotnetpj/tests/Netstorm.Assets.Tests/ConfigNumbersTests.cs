namespace Netstorm.Assets.Tests;

/// <summary>원본 CRT atol 과 ';' 목록 항목 규칙(cpppj ConfigInterfaceTests 의 경계와 같은 벡터)을 검사한다.</summary>
public sealed class ConfigNumbersTests
{
    /// <summary>앞 공백·부호·숫자 아닌 글자에서의 정지·넘침 감김까지 CRT atol 과 같다.</summary>
    [Theory]
    [InlineData("", 0)]
    [InlineData("  \t-12abc", -12)]
    [InlineData("+7", 7)]
    [InlineData("x1", 0)]
    [InlineData("4294967297", 1)]
    [InlineData("3,4", 3)]
    [InlineData(" 3", 3)]
    [InlineData("-", 0)]
    [InlineData("0x10", 0)]
    [InlineData("2147483648", int.MinValue)]
    public void ParseLong_FollowsCrtAtol(string text, int expected) => Assert.Equal(expected, ConfigNumbers.ParseLong(text));

    /// <summary>null 은 빈 문자열처럼 0이다.</summary>
    [Fact]
    public void ParseLong_TreatsNullAsEmpty() => Assert.Equal(0, ConfigNumbers.ParseLong(null));

    /// <summary>';' 로만 나누고 조각을 다듬지 않으며 없는 조각과 음수 번호는 빈 문자열이다.</summary>
    [Fact]
    public void ListItem_SplitsOnSemicolonOnly()
    {
        Assert.Equal("a", ConfigNumbers.ListItem("a;b;;d", 0));
        Assert.Equal("b", ConfigNumbers.ListItem("a;b;;d", 1));
        Assert.Equal("", ConfigNumbers.ListItem("a;b;;d", 2));
        Assert.Equal("d", ConfigNumbers.ListItem("a;b;;d", 3));
        Assert.Equal("", ConfigNumbers.ListItem("a;b;;d", 4));
        Assert.Equal("", ConfigNumbers.ListItem("a;b", -1));
        // 쉼표와 공백은 구분자가 아니고 조각을 다듬지도 않는다.
        Assert.Equal("3,4", ConfigNumbers.ListItem("3,4", 0));
        Assert.Equal(" 3", ConfigNumbers.ListItem("2; 3", 1));
        Assert.Equal("", ConfigNumbers.ListItem(null, 0));
    }
}
