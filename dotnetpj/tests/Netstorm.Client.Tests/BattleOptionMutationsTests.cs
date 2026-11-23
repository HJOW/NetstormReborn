// BattleInfo가 사용하는 실제 BattleOptions BYTE 변경과 제외 목록을 원문 순서로 처리한다.
// cpppj/src/o/RawBattleOptionMutation.cpp 를 이식한다.
// 0.914에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 옵션 BYTE와 제외 목록·음향 호출의 계약이다.</summary>
public sealed class BattleOptionMutationsTests
{
    /// <summary>실제 0x59BYTE 옵션을 준비한다.</summary>
    private static byte[] Options() => new byte[0x59];

    /// <summary>게임 종류는 첫 BYTE 그대로다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void GameTypeReadsFirstByte()
    {
        byte[] options = Options();
        options[0] = 3;
        Assert.Equal(3u, BattleOptionMutations.BattleOptionGameType(options));
        Assert.Equal(3u, BattleOptionMutations.BattleOptionGameType(options));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionMutations.BattleOptionGameType([]));
    }

    /// <summary>포함 최댓값은 정적 표의 개수-1이다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void MaximumChoiceIsCountMinusOne()
    {
        Assert.Equal(4, BattleOptionMutations.MaximumBattleOptionChoice(0));
        Assert.Equal(2, BattleOptionMutations.MaximumBattleOptionChoice(1));
        Assert.Equal(5, BattleOptionMutations.MaximumBattleOptionChoice(15));
        Assert.Equal(3, BattleOptionMutations.MaximumBattleOptionChoice(21));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionMutations.MaximumBattleOptionChoice(22));
    }

    /// <summary>순환은 최댓값을 넘으면0으로 감기고 음향 뒤에 저장한다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void CycleWrapsAndSoundsAfterCompute()
    {
        byte[] options = Options();
        options[1] = 2;
        int sounds = 0;
        BattleOptionMutations.CycleBattleOption(options, 1, () => sounds++);
        Assert.Equal(0, options[1]);
        Assert.Equal(1, sounds);
        options[1] = 1;
        BattleOptionMutations.CycleBattleOption(options, 1, () => sounds++);
        Assert.Equal(2, options[1]);
        Assert.Equal(2, sounds);
        Assert.Throws<InvalidOperationException>(() => BattleOptionMutations.CycleBattleOption(options, 1, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionMutations.CycleBattleOption([], 1, () => { }));
    }

    /// <summary>설정은 음수를 버리고 최댓값 이내만 쓴다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void SetIgnoresNegativeAndOverflow()
    {
        byte[] options = Options();
        options[1] = 9;
        BattleOptionMutations.SetBattleOptionChoice(options, 1, -1);
        Assert.Equal(9, options[1]);
        BattleOptionMutations.SetBattleOptionChoice(options, 1, 7);
        Assert.Equal(9, options[1]);
        BattleOptionMutations.SetBattleOptionChoice(options, 1, 0);
        Assert.Equal(0, options[1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => BattleOptionMutations.SetBattleOptionChoice([], 1, 1));
    }

    /// <summary>제외 추가는 중복이면 음향 없이1, 가득 차면0이다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void AddExcludedHandlesDuplicateAndFull()
    {
        byte[] options = Options();
        int sounds = 0;
        Assert.Equal(1u, BattleOptionMutations.AddBattleExcludedType(options, 5, () => sounds++));
        Assert.Equal(1, options[0x16]);
        Assert.Equal(5, options[0x17]);
        Assert.Equal(1, sounds);
        Assert.Equal(1u, BattleOptionMutations.AddBattleExcludedType(options, 5, () => sounds++));
        Assert.Equal(1, sounds);
        options[0x16] = 40;
        Assert.Equal(0u, BattleOptionMutations.AddBattleExcludedType(options, 9, () => sounds++));
        Assert.Equal(1, sounds);
        Assert.Throws<InvalidOperationException>(() => BattleOptionMutations.AddBattleExcludedType(Options(), 7, null));
        Assert.Equal(1u, BattleOptionMutations.BattleTypeExcluded(options, 5));
        Assert.Equal(0u, BattleOptionMutations.BattleTypeExcluded(options, 9));
    }

    /// <summary>제외 제거는 최초 일치만 옮기고 끝칸을 지우지 않는다. 0.914에서 추가됐다.</summary>
    [Fact]
    public void RemoveExcludedShiftsOnlyFirstMatch()
    {
        byte[] options = Options();
        options[0x16] = 3;
        options[0x17] = 5;
        options[0x18] = 7;
        options[0x19] = 9;
        int sounds = 0;
        Assert.Equal(1u, BattleOptionMutations.RemoveBattleExcludedType(options, 7, () => sounds++));
        Assert.Equal(2, options[0x16]);
        Assert.Equal(5, options[0x17]);
        Assert.Equal(9, options[0x18]);
        Assert.Equal(9, options[0x19]);
        Assert.Equal(1, sounds);
        Assert.Equal(0u, BattleOptionMutations.RemoveBattleExcludedType(options, 7, () => sounds++));
        Assert.Equal(1, sounds);
        Assert.Equal(1u, BattleOptionMutations.BattleTypeExcluded(options, 9));
        Assert.Equal(0u, BattleOptionMutations.BattleTypeExcluded(Options(), 5));
    }
}
