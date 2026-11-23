// 10.78의 실제22개 BattleOptions 문자열 표와 조건부 번역 조회를 제공한다.
// cpppj/src/o/RawBattleOptionText.cpp 를 이식한다.
// 0.914에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 정적 표와 조건부 번역의 계약이다.</summary>
public sealed class BattleOptionTextsTests
{
    /// <summary>22레코드의 이름·개수·번역 플래그가 원문 순서와 같다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void TableHasTwentyTwoRecordsInOrder()
    {
        Assert.Equal("Game Type", BattleOptionTexts.BattleOptionText(0).Name);
        Assert.Equal(5, BattleOptionTexts.BattleOptionText(0).Values.Count);
        Assert.False(BattleOptionTexts.BattleOptionText(0).TranslateValue);
        Assert.Equal("Unit Rate", BattleOptionTexts.BattleOptionText(2).Name);
        Assert.True(BattleOptionTexts.BattleOptionText(2).TranslateValue);
        Assert.Equal("Kill Reward", BattleOptionTexts.BattleOptionText(15).Name);
        Assert.Equal(6, BattleOptionTexts.BattleOptionText(15).Values.Count);
        Assert.Equal("Injection Value", BattleOptionTexts.BattleOptionText(21).Name);
        Assert.Equal(4, BattleOptionTexts.BattleOptionText(21).Values.Count);
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionText(22));
    }

    /// <summary>비번역 선택은 번역기 없이 원문을 반환한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void ChoiceWithoutTranslateFlagReturnsRaw()
    {
        Assert.Equal("4", BattleOptionTexts.BattleOptionChoiceText(1, 1, null));
        Assert.Equal("Netstorm Standard", BattleOptionTexts.BattleOptionChoiceText(0, 0, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionChoiceText(1, 3, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionChoiceText(22, 0, null));
    }

    /// <summary>번역 선택은 번역기를 요구한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void ChoiceWithTranslateFlagRequiresTranslator()
    {
        Assert.Equal("T:Fast", BattleOptionTexts.BattleOptionChoiceText(2, 2, value => "T:" + value));
        Assert.Throws<InvalidOperationException>(() => BattleOptionTexts.BattleOptionChoiceText(2, 0, null));
    }

    /// <summary>선택 주소는 원문 포인터 정체성을 보존한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void ChoiceAddressPreservesIdentity()
    {
        Assert.Equal(0x502118u, BattleOptionTexts.BattleOptionChoiceAddress(0, 0));
        Assert.Equal(0x5020c8u, BattleOptionTexts.BattleOptionChoiceAddress(0, 4));
        Assert.Equal(0x502144u, BattleOptionTexts.BattleOptionChoiceAddress(21, 3));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionChoiceAddress(1, 3));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionChoiceAddress(22, 0));
    }

    /// <summary>현재 BYTE→선택 포인터→조건부 번역을 수행한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void ValueTextReadsCurrentByte()
    {
        byte[] options = new byte[0x59];
        options[2] = 1;
        Assert.Equal("T:Medium", BattleOptionTexts.BattleOptionValueText(options, 2, value => "T:" + value));
        options[1] = 2;
        Assert.Equal("6", BattleOptionTexts.BattleOptionValueText(options, 1, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionValueText(options, 0x59, null));
        options[2] = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionValueText(options, 2, null));
    }

    /// <summary>이름은 번역 플래그와 독립적으로 번역기를 요구한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void NameTextAlwaysTranslates()
    {
        Assert.Equal("T:Game Type", BattleOptionTexts.BattleOptionNameText(0, value => "T:" + value));
        Assert.Throws<InvalidOperationException>(() => BattleOptionTexts.BattleOptionNameText(0, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionTexts.BattleOptionNameText(22, value => value));
    }
}
