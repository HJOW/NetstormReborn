// 0x67 메타 전투 옵션의 원문 복사·파생값·UI 분기를 대조한다.
// cpppj/src/client/NativeMetaBattleOptions.cpp 의 ReceiveNativeMetaBattleOptions와
// 0x67 수신 결선·공급 범위·TYPE 비용·환불 비율·재생성을 대조한다.
// 0.959에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x67 메타 전투 옵션의 실제 CS·게임·표면·음향 호출을 기록한다.
public sealed class ClientMetaBattleOptionsTests
{
    /// <summary>같은 실제 소유자와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 장기 게임 전역 owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();

        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 표면 저장소다.</summary>
        public readonly ClientSurfaceGlobals Surface;

        /// <summary>같은 실제 로컬 상태 owner다.</summary>
        public readonly ClientLocalStateGlobals Local;

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 로그 호출이다.</summary>
        public readonly List<string> Logs = [];

        /// <summary>실제 음향 횟수다.</summary>
        public int GuySounds;

        /// <summary>실제 NSInfo 갱신 횟수다.</summary>
        public int Refreshes;

        /// <summary>실제 재생성 횟수다.</summary>
        public int Reconnects;

        /// <summary>같은 실제 소유자 묶음이다.</summary>
        public Scene()
        {
            Challenge.Prepare();
            Surface = new(Game, new(new ClientLocalStateGlobals(Game, new AmbientSoundState())), new Kernel(), () => 0);
            Local = new(Game, new AmbientSoundState());
            Storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
        }

        /// <summary>같은 실제 표면 저장소다.</summary>
        public RawSurfaceStorage Storage => Surface.Storage;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaBattleOptionsHooks Hooks() => new()
        {
            Game = Game,
            Challenge = Challenge,
            Storage = Surface.Storage,
            Local = Local,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Log = text => Logs.Add(text),
            GuySound = () => GuySounds++,
            RefreshInfo = () => Refreshes++,
            Reconnect = () => Reconnects++,
        };

        /// <summary>같은 실제 기본 장치 묶음이다.</summary>
        public ClientNativeMetaBattleOptionsHooks NativeHooks(Func<uint>? owner = null) => new()
        {
            Owner = owner ?? (() => Game.Player.BattleOwner),
            Log = text => Logs.Add(text),
            GuySound = () => GuySounds++,
            RefreshInfo = () => Refreshes++,
        };

        /// <summary>원문 0x67 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(uint battle, ushort count, byte[]? options = null)
        {
            byte[] packet = new byte[Math.Max(32, 7 + count)];
            packet[1] = 0x67;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), battle);
            packet[4] = (byte)battle;
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(5, 2), count);
            if (options is not null)
            {
                options.CopyTo(packet.AsSpan(7));
            }

            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientNativeMetaBattleOptions.Receive(null!, new byte[16], scene.NativeHooks()));
        Assert.Throws<ArgumentNullException>(() => ClientNativeMetaBattleOptions.Receive(scene.Challenge, new byte[16], null!));
        var bare = new ClientNativeMetaBattleOptionsHooks();
        Assert.Throws<InvalidOperationException>(() => ClientNativeMetaBattleOptions.Receive(scene.Challenge, new byte[16], bare));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleOptions.Receive(null!, 0, new byte[16]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleOptions.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleOptions.Install(Dispatch(), null!));
    }

    /// <summary>원문 빈 observer·버전·가변 길이와 고정 F를 설치한다. 기존 관찰기는 유지한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        int observations = 0;
        Action<Memory<byte>, uint> stub = (_, _) => observations++;
        dispatch.ObservePacket(0x67, stub);
        ClientMetaBattleOptions.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x67);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(ushort.MaxValue, row.Length);
        Assert.Same(stub, row.PacketObserver);
        Assert.Null(row.Observer);
        Assert.Equal(0x004b4900u, dispatch.SnapshotReceiver(0x67)!.NativeEntry);
        Assert.Equal(0, observations);
    }

    /// <summary>무효 전투는 로그 뒤 반환한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void NativeLogsInvalidBattle()
    {
        var scene = new Scene();
        ClientNativeMetaBattleOptions.Receive(scene.Challenge, Scene.Packet(2, 0), scene.NativeHooks());
        Assert.Single(scene.Logs);
        Assert.Equal(0, scene.GuySounds);
        Assert.Equal(0, scene.Refreshes);
    }

    /// <summary>160 초과 길이는 로그 뒤 반환한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void NativeLogsLongData()
    {
        var scene = new Scene();
        scene.Challenge.Battle(2).Span[0] = 1;
        ClientNativeMetaBattleOptions.Receive(scene.Challenge, Scene.Packet(2, 200), scene.NativeHooks());
        Assert.Single(scene.Logs);
        Assert.Equal(0, scene.GuySounds);
    }

    /// <summary>자기 master 전투는 음향·복사·NSInfo를 모두 생략한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void NativeSkipsSelfMaster()
    {
        var scene = new Scene();
        scene.Challenge.Battle(2).Span[0] = 1;
        scene.Game.Player.BattleOwner = 7;
        Memory<byte> own = scene.Challenge.Participant(7);
        BinaryPrimitives.WriteUInt32LittleEndian(own.Span.Slice(0xa4, 4), 2);
        own.Span[0x64] = 4;
        ClientNativeMetaBattleOptions.Receive(scene.Challenge, Scene.Packet(2, 8, [1, 2, 3, 4, 5, 6, 7, 8]), scene.NativeHooks(() => 7));
        Assert.Empty(scene.Logs);
        Assert.Equal(0, scene.GuySounds);
        Assert.Equal(0, scene.Refreshes);
    }

    /// <summary>복사는 현재 길이를 쓰고 조건부로 NSInfo를 갱신한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void NativeCopiesOptionsAndRefreshes()
    {
        var scene = new Scene();
        scene.Challenge.Battle(2).Span[0] = 1;
        scene.Game.Player.BattleOwner = 7;
        Memory<byte> own = scene.Challenge.Participant(7);
        BinaryPrimitives.WriteUInt32LittleEndian(own.Span.Slice(0xa4, 4), 2);
        ClientNativeMetaBattleOptions.Receive(scene.Challenge, Scene.Packet(2, 8, [1, 2, 3, 4, 5, 6, 7, 8]), scene.NativeHooks(() => 7));
        Assert.Equal(1, scene.GuySounds);
        Assert.Equal([1, 2, 3, 4, 5, 6, 7, 8], scene.Challenge.Battle(2).Span.Slice(0x59, 8).ToArray());
        Assert.Equal(8u, scene.Challenge.Battle(2).Span[0x58]);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>소유자 불일치는 설치 수신까지 거부한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void FullRejectsOwnerMismatch()
    {
        var scene = new Scene();
        var other = new ClientGameplayGlobals();
        var foreign = new ClientLocalStateGlobals(other, new AmbientSoundState());
        var hooks = scene.Hooks();
        hooks.Local = foreign;
        Assert.Throws<ArgumentException>(() => ClientMetaBattleOptions.Receive(hooks, 0, new byte[32]));
    }

    /// <summary>전체 수신은 옵션 복사·파생값·재생성까지 잇는다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void FullReceiveCopiesOptionsAndDerivesValues()
    {
        var scene = new Scene();
        scene.Challenge.Battle(1).Span[0] = 1;
        scene.Game.Player.BattleOwner = 3;
        Memory<byte> own = scene.Challenge.Participant(3);
        BinaryPrimitives.WriteUInt32LittleEndian(own.Span.Slice(0xa4, 4), 0);
        scene.Storage.OwnerMode.Battle = true;
        scene.Game.World.MissionPost = false;
        scene.Game.SavedGameBox.Value = true;
        byte[] options = [9, 9, 9, 9, 1, 9, 9, 9];
        ClientMetaBattleOptions.Receive(scene.Hooks(), 5, Scene.Packet(1, 8, options));
        Assert.Equal(1, scene.GuySounds);
        Assert.Equal([9, 9, 9, 9, 1, 9, 9, 9], scene.Game.BattleOptions.AsSpan(0, 8).ToArray());
        Assert.Equal(22f, scene.Game.ManaRadius);
        Assert.Equal(484f, scene.Game.ManaRadiusSquared);
        Assert.Equal(1000f, scene.Surface.Storage.Types[122].Cost);
        Assert.Equal(0u, scene.Game.GameAux.RefundPercentBox.Value);
        Assert.Equal(1, scene.Reconnects);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Challenge.Battle(1).Span[0] = 1;
        var dispatch = Dispatch();
        ClientMetaBattleOptions.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x67, 9, null, Scene.Packet(1, 0));
        Assert.Equal(1, scene.GuySounds);
    }

    /// <summary>짧은 패킷·잘못된 전투는 원문 필드·기록 범위에서 거부한다. 0.959에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsAndBadBattlesThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientNativeMetaBattleOptions.Receive(scene.Challenge, new byte[4], scene.NativeHooks()));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientNativeMetaBattleOptions.Receive(scene.Challenge, Scene.Packet(10, 0), scene.NativeHooks()));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleOptions.Receive(scene.Hooks(), 0, new byte[5]));
    }
}
