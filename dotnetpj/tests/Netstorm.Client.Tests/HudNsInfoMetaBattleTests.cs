// 일반 부착/두 현재 관문 뒤에는 비활성 전투도 공통 스크롤/자식 배치를 수행한다.
// cpppj/src/client/HudNsInfoMeta.cpp 의 LayoutMetaBattleInfo(0047b480) 를 이식한다.
// 0.910에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 CS/노드와 호출을 기록하는 배틀 패널 본문의 계약이다.</summary>
public sealed class HudNsInfoMetaBattleTests
{
    /// <summary>같은 실제 CS/노드와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS 참가자40·전투10 기록이다.</summary>
        public ClientChallengeGlobals Challenge { get; } = new();

        /// <summary>같은 005930b0 정렬 표/현재 NSInfo다.</summary>
        public ClientNsInfoGlobals Info { get; } = new();

        /// <summary>같은 594fc8의 현재 메타맵 모드다.</summary>
        public uint Metamap { get; set; } = 1;

        /// <summary>같은 595344의 현재 플레이어 인식 값이다.</summary>
        public uint Known { get; set; } = 1;

        /// <summary>같은 540c70의 현재 로컬 소유자다.</summary>
        public uint Owner { get; set; } = 1;

        /// <summary>실제 0x16c BattleInfo 노드다.</summary>
        public HudBattleInfoNode Panel { get; } = new() { Title = 301, Summary = 302, Body = 303 };

        /// <summary>현재 실제 노드 주소다.</summary>
        public Dictionary<uint, GumpCreateNode> Nodes { get; } = [];

        /// <summary>실제 가상 Push 순서다.</summary>
        public List<uint> Pushed { get; } = [];

        /// <summary>실제 부착 호출이다.</summary>
        public List<(uint Token, float X, float Y, uint Parent, uint Mode)> Attached { get; } = [];

        /// <summary>실제 구문 분석 호출이다.</summary>
        public List<(uint Token, string Text)> Parsed { get; } = [];

        /// <summary>실제 문자열 대체 호출이다.</summary>
        public List<(uint Token, string Text)> Replaced { get; } = [];

        /// <summary>실제 맞춘 내용이다.</summary>
        public List<uint> Fitted { get; } = [];

        /// <summary>실제 요청한 상단이다.</summary>
        public List<uint> RequestedTop { get; } = [];

        /// <summary>실제 맞춘 내용이다.</summary>
        public List<uint> ContentFitted { get; } = [];

        /// <summary>실제 편집 플래그 상자다.</summary>
        public Dictionary<uint, UIntBox> Flags { get; } = [];

        /// <summary>실제 읽은 주소다.</summary>
        public List<uint> ReadCalls { get; } = [];

        /// <summary>현재 팔레트색 읽기 값이다.</summary>
        public Dictionary<uint, uint> Reads { get; } = [];

        /// <summary>같은 실제 장치 묶음이다.</summary>
        public Scene()
        {
            Challenge.Prepare();
            Panel.BaseWidth = 400;
            Panel.BaseHeight = 60;
            Panel.Scale = 1;
            Nodes[301] = new GumpCreateNode { BaseWidth = 380, BaseHeight = 40, Scale = 1 };
            Nodes[302] = new GumpCreateNode { BaseWidth = 380, BaseHeight = 30, Scale = 1 };
            Nodes[303] = new GumpCreateNode { BaseWidth = 380, BaseHeight = 100, Scale = 1 };
            Nodes[900] = Panel;
            Flags[301] = new UIntBox();
            Flags[303] = new UIntBox();
        }

        /// <summary>같은 실제 전역 묶음이다.</summary>
        public HudNsInfoMetaGlobals Globals() => new() { Challenge = Challenge, Info = Info, Metamap = () => Metamap, Known = () => Known, Owner = () => Owner };

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public HudNsInfoMetaHooks Hooks() => new()
        {
            Battle = _ => Panel,
            Node = token => Nodes.TryGetValue(token, out GumpCreateNode? node) ? node : null,
            Push = token => Pushed.Add(token),
            FitHeight = token => Fitted.Add(token),
            Attach = (token, x, y, parent, mode) => Attached.Add((token, x, y, parent, mode)),
            Replace = (token, text) => Replaced.Add((token, text)),
            Parse = (token, text) => Parsed.Add((token, text)),
            Read = address => { ReadCalls.Add(address); return Reads.TryGetValue(address, out uint value) ? value : 0u; },
            TextFlags = token => Flags.TryGetValue(token, out UIntBox? box) ? box : throw new InvalidOperationException("플래그 누락"),
            Scrollable = (_, _) => { },
            Slider = (_, _) => { },
            RequestTop = token => RequestedTop.Add(token),
            FitContent = token => ContentFitted.Add(token),
            Translate = text => text,
            OptionValueText = (_, option) => $"V{option}",
            OptionNameText = option => $"N{option}",
            Describe = type => $"D{type}",
        };
    }

    /// <summary>전투 없음 분기는 내용을 생략하고 부착한다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void EmptyBattleSkipsContentButAttaches()
    {
        var scene = new Scene();
        scene.Panel.Battle = 0;
        HudNsInfoMetaBattle.Layout(900, scene.Globals(), scene.Hooks());
        Assert.DoesNotContain(scene.Parsed, row => row.Text.Contains("Game Type", StringComparison.Ordinal));
        Assert.Contains((301u, 0f, 0f, 900u, 0u), scene.Attached);
        Assert.Contains((302u, 0f, 62f, 900u, 0u), scene.Attached);
        Assert.Contains((303u, 0f, 32f, 900u, 0u), scene.Attached);
        Assert.Equal(1u, scene.Panel.Bytes[0]);
        HudNsInfoMetaBattle.Layout(900, scene.Globals(), scene.Hooks());
        Assert.Equal(1, scene.RequestedTop.Count(row => row == 301u));
        Assert.Equal(1, scene.RequestedTop.Count(row => row == 303u));
    }

    /// <summary>게임 종류별 현재 설정 쌍을 원문 순서로 쓴다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void PrepareSettingsWritesGameTypePairs()
    {
        var plain = new HudBattleInfoNode();
        HudNsInfoMetaBattle.PrepareSettings(plain, 0);
        Assert.Equal(3u, plain.Settings[4, 0]);
        Assert.Equal(0u, plain.Settings[5, 1]);
        var one = new HudBattleInfoNode();
        HudNsInfoMetaBattle.PrepareSettings(one, 1);
        Assert.Equal(2u, one.Settings[4, 0]);
        Assert.Equal(0u, one.Settings[5, 1]);
        var two = new HudBattleInfoNode();
        HudNsInfoMetaBattle.PrepareSettings(two, 2);
        Assert.Equal(1u, two.Settings[5, 1]);
        Assert.Equal(0u, two.Settings[19, 1]);
        var three = new HudBattleInfoNode();
        HudNsInfoMetaBattle.PrepareSettings(three, 3);
        Assert.Equal(0u, three.Settings[5, 1]);
        Assert.Equal(2u, three.Settings[19, 0]);
        var four = new HudBattleInfoNode();
        HudNsInfoMetaBattle.PrepareSettings(four, 4);
        Assert.Equal(1u, four.Settings[5, 1]);
        Assert.Equal(2u, four.Settings[18, 0]);
        Assert.Equal(3u, four.Settings[4, 0]);
        Assert.Equal(0u, four.Settings[9, 0]);
    }

    /// <summary>mode2는 활성 전체 값만 쓰고 그 외는 바뀐 활성행만 쓴다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void ModeTwoWritesOnlyActiveRows()
    {
        var scene = new Scene();
        scene.Panel.Battle = 2;
        scene.Panel.Mode = 2;
        scene.Panel.Settings[1, 1] = 1;
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        battle[1] = 9;
        battle[2] = 9;
        HudNsInfoMetaBattle.Layout(900, scene.Globals(), scene.Hooks());
        Assert.Contains(scene.Parsed, row => row.Text.Contains("~[T2]", StringComparison.Ordinal));
        Assert.DoesNotContain(scene.Parsed, row => row.Text.Contains("~[T4]", StringComparison.Ordinal));
    }

    /// <summary>일반 모드는 현재 값이 기준과 다른 활성행만 쓴다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void NormalModeWritesOnlyChangedRows()
    {
        var scene = new Scene();
        scene.Panel.Battle = 2;
        scene.Panel.Mode = 0;
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        battle[1] = 2;
        battle[2] = 9;
        HudNsInfoMetaBattle.Layout(900, scene.Globals(), scene.Hooks());
        Assert.DoesNotContain(scene.Parsed, row => row.Text.Contains("~[T2]", StringComparison.Ordinal));
        Assert.Contains(scene.Parsed, row => row.Text.Contains("~[T3]", StringComparison.Ordinal));
    }

    /// <summary>제외 목록과 Click To Add 행을 쓴다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void ExcludedListAndClickToAdd()
    {
        var scene = new Scene();
        scene.Panel.Battle = 2;
        scene.Panel.Mode = 0;
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        battle[0x16] = 1;
        battle[0x17] = 5;
        HudNsInfoMetaBattle.Layout(900, scene.Globals(), scene.Hooks());
        Assert.Contains(scene.Parsed, row => row.Text == "~j~E1) ~[T127]~N~wD5~[T0]\n");
        Assert.Contains(scene.Parsed, row => row.Text == "~j~E2) ~[T1000]~N~wClick To Add~[T0]\n");
        Assert.Contains(scene.Parsed, row => row.Text == "~y~E~[T1000]Excluded:~[T0]\n");
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.910에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        scene.Panel.Battle = 0;
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaBattle.Layout(900, scene.Globals(), new HudNsInfoMetaHooks()));
        var hooks = scene.Hooks();
        hooks.Parse = null;
        scene.Panel.Battle = 2;
        scene.Challenge.Battle(2).Span[0] = 1;
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaBattle.Layout(900, scene.Globals(), hooks));
        Assert.Throws<ArgumentNullException>(() => HudNsInfoMetaBattle.Layout(900, null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => HudNsInfoMetaBattle.PrepareSettings(null!, 1));
    }
}
