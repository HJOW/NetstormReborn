// 기존 일반 부착/관문 뒤 전체 메타 본문을 검사한다.
// cpppj/src/client/HudNsInfoMeta.cpp 의 LayoutMetaPlayerInfo(00479fb0) 를 이식한다.
// 0.909에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 CS/노드와 호출을 기록하는 플레이어 패널 본문의 계약이다.</summary>
public sealed class HudNsInfoMetaPlayerTests
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

        /// <summary>실제 0xbc PlayerInfo 노드다.</summary>
        public HudPlayerInfoNode Panel { get; } = new() { Text = 101, Master = 102, Button = 103, Start = 104 };

        /// <summary>실제 특수 버튼 노드다.</summary>
        public HudPlayerButtonNode ReadyButton { get; } = new();

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

        /// <summary>실제 정렬행이다.</summary>
        public List<(uint Token, string Text, uint[] Context, uint Row)> Appended { get; } = [];

        /// <summary>실제 기본 문맥 추가 호출이다.</summary>
        public List<(uint Token, string Text)> AppendedDefault { get; } = [];

        /// <summary>실제 맞춘 내용이다.</summary>
        public List<uint> Fitted { get; } = [];

        /// <summary>실제 요청한 상단이다.</summary>
        public List<uint> RequestedTop { get; } = [];

        /// <summary>실제 편집 플래그 상자다.</summary>
        public Dictionary<uint, UIntBox> Flags { get; } = [];

        /// <summary>실제 단추 활성화다.</summary>
        public List<(uint Token, bool Value)> EnabledCalls { get; } = [];

        /// <summary>실제 단추 값이다.</summary>
        public List<(uint Token, uint Value)> Valued { get; } = [];

        /// <summary>실제 읽은 주소다.</summary>
        public List<uint> ReadCalls { get; } = [];

        /// <summary>현재 팔레트색/metadataMode 읽기 값이다.</summary>
        public Dictionary<uint, uint> Reads { get; } = new()
        {
            [0x5bec2cu] = 0x31u,
            [0x59afd8u] = 0x32u,
            [0x5bec3cu] = 0x33u,
            [0x5b5e58u] = 0x60u,
        };

        /// <summary>같은 실제 장치 묶음이다.</summary>
        public Scene()
        {
            Challenge.Prepare();
            Panel.BaseWidth = 400;
            Panel.BaseHeight = 60;
            Panel.Scale = 1;
            Nodes[101] = new GumpCreateNode { BaseWidth = 380, BaseHeight = 40, Scale = 1 };
            Nodes[102] = new GumpCreateNode { BaseWidth = 60, BaseHeight = 12, Scale = 1 };
            Nodes[103] = ReadyButton;
            ReadyButton.BaseWidth = 50;
            ReadyButton.BaseHeight = 20;
            ReadyButton.Scale = 1;
            Nodes[104] = new GumpCreateNode { BaseWidth = 80, BaseHeight = 20, Scale = 1 };
            Nodes[10] = Panel;
            for (uint slot = 0; slot < 8; slot++)
            {
                Reads[0x5c78b0u + slot * 4u] = 0x40u + slot;
                Reads[0x5accfcu + slot * 4u] = 0x50u + slot;
            }
        }

        /// <summary>같은 실제 전역 묶음이다.</summary>
        public HudNsInfoMetaGlobals Globals() => new() { Challenge = Challenge, Info = Info, Metamap = () => Metamap, Known = () => Known, Owner = () => Owner };

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public HudNsInfoMetaHooks Hooks(ReadOnlyMemory<byte>? options = null) => new()
        {
            Node = token => Nodes.TryGetValue(token, out GumpCreateNode? node) ? node : null,
            Player = _ => Panel,
            Push = token => Pushed.Add(token),
            FitHeight = token => Fitted.Add(token),
            Attach = (token, x, y, parent, mode) => Attached.Add((token, x, y, parent, mode)),
            Replace = (token, text) => Replaced.Add((token, text)),
            Parse = (token, text) => Parsed.Add((token, text)),
            Read = address => { ReadCalls.Add(address); return Reads.TryGetValue(address, out uint value) ? value : 0u; },
            TextFlags = token => Flags.TryGetValue(token, out UIntBox? box) ? box : throw new InvalidOperationException("플래그 누락"),
            ReadyButton = _ => ReadyButton,
            Enabled = (token, value) => EnabledCalls.Add((token, value)),
            Value = (token, value) => Valued.Add((token, value)),
            RequestTop = token => RequestedTop.Add(token),
            FitContent = _ => { },
            Append = (token, text, context, row) => Appended.Add((token, text, context.ToArray(), row)),
            AppendDefault = (token, text) => AppendedDefault.Add((token, text)),
            SlotRank = (_, slot) => slot * 2,
            RankPicture = rank => $"R{rank}",
        };

        /// <summary>참가자 기록을 실제 필드에 쓴다.</summary>
        public void WriteParticipant(uint index, uint battle, uint flags, string name = "x", uint slot = 0)
        {
            var record = Challenge.Participant(index).Span;
            record.Clear();
            Encoding.Latin1.GetBytes(name, record[0x26..]);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0xb8..], 0);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0xa4..], battle);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0x64..], flags);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0xa8..], slot);
        }

        /// <summary>전투 슬롯에 실제 참가자를 둔다.</summary>
        public void WriteBattleSlot(uint battle, uint slot, uint participant)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Battle(battle).Span[(0x38 + (int)slot * 4)..], participant);
        }
    }

    /// <summary>빈 슬롯은 이름 없이 비활성으로 둔다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void EmptySlotShowsPlaceholderWithoutName()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.Panel.Player = 0;
        HudNsInfoMetaPlayer.Layout(10, scene.Globals(), scene.Hooks());
        Assert.Contains((101u, "Slot empty\n"), scene.Appended.Select(row => (row.Token, row.Text)));
        Assert.Contains((103u, false), scene.EnabledCalls);
        Assert.Contains((103u, 0u), scene.Valued);
        Assert.DoesNotContain(scene.Parsed, row => row.Token == 101u && row.Text.Contains("~[C.", StringComparison.Ordinal));
        Assert.Contains((101u, "\n"), scene.AppendedDefault);
        Assert.Equal(101u, scene.Pushed[0]);
    }

    /// <summary>마스터는 미준비자가 있으면 시작 버튼을 끈다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void MasterDisablesStartWhenUnready()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.WriteParticipant(1, 2, 1 | 4, "me", slot: 3);
        scene.WriteParticipant(2, 2, 1 | 4 | 2, "ok", slot: 3);
        scene.WriteParticipant(3, 2, 1, "notready", slot: 3);
        scene.WriteBattleSlot(2, 0, 2);
        scene.WriteBattleSlot(2, 1, 3);
        scene.Panel.Player = 1;
        HudNsInfoMetaPlayer.Layout(10, scene.Globals(), scene.Hooks());
        Assert.Equal(1u, scene.Panel.Player);
        Assert.Contains((104u, false), scene.EnabledCalls);
        Assert.Equal(1u, scene.Panel.Player);
        Assert.Equal((uint)0x10, scene.Flags[101].Value & 0x10u);
        Assert.Equal(1u, scene.Panel.Player);
    }

    /// <summary>모두 준비되면 시작 버튼을 켜고 metadata를 둔다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void MasterEnablesStartWhenAllReady()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.WriteParticipant(1, 2, 1 | 4 | 2 | 0x2000, "me", slot: 3);
        scene.WriteParticipant(2, 2, 1 | 4 | 2 | 0x2000, "ok", slot: 3);
        scene.WriteBattleSlot(2, 0, 2);
        scene.Panel.Player = 1;
        HudNsInfoMetaPlayer.Layout(10, scene.Globals(), scene.Hooks());
        Assert.Contains((104u, true), scene.EnabledCalls);
        Assert.Equal(1u, scene.ReadyButton.Entry);
        Assert.Contains(scene.Appended, row => row.Token == 101u && row.Row == 1u);
    }

    /// <summary>마스터가 아니면 시작 버튼 없이 마스터 표시를 둔다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void NonMasterShowsMasterMark()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.WriteParticipant(1, 2, 1 | 4, "me", slot: 3);
        scene.WriteParticipant(2, 2, 1, "boss", slot: 3);
        scene.WriteBattleSlot(2, 0, 2);
        scene.Panel.Player = 1;
        scene.Owner = 2;
        HudNsInfoMetaPlayer.Layout(10, scene.Globals(), scene.Hooks());
        Assert.Equal(0u, scene.Flags[101].Value & 0x10u);
        Assert.DoesNotContain((104u, true), scene.EnabledCalls);
        Assert.NotEqual(0u, scene.Nodes[102].Flags & 1u);
        Assert.Equal(0u, scene.Nodes[104].Flags & 1u);
        Assert.Contains(scene.Appended, row => row.Token == 101u && row.Text == "me" && row.Context[2] == 0x33u && row.Row == 0);
    }

    /// <summary>특수 항목은 L10 한 줄만 파싱한다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void SpecialSkipsExtraIcons()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.WriteParticipant(1, 0, 1, "x", slot: 3);
        scene.Challenge.Battle(0).Span[0x3f + 3] = 1;
        scene.Panel.Player = 1;
        var hooks = scene.Hooks();
        HudNsInfoMetaPlayer.Layout(10, scene.Globals(), hooks);
        Assert.Contains(scene.Parsed, row => row.Text == "~[IfortGump.L10]");
        Assert.DoesNotContain(scene.Parsed, row => row.Text.StartsWith("~[IfortGump.L9]", StringComparison.Ordinal) || row.Text.StartsWith("~[T201]", StringComparison.Ordinal));
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.909에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        scene.Flags[101] = new UIntBox();
        scene.Panel.Player = 0;
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaPlayer.Layout(10, scene.Globals(), new HudNsInfoMetaHooks()));
        var hooks = scene.Hooks();
        hooks.TextFlags = null;
        scene.Panel.Player = 1;
        scene.WriteParticipant(1, 0, 1, "x");
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaPlayer.Layout(10, scene.Globals(), hooks));
        Assert.Throws<ArgumentNullException>(() => HudNsInfoMetaPlayer.Layout(10, null!, scene.Hooks()));
    }
}
