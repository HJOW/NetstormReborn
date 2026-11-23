// Root 고정 수신이 읽는 언어 번호·화면 크기·설정 치환의 실제 owner 공급을 검사한다.
// cpppj/src/client/LanguageOptions.h·cpp 의 kLanguageNames/OriginalLanguageName/FindOriginalLanguage 와
// cpppj/src/client/CampaignClientServices.cpp 112·114행의 names.language/substitute,
// cpppj/src/client/ClientMain.cpp 854·1154행의 LogicalScreenSize 를 이식한다.
// 0.902에서 RU-2 실제 owner 조립용으로 추가됐다.
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>같은 실제 설정·화면·장치를 빌리는 Root 공급값의 계약이다.</summary>
public sealed class RootSupplyDevicesTests
{
    /// <summary>원문 일곱 이름 뒤 클론 한국어를 더한 여덟 번호를 그대로 둔다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void LanguageNamesKeepOriginalOrderWithCloneKorean()
    {
        Assert.Equal(["english", "english", "french", "german", "spanish", "japanese", "portuguese", "korean"], RootLanguageNumbers.Names);
        Assert.Equal(5u, RootLanguageNumbers.Japanese);
        Assert.Equal(7u, RootLanguageNumbers.Korean);
    }

    /// <summary>정확한 비교이며 미등록/빈 이름은 0이다. 첫 english는 항상 0이다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void FindReturnsFirstMatchAndZeroForUnknown()
    {
        Assert.Equal(0u, RootLanguageNumbers.Find("english"));
        Assert.Equal(2u, RootLanguageNumbers.Find("french"));
        Assert.Equal(3u, RootLanguageNumbers.Find("german"));
        Assert.Equal(4u, RootLanguageNumbers.Find("spanish"));
        Assert.Equal(5u, RootLanguageNumbers.Find("japanese"));
        Assert.Equal(6u, RootLanguageNumbers.Find("portuguese"));
        Assert.Equal(7u, RootLanguageNumbers.Find("korean"));
        Assert.Equal(0u, RootLanguageNumbers.Find("KOREAN"));
        Assert.Equal(0u, RootLanguageNumbers.Find(""));
        Assert.Equal(0u, RootLanguageNumbers.Find(null));
        Assert.Equal(0u, RootLanguageNumbers.Find("unknown"));
    }

    /// <summary>현재 설정 값을 소문자로 정규화해 번호로 읽는다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void RootLanguageNumberNormalizesCurrentLanguage()
    {
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber("english"));
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber("English"));
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber(" ENGLISH "));
        Assert.Equal(7u, ClientSurfaceGlobals.RootLanguageNumber("korean"));
        Assert.Equal(7u, ClientSurfaceGlobals.RootLanguageNumber("KOREAN"));
        Assert.Equal(5u, ClientSurfaceGlobals.RootLanguageNumber("japanese"));
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber(null));
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber("unknown"));
    }

    /// <summary>확장 문자 번역표·외부 글꼴을 쓰는 번호인지 판정한다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void ExtendedNumbersAreJapaneseAndKoreanOnly()
    {
        Assert.False(RootLanguageNumbers.IsExtended(0));
        Assert.False(RootLanguageNumbers.IsExtended(1));
        Assert.False(RootLanguageNumbers.IsExtended(2));
        Assert.True(RootLanguageNumbers.IsExtended(5));
        Assert.True(RootLanguageNumbers.IsExtended(7));
        Assert.Equal("korean", RootLanguageNumbers.Name(7));
        Assert.Equal("japanese", RootLanguageNumbers.Name(5));
        Assert.Throws<ArgumentOutOfRangeException>(() => RootLanguageNumbers.Name(8));
    }

    /// <summary>수신 직전의 같은 논리 Screen을 읽는다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void RootScreenSizeReadsSameLogicalScreen()
    {
        var screen = new Screen(1024, 768);
        Assert.Equal((1024, 768), ClientSurfaceGlobals.RootScreenSize(screen));
        var wide = new Screen(1920, 1080);
        Assert.Equal((1920, 1080), ClientSurfaceGlobals.RootScreenSize(wide));
        Assert.Throws<ArgumentNullException>(() => ClientSurfaceGlobals.RootScreenSize(null!));
    }

    /// <summary>같은 실제 설정의 치환을 빌린다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void SubstituteLocalUsesSameConfiguration()
    {
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        using var outer = new Config(config.Registry, "param");
        outer.Append("1=outer");
        Assert.Equal("outer", config.SubstituteLocal("{param.1}"));
    }

    /// <summary>장면음악 장치는 같은 실제 OwnerMode.Battle을 빌린다. 0.902에서 추가됐다.</summary>
    [Fact]
    public void SceneMusicBorrowsSameBattleOwner()
    {
        var game = new ClientGameplayGlobals();
        var surface = new ClientSurfaceGlobals(game, new(new(game, new())), new(), static () => 0);
        var resources = new ClientStateResources();
        resources.Music.CurrentBattle = () => surface.Storage.OwnerMode.Battle ? 1u : 0u;
        Assert.Equal(0u, resources.Music.CurrentBattle());
        surface.Storage.OwnerMode.Battle = true;
        Assert.Equal(1u, resources.Music.CurrentBattle());
        surface.Storage.OwnerMode.Battle = false;
        Assert.Equal(0u, resources.Music.CurrentBattle());
    }
}
