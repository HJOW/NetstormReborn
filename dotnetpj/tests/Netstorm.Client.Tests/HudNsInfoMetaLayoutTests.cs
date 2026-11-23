// 일반 앞단/관문 뒤 NSInfo 전체 본문 배치를 검사한다.
// cpppj/src/client/HudNsInfoMeta.cpp 의 LayoutMetaNsInfo(0047e9d0: 일반 앞단/관문 뒤 전체 본문) 를 이식한다.
// 0.907에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 CS/노드와 호출을 기록하는 전체 본문 배치의 계약이다.</summary>
public sealed class HudNsInfoMetaLayoutTests
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

        /// <summary>실제 0xd8 NSInfo 노드다.</summary>
        public HudNsInfoNode Root { get; } = new();

        /// <summary>실제 0x16c BattleInfo 노드다.</summary>
        public HudBattleInfoNode Battle { get; } = new();

        /// <summary>실제 0xbc PlayerInfo 노드 8개다.</summary>
        public HudPlayerInfoNode[] Panels { get; } = [new(), new(), new(), new(), new(), new(), new(), new()];

        /// <summary>현재 실제 노드 주소다.</summary>
        public Dictionary<uint, GumpCreateNode> Nodes { get; } = [];

        /// <summary>실제 가상 Push 순서다.</summary>
        public List<uint> Pushed { get; } = [];

        /// <summary>실제 맞춘 내용이다.</summary>
        public List<uint> Fitted { get; } = [];

        /// <summary>실제 부착 호출이다.</summary>
        public List<(uint Token, float X, float Y, uint Parent, uint Mode)> Attached { get; } = [];

        /// <summary>실제 구문 분석 호출이다.</summary>
        public List<(uint Token, string Text)> Parsed { get; } = [];

        /// <summary>실제 도움말 교체다.</summary>
        public List<(uint Token, string Text)> HelpTexts { get; } = [];

        /// <summary>실제 도움말 단어 교체다.</summary>
        public List<(uint Token, string Text)> HelpWords { get; } = [];

        /// <summary>현재 팔레트색/metadataMode 읽기 값이다.</summary>
        public Dictionary<uint, uint> Reads { get; } = new()
        {
            [0x5c78d0u] = 0x11u,
            [0x59afd8u] = 0x22u,
            [0x5c85c8u] = 0u,
        };

        /// <summary>같은 실제 장치 묶음이다.</summary>
        public Scene()
        {
            Challenge.Prepare();
            Root.Background = 1;
            Root.Title = 2;
            Root.UpperDivider = 3;
            Root.Roster = 4;
            Root.LowerDivider = 5;
            Root.Battle = 6;
            Root.BaseWidth = 400;
            Root.BaseHeight = 300;
            for (int index = 0; index < 8; index++)
            {
                Root.Players[index] = (uint)(10 + index);
            }

            Nodes[1] = new GumpCreateNode { BaseWidth = 400, BaseHeight = 300, Scale = 1 };
            Nodes[2] = new GumpCreateNode { BaseWidth = 400, BaseHeight = 20, Scale = 1 };
            Nodes[3] = new GumpCreateNode { BaseWidth = 400, BaseHeight = 5, Scale = 1 };
            Nodes[4] = new GumpCreateNode { BaseWidth = 400, BaseHeight = 100, Scale = 1 };
            Nodes[5] = new GumpCreateNode { BaseWidth = 400, BaseHeight = 5, Scale = 1 };
            Nodes[6] = Battle;
            Battle.BaseWidth = 400;
            Battle.BaseHeight = 50;
            Battle.Scale = 1;
            for (int index = 0; index < 8; index++)
            {
                Panels[index].BaseWidth = 400;
                Panels[index].BaseHeight = 30;
                Panels[index].Scale = 1;
                Nodes[(uint)(10 + index)] = Panels[index];
            }
        }

        /// <summary>같은 실제 전역 묶음이다.</summary>
        public HudNsInfoMetaGlobals Globals() => new() { Challenge = Challenge, Info = Info, Metamap = () => Metamap, Known = () => Known, Owner = () => Owner };

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public HudNsInfoMetaHooks Hooks() => new()
        {
            Root = _ => Root,
            Player = token => Panels[token - 10],
            Battle = _ => Battle,
            Node = token => Nodes.GetValueOrDefault(token),
            Push = token => Pushed.Add(token),
            FitHeight = token => Fitted.Add(token),
            Attach = (token, x, y, parent, mode) => Attached.Add((token, x, y, parent, mode)),
            Parse = (token, text) => Parsed.Add((token, text)),
            Header = battle => battle ? "BH1" : "BH0",
            HelpText = (token, text) => HelpTexts.Add((token, text)),
            HelpWord = (token, text) => HelpWords.Add((token, text)),
            Translate = text => text,
            Replace = (token, text) => Replaced.Add((token, text)),
            Wrap = (_, _) => { },
            Read = address => Reads.TryGetValue(address, out uint value) ? value : throw new InvalidOperationException("읽기 누락:" + address.ToString("x")),
            ObjectOwner = _ => 0u,
            Weight = span => BinaryPrimitives.ReadUInt32LittleEndian(span.Span[..4]),
            Append = (token, text, context, row) => Appended.Add((token, text, context.ToArray(), row)),
        };

        /// <summary>대체된 문자열이다.</summary>
        public List<(uint Token, string Text)> Replaced { get; } = [];

        /// <summary>추가된 정렬행이다.</summary>
        public List<(uint Token, string Text, uint[] Context, uint Row)> Appended { get; } = [];

        /// <summary>참가자 기록을 실제 필드에 쓴다.</summary>
        public void WriteParticipant(uint index, uint battle, uint flags, string name = "x")
        {
            var record = Challenge.Participant(index).Span;
            record.Clear();
            Encoding.Latin1.GetBytes(name, record[0x26..]);
            record[0x64] = (byte)flags;
            BinaryPrimitives.WriteUInt32LittleEndian(record[0xa4..], battle);
        }

        /// <summary>전투 슬롯에 실제 참가자를 둔다.</summary>
        public void WriteBattleSlot(uint battle, uint slot, uint participant)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Battle(battle).Span[(0x38 + (int)slot * 4)..], participant);
        }
    }

    /// <summary>전투 없음 분기는 명부를 보이고 전투를 숨긴다. 0.907에서 추가됐다.</summary>
    [Fact]
    public void NoBattleBranchShowsRoster()
    {
        var scene = new Scene();
        scene.WriteParticipant(2, 0, 1, "x");
        scene.WriteParticipant(1, 0, 1, "y");
        HudNsInfoMetaLayout.Layout(100, scene.Globals(), scene.Hooks());
        Assert.Equal((byte)0, scene.Battle.Bytes[0]);
        Assert.NotEqual(0u, scene.Nodes[4].Flags & 1u);
        Assert.Equal(0u, scene.Nodes[6].Flags & 1u);
        Assert.Contains((2u, "BH0"), scene.Parsed);
        Assert.Contains((100u, "Player List Window"), scene.HelpTexts);
        Assert.Contains((100u, "playerListView"), scene.HelpWords);
        Assert.NotEmpty(scene.Appended);
    }

    /// <summary>전투 있음 분기는 첫 마스터만 별도 0번 패널에 놓는다. 0.907에서 추가됐다.</summary>
    [Fact]
    public void BattleBranchPutsFirstMasterOnPanelZero()
    {
        var scene = new Scene();
        scene.WriteParticipant(1, 2, 1 | 4, "me");
        scene.WriteParticipant(3, 2, 1 | 4, "master");
        scene.WriteBattleSlot(2, 0, 3);
        scene.WriteBattleSlot(2, 1, 4);
        scene.WriteParticipant(4, 2, 1, "solo");
        HudNsInfoMetaLayout.Layout(100, scene.Globals(), scene.Hooks());
        Assert.Equal(3u, scene.Panels[0].Player);
        Assert.NotEqual(0u, scene.Nodes[10].Flags & 1u);
        Assert.Equal(2u, scene.Battle.Battle);
        Assert.Equal(4u, scene.Battle.Changed);
        Assert.Contains((100u, "Battle Status Window"), scene.HelpTexts);
        Assert.Contains((100u, "battleStatusView"), scene.HelpWords);
        Assert.Contains((10u, 3f, 25f, 1u, 0u), scene.Attached);
    }

    /// <summary>마스터 없음 분기는 전투를 숨긴다. 0.907에서 추가됐다.</summary>
    [Fact]
    public void BattleBranchWithoutMasterHidesBattle()
    {
        var scene = new Scene();
        scene.WriteParticipant(1, 2, 1, "me");
        scene.WriteParticipant(4, 2, 1, "solo");
        scene.WriteBattleSlot(2, 0, 4);
        HudNsInfoMetaLayout.Layout(100, scene.Globals(), scene.Hooks());
        Assert.Equal(0u, scene.Nodes[6].Flags & 1u);
        Assert.Equal(0u, scene.Nodes[5].Flags & 1u);
        Assert.Equal(4u, scene.Panels[0].Player);
        Assert.Equal(0u, scene.Panels[1].Player);
    }

    /// <summary>남은 패널도 숨긴 뒤 실제 Attach를 호출한다. 0.907에서 추가됐다.</summary>
    [Fact]
    public void UnusedPanelsAreHiddenWithRealAttach()
    {
        var scene = new Scene();
        scene.WriteParticipant(1, 2, 1 | 4, "me");
        scene.WriteParticipant(3, 2, 1 | 4, "m1");
        scene.WriteParticipant(4, 2, 1, "s1");
        scene.WriteParticipant(5, 2, 1, "s2");
        scene.WriteBattleSlot(2, 0, 3);
        scene.WriteBattleSlot(2, 1, 4);
        scene.WriteBattleSlot(2, 2, 5);
        HudNsInfoMetaLayout.Layout(100, scene.Globals(), scene.Hooks());
        Assert.Equal(3u, scene.Panels[0].Player);
        Assert.Equal(4u, scene.Panels[1].Player);
        Assert.Equal(5u, scene.Panels[2].Player);
        for (int index = 3; index < 8; index++)
        {
            Assert.Equal(0u, scene.Panels[index].Player);
            Assert.Equal(0u, scene.Nodes[(uint)(10 + index)].Flags & 1u);
        }

        Assert.Contains((13u, 0f, 0f, 1u, 0u), scene.Attached);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.907에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        scene.WriteParticipant(1, 0, 1, "x");
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaLayout.Layout(100, scene.Globals(), new HudNsInfoMetaHooks()));
        var hooks = scene.Hooks();
        hooks.Root = null;
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaLayout.Layout(100, scene.Globals(), hooks));
        Assert.Throws<ArgumentNullException>(() => HudNsInfoMetaLayout.Layout(100, null!, scene.Hooks()));
    }
}
