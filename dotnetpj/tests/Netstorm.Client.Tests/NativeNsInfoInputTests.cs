// NSInfo 루트의 원문 입력을 같은 정렬표/CS와 현재 메뉴·카메라·송신 장치에 연결한다.
// cpppj/src/client/NativeNsInfoInput.cpp 의 HandleNativeNsInfo(0047c8c0) 를 이식한다.
// 0.911에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using Netstorm.Client.Display;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 CS/메뉴/카메라/송신 호출을 기록하는 루트 입력의 계약이다.</summary>
public sealed class NativeNsInfoInputTests
{
    /// <summary>같은 실제 명령/정렬/CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 명령 DWORD다.</summary>
        public Dictionary<uint, uint> Commands { get; } = new()
        {
            [0x542790] = 0x100,
            [0x54278c] = 0x200,
            [0x5429dc] = 0x300,
        };

        /// <summary>같은 5930b0 정렬표다.</summary>
        public Dictionary<uint, uint> RosterTable { get; } = [];

        /// <summary>같은 실제 CS 참가자 기록이다.</summary>
        public Dictionary<uint, byte[]> Records { get; } = [];

        /// <summary>실제 읽은 명령 주소다.</summary>
        public List<uint> CommandCalls { get; } = [];

        /// <summary>실제 메뉴 호출이다.</summary>
        public List<(uint Index, ScreenPoint Point)> Menus { get; } = [];

        /// <summary>실제 중심 호출이다.</summary>
        public List<uint> Centers { get; } = [];

        /// <summary>실제 송신 패킷이다.</summary>
        public List<byte[]> Packets { get; } = [];

        /// <summary>실제 재생한 추방음 횟수다.</summary>
        public int Sounds { get; private set; }

        /// <summary>실제 기본 처리 호출이다.</summary>
        public List<(uint Event, uint First, uint Second)> Based { get; } = [];

        /// <summary>현재 원시 화면 좌표다.</summary>
        public ScreenPoint MousePoint { get; set; } = new(11, 22);

        /// <summary>실제 참가자 기록을 0x100 BYTE로 준비한다.</summary>
        public Memory<byte> Record(uint index, bool active, uint sid = 0, uint head = 0)
        {
            byte[] bytes = new byte[0x100];
            bytes[0x64] = active ? (byte)1 : (byte)0;
            BitConverter.GetBytes(sid).CopyTo(bytes, 0xb8);
            BitConverter.GetBytes(head).CopyTo(bytes, 4);
            Records[index] = bytes;
            return bytes;
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativeNsInfoInputHooks Hooks() => new()
        {
            Command = address => { CommandCalls.Add(address); return Commands.TryGetValue(address, out uint code) ? code : 0u; },
            Roster = second => RosterTable.TryGetValue(second, out uint index) ? index : throw new InvalidOperationException("정렬표 누락"),
            Participant = index => Records.TryGetValue(index, out byte[]? bytes) ? bytes : throw new InvalidOperationException("참가자 누락"),
            Mouse = () => MousePoint,
            Menu = (index, point) => Menus.Add((index, point)),
            Center = sid => Centers.Add(sid),
            Status = fill => { byte[] packet = new byte[12]; fill(packet); Packets.Add(packet); },
            KickoffSound = () => Sounds++,
            Base = (@event, first, second) => { Based.Add((@event, first, second)); return 7u; },
        };
    }

    /// <summary>활성 정렬행은 메뉴를 열고 1을 반환한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void ActiveRosterOpensMenu()
    {
        var scene = new Scene();
        scene.RosterTable[5] = 9;
        scene.Record(9, active: true);
        uint result = NativeNsInfoInput.Handle(0x100, 1, 5, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal([(9u, new ScreenPoint(11, 22))], scene.Menus);
        Assert.Empty(scene.Based);
    }

    /// <summary>비활성 정렬행은 기본 처리에 원래 인자를 전달한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void InactiveRosterFallsThroughToBase()
    {
        var scene = new Scene();
        scene.RosterTable[5] = 9;
        scene.Record(9, active: false);
        uint result = NativeNsInfoInput.Handle(0x200, 3, 5, scene.Hooks());
        Assert.Equal(7u, result);
        Assert.Empty(scene.Menus);
        Assert.Empty(scene.Centers);
        Assert.Equal([(0x200u, 3u, 5u)], scene.Based);
    }

    /// <summary>활성 중심행은 참가자 SID를 전달하고 1을 반환한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void ActiveCenterSendsSid()
    {
        var scene = new Scene();
        scene.RosterTable[6] = 4;
        scene.Record(4, active: true, sid: 0x1234);
        uint result = NativeNsInfoInput.Handle(0x200, 0, 6, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Equal([0x1234u], scene.Centers);
        Assert.Empty(scene.Based);
    }

    /// <summary>직접 추방은 기록 4번과 8을 패킷에 쓰고 소리를 재생한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void DirectKickFillsPacketAndPlaysSound()
    {
        var scene = new Scene();
        scene.Record(8, active: true, head: 0xAABBCCDD);
        uint result = NativeNsInfoInput.Handle(0x300, 0, 8, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Single(scene.Packets);
        Assert.Equal(0xAABBCCDDu, BitConverter.ToUInt32(scene.Packets[0], 4));
        Assert.Equal(8u, BitConverter.ToUInt32(scene.Packets[0], 8));
        Assert.Equal(1, scene.Sounds);
        Assert.Empty(scene.Based);
    }

    /// <summary>둘째가 0 이하면 효과를 선택하지 않고 기본 처리한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void NonPositiveSecondFallsThrough()
    {
        var scene = new Scene();
        uint result = NativeNsInfoInput.Handle(0x100, 9, 0, scene.Hooks());
        Assert.Equal(7u, result);
        Assert.Empty(scene.Menus);
        Assert.Equal([(0x100u, 9u, 0u)], scene.Based);
    }

    /// <summary>비교만 26비트로 마스킹한다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void CodeMasksTo26Bits()
    {
        var scene = new Scene();
        scene.RosterTable[5] = 9;
        scene.Record(9, active: true);
        uint result = NativeNsInfoInput.Handle(0x100 | 0xFC000000u, 0, 5, scene.Hooks());
        Assert.Equal(1u, result);
        Assert.Single(scene.Menus);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.911에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => NativeNsInfoInput.Handle(1, 2, 3, new NativeNsInfoInputHooks()));
        var hooks = scene.Hooks();
        hooks.Base = null;
        Assert.Throws<InvalidOperationException>(() => NativeNsInfoInput.Handle(0x999, 2, 3, hooks));
        var shortScene = new Scene();
        shortScene.RosterTable[5] = 9;
        shortScene.Records[9] = new byte[0x10];
        Assert.Throws<ArgumentOutOfRangeException>(() => NativeNsInfoInput.Handle(0x100, 0, 5, shortScene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => NativeNsInfoInput.Handle(1, 2, 3, null!));
    }
}
