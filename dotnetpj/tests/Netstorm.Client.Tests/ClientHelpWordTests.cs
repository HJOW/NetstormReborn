using System.Text;
using Netstorm.Core.Original;

// 현재 도움말 문구의 원본 설정 해석과 장기 수명을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>NULL·빈 문자열·대소문자와 실제 표시 바이트를 구분한다.</summary>
public sealed class ClientHelpWordTests
{
    /// <summary>누락과 빈 설정은 기존 값을 건드리지 않고 대소문자만 다르면 원래 표기를 유지한다.</summary>
    [Fact]
    public void MissingEmptyAndCaseOnlyPreserveCurrentValue()
    {
        var cache = new ClientHelpWord(); Assert.Null(cache.Value); Assert.False(cache.Interpret(null)); Assert.False(cache.Interpret(""));
        Assert.True(cache.Interpret("Help")); Assert.Equal(1u, cache.Generation);
        Assert.False(cache.Interpret(null)); Assert.False(cache.Interpret("")); Assert.True(cache.Interpret("hELP"));
        Assert.Equal("Help", cache.Value); Assert.Equal(1u, cache.Generation);
        Assert.True(cache.Interpret("Info")); Assert.Equal("Info", cache.Value); Assert.Equal(2u, cache.Generation);
        Assert.True(ClientHelpWord.EqualsIgnoreCase("ABC", "abc")); Assert.False(ClientHelpWord.EqualsIgnoreCase("é", "É"));
    }

    /// <summary>실제 설정의 한국어는 표시 바이트로 한 번 변환되며 장기 소유자가 같은 캐시를 공유한다.</summary>
    [Fact]
    public void CurrentConfigUsesDisplayBytesAndSharedLocalOwner()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); ClientHelpWord cache = local.HelpWord;
        var missing = new ConfigInterface(null); Assert.False(cache.ReadOptions(missing)); Assert.Null(cache.Value);
        var configured = new ConfigInterface(null); configured.Set("helpWord", "정보");
        Assert.True(cache.ReadOptions(configured));
        Assert.Equal(Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText("정보")), cache.Value);
        Assert.Same(cache, local.HelpWord); Assert.Equal(1u, cache.Generation);
        Assert.True(local.HelpWord.ReadOptions(configured)); Assert.Equal(1u, cache.Generation);
    }
}
