// 실제87BYTE 전투 옵션의8개 슬롯 색/랭크/특수 상태를 원문 폭으로 읽고 쓴다.
// cpppj/src/o/RawBattleOptionSlots.cpp 를 이식한다.
// 0.913에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 옵션 BYTE와 슬롯 연산의 계약이다.</summary>
public sealed class BattleOptionSlotsTests
{
    /// <summary>실제 0x59BYTE 옵션을 준비한다.</summary>
    private static byte[] Options() => new byte[0x59];

    /// <summary>색 증가는7에서0으로 감긴다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ColorIncreaseWrapsAtSeven()
    {
        byte[] options = Options();
        options[0x47] = 7;
        Assert.Equal(1u, BattleOptionSlots.IncreaseBattleSlotColor(options, 0));
        Assert.Equal(0, options[0x47]);
        options[0x47] = 3;
        Assert.Equal(1u, BattleOptionSlots.IncreaseBattleSlotColor(options, 0));
        Assert.Equal(4, options[0x47]);
    }

    /// <summary>색 감소는0에서7로 감긴다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ColorDecreaseWrapsAtZero()
    {
        byte[] options = Options();
        options[0x47] = 0;
        Assert.Equal(1u, BattleOptionSlots.DecreaseBattleSlotColor(options, 0));
        Assert.Equal(7, options[0x47]);
        options[0x47] = 5;
        Assert.Equal(1u, BattleOptionSlots.DecreaseBattleSlotColor(options, 0));
        Assert.Equal(4, options[0x47]);
    }

    /// <summary>랭크 증가는23에서0으로 감긴다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void RankIncreaseWrapsAtTwentyThree()
    {
        byte[] options = Options();
        options[0x4f] = 23;
        Assert.Equal(1u, BattleOptionSlots.IncreaseBattleSlotRank(options, 0));
        Assert.Equal(0, options[0x4f]);
        options[0x4f] = 10;
        Assert.Equal(1u, BattleOptionSlots.IncreaseBattleSlotRank(options, 0));
        Assert.Equal(11, options[0x4f]);
    }

    /// <summary>랭크 감소는0에서23으로 감긴다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void RankDecreaseWrapsAtZero()
    {
        byte[] options = Options();
        options[0x4f] = 0;
        Assert.Equal(1u, BattleOptionSlots.DecreaseBattleSlotRank(options, 0));
        Assert.Equal(23, options[0x4f]);
        options[0x4f] = 10;
        Assert.Equal(1u, BattleOptionSlots.DecreaseBattleSlotRank(options, 0));
        Assert.Equal(9, options[0x4f]);
    }

    /// <summary>특수는1이면0,그외면1이다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ToggleSpecialFlipsOneAndOthers()
    {
        byte[] options = Options();
        options[0x3f] = 1;
        Assert.Equal(1u, BattleOptionSlots.ToggleBattleSlotSpecial(options, 0));
        Assert.Equal(0, options[0x3f]);
        options[0x3f] = 0;
        Assert.Equal(1u, BattleOptionSlots.ToggleBattleSlotSpecial(options, 0));
        Assert.Equal(1, options[0x3f]);
        options[0x3f] = 5;
        Assert.Equal(1u, BattleOptionSlots.ToggleBattleSlotSpecial(options, 0));
        Assert.Equal(1, options[0x3f]);
    }

    /// <summary>랭크 표는+4f BYTE를 실제 값으로 변환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void RankTableConvertsIndex()
    {
        byte[] options = Options();
        options[0x4f] = 0;
        Assert.Equal(0, BattleOptionSlots.BattleSlotRank(options, 0));
        options[0x4f] = 10;
        Assert.Equal(10, BattleOptionSlots.BattleSlotRank(options, 0));
        options[0x4f] = 11;
        Assert.Equal(15, BattleOptionSlots.BattleSlotRank(options, 0));
        options[0x4f] = 23;
        Assert.Equal(100, BattleOptionSlots.BattleSlotRank(options, 0));
    }

    /// <summary>무효 슬롯은 읽기 없이0을 반환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void InvalidSlotReturnsZeroWithoutReading()
    {
        byte[] empty = [];
        Assert.Equal(0u, BattleOptionSlots.IncreaseBattleSlotColor(empty, 8));
        Assert.Equal(0u, BattleOptionSlots.IncreaseBattleSlotColor(empty, -1));
        Assert.Equal(0u, BattleOptionSlots.DecreaseBattleSlotColor(empty, 8));
        Assert.Equal(0u, BattleOptionSlots.IncreaseBattleSlotRank(empty, 8));
        Assert.Equal(0u, BattleOptionSlots.DecreaseBattleSlotRank(empty, -1));
        Assert.Equal(0u, BattleOptionSlots.ToggleBattleSlotSpecial(empty, 8));
        Assert.Equal(0, BattleOptionSlots.BattleSlotRank(empty, 8));
        Assert.Equal(0, BattleOptionSlots.BattleSlotRank(empty, -1));
    }

    /// <summary>짧은 물리는 유효 슬롯에서도 명시 거부한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ShortSpanThrowsExplicitly()
    {
        byte[] shortOptions = new byte[0x40];
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.IncreaseBattleSlotColor(shortOptions, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.DecreaseBattleSlotColor(shortOptions, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.IncreaseBattleSlotRank(shortOptions, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.DecreaseBattleSlotRank(shortOptions, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.ToggleBattleSlotSpecial(shortOptions, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionSlots.BattleSlotRank(shortOptions, 0));
    }
}
