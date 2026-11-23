// BattleInfo의 정렬·전체·개별 메뉴 자식을 원문 선택 문자열/조건부 번역과 같은 controlled 스택에 작성한다.
// cpppj/src/client/NativeBattleInfoMenus.cpp 의 전반 3종을 이식한다.
// 0.917에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
// 제외·제거 메뉴 2종은 0.918에서 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 노드/옵션과 메뉴 작성 호출의 계약이다.</summary>
public sealed class BattleInfoMenusTests
{
    /// <summary>같은 실제 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 0x16c BattleInfo 노드다.</summary>
        public HudBattleInfoNode Node { get; } = new() { Mode = 2 };

        /// <summary>같은 현재 0x59BYTE 전투 옵션이다.</summary>
        public byte[] Options { get; } = new byte[0x59];

        /// <summary>실제 메뉴 제목이다.</summary>
        public List<string> Titles { get; } = [];

        /// <summary>실제 메뉴 항목이다.</summary>
        public List<(string Text, uint Command, uint Parameter, bool Enabled)> Items { get; } = [];

        /// <summary>실제 하위 메뉴다.</summary>
        public List<(string Name, uint Child)> Submenus { get; } = [];

        /// <summary>실제 생성/부착 횟수다.</summary>
        public int Frames { get; private set; }

        /// <summary>다음 하위 토큰이다.</summary>
        public uint NextChild { get; set; } = 100;

        /// <summary>현재 TYPE 개수다.</summary>
        public int TypeCount { get; set; } = 70;

        /// <summary>같은 현재 TYPE 표다.</summary>
        public List<OriginalTypeRecord> Types { get; } = [];

        /// <summary>전역 제외 TYPE 집합이다.</summary>
        public HashSet<uint> ExcludedGlobals { get; } = [];

        /// <summary>같은 별칭 주소의 TYPE 값이다.</summary>
        public Dictionary<uint, uint> Aliases { get; } = [];

        /// <summary>실제 선택 정체성이다.</summary>
        public Dictionary<(int Index, uint Selected), ulong> Identities { get; } = [];

        /// <summary>현재 선택 정체성이다.</summary>
        public ulong CurrentIdentity { get; set; } = 7;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativeBattleInfoMenuHooks Hooks() => new()
        {
            Read = address => address switch { 0x542b80 => 0xA0u, 0x542b7c => 0xA1u, 0x5429e0 => 0xB0u, 0x5429e4 => 0xB1u, 0x5412f8 => 0xC0u, _ => Aliases.TryGetValue(address, out uint type) ? type : 0u },
            Translate = text => text,
            Choice = (index, selected) => new BattleOptionMenuText($"C{index}/{selected}", Identities.TryGetValue((index, selected), out ulong id) ? id : selected),
            Value = (_, _) => new BattleOptionMenuText("current", CurrentIdentity),
            BeginFancy = () => Frames++,
            BeginVertical = () => Frames++,
            Attach = () => Frames++,
            Title = text => Titles.Add(text),
            Item = (text, command, parameter, enabled) => Items.Add((text, command, parameter, enabled)),
            Take = () => NextChild++,
            Submenu = (name, child) => Submenus.Add((name, child)),
            TypeCount = () => TypeCount,
            Type = type => type < (uint)Types.Count ? Types[(int)type] : throw new InvalidOperationException("TYPE 누락"),
            Types = () => Types,
            GloballyExcluded = type => ExcludedGlobals.Contains(type),
            Knowledge = type => new KnowledgeRecord { Type = type, Available = true, Description = $"D{type}" },
            Description = type => $"D{type}",
        };
    }

    /// <summary>정렬 메뉴는 현재 모드에 따라 항목을 끈다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void SortMenuDisablesCurrentMode()
    {
        var scene = new Scene();
        NativeBattleInfoMenus.BuildBattleSortMenu(scene.Node, scene.Hooks());
        Assert.Equal(["Sort Game Options By\n"], scene.Titles);
        Assert.Equal(2, scene.Items.Count);
        Assert.Equal(("Show Only Changed Options", 0xA0u, 1u, true), scene.Items[0]);
        Assert.Equal(("Show All Options", 0xA0u, 2u, false), scene.Items[1]);
        Assert.Equal(2, scene.Frames);
    }

    /// <summary>전체 메뉴는 활성 설정만 세로 하위로 만든다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void OptionsMenuBuildsOnlyActiveSettings()
    {
        var scene = new Scene();
        scene.Node.Settings[1, 1] = 1;
        scene.Node.Settings[3, 1] = 1;
        scene.Options[1] = 1;
        scene.Options[3] = 0;
        NativeBattleInfoMenus.BuildBattleOptionsMenu(scene.Node, scene.Options, scene.Hooks());
        Assert.Equal(["Battle Options\n"], scene.Titles);
        // 설정1(3값)·설정3(2값)의 모든 선택 행이다.
        Assert.Equal(3 + 2, scene.Items.Count);
        Assert.Equal(2, scene.Submenus.Count);
        Assert.Equal("Bridge Slots", scene.Submenus[0].Name);
        Assert.Equal("Knowledge", scene.Submenus[1].Name);
        Assert.Equal(4, scene.Frames);
    }

    /// <summary>개별 메뉴는 이름 제목과 모든 선택 행을 쓴다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void OptionMenuWritesTitleAndAllChoices()
    {
        var scene = new Scene();
        NativeBattleInfoMenus.BuildBattleOptionMenu(scene.Options, 1, scene.Hooks());
        Assert.Equal(["Bridge Slots\n"], scene.Titles);
        Assert.Equal(3, scene.Items.Count);
        Assert.Equal((1u << 16) | 0u, scene.Items[0].Parameter);
        Assert.Equal(2, scene.Frames);
    }

    /// <summary>현재 선택과 같은 행은 비활성화한다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void CurrentChoiceRowIsDisabled()
    {
        var scene = new Scene();
        scene.Identities[(1, 1)] = 9;
        scene.CurrentIdentity = 9;
        NativeBattleInfoMenus.BuildBattleOptionMenu(scene.Options, 1, scene.Hooks());
        Assert.True(scene.Items[0].Enabled);
        Assert.False(scene.Items[1].Enabled);
        Assert.True(scene.Items[2].Enabled);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoMenus.BuildBattleSortMenu(scene.Node, new NativeBattleInfoMenuHooks()));
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoMenus.BuildBattleOptionsMenu(scene.Node, scene.Options, new NativeBattleInfoMenuHooks()));
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoMenus.BuildBattleOptionMenu(scene.Options, 1, new NativeBattleInfoMenuHooks()));
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoMenus.BuildBattleExcludeMenu(new NativeBattleInfoMenuHooks()));
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoMenus.BuildBattleRemoveMenu(70, new NativeBattleInfoMenuHooks()));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoMenus.BuildBattleSortMenu(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoMenus.BuildBattleOptionMenu(scene.Options, 1, null!));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoMenus.BuildBattleExcludeMenu(null!));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoMenus.BuildBattleRemoveMenu(70, null!));
    }

    /// <summary>제외 메뉴는 분류·정렬 뒤 존재하는 하위만 Spells/Sun/Wind/Rain/Thunder순으로 쓴다. 0.918에서 추가됐다.</summary>
    [Fact]
    public void ExcludeMenuWritesExistingGroupsInMenuOrder()
    {
        var scene = new Scene();
        scene.TypeCount = 74;
        for (int i = 0; i < 70; i++)
        {
            scene.Types.Add(new OriginalTypeRecord { Theme = 4, Flags2 = 0 });
        }

        scene.Types.AddRange([
            new OriginalTypeRecord { Theme = 0, Flags2 = 0 },
            new OriginalTypeRecord { Theme = 1, Flags2 = 0 },
            new OriginalTypeRecord { Theme = 3, Flags2 = 0 },
            new OriginalTypeRecord { Theme = 2, Flags2 = 0x100 },
        ]);
        scene.Aliases[0x5412d0] = 71;
        scene.ExcludedGlobals.Add(72);
        NativeBattleInfoMenus.BuildBattleExcludeMenu(scene.Hooks());
        Assert.Equal(["Exclude which unit:\n"], scene.Titles);
        // 70(Wind)·73(Spells, spells 비트지만 5412f8 불일치)·72는 전역 제외·71은 별칭 제외다.
        // 기타 그룹의 0~69 더미는 메뉴에 출력하지 않는다.
        Assert.Equal(2, scene.Items.Count);
        Assert.Equal("~y~ED73", scene.Items[0].Text);
        Assert.Equal((0xB0u, 73u, true), (scene.Items[0].Command, scene.Items[0].Parameter, scene.Items[0].Enabled));
        Assert.Equal("~y~ED70", scene.Items[1].Text);
        Assert.Equal((0xB0u, 70u, true), (scene.Items[1].Command, scene.Items[1].Parameter, scene.Items[1].Enabled));
        Assert.Equal(2, scene.Submenus.Count);
        Assert.Equal(("Spells", 100u), scene.Submenus[0]);
        Assert.Equal(("Wind", 101u), scene.Submenus[1]);
    }

    /// <summary>제거 메뉴는 설명 제목과 Yes/No 항목을 쓴다. 0.918에서 추가됐다.</summary>
    [Fact]
    public void RemoveMenuWritesConfirmItems()
    {
        var scene = new Scene();
        scene.Types.Add(new OriginalTypeRecord { Theme = 0, Flags2 = 0 });
        NativeBattleInfoMenus.BuildBattleRemoveMenu(0, scene.Hooks());
        Assert.Equal(["Remove exclusion of D0?\n"], scene.Titles);
        Assert.Equal(2, scene.Items.Count);
        Assert.Equal(("Yes", 0xB1u, 0u, true), scene.Items[0]);
        Assert.Equal(("No", 0xB1u, 0u, true), scene.Items[1]);
        Assert.Equal(2, scene.Frames);
    }
}
