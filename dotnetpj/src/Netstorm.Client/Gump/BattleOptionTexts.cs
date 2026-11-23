// 10.78의 실제22개 BattleOptions 문자열 표와 조건부 번역 조회를 제공한다.
// cpppj/src/o/RawBattleOptionText.cpp 를 이식한다.
// 0.914에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 각24BYTE 레코드에서 표시 getter가 읽는 세 필드다.
public sealed record BattleOptionTextDefinition(
    // +0의 원문 이름이며 getter는 항상 현재 번역기를 호출한다.
    string Name,
    // +4 선택 포인터 표이며 +8은 개수가 아닌 포함 최댓값이다.
    IReadOnlyList<string> Values,
    // +c 비영일 때만 선택 문자열을 현재 번역기로 보낸다.
    bool TranslateValue);

// 원문0052f5d0의 정적 표를 복원하며 현재 옵션 BYTE는 caller가 빌린다.
public static class BattleOptionTexts
{
    /// <summary>원문 순서와 각 선택 번역 플래그다. 0.914에서 추가됐다.</summary>
    private static readonly BattleOptionTextDefinition[] Definitions =
    [
        new("Game Type", ["Netstorm Standard", "Netstorm Classic", "Netstorm Flexible", "Netstorm Geyserless", "Player Customizable"], false),
        new("Bridge Slots", ["2", "4", "6"], false),
        new("Unit Rate", ["Slow", "Medium", "Fast"], true),
        new("Knowledge", ["Individual", "Shared"], true),
        new("Generator Range", ["Short", "Normal", "Long", "Very Long"], true),
        new("Map Mode", ["Archipelago", "Single Island"], false),
        new("Map Size", ["Small", "Medium", "Large", "Extra Large"], false),
        new("Map Density", ["Low", "Medium", "High", "Extreme"], false),
        new("Spells", ["No", "Yes"], false),
        new("Island Dynamics", ["Off", "Low", "Medium", "High"], false),
        new("Living World", ["Off", "On"], false),
        new("Alliances", ["No", "Yes"], false),
        new("Alliances Breakable ", ["No", "Yes"], true),
        new("Allow Sending Money", ["No", "Yes"], false),
        new("Starting Cash", ["5000", "6500", "7000"], false),
        new("Kill Reward", ["0%", "25%", "50%", "75%", "100%", "150%"], true),
        new("Money per Geyser", ["1000", "2000", "3000", "5000"], false),
        new("Geyser Amount", ["Low", "Medium", "High", "Extreme"], false),
        new("Geysers Placement", ["None", "Random", "Even"], false),
        new("Geysers Respawns", ["None", "30 Sec", "1 Min", "2 Min", "5 Min"], false),
        new("Resource Injections", ["None", "30 Sec", "1 Min", "2 Min", "5 Min", "10 Min"], false),
        new("Injection Value", ["500 SP", "1000 SP", "2500 SP", "5000 SP"], false),
    ];

    /// <summary>원문22레코드의모든76선택포인터다. 호스트역참조가아닌메뉴의원문포인터정체성에만사용한다. 0.914에서 추가됐다.</summary>
    private static readonly uint[] ChoiceSourceAddresses =
    [
        0x502118U, 0x502104U, 0x5020f0U, 0x5020dcU, 0x5020c8U, 0x501614U, 0x5015f4U, 0x501610U,
        0x502278U, 0x502270U, 0x502268U, 0x50225cU, 0x502254U, 0x50224cU, 0x502244U, 0x50223cU,
        0x502230U, 0x5021dcU, 0x5021ccU, 0x5021b0U, 0x502270U, 0x5021a8U, 0x50219cU, 0x5021c8U,
        0x502270U, 0x5021c0U, 0x5021b8U, 0x50220cU, 0x502208U, 0x502140U, 0x5021c8U, 0x502270U,
        0x5021c0U, 0x502140U, 0x50213cU, 0x50220cU, 0x502208U, 0x50220cU, 0x502208U, 0x50220cU,
        0x502208U, 0x5021e8U, 0x502134U, 0x50212cU, 0x50222cU, 0x502228U, 0x502224U, 0x502220U,
        0x502218U, 0x502210U, 0x502200U, 0x5021f8U, 0x5021f0U, 0x5021e8U, 0x5021c8U, 0x502270U,
        0x5021c0U, 0x5021b8U, 0x500fecU, 0x502194U, 0x50218cU, 0x500fecU, 0x502184U, 0x50217cU,
        0x502174U, 0x50216cU, 0x500fecU, 0x502184U, 0x50217cU, 0x502174U, 0x50216cU, 0x502164U,
        0x50215cU, 0x502154U, 0x50214cU, 0x502144U,
    ];

    /// <summary>각레코드의시작과최종76경계다. 0.914에서 추가됐다.</summary>
    private static readonly int[] ChoiceSourceOffsets =
        [0, 5, 8, 11, 13, 17, 19, 23, 27, 29, 33, 35, 37, 39, 41, 44, 50, 54, 58, 61, 66, 72, 76];

    /// <summary>실제0..21 레코드를 읽고 물리 표 밖 인덱스는 거부한다. 0.914에서 추가됐다.</summary>
    public static BattleOptionTextDefinition BattleOptionText(uint index)
    {
        if (index >= (uint)Definitions.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "BattleOptions 실제 표 범위");
        }

        return Definitions[index];
    }

    /// <summary>지정된 선택 포인터와+c 플래그에 따른 현재 번역이다. 현재옵션BYTE는읽지않는다. 0.914에서 추가됐다.</summary>
    public static string BattleOptionChoiceText(uint index, uint selected, Func<string, string>? translate)
    {
        BattleOptionTextDefinition definition = BattleOptionText(index);
        if (selected >= (uint)definition.Values.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(selected), "BattleOptions 실제 선택 포인터 표 범위");
        }

        string value = definition.Values[(int)selected];
        if (!definition.TranslateValue)
        {
            return value;
        }

        if (translate is null)
        {
            throw new InvalidOperationException("BattleOptions 현재 번역기 미연결");
        }

        return translate(value);
    }

    /// <summary>보호PE의실제선택문자열포인터값이다. 현재표의정상선택범위를먼저검증한다. 0.914에서 추가됐다.</summary>
    public static uint BattleOptionChoiceAddress(uint index, uint selected)
    {
        if (selected >= (uint)BattleOptionText(index).Values.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(selected), "BattleOptions 실제 선택 주소 범위");
        }

        return ChoiceSourceAddresses[ChoiceSourceOffsets[index] + (int)selected];
    }

    /// <summary>현재 BYTE→선택 포인터→조건부 번역이다. 0.914에서 추가됐다.</summary>
    public static string BattleOptionValueText(ReadOnlySpan<byte> options, uint index, Func<string, string>? translate)
    {
        if (index >= (uint)options.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "BattleOptions 현재 BYTE 물리 범위");
        }

        return BattleOptionChoiceText(index, options[(int)index], translate);
    }

    /// <summary>현재 레코드 이름의 필수 번역 조회다. 0.914에서 추가됐다.</summary>
    public static string BattleOptionNameText(uint index, Func<string, string>? translate)
    {
        string name = BattleOptionText(index).Name;
        if (translate is null)
        {
            throw new InvalidOperationException("BattleOptions 현재 이름 번역기 미연결");
        }

        return translate(name);
    }
}
