// PlayerInfo의 전체 입력을 같은 파생 노드/CS·87BYTE 옵션과 실제 메뉴·송신에 연결한다.
// cpppj/src/client/NativePlayerInfoInput.cpp 의 HandleNativePlayerInfo(0047a900) 를 이식한다.
// 0.913에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 노드/CS/메뉴/송신 호출을 기록하는 플레이어 입력의 계약이다.</summary>
public sealed class NativePlayerInfoInputTests
{
    /// <summary>같은 실제 노드/CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 0xbc PlayerInfo 노드다.</summary>
        public HudPlayerInfoNode Node { get; } = new() { Player = 3, Button = 11, Start = 12, Text = 13 };

        /// <summary>같은 실제 CS 참가자 기록이다.</summary>
        public Dictionary<uint, byte[]> Participants { get; } = [];

        /// <summary>같은 실제 CS 전투 기록이다.</summary>
        public Dictionary<uint, byte[]> Battles { get; } = [];

        /// <summary>같은 자식 serial 표다.</summary>
        public Dictionary<uint, uint> Serials { get; } = [];

        /// <summary>실제 버튼 DWORD 값이다.</summary>
        public uint ButtonValue { get; set; } = 1;

        /// <summary>현재 540c70 소유자다.</summary>
        public uint Owner { get; set; } = 3;

        /// <summary>현재 540c74 구독자다.</summary>
        public uint Subscriber { get; set; } = 0x77;

        /// <summary>실제 송신한 상태 패킷이다.</summary>
        public List<byte[]> Packets { get; } = [];

        /// <summary>실제 송신한 옵션이다.</summary>
        public List<(uint Battle, byte[] Options, uint Kind)> DataCalls { get; } = [];

        /// <summary>실제 쓴 보조 값이다.</summary>
        public List<uint> Auxiliaries { get; } = [];

        /// <summary>실제 메뉴 제목이다.</summary>
        public List<string> Titles { get; } = [];

        /// <summary>실제 메뉴 항목이다.</summary>
        public List<(string Text, uint Command, uint Parameter)> Items { get; } = [];

        /// <summary>실제 메뉴 생성/부착 횟수다.</summary>
        public int Menus { get; private set; }

        /// <summary>실제 참가자 기록을 준비한다.</summary>
        public byte[] Record(uint player, uint battle = 2, uint flags = 0, uint slot = 1, string name = "bob")
        {
            byte[] bytes = new byte[0x100];
            BitConverter.GetBytes(Subscriber).CopyTo(bytes, 4);
            BitConverter.GetBytes(flags).CopyTo(bytes, 0x64);
            BitConverter.GetBytes(slot).CopyTo(bytes, 0xa8);
            BitConverter.GetBytes(battle).CopyTo(bytes, 0xa4);
            byte[] nameBytes = System.Text.Encoding.Latin1.GetBytes(name + "\0");
            nameBytes.CopyTo(bytes, 0x26);
            Participants[player] = bytes;
            return bytes;
        }

        /// <summary>실제 전투 옵션 기록을 준비한다.</summary>
        public byte[] Options(uint battle)
        {
            byte[] bytes = new byte[0x59];
            Battles[battle] = bytes;
            return bytes;
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public Scene()
        {
            Serials[11] = 0xA1;
            Serials[12] = 0xA2;
            Serials[13] = 0xA3;
            Record(3);
            Options(2);
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativePlayerInfoInputHooks Hooks() => new()
        {
            Command = address => address switch { 0x5427a8 => 0x10u, 0x542790 => 0x20u, 0x54278c => 0x30u, 0x5429dc => 0x40u, _ => 0u },
            Serial = child => Serials.TryGetValue(child, out uint serial) ? serial : throw new InvalidOperationException("serial 누락"),
            ButtonValue = _ => ButtonValue,
            Owner = () => Owner,
            Subscriber = () => Subscriber,
            Participant = index => Participants.TryGetValue(index, out byte[]? bytes) ? bytes : throw new InvalidOperationException("참가자 누락"),
            Battle = index => Battles.TryGetValue(index, out byte[]? bytes) ? bytes : throw new InvalidOperationException("전투 누락"),
            Auxiliary = value => Auxiliaries.Add(value),
            Status = fill => { byte[] packet = new byte[12]; fill(packet); Packets.Add(packet); },
            Data = (battle, options, kind) => DataCalls.Add((battle, options.ToArray(), kind)),
            Translate = text => text,
            BeginMenu = () => Menus++,
            AttachMenu = () => Menus++,
            Title = text => Titles.Add(text),
            Item = (text, command, parameter) => Items.Add((text, command, parameter)),
        };
    }

    /// <summary>준비 버튼은 CS 플래그를 쓰고 1을 반환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ReadyTogglesFlagAndSends()
    {
        var scene = new Scene();
        uint result = NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA1, 0, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(2u, BitConverter.ToUInt32(scene.Participants[3], 0x64));
        Assert.Single(scene.Packets);
        Assert.Equal(0x77u, BitConverter.ToUInt32(scene.Packets[0], 4));
        Assert.Equal(2u, BitConverter.ToUInt32(scene.Packets[0], 8));
    }

    /// <summary>버튼 0은 CS 플래그를 끈다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void ReadyClearsFlagWhenButtonZero()
    {
        var scene = new Scene();
        scene.ButtonValue = 0;
        BitConverter.GetBytes(2u).CopyTo(scene.Participants[3], 0x64);
        uint result = NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA1, 0, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(0u, BitConverter.ToUInt32(scene.Participants[3], 0x64));
    }

    /// <summary>시작 버튼은 최초 1회만 송신하고 예약한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void StartSendsOnceAndReserves()
    {
        var scene = new Scene();
        uint first = NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA2, 0, scene.Hooks());
        uint second = NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA2, 0, scene.Hooks());
        Assert.Equal(1u, first);
        Assert.Equal(1u, second);
        Assert.Equal(1u, scene.Node.Reserved);
        Assert.Single(scene.Packets);
        Assert.Equal(0x12u, BitConverter.ToUInt32(scene.Packets[0], 8));
    }

    /// <summary>소유자가 아니면 준비/시작 모두 0을 반환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void NonOwnerReturnsZero()
    {
        var scene = new Scene();
        scene.Owner = 9;
        Assert.Equal(0u, NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA1, 0, scene.Hooks()));
        Assert.Equal(0u, NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA2, 0, scene.Hooks()));
        Assert.Empty(scene.Packets);
        Assert.Equal(0u, scene.Node.Reserved);
    }

    /// <summary>좌행 랭크 감소는 옵션과 송신을 수행하고 1을 반환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void LeftTagRankDownChangesSlot()
    {
        var scene = new Scene();
        scene.Battles[2][0x4f + 1] = 10;
        uint result = NativePlayerInfoInput.Handle(scene.Node, 0x20, 0xA3, 3 + 300u, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(9, scene.Battles[2][0x4f + 1]);
        Assert.Single(scene.Packets);
        Assert.Single(scene.DataCalls);
        Assert.Equal((2u, 0x57u), (scene.DataCalls[0].Battle, scene.DataCalls[0].Kind));
        Assert.Equal(0x4000u | 0u, BitConverter.ToUInt32(scene.Packets[0], 8) & 0x4000u | 0u);
    }

    /// <summary>특수 전환은 소유자일 때 보조 값을 쓴다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void LeftTagSpecialWritesAuxiliaryForOwner()
    {
        var scene = new Scene();
        uint result = NativePlayerInfoInput.Handle(scene.Node, 0x20, 0xA3, 3 + 100u, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(1, scene.Battles[2][0x3f + 1]);
        Assert.Equal([1u], scene.Auxiliaries);
        Assert.Single(scene.DataCalls);
    }

    /// <summary>추방 확인은 메뉴를 열고 1을 반환한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void LeftTagKickoffOpensConfirmMenu()
    {
        var scene = new Scene();
        uint result = NativePlayerInfoInput.Handle(scene.Node, 0x20, 0xA3, 3 + 400u, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal(2, scene.Menus);
        Assert.Single(scene.Titles);
        Assert.Contains("bob", scene.Titles[0], StringComparison.Ordinal);
        Assert.Equal(2, scene.Items.Count);
        Assert.Equal((0x40u, 3u), (scene.Items[0].Command, scene.Items[0].Parameter));
        Assert.Equal((0x40u, 0u), (scene.Items[1].Command, scene.Items[1].Parameter));
        Assert.Empty(scene.Packets);
    }

    /// <summary>우행 색 증가/랭크 증가를 수행한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void RightTagColorUpAndRankUp()
    {
        var scene = new Scene();
        Assert.Equal(1u, NativePlayerInfoInput.Handle(scene.Node, 0x30, 0xA3, 3 + 200u, scene.Hooks()));
        Assert.Equal(1, scene.Battles[2][0x47 + 1]);
        Assert.Equal(1u, NativePlayerInfoInput.Handle(scene.Node, 0x30, 0xA3, 3 + 300u, scene.Hooks()));
        Assert.Equal(1, scene.Battles[2][0x4f + 1]);
        Assert.Equal(2, scene.Packets.Count);
    }

    /// <summary>처리하지 않은 입력은 0이며 미연결 장치는 명시 실패한다. 0.913에서 추가됐다.</summary>
    [Fact]
    public void UnhandledReturnsZeroAndMissingDevicesFail()
    {
        var scene = new Scene();
        Assert.Equal(0u, NativePlayerInfoInput.Handle(scene.Node, 0x50, 0, 0, scene.Hooks()));
        Assert.Throws<InvalidOperationException>(() => NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA1, 0, new NativePlayerInfoInputHooks()));
        Assert.Throws<ArgumentNullException>(() => NativePlayerInfoInput.Handle(null!, 0x10, 0xA1, 0, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => NativePlayerInfoInput.Handle(scene.Node, 0x10, 0xA1, 0, null!));
    }
}
