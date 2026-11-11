using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 새 제품 표기 11.01의 버전 상수·설정 적용·Credits 본문 확장을 검사한다
/// (cpppj <c>tests/</c>의 제품 전용 검사가 없어 직접 작성했다).
/// </summary>
/// <remarks>2026-11-03 11.01 요구에서 추가했다. 원본 fixture가 없는 직역이라 PORT_MAP 상태는 port다.</remarks>
public sealed class ProductPresentationTests
{
    /// <summary>새 제품의 정수 주·부 버전과 표시 문자열이다.</summary>
    [Fact]
    public void ProductVersionKeepsLeadingZero()
    {
        Assert.Equal(11, ProductVersion.Major);
        Assert.Equal(1, ProductVersion.Minor);
        Assert.Equal("11.01", ProductVersion.Text);
        Assert.Equal("v11.01", ProductVersion.ConfigValue);
        Assert.Equal("Patch1101", ProductVersion.CreditsObject);
        Assert.Equal("Patch1101.Credits", ProductVersion.CreditsTarget);
    }

    /// <summary>Startup의 원문 정수 표기 뒤 제품 표기를 설정에 적용한다.</summary>
    [Fact]
    public void ApplyProductVersionSetsConfigKeys()
    {
        ConfigInterface config = new(null);
        ProductPresentation.ApplyProductVersion(config);
        Assert.Equal(11, config.GetInt("MAJOR_VERSION"));
        Assert.Equal(1, config.GetInt("MINOR_VERSION"));
        Assert.Equal("v11.01", config.Configuration.Get("version"));
    }

    /// <summary>원본 Credits 본문에 세 번째 항목을 같은 $Menu 서식으로 추가한다.</summary>
    [Fact]
    public void TellBodyAppendsPatchCreditsOnce()
    {
        string original = "[Credits]\n$Menu=Netstorm 10.72 Patch Credits,Tell,Patch1072.Credits\n$Menu=Netstorm Original Credits,Tell,Original.Credits\n";
        string actual = ProductPresentation.TellBody("Credits", original);
        Assert.Contains("$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits", actual);
        // 중복 확장도 피한다
        Assert.Equal(actual, ProductPresentation.TellBody("Credits", actual));
    }

    /// <summary>대상 이름은 원본처럼 대소문자를 무시하고 비교한다.</summary>
    [Fact]
    public void TellBodyMatchesCreditsCaseInsensitive()
    {
        string original = "[Credits]\n";
        Assert.Contains("Patch1101.Credits", ProductPresentation.TellBody("CREDITS", original));
        Assert.Contains("Patch1101.Credits", ProductPresentation.TellBody("credits", original));
        Assert.Contains("Patch1101.Credits", ProductPresentation.TellBody("CrEdItS", original));
    }

    /// <summary>Credits가 아닌 대상의 본문은 바꾸지 않는다.</summary>
    [Fact]
    public void TellBodyKeepsNonCreditsUnchanged()
    {
        string original = "[Help]\n$Menu=General Help,Tell,General\n";
        Assert.Same(original, ProductPresentation.TellBody("Help", original));
        Assert.Same(original, ProductPresentation.TellBody("Blank", original));
    }

    /// <summary>추가 Credits 본문은 같은 HTML 창 공장의 제목·HJOW·AI 안내를 담는다.</summary>
    [Fact]
    public void PatchCreditsBodyHasTitleAndAiNotice()
    {
        Assert.Contains("NetStorm 11.01 Patch Credits", ProductPresentation.PatchCreditsBody);
        Assert.Contains("HJOW", ProductPresentation.PatchCreditsBody);
        Assert.Contains("Copyright 2026 HJOW", ProductPresentation.PatchCreditsBody);
        Assert.Contains("AI", ProductPresentation.PatchCreditsBody);
        Assert.Contains("{version}", ProductPresentation.PatchCreditsBody);
    }

    /// <summary>새 항목 대상이면 현재 언어의 고정 번역 본문을 그대로 돌려준다.</summary>
    [Fact]
    public void TellBodyReturnsFixedTranslationForPatchTarget()
    {
        // 언어 번호는 원문과 같은 값이다(7 한국어, 5 일본어)
        Assert.Equal(7u, ProductPresentation.KoreanLanguage);
        Assert.Equal(5u, ProductPresentation.JapaneseLanguage);
        Assert.Equal(ProductPresentation.PatchCreditsKoreanBody, ProductPresentation.TellBody("Patch1101.Credits", "anything", 7));
        Assert.Equal(ProductPresentation.PatchCreditsKoreanBody, ProductPresentation.TellBody("PATCH1101.CREDITS", "anything", 7));
        Assert.Equal(ProductPresentation.PatchCreditsJapaneseBody, ProductPresentation.TellBody("Patch1101.Credits", "anything", 5));
        Assert.Contains("Copyright 2026 HJOW", ProductPresentation.PatchCreditsKoreanBody);
        Assert.Contains("Copyright 2026 HJOW", ProductPresentation.PatchCreditsJapaneseBody);
    }

    /// <summary>Campaign 목록에는 Back 버튼 앞에 언어별 Skirmish 항목을 넣는다.</summary>
    [Fact]
    public void TellBodyInsertsSkirmishBeforeBackButton()
    {
        const string menu = "[UCampaign]\n$Menu=Campaign,Tell,First\n$Button=Back,Tell,Blank\n";
        string english = ProductPresentation.TellBody("ucampaign", menu);
        Assert.Contains("$Menu=Skirmish,Tell,Patch1101.Skirmish\n$Button=Back", english);
        string korean = ProductPresentation.TellBody("UCampaign", menu, 7);
        Assert.Contains("$Menu=스커미시,Tell,Patch1101.Skirmish\n$Button=Back", korean);
        string japanese = ProductPresentation.TellBody("ucampaign", menu, 5);
        Assert.Contains("$Menu=スカーミッシュ,Tell,Patch1101.Skirmish\n$Button=Back", japanese);
        // 중복 확장도 피한다
        Assert.Equal(english, ProductPresentation.TellBody("ucampaign", english));
    }

    /// <summary>언어별 새 항목 이름으로 Credits 목록을 확장한다.</summary>
    [Fact]
    public void TellBodyAppendsLocalizedPatchLabel()
    {
        Assert.Contains("$Menu=NetStorm 11.01 패치 크레딧,Tell,Patch1101.Credits", ProductPresentation.TellBody("credits", "[Credits]\n", 7));
        Assert.Contains("$Menu=NetStorm 11.01 パッチ クレジット,Tell,Patch1101.Credits", ProductPresentation.TellBody("credits", "[Credits]\n", 5));
    }
}
