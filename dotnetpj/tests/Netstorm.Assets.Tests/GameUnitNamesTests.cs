using System.Text.Json;

namespace Netstorm.Assets.Tests;

/// <summary>
/// 건물·유닛의 한국어 표시 이름이 AGENTS.md 의 명칭 규칙(번역해도 되는 이름과 영어 발음 표기, 모호한 발음의 확정 표기)을 지키는지 고정한다.
/// 공유 명칭 JSON(assets/unit-names.korean.json)이 내장 리소스로 들어가므로 이 검사는 그 파일의 값을 직접 확인한다 (2026-10-16 규칙 변경 반영).
/// </summary>
public sealed class GameUnitNamesTests
{
    /// <summary>번역이 허용된 이름과 발음 표기가 확정된 이름, 영어 발음 표기를 쓰는 대표 이름의 기대 표시.</summary>
    [Theory]
    [InlineData("Priest", "사제")]
    [InlineData("Temple", "신전")]
    [InlineData("Bridge", "다리")]
    [InlineData("Thrower", "발사기")]
    [InlineData("Altar", "제단")]
    [InlineData("Residence", "거주 구역")]
    [InlineData("Whirlibase", "훨리베이스")]
    [InlineData("Whirligig", "훨리기그")]
    [InlineData("Workshop", "워크샾")]
    [InlineData("Generator", "제네레이터")]
    [InlineData("Sun Barricade", "썬 바리케이드")]
    public void KoreanLanguageUsesAgreedNames(string english, string korean) =>
        Assert.Equal(korean, GameUnitNames.Format(english, GameLanguage.Korean));

    /// <summary>한국어가 아니거나 표에 없는 이름은 입력을 그대로 돌려준다.</summary>
    [Fact]
    public void OtherLanguagesAndUnknownNamesAreUnchanged()
    {
        Assert.Equal("Altar", GameUnitNames.Format("Altar", GameLanguage.English));
        Assert.Equal("Unknown Thing", GameUnitNames.Format("Unknown Thing", GameLanguage.Korean));
    }

    /// <summary>공유 표 어디에도 이전 표기(휘리·알타·레지던스)가 남아 있지 않다.</summary>
    [Fact]
    public void SharedTableHasNoRetiredSpellings()
    {
        using Stream stream = typeof(GameUnitNames).Assembly.GetManifestResourceStream("Netstorm.Assets.UnitNames.Korean.json")!;
        Dictionary<string, string> names = JsonSerializer.Deserialize<Dictionary<string, string>>(stream)!;
        Assert.Equal(55, names.Count);
        // 표의 한국어 값마다 이전 표기가 들어 있지 않은지 확인한다
        foreach ((string english, string korean) in names)
        {
            Assert.DoesNotContain("휘리", korean);
            Assert.DoesNotContain("알타", korean);
            Assert.DoesNotContain("레지던스", korean);
            Assert.False(string.IsNullOrWhiteSpace(korean), $"{english} 의 한국어 표시가 비어 있다");
        }
    }
}
