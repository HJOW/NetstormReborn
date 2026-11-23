// BattleInfo의 정렬·전체·개별 메뉴 자식을 원문 선택 문자열/조건부 번역과 같은 controlled 스택에 작성한다.
// cpppj/src/client/NativeBattleInfoMenus.cpp 의 BuildBattleSortMenu/BuildBattleOptionsMenu/BuildBattleOptionMenu 를 이식한다.
// 0.917에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

// 현재 원본 글꼴 바이트의 선택 문자열과 같은 실제 선택/번역 포인터의 논리 정체성이다.
public sealed record BattleOptionMenuText(
    // 현재 원본 글꼴 바이트의 선택 문자열이다.
    string Text,
    // 같은 실제 선택/번역 포인터의 논리 정체성이며 호스트 임시string 주소가 아니다.
    ulong Identity);

// 문자열 주소의 비교 의미와 호스트 문자열 소유를 구별한다.
public sealed class NativeBattleInfoMenuHooks
{
    /// <summary>같은 현재 명령 DWORD와 제외할 TYPE 별칭을 읽는다.</summary>
    public Func<uint, uint>? Read { get; set; }

    /// <summary>현재 원문 언어의 실제 번역이다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>0041c410의 현재 선택 포인터/조건부 번역 정체성이다.</summary>
    public Func<int, uint, BattleOptionMenuText>? Choice { get; set; }

    /// <summary>0041c3d0의 현재 BYTE 선택/조건부 번역 정체성이다.</summary>
    public Func<ReadOnlyMemory<byte>, int, BattleOptionMenuText>? Value { get; set; }

    /// <summary>원문의 같은 controlled 생성이다.</summary>
    public Action? BeginFancy { get; set; }

    /// <summary>원문의 같은 controlled 생성이다.</summary>
    public Action? BeginVertical { get; set; }

    /// <summary>Current를 현재루트/원시좌표에 부착한다.</summary>
    public Action? Attach { get; set; }

    /// <summary>실제 AddTitle이다.</summary>
    public Action<string>? Title { get; set; }

    /// <summary>include1/force0와 원문 enabled를 같은 AddText에 전달한다.</summary>
    public Action<string, uint, uint, bool>? Item { get; set; }

    /// <summary>현재 하위 controlled 칸을 꺼내 같은 메뉴 토큰을 반환한다.</summary>
    public Func<uint>? Take { get; set; }

    /// <summary>현재 부모에 실제 하위 메뉴를 추가한다.</summary>
    public Action<string, uint>? Submenu { get; set; }

    /// <summary>00541348의 현재 TYPE 개수를 각 반복 조건에서 읽는다.</summary>
    public Func<int>? TypeCount { get; set; }

    /// <summary>현재 TYPE+98/+ec를 같은 저장소에서 읽는다.</summary>
    public Func<uint, OriginalTypeRecord>? Type { get; set; }

    /// <summary>기존 CRT/453f70 정렬이 같은 현재 표를 빌린다.</summary>
    public Func<IReadOnlyList<OriginalTypeRecord>>? Types { get; set; }

    /// <summary>52f7e0의 현재 전역 옵션 조회이며 편집 중인 전투 옵션으로 대체하지 않는다.</summary>
    public Func<uint, bool>? GloballyExcluded { get; set; }

    /// <summary>492960(type,0,1)의 실제마나/설명·현재기술/생산 조회를 수행한다.</summary>
    public Func<uint, KnowledgeRecord>? Knowledge { get; set; }

    /// <summary>현재 TYPE의49af80 번역/설정치환을 메뉴 작성 때 다시 읽는다.</summary>
    public Func<uint, string>? Description { get; set; }
}

// BattleInfo의 정렬·전체·개별 메뉴 자식을 같은 controlled 스택에 작성한다. 제외·제거 메뉴는 후속 단위에서 잇는다.
public static class NativeBattleInfoMenus
{
    /// <summary>선택한 원문 위치에서만 장치를 요구한다. 0.917에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("BattleInfo 메뉴의 실제 현재 장치 미연결");

    /// <summary>제목/설명을 고정%s 인자로 복사하고 실제256BYTE 한도를 유지한다. 0.917에서 추가됐다.</summary>
    private static string Format(string pattern, string value)
    {
        string text = pattern.Replace("%s", value, StringComparison.Ordinal);
        if (text.Length >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(pattern), "BattleInfo 메뉴의 실제256BYTE 서식 범위");
        }

        return text;
    }

    /// <summary>각 행은 choice→current→명령→choice 재조회 순서이며 비교는 임시string의 주소/내용으로 바꾸지 않는다. 0.917에서 추가됐다.</summary>
    private static void Choices(ReadOnlyMemory<byte> options, int index, int maximum, NativeBattleInfoMenuHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        for (int selected = 0; selected <= maximum; selected++)
        {
            BattleOptionMenuText first = Need(hooks.Choice)(index, unchecked((uint)selected));
            BattleOptionMenuText current = Need(hooks.Value)(options, index);
            bool enabled = first.Identity != current.Identity;
            uint parameter = (unchecked((uint)index) << 16) | unchecked((uint)selected);
            uint command = Need(hooks.Read)(0x542b7c);
            BattleOptionMenuText text = Need(hooks.Choice)(index, unchecked((uint)selected));
            Need(hooks.Item)(text.Text, command, parameter, enabled);
        }
    }

    /// <summary>정렬1/2의 원문 현재mode별 enabled와 Fancy 제목/부착이다. 0.917에서 추가됐다.</summary>
    /// <param name="node">같은 0x16c BattleInfo 노드다.</param>
    /// <param name="hooks">실제 메뉴 작성 장치다.</param>
    public static void BuildBattleSortMenu(HudBattleInfoNode node, NativeBattleInfoMenuHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.BeginFancy)();
        Need(hooks.Title)(Need(hooks.Translate)("Sort Game Options By\n"));
        uint first = Need(hooks.Read)(0x542b80);
        Need(hooks.Item)("Show Only Changed Options", first, 1, node.Mode != 1);
        uint second = Need(hooks.Read)(0x542b80);
        Need(hooks.Item)("Show All Options", second, 2, node.Mode != 2);
        Need(hooks.Attach)();
    }

    /// <summary>활성 설정1..21의 모든 선택값을 각각 세로 하위 메뉴로 만든다. 0.917에서 추가됐다.</summary>
    /// <param name="node">같은 0x16c BattleInfo 노드다.</param>
    /// <param name="options">같은 현재 0x59BYTE 전투 옵션이다.</param>
    /// <param name="hooks">실제 메뉴 작성 장치다.</param>
    public static void BuildBattleOptionsMenu(HudBattleInfoNode node, ReadOnlyMemory<byte> options, NativeBattleInfoMenuHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.BeginFancy)();
        Need(hooks.Title)(Need(hooks.Translate)("Battle Options\n"));
        for (int index = 1; index < 22; index++)
        {
            // 게임 종류0은 전체 설정 하위 메뉴에서 제외한다.
            if (node.Settings[index, 1] == 0)
            {
                continue;
            }

            Need(hooks.BeginVertical)();
            int maximum = BattleOptionMutations.MaximumBattleOptionChoice(index);
            Choices(options, index, maximum, hooks);
            uint child = Need(hooks.Take)();
            string name = BattleOptionTexts.BattleOptionNameText(unchecked((uint)index), Need(hooks.Translate));
            Need(hooks.Submenu)(name, child);
        }

        Need(hooks.Attach)();
    }

    /// <summary>개별설정의 포함최댓값/현재포인터 비교·현재이름 제목/부착이다. 0.917에서 추가됐다.</summary>
    /// <param name="options">같은 현재 0x59BYTE 전투 옵션이다.</param>
    /// <param name="index">개별 설정 번호다.</param>
    /// <param name="hooks">실제 메뉴 작성 장치다.</param>
    public static void BuildBattleOptionMenu(ReadOnlyMemory<byte> options, int index, NativeBattleInfoMenuHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.BeginFancy)();
        int maximum = BattleOptionMutations.MaximumBattleOptionChoice(index);
        string name = BattleOptionTexts.BattleOptionNameText(unchecked((uint)index), Need(hooks.Translate));
        Need(hooks.Title)(Format("%s\n", name));
        Choices(options, index, maximum, hooks);
        Need(hooks.Attach)();
    }
}
