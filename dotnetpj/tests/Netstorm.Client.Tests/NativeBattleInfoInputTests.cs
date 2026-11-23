// BattleInfo의 입력 분기/현재 옵션 기본값 적용과 메뉴·재부착·송신의 실제 부모를 복원한다.
// cpppj/src/client/NativeBattleInfoInput.cpp 의 HandleNativeBattleInfo(0047b8f0) 를 이식한다.
// 0.915에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 노드/전투 기록과 메뉴·재부착·송신 호출의 계약이다.</summary>
public sealed class NativeBattleInfoInputTests
{
    /// <summary>같은 실제 노드/전투 기록과 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 0x16c BattleInfo 노드다.</summary>
        public HudBattleInfoNode Node { get; } = new() { Title = 21, Battle = 2, Mode = 2, Changed = 1 };

        /// <summary>같은 실제 CS 전투 기록이다.</summary>
        public byte[] Record { get; } = new byte[0x100];

        /// <summary>제목 serial이다.</summary>
        public uint TitleSerial { get; set; } = 0xB1;

        /// <summary>같은 현재594fc8이다.</summary>
        public uint Metamap { get; set; } = 1;

        /// <summary>실제 메뉴 호출이다.</summary>
        public List<string> Menus { get; } = [];

        /// <summary>실제 가상 Push다.</summary>
        public List<uint> Pushed { get; } = [];

        /// <summary>실제 가상 부착이다.</summary>
        public List<(uint Token, float X, float Y, uint Parent, uint Mode)> Attached { get; } = [];

        /// <summary>실제 송신이다.</summary>
        public List<(uint Battle, byte[] Options, uint Kind)> DataCalls { get; } = [];

        /// <summary>실제 음향 횟수다.</summary>
        public int Sounds { get; private set; }

        /// <summary>실제 전투 기록을 준비한다.</summary>
        public Scene()
        {
            Record[0] = 1;
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativeBattleInfoInputHooks Hooks() => new()
        {
            Metamap = () => Metamap,
            Battle = _ => Record,
            Command = address => address switch { 0x54278c => 0x10u, 0x542b80 => 0x11u, 0x542b7c => 0x12u, 0x5429e0 => 0x13u, 0x5429e4 => 0x14u, _ => 0u },
            Serial = _ => TitleSerial,
            GuySound = () => Sounds++,
            SortMenu = _ => Menus.Add("sort"),
            OptionsMenu = (_, _) => Menus.Add("options"),
            ExcludeMenu = (_, _) => Menus.Add("exclude"),
            OptionMenu = (_, _, index) => Menus.Add($"option{index}"),
            RemoveMenu = type => Menus.Add($"remove{type}"),
            Push = token => Pushed.Add(token),
            Attach = (token, x, y, parent, mode) => Attached.Add((token, x, y, parent, mode)),
            Data = (battle, options, kind) => DataCalls.Add((battle, options.ToArray(), kind)),
        };
    }

    /// <summary>비활성 전투와 꺼진 메타맵은 0을 반환한다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void InactiveBattleAndMetamapOffReturnZero()
    {
        var scene = new Scene();
        scene.Record[0] = 0;
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1, scene.Hooks()));
        scene.Record[0] = 1;
        scene.Metamap = 0;
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1, scene.Hooks()));
        Assert.Empty(scene.Menus);
        Assert.Empty(scene.Pushed);
    }

    /// <summary>mode2 첫 행은 순환·게임 종류 재설정·기본값 적용 뒤 송신한다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void ModeTwoFirstRowCyclesAndAppliesDefaults()
    {
        var scene = new Scene();
        scene.Node.Settings[0, 0] = 2;
        scene.Node.Settings[1, 0] = 5;
        uint result = NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(1, scene.Sounds);
        Assert.Equal(scene.Node.Settings[1, 0], scene.Record[1]);
        Assert.Single(scene.DataCalls);
        Assert.Equal((2u, 0x57u), (scene.DataCalls[0].Battle, scene.DataCalls[0].Kind));
        Assert.Single(scene.Pushed);
        Assert.Single(scene.Attached);
    }

    /// <summary>일반 모드는 설정 쌍을 쓰고 정렬 메뉴는 변경 없이 연다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void NormalModeSetsPairAndSortMenuNeedsNoChange()
    {
        var scene = new Scene();
        scene.Node.Mode = 0;
        scene.Node.Settings[3, 0] = 1;
        uint result = NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 4, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(1, scene.Record[3]);
        Assert.Single(scene.DataCalls);
        var plain = new Scene();
        plain.Node.Changed = 0;
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, plain.Node, 0x10, 0xB1, 24, plain.Hooks()));
        Assert.Equal(["sort"], plain.Menus);
        Assert.Empty(plain.DataCalls);
    }

    /// <summary>옵션·제외·제거 메뉴는 변경 잠금 때 0을 반환한다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void OptionMenusRespectChangedLock()
    {
        var scene = new Scene();
        scene.Node.Changed = 0;
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 23, scene.Hooks()));
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1000, scene.Hooks()));
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 122, scene.Hooks()));
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 30, scene.Hooks()));
        Assert.Empty(scene.Menus);
        var open = new Scene();
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, open.Node, 0x10, 0xB1, 23, open.Hooks()));
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, open.Node, 0x10, 0xB1, 1000, open.Hooks()));
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, open.Node, 0x10, 0xB1, 150, open.Hooks()));
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, open.Node, 0x10, 0xB1, 30, open.Hooks()));
        Assert.Equal(["options", "exclude", "remove28", "option3"], open.Menus);
    }

    /// <summary>모드 변경은 재부착만 하고 직접 설정·제외 송신을 수행한다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void ModeChangeAndDirectSetAndExclude()
    {
        var scene = new Scene();
        Assert.Equal(0u, NativeBattleInfoInput.Handle(900, scene.Node, 0x11, 0, 3, scene.Hooks()));
        Assert.Equal(3u, scene.Node.Mode);
        Assert.Single(scene.Pushed);
        Assert.Empty(scene.DataCalls);
        uint set = NativeBattleInfoInput.Handle(900, scene.Node, 0x12, 0, (1u << 16) | 2u, scene.Hooks());
        Assert.Equal(0u, set);
        Assert.Equal(2, scene.Record[1]);
        Assert.Single(scene.DataCalls);
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, scene.Node, 0x13, 0, 9, scene.Hooks()));
        Assert.Equal(1, scene.Record[0x16]);
        Assert.Equal(9, scene.Record[0x17]);
        Assert.Equal(1u, NativeBattleInfoInput.Handle(900, scene.Node, 0x14, 0, 9, scene.Hooks()));
        Assert.Equal(0, scene.Record[0x16]);
    }

    /// <summary>기본값 적용은 1..21을 원문 순서로 쓴다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void ApplyDefaultsWritesOneThroughTwentyOne()
    {
        var node = new HudBattleInfoNode();
        node.Settings[1, 0] = 2;
        node.Settings[21, 0] = 3;
        byte[] options = new byte[0x59];
        NativeBattleInfoInput.ApplyMetaBattleDefaults(node, options);
        Assert.Equal(2, options[1]);
        Assert.Equal(3, options[21]);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.915에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1, new NativeBattleInfoInputHooks()));
        var hooks = scene.Hooks();
        hooks.Push = null;
        Assert.Throws<InvalidOperationException>(() => NativeBattleInfoInput.Handle(900, scene.Node, 0x11, 0, 3, hooks));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoInput.Handle(900, null!, 0x10, 0xB1, 1, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoInput.Handle(900, scene.Node, 0x10, 0xB1, 1, null!));
        Assert.Throws<ArgumentNullException>(() => NativeBattleInfoInput.ApplyMetaBattleDefaults(null!, new byte[0x59]));
    }
}
